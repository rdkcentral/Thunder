#ifndef EXTENDEDOPENCDMSESSION_H
#define EXTENDEDOPENCDMSESSION_H

#include "DataExchange.h"
#include "IOCDM.h"
#include "Module.h"
#include "open_cdm.h"
#include "open_cdm_impl.h"
using namespace WPEFramework;

// If we build in release, we do not want to "hang" forever, forcefull close after 5S waiting...
#ifdef __DEBUG__
const unsigned int a_Time = WPEFramework::Core::infinite;
#else
const unsigned int a_Time = 5000;// Expect time in MS
#endif

extern Core::CriticalSection _systemLock;
extern const char EmptyString[];

KeyStatus CDMState(const OCDM::ISession::KeyStatus state);

struct ExtendedOpenCDMSession : public OpenCDMSession {
protected:
    ExtendedOpenCDMSession() = delete;
    ExtendedOpenCDMSession(const ExtendedOpenCDMSession&) = delete;
    ExtendedOpenCDMSession& operator=(ExtendedOpenCDMSession&) = delete;

public:
    // TODO: reduce to only one ctor
    virtual ~ExtendedOpenCDMSession()
    {
        if (_sessionExt) {
           _sessionExt->Release();
           _sessionExt = nullptr;
        }

        if (OpenCDMSession::IsValid() == true) {
            Revoke(&_sink);
        }
    }

protected:
    WPEFramework::Core::Sink<Sink> _sink;
    std::string _URL;
    std::string _error;
    uint32_t _errorCode;
    OCDM::OCDM_RESULT _sysError;
    OCDM::ISession::KeyStatus _key;
//    OpenCDMSessionCallbacks* _callback;
    void* _userData;
    OCDM::ISessionExt* _sessionExt;
};

#endif /* EXTENDEDOPENCDMSESSION_H */
