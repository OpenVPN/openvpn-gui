#ifndef SAVEPASS_H
#define SAVEPASS_H

#include <wchar.h>

#define USER_PASS_LEN 128
#define KEY_PASS_LEN 128

int SaveKeyPass(const WCHAR *config_name, const WCHAR *password);
int SaveAuthPass(const WCHAR *config_name, const WCHAR *password);
int SaveUsername(const WCHAR *config_name, const WCHAR *username);

int RecallKeyPass(const WCHAR *config_name, WCHAR *password);
int RecallAuthPass(const WCHAR *config_name, WCHAR *password);
int RecallUsername(const WCHAR *config_name, WCHAR *username);

void DeleteSavedAuthPass(const WCHAR *config_name);
void DeleteSavedKeyPass(const WCHAR *config_name);
void DeleteSavedPasswords(const WCHAR *config_name);

BOOL IsAuthPassSaved(const WCHAR *config_name);
BOOL IsKeyPassSaved(const WCHAR *config_name);
#endif
