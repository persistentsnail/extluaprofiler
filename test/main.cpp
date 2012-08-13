extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
int luaopen_kprofile(lua_State *L);
};

int main(int argc, char *argv[])
{
	lua_State *L = luaL_newstate();
	luaL_openlibs(L);
	luaopen_kprofile(L);
	luaL_dofile(L, "test.lua");
	lua_close(L);
	return 0;
}

