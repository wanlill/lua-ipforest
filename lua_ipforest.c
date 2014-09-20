/* Lua IP FOREST - COLLECTION OF RADIX IP TREES FOR LUA
 *
 * Copyright (c) 2010-2014  tcpper <tcpper@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/* Caveats:
 * - ip radix tree is presented by lightuserdata in lua.
 * - trees are put into a forest table.
 * - only support load_tree from file and match_tree
 * - ip file can be of the following format
 *   - 192.168.0.10-30
 *   - 192.168.0.10-192.168.1.300
 *   - 192.168.0.0/24
 *   - 192.168.0.0/255.255.255.0
 *   - 192.168.0.9
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <math.h>
#include <limits.h>
#include <lua.h>
#include <lauxlib.h>
#include "ipforest_types.h"
#include "ipforest_parser.h"
#include "ipforest_radix_tree.h"


#ifndef IPFOREST_MODNAME
#define IPFOREST_MODNAME   "ipforest"
#endif

#ifndef IPFOREST_VERSION
#define IPFOREST_VERSION   "0.1.0"
#endif



#ifndef IPFOREST_IDX
static const char IPFOREST = 'K';
#define IPFOREST_IDX ((void *)&IPFOREST)
#endif

inline static int
_get_forest_table(lua_State *l)
{
    lua_pushlightuserdata(l, IPFOREST_IDX);
    lua_gettable(l, LUA_REGISTRYINDEX);
    return 1;
}

/* 
 * push light user data associated with the tree onto stack if found
 */
inline static int
_find_tree(lua_State *l, const char *tname)
{
    _get_forest_table(l);
    lua_getfield(l, -1, tname);
    if (lua_isnil(l, -1)) {
        lua_pop(l, 2);
        return IPFOREST_FALSE;
    } else {
        lua_remove(l, -2);
        return IPFOREST_TRUE;
    }
}

/*
 * free what is associated with the light user data on the top of the stack
 * tree should be placed on the top of the stack before called which will be
 * poped from stack before ret
 */
inline static int
_free_tree(lua_State *l, const char *tname)
{
    ipforest_radix_tree_t *tree;
    /* deal with light user data */
    tree = lua_touserdata(l, -1);
    ipforest_radix_tree_free(tree);

    /* pop light user data */
    lua_pop(l, 1);

    /* clean up forest table */
    _get_forest_table(l);
    lua_pushnil(l);
    lua_setfield(l, -2, tname);
    lua_pop(l, 1);

    return 0;
}

/*
 * compact what is associated with the light user data on the top of the stack
 * tree should be placed on the top of the stack before called which will be
 * poped from stack before ret
 */
inline static int
_compact_tree(lua_State *l, const char *tname)
{
    ipforest_radix_tree_t *tree;
    /* deal with light user data */
    tree = lua_touserdata(l, -1);
    ipforest_radix_tree_compact(tree);

    /* pop light user data */
    lua_pop(l, 1);

    return 0;
}

inline static size_t
log_2(size_t num)
{
    int i;

    i = 0;

    assert(num > 0);

    while(num > 0) {
        num = num >> 1;
        i++;
    }
    return i - 1;
}

inline static IPFOREST_BOOLEAN
_append_tree(ipforest_radix_tree_t *tree, const char *buf)
{
    int i, count;
    ipforest_ipaddr_t *paddr;

    /* initial ipforest_ipaddr_t array size */
    count = 0;
    paddr = NULL;

    count = ipforest_parse_ip_line(buf, NULL);
    if (count <= 0 ) {
        goto fail;
    }

    paddr = malloc(count * sizeof(ipforest_ipaddr_t));
    if (!paddr) {
        goto fail;
    }

    ipforest_parse_ip_line(buf, paddr);

    /* printf("count: %d\n", tmp); */

    for (i = 0; i < count; i++) {
        /* printf("0x%08x, 0x%08x\n", paddr[i].addr, paddr[i].mask); */
        if (ipforest_radix_tree_insert(tree, paddr[i].addr, paddr[i].mask) < 0) {
            goto fail;
        }
    }

    free(paddr);
    return IPFOREST_TRUE;
    
fail:
    /* safe to free NULL */
    free(paddr);
    return IPFOREST_FALSE;
}

/* push light user data associated with the tree onto stack if created */
inline static IPFOREST_BOOLEAN
_create_tree(lua_State *l)
{
   ipforest_radix_tree_t *tree;
    /* create a tree */
    tree = ipforest_radix_tree_alloc();
    if (!tree) {
        goto fail;
    }

    lua_pushlightuserdata(l, tree);
    return IPFOREST_TRUE;

fail:
    return IPFOREST_FALSE;
}

/* push light user data associated with the tree onto stack if load */
inline static IPFOREST_BOOLEAN
_load_tree(lua_State *l, const char *fname)
{
    size_t len;
    char buf[LINE_MAX];
    FILE *stream;
    ipforest_radix_tree_t *tree;

    stream = fopen(fname, "r");
    if (!stream) {
        goto open_error;
    }

    if (!_create_tree(l)) {
        goto before_parse_error;
    }

    tree = lua_touserdata(l, -1);
    lua_pop(l, 1);

    while (fgets(buf, LINE_MAX, stream)) {
        len = strlen(buf);
        if (!feof(stream)) {
            if (buf[len - 1] != '\n') {
                goto parse_error;
            }
        }
     
        /* maybe eof and a last '\n' */
        if (buf[len - 1] == '\n') {
            if (len > 1 && buf[len - 2] == '\r') {
                buf[len - 2] = '\0';
            } else {
                buf[len - 1] = '\0';
            }
        }

        /* ignore empty line and comments */
        if (buf[0] == '#' || buf[0] == '\0') {
            continue;
        }

        if (!_append_tree(tree, buf)) {
            goto parse_error;
        }
    }

    /* compact tree to delete free node */
    ipforest_radix_tree_compact(tree);

    /* close file */
    fclose(stream);

    lua_pushlightuserdata(l, tree);
    return IPFOREST_TRUE;

 parse_error:
    ipforest_radix_tree_free(tree);

 before_parse_error:
    fclose(stream);

 open_error:
    return IPFOREST_FALSE;
}

static int
load_tree(lua_State *l)
{
    const char *tname, *fname;
    size_t tname_len, fname_len;

    tname = luaL_checklstring(l, 1, &tname_len);
    fname = luaL_checklstring(l, 2, &fname_len);

    if (tname_len <= 0 || fname_len <= 0) {
        goto fail;
    }

    if (_find_tree(l, tname)) {
        _free_tree(l, tname);
    }

    /* push forest table onto stack */
    _get_forest_table(l);
    /* push new created tree onto stack */
    if (_load_tree(l, fname)) {
        lua_setfield(l, -2, tname);
        /* pop forest table from stack */
        lua_pop(l, 1);
    } else {
        /* pop forest table from stack */
        lua_pop(l, 1);
        goto fail;
    }
    
    lua_pushboolean(l, IPFOREST_TRUE);
    return 1;

fail:
    /* we don't use error for it is difficult to be tested using lunit */
    /* lua_pushstring(l, "load tree error"); */
    /* lua_error(l); */

    lua_pushboolean(l, IPFOREST_FALSE);
    return 1;
}

static int
reset_tree(lua_State *l)
{
    const char *tname;
    size_t tname_len;

    tname = luaL_checklstring(l, 1, &tname_len);

    if (tname_len <= 0) {
        goto fail;
    }

    if (_find_tree(l, tname)) {
        _free_tree(l, tname);
    }

    /* push forest table onto stack */
    _get_forest_table(l);
    /* push new created tree onto stack */
    if (_create_tree(l)) {
        lua_setfield(l, -2, tname);
        /* pop forest table from stack */
        lua_pop(l, 1);
    } else {
        /* pop forest table from stack */
        lua_pop(l, 1);
        goto fail;
    }
    
    lua_pushboolean(l, IPFOREST_TRUE);
    return 1;

fail:
    /* we don't use error for it is difficult to be tested using lunit */
    /* lua_pushstring(l, "create tree error"); */
    /* lua_error(l); */

    lua_pushboolean(l, IPFOREST_FALSE);
    return 1;
}

static int
append_tree(lua_State *l)
{
    const char *tname, *buf;
    size_t tname_len, buf_len;
    ipforest_radix_tree_t *tree;

    tname = luaL_checklstring(l, 1, &tname_len);
    buf = luaL_checklstring(l, 2, &buf_len);

    if (tname_len <= 0 || buf_len <= 0) {
        goto fail;
    }

    if (!_find_tree(l, tname)) {
        goto fail;
    }

    tree = lua_touserdata(l, -1);
    if (_append_tree(tree, buf)) {
        lua_pop(l, 1);
        lua_pushboolean(l, IPFOREST_TRUE);
        return 1;
    }

    lua_pop(l, 1);

fail: 
    lua_pushboolean(l, IPFOREST_FALSE);
    return 1;
}

static int
has_tree(lua_State *l)
{
    const char *tname;
    size_t tname_len;
    tname = luaL_checklstring(l, 1, &tname_len);

    if (_find_tree(l, tname)) {
        /* pop light user data */
        lua_pop(l, 1);

        lua_pushboolean(l, IPFOREST_TRUE);
        return 1;
    }

    lua_pushboolean(l, IPFOREST_FALSE);
    return 1;
}

static int
free_tree(lua_State *l)
{
    const char *tname;
    size_t tname_len;
    tname = luaL_checklstring(l, 1, &tname_len);

    if (_find_tree(l, tname)) {
        _free_tree(l, tname);
        lua_pushboolean(l, IPFOREST_TRUE);
        return 1;
    }

    lua_pushboolean(l, IPFOREST_FALSE);
    return 1;
}

static int
compact_tree(lua_State *l)
{
    const char *tname;
    size_t tname_len;
    tname = luaL_checklstring(l, 1, &tname_len);

    if (_find_tree(l, tname)) {
        _compact_tree(l, tname);
        lua_pushboolean(l, IPFOREST_TRUE);
        return 1;
    }

    lua_pushboolean(l, IPFOREST_FALSE);
    return 1;
}

static int
match_tree(lua_State *l)
{

    struct in_addr addr;
    const char *tname, *ipstr;
    size_t tname_len, ipstr_len;
    ipforest_radix_tree_t *tree;

    tname = luaL_checklstring(l, 1, &tname_len);
    ipstr = luaL_checklstring(l, 2, &ipstr_len);

    if (tname_len <= 0 || ipstr_len <= 0) {
        goto fail;
    }

    if (_find_tree(l, tname)) {
        tree = lua_touserdata(l, -1);
        assert(tree);
        if (inet_aton(ipstr, &addr) > 0) {
            /* do a 32 bit mask lookup */
            if (ipforest_radix_tree_lookup(tree, ntohl(addr.s_addr), 0xffffffff)) {
                /* pop light user data */
                lua_pop(l, 1);
                lua_pushboolean(l, IPFOREST_TRUE);
                return 1;
            }
        }
        lua_pop(l, 1);
    }

 fail:
    lua_pushboolean(l, IPFOREST_FALSE);
    return 1;
}

/* Return ipforest module table */
static int
lua_ipforest_new(lua_State *l)
{
    luaL_Reg *preg;
    luaL_Reg reg[] = {
        { "reset", reset_tree },
        { "load", load_tree },
        { "append", append_tree },
        { "has", has_tree },
        { "free", free_tree },
        { "match", match_tree },
        { "compact", compact_tree },
        { NULL, NULL }
    };

    /* ipforest module table */
    lua_newtable(l);

    /* create global forest table */
    lua_pushlightuserdata(l, IPFOREST_IDX);
    lua_newtable(l);
    lua_settable(l, LUA_REGISTRYINDEX);

    /* set module name / version fields */
    lua_pushliteral(l, IPFOREST_MODNAME);
    lua_setfield(l, -2, "_NAME");
    lua_pushliteral(l, IPFOREST_VERSION);
    lua_setfield(l, -2, "_VERSION");

    /* register cfunctions */
    for (preg = reg; preg->name != NULL; preg++) {
        lua_pushstring(l, preg->name);
        lua_pushcfunction(l, preg->func);
        lua_settable(l, -3);
    }

    return 1;
}

int
luaopen_ipforest(lua_State *l)
{
    lua_ipforest_new(l);

#ifdef ENABLE_IPFOREST_GLOBAL
    /* Register a global "ipforest" table. */
    lua_pushvalue(l, -1);
    lua_setglobal(l, IPFOREST_MODNAME);
#endif

    /* Return cjson table */
    return 1;
}

/* vi:ai et sw=4 ts=4:
 */
