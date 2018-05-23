#ifndef __GENERICS_H
#define __GENERICS_H

#include <list>
#include <map>
#include <vector>
#include <typeinfo>
#include <iostream>
#include <sstream>
#include <algorithm>

#ifdef WIN32
#include <xutility>
#endif

#include "Module.h"
#include "Portability.h"

#include "IIterator.h"
#include "IObserver.h"
#include "IAction.h"

#include "Trace.h"
#include "Sync.h"
#include "Singleton.h"
#include "TextFragment.h"
#include "Serialization.h"
#include "StateTrigger.h"
#include "Thread.h"
#include "Time.h"
#include "Timer.h"
#include "Proxy.h"
#include "Queue.h"
#include "ILogService.h"
#include "Logger.h"
#include "KeyValue.h"
#include "TriState.h"
#include "MessageException.h"
#include "DataElement.h"
#include "DataElementFile.h"
#include "DataBuffer.h"
#include "TextReader.h"
#include "ReadWriteLock.h"
#include "LockableContainer.h"
#include "Processor.h"
#include "TypeTraits.h"
#include "Optional.h"
#include "ISO639.h"
#include "SerialPort.h"
#include "Enumerate.h"
#include "Range.h"
#include "Parser.h"
#include "JSON.h"
#include "SocketPort.h"
#include "StreamText.h"
#include "StreamJSON.h"
#include "StreamTypeLengthValue.h"
#include "SocketServer.h"
#include "FileSystem.h"
#include "Library.h"
#include "Services.h"
#include "XGetopt.h"
#include "Media.h"
#include "Netlink.h"
#include "NetworkInfo.h"
#include "ASN1.h"
#include "Link.h"
#include "Factory.h"
#include "IPCConnector.h"
#include "IPCChannel.h"
#include "ValueRecorder.h"
#include "Synchronize.h"
#include "Stream.h"
#include "SystemInfo.h"
#include "CyclicBuffer.h"
#include "Process.h"
#include "ProcessInfo.h"
#include "Measurement.h"
#include "Frame.h"
#include "SharedBuffer.h"

#ifdef __WIN32__
#pragma comment(lib, "core.lib")
#endif

#endif
