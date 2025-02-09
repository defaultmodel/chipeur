#ifndef PATH_H
#define PATH_H

#include <windows.h>

int concat_paths(PCWSTR leftPath, PCWSTR rightPath, PWSTR* fullPathOut);
int get_logindata_path(PCWSTR loginDataSubPath, PWSTR* loginDataPathOut);
int get_localstate_path(PCWSTR localStateSubPath, PWSTR* localStatePathOut);

#endif
