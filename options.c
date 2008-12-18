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

#include "options.h"
#include "main.h"
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <windows.h>
#include "openvpn-gui-res.h"

extern struct options o;

void
init_options (struct options *opt)
{
  CLEAR (*opt);
}

int Createargcargv(struct options* options, char* command_line)
{

    int    argc;
    char** argv;

    char*  arg;
    int    myindex;
    int    result;

    int	i;
    // count the arguments

    argc = 1;
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
    argv = (char**)malloc(argc * sizeof(char*));

    arg = command_line;
    myindex = 1;

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

    // put the program name into argv[0]
    char filename[_MAX_PATH];

    GetModuleFileName(NULL, filename, _MAX_PATH);
    argv[0] = filename;

    parse_argv(options, argc, argv);

    free(argv);
    return(0);

}

void
parse_argv (struct options* options,
	    int argc,
	    char *argv[])
{
  int i, j;

  /* parse command line */
  for (i = 1; i < argc; ++i)
    {
      char *p[MAX_PARMS];
      CLEAR (p);
      p[0] = argv[i];
      if (strncmp(p[0], "--", 2))
	{
          /* Missing -- before option. */
	  ShowLocalizedMsg(GUI_NAME, ERR_BAD_PARAMETER, p[0]);
	  exit(0);
        }
      else
	p[0] += 2;

      for (j = 1; j < MAX_PARMS; ++j)
	{
	  if (i + j < argc)
	    {
	      char *arg = argv[i + j];
	      if (strncmp (arg, "--", 2))
		p[j] = arg;
	      else
		break;
	    }
	}
      i = add_option (options, i, p);
    }
}

static int
add_option (struct options *options,
	    int i,
	    char *p[])
{

  if (streq (p[0], "help"))
    {
      TCHAR usagetext[5000];
      TCHAR usagecaption[200];
      
      LoadString(o.hInstance, INFO_USAGE, usagetext, sizeof(usagetext) / sizeof(TCHAR));
      LoadString(o.hInstance, INFO_USAGECAPTION, usagecaption, sizeof(usagetext) / sizeof(TCHAR));
      MessageBox(NULL, usagetext, usagecaption, MB_OK);
      exit(0);
    }
  else if (streq (p[0], "connect") && p[1])
    {
      ++i;
      static int auto_connect_nr=0;
      if (auto_connect_nr == MAX_CONFIGS)
        {
          /* Too many configs */
          ShowLocalizedMsg(GUI_NAME, ERR_TO_MANY_CONFIGS, MAX_CONFIGS);
          exit(1);
        }

      options->auto_connect[auto_connect_nr] = p[1];
      auto_connect_nr++;
    }
  else if (streq (p[0], "exe_path") && p[1])
    {
      ++i;
      strncpy(options->exe_path, p[1], sizeof(options->exe_path) - 1);
    }
  else if (streq (p[0], "config_dir") && p[1])
    {
      ++i;
      strncpy(options->config_dir, p[1], sizeof(options->config_dir) - 1);
    }
  else if (streq (p[0], "ext_string") && p[1])
    {
      ++i;
      strncpy(options->ext_string, p[1], sizeof(options->ext_string) - 1);
    }
  else if (streq (p[0], "log_dir") && p[1])
    {
      ++i;
      strncpy(options->log_dir, p[1], sizeof(options->log_dir) - 1);
    }
  else if (streq (p[0], "priority_string") && p[1])
    {
      ++i;
      strncpy(options->priority_string, p[1], sizeof(options->priority_string) - 1);
    }
  else if (streq (p[0], "append_string") && p[1])
    {
      ++i;
      strncpy(options->append_string, p[1], sizeof(options->append_string) - 1);
    }
  else if (streq (p[0], "log_viewer") && p[1])
    {
      ++i;
      strncpy(options->log_viewer, p[1], sizeof(options->log_viewer) - 1);
    }
  else if (streq (p[0], "editor") && p[1])
    {
      ++i;
      strncpy(options->editor, p[1], sizeof(options->editor) - 1);
    }
  else if (streq (p[0], "allow_edit") && p[1])
    {
      ++i;
      strncpy(options->allow_edit, p[1], sizeof(options->allow_edit) - 1);
    }
  else if (streq (p[0], "allow_service") && p[1])
    {
      ++i;
      strncpy(options->allow_service, p[1], sizeof(options->allow_service) - 1);
    }
  else if (streq (p[0], "allow_password") && p[1])
    {
      ++i;
      strncpy(options->allow_password, p[1], sizeof(options->allow_password) - 1);
    }
  else if (streq (p[0], "allow_proxy") && p[1])
    {
      ++i;
      strncpy(options->allow_proxy, p[1], sizeof(options->allow_proxy) - 1);
    }
  else if (streq (p[0], "show_balloon") && p[1])
    {
      ++i;
      strncpy(options->show_balloon, p[1], sizeof(options->show_balloon) - 1);
    }
  else if (streq (p[0], "service_only") && p[1])
    {
      ++i;
      strncpy(options->service_only, p[1], sizeof(options->service_only) - 1);
    }
  else if (streq (p[0], "show_script_window") && p[1])
    {
      ++i;
      strncpy(options->show_script_window, p[1], sizeof(options->show_script_window) - 1);
    }
  else if (streq (p[0], "silent_connection") && p[1])
    {
      ++i;
      strncpy(options->silent_connection, p[1], sizeof(options->silent_connection) - 1);
    }
  else if (streq (p[0], "passphrase_attempts") && p[1])
    {
      ++i;
      strncpy(options->psw_attempts_string, p[1], sizeof(options->psw_attempts_string) - 1);
    }
  else if (streq (p[0], "connectscript_timeout") && p[1])
    {
      ++i;
      strncpy(options->connectscript_timeout_string, p[1], sizeof(options->connectscript_timeout_string) - 1);
    }
  else if (streq (p[0], "disconnectscript_timeout") && p[1])
    {
      ++i;
      strncpy(options->disconnectscript_timeout_string, p[1], 
              sizeof(options->disconnectscript_timeout_string) - 1);
    }
  else if (streq (p[0], "preconnectscript_timeout") && p[1])
    {
      ++i;
      strncpy(options->preconnectscript_timeout_string, p[1], 
              sizeof(options->preconnectscript_timeout_string) - 1);
    }
  else
    {
        /* Unrecognized option or missing parameter */
	ShowLocalizedMsg(GUI_NAME, ERR_BAD_OPTION, p[0]);
        exit(1);
    }
  return i;
}

/*
 * Returns TRUE if option exist in config file.
 */
int ConfigFileOptionExist(int config, const char *option)
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
      if ((strncmp(line, option, sizeof(option)) == 0))
        {
          fclose(fp);
          return(1);
        }
    }

  fclose(fp);
  return(0);
}

