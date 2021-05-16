/*
 *  OpenVPN-GUI -- A Windows GUI for OpenVPN.
 *
 *  Copyright (C) 2004 Mathias Sundman <mathias@nilings.se>
 *                2010 Heiko Hund <heikoh@users.sf.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program (see the file COPYING included with this
 *  distribution); if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef DISABLE_CHANGE_PASSWORD
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/pkcs12.h>
#endif

#include <windows.h>
#include <wincrypt.h>

#include "main.h"
#include "options.h"
#include "passphrase.h"
#include "openvpn.h"
#include "openvpn-gui-res.h"
#include "chartable.h"
#include "localization.h"
#include "misc.h"

extern options_t o;

/*
 * Create a random password from the printable ASCII range
 */
BOOL
GetRandomPassword(char *buf, size_t len)
{
    HCRYPTPROV cp;
    BOOL retval = FALSE;
    unsigned i;

    if (!CryptAcquireContext(&cp, NULL, NULL, PROV_DSS, CRYPT_VERIFYCONTEXT))
        return FALSE;

    if (!CryptGenRandom(cp, len, (PBYTE) buf))
        goto out;

    /* Make sure all values are between 0x21 '!' and 0x7e '~' */
    for (i = 0; i < len; ++i)
        buf[i] = (buf[i] & 0x5d) + 0x21;

    retval = TRUE;
out:
    CryptReleaseContext(cp, 0);
    return retval;
}


#ifndef DISABLE_CHANGE_PASSWORD

const int KEYFILE_FORMAT_PKCS12 = 1;
const int KEYFILE_FORMAT_PEM = 2;
const int MIN_PASSWORD_LEN = 8;


/*
 * Return TRUE if new passwords match
 */
static int
ConfirmNewPassword(HWND hwndDlg)
{
  TCHAR newpsw[50];
  TCHAR newpsw2[50];

  GetDlgItemText(hwndDlg, ID_EDT_PASS_NEW, newpsw, _countof(newpsw) - 1);
  GetDlgItemText(hwndDlg, ID_EDT_PASS_NEW2, newpsw2, _countof(newpsw2) - 1);

  if (_tcsncmp(newpsw, newpsw2, _countof(newpsw)) == 0)
    return true;
  else
    return false;
}


/*
 * Return lengh of the new password
 */
static int
NewPasswordLengh(HWND hwndDlg)
{
  TCHAR newpsw[50];

  GetDlgItemText(hwndDlg, ID_EDT_PASS_NEW, newpsw, _countof(newpsw) - 1);

  return (_tcslen(newpsw));
}


static int
ConvertUnicode2Ascii(WCHAR str_unicode[], char str_ascii[], unsigned int str_ascii_size)
{
  unsigned int i;
  unsigned int j;
  int illegal_chars = false;
  for (i=0; (i < wcslen(str_unicode)) && (i < (str_ascii_size - 1)); i++)
    {
      for (j=0; j <= 256; j++)
        {
          if (j == 256)
            {
              illegal_chars = true;
              j = 0x2e;
              break;
            }
          if (str_unicode[i] == unicode_to_ascii[j]) break;
        }
      str_ascii[i] = (char) j;
   }
  str_ascii[i] = '\0';

  if (illegal_chars)
    return(false);
  else
    return(true);
}


/*
 * ChangePasswordPEM() returns:
 *   -1  Wrong password
 *    0  Changing password failed for unknown reason
 *    1  Password changed successfully
 */
static int
ChangePasswordPEM(HWND hwndDlg)
{
  TCHAR keyfile[MAX_PATH];
  char oldpsw[50];
  char newpsw[50];
  WCHAR oldpsw_unicode[50];
  WCHAR newpsw_unicode[50];
  FILE *fp;

  EVP_PKEY *privkey;

  /* Get filename, old_psw and new_psw from Dialog */
  GetDlgItemText(hwndDlg, ID_TXT_KEYFILE, keyfile, _countof(keyfile) - 1);
  GetDlgItemTextW(hwndDlg, ID_EDT_PASS_CUR, oldpsw_unicode, sizeof(oldpsw_unicode)/2 - 1);
  GetDlgItemTextW(hwndDlg, ID_EDT_PASS_NEW, newpsw_unicode, sizeof(newpsw_unicode)/2 - 1);

  /* Convert Unicode to ASCII (CP850) */
  ConvertUnicode2Ascii(oldpsw_unicode, oldpsw, sizeof(oldpsw));
  if (!ConvertUnicode2Ascii(newpsw_unicode, newpsw, sizeof(newpsw)))
    {
      ShowLocalizedMsg(IDS_ERR_INVALID_CHARS_IN_PSW);
      return(-1);
    }

  privkey = EVP_PKEY_new();

  /* Open old keyfile for reading */
  if (! (fp = _tfopen (keyfile, _T("r"))))
    {
      /* can't open key file */
      ShowLocalizedMsg(IDS_ERR_OPEN_PRIVATE_KEY_FILE, keyfile);
      return(0);
    }

  /* Import old key */
  if (! (privkey = PEM_read_PrivateKey (fp, NULL, NULL, oldpsw)))
    {
      /* wrong password */
      ShowLocalizedMsg(IDS_ERR_OLD_PWD_INCORRECT);
      fclose(fp);
      return(-1);
    }

  fclose(fp);

  /* Open keyfile for writing */
  if (! (fp = _tfopen (keyfile, _T("w"))))
    {
      /* can't open file rw */
      ShowLocalizedMsg(IDS_ERR_OPEN_WRITE_KEY, keyfile);
      EVP_PKEY_free(privkey);
      return(0);
    }

  /* Write new key to file */
  if (strlen(newpsw) == 0)
    {
      /* No passphrase */
      if ( !(PEM_write_PrivateKey(fp, privkey, \
                                  NULL, NULL,          /* Use NO encryption */
                                  0, 0, NULL)))
        {
          /* error writing new key */
          ShowLocalizedMsg(IDS_ERR_WRITE_NEW_KEY, keyfile);
          EVP_PKEY_free(privkey);
          fclose(fp);
          return(0);
        }
    }
  else
    {
      /* Use passphrase */
      if ( !(PEM_write_PrivateKey(fp, privkey, \
                                  EVP_des_ede3_cbc(),  /* Use 3DES encryption */
                                  (UCHAR*) newpsw, (int) strlen(newpsw), 0, NULL)))
        {
          /* can't write new key */
          ShowLocalizedMsg(IDS_ERR_WRITE_NEW_KEY, keyfile);
          EVP_PKEY_free(privkey);
          fclose(fp);
          return(0);
        }
    }

  EVP_PKEY_free(privkey);
  fclose(fp);

  /* signal success to user */
  ShowLocalizedMsg(IDS_NFO_PWD_CHANGED);
  return(1);
}


/*
 * ChangePasswordPKCS12() returns:
 *   -1  Wrong password
 *    0  Changing password failed for unknown reason
 *    1  Password changed successfully
 */
static int
ChangePasswordPKCS12(HWND hwndDlg)
{
  TCHAR keyfile[MAX_PATH];
  char oldpsw[50];
  char newpsw[50];
  WCHAR oldpsw_unicode[50];
  WCHAR newpsw_unicode[50];
  FILE *fp;

  EVP_PKEY *privkey;
  X509 *cert;
  STACK_OF(X509) *ca = NULL;
  PKCS12 *p12;
  char *alias;

  /* Get filename, old_psw and new_psw from Dialog */
  GetDlgItemText(hwndDlg, ID_TXT_KEYFILE, keyfile, _countof(keyfile) - 1);
  GetDlgItemTextW(hwndDlg, ID_EDT_PASS_CUR, oldpsw_unicode, sizeof(oldpsw_unicode)/2 - 1);
  GetDlgItemTextW(hwndDlg, ID_EDT_PASS_NEW, newpsw_unicode, sizeof(newpsw_unicode)/2 - 1);

  /* Convert Unicode to ASCII (CP850) */
  ConvertUnicode2Ascii(oldpsw_unicode, oldpsw, sizeof(oldpsw));
  if (!ConvertUnicode2Ascii(newpsw_unicode, newpsw, sizeof(newpsw)))
    {
      ShowLocalizedMsg(IDS_ERR_INVALID_CHARS_IN_PSW);
      return(-1);
    }

  /* Load the PKCS #12 file */
  if (!(fp = _tfopen(keyfile, _T("rb"))))
    {
      /* error opening file */
      ShowLocalizedMsg(IDS_ERR_OPEN_PRIVATE_KEY_FILE, keyfile);
      return(0);
    }
  p12 = d2i_PKCS12_fp(fp, NULL);
  fclose (fp);
  if (!p12)
    {
      /* error reading PKCS #12 */
      ShowLocalizedMsg(IDS_ERR_READ_PKCS12, keyfile);
      return(0);
    }

  /* Parse the PKCS #12 file */
  if (!PKCS12_parse(p12, oldpsw, &privkey, &cert, &ca))
    {
      /* old password incorrect */
      ShowLocalizedMsg(IDS_ERR_OLD_PWD_INCORRECT);
      PKCS12_free(p12);
      return(-1);
    }

  /* Free old PKCS12 object */
  PKCS12_free(p12);

  /* Get FriendlyName of old cert */
  alias = (char*) X509_alias_get0(cert, NULL);

  /* Create new PKCS12 object */
  p12 = PKCS12_create(newpsw, alias, privkey, cert, ca, 0,0,0,0,0);
  if (!p12)
    {
      /* create failed */
      ShowLocalizedMsg(IDS_ERR_CREATE_PKCS12);
      return(0);
    }

  /* Free old key, cert and ca */
  EVP_PKEY_free(privkey);
  X509_free(cert);
  sk_X509_pop_free(ca, X509_free);

  /* Open keyfile for writing */
  if (!(fp = _tfopen(keyfile, _T("wb"))))
    {
      ShowLocalizedMsg(IDS_ERR_OPEN_WRITE_KEY, keyfile);
      PKCS12_free(p12);
      return(0);
    }

  /* Write new key to file */
  i2d_PKCS12_fp(fp, p12);

  PKCS12_free(p12);
  fclose(fp);
  /* signal success to user */
  ShowLocalizedMsg(IDS_NFO_PWD_CHANGED);

  return(1);
}


INT_PTR CALLBACK
ChangePassphraseDialogFunc(HWND hwndDlg, UINT msg, WPARAM wParam, UNUSED LPARAM lParam)
{
  HICON hIcon;
  TCHAR keyfile[MAX_PATH];
  int keyfile_format;
  BOOL Translated;

  switch (msg) {

    case WM_INITDIALOG:
      hIcon = LoadLocalizedIcon(ID_ICO_APP);
      if (hIcon) {
        SendMessage(hwndDlg, WM_SETICON, (WPARAM) (ICON_SMALL), (LPARAM) (hIcon));
        SendMessage(hwndDlg, WM_SETICON, (WPARAM) (ICON_BIG), (LPARAM) (hIcon));
      }
      return FALSE;

    case WM_COMMAND:
      switch (LOWORD(wParam)) {

        case IDOK:

          /* Check if the type new passwords match. */
          if (!ConfirmNewPassword (hwndDlg))
            {
              /* passwords don't match */
              ShowLocalizedMsg(IDS_ERR_PWD_DONT_MATCH);
              break;
            }

          /* Confirm if the new password is empty. */
          if (NewPasswordLengh(hwndDlg) == 0)
            {
               if (ShowLocalizedMsgEx(MB_YESNO, _T(PACKAGE_NAME), IDS_NFO_EMPTY_PWD) == IDNO)
                  break;
            }
          /* Else check minimum length of password */
          else if (NewPasswordLengh(hwndDlg) < MIN_PASSWORD_LEN)
            {
              ShowLocalizedMsg(IDS_ERR_PWD_TO_SHORT, MIN_PASSWORD_LEN);
              break;
            }

          GetDlgItemText(hwndDlg, ID_TXT_KEYFILE, keyfile, _countof(keyfile) - 1);
          keyfile_format=GetDlgItemInt(hwndDlg, ID_TXT_KEYFORMAT, &Translated, FALSE);
          if (keyfile_format == KEYFILE_FORMAT_PEM)
            {
              /* Change password of a PEM file */
              if (ChangePasswordPEM(hwndDlg) == -1) /* Wrong password */
                break;
            }
          else if (keyfile_format == KEYFILE_FORMAT_PKCS12)
            {
              /* Change password of a .P12 file */
              if (ChangePasswordPKCS12(hwndDlg) == -1) /* Wrong password */
                break;
            }
          else
            {
              /* Unknown key format */
              ShowLocalizedMsg(IDS_ERR_UNKNOWN_KEYFILE_FORMAT);
            }

          DestroyWindow(hwndDlg);
          break;

        case IDCANCEL:
          DestroyWindow(hwndDlg);
          break;
      }
      break;


    case WM_DESTROY:
      PostQuitMessage(0);
      break;


    case WM_CLOSE:
      DestroyWindow(hwndDlg);
      return FALSE;

  }
  return FALSE;
}


static int
LineBeginsWith(char *line, const char *keyword, const unsigned int len)
{
  if (strncmp(line, keyword, len) == 0)
    {
      if ((line[len] == '\t') || (line[len] == ' '))
        return true;
    }

  return false;
}


static int
ParseKeyFilenameLine(connection_t *c, TCHAR *keyfilename, size_t keyfilenamesize, char *line)
{
  const int STATE_INITIAL = 0;
  const int STATE_READING_QUOTED_PARM = 1;
  const int STATE_READING_UNQUOTED_PARM = 2;
  int i=0;
  unsigned int j=0;
  int state = STATE_INITIAL;
  int backslash=0;
  TCHAR temp_filename[MAX_PATH];

  while(line[i] != '\0')
    {
      if (state == STATE_INITIAL)
        {
          if (line[i] == '\"')
            {
              state=STATE_READING_QUOTED_PARM;
            }
          else if ((line[i] == 0x0A) || (line[i] == 0x0D))
            break;
          else if ((line[i] == ';') || (line[i] == '#'))
            break;
          else if ((line[i] != ' ') && (line[i] != '\t'))
            {
              if (line[i] == '\\')
                {
                  if(!backslash)
                    {
                      keyfilename[j++]=line[i];
                      state=STATE_READING_UNQUOTED_PARM;
                      backslash=1;
                    }
                  else
                    {
                      backslash=0;
                    }
                }
              else
                {
                  if (backslash) backslash=0;
                  keyfilename[j++]=line[i];
                  state=STATE_READING_UNQUOTED_PARM;
                }
            }
        }

      else if (state == STATE_READING_QUOTED_PARM)
        {
          if (line[i] == '\"')
            break;
          if ((line[i] == 0x0A) || (line[i] == 0x0D))
            break;
          if (line[i] == '\\')
            {
              if (!backslash)
                {
                  keyfilename[j++]=line[i];
                  backslash=1;
                }
              else
                {
                  backslash=0;
                }
            }
          else
            {
              if (backslash) backslash=0;
              keyfilename[j++]=line[i];
            }
        }

      else if (state == STATE_READING_UNQUOTED_PARM)
        {
          if (line[i] == '\"')
            break;
          if ((line[i] == 0x0A) || (line[i] == 0x0D))
            break;
          if ((line[i] == ';') || (line[i] == '#'))
            break;
          if (line[i] == ' ')
            break;
          if (line[i] == '\t')
            break;
          if (line[i] == '\\')
            {
              if (!backslash)
                {
                  keyfilename[j++]=line[i];
                  backslash=1;
                }
              else
                {
                  backslash=0;
                }
            }
          else
            {
              if (backslash) backslash=0;
              keyfilename[j++]=line[i];
            }
        }

      if (j >= (keyfilenamesize - 1))
        {
          /* key filename to long */
          ShowLocalizedMsg(IDS_ERR_KEY_FILENAME_TO_LONG);
          return(0);
        }
      i++;
    }
  keyfilename[j]='\0';

  /* Prepend filename with configdir path if needed */
  if ((keyfilename[0] != '\\') && (keyfilename[0] != '/') && (keyfilename[1] != ':'))
    {
      _tcsncpy(temp_filename, c->config_dir, _countof(temp_filename));
      if (temp_filename[_tcslen(temp_filename) - 1] != '\\')
        _tcscat(temp_filename, _T("\\"));
      _tcsncat(temp_filename, keyfilename,
              _countof(temp_filename) - _tcslen(temp_filename) - 1);
      _tcsncpy(keyfilename, temp_filename, keyfilenamesize - 1);
    }

  return(1);
}

static int
GetKeyFilename(connection_t *c, TCHAR *keyfilename, size_t keyfilenamesize, int *keyfile_format, bool silent)
{
  FILE *fp = NULL;
  char line[256];
  int found_key=0;
  int found_pkcs12=0;
  TCHAR configfile_path[MAX_PATH];
  int ret = 0;

  _tcsncpy(configfile_path, c->config_dir, _countof(configfile_path));
  if (!(configfile_path[_tcslen(configfile_path)-1] == '\\'))
    _tcscat(configfile_path, _T("\\"));
  _tcsncat(configfile_path, c->config_file,
          _countof(configfile_path) - _tcslen(configfile_path) - 1);

  if (!(fp=_tfopen(configfile_path, _T("r"))))
    {
      /* can't open config file */
      if (!silent)
        ShowLocalizedMsg(IDS_ERR_OPEN_CONFIG, configfile_path);
      goto out;
    }

  while (fgets(line, sizeof (line), fp))
    {
      if (LineBeginsWith(line, "key", 3))
        {
          if (found_key)
            {
              /* only one key option */
              if (!silent)
                ShowLocalizedMsg(IDS_ERR_ONLY_ONE_KEY_OPTION);
              goto out;
            }
          if (found_pkcs12)
            {
              /* key XOR pkcs12 */
	      if (!silent)
                ShowLocalizedMsg(IDS_ERR_ONLY_KEY_OR_PKCS12);
              goto out;
            }
          found_key=1;
          *keyfile_format = KEYFILE_FORMAT_PEM;
          if (!ParseKeyFilenameLine(c, keyfilename, keyfilenamesize, &line[4]))
            goto out;
        }
      if (LineBeginsWith(line, "pkcs12", 6))
        {
          if (found_pkcs12)
            {
              /* only one pkcs12 option */
	      if (!silent)
                ShowLocalizedMsg(IDS_ERR_ONLY_ONE_PKCS12_OPTION);
              goto out;
            }
          if (found_key)
            {
              /* only key XOR pkcs12 */
	      if (!silent)
                ShowLocalizedMsg(IDS_ERR_ONLY_KEY_OR_PKCS12);
              goto out;
            }
          found_pkcs12=1;
          *keyfile_format = KEYFILE_FORMAT_PKCS12;
          if (!ParseKeyFilenameLine(c, keyfilename, keyfilenamesize, &line[7]))
            goto out;
        }
    }

  if ((!found_key) && (!found_pkcs12))
    {
      /* must have key or pkcs12 option */
      if (!silent)
        ShowLocalizedMsg(IDS_ERR_HAVE_KEY_OR_PKCS12);
      goto out;
    }
  ret = 1;
out:
  if (fp)
    fclose(fp);
  return ret;
}


static DWORD WINAPI
ChangePassphraseThread(LPVOID data)
{
  HWND hwndChangePSW;
  MSG messages;
  TCHAR conn_name[100];
  TCHAR keyfilename[MAX_PATH];
  int keyfile_format=0;
  connection_t *c = data;

  /* Cut of extention from config filename. */
  _tcsncpy(conn_name, c->config_file, _countof(conn_name));
  conn_name[_tcslen(conn_name) - (_tcslen(o.ext_string)+1)]=0;

  /* Get Key filename from config file */
  if (!GetKeyFilename(c, keyfilename, _countof(keyfilename), &keyfile_format, false))
    {
      ExitThread(1);
    }

  /* Show ChangePassphrase Dialog */  
  hwndChangePSW = CreateLocalizedDialog(ID_DLG_CHGPASS, ChangePassphraseDialogFunc);
  if (!hwndChangePSW)
    ExitThread(1);
  SetDlgItemText(hwndChangePSW, ID_TXT_KEYFILE, keyfilename); 
  SetDlgItemInt(hwndChangePSW, ID_TXT_KEYFORMAT, (UINT) keyfile_format, FALSE);

  SetWindowText(hwndChangePSW, LoadLocalizedString(IDS_NFO_CHANGE_PWD, conn_name));

  ShowWindow(hwndChangePSW, SW_SHOW);


  /* Run the message loop. It will run until GetMessage() returns 0 */
  while (GetMessage (&messages, NULL, 0, 0))
    {
      if(!IsDialogMessage(hwndChangePSW, &messages))
      {
        TranslateMessage(&messages);
        DispatchMessage(&messages);
      }
    }

  CloseHandle (hwndChangePSW);
  ExitThread(0);
}


void
ShowChangePassphraseDialog(connection_t *c)
{
  HANDLE hThread;
  DWORD IDThread;

  /* Start a new thread to have our own message-loop for this dialog */
  hThread = CreateThread(NULL, 0, ChangePassphraseThread, c, 0, &IDThread);
  if (hThread == NULL)
    {
    /* error creating thread */
    ShowLocalizedMsg(IDS_ERR_CREATE_PASS_THREAD);
    return;
  }
  CloseHandle (hThread);
}

bool
CheckKeyFileWriteAccess (connection_t *c)
{
   TCHAR keyfile[MAX_PATH];
   int format = 0;
   if (!GetKeyFilename (c, keyfile, _countof(keyfile), &format, true))
     return FALSE;
   else
     return CheckFileAccess (keyfile, GENERIC_WRITE);
}

#endif
