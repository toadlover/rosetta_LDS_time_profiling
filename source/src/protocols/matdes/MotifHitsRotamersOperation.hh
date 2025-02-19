// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file   protocols/matdes/MotifHitsRotamersOperation.hh
///
/// @brief
/// @author JAF


#ifndef INCLUDED_protocols_matdes_MotifHitsRotamersOperation_hh
#define INCLUDED_protocols_matdes_MotifHitsRotamersOperation_hh

#include <protocols/matdes/MotifHitsRotamersOperation.fwd.hh>

#include <core/pack/rotamer_set/RotamerSet.fwd.hh>
#include <core/pack/rotamer_set/RotamerSetOperation.hh>
#include <core/scoring/ScoreFunction.fwd.hh>
#include <core/scoring/motif/motif_hash_stuff.hh>
//#include <core/io/pdb/pose_io.hh>



#ifdef WIN32
#include <utility/graph/Graph.hh>
#endif


namespace protocols {
namespace matdes  {


class MotifHitsRotamersOperation : public core::pack::rotamer_set::RotamerSetOperation
{
public:

	MotifHitsRotamersOperation(core::scoring::motif::MotifHits const & motif_hits);

	~MotifHitsRotamersOperation() override{}

	core::pack::rotamer_set::RotamerSetOperationOP
	clone() const override;

	void
	alter_rotamer_set(
		core::pose::Pose const & pose,
		core::scoring::ScoreFunction const & sfxn,
		core::pack::task::PackerTask const & ptask,
		utility::graph::GraphCOP packer_neighbor_graph,
		core::pack::rotamer_set::RotamerSet & rotamer_set
	) override;

private:
	core::scoring::motif::MotifHits motif_hits_ ;
	int total_count_  ;
	// core::Size total_rot_;
	// utility::vector1< core::pose::PoseCOP > poses_;

}; // MotifHitsRotamersOperation


} // namespace matdes
} // namespace protocols

#endif
