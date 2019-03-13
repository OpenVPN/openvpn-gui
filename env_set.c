/*
 *  OpenVPN-GUI -- A Windows GUI for OpenVPN.
 *
 *  Copyright (C) 2017 Selva Nair <selva.nair@gmail.com>
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

#include <windows.h>
#include <wchar.h>
#include "main.h"
#include "options.h"
#include "misc.h"
#include "openvpn.h"
#include "env_set.h"

struct env_item {
    wchar_t *nameval;
    struct env_item *next;
};

/* To match with openvpn we accept only :ALPHA:, :DIGIT: or '_' in names */
BOOL
is_valid_env_name(const char *name)
{
    if (name[0] == '\0')
    {
        return false;
    }
    while (*name)
    {
        const char c = *name;
        if (!isalnum(c) && c != '_')
        {
            return false;
        }
        name++;
    }
    return true;
}

/* Compare two strings of the form name1=val and name2=val
 * return value is -ve, 0 or +ve for name1 less than, equal
 * to or greater than name2.
 * The comparison is locale-independent and case insensitive.
 * If '=' is missing the whole string is used as name.
 */
int
env_name_compare(const wchar_t *nameval1, const wchar_t *nameval2)
{
    size_t len1 = wcscspn(nameval1, L"=");
    size_t len2 = wcscspn(nameval2, L"=");

    /* For env variable names we want locale independent case-insensitive
     * unicode string comparsion. Use CompareStringOrdinal following
     * https://msdn.microsoft.com/en-us/library/windows/desktop/dd318144(v=vs.85).aspx
     */
    BOOL ignore_case = true;
    int cmp = CompareStringOrdinal(nameval1, (int)len1, nameval2, (int)len2, ignore_case);

    return cmp - 2; /* -2 to bring the result match strcmp semantics */
}

static void
env_item_free(struct env_item *item)
{
    free(item->nameval);
    free(item);
}

/* Delete an env var item with matching name: if name is of the
 * form xxx=yyy, only the part xxx is used for matching.
 * Returns the head of the list.
 */
static struct env_item *
env_item_del(struct env_item *head, const wchar_t *name)
{
    struct env_item *item, *prev = NULL;

    if (!name) return head;

    for (item = head; item; item = item->next)
    {
        if (env_name_compare(name, item->nameval) == 0)
        {
            /* matching item found */
            if (prev)
                prev->next = item->next;
            else /* head is going to be deleted */
                head = item->next;
            env_item_free(item);
            break;
        }
        prev = item;
        continue;
    }

    return head;
}

/* Insert a env var item to an env set: any existing item
 * with same name is replaced by the new entry. Else
 * the item is added at an alphabetically sorted location.
 * Returns the new head of the list.
 */
struct env_item *
env_item_insert(struct env_item *head, struct env_item *item)
{
    struct env_item *tmp, *prev = NULL;
    int cmp = -1;

    /* find location of new item in sorted order: head == NULL is ok */
    for (tmp = head; tmp; tmp = tmp->next)
    {
        cmp = env_name_compare(item->nameval, tmp->nameval);
        if (cmp <= 0) /* found the position to add */
        {
             break;
        }
        prev = tmp;
        continue;
    }
    if (cmp == 0) /* name already set -- replace */
    {
        item->next = tmp->next;
        env_item_free(tmp);
    }
    else /* add item at this point */
    {
        item->next = tmp;
    }

    if (prev)
        prev->next = item;
    else
        head = item;

    return head;
}

void
env_item_del_all(struct env_item *head)
{
    struct env_item *next;
    for ( ; head; head = next)
    {
        next = head->next;
        env_item_free(head);
    }
}

/* convenience functions for create, add and delete an item
 * given nameval as a utf8 string.
 */

/* Create a new env item given nameval name=val */
struct env_item *
env_item_new_utf8(const char *nameval)
{
    struct env_item *new = malloc(sizeof(struct env_item));

    if (!new)
    {
        return NULL;
    }
    new->nameval = Widen(nameval);
    new->next = NULL;

    if (!new->nameval)
    {
        free(new);
        return NULL;
    }
    return new;
}

/* Insert an env item to the set given nameval: name=val.
 * Returns new head of the list.
 */
static struct env_item*
env_item_insert_utf8(struct env_item *head, const char *nameval)
{
    struct env_item *item = env_item_new_utf8(nameval);

    if (!item)
        return head;

    return env_item_insert(head, item);
}

/* Delete an env item from the list with matching name. Returns the
 * new head of the list. If name is given as name=val, only the
 * name part is used for matching.
 */
static struct env_item *
env_item_del_utf8(struct env_item *head, const char *name)
{
    wchar_t *wname = Widen(name);

    if (wname)
    {
        head = env_item_del(head, wname);
        free(wname);
    }
    return head;
}

/*
 * Make an env block by merging items in es to the process env block
 * retaining alphabetical order as necessary on Windows.
 * Returns NULL on error or a newly allocated string that may be passed
 * to CreateProcess as the env block. The caller must free the returned
 * pointer.
 */
wchar_t *
merge_env_block(const struct env_item *es)
{
    size_t len = 0;
    /* e should be treated as read-only though cannot be defined as const
     * due to the need to call FreeEnvironmentStrings in the end.
     */
    wchar_t *e = GetEnvironmentStringsW();
    const struct env_item *item;
    const wchar_t *pe;

    if (!e)
    {
        return NULL;
    }

    for (pe = e; *pe; pe += wcslen(pe)+1)
    {
       ;
    }
    len = (pe + 1 - e); /* including the extra '\0' at the end */

    for(item = es; item; item = item->next)
    {
        len += wcslen(item->nameval) + 1;
    }

    wchar_t *env = malloc(sizeof(wchar_t)*len);
    if (!env)
    {
        /* no memory -- return NULL */
        FreeEnvironmentStringsW(e);
        return NULL;
    }

    wchar_t *p = env;
    item = es;
    pe = e;
    len = wcslen(pe) + 1;

    /* Merge two sroted collections env set and process env.
     * In case of duplicates the env set entry replaces that in the
     * process env.
     */
    while (item && *pe)
    {
        int cmp = env_name_compare(item->nameval, pe);
        if (cmp <= 0) /* add entry from env set */
        {
            wcscpy(p, item->nameval);
            p += wcslen(item->nameval) + 1;
            item = item->next;
        }
        else  /* add entry from process env */
        {
            wcscpy(p, pe);
            p += len;
        }
        if (cmp >= 0) /* pe was added (cmp >0) or has to be skipped (cmp==0) */
        {
            pe += len;
            if (*pe) /* update len */
                len = wcslen(pe) + 1;
        }
    }
    /* Add any remaining entries -- either item or *pe is NULL at this point.
     * So only one of the two following loops will run.
     */
    for ( ; item; item = item->next)
    {
        wcscpy(p, item->nameval);
        p += wcslen(item->nameval) + 1;
    }
    for ( ; *pe; pe += len, p += len)
    {
        wcscpy(p, pe);
        len = wcslen(pe) + 1;
    }
    *p = L'\0';

    FreeEnvironmentStringsW(e);

    return env;
}

/* Expect "setenv name value" and add name=value
 * to a private env set with name prefixed by OPENVPN_.
 * If value is missing we delete name from the env set.
 */
void
process_setenv(connection_t *c, UNUSED time_t timestamp, const char *msg)
{
    const char *prefix = "OPENVPN_";
    char *p;
    char *nameval;

    if (!strbegins(msg, "setenv "))
        return;

    msg += strlen("setenv ");    /* character following "setenv" */
    msg += strspn(msg, " \t");   /* skip leading space */
    if (msg[0] == '\0')
    {
        WriteStatusLog(c, L"GUI> ", L"Error: Name empty in echo setenv", false);
        return;
    }

    nameval = malloc(strlen(prefix) + strlen(msg) + 1);
    if (!nameval)
    {
        WriteStatusLog(c, L"GUI> ", L"Error: Out of memory for adding env var", false);
        return;
    }

    strcpy(nameval, prefix);
    strcat(nameval, msg);

    if ((p = strchr(nameval, ' ')) != NULL)
    {
        *p = '\0'; /* temporary null termination */
        if (is_valid_env_name(nameval))
        {
            *p = '=';
            c->es = env_item_insert_utf8(c->es, nameval);
        }
        else
        {
            *p = ' ';
            WriteStatusLog(c, L"GUI> ", L"Error: empty or illegal name in echo setenv", false);
        }
    }
    /* if only name is specified and valid, delete the value from env set */
    else if (is_valid_env_name(nameval))
    {
        c->es = env_item_del_utf8(c->es, nameval);
    }
    free(nameval); /* env_item keeps a private wide string copy */
}
