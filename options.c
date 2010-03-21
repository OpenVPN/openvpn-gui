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
 *
 */

#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <windows.h>

#include "config.h"
#include "options.h"
#include "main.h"
#include "openvpn-gui-res.h"
#include "localization.h"

extern struct options o;

void
init_options (struct options *opt)
{
  CLEAR (*opt);
}

int Createargcargv(struct options *options, TCHAR *command_line)
{

    int     argc;
    TCHAR **argv;

    TCHAR*  arg;
    int     myindex;

    // count the arguments

    argc = 0;
    arg  = command_line;

    while (arg[0] != 0) {

        while (arg[0] != 0 && arg[0] == ' ') {
            arg++;
        }

        if (arg[0] != 0) {

            argc++;

            if (arg[0] == '\"') {
               arg++; 
               while (arg[0] != 0 && arg[0] != '\"') {
                  arg++;
               }
            } 
            while (arg[0] != 0 && arg[0] != ' ') {
               arg++;
            }

        }

    }    

    // tokenize the arguments
    argv = (TCHAR**) malloc(argc * sizeof(TCHAR*));

    arg = command_line;
    myindex = 0;

    while (arg[0] != 0) {

        while (arg[0] != 0 && arg[0] == ' ') {
            arg++;
        }

        if (arg[0] != 0) {

            if (arg[0] == '\"') {
               arg++;
               argv[myindex] = arg;
               myindex++;
               while (arg[0] != 0 && arg[0] != '\"') {
                  arg++;
               }
               if (arg[0] != 0) {
                  arg[0] = 0;    
                  arg++;
               }
               while (arg[0] != 0 && arg[0] != ' ') {
                   arg++;
               }
            }
            else {
               argv[myindex] = arg;
               myindex++;
               while (arg[0] != 0 && arg[0] != ' ') {
                   arg++;
               }
               if (arg[0] != 0) {
                   arg[0] = 0;    
                   arg++;
               }
            }


        }

    }    

    parse_argv(options, argc, argv);

    free(argv);
    return(0);

}

static int
add_option (struct options *options,
	    int i,
	    TCHAR *p[])
{

  if (streq (p[0], _T("help")))
    {
      TCHAR usagecaption[200];
      
      LoadLocalizedStringBuf(usagecaption, _tsizeof(usagecaption), IDS_NFO_USAGECAPTION);
      MessageBox(NULL, LoadLocalizedString(IDS_NFO_USAGE), usagecaption, MB_OK);
      exit(0);
    }
  else if (streq (p[0], _T("connect")) && p[1])
    {
      ++i;
      static int auto_connect_nr=0;
      if (auto_connect_nr == MAX_CONFIGS)
        {
          /* Too many configs */
          ShowLocalizedMsg(IDS_ERR_MANY_CONFIGS, MAX_CONFIGS);
          exit(1);
        }

      options->auto_connect[auto_connect_nr] = p[1];
      auto_connect_nr++;
    }
  else if (streq (p[0], _T("exe_path")) && p[1])
    {
      ++i;
      _tcsncpy(options->exe_path, p[1], _tsizeof(options->exe_path) - 1);
    }
  else if (streq (p[0], _T("config_dir")) && p[1])
    {
      ++i;
      _tcsncpy(options->config_dir, p[1], _tsizeof(options->config_dir) - 1);
    }
  else if (streq (p[0], _T("ext_string")) && p[1])
    {
      ++i;
      _tcsncpy(options->ext_string, p[1], _tsizeof(options->ext_string) - 1);
    }
  else if (streq (p[0], _T("log_dir")) && p[1])
    {
      ++i;
      _tcsncpy(options->log_dir, p[1], _tsizeof(options->log_dir) - 1);
    }
  else if (streq (p[0], _T("priority_string")) && p[1])
    {
      ++i;
      _tcsncpy(options->priority_string, p[1], _tsizeof(options->priority_string) - 1);
    }
  else if (streq (p[0], _T("append_string")) && p[1])
    {
      ++i;
      _tcsncpy(options->append_string, p[1], _tsizeof(options->append_string) - 1);
    }
  else if (streq (p[0], _T("log_viewer")) && p[1])
    {
      ++i;
      _tcsncpy(options->log_viewer, p[1], _tsizeof(options->log_viewer) - 1);
    }
  else if (streq (p[0], _T("editor")) && p[1])
    {
      ++i;
      _tcsncpy(options->editor, p[1], _tsizeof(options->editor) - 1);
    }
  else if (streq (p[0], _T("allow_edit")) && p[1])
    {
      ++i;
      _tcsncpy(options->allow_edit, p[1], _tsizeof(options->allow_edit) - 1);
    }
  else if (streq (p[0], _T("allow_service")) && p[1])
    {
      ++i;
      _tcsncpy(options->allow_service, p[1], _tsizeof(options->allow_service) - 1);
    }
  else if (streq (p[0], _T("allow_password")) && p[1])
    {
      ++i;
      _tcsncpy(options->allow_password, p[1], _tsizeof(options->allow_password) - 1);
    }
  else if (streq (p[0], _T("allow_proxy")) && p[1])
    {
      ++i;
      _tcsncpy(options->allow_proxy, p[1], _tsizeof(options->allow_proxy) - 1);
    }
  else if (streq (p[0], _T("show_balloon")) && p[1])
    {
      ++i;
      _tcsncpy(options->show_balloon, p[1], _tsizeof(options->show_balloon) - 1);
    }
  else if (streq (p[0], _T("service_only")) && p[1])
    {
      ++i;
      _tcsncpy(options->service_only, p[1], _tsizeof(options->service_only) - 1);
    }
  else if (streq (p[0], _T("show_script_window")) && p[1])
    {
      ++i;
      _tcsncpy(options->show_script_window, p[1], _tsizeof(options->show_script_window) - 1);
    }
  else if (streq (p[0], _T("silent_connection")) && p[1])
    {
      ++i;
      _tcsncpy(options->silent_connection, p[1], _tsizeof(options->silent_connection) - 1);
    }
  else if (streq (p[0], _T("passphrase_attempts")) && p[1])
    {
      ++i;
      _tcsncpy(options->psw_attempts_string, p[1], _tsizeof(options->psw_attempts_string) - 1);
    }
  else if (streq (p[0], _T("connectscript_timeout")) && p[1])
    {
      ++i;
      _tcsncpy(options->connectscript_timeout_string, p[1], _tsizeof(options->connectscript_timeout_string) - 1);
    }
  else if (streq (p[0], _T("disconnectscript_timeout")) && p[1])
    {
      ++i;
      _tcsncpy(options->disconnectscript_timeout_string, p[1], 
              _tsizeof(options->disconnectscript_timeout_string) - 1);
    }
  else if (streq (p[0], _T("preconnectscript_timeout")) && p[1])
    {
      ++i;
      _tcsncpy(options->preconnectscript_timeout_string, p[1], 
              _tsizeof(options->preconnectscript_timeout_string) - 1);
    }
  else
    {
        /* Unrecognized option or missing parameter */
	ShowLocalizedMsg(IDS_ERR_BAD_OPTION, p[0]);
        exit(1);
    }
  return i;
}

void
parse_argv (struct options* options,
	    int argc,
	    TCHAR *argv[])
{
  int i, j;

  /* parse command line */
  for (i = 1; i < argc; ++i)
    {
      TCHAR *p[MAX_PARMS];
      CLEAR (p);
      p[0] = argv[i];
      if (_tcsncmp(p[0], _T("--"), 2))
	{
          /* Missing -- before option. */
	  ShowLocalizedMsg(IDS_ERR_BAD_PARAMETER, p[0]);
	  exit(0);
        }
      else
	p[0] += 2;

      for (j = 1; j < MAX_PARMS; ++j)
	{
	  if (i + j < argc)
	    {
	      TCHAR *arg = argv[i + j];
	      if (_tcsncmp (arg, _T("--"), 2))
		p[j] = arg;
	      else
		break;
	    }
	}
      i = add_option (options, i, p);
    }
}

/*
 * Returns TRUE if option exist in config file.
 */
int ConfigFileOptionExist(int config, const char *option)
{
  FILE *fp;
  char line[256];
  TCHAR configfile_path[MAX_PATH];

  _tcsncpy(configfile_path, o.cnn[config].config_dir, _tsizeof(configfile_path));
  if (configfile_path[_tcslen(configfile_path) - 1] != _T('\\'))
    _tcscat(configfile_path, _T("\\"));
  _tcsncat(configfile_path, o.cnn[config].config_file, 
           _tsizeof(configfile_path) - _tcslen(configfile_path) - 1);

  if (!(fp=_tfopen(configfile_path, _T("r"))))
    {
      /* can't open config file */
      ShowLocalizedMsg(IDS_ERR_OPEN_CONFIG, configfile_path);
      return(0);
    }

  while (fgets(line, sizeof (line), fp))
    {
      if ((strncmp(line, option, sizeof(option)) == 0))
        {
          fclose(fp);
          return(1);
        }
    }

  fclose(fp);
  return(0);
}

