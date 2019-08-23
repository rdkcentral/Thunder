#ifndef __IOPENCDMI_H
#define __IOPENCDMI_H

#include <com/com.h>
#include <core/core.h>

namespace OCDM {

typedef enum {
    OCDM_SUCCESS = 0,
    OCDM_S_FALSE = 1,
    OCDM_KEYSYSTEM_NOT_SUPPORTED = 0x80000002,
    OCDM_INVALID_SESSION = 0x80000003,
    OCDM_INVALID_DECRYPT_BUFFER = 0x80000004,
    OCDM_OUT_OF_MEMORY = 0x80000005,
    OCDM_FAIL = 0x80004005,
    OCDM_INVALID_ARG = 0x80070057,
    OCDM_SERVER_INTERNAL_ERROR = 0x8004C600,
    OCDM_SERVER_INVALID_MESSAGE = 0x8004C601,
    OCDM_SERVER_SERVICE_SPECIFIC = 0x8004C604,
} OCDM_RESULT;

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
        virtual void OnKeyMessage(const uint8_t* keyMessage /* @length:keyLength */, //__in_bcount(f_cbKeyMessage)
            const uint16_t keyLength, //__in
            const std::string& URL) = 0; //__in_z_opt

        // Event fired when MediaKeySession encounters an error.
        virtual void OnError(const int16_t error, const OCDM_RESULT sysError, const std::string& errorMessage) = 0;

        // Event fired on key status update
        virtual void OnKeyStatusUpdate(const uint8_t keyID[] /* @length:keyIDLength */,
                                       const uint8_t keyIDLength,
                                       const ISession::KeyStatus status) = 0;

        virtual void OnKeyStatusesUpdated() const = 0;
    };

    enum { ID = WPEFramework::RPC::ID_SESSION };

    virtual ~ISession(void) {}

    // Loads the data stored for the specified session into the cdm object
    virtual OCDM_RESULT Load() = 0;

    // Process a key message response.
    virtual void
    Update(const uint8_t* keyMessage /* @length:keyLength */, //__in_bcount(f_cbKeyMessageResponse)
        const uint16_t keyLength)
        = 0; //__in

    // Removes all license(s) and key(s) associated with the session
    virtual OCDM_RESULT Remove() = 0;

    // Report the current status of the Session with respect to the KeyExchange.
    virtual KeyStatus Status() const = 0;
    virtual KeyStatus Status(const uint8_t keyID[] /* @length:keyIDLength */, const uint8_t keyIDLength) const = 0;

    // Report the name to be used for the Shared Memory for exchanging the
    // Encrypted fragements.
    virtual std::string BufferId() const = 0;

    // Report the name to be used for the Shared Memory for exchanging the
    // Encrypted fragements.
    virtual std::string SessionId() const = 0;

    // We are completely done with the session, it can be closed.
    virtual void Close() = 0;

    // During instantiation a callback is set, here we can decouple.
    virtual void Revoke(OCDM::ISession::ICallback* callback) = 0;
};

struct ISessionExt : virtual public WPEFramework::Core::IUnknown {
    enum { ID = WPEFramework::RPC::ID_SESSION_EXTENSION };

    enum LicenseTypeExt { Invalid = 0,
        LimitedDuration,
        Standard };

    enum SessionStateExt {
        LicenseAcquisitionState = 0,
        InactiveDecryptionState,
        ActiveDecryptionState,
        InvalidState
    };

    virtual uint32_t SessionIdExt() const = 0;

    // Report the name to be used for the Shared Memory for exchanging the
    // Encrypted fragements.
    virtual std::string BufferIdExt() const = 0;

    virtual OCDM_RESULT SetDrmHeader(const uint8_t drmHeader[] /* @length:drmHeaderLength */,
        uint32_t drmHeaderLength)
        = 0;

    virtual OCDM_RESULT GetChallengeDataExt(uint8_t* challenge /* @inout @length:challengeSize */,
        uint32_t& challengeSize /* @inout */,
        uint32_t isLDL)
        = 0;

    virtual OCDM_RESULT CancelChallengeDataExt() = 0;

    virtual OCDM_RESULT StoreLicenseData(const uint8_t licenseData[] /* @length:licenseDataSize */,
        uint32_t licenseDataSize,
        uint8_t* secureStopId /* @out @length:16 */)
        = 0;

    virtual OCDM_RESULT InitDecryptContextByKid() = 0;

    virtual OCDM_RESULT CleanDecryptContext() = 0;
};

struct IAccessorOCDM : virtual public WPEFramework::Core::IUnknown {

    enum { ID = WPEFramework::RPC::ID_ACCESSOROCDM };

    struct INotification : virtual public WPEFramework::Core::IUnknown {

        enum { ID = WPEFramework::RPC::ID_ACCESSOROCDM_NOTIFICATION };

        virtual ~INotification() {}

        virtual void Create(const string& sessionId) = 0;
        virtual void Destroy(const string& sessionId) = 0;
        virtual void KeyChange(const string& sessionId, const uint8_t keyId[] /* @length:length */,
            const uint8_t length,
            const OCDM::ISession::KeyStatus status)
            = 0;
    };

    virtual ~IAccessorOCDM() {}

    virtual bool IsTypeSupported(const std::string keySystem,
        const std::string mimeType) const = 0;

    // Create a MediaKeySession using the supplied init data and CDM data.
    virtual OCDM_RESULT
    CreateSession(const string keySystem, const int32_t licenseType,
        const std::string initDataType, const uint8_t* initData /* @length:initDataLength */,
        const uint16_t initDataLength, const uint8_t* CDMData /* @length:CDMDataLength */,
        const uint16_t CDMDataLength, ISession::ICallback* callback,
        std::string& sessionId /* @out */, ISession*& session /* @out */)
        = 0;

    // Set Server Certificate
    virtual OCDM_RESULT
    SetServerCertificate(const string keySystem, const uint8_t* serverCertificate /* @length:serverCertificateLength */,
        const uint16_t serverCertificateLength)
        = 0;

    virtual void Register(INotification* sink) = 0;

    virtual void Unregister(INotification* sink) = 0;

    virtual ISession* Session(const std::string sessionId) = 0;

    virtual ISession* Session(const uint8_t keyId[] /* @length:length */, const uint8_t length) = 0;
};

struct IAccessorOCDMExt : virtual public WPEFramework::Core::IUnknown {

    enum { ID = WPEFramework::RPC::ID_ACCESSOROCDM_EXTENSION };

    virtual time_t GetDrmSystemTime(const std::string& keySystem) const = 0;

    virtual OCDM_RESULT CreateSessionExt(
        const std::string keySystem,
        const uint8_t drmHeader[] /* @length:drmHeaderLength */,
        uint32_t drmHeaderLength,
        ::OCDM::ISession::ICallback* callback,
        std::string& sessionId /* @out */,
        ISessionExt*& session /* @out */)
        = 0;

    virtual std::string GetVersionExt(const std::string& keySystem) const = 0;

    virtual uint32_t GetLdlSessionLimit(const std::string& keySystem) const = 0;

    virtual bool IsSecureStopEnabled(const std::string& keySystem) = 0;

    virtual OCDM_RESULT EnableSecureStop(const std::string& keySystem,
        bool enable)
        = 0;

    virtual uint32_t ResetSecureStops(const std::string& keySystem) = 0;

    virtual OCDM_RESULT GetSecureStopIds(const std::string& keySystem,
        uint8_t ids[] /* @out @length:idSize */, uint8_t idSize,
        uint32_t& count /* @inout */)
        = 0;

    virtual OCDM_RESULT GetSecureStop(const std::string& keySystem,
        const uint8_t sessionID[] /* @length:sessionIDLength */,
        uint32_t sessionIDLength, uint8_t* rawData /* @out @length:rawSize */,
        uint16_t& rawSize /* @inout */)
        = 0;

    virtual OCDM_RESULT CommitSecureStop(const std::string& keySystem,
        const uint8_t sessionID[] /* @length:sessionIDLength */,
        uint32_t sessionIDLength,
        const uint8_t serverResponse[] /* @length:serverResponseLength */,
        uint32_t serverResponseLength)
        = 0;

    virtual OCDM_RESULT CreateSystemExt(const std::string& keySystem) = 0;

    virtual OCDM_RESULT InitSystemExt(const std::string& keySystem) = 0;

    virtual OCDM_RESULT TeardownSystemExt(const std::string& keySystem) = 0;

    virtual OCDM_RESULT DeleteKeyStore(const std::string& keySystem) = 0;

    virtual OCDM_RESULT DeleteSecureStore(const std::string& keySystem) = 0;

    virtual OCDM_RESULT GetKeyStoreHash(const std::string& keySystem,
        uint8_t keyStoreHash[] /* @out @length:keyStoreHashLength */,
        uint32_t keyStoreHashLength)
        = 0;

    virtual OCDM_RESULT GetSecureStoreHash(const std::string& keySystem,
        uint8_t secureStoreHash[] /* @out @length:secureStoreHashLength */,
        uint32_t secureStoreHashLength)
        = 0;
};
}

#endif // __OPENCDMI_
