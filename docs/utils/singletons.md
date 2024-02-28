## Disposing singletons

    When a singleton uses a socket it must open the connection before registering itself in the singleton list (so do this in the constructor of the singleton type) to make sure it gets cleanup before the resource monitor singleton is destructed

    When a singleton uses itself (via the instance method) or any other singleton during destruction be very careful (at least when that is a singleton type) to make sure it does not create a new singleton by accident.
    
    A client lib dispose should not call Singleton::Dispose but only clean up its own internal singletons