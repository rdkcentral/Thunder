#pragma once

#include "IDriver.h"
#include "HCISocket.h"
#include "GATTSocket.h"
#include "Profile.h"

#ifdef __WINDOWS__
#pragma comment(lib, "bluetooth.lib")
#endif
