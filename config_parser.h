/*
 *  This file is a part of OpenVPN-GUI -- A Windows GUI for OpenVPN.
 *
 *  Copyright (C) 2016 Selva Nair <selva.nair@gmail.com>
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

#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#define MAX_LINE_LENGTH 256

typedef struct config_entry config_entry_t;

struct config_entry
{
    wchar_t line[MAX_LINE_LENGTH];
    wchar_t sline[MAX_LINE_LENGTH];
    wchar_t *tokens[16];
    wchar_t *comment;
    int ntokens;
    config_entry_t *next;
};

/**
 * Parse an ovpn file into a list of tokenized
 * structs.
 * @param fname : filename of the config to parse
 * @returns the pointer to the head of a list of
 *          config_entry_t structs.
 *          The called must free it after use by calling
 *          config_list_free()
 */
config_entry_t *config_parse(wchar_t *fname);

/**
 * Free a list of config_entry_t structs
 * @param head : list head returned by config_parse()
 */
void config_list_free(config_entry_t *head);

#endif /* ifndef CONFIG_PARSER_H */
