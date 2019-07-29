#pragma once

#include "IDriver.h"
#include "HCISocket.h"
#include "GATTSocket.h"

#ifdef __WIN32__
#pragma comment(lib, "bluetooth.lib")
#endif
