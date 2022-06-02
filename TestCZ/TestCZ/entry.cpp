#include <iostream>
#include <sstream>
#include <string_view>
#include <functional>
#include <tuple>
#include <array>
#include <span>
#include <thread>
#include <unordered_map>

#include "ud.hpp"

struct pattern_t
{
	using func_t = std::function< std::uintptr_t( const ud::segment_t& ) >;
	func_t callback;
	
	pattern_t( ) = delete;
	pattern_t( func_t callback ) : callback{ callback } {}
	pattern_t( const std::string_view pattern ) : callback{ 
		[ pattern ] ( const ud::segment_t& seg ) {
			return *seg.find_pattern( pattern );
		}
	} {}
};

struct address_t
{
	std::string_view name;
	std::string_view type;
	
	pattern_t pattern;

	address_t( ) = delete;
	address_t( const std::string_view name, const std::string_view type, const std::string_view pattern ) : name{ name }, type{ type }, pattern{ pattern } {}
	address_t( const std::string_view name, const std::string_view type, const pattern_t::func_t pattern ) : name{ name }, type{ type }, pattern{ pattern } {}
};

const auto addresses = std::to_array< address_t >( {
	// primitive scans
	{ "udp_send_data", "void*", "48 89 5C 24 08 57 48 83 EC 20 48 8B FA 48 8B D9 45 85" },
	{ "make_argument", "void* ( __fastcall* )( void*, const char* )", "48 89 5C 24 08 57 48 83 EC 20 48 8B DA 48 8B F9 48 85 D2 75 12" },
	{ "make_str", "packet_str_t* ( __fastcall* )( void* block, const char*, std::size_t )", "48 89 5C 24 10 48 89 6C 24 18 56 57 41 57 48 83 EC 20 48 8B 69 18 49" },
	{ "encode_gid", "void ( __fastcall* )( void*, const void* )", "4C 8B DC 57 48 81 EC 80 00 00 00 49 C7 43 98 FE FF FF FF 49 89 5B 08 48" },

	// advanced scans
	{ "message_2_bin", "void* ( __fastcall* )( void*, void*, void* )", [ ] ( const ud::segment_t& wiz ) {
		return *wiz.find_pattern( "58 C3 CC CC CC CC CC CC CC CC 4C 8B DC 55 56 57" ) + 0xA;
	} },

	{ "send_event", "void* ( __fastcall* )( void*, void*, void* )", [ ] ( const ud::segment_t& wiz ) {
		return *wiz.find_pattern( "49 0F 45 7D 38" ) - 0x5D;
	} },

	{ "get_string", "packet_str_t* ( __fastcall* )( void* )", [ ] ( const ud::segment_t& wiz ) {
		return *wiz.find_pattern( "00 CC CC CC CC CC CC CC CC CC CC CC CC 40 53 48 81 EC 90 00 00 00" ) + 13;
	} },

	{ "get_message_name", "packet_str_t* ( __fastcall* )( void*, void* )", [ ] ( const ud::segment_t& wiz ) {
		return *wiz.find_pattern( "48 83 C4 50 5B C3 0F B6 42" ) - 0x16;
	} },

	{ "encode_str", "void* ( __fastcall* )( void*, const char*, std::size_t )", [ ] ( const ud::segment_t& wiz ) {
		return *wiz.find_pattern( "5B C3 CC CC CC CC CC CC CC 4C 8B DC 56 57" ) + 9;
	} },
		
	{ "encode_uint", "void ( __fastcall* )( void*, std::uint32_t )", [ ] ( const ud::segment_t& wiz ) { 
		return *wiz.find_pattern( "00 5F C3 CC CC CC CC CC CC CC 4C 8B DC 57 48 81 EC 80 00 00 00" ) + 10;
	} },
		
	{ "g_record", "void**", [ ] ( const ud::segment_t& wiz ) {
		const auto call = *wiz.find_pattern( "48 8B 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 90 4C 8B 0F 45 33 C0 48 8B D0 48 8B CF 41 FF 91 ?? ?? ?? ?? 90 48 8B 55 78" );
		
		return ud::calculate_relative< std::uint32_t >( call, 7, 3 );
	} }	
} );

class codegen_t
{
	std::stringstream ss;
	std::unordered_map< std::string, std::uintptr_t > resolved_addresses;
	
	static std::uintptr_t get_address( const pattern_t& pattern )
	{
		static const auto image_base = reinterpret_cast< std::uintptr_t >( GetModuleHandleA( nullptr ) );
		static const ud::segment_t wiz{ ".text" };
		
		return pattern.callback( wiz ) - image_base;
	}

	void indent( const std::size_t count )
	{
		ss << std::string( count * 4, ' ' );
	}

	// hacky method but cleaner than embedding it directly
	void write_header( )
	{
		ss <<
			#include "header.txt"
		;
	}
	
	void write_address( const address_t& address )
	{
		indent( 2 );
		
		ss << "const auto " << address.name << " = base< " <<
			address.type << " >( 0x" << std::hex << get_address( address.pattern ) << std::dec << " );" << '\n';
	}
	
	void init( const std::span< const address_t > addresses )
	{
		write_header( );
		
		for ( const auto& address : addresses )
			write_address( address );
		
		ss << '}';
	}

public:
	
	std::string get( ) const
	{
		return ss.str( );
	}
	
	codegen_t( ) = delete;
	codegen_t( const std::span< const address_t > addresses )
	{
		init( addresses );
	}
};

template < typename period_t >
period_t get_time( )
{
	return std::chrono::duration_cast< period_t >( std::chrono::high_resolution_clock::now( ).time_since_epoch( ) );
}

void dll_main( )
{
	FILE* stream;

	AllocConsole( );
	freopen_s( &stream, "CONOUT$", "w", stdout );

	
	const auto start = get_time< std::chrono::microseconds >( );
	codegen_t generator( addresses );
	
	const auto end = get_time< std::chrono::microseconds >( );
	std::cout << generator.get( ) << "\nTook " << end - start;
}

bool __stdcall DllMain( void* const mod, const std::uint32_t reason, void* )
{
	if ( reason == DLL_PROCESS_ATTACH )
		std::thread{ dll_main }.detach( );
	
	return true;
}