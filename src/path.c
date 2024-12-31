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
PWSTR get_localappdata_path() {
  PWSTR appdataPath = NULL;
  HRESULT hr =
      SHGetKnownFolderPath(&FOLDERID_LocalAppData, 0, NULL, &appdataPath);
  if (FAILED(hr)) {
    fprintf(stderr, "Failed to retrieve APPDATA path. Error code: %08lX\n", hr);
    return NULL;
  }
  return appdataPath;
}

// Returns the concatenation of `leftPath` and `rightPath`
PWSTR concat_paths(PCWSTR leftPath, PCWSTR rightPath) {
  size_t totalLength = wcslen(leftPath) + wcslen(rightPath) + 1;
  PWSTR fullPath = (PWSTR)malloc(totalLength * sizeof(wchar_t));
  if (!fullPath) {
    fprintf(stderr, "malloc errored out");
  }

  wcscpy(fullPath, leftPath);
  wcscat(fullPath, rightPath);
  return fullPath;
}
