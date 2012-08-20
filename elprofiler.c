#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lua.h>
#include <lauxlib.h>

#include "elprof_core.h"

static int exit_id;
static int elprof_state_id;
static elprof_STATE *s_S;

static int profiler_stop(lua_State *L);
static void callhook(lua_State *L, lua_Debug *ar)
{
	lua_getinfo(L, "nS", ar); 
	
	/* ignore LUA API AND C Function */
   /* if (ar->linedefined == -1) */
   /*		return;  */
		
	if (!ar->event) 
		/* entering a function */
		elprof_callhookIN(s_S, ar->name, ar->source, ar->linedefined);
 	 else
  		/* ar->event == "return" */
    	elprof_callhookOUT(s_S);
}

static void exit_profiler(lua_State *L)
{
	elprof_STATE* S;
	lua_pushlightuserdata(L, &elprof_state_id);
	lua_gettable(L, LUA_REGISTRYINDEX);
	if (!lua_isnil(L, -1))
	{
		S = (elprof_STATE*)lua_touserdata(L, -1);
		elprof_core_finalize(S);
	}
	lua_pushlightuserdata(L, &exit_id);
	lua_gettable(L, LUA_REGISTRYINDEX);
	lua_call(L, 0, 0);
}

static int profiler_start(lua_State *L)
{
	elprof_STATE *S;
	const char *outfile;
	int top;

	lua_pushlightuserdata(L, &elprof_state_id);
	lua_gettable(L, LUA_REGISTRYINDEX);

	/* mismatch start/stop */
	if (!lua_isnil(L, -1))
	{
		top = lua_gettop(L);
		profiler_stop(L);
		lua_settop(L, top);
	}
	lua_pop(L, 1);

	outfile = NULL;
	if (lua_gettop(L) >= 1)
		outfile = luaL_checkstring(L, 1);
	
	S = elprof_core_init(outfile);
	if (!S) return luaL_error(L, "extLuaProfiler error: output file could not be opened!");

	lua_sethook(L, (lua_Hook)callhook, LUA_MASKCALL | LUA_MASKRET, 0);
	
	lua_pushlightuserdata(L, &elprof_state_id);
	lua_pushlightuserdata(L, S);
	lua_settable(L, LUA_REGISTRYINDEX);
	
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

	s_S = S;
	return 1;
}

static int profiler_stop(lua_State *L)
{
	elprof_STATE* S;
	lua_sethook(L, (lua_Hook)callhook, 0, 0);
	lua_pushlightuserdata(L, &elprof_state_id);
	lua_gettable(L, LUA_REGISTRYINDEX);
	
	if(!lua_isnil(L, -1))
	{
		S = (elprof_STATE*)lua_touserdata(L, -1);
		elprof_core_finalize(S);
		lua_pushlightuserdata(L, &elprof_state_id);
		lua_pushnil(L);
		lua_settable(L, LUA_REGISTRYINDEX);
		lua_pushboolean(L, 1);
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
