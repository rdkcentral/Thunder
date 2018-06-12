/*
 * Copyright 2014 Fraunhofer FOKUS
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef CDMI_H_
#define CDMI_H_

// For the support of portable data types such as uint8_t.
#include <typeinfo>
#include <vector>
#include <string>
#include <stdint.h>

#define WARN_UNUSED_RESULT __attribute__((warn_unused_result))

class BufferReader {
private:
  BufferReader() = delete;
  BufferReader(const BufferReader&) = delete;
  BufferReader& operator=(const BufferReader&) = delete;

  // Internal implementation of multi-byte reads
  template <typename T>
  bool Read(T* v) {
    if ((v != nullptr) && (HasBytes(sizeof(T)) == true)) {
      T tmp = 0;
      for (size_t i = 0; i < sizeof(T); i++) {
        tmp <<= 8;
        tmp += buf_[pos_++];
      }
      *v = tmp;
      return true;
    }
    return false;
  }

public:
  inline BufferReader(const uint8_t* buf, size_t size)
      : buf_(buf), size_(buf != NULL ? size : 0), pos_(0) {}
  inline ~BufferReader() {}

public:
  inline bool HasBytes(size_t count) const { return pos_ + count <= size_; }
  inline bool IsEOF() const { return pos_ >= size_; }
  inline const uint8_t* data() const { return buf_; }
  inline size_t size() const { return size_; }
  inline size_t pos() const { return pos_; }

  // Read a value from the stream, performing endian correction,
  // and advance the stream pointer.
  inline bool Read1(uint8_t* v) WARN_UNUSED_RESULT { return Read(v); }
  inline bool Read2(uint16_t* v) WARN_UNUSED_RESULT { return Read(v); }
  inline bool Read2s(int16_t* v) WARN_UNUSED_RESULT { return Read(v); }
  inline bool Read4(uint32_t* v) WARN_UNUSED_RESULT { return Read(v); }
  inline bool Read4s(int32_t* v) WARN_UNUSED_RESULT { return Read(v); }
  inline bool Read8(uint64_t* v) WARN_UNUSED_RESULT { return Read(v); }
  inline bool Read8s(int64_t* v) WARN_UNUSED_RESULT { return Read(v); }

  inline bool ReadString(std::string* str, size_t count) WARN_UNUSED_RESULT {
    if ((str != nullptr) && (HasBytes(count) == true)) {
      str->assign(buf_ + pos_, buf_ + pos_ + count);
      pos_ += count;
      return true;
    }
    return false;
  }
  inline bool ReadVec(std::vector<uint8_t>* vec, size_t count) WARN_UNUSED_RESULT {
    if ((vec != nullptr) && (HasBytes(count) == true)) {
      vec->clear();
      vec->insert(vec->end(), buf_ + pos_, buf_ + pos_ + count);
      pos_ += count;
      return true;
    }
    return false;
  }

  // These variants read a 4-byte integer of the corresponding signedness and
  // store it in the 8-byte return type.
  inline bool Read4Into8(uint64_t* v) WARN_UNUSED_RESULT {
    uint32_t tmp;
    if ((v != nullptr) && (Read4(&tmp) == true)) {
      *v = tmp;
      return true;
    }
    return false;
  }
  inline bool Read4sInto8s(int64_t* v) WARN_UNUSED_RESULT {
    int32_t tmp;
    if ((v != nullptr) && (Read4s(&tmp) == true)) {
      *v = tmp;
      return true;
    }
    return false;
 
  }

  // Advance the stream by this many bytes.
  inline bool SkipBytes(size_t bytes) WARN_UNUSED_RESULT {
    if (HasBytes(bytes) == true) {
      pos_ += bytes;
      return true;
    }
    return false;
  }

private:
  const uint8_t* buf_;
  size_t size_;
  size_t pos_;
};

namespace CDMi
{

// EME error code to which CDMi errors are mapped. Please
// refer to the EME spec for details of the errors
// https://dvcs.w3.org/hg/html-media/raw-file/tip/encrypted-media/encrypted-media.html
#define MEDIA_KEYERR_UNKNOWN        1
#define MEDIA_KEYERR_CLIENT         2
#define MEDIA_KEYERR_SERVICE        3
#define MEDIA_KEYERR_OUTPUT         4
#define MEDIA_KEYERR_HARDWARECHANGE 5
#define MEDIA_KEYERR_DOMAIN         6

// The status code returned by CDMi APIs.
typedef int32_t CDMi_RESULT;

// REVIEW: Why make up new truth values, what's wrong with true/false?
#define CDMi_SUCCESS            ((CDMi_RESULT)0)
#define CDMi_S_FALSE            ((CDMi_RESULT)1)
#define CDMi_E_OUT_OF_MEMORY    ((CDMi_RESULT)0x80000002)
#define CDMi_E_FAIL             ((CDMi_RESULT)0x80004005)
#define CDMi_E_INVALID_ARG      ((CDMi_RESULT)0x80070057)

#define CDMi_E_SERVER_INTERNAL_ERROR    ((CDMi_RESULT)0x8004C600)
#define CDMi_E_SERVER_INVALID_MESSAGE   ((CDMi_RESULT)0x8004C601)
#define CDMi_E_SERVER_SERVICE_SPECIFIC  ((CDMi_RESULT)0x8004C604)

// More CDMi status codes can be defined. In general
// CDMi status codes should use the same PK error codes.

#define CDMi_FAILED(Status)     ((CDMi_RESULT)(Status)<0)
#define CDMi_SUCCEEDED(Status)  ((CDMi_RESULT)(Status) >= 0)

/* Media Key status required by EME */
#define  MEDIA_KEY_STATUS_USABLE 0
#define  MEDIA_KEY_STATUS_INTERNAL_ERROR  1
#define  MEDIA_KEY_STATUS_EXPIRED  2
#define  MEDIA_KEY_STATUS_OUTPUT_NOT_ALLOWED  3
#define  MEDIA_KEY_STATUS_OUTPUT_DOWNSCALED  4
#define  MEDIA_KEY_STATUS_KEY_STATUS_PENDING  5
#define  MEDIA_KEY_STATUS_KEY_STATUS_MAX  KEY_STATUS_PENDING

typedef enum {
    Temporary,
    PersistentUsageRecord,
    PersistentLicense
} LicenseType;

// IMediaKeySessionCallback defines the callback interface to receive
// events originated from MediaKeySession.
class IMediaKeySessionCallback
{
public:
    virtual ~IMediaKeySessionCallback(void) {}

    // Event fired when a key message is successfully created.
    virtual void OnKeyMessage(
        const uint8_t *f_pbKeyMessage, //__in_bcount(f_cbKeyMessage)
        uint32_t f_cbKeyMessage, //__in
        char *f_pszUrl) = 0; //__in_z_opt

    // Event fired when MediaKeySession has found a usable key.
    virtual  void OnKeyReady(void) = 0;

    // Event fired when MediaKeySession encounters an error.
    virtual void OnKeyError(
        int16_t f_nError,
        CDMi_RESULT f_crSysError,
        const char* errorMessage) = 0;

    //Event fired on key status update
    virtual void OnKeyStatusUpdate(const char* keyMessage, const uint8_t* buffer, const uint8_t length) = 0;
};
 
// IMediaKeySession defines the MediaKeySession interface.
class IMediaKeySession
{
public:
    IMediaKeySession(void) {}
    virtual ~IMediaKeySession(void) {}

    // Kicks off the process of acquiring a key. A MediaKeySession callback is supplied
    // to receive notifications during the process.
    virtual void Run(
        const IMediaKeySessionCallback *f_piMediaKeySessionCallback) = 0; //__in

    // Loads the data stored for the specified session into the cdm object
    virtual CDMi_RESULT Load() = 0;

    // Process a key message response.
    virtual void Update(
        const uint8_t *f_pbKeyMessageResponse, //__in_bcount(f_cbKeyMessageResponse)
        uint32_t f_cbKeyMessageResponse) = 0; //__in

    //Removes all license(s) and key(s) associated with the session
    virtual CDMi_RESULT Remove() = 0;

    // Explicitly release all resources associated with the MediaKeySession.
    virtual CDMi_RESULT Close(void) = 0;

    // Return the session ID of the MediaKeySession. The returned pointer
    // is valid as long as the associated MediaKeySession still exists.
    virtual const char *GetSessionId(void) const = 0;

    // Return the key system of the MediaKeySession.
    virtual const char *GetKeySystem(void) const = 0;

    virtual CDMi_RESULT Decrypt(
        const uint8_t *f_pbSessionKey,
        uint32_t f_cbSessionKey,
        const uint32_t *f_pdwSubSampleMapping,
        uint32_t f_cdwSubSampleMapping,
        const uint8_t *f_pbIV,
        uint32_t f_cbIV,
        const uint8_t *f_pbData,
        uint32_t f_cbData,
        uint32_t *f_pcbOpaqueClearContent,
        uint8_t **f_ppbOpaqueClearContent) = 0;

    virtual CDMi_RESULT ReleaseClearContent(
        const uint8_t *f_pbSessionKey,
        uint32_t f_cbSessionKey,
        const uint32_t  f_cbClearContentOpaque,
        uint8_t  *f_pbClearContentOpaque) = 0;
};

// IMediaKeys defines the MediaKeys interface.
class IMediaKeys
{
public:
    IMediaKeys(void) {}
    virtual ~IMediaKeys(void) {}

   // Create a MediaKeySession using the supplied init data and CDM data.
    virtual CDMi_RESULT CreateMediaKeySession(
        int32_t licenseType,
        const char *f_pwszInitDataType,
        const uint8_t *f_pbInitData,
        uint32_t f_cbInitData,
        const uint8_t *f_pbCDMData,
        uint32_t f_cbCDMData,
        IMediaKeySession **f_ppiMediaKeySession) = 0;

    // Set Server Certificate
    virtual CDMi_RESULT SetServerCertificate(
        const uint8_t *f_pbServerCertificate,
        uint32_t f_cbServerCertificate) = 0;

    // Destroy a MediaKeySession instance.
    virtual CDMi_RESULT DestroyMediaKeySession(
        IMediaKeySession *f_piMediaKeySession) = 0;
};

struct ISystemFactory {
    virtual IMediaKeys* Instance() = 0;
    virtual const char* KeySystem() const = 0;
    virtual const std::vector<std::string>& MimeTypes() const = 0;
};

template <typename IMPLEMENTATION>
class SystemFactoryType : public ISystemFactory {
private:
    SystemFactoryType () = delete;
    SystemFactoryType (const SystemFactoryType<IMPLEMENTATION>&) = delete;
    SystemFactoryType<IMPLEMENTATION>& operator=(const SystemFactoryType<IMPLEMENTATION>&) = delete;

public:
    SystemFactoryType(const std::vector<std::string>& list) : _mimes(list) {
    }
    virtual ~SystemFactoryType() {
    }

public:
    virtual IMediaKeys* Instance() {
        return (&_instance);
    }
    virtual const std::vector<std::string>& MimeTypes() const {
        return (_mimes);
    }
    virtual const char* KeySystem() const {
        return (typeid(IMPLEMENTATION).name());
    }

private:
    const std::vector<std::string> _mimes;
    IMPLEMENTATION _instance;
};

} // namespace CDMi

#ifdef __cplusplus
extern "C" {
#endif

    CDMi::ISystemFactory*  GetSystemFactory();

#ifdef __cplusplus
}
#endif

#endif // CDMI_H_
