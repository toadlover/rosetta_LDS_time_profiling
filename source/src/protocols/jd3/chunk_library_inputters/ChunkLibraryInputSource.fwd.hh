// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file   protocols/jd3/ChunkLibraryInputSource.fwd.hh
/// @brief  Forward declaration of the %ChunkLibraryInputSource class
/// @author Andrew Leaver-Fay (aleaverfay@gmail.com)


#ifndef INCLUDED_protocols_jd3_chunk_library_inputters_ChunkLibraryInputSource_HH
#define INCLUDED_protocols_jd3_chunk_library_inputters_ChunkLibraryInputSource_HH

#include <utility/pointer/owning_ptr.hh>
#include <utility/vector1.fwd.hh>

namespace protocols {
namespace jd3 {
namespace chunk_library_inputters {

class ChunkLibraryInputSource;

typedef utility::pointer::shared_ptr< ChunkLibraryInputSource > ChunkLibraryInputSourceOP;
typedef utility::pointer::shared_ptr< ChunkLibraryInputSource const > ChunkLibraryInputSourceCOP;

typedef utility::vector1< ChunkLibraryInputSourceOP > ChunkLibraryInputSources;

} // namespace chunk_library_inputters
} // namespace jd3
} // namespace protocols

#endif //INCLUDED_protocols_jd3_ChunkLibraryInputSource_HH
