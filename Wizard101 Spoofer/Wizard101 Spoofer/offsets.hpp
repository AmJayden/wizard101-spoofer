#pragma once
#include <string>
#include <cstdint>
#include <cstddef>

#include <Windows.h>

namespace offsets
{
#pragma pack( push, 1 )
    struct packet_str_t
    {
        const char* str;
        std::size_t reserved1, reserved2, sz;
    };
#pragma pack( pop )

    template < typename type >
    type base( const std::uintptr_t base )
    {
        static const auto image_base = reinterpret_cast< std::uintptr_t >( GetModuleHandleA( nullptr ) );

        return type( image_base + base );
    }

    inline std::string read_packet_str( const packet_str_t* str )
    {
        return str->sz >= 0x10 ? str->str : reinterpret_cast< const char* >( str );;
    }

    const auto udp_send_data = base< void* >( 0x1043260 );
    const auto make_argument = base< void* ( __fastcall* )( void*, const char* ) >( 0xffcfe0 );
    const auto make_str = base< packet_str_t* ( __fastcall* )( void* block, const char*, std::size_t ) >( 0x1fed00 );
    const auto encode_gid = base< void ( __fastcall* )( void*, const void* ) >( 0x1fe910 );
    const auto message_2_bin = base< void* ( __fastcall* )( void*, void*, void* ) >( 0x1086ec0 );
    const auto send_event = base< void* ( __fastcall* )( void*, void*, void* ) >( 0x10810c0 );
    const auto get_string = base< packet_str_t* ( __fastcall* )( void* ) >( 0xff92b0 );
    const auto get_message_name = base< packet_str_t* ( __fastcall* )( void*, void* ) >( 0x10868e0 );
    const auto encode_str = base< void* ( __fastcall* )( void*, const char*, std::size_t ) >( 0x305220 );
    const auto encode_uint = base< void ( __fastcall* )( void*, std::uint32_t ) >( 0x1ffed0 );
    const auto g_record = base< void** >( 0x30bd840 );
}