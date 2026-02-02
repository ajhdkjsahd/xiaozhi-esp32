#include "mcp_server.h"
#include "screen_driver.h" // 引入我们刚才写的驱动
#include <esp_log.h>

#define TAG_EYE "眼睛控制："

class Eye_Controller  { 
private:
    bool is_open_ = true; // 记录当前逻辑状态

public:
    Eye_Controller() {
        // 注册 MCP 工具
        auto& server = McpServer::GetInstance();
        
        // 1. 打开眼睛
        server.AddTool("眼睛.睁开", "把眼睛睁开/显示睁眼表情", 
                       PropertyList(), [this](const PropertyList&) {
                           is_open_ = true;
                           // 调用屏幕驱动
                           ScreenDriver::GetInstance().ForceOpenEye();
                           ESP_LOGW(TAG_EYE, "已睁开眼睛");
                           return "{\"状态\": \"眼睛已睁开\"}";
                       });

        // 2. 关闭眼睛
        server.AddTool("眼睛.闭上", "把眼睛闭上/显示闭眼表情", 
                       PropertyList(), [this](const PropertyList&) {
                           is_open_ = false;
                           // 调用屏幕驱动
                           ScreenDriver::GetInstance().ForceCloseEye();
                           ESP_LOGW(TAG_EYE, "已闭上眼睛");
                           return "{\"状态\": \"眼睛已闭上\"}";
                       });
    }
};