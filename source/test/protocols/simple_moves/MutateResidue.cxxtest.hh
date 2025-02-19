// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file   protocols/membrane/MutateResidue.cxxtest.hh
/// @brief   Unit test for the MutateResidue mover, to confirm that atom placements are sensible after
/// mutation.
/// @details MutateResidue is supposed to convert one residue type to another.  Its default behaviour has
/// traditionally been to copy all atom positions from one residue type to the other for matching names.
/// While convenient for F->Y mutations and the like, something like an F->L mutation will result in
/// strange behaviour.  This test confirms that the new behaviour, which is to rebuild the side-chain and
/// then copy mainchain TORSIONS instead of atom positions, works.
/// @author  Vikram K. Mulligan (vmullig@uw.edu)

// Test Headers
#include <cxxtest/TestSuite.h>
#include <test/core/init_util.hh>
#include <test/util/pdb1ubq.hh>

// Package Headers
#include <protocols/simple_moves/MutateResidue.fwd.hh>
#include <protocols/simple_moves/MutateResidue.hh>

#include <core/conformation/Residue.hh>
#include <core/conformation/util.hh>

#include <core/chemical/ResidueType.hh>


#include <core/pose/Pose.hh>
#include <core/import_pose/import_pose.hh>
#include <core/scoring/ScoreFunction.hh>
#include <core/scoring/ScoreFunctionFactory.hh>

#include <core/types.hh>

// Utility Headers
#include <basic/Tracer.hh>

using namespace core;

static basic::Tracer TR("protocols.simple_moves.MutateResidue.cxxtest");

class MutateResidueTests : public CxxTest::TestSuite {

private: // variables
	core::pose::PoseOP testpose_;
	core::scoring::ScoreFunctionOP scorefxn_;

public: // test functions

	// Test Setup Functions ///////////////////////////

	/// @brief Setup Test
	void setUp() {

		using namespace core::pose;
		using namespace core::import_pose;

		// Initialize core & options system
		core_init();

		// Load in pose from pdb
		testpose_ = pdb1ubq5to13_poseop();
		core::scoring::ScoreFunctionOP scorefxn_;

	}

	/// @brief Tear Down Test
	void tearDown() {}

	// Test Methods /////////////////////////////////

	/// @brief Test Uniform Translation (Check first coord)
	void test_mutate_residue() {
		using namespace protocols::simple_moves;
		core::pose::PoseOP testpose_copy_(testpose_->clone());
		//testpose_->dump_pdb("before1.pdb");//DELETE ME
		//testpose_copy_->dump_pdb("before2.pdb");//DELETE ME

		utility::vector1 <core::Real> res4_chivals;
		res4_chivals = testpose_->residue(4).chi();

		//Directly mutate L8F
		MutateResidueOP mutres( new MutateResidue );
		mutres->set_target(4);
		mutres->set_res_name("PHE");
		mutres->apply(*testpose_);

		//Indirectly mutate L8G;G8F
		MutateResidueOP mutres2( new MutateResidue );
		mutres2->set_target(4);
		mutres2->set_res_name("GLY");

		mutres2->apply(*testpose_copy_);
		mutres->apply(*testpose_copy_);

		TS_ASSERT_EQUALS( res4_chivals.size() , 2  );
		TS_ASSERT_EQUALS( testpose_copy_->residue(4).nchi() , 2  );

		for ( core::Size i=1; i<=2; ++i ) testpose_copy_->set_chi(i, 4, res4_chivals[i] );

		//testpose_->dump_pdb("after1.pdb"); //DELETE ME
		//testpose_copy_->dump_pdb("after2.pdb"); //DELETE ME

		//Confirm that atoms end up in the same place as when you mutate to glycine first:
		for ( core::Size ia=1, iamax=testpose_->residue(4).natoms(); ia<=iamax; ++ia ) {
			TS_ASSERT_DELTA( testpose_->residue(4).xyz(ia).x(), testpose_copy_->residue(4).xyz(ia).x(), 1e-5  );
			TS_ASSERT_DELTA( testpose_->residue(4).xyz(ia).y(), testpose_copy_->residue(4).xyz(ia).y(), 1e-5  );
			TS_ASSERT_DELTA( testpose_->residue(4).xyz(ia).z(), testpose_copy_->residue(4).xyz(ia).z(), 1e-5  );
		}

		return;
	}

	/// @brief Test that we can mutate a disulfide-bonded cysteine.
	/// @author Vikram K. Mulligan (vmulligan@flatironinstitute.org).
	void test_mutate_disulfide() {
		using namespace protocols::simple_moves;
		core::pose::PoseOP pose( core::import_pose::pose_from_file( "protocols/simple_moves/1A2A_A.pdb", false, core::import_pose::PDB_file ) );

		//Score the pose:
		core::scoring::ScoreFunctionOP sfxn( core::scoring::get_score_function() );
		TS_ASSERT_THROWS_NOTHING(
			(*sfxn)(*pose);
		);

		//Confirm that there's a disulfide:
		TS_ASSERT_EQUALS( pose->residue_type(2).base_name(), "CYS" );
		TS_ASSERT_EQUALS( pose->residue_type(18).base_name(), "CYS" );
		TS_ASSERT( pose->residue_type(2).is_disulfide_bonded() );
		core::Size const other_res( core::conformation::get_disulf_partner( pose->conformation(), 2 ) );
		TS_ASSERT_EQUALS( other_res, 18 );
		TS_ASSERT( pose->residue_type(18).is_disulfide_bonded() );
		TS_ASSERT( pose->residue_type(18).has_variant_type( core::chemical::DISULFIDE ) );
		TS_ASSERT( pose->residue_type(18).has_property( core::chemical::DISULFIDE_BONDED ) );

		//Mutate the residue:
		MutateResidue mutres;
		mutres.set_target("29A");
		mutres.set_res_name("ALA");
		TS_ASSERT( mutres.break_disulfide_bonds() ); //Ensure that the default is to have this ON.
		mutres.apply(*pose);

		//Confirm that the disulfide is properly broken:
		TS_ASSERT_EQUALS( pose->residue_type(2).base_name(), "ALA" );
		TS_ASSERT_EQUALS( pose->residue_type(18).base_name(), "CYS" );
		TS_ASSERT( !(pose->residue_type(2).is_disulfide_bonded()) );
		TS_ASSERT( !(pose->residue_type(18).is_disulfide_bonded()) );
		TS_ASSERT( !(pose->residue_type(18).has_variant_type( core::chemical::DISULFIDE ) ) );
		TS_ASSERT( !(pose->residue_type(18).has_property( core::chemical::DISULFIDE_BONDED ) ) );

		//Confirm that we can score the pose:
		TS_ASSERT_THROWS_NOTHING(
			(*sfxn)(*pose);
		);

	}

}; // MutateResidueTests
