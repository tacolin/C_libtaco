#include "basic.h"

#include <unistd.h>

#include "slock.h"

#include <lua5.2/lua.h>
#include <lua5.2/lauxlib.h>

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char const *argv[])
{
    lua_State* L = luaL_newstate();
    luaL_openlibs(L);

    int chk = luaL_loadfile(L, "file.lua");
    check_if(chk != LUA_OK, goto _ERROR, "luaL_loadfile failed");

    chk = lua_pcall(L, 0, 0, 0);
    check_if(chk != LUA_OK, goto _ERROR, "lua_pcall failed : %s", lua_tostring(L, -1));

    lua_close(L);
    dprint("ok");
    return 0;

_ERROR:
    lua_close(L);
    dprint("fail");
    return -1;
}
