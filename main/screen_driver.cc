#include "screen_driver.h"
#include "esp_log.h"
#include <cstring>
#include <vector> // 引入 vector 以防栈溢出
#include "eye_animator.h"

#define TAG "SCREEN"

#define TEST_TASK_STACK_SIZE    (4096)


void ScreenDriver::Init() {

    // 这里填入你之前写的 UART 初始化代码

    // 1. 配置参数
    const uart_config_t uart_config = {
        .baud_rate = TEST_UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    // 2. 安装驱动 (注意设置 TX Buffer)
    ESP_ERROR_CHECK(uart_driver_install(TEST_UART_PORT_NUM, BUF_SIZE * 2, BUF_SIZE * 2, 0, NULL, 0));
    
    // 3. 应用配置
    ESP_ERROR_CHECK(uart_param_config(TEST_UART_PORT_NUM, &uart_config));

    // 4. 设置引脚
    ESP_ERROR_CHECK(uart_set_pin(TEST_UART_PORT_NUM, TEST_UART_TXD, TEST_UART_RXD, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    ESP_LOGI(TAG, "UART1 Initialized on TX:%d RX:%d", TEST_UART_TXD, TEST_UART_RXD);

    EyeAnimator::GetInstance().Start(); // <--- 启动动画引擎

    ESP_LOGI(TAG, "屏幕串口初始化完成");
}


// 修改 SetEyeState，只负责转发状态
void ScreenDriver::SetEyeState(EyeState state) {
    // 如果处于强制闭眼状态，忽略所有状态切换请求（除了 CLOSE）
    if (force_closed_ && state != EyeState::CLOSE) {
        return; 
    }
    // 转发给动画引擎
    EyeAnimator::GetInstance().SetState(state);
}

// 给 MCP 调用的专门接口
void ScreenDriver::ForceCloseEye() {
    force_closed_ = true;
    // 告诉动画引擎：别动了，闭眼！
    EyeAnimator::GetInstance().SetForceClose(true);
}

void ScreenDriver::ForceOpenEye() {
    force_closed_ = false;
    // 告诉动画引擎：解除封印，开始表演！
    EyeAnimator::GetInstance().SetForceClose(false);
    // 顺便重置状态为 OPEN
    EyeAnimator::GetInstance().SetState(EyeState::OPEN);
}

void ScreenDriver::SendCommand(const char* cmd) {
    if (cmd == nullptr) return;
    uart_write_bytes(TEST_UART_PORT_NUM, cmd, strlen(cmd));
    // 很多串口屏需要结束符，比如 0xFF 0xFF 0xFF，别忘了加
    // const char end_bytes[] = "\r\n"; 
    // uart_write_bytes(TEST_UART_PORT_NUM, end_bytes, 2);
}



void ScreenDriver::SendSubtitle(const std::string& text) {
    // 1. 定义大缓冲区 (1024字节足够容纳约500个汉字)
    // 使用 static 防止频繁申请内存，或者使用 vector 放在堆上防止撑爆任务栈
    static char gbk_buf[1024]; 
    static char final_cmd[1200]; // 指令缓冲区要比内容大一点

    // 2. 长度保护：防止源文本过长导致内存溢出
    // 我们先截断源文本，留一点余量给转码膨胀
    std::string safe_text = text;
    if (safe_text.length() > 800) {
        safe_text = safe_text.substr(0, 800) + "...";
    }

    // 3. 【关键步骤】先转码：UTF-8 内容 -> GBK 内容
    // 注意：这里只转码内容，不转码 "SET_TXT..." 这些指令头
    memset(gbk_buf, 0, sizeof(gbk_buf)); // 清空缓冲区
    UTF_8ToGB2312(gbk_buf, (char*)safe_text.c_str(), safe_text.length());

    // 4. 【关键步骤】清洗特殊字符 (防止指令格式被破坏)
    // 你的协议是 SET_TXT(1, '内容'); 使用了单引号包裹
    // 所以必须把内容里的 单引号(') 替换掉，否则会截断指令
    for (int i = 0; gbk_buf[i] != '\0'; i++) {
        if (gbk_buf[i] == '\'') {
            gbk_buf[i] = '\"'; // 把单引号强行改成双引号
        }
        // 处理换行符，防止屏幕无法识别
        if (gbk_buf[i] == '\n' || gbk_buf[i] == '\r') {
            gbk_buf[i] = ' ';  // 替换成空格
        }
    }

    // 5. 拼装最终指令
    // 格式：SET_TXT(1,'转换并清洗后的GBK内容');\r\n
    snprintf(final_cmd, sizeof(final_cmd), "SET_TXT(1,'%s');\r\n", gbk_buf);

    // 6. 发送
    SendCommand(final_cmd);
}