// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file
/// @brief

#ifndef INCLUDED_protocols_cryst_cryst_rms_hh
#define INCLUDED_protocols_cryst_cryst_rms_hh

#include <core/pose/Pose.fwd.hh>
#include <core/types.hh>

namespace protocols {
namespace cryst {

core::Size
get_nres_asu( core::pose::Pose const & pose );

core::Real
crystRMSfast (core::pose::Pose &pose_native, core::pose::Pose &pose_decoy);

core::Real
crystRMS (core::pose::Pose &pose_native, core::pose::Pose &pose_decoy, bool allatom=false, int native_cdist = 16, int decoy_cdist = 10);

}
}

#endif
