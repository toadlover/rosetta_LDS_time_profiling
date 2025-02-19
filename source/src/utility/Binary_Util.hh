// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file utility/BinaryRNASilentStruct.hh
///
/// @brief
/// @author Frank DiMaio
/// @author Mike Tyka
/// @author Rhiju Das

#ifndef INCLUDED_utility_Binary_Util_hh
#define INCLUDED_utility_Binary_Util_hh

// C++ Headers
// Could be iosfwd, except for Windows PyRosetta.
#include <string>
#include <platform/types.hh>

namespace utility {

///////////////////////////////////
// Endianness swap
// Only works with aligned 4-byte quantities, will cause a bus error
//    on some platforms if used on unaligned data.
void swap4_aligned(void *v, long ndata);

//////////////////////////
///
///  uuencode
///
inline unsigned char code_to_6bit(unsigned char _6bit) {
	//return _6bit;
	const unsigned char conversion [] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	return conversion[ _6bit & 63 ];
}

inline unsigned char code_from_6bit(unsigned char _8bit) {
	//return _8bit;
	if ( ( _8bit >= 'A') && (_8bit <= 'Z') ) return _8bit - 'A';
	if ( ( _8bit >= 'a') && (_8bit <= 'z') ) return _8bit - 'a' + 26;
	if ( ( _8bit >= '0') && (_8bit <= '9') ) return _8bit - '0' + 52;
	if (   _8bit == '+'  ) return 62;
	return 63;
}

inline void encode_24_to_32(
	unsigned char i0,
	unsigned char i1,
	unsigned char i2,
	unsigned char &o0,
	unsigned char &o1,
	unsigned char &o2,
	unsigned char &o3
) {
	// converts 3 8 bit chars into
	o0 = code_to_6bit(i0 & 63);  // delete last two bits
	o1 = code_to_6bit(((i1 << 2) | (i0 >> 6)) & 63);
	o2 = code_to_6bit(((i1 >> 4) | ((i2 << 4) & 63)) & 63);
	o3 = code_to_6bit(i2 >> 2);  // right shift by two
}

inline void decode_32_to_24(
	unsigned char i0,
	unsigned char i1,
	unsigned char i2,
	unsigned char i3,
	unsigned char * o0,
	unsigned short goodbytes
) {
	i0 = code_from_6bit(i0);
	i1 = code_from_6bit(i1);
	i2 = code_from_6bit(i2);
	i3 = code_from_6bit(i3);
	*(o0) = i0 | (i1 << 6);
	if ( goodbytes > 1 ) {
		*(o0 + 1) = (i1 >> 2) | (i2 << 4);
		if ( goodbytes > 2 ) {
			*(o0 + 2) = (i3 << 2) | (i2 >> 4);
		}
	}
}


void encode6bit(const unsigned char* memory, unsigned int length, std::string &jar);

/// @brief Given 3*N bytes of memory to fill, and a string containing 4*N characters, decode the
/// characters (interpreting 4 bytes of ASCII text as 3 binary bytes) and populate the block of memory.
/// @param[in] memory A pointer to the first byte of memory to fill.  It is assumed that we're filling a
/// contiguous block of memory.
/// @param[in] jar The string containing the characters that will be decoded and converted to bytes.
/// @param[in] maxbytes The maximum number of bytes to put into the memory pointed to by the "memory" pointer.
/// (i.e. The size of the array that we're filling).
/// @note Assumes memory already allocated!!!  There is no direct check for vector overflows, since this function
/// has no knowlege of what it is that it's filling (or how big the object is).  The function relies on maxbytes
/// to prevent overflows.
/// @returns The number of bytes filled.
/// @author Originally author unknown.  Revised in 2021 by Vikram K. Mulligan (vmulligan@flatironinstitute.org).
platform::Size
decode6bit(
	unsigned char * memory,
	std::string const & jar,
	platform::Size const max_bytes
);


//////////////
}

#endif
