/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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

#include "Hash.h"

/* COTR */
Implementation::HashTypeNetflix::HashTypeNetflix(const Implementation::VaultNetflix* vault, const uint32_t secretId)
    :_failure(false)
{
    ASSERT(vault != nullptr);
    ASSERT(secretId != 0);
    _vaultNetflix = vault;

    uint32_t retVal = vault->FindKey(secretId, &_secretKey);
    _netflixHandle = vault->getNetflixHandle();
    if (retVal != 0 || _netflixHandle == nullptr) {
        TRACE_L1(_T("SEC: hash for netflix cant be instantiated\n"));
        _failure = true;
    }
}

/*********************************************************************
 * @function Ingest
 *
 * @brief    Ingest the data for which hmac needs to be found 
 *
 * @param[in] length - size of data
 * @param[in] data - data buffer
 *
 * @return status success/failure
 *
 *********************************************************************/
uint32_t Implementation::HashTypeNetflix::Ingest(const uint32_t length, const uint8_t* data)
{
    ASSERT(data != nullptr);
    if (false == _failure) {
        _buffer.append(reinterpret_cast<const char*>(data), length);
    }
    else {
        TRACE_L1(_T("SEC :hash Ingest Failure \n"));
    }
    return (_failure ? 0 : length);
}

/*********************************************************************
 * @function Calculate
 *
 * @brief    Caluculate the hmac value using sec netflix calls
 *
 * @param[in] maxLength - maximum length of data
 * @param[in] data - calculated hmac 
 *
 * @return size of calculated hmac 
 *
 *********************************************************************/
uint8_t Implementation::HashTypeNetflix::Calculate(const uint8_t maxLength, uint8_t* data)
{
    uint8_t result = 0;
    if (true == _failure) {
        TRACE_L1(_T("SEC : Hash calculation failure"));
    }
    else {
        if (maxLength == 0) {
            TRACE_L1(_T("SEC :outbuffer size invalid \n"));
        }
        else {
            Sec_Result  result_sec = SEC_RESULT_FAILURE;
            uint32_t bytesWritten;
            std::string data_out;
            uint8_t* data_in = reinterpret_cast<uint8_t*>(&_buffer[0]);
            uint8_t sig_data[maxLength];
            result_sec = SecNetflix_Hmac(_netflixHandle, _secretKey, data_in, _buffer.size(),sig_data, maxLength, &bytesWritten);
            if (result_sec == SEC_RESULT_SUCCESS) {
                memcpy(data, sig_data, bytesWritten);
                result = bytesWritten;
                TRACE_L2(_T("SEC:HMAC signature calculated  bytes written is %d and maxLength is %d  \n"), bytesWritten, maxLength);
            }
            else { 
                TRACE_L1(_T("SEC :Hmac calculation by Secnetflix_Hmac failed ,retVal =%d \n"),result_sec);
            }
        }
    }
    return (result);
}

