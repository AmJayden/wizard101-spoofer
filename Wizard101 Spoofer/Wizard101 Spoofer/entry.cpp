#include <iostream>
#include <thread>
#include <mutex>
#include <functional>
#include <unordered_map>

#include <Windows.h>

#include "MinHook.h"
#include "offsets.hpp"
#include "ud.hpp"

#pragma comment( lib, "libMinHook.x64.lib" )

namespace packet
{
	void* encode_string( void* const packet, const std::string_view str )
	{
		return offsets::encode_str( packet, str.data( ), str.size( ) );
	}

	std::string get_packet_name( void* packet )
	{
		return offsets::read_packet_str( offsets::get_message_name( *offsets::g_record, packet ) );
	}

	void* current_packet;
	void* ( __fastcall* message_to_binary_original )( void*, void*, void* );
	void* message_to_binary_hook( void* a1, void* a2, void* a3 )
	{
		current_packet = a3;
		
		return message_to_binary_original( a1, a2, a3 );
	}
	
	std::string random_string( const std::size_t length )
	{
		static const auto chars = "01234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ`~!@#$%^&*()-=_+{}|\\\";'/.,<>?:";
		std::string str( length, '\0' );
		
		std::generate_n( str.begin( ), length, [ ]
		{
			return chars[ std::rand( ) % 10 ];
		} );

		return str;
	}

	bool woof_hwid( void* packet )
	{
		const auto arg = offsets::make_argument( packet, "MachineID" );
		static const auto random_id = random_string( 128 );
		
		std::cout << "[wiz-packet] Old HWID: " << offsets::read_packet_str( offsets::get_string( arg ) ) << '\n';

		offsets::encode_gid( arg, &random_id );
		std::cout << "[wiz-packet] New HWID: " << offsets::read_packet_str( offsets::get_string( arg ) ) << '\n';

		if ( const auto crc = offsets::make_argument( packet, "CRC" ) )
			encode_string( crc, random_id.c_str( ) );
		
		if ( const auto steam_patcher = offsets::make_argument( packet, "IsSteamPatcher" ) )
			offsets::encode_uint( steam_patcher, 0 );

		return true;
	}

	static const std::unordered_map< std::string, std::function< bool( void* ) > > packet_callbacks
	{
		{ "MSG_USER_AUTHEN_V3", woof_hwid },
		{ "MSG_ATTACH", woof_hwid },
		{ "MSG_USER_AUTHEN", woof_hwid },
		{ "MSG_USER_VALIDATE", woof_hwid },
		{ "MSG_USER_AUTHEN_V2", woof_hwid },
		{ "MSG_WEB_AUTHEN", woof_hwid },
		{ "MSG_WEB_VALIDATE", woof_hwid }
	};

	void* ( __fastcall* udp_send_data_original )( void*, void*, int );
	void* udp_send_data_hook( void* a1, void* a2, int a3 )
	{
		const auto packet_name = offsets::read_packet_str( offsets::get_message_name( *offsets::g_record, current_packet ) );
		std::cout << "[wiz-packet] packet name: " << packet_name << '\n';
		
		if ( const auto callback = packet_callbacks.find( packet_name ); callback != packet_callbacks.cend( ) )
			if ( !callback->second( current_packet ) )
				return nullptr;

		return udp_send_data_original( a1, message_to_binary_original( *offsets::g_record, a2, current_packet ), a3 );
	}

	void* ( __fastcall* send_event_original )( void*, void*, void* );
	void* __fastcall send_event_hook( void* a1, offsets::packet_str_t* packet, void* a3 )
	{
		static auto within = false;
		
		if ( !within )
		{
			within = true;
			
			const auto packet_name = offsets::read_packet_str( packet );
			
			if ( !packet_name.contains( "Text" ) && !packet_name.contains( "Gui" ) && !packet_name.contains( "GUI" ) && 
				!packet_name.contains( "MSG" ) && !packet_name.contains( "Mouse" ) )
				std::cout << "[wiz-event] event name: " << packet_name << '\n';

			within = false;
		}
		
		return send_event_original( a1, packet, a3 );;
	}
}

void dll_main( )
{	
	const ud::module_t wiz;
	std::cout << "[wiz-log] Image base: " << std::hex << wiz.start << std::endl;
	
	MH_Initialize( );

	MH_CreateHook( offsets::message_2_bin, &packet::message_to_binary_hook, reinterpret_cast< void** >( &packet::message_to_binary_original ) );
	MH_CreateHook( offsets::udp_send_data, &packet::udp_send_data_hook, reinterpret_cast< void** >( &packet::udp_send_data_original ) );
	MH_CreateHook( offsets::send_event, &packet::send_event_hook, reinterpret_cast< void** >( &packet::send_event_original ) );

	MH_EnableHook( MH_ALL_HOOKS );
}

bool __stdcall DllMain( void* handle, const std::uint32_t reason, void* )
{
	if ( reason == DLL_PROCESS_ATTACH )
	{
		FILE* stream;
		
		AllocConsole( );
		freopen_s( &stream, "CONOUT$", "w", stdout );
		freopen_s( &stream, "CONOUT$", "w", stderr );
		freopen_s( &stream, "CONIN$", "r", stdin );
		
		std::thread{ dll_main }.detach( );
	}

	return true;
}