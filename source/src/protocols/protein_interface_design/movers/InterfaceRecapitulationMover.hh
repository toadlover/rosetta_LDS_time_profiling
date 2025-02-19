// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file protocols/protein_interface_design/movers/InterfaceRecapitulationMover.hh
/// @author Sarel Fleishman (sarelf@u.washington.edu)

#ifndef INCLUDED_protocols_protein_interface_design_movers_InterfaceRecapitulationMover_hh
#define INCLUDED_protocols_protein_interface_design_movers_InterfaceRecapitulationMover_hh

// Project Headers
#include <core/pose/Pose.fwd.hh>
#include <protocols/moves/Mover.hh>
#include <protocols/minimization_packing/PackRotamersMover.fwd.hh>
#include <utility/tag/Tag.fwd.hh>
#include <basic/datacache/DataMap.fwd.hh>

// C++ headers
#include <string>


//Auto Headers
#include <protocols/calc_taskop_movers/DesignRepackMover.fwd.hh>


// Unit headers

namespace protocols {
namespace protein_interface_design {
namespace movers {

/// @brief a pure virtual base class for movers which redesign and repack the interface
class InterfaceRecapitulationMover : public protocols::moves::Mover
{
public:
	InterfaceRecapitulationMover();
	void apply( core::pose::Pose & pose ) override;
	protocols::moves::MoverOP clone() const override;
	protocols::moves::MoverOP fresh_instance() const override;
	void set_reference_pose( core::pose::PoseCOP );
	core::pose::PoseCOP get_reference_pose() const;
	void parse_my_tag( utility::tag::TagCOP, basic::datacache::DataMap & ) override;
	~InterfaceRecapitulationMover() override;

	std::string
	get_name() const override;

	static
	std::string
	mover_name();

	static
	void
	provide_xml_schema( utility::tag::XMLSchemaDefinition & xsd );

private:
	core::pose::PoseCOP saved_pose_;
	calc_taskop_movers::DesignRepackMoverOP design_mover_;
	protocols::minimization_packing::PackRotamersMoverOP design_mover2_;//ugly adaptation for the PackRotamers baseclass
	bool pssm_;
};

} // movers
} // protein_interface_design
} // protocols

#endif /*INCLUDED_protocols_protein_interface_design_movers_InterfaceRecapitulationMover_HH*/
