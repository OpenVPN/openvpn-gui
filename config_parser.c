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

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <wchar.h>
#include "main.h"
#include "misc.h"
#include "config_parser.h"

static int
legal_escape(wchar_t c)
{
    wchar_t *escapes = L"\" \\"; /* ", space, and backslash */
    return (wcschr(escapes, c) != NULL);
}

static int
is_comment(wchar_t *s)
{
    wchar_t *comment_chars = L";#";
    return (s && (wcschr(comment_chars, s[0]) != NULL));
}

static int
copy_token(wchar_t **dest, wchar_t **src, wchar_t *delim)
{
    wchar_t *p = *src;
    wchar_t *s = *dest;

    /* copy src to dest until delim character with escaped chars converted */
    for (; *p != L'\0' && wcschr(delim, *p) == NULL; p++, s++)
    {
        if (*p == L'\\' && legal_escape(*(p + 1)))
        {
            *s = *(++p);
        }
        else if (*p == L'\\')
        {
            MsgToEventLog(EVENTLOG_ERROR_TYPE, L"Parse error in copy_token: illegal backslash");
            return -1; /* parse error -- illegal backslash in input */
        }
        else
        {
            *s = *p;
        }
    }
    /* at this point p is one of the delimiters or null */
    *s = L'\0';
    *src = p;
    *dest = s;
    return 0;
}

static int
tokenize(config_entry_t *ce)
{
    wchar_t *p, *s;
    p = ce->line;
    s = ce->sline;
    unsigned int i = 0;
    int status = 0;

    for (; *p != L'\0'; p++, s++)
    {
        if (*p == L' ' || *p == L'\t')
        {
            continue;
        }

        if (_countof(ce->tokens) <= i)
        {
            return -1;
        }
        ce->tokens[i++] = s;

        if (*p == L'\'')
        {
            int len = wcscspn(++p, L"\'");
            wcsncpy(s, p, len);
            s += len;
            p += len;
        }
        else if (*p == L'\"')
        {
            p++;
            status = copy_token(&s, &p, L"\"");
        }
        else if (is_comment(p))
        {
            /* store rest of the line as comment -- remove from tokens */
            ce->comment = s;
            wcsncpy(s, p, wcslen(p));
            ce->tokens[--i] = NULL;
            break;
        }
        else
        {
            status = copy_token(&s, &p, L" \t");
        }

        if (status != 0)
        {
            return status;
        }

        if (*p == L'\0')
        {
            break;
        }
    }
    ce->ntokens = i;
    return 0;
}

config_entry_t *
config_readline(FILE *fd, int first)
{
    int len;
    char tmp[MAX_LINE_LENGTH];
    int offset = 0;

    if (fgets(tmp, _countof(tmp) - 1, fd) == NULL)
    {
        return NULL;
    }
    /* remove UTF-8 BOM */
    if (first && strncmp(tmp, "\xEF\xBB\xBF", 3) == 0)
    {
        offset = 3;
    }

    config_entry_t *ce = calloc(sizeof(*ce), 1);
    if (!ce)
    {
        MsgToEventLog(EVENTLOG_ERROR_TYPE, L"Out of memory in config_readline");
        return NULL;
    }

    mbstowcs(ce->line, &tmp[offset], _countof(ce->line) - 1);

    len = wcscspn(ce->line, L"\n\r");
    ce->line[len] = L'\0';

    if (tokenize(ce) != 0)
    {
        free(ce);
        return NULL;
    }

    /* skip leading "--" in first token if any */
    if (ce->ntokens > 0)
    {
        ce->tokens[0] += wcsspn(ce->tokens[0], L"--");
    }

    return ce;
}

config_entry_t *
config_parse(wchar_t *fname)
{
    FILE *fd = NULL;
    config_entry_t *head, *tail;

    if (!fname || _wfopen_s(&fd, fname, L"r"))
    {
        MsgToEventLog(EVENTLOG_ERROR_TYPE, L"Error opening <%ls> in config_parse", fname);
        return NULL;
    }
    head = tail = config_readline(fd, 1);

    while (tail)
    {
        tail->next = config_readline(fd, 0);
        tail = tail->next;
    }
    fclose(fd);
    return head;
}

void
config_list_free(config_entry_t *head)
{
    config_entry_t *next;
    while (head)
    {
        next = head->next;
        free(head);
        head = next;
    }
    return;
}
