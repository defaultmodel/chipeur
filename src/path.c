// clang-format off
// knownfolders needs to be after shlobj
#include <shlobj.h>
#include <knownfolders.h>
// clang-format on

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

/// Windows doesn't perform any kind of translation when storing or reading file
/// names and they are stored as UTF-16. This is why we handle PWSTR's here

// Returns the Local AppData path gathered from environment variables
// NOTE This function uses `SHGetKnownFolderPath` thus the caller must call
// CoTaskMemFree on the returned PWSTR
int get_localappdata_path(PWSTR* appdataPathOut) {
  HRESULT hr =
      SHGetKnownFolderPath(&FOLDERID_LocalAppData, 0, NULL, appdataPathOut);
  if (FAILED(hr)) {
    fprintf(stderr, "Failed to retrieve APPDATA path. Error code: %08lX\n", hr);
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

// Returns the concatenation of `leftPath` and `rightPath` in `fullPathOut`
// NOTE `fullPathOut should be freed by the caller`
int concat_paths(PCWSTR leftPath, PCWSTR rightPath, PWSTR* fullPathOut) {
  size_t totalLength = wcslen(leftPath) + wcslen(rightPath) + 1;
  *fullPathOut = (PWSTR)malloc(totalLength * sizeof(wchar_t));
  if (!*fullPathOut) {
    fprintf(stderr, "Failed to allocate memory for concatenated path\n");
    return EXIT_FAILURE;
  }

  wcscpy(*fullPathOut, leftPath);
  wcscat(*fullPathOut, rightPath);

  return EXIT_SUCCESS;
}
