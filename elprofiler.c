#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lua.h>
#include <lauxlib.h>

#include "elprof_core.h"

static int exit_id;
static int profiler_is_running = 0;

static int profiler_stop(lua_State *L);
static void callhook(lua_State *L, lua_Debug *ar)
{
	int ret;
	if (!profiler_is_running)
	{
		lua_sethook(L, (lua_Hook)callhook, 0, 0);
		return;
	}
	if (!ar->event) 
	/* entering a function */
	{
		lua_getinfo(L, "nS", ar); 
		ret = elprof_callhookIN(L, ar->name, ar->source, ar->linedefined);
	}
	else
	/* ar->event == "return" */
		ret = elprof_callhookOUT(L);
	if (ret == -1)
		profiler_stop(L);
}

static void exit_profiler(lua_State *L)
{	
	if (profiler_is_running)
		elprof_core_finalize();
		
	lua_pushlightuserdata(L, &exit_id);
	lua_gettable(L, LUA_REGISTRYINDEX);
	lua_call(L, 0, 0);
}

static int profiler_start(lua_State *L)
{
	const char *outfile;
	int ret;

	/* mismatch start/stop */
	if (profiler_is_running)
	{
		profiler_stop(L);
		lua_pop(L, 1);
	}

	outfile = NULL;
	if (lua_gettop(L) >= 1)
		outfile = luaL_checkstring(L, 1);
	
	ret = elprof_core_init(L, outfile);
	if (ret == -1) return luaL_error(L, "extLuaProfiler error: output file could not be opened!");

	lua_sethook(L, (lua_Hook)callhook, LUA_MASKCALL | LUA_MASKRET, 0);
	profiler_is_running = 1;
	
	/* use our own exit function instead */
	lua_getglobal(L, "os");
	lua_pushlightuserdata(L, &exit_id);
	lua_pushstring(L, "exit");
	lua_gettable(L, -3);
	lua_settable(L, LUA_REGISTRYINDEX);
	lua_pushstring(L, "exit");
	lua_pushcfunction(L, (lua_CFunction)exit_profiler);
	lua_settable(L, -3);
	
	/* elprof_callhookIN(S, "profiler_start", "(C)", -1); */
	lua_pushboolean(L, 1);

	return 1;
}

static int profiler_stop(lua_State *L)
{
	lua_sethook(L, (lua_Hook)callhook, 0, 0);
	
	if(profiler_is_running)
	{
		elprof_core_finalize();
		lua_pushboolean(L, 1);
		profiler_is_running = 0;
	}
	else
		lua_pushboolean(L, 0);
  return 1;
}

static const luaL_reg prof_funcs[] =
{
	{ "start", profiler_start },
	{ "stop", profiler_stop  },
	{ NULL, NULL }
};

int luaopen_elprofiler(lua_State *L)
{
	luaL_register(L, "elprofiler", prof_funcs);
	return 0;
}
