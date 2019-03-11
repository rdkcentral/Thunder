#ifndef __IOPENCDMI_H
#define __IOPENCDMI_H

#include <core/core.h>
#include <com/com.h>

namespace OCDM {

typedef int32_t OCDM_RESULT;

// ISession defines the interface towards a DRM context that can decrypt data
// using a given key.
struct ISession : virtual public WPEFramework::Core::IUnknown {
    enum KeyStatus {
        Usable = 0,
        Expired,
        Released,
        OutputRestricted,
        OutputDownscaled,
        StatusPending,
        InternalError
    };

    // ICallback defines the callback interface to receive
    // events originated from the session.
    struct ICallback : virtual public WPEFramework::Core::IUnknown {
        enum { ID = WPEFramework::RPC::ID_SESSION_CALLBACK };

        virtual ~ICallback() {}

        // Event fired when a key message is successfully created.
        virtual void OnKeyMessage(
            const uint8_t* keyMessage, //__in_bcount(f_cbKeyMessage)
            const uint16_t keyLength, //__in
            const std::string URL)
            = 0; //__in_z_opt

        // Event fired when MediaKeySession has found a usable key.
        virtual void OnKeyReady() = 0;

        // Event fired when MediaKeySession encounters an error.
        virtual void OnKeyError(
            const int16_t error,
            const OCDM_RESULT sysError,
            const std::string errorMessage)
            = 0;

        // Event fired on key status update
        virtual void OnKeyStatusUpdate(const ISession::KeyStatus status) = 0;
    };

    enum { ID = WPEFramework::RPC::ID_SESSION };

    virtual ~ISession(void) {}

    // Loads the data stored for the specified session into the cdm object
    virtual OCDM_RESULT Load() = 0;

    // Process a key message response.
    virtual void Update(
        const uint8_t* keyMessage, //__in_bcount(f_cbKeyMessageResponse)
        const uint16_t keyLength)
        = 0; //__in

    //Removes all license(s) and key(s) associated with the session
    virtual OCDM_RESULT Remove() = 0;

    //Report the current status of the Session with respect to the KeyExchange.
    virtual KeyStatus Status() const = 0;

    //Report the name to be used for the Shared Memory for exchanging the Encrypted fragements.
    virtual std::string BufferId() const = 0;

    //Report the name to be used for the Shared Memory for exchanging the Encrypted fragements.
    virtual std::string SessionId() const = 0;

    //We are completely done with the session, it can be closed.
    virtual void Close() = 0;

    //During instantiation a callback is set, here we can decouple.
    virtual void Revoke(OCDM::ISession::ICallback* callback) = 0;
};

struct IAccessorOCDM : virtual public WPEFramework::Core::IUnknown {

    enum { ID = WPEFramework::RPC::ID_ACCESSOROCDM };

    struct INotification : virtual public WPEFramework::Core::IUnknown {

        enum { ID = WPEFramework::RPC::ID_ACCESSOROCDM_NOTIFICATION };

        virtual ~INotification() {}

        virtual void Create(const string& sessionId) = 0;
        virtual void Destroy(const string& sessionId) = 0;
        virtual void KeyChange(const string& sessionId, const uint8_t keyId[], const uint8_t length, const OCDM::ISession::KeyStatus status) = 0;
    };

    virtual ~IAccessorOCDM() {}

    virtual OCDM::OCDM_RESULT IsTypeSupported(
        const std::string keySystem,
        const std::string mimeType) const = 0;

    // Create a MediaKeySession using the supplied init data and CDM data.
    virtual OCDM_RESULT CreateSession(
        const string keySystem,
        const int32_t licenseType,
        const std::string initDataType,
        const uint8_t* initData,
        const uint16_t initDataLength,
        const uint8_t* CDMData,
        const uint16_t CDMDataLength,
        ISession::ICallback* callback,
        std::string& sessionId,
        ISession*& session)
        = 0;

    // Set Server Certificate
    virtual OCDM_RESULT SetServerCertificate(
        const string keySystem,
        const uint8_t* serverCertificate,
        const uint16_t serverCertificateLength)
        = 0;

    virtual void Register(
        INotification* sink)
        = 0;

    virtual void Unregister(
        INotification* sink)
        = 0;

    virtual ISession* Session(
        const std::string sessionId)
        = 0;

    virtual ISession* Session(
        const uint8_t keyId[], const uint8_t length)
        = 0;
};
}

#endif // __OPENCDMI_
