#include <GarrysMod/Lua/Interface.h>
#include <enet/enet.h>
#include <lua.hpp>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <stdexcept>

namespace enet
{

static bool ParseAddress( lua_State *state, const char *addr, ENetAddress &address )
{
	std::string addr_str = addr, host_str, port_str;
	size_t pos = addr_str.find( ':' );
	if( pos != addr_str.npos )
	{
		host_str = addr_str.substr( 0, pos - 1 );
		port_str = addr_str.substr( pos + 1 );
	}

	if( host_str.empty( ) )
	{
		LUA->PushNil( );
		LUA->PushString( "invalid host name" );
		return false;
	}

	if( port_str.empty( ) )
	{
		LUA->PushNil( );
		LUA->PushString( "invalid port" );
		return false;
	}

	if( host_str == "*" )
		address.host = ENET_HOST_ANY;
	else if( enet_address_set_host( &address, host_str.c_str( ) ) != 0 )
	{
		LUA->PushNil( );
		LUA->PushString( "failed to resolve host name" );
		return false;
	}

	bool success = true;
	if( port_str == "*" )
		address.port = ENET_PORT_ANY;
	else
		try
		{
			size_t end = 0;
			address.port = static_cast<enet_uint16>( std::stoi( port_str, &end ) );
			if( end < port_str.size( ) )
				throw std::invalid_argument( "port part is not fully numerical" );
		}
		catch( const std::invalid_argument & )
		{
			LUA->PushNil( );
			LUA->PushString( "port is invalid (not a number)" );
			success = false;
		}
		catch( const std::out_of_range & )
		{
			LUA->PushNil( );
			LUA->PushString( "port is outside of the 32 bits integer range" );
			success = false;
		}

	return success;
}

static bool GetPacketFlags( lua_State *state, const char *type, enet_uint32 &flags )
{
	if( strcmp( type, "reliable" ) == 0 )
		flags = ENET_PACKET_FLAG_RELIABLE;
	else if( strcmp( type, "unsequenced" ) == 0 )
		flags = ENET_PACKET_FLAG_UNSEQUENCED;
	else if( strcmp( type, "fragment" ) == 0 )
		flags = ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT;
	else if( strcmp( type, "unreliable" ) == 0 )
		flags = 0;
	else
	{
		LUA->PushNil( );
		LUA->PushString( "unknown packet flag" );
		return false;
	}

	return true;
}

namespace host
{

static bool Create( lua_State *state, ENetHost *host, bool onlyexisting = false );

}

namespace peer
{

static const char *metaname = "ENetPeer";
static uint8_t metatype = 231;
static const char *tablename = "enet_peers";
static const char *invalid_error = "invalid ENetPeer";

struct userdata
{
	ENetPeer *peer;
	uint8_t type;
	ENetHost *host;
	size_t index;
};

inline void Check( lua_State *state, int32_t index )
{
	if( !LUA->IsType( index, metatype ) )
		luaL_typerror( state, index, metaname );
}

inline userdata *GetUserdata( lua_State *state, int32_t index )
{
	return static_cast<userdata *>( LUA->GetUserdata( index ) );
}

static ENetPeer *GetAndValidate( lua_State *state, int32_t index )
{
	Check( state, index );
	ENetPeer *peer = GetUserdata( state, index )->peer;
	if( peer == nullptr )
		LUA->ArgError( index, invalid_error );

	return peer;
}

static bool Create( lua_State *state, ENetPeer *peer )
{
	LUA->PushSpecial( GarrysMod::Lua::SPECIAL_REG );

	LUA->GetField( -1, tablename );

	LUA->Remove( -2 );

	LUA->PushUserdata( peer );
	LUA->GetTable( -2 );

	if( !LUA->IsType( -1, GarrysMod::Lua::Type::NIL ) )
	{
		LUA->Remove( -2 );
		return false;
	}

	LUA->Pop( 1 );

	userdata *udata = static_cast<userdata *>( LUA->NewUserdata( sizeof( userdata ) ) );
	udata->type = metatype;
	udata->peer = peer;
	udata->host = peer->host;
	udata->index = 0;
	for( size_t k = 0; k < peer->host->peerCount; ++k )
		if( &peer->host->peers[k] == peer )
			udata->index = k;

	LUA->CreateMetaTableType( metaname, metatype );
	LUA->SetMetaTable( -2 );

	LUA->CreateTable( );
	lua_setfenv( state, -2 );

	LUA->PushUserdata( peer );
	LUA->Push( -2 );
	LUA->SetTable( -4 );

	LUA->Remove( -2 );
	return true;
}

LUA_FUNCTION_STATIC( tostring )
{
	lua_pushfstring( state, "%s: %p", metaname, GetAndValidate( state, 1 ) );
	return 1;
}

LUA_FUNCTION_STATIC( eq )
{
	LUA->PushBool( GetAndValidate( state, 1 ) == GetAndValidate( state, 2 ) );
	return 1;
}

LUA_FUNCTION_STATIC( index )
{
	LUA->GetMetaTable( 1 );
	LUA->Push( 2 );
	LUA->RawGet( -2 );
	if( !LUA->IsType( -1, GarrysMod::Lua::Type::NIL ) )
		return 1;

	LUA->Pop( 2 );

	lua_getfenv( state, 1 );
	LUA->Push( 2 );
	LUA->RawGet( -2 );
	return 1;
}

LUA_FUNCTION_STATIC( newindex )
{
	lua_getfenv( state, 1 );
	LUA->Push( 2 );
	LUA->Push( 3 );
	LUA->RawSet( -3 );
	return 0;
}

LUA_FUNCTION_STATIC( valid )
{
	Check( state, 1 );
	LUA->PushBool( GetUserdata( state, 1 )->peer != nullptr );
	return 1;
}

LUA_FUNCTION_STATIC( disconnect )
{
	ENetPeer *peer = GetAndValidate( state, 1 );
	enet_uint32 data = LUA->Top( ) > 1 ? static_cast<enet_uint32>( LUA->CheckNumber( 2 ) ) : 0;
	enet_peer_disconnect( peer, data );
	return 0;
}

LUA_FUNCTION_STATIC( disconnect_now )
{
	ENetPeer *peer = GetAndValidate( state, 1 );
	enet_uint32 data = LUA->Top( ) > 1 ? static_cast<enet_uint32>( LUA->CheckNumber( 2 ) ) : 0;
	enet_peer_disconnect_now( peer, data );
	return 0;
}

LUA_FUNCTION_STATIC( disconnect_later )
{
	ENetPeer *peer = GetAndValidate( state, 1 );
	enet_uint32 data = LUA->Top( ) > 1 ? static_cast<enet_uint32>( LUA->CheckNumber( 2 ) ) : 0;
	enet_peer_disconnect_later( peer, data );
	return 0;
}

LUA_FUNCTION_STATIC( reset )
{
	enet_peer_reset( GetAndValidate( state, 1 ) );
	return 0;
}

LUA_FUNCTION_STATIC( ping )
{
	enet_peer_ping( GetAndValidate( state, 1 ) );
	return 0;
}

LUA_FUNCTION_STATIC( receive )
{
	enet_uint8 channel = 0;
	ENetPacket *packet = enet_peer_receive( GetAndValidate( state, 1 ), &channel );
	if( packet == nullptr )
		return 0;

	LUA->PushString( reinterpret_cast<char *>( packet->data ), packet->dataLength );
	LUA->PushNumber( channel );
	LUA->PushNumber( packet->flags );
	enet_packet_destroy( packet );
	return 3;
}

LUA_FUNCTION_STATIC( send )
{
	ENetPeer *peer = GetAndValidate( state, 1 );
	LUA->CheckType( 2, GarrysMod::Lua::Type::STRING );
	size_t len = 0;
	const char *data = LUA->GetString( 2, &len );
	enet_uint8 channel = 1;
	enet_uint32 flags = 0;

	switch( LUA->Top( ) )
	{
		default:
			if( !GetPacketFlags( state, LUA->CheckString( 4 ), flags ) )
				return 2;

		case 3:
			if( !LUA->IsType( 3, GarrysMod::Lua::Type::NIL ) )
				channel = static_cast<enet_uint8>( LUA->CheckNumber( 3 ) );

		case 2:
			/* do nothing */;
	}

	ENetPacket *packet = enet_packet_create( data, len, flags );
	if( enet_peer_send( peer, channel, packet ) != 0 )
	{
		enet_packet_destroy( packet );
		LUA->PushNil( );
		LUA->PushString( "failed to send packet" );
		return 2;
	}

	enet_packet_destroy( packet );
	LUA->PushBool( true );
	return 1;
}

LUA_FUNCTION_STATIC( throttle_configure )
{
	ENetPeer *peer = GetAndValidate( state, 1 );

	enet_uint32 interval = peer->packetThrottleInterval,
		acceleration = peer->packetThrottleAcceleration,
		deceleration = peer->packetThrottleDeceleration;
	bool set = false;
	switch( LUA->Top( ) )
	{
		default:
			set = true;
			deceleration = static_cast<enet_uint32>( LUA->CheckNumber( 4 ) );

		case 3:
			if( !LUA->IsType( 3, GarrysMod::Lua::Type::NIL ) )
			{
				set = true;
				acceleration = static_cast<enet_uint32>( LUA->CheckNumber( 3 ) );
			}

		case 2:
			if( !LUA->IsType( 2, GarrysMod::Lua::Type::NIL ) )
			{
				set = true;
				interval = static_cast<enet_uint32>( LUA->CheckNumber( 2 ) );
			}

		case 1:
			/* do nothing */;
	}

	if( set )
	{
		enet_peer_throttle_configure( peer, interval, acceleration, deceleration );
		return 0;
	}

	LUA->PushNumber( interval );
	LUA->PushNumber( acceleration );
	LUA->PushNumber( deceleration );
	return 3;
}

LUA_FUNCTION_STATIC( ping_interval )
{
	ENetPeer *peer = GetAndValidate( state, 1 );

	if( LUA->Top( ) > 1 )
	{
		enet_peer_ping_interval( peer, static_cast<enet_uint32>( LUA->CheckNumber( 2 ) ) );
		return 0;
	}

	LUA->PushNumber( peer->pingInterval );
	return 1;
}

LUA_FUNCTION_STATIC( timeout )
{
	ENetPeer *peer = GetAndValidate( state, 1 );

	enet_uint32 timeout_limit = peer->timeoutLimit,
		timeout_minimum = peer->timeoutMinimum,
		timeout_maximum = peer->timeoutMaximum;
	bool set = false;
	switch( LUA->Top( ) )
	{
		default:
			set = true;
			timeout_maximum = static_cast<enet_uint32>( LUA->CheckNumber( 4 ) );

		case 3:
			if( !LUA->IsType( 3, GarrysMod::Lua::Type::NIL ) )
			{
				set = true;
				timeout_minimum = static_cast<enet_uint32>( LUA->CheckNumber( 3 ) );
			}

		case 2:
			if( !LUA->IsType( 2, GarrysMod::Lua::Type::NIL ) )
			{
				set = true;
				timeout_limit = static_cast<enet_uint32>( LUA->CheckNumber( 2 ) );
			}

		case 1:
			/* do nothing */;
	}

	if( set )
	{
		enet_peer_timeout( peer, timeout_limit, timeout_minimum, timeout_maximum );
		return 0;
	}

	LUA->PushNumber( timeout_limit );
	LUA->PushNumber( timeout_minimum );
	LUA->PushNumber( timeout_maximum );
	return 3;
}

LUA_FUNCTION_STATIC( peer_index )
{
	Check( state, 1 );
	LUA->PushNumber( GetUserdata( state, 1 )->index );
	return 1;
}

LUA_FUNCTION_STATIC( peer_state )
{
	switch( GetAndValidate( state, 1 )->state )
	{
		case ENET_PEER_STATE_DISCONNECTED:
			LUA->PushString( "disconnected" );
			break;

		case ENET_PEER_STATE_CONNECTING:
			LUA->PushString( "connecting" );
			break;

		case ENET_PEER_STATE_ACKNOWLEDGING_CONNECT:
			LUA->PushString( "acknowledging_connect" );
			break;

		case ENET_PEER_STATE_CONNECTION_PENDING:
			LUA->PushString( "connection_pending" );
			break;

		case ENET_PEER_STATE_CONNECTION_SUCCEEDED:
			LUA->PushString( "connection_succeeded" );
			break;

		case ENET_PEER_STATE_CONNECTED:
			LUA->PushString( "connected" );
			break;

		case ENET_PEER_STATE_DISCONNECT_LATER:
			LUA->PushString( "disconnect_later" );
			break;

		case ENET_PEER_STATE_DISCONNECTING:
			LUA->PushString( "disconnecting" );
			break;

		case ENET_PEER_STATE_ACKNOWLEDGING_DISCONNECT:
			LUA->PushString( "acknowledging_disconnect" );
			break;

		case ENET_PEER_STATE_ZOMBIE:
			LUA->PushString( "zombie" );
			break;

		default:
			LUA->PushString( "unknown" );
	}
	return 1;
}

LUA_FUNCTION_STATIC( connect_id )
{
	LUA->PushNumber( GetAndValidate( state, 1 )->connectID );
	return 1;
}

LUA_FUNCTION_STATIC( round_trip_time )
{
	ENetPeer *peer = GetAndValidate( state, 1 );

	if( LUA->Top( ) > 1 )
	{
		peer->roundTripTime = static_cast<enet_uint32>( LUA->CheckNumber( 2 ) );
		return 0;
	}

	LUA->PushNumber( peer->roundTripTime );
	return 1;
}

LUA_FUNCTION_STATIC( last_round_trip_time )
{
	ENetPeer *peer = GetAndValidate( state, 1 );

	if( LUA->Top( ) > 1 )
	{
		peer->lastRoundTripTime = static_cast<enet_uint32>( LUA->CheckNumber( 2 ) );
		return 0;
	}

	LUA->PushNumber( peer->lastRoundTripTime );
	return 1;
}

LUA_FUNCTION_STATIC( host )
{
	host::Create( state, GetAndValidate( state, 1 )->host, true );
	return 1;
}

LUA_FUNCTION_STATIC( local_address )
{
	const ENetAddress &address = GetAndValidate( state, 1 )->host->address;
	lua_pushfstring(
		state,
		"%d.%d.%d.%d:%d",
		( ( address.host ) & 0xFF ),
		( ( address.host >> 8 ) & 0xFF ),
		( ( address.host >> 16 ) & 0xFF ),
		( address.host >> 24 & 0xFF ),
		address.port
	);
	return 1;
}

LUA_FUNCTION_STATIC( remote_address )
{
	const ENetAddress &address = GetAndValidate( state, 1 )->address;
	lua_pushfstring(
		state,
		"%d.%d.%d.%d:%d",
		( ( address.host ) & 0xFF ),
		( ( address.host >> 8 ) & 0xFF ),
		( ( address.host >> 16 ) & 0xFF ),
		( address.host >> 24 & 0xFF ),
		address.port
	);
	return 1;
}

static void Initialize( lua_State *state )
{
	LUA->CreateMetaTableType( metaname, metatype );

	LUA->PushCFunction( tostring );
	LUA->SetField( -2, "__tostring" );

	LUA->PushCFunction( eq );
	LUA->SetField( -2, "__eq" );

	LUA->PushCFunction( index );
	LUA->SetField( -2, "__index" );

	LUA->PushCFunction( newindex );
	LUA->SetField( -2, "__newindex" );

	LUA->PushCFunction( valid );
	LUA->SetField( -2, "valid" );

	LUA->PushCFunction( disconnect );
	LUA->SetField( -2, "disconnect" );

	LUA->PushCFunction( disconnect_now );
	LUA->SetField( -2, "disconnect_now" );

	LUA->PushCFunction( disconnect_later );
	LUA->SetField( -2, "disconnect_later" );

	LUA->PushCFunction( reset );
	LUA->SetField( -2, "reset" );

	LUA->PushCFunction( ping );
	LUA->SetField( -2, "ping" );

	LUA->PushCFunction( receive );
	LUA->SetField( -2, "receive" );

	LUA->PushCFunction( send );
	LUA->SetField( -2, "send" );

	LUA->PushCFunction( throttle_configure );
	LUA->SetField( -2, "throttle_configure" );

	LUA->PushCFunction( ping_interval );
	LUA->SetField( -2, "ping_interval" );

	LUA->PushCFunction( timeout );
	LUA->SetField( -2, "timeout" );

	LUA->PushCFunction( peer_index );
	LUA->SetField( -2, "index" );

	LUA->PushCFunction( peer_state );
	LUA->SetField( -2, "state" );

	LUA->PushCFunction( connect_id );
	LUA->SetField( -2, "connect_id" );

	LUA->PushCFunction( round_trip_time );
	LUA->SetField( -2, "round_trip_time" );

	LUA->PushCFunction( last_round_trip_time );
	LUA->SetField( -2, "last_round_trip_time" );

	LUA->PushCFunction( host );
	LUA->SetField( -2, "host" );

	LUA->PushCFunction( local_address );
	LUA->SetField( -2, "local_address" );

	LUA->PushCFunction( remote_address );
	LUA->SetField( -2, "remote_address" );

	LUA->Pop( 1 );

	LUA->PushSpecial( GarrysMod::Lua::SPECIAL_REG );

	LUA->CreateTable( );

	LUA->CreateTable( );

	LUA->PushString( "v" );
	LUA->SetField( -2, "__mode" );

	LUA->SetMetaTable( -2 );

	LUA->SetField( -2, tablename );

	LUA->Pop( 1 );
}

static void Deinitialize( lua_State *state )
{
	LUA->PushSpecial( GarrysMod::Lua::SPECIAL_REG );

	LUA->PushNil( );
	LUA->SetField( -2, metaname );

	LUA->PushNil( );
	LUA->SetField( -2, tablename );

	LUA->Pop( 1 );
}

}

namespace host
{

static const char *metaname = "ENetHost";
static uint8_t metatype = 230;
static const char *tablename = "enet_hosts";
static const char *invalid_error = "invalid ENetHost";

struct userdata
{
	ENetHost *host;
	uint8_t type;
};

inline void Check( lua_State *state, int32_t index )
{
	if( !LUA->IsType( index, metatype ) )
		luaL_typerror( state, index, metaname );
}

inline userdata *GetUserdata( lua_State *state, int32_t index )
{
	return static_cast<userdata *>( LUA->GetUserdata( index ) );
}

static ENetHost *GetAndValidate( lua_State *state, int32_t index )
{
	Check( state, index );
	ENetHost *host = GetUserdata( state, index )->host;
	if( host == nullptr )
		LUA->ArgError( index, invalid_error );

	return host;
}

static bool Create( lua_State *state, ENetHost *host, bool onlyexisting )
{
	if( onlyexisting )
	{
		LUA->PushSpecial( GarrysMod::Lua::SPECIAL_REG );

		LUA->GetField( -1, tablename );

		LUA->Remove( -2 );

		LUA->PushUserdata( host );
		LUA->GetTable( -2 );

		if( !LUA->IsType( -1, GarrysMod::Lua::Type::NIL ) )
		{
			LUA->Remove( -2 );
			return true;
		}

		LUA->Pop( 1 );
		return false;
	}

	userdata *udata = static_cast<userdata *>( LUA->NewUserdata( sizeof( userdata ) ) );
	udata->type = metatype;
	udata->host = host;

	LUA->CreateMetaTableType( metaname, metatype );
	LUA->SetMetaTable( -2 );

	LUA->CreateTable( );
	lua_setfenv( state, -2 );

	return true;
}

static int32_t PushEvent( lua_State *state, const ENetEvent &ev )
{
	LUA->CreateTable( );

	if( ev.peer != nullptr )
	{
		peer::Create( state, ev.peer );
		LUA->SetField( -2, "peer" );
	}

	switch( ev.type )
	{
		case ENET_EVENT_TYPE_NONE:
			LUA->PushString( "none" );
			break;

		case ENET_EVENT_TYPE_CONNECT:
			LUA->PushNumber( ev.data );
			LUA->SetField( -2, "data" );

			LUA->PushString( "connect" );
			break;

		case ENET_EVENT_TYPE_DISCONNECT:
			LUA->PushNumber( ev.data );
			LUA->SetField( -2, "data" );

			LUA->PushString( "disconnect" );
			break;

		case ENET_EVENT_TYPE_RECEIVE:
			LUA->PushNumber( ev.channelID );
			LUA->SetField( -2, "channel" );

			LUA->PushString( reinterpret_cast<char *>( ev.packet->data ), ev.packet->dataLength );
			LUA->SetField( -2, "data" );

			LUA->PushNumber( ev.packet->flags );
			LUA->SetField( -2, "flags" );

			LUA->PushString( "receive" );

			enet_packet_destroy( ev.packet );
			break;
	}

	LUA->SetField( -2, "type" );
	return 1;
}

LUA_FUNCTION_STATIC( gc )
{
	Check( state, 1 );
	userdata *udata = GetUserdata( state, 1 );

	ENetHost *host = udata->host;
	if( host != nullptr )
	{
		LUA->PushSpecial( GarrysMod::Lua::SPECIAL_REG );

		LUA->GetField( -1, peer::tablename );

		if( LUA->IsType( -1, GarrysMod::Lua::Type::TABLE ) )
			for( size_t k = 0; k < host->peerCount; ++k )
			{
				LUA->PushUserdata( &host->peers[k] );
				LUA->GetTable( -2 );

				if( LUA->IsType( -1, peer::metatype ) )
					peer::GetUserdata( state, -1 )->peer = nullptr;

				LUA->Pop( 1 );
			}

		LUA->Pop( 2 );

		enet_host_destroy( host );
		udata->host = nullptr;
	}

	return 0;
}

LUA_FUNCTION_STATIC( tostring )
{
	lua_pushfstring( state, "%s: %p", metaname, GetAndValidate( state, 1 ) );
	return 1;
}

LUA_FUNCTION_STATIC( eq )
{
	LUA->PushBool( GetAndValidate( state, 1 ) == GetAndValidate( state, 2 ) );
	return 1;
}

LUA_FUNCTION_STATIC( index )
{
	LUA->GetMetaTable( 1 );
	LUA->Push( 2 );
	LUA->RawGet( -2 );
	if( !LUA->IsType( -1, GarrysMod::Lua::Type::NIL ) )
		return 1;

	LUA->Pop( 2 );

	lua_getfenv( state, 1 );
	LUA->Push( 2 );
	LUA->RawGet( -2 );
	return 1;
}

LUA_FUNCTION_STATIC( newindex )
{
	lua_getfenv( state, 1 );
	LUA->Push( 2 );
	LUA->Push( 3 );
	LUA->RawSet( -3 );
	return 0;
}

LUA_FUNCTION_STATIC( valid )
{
	Check( state, 1 );
	LUA->PushBool( GetUserdata( state, 1 )->host != nullptr );
	return 1;
}

LUA_FUNCTION_STATIC( service )
{
	ENetHost *host = GetAndValidate( state, 1 );
	enet_uint32 timeout = LUA->Top( ) > 1 ? static_cast<enet_uint32>( LUA->CheckNumber( 2 ) ) : 0;

	ENetEvent ev;
	int32_t ret = enet_host_service( host, &ev, timeout );
	if( ret < 0 )
	{
		LUA->PushNil( );
		LUA->PushString( "failed to service ENetHost" );
		return 2;
	}
	else if( ret == 0 )
		return 0;

	return PushEvent( state, ev );
}

LUA_FUNCTION_STATIC( check_events )
{
	ENetEvent ev;
	int32_t ret = enet_host_check_events( GetAndValidate( state, 1 ), &ev );
	if( ret < 0 )
	{
		LUA->PushNil( );
		LUA->PushString( "failed to check events for ENetHost" );
		return 2;
	}
	else if( ret == 0 )
		return 0;

	return PushEvent( state, ev );
}

LUA_FUNCTION_STATIC( compress_with_range_coder )
{
	return enet_host_compress_with_range_coder( GetAndValidate( state, 1 ) ) == 0;
}

LUA_FUNCTION_STATIC( connect )
{
	ENetHost *host = GetAndValidate( state, 1 );
	const char *addr = LUA->CheckString( 2 );
	size_t channels = 1;
	enet_uint32 data = 0;

	switch( LUA->Top( ) )
	{
		default:
			data = static_cast<enet_uint32>( LUA->CheckNumber( 4 ) );

		case 3:
			if( !LUA->IsType( 3, GarrysMod::Lua::Type::NIL ) )
				channels = static_cast<size_t>( LUA->CheckNumber( 3 ) );

		case 2:
			/* do nothing */;
	}

	ENetAddress address;
	if( !ParseAddress( state, addr, address ) )
		return 2;

	ENetPeer *peer = enet_host_connect( host, &address, channels, data );
	if( peer == nullptr )
	{
		LUA->PushNil( );
		LUA->PushString( "failed to create ENetPeer" );
		return 2;
	}

	peer::Create( state, peer );
	return 1;
}

LUA_FUNCTION_STATIC( flush )
{
	enet_host_flush( GetAndValidate( state, 1 ) );
	return 0;
}

LUA_FUNCTION_STATIC( broadcast )
{
	ENetHost *host = GetAndValidate( state, 1 );
	LUA->CheckType( 2, GarrysMod::Lua::Type::STRING );
	size_t len = 0;
	const char *data = LUA->GetString( 2, &len );
	enet_uint8 channel = 1;
	enet_uint32 flags = 0;

	switch( LUA->Top( ) )
	{
		default:
			if( !GetPacketFlags( state, LUA->CheckString( 4 ), flags ) )
				return 2;

		case 3:
			if( !LUA->IsType( 3, GarrysMod::Lua::Type::NIL ) )
				channel = static_cast<enet_uint8>( LUA->CheckNumber( 3 ) );

		case 2:
			/* do nothing */;
	}

	ENetPacket *packet = enet_packet_create( data, len, flags );
	enet_host_broadcast( host, channel, packet );
	enet_packet_destroy( packet );
	return 0;
}

LUA_FUNCTION_STATIC( channel_limit )
{
	ENetHost *host = GetAndValidate( state, 1 );

	if( LUA->Top( ) > 1 )
	{
		enet_host_channel_limit( host, static_cast<size_t>( LUA->CheckNumber( 2 ) ) );
		return 0;
	}

	LUA->PushNumber( host->channelLimit );
	return 1;
}

LUA_FUNCTION_STATIC( bandwidth_limit )
{
	ENetHost *host = GetAndValidate( state, 1 );

	if( LUA->Top( ) > 2 )
	{
		enet_host_bandwidth_limit(
			GetAndValidate( state, 1 ),
			static_cast<enet_uint32>( LUA->CheckNumber( 2 ) ),
			static_cast<enet_uint32>( LUA->CheckNumber( 3 ) )
		);
		return 0;
	}

	LUA->PushNumber( host->incomingBandwidth );
	LUA->PushNumber( host->outgoingBandwidth );
	return 1;
}

LUA_FUNCTION_STATIC( local_address )
{
	const ENetAddress &address = GetAndValidate( state, 1 )->address;
	lua_pushfstring(
		state,
		"%d.%d.%d.%d:%d",
		( ( address.host ) & 0xFF ),
		( ( address.host >> 8 ) & 0xFF ),
		( ( address.host >> 16 ) & 0xFF ),
		( address.host >> 24 & 0xFF ),
		address.port
	);
	return 1;
}

LUA_FUNCTION_STATIC( total_sent_data )
{
	ENetHost *host = GetAndValidate( state, 1 );

	if( LUA->Top( ) > 1 )
	{
		host->totalSentData = static_cast<enet_uint32>( LUA->CheckNumber( 2 ) );
		return 0;
	}

	LUA->PushNumber( host->totalSentData );
	return 1;
}

LUA_FUNCTION_STATIC( total_sent_packets )
{
	ENetHost *host = GetAndValidate( state, 1 );

	if( LUA->Top( ) > 1 )
	{
		host->totalSentPackets = static_cast<enet_uint32>( LUA->CheckNumber( 2 ) );
		return 0;
	}

	LUA->PushNumber( host->totalSentPackets );
	return 1;
}

LUA_FUNCTION_STATIC( total_received_data )
{
	ENetHost *host = GetAndValidate( state, 1 );

	if( LUA->Top( ) > 1 )
	{
		host->totalReceivedData = static_cast<enet_uint32>( LUA->CheckNumber( 2 ) );
		return 0;
	}

	LUA->PushNumber( host->totalReceivedData );
	return 1;
}

LUA_FUNCTION_STATIC( total_received_packets )
{
	ENetHost *host = GetAndValidate( state, 1 );

	if( LUA->Top( ) > 1 )
	{
		host->totalReceivedPackets = static_cast<enet_uint32>( LUA->CheckNumber( 2 ) );
		return 0;
	}

	LUA->PushNumber( host->totalReceivedPackets );
	return 1;
}

LUA_FUNCTION_STATIC( service_time )
{
	LUA->PushNumber( GetAndValidate( state, 1 )->serviceTime );
	return 1;
}

LUA_FUNCTION_STATIC( peer_count )
{
	LUA->PushNumber( GetAndValidate( state, 1 )->peerCount );
	return 1;
}

LUA_FUNCTION_STATIC( peer )
{
	ENetHost *host = GetAndValidate( state, 1 );
	size_t index = static_cast<size_t>( LUA->CheckNumber( 2 ) ) - 1;

	if( index < 0 || index >= host->peerCount )
	{
		LUA->PushNil( );
		LUA->PushString( "invalid peer index" );
		return 2;
	}

	peer::Create( state, &host->peers[index] );
	return 1;
}

static void Initialize( lua_State *state )
{
	LUA->CreateMetaTableType( metaname, metatype );

	LUA->PushCFunction( gc );
	LUA->SetField( -2, "__gc" );

	LUA->PushCFunction( tostring );
	LUA->SetField( -2, "__tostring" );

	LUA->PushCFunction( eq );
	LUA->SetField( -2, "__eq" );

	LUA->PushCFunction( index );
	LUA->SetField( -2, "__index" );

	LUA->PushCFunction( newindex );
	LUA->SetField( -2, "__newindex" );

	LUA->PushCFunction( valid );
	LUA->SetField( -2, "valid" );

	LUA->PushCFunction( service );
	LUA->SetField( -2, "service" );

	LUA->PushCFunction( check_events );
	LUA->SetField( -2, "check_events" );

	LUA->PushCFunction( compress_with_range_coder );
	LUA->SetField( -2, "compress_with_range_coder" );

	LUA->PushCFunction( connect );
	LUA->SetField( -2, "connect" );

	LUA->PushCFunction( flush );
	LUA->SetField( -2, "flush" );

	LUA->PushCFunction( broadcast );
	LUA->SetField( -2, "broadcast" );

	LUA->PushCFunction( channel_limit );
	LUA->SetField( -2, "channel_limit" );

	LUA->PushCFunction( bandwidth_limit );
	LUA->SetField( -2, "bandwidth_limit" );

	LUA->PushCFunction( local_address );
	LUA->SetField( -2, "local_address" );

	LUA->PushCFunction( gc );
	LUA->SetField( -2, "destroy" );

	LUA->PushCFunction( total_sent_data );
	LUA->SetField( -2, "total_sent_data" );

	LUA->PushCFunction( total_sent_packets );
	LUA->SetField( -2, "total_sent_packets" );

	LUA->PushCFunction( total_received_data );
	LUA->SetField( -2, "total_received_data" );

	LUA->PushCFunction( total_received_packets );
	LUA->SetField( -2, "total_received_packets" );

	LUA->PushCFunction( service_time );
	LUA->SetField( -2, "service_time" );

	LUA->PushCFunction( peer_count );
	LUA->SetField( -2, "peer_count" );

	LUA->PushCFunction( peer );
	LUA->SetField( -2, "peer" );

	LUA->Pop( 1 );

	LUA->PushSpecial( GarrysMod::Lua::SPECIAL_REG );

	LUA->CreateTable( );

	LUA->CreateTable( );

	LUA->PushString( "v" );
	LUA->SetField( -2, "__mode" );

	LUA->SetMetaTable( -2 );

	LUA->SetField( -2, tablename );

	LUA->Pop( 1 );
}

static void Deinitialize( lua_State *state )
{
	LUA->PushSpecial( GarrysMod::Lua::SPECIAL_REG );

	LUA->PushNil( );
	LUA->SetField( -2, metaname );

	LUA->PushNil( );
	LUA->SetField( -2, tablename );

	LUA->Pop( 1 );
}

}

LUA_FUNCTION_STATIC( host_create )
{
	bool have_address = true;
	ENetAddress address = { 0 };
	if( LUA->Top( ) == 0 || LUA->IsType( 1, GarrysMod::Lua::Type::NIL ) )
		have_address = false;
	else if( !ParseAddress( state, LUA->CheckString( 1 ), address ) )
		return 2;

	size_t peer_count = 64, channel_count = 1;
	enet_uint32 in_bandwidth = 0, out_bandwidth = 0;
	switch( LUA->Top( ) )
	{
		default:
			out_bandwidth = static_cast<enet_uint32>( LUA->CheckNumber( 5 ) );

		case 4:
			if( !LUA->IsType( 4, GarrysMod::Lua::Type::NIL ) )
				in_bandwidth = static_cast<enet_uint32>( LUA->CheckNumber( 4 ) );

		case 3:
			if( !LUA->IsType( 3, GarrysMod::Lua::Type::NIL ) )
				channel_count = static_cast<size_t>( LUA->CheckNumber( 3 ) );

		case 2:
			if( !LUA->IsType( 2, GarrysMod::Lua::Type::NIL ) )
				peer_count = static_cast<size_t>( LUA->CheckNumber( 2 ) );

		case 1:
			/* do nothing */;
	}

	ENetHost *host = enet_host_create(
		have_address ? &address : nullptr,
		peer_count,
		channel_count,
		in_bandwidth,
		out_bandwidth
	);
	if( host == nullptr )
	{
		LUA->PushNil( );
		LUA->PushString( "failed to create ENetHost" );
		return 2;
	}

	host::Create( state, host );
	return 1;
}

LUA_FUNCTION_STATIC( linked_version )
{
	ENetVersion version = enet_linked_version( );
	lua_pushfstring(
		state,
		"%d.%d.%d",
		ENET_VERSION_GET_MAJOR( version ),
		ENET_VERSION_GET_MINOR( version ),
		ENET_VERSION_GET_PATCH( version )
	);
	return 1;
}

static void Initialize( lua_State *state )
{
	if( enet_initialize( ) != 0 )
		LUA->ThrowError( "failed to initialize ENet" );

	LUA->PushSpecial( GarrysMod::Lua::SPECIAL_GLOB );

	LUA->CreateTable( );

	LUA->PushCFunction( host_create );
	LUA->SetField( -2, "host_create" );

	LUA->PushCFunction( linked_version );
	LUA->SetField( -2, "linked_version" );

	LUA->SetField( -2, "enet" );

	LUA->Pop( 1 );
}

static void Deinitialize( lua_State *state )
{
	LUA->PushSpecial( GarrysMod::Lua::SPECIAL_GLOB );

	LUA->PushNil( );
	LUA->SetField( -2, "enet" );

	LUA->Pop( 1 );

	enet_deinitialize( );
}

}

GMOD_MODULE_OPEN( )
{
	enet::Initialize( state );
	enet::host::Initialize( state );
	enet::peer::Initialize( state );
	return 0;
}

GMOD_MODULE_CLOSE( )
{
	enet::peer::Deinitialize( state );
	enet::host::Deinitialize( state );
	enet::Deinitialize( state );
	return 0;
}