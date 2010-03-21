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

#include "config.h"
#include "main.h"
#include "openvpn-gui-res.h"
#include "options.h"
#include "localization.h"

#define MATCH_FALSE	0
#define MATCH_FILE	1
#define	MATCH_DIR	2

extern struct options o;

static int
match (const WIN32_FIND_DATA *find, const TCHAR *ext)
{
  int i;

  if (find->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    return MATCH_DIR;

  if (!_tcslen(ext))
    return MATCH_FILE;

  i = _tcslen(find->cFileName) - _tcslen(ext) - 1;
  if (i < 1)
    return MATCH_FALSE;

  if (find->cFileName[i] == '.' && !_tcsicmp(find->cFileName + i + 1, ext))
    return MATCH_FILE;
  else
    return MATCH_FALSE;
}

/*
 * Modify the extension on a filename.
 */
static bool
modext (TCHAR *dest, unsigned int size, const TCHAR *src, const TCHAR *newext)
{
  int i;

  if (size > 0 && (_tcslen(src) + 1) <= size)
    {
      _tcscpy(dest, src);
      dest[size - 1] = _T('\0');
      i = _tcslen(dest);
      while (--i >= 0)
	{
	  if (dest[i] == _T('\\'))
	    break;
	  if (dest[i] == _T('.'))
	    {
	      dest[i] = _T('\0');
	      break;
	    }
	}
      if (_tcslen(dest) + _tcslen(newext) + 2 <= size)
	{
	  _tcscat(dest, _T("."));
	  _tcscat(dest, newext);
	  return true;
	}
      dest[0] = _T('\0');
    }
  return false;
}

int ConfigAlreadyExists(TCHAR newconfig[])
{
  int i;

  for (i=0; i<o.num_configs; i++)
    {
      if (_tcsicmp(o.cnn[i].config_file, newconfig) == 0)
        return true;
    }

  return false;
}

int AddConfigFileToList(int config, TCHAR filename[], TCHAR config_dir[])
{
  TCHAR log_file[MAX_PATH];
  int i;

  /* Save config file name */
  _tcsncpy(o.cnn[config].config_file, filename, _tsizeof(o.cnn[config].config_file));

  /* Save config dir */
  _tcsncpy(o.cnn[config].config_dir, config_dir, _tsizeof(o.cnn[config].config_dir));

  /* Save connection name (config_name - extension) */
  _tcsncpy(o.cnn[config].config_name, o.cnn[config].config_file,
          _tsizeof(o.cnn[config].config_name));
  o.cnn[config].config_name[_tcslen(o.cnn[config].config_name) - 
                            _tcslen(o.ext_string) + 1] = _T('\0');

  /* get log file pathname */
  if (!modext(log_file, _tsizeof(log_file), o.cnn[config].config_file, _T("log")))
    {
      /* cannot construct logfile-name */
      ShowLocalizedMsg(IDS_ERR_LOG_CONSTRUCT, o.cnn[config].config_file);
      return(false);
    }
  _sntprintf_0(o.cnn[config].log_path, _T("%s\\%s"), o.log_dir, log_file);

  /* Check if connection should be autostarted */
  for (i=0; (i < MAX_CONFIGS) && o.auto_connect[i]; i++)
    {
      if (_tcsicmp(o.cnn[config].config_file, o.auto_connect[i]) == 0)
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
  TCHAR find_string[MAX_PATH];
  TCHAR subdir_table[MAX_CONFIG_SUBDIRS][MAX_PATH];
  int subdir=0;
  int subdir_counter=0;

  /* Reset config counter */
  o.num_configs=0;
  
  _sntprintf_0(find_string, _T("%s\\*"), o.config_dir);

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
          ShowLocalizedMsg(IDS_ERR_MANY_CONFIGS, MAX_CONFIGS);
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
          if ((_tcsncmp(find_obj.cFileName, _T("."), _tcslen(find_obj.cFileName)) != 0) &&
              (_tcsncmp(find_obj.cFileName, _T(".."), _tcslen(find_obj.cFileName)) != 0) &&
              (subdir < MAX_CONFIG_SUBDIRS))
            {
              /* Add dir to dir_table */
              _sntprintf_0(subdir_table[subdir], _T("%s\\%s"), o.config_dir, find_obj.cFileName);
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

      _sntprintf_0(find_string, _T("%s\\*"), subdir_table[subdir_counter]);

      find_handle = FindFirstFile (find_string, &find_obj);
      if (find_handle == INVALID_HANDLE_VALUE)
        continue;

      do
        { 
          if (o.num_configs >= MAX_CONFIGS)
            {
              /* too many configs */
              ShowLocalizedMsg(IDS_ERR_MANY_CONFIGS, MAX_CONFIGS);
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
                  ShowLocalizedMsg(IDS_ERR_CONFIG_EXIST, find_obj.cFileName);
                }
            }

          /* more files to process? */
          more_files = FindNextFile (find_handle, &find_obj);
        } while (more_files);
       
      FindClose (find_handle);
    }

  return (true);
}

