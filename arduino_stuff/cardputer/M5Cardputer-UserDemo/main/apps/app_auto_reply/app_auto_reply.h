/**
 * @file app_auto_reply.h
 * @brief ESP-NOW chat auto-reply app.
 */
#pragma once
#include <mooncake.h>
#include "../../hal/hal.h"
#include "../utils/anim/anim_define.h"
#include "../utils/icon/icon_define.h"
#include "../utils/theme/theme_define.h"

namespace MOONCAKE
{
    namespace APPS
    {
        class AppAutoReply : public APP_BASE
        {
            private:
                struct Data_t
                {
                    HAL::Hal* hal = nullptr;
                    int last_key_num = 0;
                    char string_buffer[160];
                    std::string reply_buffer = "[auto] Got your message";
                    uint32_t last_reply_time = 0;
                    uint32_t reply_cooldown_ms = 700;
                    uint32_t cursor_update_time_count = 0;
                    uint32_t cursor_update_period = 500;
                    bool cursor_state = false;
                };

                Data_t _data;

                void _draw_header();
                void _draw_editor();
                void _update_cursor();
                void _update_input();
                void _update_receive();
                void _print_wrapped(const char* prefix, const std::string& message);
                bool _should_reply(const std::string& message);

            public:
                void onCreate() override;
                void onResume() override;
                void onRunning() override;
        };

        class AppAutoReply_Packer : public APP_PACKER_BASE
        {
            std::string getAppName() override;
            void* getAppIcon() override;
            void* newApp() override;
            void deleteApp(void *app) override;
        };
    }
}
