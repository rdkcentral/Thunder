#pragma once

// ---- Include system wide include files ----

// ---- Include local include files ----
#include "Module.h"
#include "IRPCIterator.h"

namespace WPEFramework {
namespace RPC {

    typedef IIteratorType<uint32_t, ID_VALUEITERATOR> IValueIterator;
    typedef IteratorType<IValueIterator> ValueIterator;

}
}
