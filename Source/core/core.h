 /*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __GENERICS_H
#define __GENERICS_H

#ifndef MODULE_NAME
#error "Please define a MODULE_NAME that describes the binary/library you are building."
#endif

#include "Module.h"
#include "Portability.h"

#include "IAction.h"
#include "IIterator.h"
#include "IObserver.h"

#include "ASN1.h"
#include "DoorBell.h"
#include "CyclicBuffer.h"
#include "DataBuffer.h"
#include "DataElement.h"
#include "DataElementFile.h"
#include "Enumerate.h"
#include "Factory.h"
#include "FileSystem.h"
#include "FileObserver.h"
#include "Frame.h"
#include "IPCMessage.h"
#include "IPCChannel.h"
#include "IPCConnector.h"
#include "ISO639.h"
#include "IPFrame.h"
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
#include "Rectangle.h"
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
#include "StopWatch.h"
#include "Stream.h"
#include "StreamJSON.h"
#include "StreamText.h"
#include "StreamTypeLengthValue.h"
#include "Sync.h"
#include "Synchronize.h"
#include "SynchronousChannel.h"
#include "SystemInfo.h"
#include "TextFragment.h"
#include "TextReader.h"
#include "Thread.h"
#include "ThreadPool.h"
#include "Time.h"
#include "Timer.h"
#include "Trace.h"
#include "TriState.h"
#include "TypeTraits.h"
#include "ValueRecorder.h"
#include "XGetopt.h"
#include "WorkerPool.h"
#include "IWarningReportingControl.h"
#include "WarningReportingControl.h"
#include "WarningReportingCategories.h"
#include "CallsignTLS.h"
#include "TokenizedStringList.h"
#include "MessageStore.h"

#ifdef __WINDOWS__
#pragma comment(lib, "core.lib")
#endif

#endif
