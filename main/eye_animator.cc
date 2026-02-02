#include "eye_animator.h"
#include <esp_log.h>
#include <cstdlib> // for rand()
#include <ctime>

#define TAG "ANIM"

// === 原有的全幅眨眼序列 (用于精神状态) ===
static const std::vector<std::vector<int>> ACTIVE_BLINK_SEQS = {
    {6, 7, 8, 9, 10, 11},
    {20, 21, 22, 23, 24, 25},
    {31, 32, 33, 34, 35, 36}
};

// === 新增：慵懒眨眼序列 (仅在空闲时使用) ===
// 逻辑：半睁(45) -> 微闭(44) -> 闭(42) -> 全闭(43) -> 闭(42) -> 微闭(44) -> 半睁(45)
// 这样形成一个完整的闭合循环
static const std::vector<int> LAZY_BLINK_SEQ = {45, 44, 42, 43, 42, 44, 45};

// === 慵懒微动池 (平时停留的帧) ===
// 你说 44/45 是平替 32/33，这里假设 42/43 是稍微睁大一点的半睁
// 如果平时主要停留在半睁，可以用 42, 43
static const std::vector<int> LAZY_IDLE_FRAMES = {44, 45}; 

// === 精神微动池 (保持不变，剔除掉 42-45) ===
static const std::vector<int> ACTIVE_IDLE_FRAMES = {
    0, 1, 2, 3, 4, 5, 12, 13, 14, 15, 16, 17, 18, 19, 
    26, 27, 28, 29, 30, 37, 38, 39, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59
};

// 辅助：生成范围随机数
int RandomInt(int min, int max) {
    return min + (rand() % (max - min + 1));
}

void EyeAnimator::Start() {
    // 简单的随机种子初始化
    srand(xTaskGetTickCount()); 
    xTaskCreate(AnimationTask, "EyeAnim", 4096, this, 5, &task_handle_);
}

void EyeAnimator::SetState(EyeState state) {
    // 检测状态跳变：从非聆听状态 -> 变成聆听状态
    if (current_state_ == EyeState::OPEN && state == EyeState::LISTENING) {
        // 在这里插入“唤醒反应”
        // 我们可以在下一次 Task 循环中处理，或者直接在这里创建一个一次性任务
        // 最简单的方法是设置一个标志位，让主循环优先处理
        play_wake_up_anim_ = true; 
    }
    
    current_state_ = state;
}

void EyeAnimator::SetForceClose(bool force) {
    force_closed_ = force;
}

void EyeAnimator::PlayFrame(int image_id) {
    // === 地址映射逻辑 ===
    // 基地址：2212352
    // 偏移量：80000
    uint32_t address = 2212352 + (image_id * 80000);

    // 拼装指令
    // 注意这里用了 %lu (unsigned long)，对应 uint32_t
    char buf[64];
    snprintf(buf, sizeof(buf), "FSIMG(%lu,20,20,200,200,0);\r\n", address);
    
    // 发送指令
    ScreenDriver::GetInstance().SendCommand(buf);
}

// 修改函数签名，加个参数，或者在内部判断 current_state_
void EyeAnimator::PlayBlink() {
    if (current_state_ == EyeState::OPEN) {
        // === 播放慵懒眨眼 ===
        // 速度可以稍微慢一点，像是在打瞌睡
        for (int img_id : LAZY_BLINK_SEQ) {
            PlayFrame(img_id);
            vTaskDelay(pdMS_TO_TICKS(200)); // 60ms 一帧，慢悠悠的
        }
    } else {
        // === 播放精神眨眼 (原有逻辑) ===
        int seq_idx = RandomInt(0, ACTIVE_BLINK_SEQS.size() - 1);
        const auto& seq = ACTIVE_BLINK_SEQS[seq_idx];
        for (int img_id : seq) {
            PlayFrame(img_id);
            vTaskDelay(pdMS_TO_TICKS(35)); // 35ms 一帧，快速利落
        }
    }
}

// === 核心动画逻辑任务 ===
void EyeAnimator::AnimationTask(void* arg) {
    EyeAnimator* self = (EyeAnimator*)arg;
    
    // 上次眨眼的时间
    TickType_t last_blink_time = xTaskGetTickCount();
    // 下次眨眼的间隔 (初始化为 3秒)
    int next_blink_interval = 3000; 

    while (true) {
        // === 1. 处理强制闭眼逻辑 ===
        // 如果被强制闭眼了，就显示闭眼图，然后挂起等待，不跑后面的随机动画
        if (self->force_closed_ || self->current_state_ == EyeState::CLOSE) {
            // 假设第0张就是彻底闭眼的图（或者是眨眼序列中间的那张）
            // 这里假设你图片序列里 ID 0 是闭眼，或者用眨眼序列的第一组的第3帧
            self->PlayFrame(8); // <--- 请确认 ID 0 是不是闭眼图，如果不是请修改 ID
            
            vTaskDelay(pdMS_TO_TICKS(500)); // 每500ms检查一次是否解除了强制
            continue; // 跳过本次循环，不执行眨眼和微动
        }

        // === 新增：2. 处理唤醒动画 (快速眨眼两下) ===
        if (self->play_wake_up_anim_) {
            self->play_wake_up_anim_ = false; // 清除标志位
            
            // 快速眨眼两次
            self->PlayBlink(); 
            vTaskDelay(pdMS_TO_TICKS(100)); // 间隔一点点
            self->PlayBlink();
            
            // 播放完后，继续后面的正常逻辑
            vTaskDelay(pdMS_TO_TICKS(200));
        }

        // === 2. 检查是否该眨眼了 ===
        TickType_t now = xTaskGetTickCount();
        if (pdTICKS_TO_MS(now - last_blink_time) > next_blink_interval) {
            self->PlayBlink(); // <--- 这里的 PlayBlink 已经自动适配状态了
            last_blink_time = now;
            
            // 慵懒模式下，眨眼间隔可以更长，或者更随机
            if (self->current_state_ == EyeState::OPEN) {
                next_blink_interval = RandomInt(3000, 8000); // 偶尔眨一下
            } else {
                next_blink_interval = RandomInt(2000, 6000);
            }
        }

        // === 3. 非眨眼期间的微动 ===
        const std::vector<int>* current_pool;
        if (self->current_state_ == EyeState::OPEN) {
            current_pool = &LAZY_IDLE_FRAMES; // 用 42, 43
        } else {
            current_pool = &ACTIVE_IDLE_FRAMES; // 用精神的大眼图
        }

        int current_img_idx = (*current_pool)[RandomInt(0, current_pool->size() - 1)];
        self->PlayFrame(current_img_idx);

        int wait_time = 0;

        switch (self->current_state_) {
            case EyeState::LISTENING:
                // 聆听：眼神专注，变动极慢
                wait_time = RandomInt(800, 1500);
                break;

            case EyeState::SPEAKING:
                // 说话：眼神活跃，变动快
                wait_time = RandomInt(150, 400);
                break;

            case EyeState::THINKING:
                // 思考：极快跳动，模拟眼球快速转动检索信息
                wait_time = RandomInt(50, 150);
                break;

            case EyeState::OPEN:
                // 空闲：半睁眼，呼吸极慢，显得很放松
                wait_time = RandomInt(2000, 4000); // 2~4秒才动一下
                break;

            default:
                // 空闲：悠闲的节奏
                wait_time = RandomInt(500, 1200);
                break;
        }

        vTaskDelay(pdMS_TO_TICKS(wait_time));
    }
}