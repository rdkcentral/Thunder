#ifndef __MODULE_BLUETOOTH_H
#define __MODULE_BLUETOOTH_H

#ifndef MODULE_NAME
#define MODULE_NAME Bluetooth 
#endif

#include "../core/core.h"
#include "../tracing/tracing.h"

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/mgmt.h>

#undef EXTERNAL

#ifdef __WIN32__
#ifdef BLUETOOTH_EXPORTS
#define EXTERNAL EXTERNAL_EXPORT
#else
#define EXTERNAL EXTERNAL_IMPORT
#endif
#else
#define EXTERNAL
#endif

#endif // __MODULE_BLUETOOTH_H
