#include "TraceCategories.h"

// ---- Class Definition ----
namespace WPEFramework {
	namespace Trace {

		/* static */ const std::string Constructor::_text("Constructor called");
		/* static */ const std::string Destructor::_text("Destructor called");
		/* static */ const std::string CopyConstructor::_text("Copy Constructor called");
		/* static */ const std::string AssignmentOperator::_text("Assignment Operator called");

	}
} // namespace Trace

