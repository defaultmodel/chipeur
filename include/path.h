#include <windows.h>

int get_localappdata_path(PWSTR* appdataPathOut);
int concat_paths(PCWSTR leftPath, PCWSTR rightPath, PWSTR* fullPathOut);
