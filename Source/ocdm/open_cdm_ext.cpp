#include "open_cdm_ext.h"

#include "open_cdm_impl.h"

// TODO: naming: Extended...Ext
// TODO: shares code with ExtendedOpenCDMSession, maybe intro baseclass?
struct ExtendedOpenCDMSessionExt : public OpenCDMSession {
private:
    ExtendedOpenCDMSessionExt() = delete;
    ExtendedOpenCDMSessionExt(const ExtendedOpenCDMSessionExt&) = delete;
    ExtendedOpenCDMSessionExt& operator= (ExtendedOpenCDMSessionExt&) = delete;

    enum sessionState {
        // Initialized.
        SESSION_INIT    = 0x00,

        // ExtendedOpenCDMSessionExt created, waiting for message callback.
        SESSION_MESSAGE = 0x01,
        SESSION_READY   = 0x02,
        SESSION_ERROR   = 0x04,
        SESSION_LOADED  = 0x08,
        SESSION_UPDATE  = 0x10
    };

public:
    ExtendedOpenCDMSessionExt(
        OpenCDMAccessor* system, uint32_t sessionId, const char contentId[], uint32_t contentIdLength,
        enum OcdmLicenseType licenseType, const uint8_t drmHeader[], uint32_t drmHeaderLength)
        : OpenCDMSession()
        //, _sink(this)
        , _state(SESSION_INIT)
        , _message()
        , _URL()
        , _error()
        , _errorCode(0)
        , _sysError(0)
        , _userData(this)
        , _realSession(nullptr)
     {

        std::string bufferId;
        OCDM::ISessionExt* realSession = nullptr;

        // TODO: real conversion between license types
        system->CreateSessionExt(sessionId, contentId, contentIdLength, (OCDM::IAccessorOCDMExt::LicenseTypeExt)(uint32_t)licenseType, drmHeader, drmHeaderLength, realSession);

        if (realSession == nullptr) {
            TRACE_L1("Creating a Session failed. %d", __LINE__);
        }
        else {
            // TODO ?
            OpenCDMSession::SessionExt(realSession);
        }

        _realSession = realSession;
    }

    virtual ~ExtendedOpenCDMSessionExt() {
        // TODO: do we need something like this here as well?
        /*
        if (OpenCDMSession::IsValid() == true) {
            OpenCDMSession::Session(nullptr);
        }
        */
    }

    uint32_t SessionIdExt() const
    {
        return _realSession->SessionIdExt();
    }

private:
    //WPEFramework::Core::Sink<Sink> _sink;
    WPEFramework::Core::StateTrigger<sessionState> _state;
    std::string _message;
    std::string _URL;
    std::string _error;
    uint32_t _errorCode;
    OCDM::OCDM_RESULT _sysError;
    void* _userData;
    OCDM::ISessionExt* _realSession;
};


OpenCDMError opencdm_system_get_drm_time(struct OpenCDMAccessor* system, time_t * time) {
    OpenCDMError result (ERROR_INVALID_ACCESSOR);

    if (system != nullptr) {
        *time = static_cast<OpenCDMError>(system->GetDrmSystemTime());
        result = ERROR_NONE;
    }
    return (result);
}

OpenCDMError opencdm_create_session_netflix(struct OpenCDMAccessor* system, struct OpenCDMSession ** opencdmSession, uint32_t sessionId, const char contentId[], uint32_t contentIdLength,
                                            enum OcdmLicenseType licenseType, const uint8_t drmHeader[], uint32_t drmHeaderLength)
{

    OpenCDMError result (ERROR_INVALID_ACCESSOR);

    if (system != nullptr) {
        *opencdmSession = new ExtendedOpenCDMSessionExt(system, sessionId, contentId, contentIdLength, licenseType, drmHeader, drmHeaderLength);
        result = OpenCDMError::ERROR_NONE;
    }

    return (result);
}

uint32_t opencdm_session_get_session_id_netflix(struct OpenCDMSession * opencdmSession)
{
    ExtendedOpenCDMSessionExt* sessionExt = static_cast<ExtendedOpenCDMSessionExt*>(opencdmSession);

    return sessionExt->SessionIdExt();
}
