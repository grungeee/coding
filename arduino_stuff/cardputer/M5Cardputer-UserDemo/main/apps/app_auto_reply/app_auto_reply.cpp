/**
 * @file app_auto_reply.cpp
 * @brief ESP-NOW chat auto-reply app.
 */
#include "app_auto_reply.h"
#include "spdlog/spdlog.h"
#include "../app_chat/assets/chat_big.h"
#include "../app_chat/assets/chat_small.h"
#include "../utils/esp_now_wrap/esp_now_wrap.h"
#include <algorithm>
#include <cctype>

using namespace MOONCAKE::APPS;

#define _keyboard _data.hal->keyboard()
#define _canvas _data.hal->canvas()
#define _canvas_update _data.hal->canvas_update
#define _canvas_clear() _canvas->fillScreen(THEME_COLOR_BG)

static const char* AUTO_REPLY_PREFIX = "[auto]";

void AppAutoReply::_draw_header()
{
    _canvas->fillRect(0, 0, _canvas->width(), 32, THEME_COLOR_BG);
    _canvas->setCursor(0, 0);
    _canvas->setTextSize(1);
    _canvas->setTextColor(TFT_ORANGE, THEME_COLOR_BG);
    _canvas->print("ESP-NOW AUTO REPLY\n");
    _canvas->setTextColor(THEME_COLOR_REPL_TEXT, THEME_COLOR_BG);
    _canvas->print("Enter edits reply. Home exits.\n");
}

void AppAutoReply::_draw_editor()
{
    _update_cursor();

    _canvas->fillRect(0, 32, _canvas->width(), 22, THEME_COLOR_BG);
    _canvas->setCursor(0, 34);
    _canvas->setTextColor(TFT_GREENYELLOW, THEME_COLOR_BG);
    snprintf(
        _data.string_buffer,
        sizeof(_data.string_buffer),
        "Reply: %.34s%c",
        _data.reply_buffer.c_str(),
        _data.cursor_state ? '_' : ' '
    );
    _canvas->print(_data.string_buffer);
    _canvas->setTextColor(THEME_COLOR_REPL_TEXT, THEME_COLOR_BG);
    _canvas_update();
}

void AppAutoReply::_update_cursor()
{
    if ((millis() - _data.cursor_update_time_count) > _data.cursor_update_period)
    {
        _data.cursor_state = !_data.cursor_state;
        _data.cursor_update_time_count = millis();
    }
}

void AppAutoReply::_update_input()
{
    if (_keyboard->keyList().size() == _data.last_key_num)
        return;

    if (_keyboard->keyList().size() == 0)
    {
        _data.last_key_num = 0;
        return;
    }

    _keyboard->updateKeysState();

    if (_keyboard->keysState().space)
    {
        _data.reply_buffer += ' ';
    }
    else if (_keyboard->keysState().del)
    {
        if (!_data.reply_buffer.empty())
            _data.reply_buffer.pop_back();
    }
    else if (!_keyboard->keysState().enter)
    {
        for (auto& i : _keyboard->keysState().values)
            _data.reply_buffer += i;
    }

    if (_data.reply_buffer.empty())
        _data.reply_buffer = AUTO_REPLY_PREFIX;

    if (_data.reply_buffer.rfind(AUTO_REPLY_PREFIX, 0) != 0)
        _data.reply_buffer = std::string(AUTO_REPLY_PREFIX) + " " + _data.reply_buffer;

    _draw_editor();
    _data.last_key_num = _keyboard->keyList().size();
}

void AppAutoReply::_print_wrapped(const char* prefix, const std::string& message)
{
    std::string line = std::string(prefix) + message;
    const size_t chars_per_line = std::max(1, _canvas->width() / FONT_REPL_WIDTH);

    while (line.length() > chars_per_line)
    {
        _canvas->printf("%s\n", line.substr(0, chars_per_line).c_str());
        line = line.substr(chars_per_line);
    }

    _canvas->printf("%s\n", line.c_str());
}

bool AppAutoReply::_should_reply(const std::string& message)
{
    if (message.empty())
        return false;

    if (message.rfind(AUTO_REPLY_PREFIX, 0) == 0)
        return false;

    if ((millis() - _data.last_reply_time) < _data.reply_cooldown_ms)
        return false;

    return true;
}

void AppAutoReply::_update_receive()
{
    const int received_size = espnow_wrap_available();
    if (received_size <= 0)
        return;

    std::string incoming(
        reinterpret_cast<char*>(espnow_wrap_get_received()),
        static_cast<size_t>(received_size)
    );

    incoming.erase(
        std::remove_if(
            incoming.begin(),
            incoming.end(),
            [](unsigned char value) {
                return value != '\n' && value != '\r' && !std::isprint(value);
            }
        ),
        incoming.end()
    );

    _canvas->setCursor(0, 58);
    _canvas->setTextColor(THEME_COLOR_REPL_TEXT, THEME_COLOR_BG);
    _print_wrapped(">> ", incoming);

    if (_should_reply(incoming))
    {
        espnow_wrap_send(
            reinterpret_cast<uint8_t*>(const_cast<char*>(_data.reply_buffer.c_str())),
            _data.reply_buffer.size()
        );
        _data.last_reply_time = millis();

        _canvas->setTextColor(TFT_GREENYELLOW, THEME_COLOR_BG);
        _print_wrapped("<< ", _data.reply_buffer);
        _canvas->setTextColor(THEME_COLOR_REPL_TEXT, THEME_COLOR_BG);
    }

    _draw_editor();
    espnow_wrap_clear();
}

void AppAutoReply::onCreate()
{
    spdlog::info("{} onCreate", getAppName());
    _data.hal = mcAppGetDatabase()->Get("HAL")->value<HAL::Hal *>();
}

void AppAutoReply::onResume()
{
    spdlog::info("{} onResume", getAppName());

    ANIM_APP_OPEN();

    _canvas_clear();
    _canvas->setTextScroll(true);
    _canvas->setBaseColor(THEME_COLOR_BG);
    _canvas->setTextColor(THEME_COLOR_REPL_TEXT, THEME_COLOR_BG);
    _canvas->setFont(FONT_REPL);
    _canvas->setTextSize(FONT_SIZE_REPL);

    _draw_header();
    _draw_editor();

    if (!espnow_wrap_is_inited())
        espnow_wrap_init();
}

void AppAutoReply::onRunning()
{
    _update_input();
    _update_receive();
    _draw_editor();

    if (_data.hal->homeButton()->pressed())
    {
        _data.hal->playNextSound();
        spdlog::info("quit auto reply");
        destroyApp();
    }
}

std::string AppAutoReply_Packer::getAppName()
{
    return "AUTO REPLY";
}

void* AppAutoReply_Packer::getAppIcon()
{
    return (void*)(new AppIcon_t(image_data_chat_big, image_data_chat_small));
}

void* AppAutoReply_Packer::newApp()
{
    return new AppAutoReply;
}

void AppAutoReply_Packer::deleteApp(void *app)
{
    delete (AppAutoReply*)app;
}
