#include <GarrysMod/Lua/Interface.h>
#include <enet/enet.h>

extern "C" int luaopen_enet( lua_State * );

GMOD_MODULE_OPEN( )
{
	return luaopen_enet( state );
}

GMOD_MODULE_CLOSE( )
{
	(void)state;
	enet_deinitialize( );
	return 0;
}