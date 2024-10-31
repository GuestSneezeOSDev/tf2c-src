//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//
#if !defined(_STATIC_LINKED) || defined(_SHARED_LIB)

#include "stdlib.h"
#include "vstdlib/random.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern "C" {

// It's very important that these functions be declared DLL_LOCAL (which does
// nothing on MSVC, but sets symbol visibility to "hidden" on GCC), because
// if they have "default" (public) symbol visibility, then when the shared
// object file built with these functions is loaded into process memory, the
// runtime dynamic linker will resolve the PLT entries for "rand" and "srand"
// to point to the non-overridden standard glibc implementations of the
// functions, rather than the override versions here as intended!
//
// This becomes a noticeable concern when the server DLL is intentionally
// built with -fvisibility=default, rather than -fvisibility=hidden, such that
// all symbols are made public and easily available to e.g. SourceMod. We do
// NOT want any such default decree of public symbol visibility to affect
// these particular functions, because that causes the intended override
// functionality to be circumvented.
//
// And what, you may ask, is the problem if the server DLL uses the standard
// libc rand function rather than the override version here? The problem is
// that the C and C++ standards dictate that RAND_MAX shall be AT LEAST
// (2^15)-1, i.e. 32767. So different libc implementations may well have
// different ranges that they return, yet code will still expect things to be
// sane and for VALVE_RAND_MAX to be perfectly fine.
//
// The current situation is that on MSVC, RAND_MAX is (2^15)-1, equal to
// VALVE_RAND_MAX. However, on Linux, glibc uses a RAND_MAX of (2^31)-1.
//
// I frankly don't entirely understand why VALVE_RAND_MAX must exist, and code
// can't simply use, you know, the actual real RAND_MAX, which would tend to
// ensure that whatever the implementation-specific range is, is applied. But
// even though it's weird and seems probably wrong for things to work that
// way, that's the way they work, and so we have to deal with this stupid
// shit.

DLL_LOCAL
void __cdecl srand(unsigned int)
{
}

DLL_LOCAL
int __cdecl rand()
{
	return RandomInt( 0, VALVE_RAND_MAX );
}

} // extern "C"

#endif // !_STATIC_LINKED || _SHARED_LIB
