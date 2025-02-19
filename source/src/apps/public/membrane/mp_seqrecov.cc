// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file mp_seqrecov.cc
/// @brief Compute recovery statistics that consider the membrane environment
/// @author Ron Jacak
/// @author P. Douglas Renfrew (renfrew@nyu.edu) (added rotamer recovery, cleanup)
/// @author Rebecca Alford (ralford3@jhu.edu) Adapted for membrane solvation

// Unit headers
#include <devel/init.hh>

//project Headers
#include <core/conformation/PointGraph.hh>
#include <core/conformation/find_neighbors.hh>

#include <core/pack/task/PackerTask.hh>
#include <core/pack/task/operation/TaskOperations.hh>
#include <core/pack/task/operation/TaskOperationFactory.hh>
#include <core/pack/task/TaskFactory.hh>
#include <core/pose/Pose.hh>
#include <core/pose/PDBInfo.hh>
#include <core/conformation/Residue.hh>
#include <core/scoring/ScoreFunction.hh>
#include <core/scoring/ScoreFunctionFactory.hh>

// Needed for membrane implementation
#include <protocols/membrane/AddMembraneMover.hh>
#include <core/conformation/membrane/ImplicitLipidInfo.hh>
#include <core/conformation/membrane/MembraneInfo.hh>
#include <core/conformation/membrane/MembraneGeometry.hh>
#include <core/conformation/Conformation.hh>
#include <utility/io/ozstream.hh>
#include <utility/file/file_sys_util.hh>

// basic headers
#include <basic/Tracer.hh>
#include <basic/datacache/DataMap.hh>

// Utility Headers
#include <utility/vector1.hh>
#include <utility/excn/Exceptions.hh>

// Option keys

// Numeric Headers

// ObjexxFCL Headers
#include <ObjexxFCL/format.hh>
#include <ObjexxFCL/FArray2D.hh>

// C++ headers
#include <sstream>

//Auto Headers
#include <core/conformation/PointGraphData.hh>
#include <utility/graph/UpperEdgeGraph.hh>
#include <core/import_pose/import_pose.hh>


static basic::Tracer TR( "sequence_recovery" );

using namespace core;
using namespace protocols;
using namespace basic::options;
using namespace basic::options::OptionKeys;

using namespace ObjexxFCL::format;

namespace sequence_recovery {
FileOptionKey const native_pdb_list( "sequence_recovery::native_pdb_list" );
FileOptionKey const redesign_pdb_list( "sequence_recovery::redesign_pdb_list" );
FileOptionKey const parse_taskops_file( "sequence_recovery::parse_taskops_file" );
BooleanOptionKey const rotamer_recovery( "sequence_recovery::rotamer_recovery" );
StringOptionKey const seq_recov_filename( "sequence_recovery::seq_recov_filename" );
StringOptionKey const sub_matrix_filename( "sequence_recovery::sub_matrix_filename" );
IntegerOptionKey const se_cutoff( "sequence_recovery::se_cutoff" );
}

std::string usage_string;

void init_usage_prompt( std::string exe ) {

	// place the prompt up here so that it gets updated easily; global this way, but that's ok
	std::stringstream usage_stream;
	usage_stream << "No files given: Use either -file:s or -file:l to designate a single pdb or a list of pdbs.\n\n"
		<< "Usage: " << exe
		<< "\n\t-native_pdb_list <list file>"
		<< "\n\t-redesign_pdb_list <list file>"

		<< "\n\t[-seq_recov_filename <file>] file to output sequence recoveries to (default: sequencerecovery.txt)"
		<< "\n\t[-sub_matrix_filename <file>] file to output substitution matrix to (default: submatrix.txt)"
		<< "\n\t[-parse_taskops_file <file>] tagfile which contains task operations to apply before measuring recovery (optional)"
		<< "\n\t[-ignore_unrecognized_res]"

		<< "\n\n";

	usage_string = usage_stream.str();

}


/// @brief load custom TaskOperations according to an xml-like utility::tag file
/// @details The sequence recovery app can only handle taskops that do not use
/// ResidueSelectors, unless they are anonymous (i.e. unnamed) ResidueSelectors
/// that are declared as subtags of TaskOperations.
core::pack::task::TaskFactoryOP setup_tf( core::pack::task::TaskFactoryOP task_factory_ ) {

	using namespace core::pack::task::operation;

	if ( option[ sequence_recovery::parse_taskops_file ].user() ) {
		std::string tagfile_name( option[ sequence_recovery::parse_taskops_file ]() );
		TaskOperationFactory::TaskOperationOPs tops;
		basic::datacache::DataMap dm; // empty data map! any TaskOperation that tries to retrieve data out of this datamap will fail to find it.
		TaskOperationFactory::get_instance()->newTaskOperations( tops, dm, tagfile_name );
		for ( TaskOperationFactory::TaskOperationOPs::iterator it( tops.begin() ), itend( tops.end() ); it != itend; ++it ) {
			task_factory_->push_back( *it );
		}
	} else {
		task_factory_->push_back( TaskOperationCOP( new pack::task::operation::InitializeFromCommandline ) );
	}

	return task_factory_;

}


/// @brief helper method which uses the tenA nb graph in the pose object to fill a vector with nb counts
void fill_num_neighbors( pose::Pose & pose, utility::vector1< core::Size > & num_nbs ) {

	using core::conformation::PointGraph;
	using core::conformation::PointGraphOP;

	PointGraphOP pg( new PointGraph ); // create graph
	core::conformation::residue_point_graph_from_conformation( pose.conformation(), *pg ); // create vertices
	core::conformation::find_neighbors<core::conformation::PointGraphVertexData,core::conformation::PointGraphEdgeData>( pg, 10.0 /* ten angstrom distance */ ); // create edges

	num_nbs.resize( pose.size(), 0 );
	for ( core::Size ii=1; ii <= pose.size(); ++ii ) {

		// a PointGraph is a typedef of UpperEdgeGraph< PointGraphVertexData, PointGraphEdgeData >
		// so any of the method in UpperEdgeGraph should be avail. here. The UpperEdgeGraph provides access to nodes
		// via a get_vertex() method, and each vertex can report back how many nbs it has.
		// So something that before was really complicated (nb count calculation) is done in <10 lines of code.
		// the assumption we're making here is that a pose residue position ii is the same index as the point graph vertex
		// that is indeed the case if you look at what the function residue_point_graph_from_pose().
		num_nbs[ ii ] = pg->get_vertex(ii).num_neighbors_counting_self();
	}

	return;
}

/// @brief return the set of residues that are designable based given pose
std::set< Size > fill_designable_set( pose::Pose & pose, pack::task::TaskFactoryOP & tf ) {

	//we need to score the pose for many of the task operations passed from cmd line
	if ( option[ sequence_recovery::parse_taskops_file ].user() ) {
		scoring::ScoreFunctionOP scorefxn = scoring::get_score_function();
		(*scorefxn)( pose );
	}
	std::set< Size > designable_set;
	core::pack::task::PackerTaskOP design_task( tf->create_task_and_apply_taskoperations( pose ) );

#ifndef NDEBUG
	TR<< "Task for " << pose.pdb_info()->name() << " is: \n" << *(design_task)  << std::endl;
#endif

	// iterate over all residues
	for ( Size ii = 1; ii<= design_task->total_residue(); ++ii ) {
		if ( design_task->being_designed( ii ) ) {
			designable_set.insert( ii );
		}
	}

	return designable_set;

}


/// @brief iterates over all designed positions and determines identity to native. outputs recoveries to file.
void measure_sequence_recovery( utility::vector1<core::pose::Pose> & native_poses, utility::vector1<core::pose::Pose> & redesign_poses ) {

	// setup main arrays used for calculation
	// All proteins - all residues
	utility::vector1< core::Size > n_correct( chemical::num_canonical_aas, 0 );
	utility::vector1< core::Size > n_native( chemical::num_canonical_aas, 0 );
	utility::vector1< core::Size > n_designed( chemical::num_canonical_aas, 0 );

	// All proteins - core/buried residues
	utility::vector1< core::Size > n_correct_core( chemical::num_canonical_aas, 0 );
	utility::vector1< core::Size > n_native_core( chemical::num_canonical_aas, 0 );
	utility::vector1< core::Size > n_designed_core( chemical::num_canonical_aas, 0 );

	// All proteins - surface exposed residues
	utility::vector1< core::Size > n_correct_surface( chemical::num_canonical_aas, 0 );
	utility::vector1< core::Size > n_native_surface( chemical::num_canonical_aas, 0 );
	utility::vector1< core::Size > n_designed_surface( chemical::num_canonical_aas, 0 );

	// Membrane proteins - aqueous facing residues
	utility::vector1< core::Size > n_correct_aqueous( chemical::num_canonical_aas, 0 );
	utility::vector1< core::Size > n_native_aqueous( chemical::num_canonical_aas, 0 );
	utility::vector1< core::Size > n_designed_aqueous( chemical::num_canonical_aas, 0 );

	// Membrane proteins - interfacial residues
	utility::vector1< core::Size > n_correct_interface( chemical::num_canonical_aas, 0 );
	utility::vector1< core::Size > n_native_interface( chemical::num_canonical_aas, 0 );
	utility::vector1< core::Size > n_designed_interface( chemical::num_canonical_aas, 0 );

	// Membrane proteins - lipid facing residues
	utility::vector1< core::Size > n_correct_lipid_facing( chemical::num_canonical_aas, 0 );
	utility::vector1< core::Size > n_native_lipid_facing( chemical::num_canonical_aas, 0 );
	utility::vector1< core::Size > n_designed_lipid_facing( chemical::num_canonical_aas, 0 );

	ObjexxFCL::FArray2D_int sub_matrix( chemical::num_canonical_aas, chemical::num_canonical_aas, 0 );

	Size n_correct_total(0); Size n_total(0);
	Size n_correct_total_core(0); Size n_total_core(0);
	Size n_correct_total_surface(0); Size n_total_surface(0);
	Size n_correct_total_aqueous(0); Size n_total_aqueous(0);
	Size n_correct_total_interface(0); Size n_total_interface(0);
	Size n_correct_total_lipid_facing(0); Size n_total_lipid_facing(0);

	Size surface_exposed_cutoff = option[ sequence_recovery::se_cutoff ];
	Size core_cutoff = 24;

	// iterate through all the structures
	utility::vector1< core::pose::Pose >::iterator native_itr( native_poses.begin() ), native_last( native_poses.end() );
	utility::vector1< core::pose::Pose >::iterator redesign_itr( redesign_poses.begin() ), redesign_last( redesign_poses.end() );

	while ( ( native_itr != native_last ) && (redesign_itr != redesign_last ) ) {

		// get local copies of the poses
		core::pose::Pose native_pose( *native_itr );
		core::pose::Pose redesign_pose( *redesign_itr );

		// figure out the task & neighbor info
		core::pack::task::TaskFactoryOP task_factory( new core::pack::task::TaskFactory );
		std::set< Size > design_set;
		utility::vector1< core::Size > num_neighbors;
		// setup what residues we are going to look at...
		setup_tf( task_factory );
		design_set = fill_designable_set( native_pose, task_factory );
		fill_num_neighbors( native_pose, num_neighbors );

		// record native sequence
		// native_sequence vector is sized for the WHOLE pose not just those being designed
		// it doesn't matter because we only iterate over the number of designed positions
		Size const nres( native_pose.size() );
		utility::vector1< chemical::AA > native_sequence( nres );

		// iterate over designable positions
		for ( std::set< core::Size >::const_iterator it = design_set.begin(), end = design_set.end(); it != end; ++it ) {

			if ( ! native_pose.residue(*it).is_protein() ) {
				native_sequence[ *it ] = chemical::aa_unk;
				continue;
			}
			//figure out info about the native pose
			native_sequence[ *it ] = native_pose.residue( *it ).aa();
			n_native[ native_pose.residue(*it).aa() ]++;

			//determine core/surface
			if ( num_neighbors[*it] >= core_cutoff ) {
				n_native_core[ native_pose.residue(*it).aa() ]++;
				n_total_core++;
			}

			if ( num_neighbors[*it] < surface_exposed_cutoff ) {
				n_native_surface[ native_pose.residue(*it).aa() ]++;
				n_total_surface++;

				// determine aqueous/interface/lipid_facing
				numeric::xyzVector< core::Real > xyz( native_pose.residue( *it ).atom( "CA" ).xyz() );
				xyz.z() = native_pose.conformation().membrane_info()->atom_z_position( native_pose.conformation(), *it, 2 /* CA */ );
				core::Real s = native_pose.conformation().membrane_info()->membrane_geometry()->f_transition( native_pose.conformation(), *it, 2 /* CA */ );

				if ( s < 0.25 ) {
					n_native_lipid_facing[ native_pose.residue(*it).aa() ]++;
					n_total_lipid_facing++;
				} else if ( 0.25 <= s && s <= 0.75 ) {
					n_native_interface[ native_pose.residue(*it).aa() ]++;
					n_total_interface++;
				} else if ( 0.75 < s ) {
					n_native_aqueous[ native_pose.residue(*it).aa() ]++;
					n_total_aqueous++;
				}
			}

		} // end finding native seq

		/// measure seq recov
		for ( std::set< core::Size >::const_iterator it = design_set.begin(), end = design_set.end(); it != end; ++it ) {

			// calculating the CA scaling in the redesigned pose
			numeric::xyzVector< core::Real > xyz( native_pose.residue( *it ).atom( "CA" ).xyz() );
			xyz.z() = native_pose.conformation().membrane_info()->atom_z_position( native_pose.conformation(), *it, 2 /* CA */ );
			core::Real s = native_pose.conformation().membrane_info()->membrane_geometry()->f_transition( native_pose.conformation(), *it, 2 /* CA */ );

			// don't worry about recovery of non-protein residues
			if ( redesign_pose.residue( *it ).is_protein() ) {
				n_total++;
				// std::cout << *it << " " << s << " " <<  redesign_pose.residue( *it ).name3() << " " << native_sequence[ *it ] << std::endl;

				// increment the designed count
				n_designed[ redesign_pose.residue(*it).aa() ]++;

				if ( num_neighbors[*it] >= core_cutoff ) { n_designed_core[ redesign_pose.residue(*it).aa() ]++; }
				if ( num_neighbors[*it] < surface_exposed_cutoff ) {

					// Increment the overall surface designed count
					n_designed_surface[ redesign_pose.residue(*it).aa() ]++;

					// Increment membrane surface exposed categories as appropriate
					if ( s < 0.25 ) {
						n_designed_lipid_facing[ redesign_pose.residue(*it).aa() ]++;
					} else if ( 0.25 <= s && s <= 0.75 ) {
						n_designed_interface[ redesign_pose.residue(*it).aa() ]++;
					} else if ( 0.75 < s ) {
						n_designed_aqueous[ redesign_pose.residue(*it).aa() ]++;
					}
				}

				// then check if it's the same
				if ( native_sequence[ *it ] == redesign_pose.residue(*it).aa() ) {
					n_correct[ redesign_pose.residue(*it).aa() ]++;

					if ( num_neighbors[*it] >= core_cutoff ) {
						n_correct_core[ redesign_pose.residue(*it).aa() ]++;
						n_correct_total_core++;
					}
					if ( num_neighbors[*it] < surface_exposed_cutoff ) {

						// Increment for surface
						n_correct_surface[ redesign_pose.residue(*it).aa() ]++;
						n_correct_total_surface++;

						// now check membrane categories
						if ( s < 0.25 ) {
							n_correct_lipid_facing[ redesign_pose.residue(*it).aa() ]++;
							n_correct_total_lipid_facing++;
						} else if ( 0.25 <= s && s <= 0.75 ) {
							n_correct_interface[ redesign_pose.residue(*it).aa() ]++;
							n_correct_total_interface++;
						} else if ( 0.75 < s ) {
							n_correct_aqueous[ redesign_pose.residue(*it).aa() ]++;
							n_correct_total_aqueous++;
						}

					}
				}

				n_correct_total++;

				// set the substitution matrix for this go round
				sub_matrix( native_pose.residue(*it).aa(), redesign_pose.residue(*it).aa() )++;
			}

		} // end measure seq reovery

		// increment iterators
		native_itr++; redesign_itr++;
	}

	// open sequence recovery file stream
	utility::io::ozstream outputFile( option[ sequence_recovery::seq_recov_filename ].value() ) ;

	// write header
	outputFile << "Residue\tNo.correct.core\tNo.native.core\tNo.designed.core\tCorrect.v.native.core\tCorrect.v.designed.core\t"
		<< "No.correct\tNo.native\tNo.designed\tCorrect.v.native\tCorrect.v.designed\t"
		<< "No.correct.surface\tNo.native.surface\tNo.designed.surface\tCorrect.v.native.surface\tCorrect.v.designed.surface\t"
		<< "No.correct.lipid.facing\tNo.native.lipid.facing\tNo.designed.lipid.facing\tCorrect.v.native.lipid.facing\tCorrect.v.designed.lipid.facing\t"
		<< "No.correct.interface\tNo.native.interface\tNo.designed.interface\tCorrect.v.native.interface\tCorrect.v.designed.interface\t"
		<< "No.correct.aqueous\tNo.native.aqueous\tNo.designed.aqueous\tCorrect.v.native.aqueous\tCorrect.v.designed.aqueous" << std::endl;

	// write AA data
	for ( Size ii = 1; ii <= chemical::num_canonical_aas; ++ii ) {

		outputFile << chemical::name_from_aa( chemical::AA(ii) ) << "\t"
			<< n_correct_core[ ii ] << "\t" << n_native_core[ ii ] << "\t" << n_designed_core[ ii ] << "\t";

		if ( n_native_core[ii] != 0 ) outputFile << F(4,2, (float)n_correct_core[ii]/n_native_core[ii] ) << "\t";
		else outputFile << "0.0\t";
		if ( n_designed_core[ii] != 0 ) outputFile << F(4,2, (float)n_correct_core[ii]/n_designed_core[ii] ) << "\t";
		else outputFile << "0.0\t";

		outputFile << n_correct[ ii ] << "\t" << n_native[ ii ] << "\t" << n_designed[ ii ] << "\t";
		if ( n_native[ii] != 0 ) outputFile << F(4,2, (float)n_correct[ii]/n_native[ii] ) << "\t";
		else outputFile << "0.0\t";
		if ( n_designed[ii] != 0 ) outputFile << F(4,2, (float)n_correct[ii]/n_designed[ii] ) << "\t";
		else outputFile << "0.0\t";

		outputFile << n_correct_surface[ ii ] << "\t" << n_native_surface[ ii ] << "\t" << n_designed_surface[ ii ] << "\t";

		if ( n_native_surface[ii] != 0 ) outputFile << F(4,2, (float)n_correct_surface[ii]/n_native_surface[ii] ) << "\t";
		else outputFile << "0.0\t";
		if ( n_designed_surface[ii] != 0 ) outputFile << F(4,2, (float)n_correct_surface[ii]/n_designed_surface[ii] ) << "\t";
		else outputFile << "0.0\t";

		outputFile << n_correct_lipid_facing[ ii ] << "\t" << n_native_lipid_facing[ ii ] << "\t" << n_designed_lipid_facing[ ii ] << "\t";

		if ( n_native_lipid_facing[ii] != 0 ) outputFile << F(4,2, (float)n_correct_lipid_facing[ii]/n_native_lipid_facing[ii] ) << "\t";
		else outputFile << "0.0\t";
		if ( n_designed_lipid_facing[ii] != 0 ) outputFile << F(4,2, (float)n_correct_lipid_facing[ii]/n_designed_lipid_facing[ii] ) << "\t";
		else outputFile << "0.0\t";

		outputFile << n_correct_interface[ ii ] << "\t" << n_native_interface[ ii ] << "\t" << n_designed_interface[ ii ] << "\t";

		if ( n_native_interface[ii] != 0 ) outputFile << F(4,2, (float)n_correct_interface[ii]/n_native_interface[ii] ) << "\t";
		else outputFile << "0.0\t";
		if ( n_designed_interface[ii] != 0 ) outputFile << F(4,2, (float)n_correct_interface[ii]/n_designed_interface[ii] ) << "\t";
		else outputFile << "0.0\t";

		outputFile << n_correct_aqueous[ ii ] << "\t" << n_native_aqueous[ ii ] << "\t" << n_designed_aqueous[ ii ] << "\t";

		if ( n_native_aqueous[ii] != 0 ) outputFile << F(4,2, (float)n_correct_aqueous[ii]/n_native_aqueous[ii] ) << "\t";
		else outputFile << "0.0\t";
		if ( n_designed_aqueous[ii] != 0 ) outputFile << F(4,2, (float)n_correct_aqueous[ii]/n_designed_aqueous[ii] ) << "\t";
		else outputFile << "0.0\t";

		outputFile << std::endl;
	}

	// write totals
	outputFile << "Total\t"
		<< n_correct_total_core << "\t" << n_total_core << "\t\t" << F(5,3, (float)n_correct_total_core/n_total_core ) << "\t\t"
		<< n_correct_total << "\t" << n_total << "\t\t" << F(5,3, (float)n_correct_total/n_total ) << "\t\tTotal\t"
		<< n_correct_total_surface << "\t" << n_total_surface << "\t\t" << F(5,3, (float)n_correct_total_surface/n_total_surface ) << "\t\t"
		<< n_correct_total_lipid_facing << "\t" << n_total_lipid_facing << "\t\t" << F(5,3, (float)n_correct_total_lipid_facing/n_total_lipid_facing ) << "\t\t"
		<< n_correct_total_interface << "\t" << n_total_interface << "\t\t" << F(5,3, (float)n_correct_total_interface/n_total_interface ) << "\t\t"
		<< n_correct_total_aqueous << "\t" << n_total_aqueous << "\t\t" << F(5,3, (float)n_correct_total_aqueous/n_total_aqueous ) << "\t\t"
		<< std::endl;

	// output the sequence substitution file
	utility::io::ozstream matrixFile( option[ sequence_recovery::sub_matrix_filename ].value() ) ; //defaults to submatrix.txt

	// write the header
	matrixFile << "AA_TYPE" << "\t" ;
	for ( Size ii = 1; ii <= chemical::num_canonical_aas; ++ii ) {
		matrixFile << "nat_"<<chemical::name_from_aa( chemical::AA(ii) ) << "\t";
	}
	matrixFile<<std::endl;

	// now write the numbers
	for ( Size ii = 1; ii <= chemical::num_canonical_aas; ++ii ) { //redesigns
		matrixFile << "sub_" << chemical::name_from_aa( chemical::AA(ii) );
		for ( Size jj = 1; jj <= chemical::num_canonical_aas; ++jj ) { //natives
			//std::cout<<"Native: "<< jj << " Sub: " << ii << "  Value: "<<sub_matrix( jj, ii ) << std::endl;
			matrixFile<< "\t" << sub_matrix( jj, ii );
		}
		matrixFile << std::endl;
	}

}


//@brief method which contains logic for calculating rotamer recovery. not implemented.
void measure_rotamer_recovery( utility::vector1<core::pose::Pose> & /*native_poses*/, utility::vector1<core::pose::Pose> & /*redesign_poses*/ ) {}


//@brief main method for the sequence recovery protocol
int main( int argc, char* argv[] ) {

	try {

		using utility::file::file_exists;
		using utility::file::FileName;

		option.add( sequence_recovery::native_pdb_list, "List of pdb files of the native structures." );
		option.add( sequence_recovery::redesign_pdb_list, "List of pdb files of the redesigned structures." );
		option.add( sequence_recovery::parse_taskops_file, "XML file which contains task operations to apply before measuring recovery (optional)" );
		option.add( sequence_recovery::rotamer_recovery, "Compare the rotamer recovery instead of sequence recovery." ).def( false );
		option.add( sequence_recovery::seq_recov_filename, "Name of file for sequence recovery output." ).def("sequencerecovery.txt");
		option.add( sequence_recovery::sub_matrix_filename, "Name of file substitution matrix output." ).def("submatrix.txt");
		option.add( sequence_recovery::se_cutoff, "Integer for how many nbs a residue must have less than or equal to to be considered surface exposed." ).def( 16 );


		devel::init( argc, argv );

		// changing this so that native_pdb_list and redesign_pdb_list do not have default values. giving these options can lead
		// to users measuring recovery against the wrong set of PDBs.
		if ( argc == 1 || !option[ sequence_recovery::native_pdb_list ].user() || !option[ sequence_recovery::redesign_pdb_list ].user() ) {
			init_usage_prompt( argv[0] );
			utility_exit_with_message_status( usage_string, 1 );
		}

		// read list file. open the file specified by the flag 'native_pdb_list' and read in all the lines in it
		std::vector< FileName > native_pdb_file_names;
		std::string native_pdb_list_file_name( option[ sequence_recovery::native_pdb_list ].value() );
		std::ifstream native_data( native_pdb_list_file_name.c_str() );
		std::string native_line;
		if ( !native_data.good() ) {
			utility_exit_with_message( "Unable to open file: " + native_pdb_list_file_name + '\n' );
		}
		while ( getline( native_data, native_line ) ) {
			native_pdb_file_names.push_back( FileName( native_line ) );
		}

		native_data.close();

		// read list file. open the file specified by the flag 'redesign_pdb_list' and read in all the lines in it
		std::vector< FileName > redesign_pdb_file_names;
		std::string redesign_pdb_list_file_name( option[ sequence_recovery::redesign_pdb_list ].value() );
		std::ifstream redesign_data( redesign_pdb_list_file_name.c_str() );
		std::string redesign_line;
		if ( !redesign_data.good() ) {
			utility_exit_with_message( "Unable to open file: " + redesign_pdb_list_file_name + '\n' );
		}
		while ( getline( redesign_data, redesign_line ) ) {
			redesign_pdb_file_names.push_back( FileName( redesign_line ) );
		}
		redesign_data.close();

		// check that the vectors are the same size. if not error out immediately.
		if ( native_pdb_file_names.size() != redesign_pdb_file_names.size() ) {
			utility_exit_with_message( "Size of native pdb list file: " + native_pdb_list_file_name + " does not equal size of redesign pdb list: " + redesign_pdb_list_file_name + "!\n" );
		}

		// iterate over both FileName vector and read in the PDB files
		utility::vector1< pose::Pose > native_poses;
		utility::vector1< pose::Pose > redesign_poses;

		std::vector< FileName >::iterator native_pdb( native_pdb_file_names.begin() ), native_last_pdb(native_pdb_file_names.end());
		std::vector< FileName >::iterator redesign_pdb( redesign_pdb_file_names.begin() ), redesign_last_pdb(redesign_pdb_file_names.end());

		while ( ( native_pdb != native_last_pdb ) && ( redesign_pdb != redesign_last_pdb ) ) {

			// check to make sure the file exists
			if ( !file_exists( *native_pdb ) ) {
				utility_exit_with_message( "Native pdb " + std::string(*native_pdb) + " not found! skipping" );
			}
			if ( !file_exists( *redesign_pdb ) ) {
				utility_exit_with_message( "Redesign pdb " + std::string(*redesign_pdb) + " not found! skipping" );
			}

			// Get tag for spanfile
			std::string temp = std::string(*native_pdb);
			std::string tempstr = temp.substr(0, temp.size()-4 );
			std::string filename( tempstr + ".span" );
			TR << "The filename is " << filename << std::endl;

			// Create a single Add Membrane Mover based on this spanfile
			using namespace protocols::membrane;
			AddMembraneMoverOP add_memb = AddMembraneMoverOP( new AddMembraneMover( filename ) );

			TR << "Reading in poses " << *native_pdb << " and " << *redesign_pdb << std::endl;

			core::pose::Pose native_pose, redesign_pose;
			core::import_pose::pose_from_file( native_pose, *native_pdb , core::import_pose::PDB_file);
			add_memb->apply( native_pose ); // add membrane framework hooks
			core::import_pose::pose_from_file( redesign_pose, *redesign_pdb , core::import_pose::PDB_file);
			add_memb->apply( redesign_pose ); // add membrane framework hooks
			native_poses.push_back( native_pose ); redesign_poses.push_back( redesign_pose );
			native_pdb++; redesign_pdb++;
		}


		if ( option[ sequence_recovery::rotamer_recovery ].value() ) {
			TR << "Measuring rotamer recovery"  << std::endl;
			measure_rotamer_recovery( native_poses, redesign_poses );
		} else {
			TR << "Measuring sequence recovery" << std::endl;
			measure_sequence_recovery( native_poses, redesign_poses );
		}
	} catch ( utility::excn::Exception const & e ) {
		e.display();
		return -1;
	}

}

