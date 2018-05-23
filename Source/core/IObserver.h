#ifndef __IOBSERVER_H
#define __IOBSERVER_H

#include "Module.h"
#include "Portability.h"

namespace WPEFramework {
namespace Core {
    template <typename OBSERVING>
    struct IObserverType {
        virtual ~IObserverType(){};
        virtual void Handle(OBSERVING element);
    };
}
} // namespace Core

#endif // __IOBSERVER_H
