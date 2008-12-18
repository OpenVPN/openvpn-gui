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
#include "openvpn-gui-res.h"
#include "options.h"

#define MATCH_FALSE	0
#define MATCH_FILE	1
#define	MATCH_DIR	2

extern struct options o;

static int
match (const WIN32_FIND_DATA *find, const char *ext)
{
  int i;

  if (find->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    return MATCH_DIR;

  if (!strlen (ext))
    return MATCH_FILE;

  i = strlen (find->cFileName) - strlen (ext) - 1;
  if (i < 1)
    return MATCH_FALSE;

  if (find->cFileName[i] == '.' && !strcasecmp (find->cFileName + i + 1, ext))
    return MATCH_FILE;
  else
    return MATCH_FALSE;
}

/*
 * Modify the extension on a filename.
 */
static bool
modext (char *dest, unsigned int size, const char *src, const char *newext)
{
  int i;

  if (size > 0 && (strlen (src) + 1) <= size)
    {
      strcpy (dest, src);
      dest [size - 1] = '\0';
      i = strlen (dest);
      while (--i >= 0)
	{
	  if (dest[i] == '\\')
	    break;
	  if (dest[i] == '.')
	    {
	      dest[i] = '\0';
	      break;
	    }
	}
      if (strlen (dest) + strlen(newext) + 2 <= size)
	{
	  strcat (dest, ".");
	  strcat (dest, newext);
	  return true;
	}
      dest [0] = '\0';
    }
  return false;
}

int ConfigAlreadyExists(char newconfig[])
{
  int i;

  for (i=0; i<o.num_configs; i++)
    {
      if (strcasecmp(o.cnn[i].config_file, newconfig) == 0)
        return true;
    }

  return false;
}

int AddConfigFileToList(int config, char filename[], char config_dir[])
{
  char log_file[MAX_PATH];
  int i;

  /* Save config file name */
  strncpy(o.cnn[config].config_file, filename, sizeof(o.cnn[config].config_file));

  /* Save config dir */
  strncpy(o.cnn[config].config_dir, config_dir, sizeof(o.cnn[config].config_dir));

  /* Save connection name (config_name - extension) */
  strncpy(o.cnn[config].config_name, o.cnn[config].config_file,
          sizeof(o.cnn[config].config_name));
  o.cnn[config].config_name[strlen(o.cnn[config].config_name) - 
                                   (strlen(o.ext_string)+1)]=0;

  /* get log file pathname */
  if (!modext (log_file, sizeof (log_file), o.cnn[config].config_file, "log"))
    {
      /* cannot construct logfile-name */
      ShowLocalizedMsg (GUI_NAME, ERR_CANNOT_CONSTRUCT_LOG, o.cnn[config].config_file);
      return(false);
    }
  mysnprintf (o.cnn[config].log_path, "%s\\%s", o.log_dir, log_file);

  /* Check if connection should be autostarted */
  for (i=0; (i < MAX_CONFIGS) && o.auto_connect[i]; i++)
    {
      if (strcasecmp(o.cnn[config].config_file, o.auto_connect[i]) == 0)
        {
          o.cnn[config].auto_connect = true;
          break;
        }
    }

  return(true);
}


int
BuildFileList()
{
  WIN32_FIND_DATA find_obj;
  HANDLE find_handle;
  BOOL more_files;
  char find_string[MAX_PATH];
  int i;
  char subdir_table[MAX_CONFIG_SUBDIRS][MAX_PATH];
  int subdir=0;
  int subdir_counter=0;

  /* Reset config counter */
  o.num_configs=0;
  
  mysnprintf (find_string, "%s\\*", o.config_dir);

  find_handle = FindFirstFile (find_string, &find_obj);
  if (find_handle == INVALID_HANDLE_VALUE)
    {
      return(true);
    }

  /*
   * Loop over each config file in main config dir
   */
  do
    {
      if (o.num_configs >= MAX_CONFIGS)
        {
          /* too many configs */
          ShowLocalizedMsg(GUI_NAME, ERR_TO_MANY_CONFIGS, MAX_CONFIGS);
          break;
        }

      /* does file have the correct type and extension? */
      if (match (&find_obj, o.ext_string) == MATCH_FILE)
        {
          /* Add config file to list */
          AddConfigFileToList(o.num_configs, find_obj.cFileName, o.config_dir); 

          o.num_configs++;
        }

      if (match (&find_obj, o.ext_string) == MATCH_DIR)
        {
          if ((strncmp(find_obj.cFileName, ".", strlen(find_obj.cFileName)) != 0) &&
              (strncmp(find_obj.cFileName, "..", strlen(find_obj.cFileName)) != 0) &&
              (subdir < MAX_CONFIG_SUBDIRS))
            {
              /* Add dir to dir_table */
              mysnprintf(subdir_table[subdir], "%s\\%s", o.config_dir, find_obj.cFileName);
              subdir++;
            }
        }

      /* more files to process? */
      more_files = FindNextFile (find_handle, &find_obj);
    } while (more_files);
    
  FindClose (find_handle);


 /*
  * Loop over each config file in every subdir
  */
  for (subdir_counter=0; subdir_counter < subdir; subdir_counter++)
    {

      mysnprintf (find_string, "%s\\*", subdir_table[subdir_counter]);

      find_handle = FindFirstFile (find_string, &find_obj);
      if (find_handle == INVALID_HANDLE_VALUE)
        continue;

      do
        { 
          if (o.num_configs >= MAX_CONFIGS)
            {
              /* too many configs */
              ShowLocalizedMsg(GUI_NAME, ERR_TO_MANY_CONFIGS, MAX_CONFIGS);
              FindClose (find_handle);
              return(true);
            }

          /* does file have the correct type and extension? */
          if (match (&find_obj, o.ext_string) == MATCH_FILE)
            {
              if (!ConfigAlreadyExists(find_obj.cFileName))
                {
                  /* Add config file to list */
                  AddConfigFileToList(o.num_configs, find_obj.cFileName, subdir_table[subdir_counter]); 
                  o.num_configs++;
                }
              else
                {
                  /* Config filename already exists */
                  ShowLocalizedMsg(GUI_NAME, ERR_CONFIG_ALREADY_EXIST, find_obj.cFileName);
                }
            }

          /* more files to process? */
          more_files = FindNextFile (find_handle, &find_obj);
        } while (more_files);
       
      FindClose (find_handle);
    }

  return (true);
}

