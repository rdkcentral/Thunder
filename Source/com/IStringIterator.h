#pragma once

// ---- Include system wide include files ----

// ---- Include local include files ----
#include "Module.h"
#include "IRPCIterator.h"

namespace WPEFramework {
namespace RPC {

    typedef IRPCIteratorType<string, ID_STRINGITERATOR> IStringIterator;
    typedef RPCIteratorType<IStringIterator> StringIterator;

}
}
