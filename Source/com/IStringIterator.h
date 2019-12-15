#pragma once

// ---- Include system wide include files ----

// ---- Include local include files ----
#include "Module.h"
#include "IRPCIterator.h"

namespace WPEFramework {
namespace RPC {

    typedef IIteratorType<string, ID_STRINGITERATOR> IStringIterator;
    typedef IteratorType<IStringIterator> StringIterator;
}
}
