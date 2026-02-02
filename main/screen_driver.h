#pragma once
#include <stdint.h>
#include <driver/gpio.h>
#include <driver/uart.h>
#include <string>    // <--- 必须加上这一行！！！
#include "utf8togb2312.h"

// ================= 配置区域 =================
// 在这里修改引脚和波特率，方便统一管理
#define TEST_UART_PORT_NUM      (UART_NUM_1)
#define TEST_UART_TXD           (GPIO_NUM_9)   // 发送引脚
#define TEST_UART_RXD           (GPIO_NUM_10)  // 接收引脚
#define TEST_UART_BAUD_RATE     (115200)
#define BUF_SIZE                (1024)

// 定义眼睛的状态枚举
enum class EyeState {
    OPEN,       // 睁开（默认/空闲）
    CLOSE,      // 闭上（MCP控制）
    LISTENING,  // 聆听中（动画）
    THINKING,   // 思考中（动画）
    SPEAKING    // 说话中（动画）
};

class ScreenDriver {
public:
    static ScreenDriver& GetInstance() {
        static ScreenDriver instance;
        return instance;
    }

    void Init();
    void SetEyeState(EyeState state);
    void ForceOpenEye();
    void ForceCloseEye();

    // 在类里增加一个函数声明 -- 发送文本字幕
    void SendSubtitle(const std::string& text);

        void SendCommand(const char* cmd); // 假设你的屏幕是用字符串指令

private:
    ScreenDriver() = default;

    bool force_closed_ = false;
};