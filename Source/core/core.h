#ifndef __GENERICS_H
#define __GENERICS_H

#include <algorithm>
#include <iostream>
#include <list>
#include <map>
#include <sstream>
#include <typeinfo>
#include <vector>

#ifdef WIN32
#include <xutility>
#endif

#include "Module.h"
#include "Portability.h"

#include "IAction.h"
#include "IIterator.h"
#include "IObserver.h"

#include "ASN1.h"
#include "CyclicBuffer.h"
#include "DataBuffer.h"
#include "DataElement.h"
#include "DataElementFile.h"
#include "Enumerate.h"
#include "Factory.h"
#include "FileSystem.h"
#include "Frame.h"
#include "IPCChannel.h"
#include "IPCConnector.h"
#include "ISO639.h"
#include "JSON.h"
#include "JSONRPC.h"
#include "KeyValue.h"
#include "Library.h"
#include "Link.h"
#include "LockableContainer.h"
#include "Measurement.h"
#include "Media.h"
#include "MessageException.h"
#include "Netlink.h"
#include "NetworkInfo.h"
#include "Optional.h"
#include "Parser.h"
#include "Process.h"
#include "ProcessInfo.h"
#include "Proxy.h"
#include "Queue.h"
#include "Range.h"
#include "ReadWriteLock.h"
#include "ResourceMonitor.h"
#include "SerialPort.h"
#include "Serialization.h"
#include "Services.h"
#include "SharedBuffer.h"
#include "Singleton.h"
#include "SocketPort.h"
#include "SocketServer.h"
#include "StateTrigger.h"
#include "Stream.h"
#include "StreamJSON.h"
#include "StreamText.h"
#include "StreamTypeLengthValue.h"
#include "Sync.h"
#include "Synchronize.h"
#include "SystemInfo.h"
#include "TextFragment.h"
#include "TextReader.h"
#include "Thread.h"
#include "Time.h"
#include "Timer.h"
#include "Trace.h"
#include "TriState.h"
#include "TypeTraits.h"
#include "ValueRecorder.h"
#include "XGetopt.h"

#ifdef __WIN32__
#pragma comment(lib, "core.lib")
#endif

#endif
