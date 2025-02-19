#ifndef INCLUDED_devel_optest_A_hh
#define INCLUDED_devel_optest_A_hh

#include <devel/optest/A.fwd.hh>

#include <utility/VirtualBase.hh>

namespace devel {
namespace optest {

class A : public utility::VirtualBase {
public:
	/// @brief Automatically generated virtual destructor for class deriving directly from VirtualBase
	~A() override;
	A() : my_int_( ++instance_count_ ) {}

	void my_int( int setting )      { my_int_ = setting; }
	int  my_int()             const { return my_int_; }

private:

	int my_int_;

	static int instance_count_;

};


}
}

#endif
