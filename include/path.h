#ifndef PATH_H
#define PATH_H

#include <windows.h>

int concat_paths(PCWSTR leftPath, PCWSTR rightPath, PWSTR* fullPathOut);
int get_logindata_path(PWSTR* appdataPathOut);
int get_localstate_path(PWSTR* localStatePathOut);

#endif
