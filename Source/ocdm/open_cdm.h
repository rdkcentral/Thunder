/*
 * Copyright 2016-2017 TATA ELXSI
 * Copyright 2016-2017 Metrological
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

#ifndef __OCDM_WRAPPER_H_
#define __OCDM_WRAPPER_H_

// WPEWebkit implementation is using the following header file to integrate their solution with 
// DRM systems (PlayReady/WideVine/ClearKey).
// The implementation behind this class/interface exists in two flavors.
// 1) Fraunhofers adapted reference implementation, based on SUNRPC
// 2) Metrologicals WPEFrameworks, based on the WPEFramework RPC mechanism.
//
// The second option exists because during testing the reference/adapted implementation of Frauenhofer
// it was observed:
// - Older implementations of ucLibc had different dynamic characteristics that caused deadlocks
// - Message exchange over the SUNRPC mechanism is using continous heap memory allocations/deallocactions, 
//   leading to a higher risk of memory fragmentation.
// - SUNRPC only works on UDP/TCP, given the nature and size of the messages, UDP was not an option so TCP
//   is used. There is no domain socket implementation for the SUNRPC mechanism. Domain sockets transfer 
//   data, as an average, twice as fast as TCP sockets.
// - SUNRPC requires an external process (bind) to do program number lookup to TCP port connecting. Currently
//   the Frauenhofer OpenCDMi reference implementation is the only implementation requiring this service on
//   most deplyments with the WPEWebkit.
// - Common Vulnerabilities and Exposures (CVE's) have been reported with the SUNRPC that have not been resolved 
//   on most platforms where the solution is deployed.
// - The WPEFramework RPC mechanism allows for a configurable in or out of process deplyment of the OpenCDMi 
//   deployment without rebuilding.  
//
// So due to performance and security exploits it was decided to offer a second implementation of the OpenCDMi
// specification that did notrequire to change the WPEWebKit

#include <string.h>
#include <stdint.h>

#ifdef __cplusplus

#include <string>
#include <list>

namespace media {

class AccessorOCDM;
class OpenCdmSession;

class OpenCdm {
private:
  OpenCdm& operator= (const OpenCdm&) = delete;

public:
  enum LicenseType {
      Temporary = 0,
      PersistentUsageRecord,
      PersistentLicense
  };
  enum KeyStatus {
      Usable = 0,
      Expired,
      Released,
      OutputRestricted,
      OutputDownscaled,
      StatusPending,
      InternalError
  };

public:
  OpenCdm();
  OpenCdm(const OpenCdm& copy);
  explicit OpenCdm(const std::string& sessionId);
  ~OpenCdm();

public:
  bool GetSession(const uint8_t keyId[], const uint8_t length, const uint32_t waitTime);

  // ---------------------------------------------------------------------------------------------
  // INSTANTIATION OPERATIONS:
  // ---------------------------------------------------------------------------------------------
  // Before instantiating the ROOT DRM OBJECT, Check if it is capable of decrypting the requested
  // asset.
  bool IsTypeSupported(const std::string& keySystem, const std::string& mimeType) const;

  // The next call is the startng point of creating a decryption context. It select the DRM system 
  // to be used within this OpenCDM object.
  void SelectKeySystem(const std::string& keySystem);

  // ---------------------------------------------------------------------------------------------
  // ROOT DRM OBJECT OPERATIONS:
  // ---------------------------------------------------------------------------------------------
  // If required, ServerCertificates can be added to this OpenCdm object (DRM Context).
  int SetServerCertificate(const uint8_t*, const uint32_t);

  // Now for every particular stream a session needs to be created. Create a session for all
  // encrypted streams that require decryption. (This allows for MultiKey decryption)
  std::string CreateSession(const std::string&, const uint8_t* addData, const uint16_t addDataLength, const LicenseType license);

  // ---------------------------------------------------------------------------------------------
  // ROOT DRM -> SESSION OBJECT OPERATIONS:
  // ---------------------------------------------------------------------------------------------
  void GetKeyMessage(std::string&, uint8_t*, uint16_t&);
  KeyStatus Update(const uint8_t*, const uint16_t, std::string&);
  int Load(std::string&);
  int Remove(std::string&);
  int Close();
  KeyStatus Status() const;

  uint32_t Decrypt(uint8_t*, const uint32_t, const uint8_t*, const uint16_t);
  uint32_t Decrypt(uint8_t*, const uint32_t, const uint8_t*, const uint16_t, const uint8_t, const uint8_t[], const uint32_t waitTime = 6000);

  inline const std::string& KeySystem() const {
    return (_keySystem);
  }


private:
  AccessorOCDM* _implementation;
  OpenCdmSession* _session;
  std::string _keySystem;
};

} // namespace media

#else

void* acquire_session(const uint8_t* keyId, const uint8_t keyLength, const uint32_t waitTime);
void release_session(void* session);
uint32_t decrypt(void* session, uint8_t*, const uint32_t, const uint8_t*, const uint16_t);

#endif

#endif // __OCDM_WRAPPER_H_
