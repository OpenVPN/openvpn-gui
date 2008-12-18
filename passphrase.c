/*
 *  OpenVPN-GUI -- A Windows GUI for OpenVPN.
 *
 *  Copyright (C) 2004 Mathias Sundman <mathias@nilings.se>
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

#include <windows.h>
#include "main.h"
#include "options.h"
#include "passphrase.h"
#include "openvpn.h"
#include "openvpn-gui-res.h"
#include "chartable.h"

#ifndef DISABLE_CHANGE_PASSWORD
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/pkcs12.h>
#endif

WCHAR passphrase[256];
extern struct options o;

int ConvertUnicode2Ascii(WCHAR str_unicode[], char str_ascii[], unsigned int str_ascii_size)
{
  unsigned int i;
  unsigned int j;
  int illegal_chars = false;
  char *str_unicode_ptr = (char *) str_unicode;
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

void CheckPrivateKeyPassphrasePrompt (char *line, int config)
{
  DWORD nCharsWritten;
  char passphrase_ascii[256];

  /* Check for Passphrase prompt */
  if (strncmp(line, "Enter PEM pass phrase:", 22) == 0) 
    {
      CLEAR(passphrase);  
      if (DialogBox(o.hInstance, (LPCTSTR)IDD_PASSPHRASE, NULL, 
                   (DLGPROC)PassphraseDialogFunc) == IDCANCEL)
        {
          StopOpenVPN(config);
        }

      if (wcslen(passphrase) > 0)
        {
          CLEAR(passphrase_ascii);
          ConvertUnicode2Ascii(passphrase, passphrase_ascii, sizeof(passphrase_ascii));

          if (!WriteFile(o.cnn[config].hStdIn, passphrase_ascii,
                    strlen(passphrase_ascii), &nCharsWritten, NULL))
            {
              /* PassPhrase -> stdin failed */
              ShowLocalizedMsg(GUI_NAME, ERR_PASSPHRASE2STDIN, "");
            }
        }
      if (!WriteFile(o.cnn[config].hStdIn, "\r\n",
                        2, &nCharsWritten, NULL))
        {
          /* CR -> stdin failed */
          ShowLocalizedMsg(GUI_NAME, ERR_CR2STDIN, "");
        }
      /* Remove Passphrase prompt from lastline buffer */
      line[0]='\0';

      /* Clear passphrase buffer */
      CLEAR(passphrase);
      CLEAR(passphrase_ascii);
    }

  /* Check for new passphrase prompt introduced with OpenVPN 2.0-beta12. */
  if (strncmp(line, "Enter Private Key Password:", 27) == 0) 
    {
      CLEAR(passphrase);  
      if (DialogBox(o.hInstance, (LPCTSTR)IDD_PASSPHRASE, NULL, 
                   (DLGPROC)PassphraseDialogFunc) == IDCANCEL)
        {
          StopOpenVPN(config);
        }

      if (wcslen(passphrase) > 0)
        {
          CLEAR(passphrase_ascii);
          ConvertUnicode2Ascii(passphrase, passphrase_ascii, sizeof(passphrase_ascii));

          if (!WriteFile(o.cnn[config].hStdIn, passphrase_ascii,
                    strlen(passphrase_ascii), &nCharsWritten, NULL))
            {
              /* PassPhrase -> stdin failed */
              ShowLocalizedMsg(GUI_NAME, ERR_PASSPHRASE2STDIN, "");
            }
        }
      else
        {
          if (!WriteFile(o.cnn[config].hStdIn, "\n",
                        1, &nCharsWritten, NULL))
            {
              /* CR -> stdin failed */
              ShowLocalizedMsg(GUI_NAME, ERR_CR2STDIN, "");
            }
        }
      /* Remove Passphrase prompt from lastline buffer */
      line[0]='\0';

      /* Clear passphrase buffer */
      CLEAR(passphrase);
      CLEAR(passphrase_ascii);
    }

}

void CheckAuthUsernamePrompt (char *line, int config)
{
  DWORD nCharsWritten;
  struct user_auth user_auth;

  /* Check for Passphrase prompt */
  if (strncmp(line, "Enter Auth Username:", 20) == 0) 
    {
      CLEAR(user_auth);  
      if (DialogBoxParam(o.hInstance, 
                         (LPCTSTR)IDD_AUTH_PASSWORD,
                         NULL,
                         (DLGPROC)AuthPasswordDialogFunc,
                         (LPARAM)&user_auth) == IDCANCEL)
        {
          StopOpenVPN(config);
        }

      if (strlen(user_auth.username) > 0)
        {
          if (!WriteFile(o.cnn[config].hStdIn, user_auth.username,
                    strlen(user_auth.username), &nCharsWritten, NULL))
            {
              ShowLocalizedMsg(GUI_NAME, ERR_AUTH_USERNAME2STDIN, "");
            }
        }
      else
        {
          if (!WriteFile(o.cnn[config].hStdIn, "\n",
                        1, &nCharsWritten, NULL))
            {
              ShowLocalizedMsg(GUI_NAME, ERR_CR2STDIN, "");
            }
        }

      if (strlen(user_auth.password) > 0)
        {
          if (!WriteFile(o.cnn[config].hStdIn, user_auth.password,
                    strlen(user_auth.password), &nCharsWritten, NULL))
            {
              ShowLocalizedMsg(GUI_NAME, ERR_AUTH_PASSWORD2STDIN, "");
            }
        }
      else
        {
          if (!WriteFile(o.cnn[config].hStdIn, "\n",
                        1, &nCharsWritten, NULL))
            {
              ShowLocalizedMsg(GUI_NAME, ERR_CR2STDIN, "");
            }
        }

      /* Remove Username prompt from lastline buffer */
      line[0]='\0';

      /* Clear user_auth buffer */
      CLEAR(user_auth);  
    }

}

void CheckAuthPasswordPrompt (char *line)
{

  /* Check for Passphrase prompt */
  if (strncmp(line, "Enter Auth Password:", 20) == 0) 
    {

      /* Remove Passphrase prompt from lastline buffer */
      line[0]='\0';

    }
}

BOOL CALLBACK PassphraseDialogFunc (HWND hwndDlg, UINT msg, WPARAM wParam, UNUSED LPARAM lParam)
{
  static char empty_string[100];

  switch (msg) {

    case WM_INITDIALOG:
      SetForegroundWindow(hwndDlg);
      break;

    case WM_COMMAND:
      switch (LOWORD(wParam)) {

        case IDOK:			// button
          GetDlgItemTextW(hwndDlg, EDIT_PASSPHRASE, passphrase, sizeof(passphrase)/2 - 1);

          /* Clear buffer */
          SetDlgItemText(hwndDlg, EDIT_PASSPHRASE, empty_string);

          EndDialog(hwndDlg, LOWORD(wParam));
          return TRUE;

        case IDCANCEL:			// button
          EndDialog(hwndDlg, LOWORD(wParam));
          return TRUE;
      }
      break;
    case WM_CLOSE:
      EndDialog(hwndDlg, LOWORD(wParam));
      return TRUE;
     
  }
  return FALSE;
}

BOOL CALLBACK AuthPasswordDialogFunc (HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
  static struct user_auth *user_auth;
  static char empty_string[100];
  WCHAR username_unicode[50];
  WCHAR password_unicode[50];

  switch (msg) {

    case WM_INITDIALOG:
      user_auth = (struct user_auth *) lParam;
      SetForegroundWindow(hwndDlg);
      break;

    case WM_COMMAND:
      switch (LOWORD(wParam)) {

        case IDOK:			// button
          GetDlgItemTextW(hwndDlg, EDIT_AUTH_USERNAME, username_unicode, sizeof(username_unicode)/2 - 1);
          GetDlgItemTextW(hwndDlg, EDIT_AUTH_PASSWORD, password_unicode, sizeof(password_unicode)/2 - 1);

          /* Convert username/password from Unicode to Ascii (CP850) */
          ConvertUnicode2Ascii(username_unicode, user_auth->username, sizeof(user_auth->username) - 1);
          ConvertUnicode2Ascii(password_unicode, user_auth->password, sizeof(user_auth->password) - 1);

          /* Clear buffers */
          SetDlgItemText(hwndDlg, EDIT_AUTH_USERNAME, empty_string);
          SetDlgItemText(hwndDlg, EDIT_AUTH_PASSWORD, empty_string);

          EndDialog(hwndDlg, LOWORD(wParam));
          return TRUE;

        case IDCANCEL:			// button
          EndDialog(hwndDlg, LOWORD(wParam));
          return TRUE;
      }
      break;
    case WM_CLOSE:
      EndDialog(hwndDlg, LOWORD(wParam));
      return TRUE;
     
  }
  return FALSE;
}


#ifndef DISABLE_CHANGE_PASSWORD

const int KEYFILE_FORMAT_PKCS12 = 1;
const int KEYFILE_FORMAT_PEM = 2;

void ShowChangePassphraseDialog(int config)
{
  HANDLE hThread; 
  DWORD IDThread;

  /* Start a new thread to have our own message-loop for this dialog */
  hThread = CreateThread(NULL, 0, 
            (LPTHREAD_START_ROUTINE) ChangePassphraseThread,
            (int *) config,  // pass config nr
            0, &IDThread); 
  if (hThread == NULL) 
    {
    /* error creating thread */
    ShowLocalizedMsg (GUI_NAME, ERR_CREATE_PASS_THREAD, "");
    return;
  }

}

void ChangePassphraseThread(int config)
{
  HWND hwndChangePSW;
  MSG messages;
  char conn_name[100];
  char msg[100];
  char keyfilename[MAX_PATH];
  int keyfile_format=0;
  TCHAR buf[1000];

  /* Cut of extention from config filename. */
  strncpy(conn_name, o.cnn[config].config_file, sizeof(conn_name));
  conn_name[strlen(conn_name) - (strlen(o.ext_string)+1)]=0;

  /* Get Key filename from config file */
  if (!GetKeyFilename(config, keyfilename, sizeof(keyfilename), &keyfile_format))
    {
      ExitThread(1);
    }

  /* Show ChangePassphrase Dialog */  
  if (!(hwndChangePSW = CreateDialog (o.hInstance, 
        MAKEINTRESOURCE (IDD_CHANGEPSW),
        NULL, (DLGPROC) ChangePassphraseDialogFunc)))
    return;
  SetDlgItemText(hwndChangePSW, TEXT_KEYFILE, keyfilename); 
  SetDlgItemInt(hwndChangePSW, TEXT_KEYFORMAT, (UINT) keyfile_format, FALSE);

  myLoadString(INFO_CHANGE_PWD);
  mysnprintf(msg, buf, conn_name);
  SetWindowText(hwndChangePSW, msg);

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

  ExitThread(0);
}

BOOL CALLBACK ChangePassphraseDialogFunc (HWND hwndDlg, UINT msg, WPARAM wParam, UNUSED LPARAM lParam)
{
  HICON hIcon;
  char keyfile[MAX_PATH];
  int keyfile_format;
  BOOL Translated;
  TCHAR buf[1000];

  switch (msg) {

    case WM_INITDIALOG:
      hIcon = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(APP_ICON), 
                                                      IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
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
              ShowLocalizedMsg(GUI_NAME, ERR_PWD_DONT_MATCH, "");
              break;
            }

          /* Check minimum length of password */
          if (NewPasswordLengh(hwndDlg) < MIN_PASSWORD_LEN)
            {
              ShowLocalizedMsg(GUI_NAME, ERR_PWD_TO_SHORT, MIN_PASSWORD_LEN);
              break;
            }

          /* Check if the new password is empty. */
          if (NewPasswordLengh(hwndDlg) == 0)
            {
              myLoadString(INFO_EMPTY_PWD);
              if (MessageBox(NULL, buf, GUI_NAME, MB_YESNO) != IDYES)
                break;
            }

          GetDlgItemText(hwndDlg, TEXT_KEYFILE, keyfile, sizeof(keyfile) - 1);
          keyfile_format=GetDlgItemInt(hwndDlg, TEXT_KEYFORMAT, &Translated, FALSE);
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
              ShowLocalizedMsg (GUI_NAME, ERR_UNKNOWN_KEYFILE_FORMAT, "");
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


/* Return TRUE if new passwords match */
int ConfirmNewPassword(HWND hwndDlg)
{
  char newpsw[50];
  char newpsw2[50];

  GetDlgItemText(hwndDlg, EDIT_PSW_NEW, newpsw, sizeof(newpsw) - 1); 
  GetDlgItemText(hwndDlg, EDIT_PSW_NEW2, newpsw2, sizeof(newpsw2) - 1); 

  if (strncmp(newpsw, newpsw2, sizeof(newpsw)) == 0)
    return true;
  else
    return false;
}

/* Return lengh of the new password */
int NewPasswordLengh(HWND hwndDlg)
{
  char newpsw[50];

  GetDlgItemText(hwndDlg, EDIT_PSW_NEW, newpsw, sizeof(newpsw) - 1); 

  return (strlen(newpsw));
}

int ParseKeyFilenameLine(int config, char *keyfilename, unsigned int keyfilenamesize, char *line)
{
  const int STATE_INITIAL = 0;
  const int STATE_READING_QUOTED_PARM = 1;
  const int STATE_READING_UNQUOTED_PARM = 2;
  int i=0;
  unsigned int j=0;
  int state = STATE_INITIAL;
  int backslash=0;
  char temp_filename[MAX_PATH];

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
          ShowLocalizedMsg(GUI_NAME, ERR_KEY_FILENAME_TO_LONG, "");
          return(0);
        }
      i++;
    }
  keyfilename[j]='\0';

  /* Prepend filename with configdir path if needed */
  if ((keyfilename[0] != '\\') && (keyfilename[0] != '/') && (keyfilename[1] != ':'))
    {
      strncpy(temp_filename, o.cnn[config].config_dir, sizeof(temp_filename));
      if (temp_filename[strlen(temp_filename) - 1] != '\\')
        strcat(temp_filename, "\\");
      strncat(temp_filename, keyfilename, 
              sizeof(temp_filename) - strlen(temp_filename) - 1);
      strncpy(keyfilename, temp_filename, keyfilenamesize - 1);
    }

  return(1);
}


/* ChangePasswordPEM() returns:
 * -1 Wrong password
 * 0  Changing password failed for unknown reason
 * 1  Password changed successfully
 */
int ChangePasswordPEM(HWND hwndDlg)
{
  char keyfile[MAX_PATH];
  char oldpsw[50];
  char newpsw[50];
  WCHAR oldpsw_unicode[50];
  WCHAR newpsw_unicode[50];
  FILE *fp;

  EVP_PKEY *privkey;
 
  /* Get filename, old_psw and new_psw from Dialog */
  GetDlgItemText(hwndDlg, TEXT_KEYFILE, keyfile, sizeof(keyfile) - 1); 
  GetDlgItemTextW(hwndDlg, EDIT_PSW_CURRENT, oldpsw_unicode, sizeof(oldpsw_unicode)/2 - 1); 
  GetDlgItemTextW(hwndDlg, EDIT_PSW_NEW, newpsw_unicode, sizeof(newpsw_unicode)/2 - 1); 

  /* Convert Unicode to ASCII (CP850) */
  ConvertUnicode2Ascii(oldpsw_unicode, oldpsw, sizeof(oldpsw));
  if (!ConvertUnicode2Ascii(newpsw_unicode, newpsw, sizeof(newpsw)))
    {
      ShowLocalizedMsg(GUI_NAME, ERR_INVALID_CHARS_IN_PSW, "");
      return(-1);
    }

  privkey = EVP_PKEY_new();

  /* Open old keyfile for reading */
  if (! (fp = fopen (keyfile, "r")))
    {
      /* can't open key file */
      ShowLocalizedMsg(GUI_NAME, ERR_OPEN_PRIVATE_KEY_FILE, keyfile);
      return(0);
    }

  /* Import old key */
  if (! (privkey = PEM_read_PrivateKey (fp, NULL, NULL, oldpsw)))
    {
      /* wrong password */
      ShowLocalizedMsg(GUI_NAME, ERR_OLD_PWD_INCORRECT, ""); 
      fclose(fp);
      return(-1);
    }

  fclose(fp);

  /* Open keyfile for writing */
  if (! (fp = fopen (keyfile, "w")))
    {
      /* can't open file rw */
      ShowLocalizedMsg(GUI_NAME, ERR_OPEN_WRITE_KEY, keyfile);
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
          ShowLocalizedMsg(GUI_NAME, ERR_WRITE_NEW_KEY, keyfile);
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
                                  newpsw, (int) strlen(newpsw), 0, NULL)))
        {
          /* can't write new key */
          ShowLocalizedMsg(GUI_NAME, ERR_WRITE_NEW_KEY, keyfile);
          EVP_PKEY_free(privkey);
          fclose(fp);
          return(0);
        }
    }

  EVP_PKEY_free(privkey);
  fclose(fp);

  /* signal success to user */
  ShowLocalizedMsg(GUI_NAME, INFO_PWD_CHANGED, "");
  return(1);
}


/* ChangePasswordPKCS12() returns:
 * -1 Wrong password
 * 0  Changing password failed for unknown reason
 * 1  Password changed successfully
 */
int ChangePasswordPKCS12(HWND hwndDlg)
{
  char keyfile[MAX_PATH];
  char oldpsw[50];
  char newpsw[50];
  WCHAR oldpsw_unicode[50];
  WCHAR newpsw_unicode[50];
  FILE *fp;

  EVP_PKEY *privkey;
  X509 *cert;
  STACK_OF(X509) *ca = NULL;
  PKCS12 *p12;
  PKCS12 *new_p12;
  char *alias;

  /* Get filename, old_psw and new_psw from Dialog */
  GetDlgItemText(hwndDlg, TEXT_KEYFILE, keyfile, sizeof(keyfile) - 1); 
  GetDlgItemTextW(hwndDlg, EDIT_PSW_CURRENT, oldpsw_unicode, sizeof(oldpsw_unicode)/2 - 1); 
  GetDlgItemTextW(hwndDlg, EDIT_PSW_NEW, newpsw_unicode, sizeof(newpsw_unicode)/2 - 1); 

  /* Convert Unicode to ASCII (CP850) */
  ConvertUnicode2Ascii(oldpsw_unicode, oldpsw, sizeof(oldpsw));
  if (!ConvertUnicode2Ascii(newpsw_unicode, newpsw, sizeof(newpsw)))
    {
      ShowLocalizedMsg(GUI_NAME, ERR_INVALID_CHARS_IN_PSW, "");
      return(-1);
    }

  /* Load the PKCS #12 file */
  if (!(fp = fopen(keyfile, "rb")))
    {
      /* error opening file */
      ShowLocalizedMsg(GUI_NAME, ERR_OPEN_PRIVATE_KEY_FILE, keyfile);
      return(0);
    }
  p12 = d2i_PKCS12_fp(fp, NULL);
  fclose (fp);
  if (!p12) 
    {
      /* error reading PKCS #12 */
      ShowLocalizedMsg(GUI_NAME, ERR_READ_PKCS12, keyfile);
      return(0);
    }

  /* Parse the PKCS #12 file */
  if (!PKCS12_parse(p12, oldpsw, &privkey, &cert, &ca))
    {
      /* old password incorrect */
      ShowLocalizedMsg(GUI_NAME, ERR_OLD_PWD_INCORRECT, ""); 
      PKCS12_free(p12);
      return(-1);
    }

  /* Free old PKCS12 object */
  PKCS12_free(p12);

  /* Get FriendlyName of old cert */
  alias = X509_alias_get0(cert, NULL);

  /* Create new PKCS12 object */
  p12 = PKCS12_create(newpsw, alias, privkey, cert, ca, 0,0,0,0,0);
  if (!p12)
    {
      /* create failed */
      //ShowMsg(GUI_NAME, ERR_error_string(ERR_peek_last_error(), NULL));
      ShowLocalizedMsg(GUI_NAME, ERR_CREATE_PKCS12, "");
      return(0);
    }

  /* Free old key, cert and ca */
  EVP_PKEY_free(privkey);
  X509_free(cert);
  sk_X509_pop_free(ca, X509_free);

  /* Open keyfile for writing */
  if (!(fp = fopen(keyfile, "wb")))
    {
      ShowLocalizedMsg(GUI_NAME, ERR_OPEN_WRITE_KEY, keyfile);
      PKCS12_free(p12);
      return(0);
    }

  /* Write new key to file */
  i2d_PKCS12_fp(fp, p12);

  PKCS12_free(p12);
  fclose(fp);
  /* signal success to user */
  ShowLocalizedMsg(GUI_NAME, INFO_PWD_CHANGED, "");

  return(1);
}

int LineBeginsWith(char *line, const char *keyword, const unsigned int len)
{
  if (strncmp(line, keyword, len) == 0)
    {
      if ((line[len] == '\t') || (line[len] == ' '))
        return true;
    }

  return false;
}

int GetKeyFilename(int config, char *keyfilename, unsigned int keyfilenamesize, int *keyfile_format)
{
  FILE *fp;
  char line[256];
  int found_key=0;
  int found_pkcs12=0;
  char configfile_path[MAX_PATH];

  strncpy(configfile_path, o.cnn[config].config_dir, sizeof(configfile_path));
  if (!(configfile_path[strlen(configfile_path)-1] == '\\'))
    strcat(configfile_path, "\\");
  strncat(configfile_path, o.cnn[config].config_file, 
          sizeof(configfile_path) - strlen(configfile_path) - 1);

  if (!(fp=fopen(configfile_path, "r")))
    {
      /* can't open config file */
      ShowLocalizedMsg(GUI_NAME, ERR_OPEN_CONFIG, configfile_path);
      return(0);
    }

  while (fgets(line, sizeof (line), fp))
    {
      if (LineBeginsWith(line, "key", 3))
        {
          if (found_key)
            {
              /* only one key option */
              ShowLocalizedMsg(GUI_NAME, ERR_ONLY_ONE_KEY_OPTION, "");
              return(0);
            }
          if (found_pkcs12)
            {
              /* key XOR pkcs12 */
              ShowLocalizedMsg(GUI_NAME, ERR_ONLY_KEY_OR_PKCS12, "");
              return(0);
            }
          found_key=1;
          *keyfile_format = KEYFILE_FORMAT_PEM;
          if (!ParseKeyFilenameLine(config, keyfilename, keyfilenamesize, &line[4]))
            return(0);
        }
      if (LineBeginsWith(line, "pkcs12", 6))
        {
          if (found_pkcs12)
            {
              /* only one pkcs12 option */
              ShowLocalizedMsg(GUI_NAME, ERR_ONLY_ONE_PKCS12_OPTION, "");
              return(0);
            }
          if (found_key)
            {
              /* only key XOR pkcs12 */
              ShowLocalizedMsg(GUI_NAME, ERR_ONLY_KEY_OR_PKCS12, "");
              return(0);
            }
          found_pkcs12=1;
          *keyfile_format = KEYFILE_FORMAT_PKCS12;
          if (!ParseKeyFilenameLine(config, keyfilename, keyfilenamesize, &line[7]))
            return(0);
        }      
    }

  if ((!found_key) && (!found_pkcs12))
    {
      /* must have key or pkcs12 option */
      ShowLocalizedMsg(GUI_NAME, ERR_MUST_HAVE_KEY_OR_PKCS12, "");
      return(0);
    }

  return(1);
}


#endif
