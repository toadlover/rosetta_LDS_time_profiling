// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file protocols/filters/Filter.cc
/// @brief
/// @details
///   Contains currently:
///
///
/// @author Florian Richter, Sarel Fleishman (sarelf@uw.edu)

// Unit Headers
#include <protocols/filters/Filter.hh>


#include <basic/datacache/DataMap.fwd.hh>

// Project Headers
#include <basic/Tracer.hh>

#include <utility>
#include <utility/vector1.hh>
#include <core/pose/extra_pose_info_util.hh>


static basic::Tracer TR( "protocols.filters.Filter" );

namespace protocols {
namespace filters {

using namespace core;
typedef std::pair< std::string const, FilterCOP > StringFilter_pair;
using TagCOP = utility::tag::TagCOP;

FilterCollection::~FilterCollection() = default;

bool
FilterCollection::apply( core::pose::Pose const & pose ) const
{
	for ( protocols::filters::FilterCOP filter : filters_ ) {
		if ( ! filter->apply( pose ) ) {
			return false;
		}
	}

	return true;
}

void
FilterCollection::report( std::ostream & out, core::pose::Pose const & pose ) const
{
	for ( protocols::filters::FilterCOP filter : filters_ ) {
		filter->report( out, pose );
	}
}

Filter::Filter()
: utility::VirtualBase(),
	type_( "UNDEFINED TYPE" ),
	scorename_("defaultscorename")
{}

Filter::Filter( std::string const & type )
: utility::VirtualBase(),
	type_( type ),
	scorename_("defaultscorename")
{}

Filter::~Filter() = default;

void
Filter::parse_my_tag(
	TagCOP,
	basic::datacache::DataMap &
) {
	TR.Warning << "The parse_my_tag method has been invoked for filter " << name() << " but it hasn't been defined. Are you sure this is appropriate?" << std::endl;
}

core::Real Filter::score( core::pose::Pose & pose ) {
	core::Real score = report_sm( pose );
	core::pose::setPoseExtraScore( pose, scorename_, score );
	return score;
}

/// @brief Provide citations to the passed CitationCollectionList
/// Subclasses should add the info for themselves and any other classes they use.
/// @details The default implementation of this function does nothing.  It may be
/// overriden by filters wishing to provide citation information.
void
Filter::provide_citation_info(basic::citation_manager::CitationCollectionList & ) const {
	// Do nothing
}

} // filters
} // protocols
