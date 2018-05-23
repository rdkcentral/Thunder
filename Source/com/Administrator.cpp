#include "Administrator.h"

namespace WPEFramework {
namespace RPC {
    Administrator::Administrator()
        : _stubs()
        , _proxy()
        , _factory(8)
    {
    }

    /* virtual */ Administrator::~Administrator()
    {
    }

    /* static */ Administrator& Administrator::Instance()
    {
        static Administrator systemAdministrator;

        return (systemAdministrator);
    }
}
} // namespace Core
