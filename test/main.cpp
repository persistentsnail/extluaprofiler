extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
int luaopen_elprofiler(lua_State *L);
};

int main(int argc, char *argv[])
{
	lua_State *L = luaL_newstate();
	luaL_openlibs(L);
	//luaopen_elprofiler(L);
	if (-1 == luaL_dofile(L, "test.lua"))
	{
		printf("%s\n", lua_tostring(L, -1));
	}
	lua_close(L);
	return 0;
}

