// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file   protocols/moves/MoverContainer.cxxtest.hh
/// @brief  tests for container Movers classes.
/// @author Sergey Lyskov

// Test headers
#include <cxxtest/TestSuite.h>

#include <test/core/init_util.hh>

#include <core/pose/Pose.hh>
#include <core/types.hh>

// Unit headers
#include <protocols/moves/RepeatMover.hh>
#include <protocols/moves/MoverContainer.hh>

#include <basic/Tracer.hh>

//Auto Headers
#include <protocols/moves/Mover.fwd.hh>
#include <utility/pointer/owning_ptr.hh>
#include <string>
#include <vector>


/// We want to isolate static instances of DummyMover so we put it inside privet namespace.
namespace MoveContainerCxxTest {
#include <test/protocols/moves/DummyMover.hh>
}
using namespace MoveContainerCxxTest;
typedef utility::pointer::shared_ptr< DummyMover > DummyMoverOP;

using basic::Error;
using basic::Warning;

static basic::Tracer TR("protocols.moves.MoverContainer.cxxtest");

// using declarations
using namespace core;
using namespace protocols::moves;

///////////////////////////////////////////////////////////////////////////
/// @name ContainerMoversTest
/// @brief: test for: SequenceMover
/// @details
/// @author Sergey Lyskov Fri Nov 02 2007
///////////////////////////////////////////////////////////////////////////
class ContainerMoversTest : public CxxTest::TestSuite {

public:
	ContainerMoversTest() {}

	void setUp() {
		extern int command_line_argc; extern char ** command_line_argv;
		if ( command_line_argc > 1 ) core::init::init(command_line_argc, command_line_argv);
		else {
			std::string commandline = "core.test -mute all";
			initialize_from_commandline_w_db( commandline );
		}

		basic::random::init_random_generators(1000, "mt19937");
	}

	void tearDown() {
	}

	void test_RepeatMover() {
		pose::Pose pose;

		for ( Size i=0; i<256; i++ ) {
			DummyMover::reset();
			DummyMoverOP dm( new DummyMover );
			RepeatMover RM(dm, i);

			RM.apply(pose);

			TS_ASSERT_EQUALS(dm->call_count(), int(i));
		}
	}


	void test_SequenceMover() {
		//TR << "test_SequenceMover...\n";
		const Size N = 100; ///< number of movers to test in sequence.

		pose::Pose pose;

		DummyMover::reset();
		SequenceMover SM;
		for ( Size i=0; i<N; i++ ) SM.add_mover(utility::pointer::make_shared< DummyMover >(i));

		SM.apply(pose);

		TS_ASSERT_EQUALS(DummyMover::call_records().size(), N);
		if ( DummyMover::call_records().size() == N ) {
			for ( Size i=0; i<N; i++ ) {
				TS_ASSERT_EQUALS(DummyMover::call_records()[i], int(i));
			}
		}
		//TR << "test_SequenceMover... Ok.\n";
	}


	void test_CycleMover() {
		const Size N = 114; ///< number of movers.
		const Size NSteps = 1514; ///< number of steps to test.

		pose::Pose pose;

		CycleMover CM;
		for ( Size i=0; i<N; i++ ) CM.add_mover(utility::pointer::make_shared< DummyMover >(i));

		for ( Size s=0; s<NSteps; s++ ) {
			DummyMover::reset();
			CM.apply(pose);

			TS_ASSERT_EQUALS(DummyMover::call_records().size(), 1u);
			if ( DummyMover::call_records().size() == 1 ) {
				int k = s % N;
				TS_ASSERT_EQUALS(DummyMover::call_records()[0], k);
			}
		}
	}

};

