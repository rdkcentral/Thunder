#ifndef __CONFIG_FRAMEWORKSUPPORT_H
#define __CONFIG_FRAMEWORKSUPPORT_H

// This header file should be used to turn on and off non-functional code.
// Non-functional code is considered to be code that the application can
// do without. Examples are performance counter code, in the framework
// the number of request being processed are being counted. For the product
// this has no added value, other then gathering statistics on its use,
// mainly intersting for the developper to take the proper optimization
// decisions for the code.

#define RUNTIME_STATISTICS 0

#endif
