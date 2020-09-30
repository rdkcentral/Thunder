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

#ifndef __ACCESS_PROVISION_H__
#define __ACCESS_PROVISION_H__

extern "C" {

/*
 * GetDeviceId - function to obtain the unique Device ID 
 *
 * Parameters
 *  MaxIdLength - holds the maximal char length of the Id parameter
 *  Id          - Buffer holds the unique Device ID upon succession, empty in case of failure
 *
 * Return value
 *  < 0 - failure, absolute value returned is the length required to store the Id
 *  > 0 - success, char length of the returned Id
 *
 * Post-condition; return value 0 should not occur
 *
 */
int GetDeviceId(unsigned short MaxIdLength, char Id[]);

/*
 * GetDRMId - function to obtain the DRM Id in the clear
 *
 * Parameters
 *  MaxIdLength - holds the maximal char length of the Id parameter
 *  Id          - Buffer holds the DRM ID upon succession, empty in case of failure
 *  label       - Name of the DRMId requested (if not supplied, label == playready)
 *
 * Return value
 *  < 0 - failure, absolute value returned is the length required to store the Id
 *  > 0 - success, char length of the returned Id
 *
 * Post-condition; return value 0 should not occur
 *
 */

int GetDRMId(const char label[], const unsigned short MaxIdLength, char Id[]);
}

#endif // __ACCESS_PROVISION_H__
