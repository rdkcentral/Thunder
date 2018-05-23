#ifndef __PLUGIN_FRAMEWORK_SUPPORT_H
#define __PLUGIN_FRAMEWORK_SUPPORT_H

#include "Module.h"

#include "Configuration.h"
#include "IPlugin.h"
#include "IShell.h"
#include "IStateControl.h"
#include "ISubSystem.h"
#include "IRPCIterator.h"
#include "Request.h"
#include "Service.h"
#include "Channel.h"
#include "VirtualInput.h"
#include "Logging.h"

#ifdef __WIN32__
#pragma comment(lib, "plugins.lib")
#endif

#endif // __PLUGIN_FRAMEWORK_SUPPORT_H
