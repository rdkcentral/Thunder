#ifndef __CONFIG_GENERICS_H
#define __CONFIG_GENERICS_H

// This header file should be used to turn on and off non-functional code.
// Non-functional code is considered to be code that the application can
// do without. Examples are perfrmanance counter code, in the generics
// SocketPort class there are counter, counting the number of iterations
// done sofar. Using this number, developpers can determine wheter it
// the socket layer is used efficiently or not. It adds no functionality
// whatsoever for the users of this class.

#define SOCKET_TEST_VECTORS 1

// Whether we need to keep track of locked critical section stack.

#endif
