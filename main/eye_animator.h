#pragma once
#include <vector>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "screen_driver.h" // 引用你之前的驱动

class EyeAnimator {
public:
    static EyeAnimator& GetInstance() {
        static EyeAnimator instance;
        return instance;
    }

    void Start(); // 启动动画任务
    void SetState(EyeState state); // 设置当前情绪状态

    // === 新增：强制控制 ===
    void SetForceClose(bool force);

private:
    EyeAnimator() = default;
    static void AnimationTask(void* arg); // 任务函数

    void PlayFrame(int image_id);     // 发送指令给屏幕
    void PlayBlink();                 // 播放一次眨眼
    
    // 内部变量
    EyeState current_state_ = EyeState::OPEN;
    bool force_closed_ = false; // === 新增 ===
    TaskHandle_t task_handle_ = nullptr;
    bool play_wake_up_anim_ = false;
};