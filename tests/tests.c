#include <stdlib.h>
#include "z8lua/lua.h"
#include "api.h"

int main()
{
    lua_State* vm = luaL_newstate();
    if (!vm)
    {
        SDL_Log("Couldn't create Lua state.");
        return EXIT_FAILURE;
    }
    luaL_openlibs(vm);
    register_api(vm);

    if (luaL_dostring(vm, "dofile('unit_tests.p8')"))
    {
        SDL_Log("Lua error: %s", lua_tostring(vm, -1));
        return EXIT_FAILURE;
    }

    lua_close(vm);
    return EXIT_SUCCESS;
}
