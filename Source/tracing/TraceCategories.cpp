 /*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
