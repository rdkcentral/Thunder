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

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef _MSVC_LANG
#undef EXTERNAL
#ifdef DISPLAYINFO_EXPORTS
#define EXTERNAL __declspec(dllexport)
#else
#define EXTERNAL __declspec(dllimport)
#pragma comment(lib, "displayinfo.lib")
#endif
#else
#define EXTERNAL __attribute__ ((visibility ("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif
struct displayinfo_type;

typedef enum displayinfo_hdr_type {
    DISPLAYINFO_HDR_OFF,
    DISPLAYINFO_HDR_10,
    DISPLAYINFO_HDR_10PLUS,
    DISPLAYINFO_HDR_DOLBYVISION,
    DISPLAYINFO_HDR_TECHNICOLOR,
    DISPLAYINFO_HDR_UNKNOWN
} displayinfo_hdr_t;

typedef enum displayinfo_hdcp_protection_type {
    DISPLAYINFO_HDCP_UNENCRYPTED,
    DISPLAYINFO_HDCP_1X,
    DISPLAYINFO_HDCP_2X,
    DISPLAYINFO_HDCP_UNKNOWN
} displayinfo_hdcp_protection_t;

typedef enum displayinfo_error_type {
    ERROR_NONE = 0,
    ERROR_UNKNOWN = 1,
    ERROR_INVALID_INSTANCE = 2,
} displayinfo_error_t;

/**
* \brief Will be called if there are changes regaring the display output, you need to query 
*        yourself what exacally is changed
*
* \param session The session the notification applies to.
* \param userData Pointer passed along when \ref displayinfo_register was issued.
*/
typedef void (*displayinfo_updated_cb)(struct displayinfo_type* session, void* userdata);

/**
 * \brief Get a implementation instance name to use for displayinfo_instance based on index\
 *        If \ref length is 0  and \ref buffer is NULL, only a check is assumed.
 * 
 * \param index The index of a DisplayInfo implementation
 * \param length Length \ref buffer, can be 0 for a check only
 * \param buffer Pointer to a buffer, can be NULL if \ref length is 0 for a check only
 * 
 * \return true if a instance was found for the index, false otherwise.
 **/
EXTERNAL bool displayinfo_enumerate(const uint8_t index, const uint8_t length, char* buffer);

/**
 * \brief Get a \ref displayinfo_type instance that matches the a DisplayInfo implementation.
 *
 * \param displayName Name a the implementation that holds the.
 * 
 * \return \ref displayinfo_type instance, NULL on error.
 **/
EXTERNAL struct displayinfo_type* displayinfo_instance(const char displayName[]);

/**
 * \brief Release the \ref displayinfo_type instance.
 * 
 * \param instance Instance of \ref displayinfo_type.
 * 
 **/
EXTERNAL void displayinfo_release(struct displayinfo_type* instance);

/**
 * \brief Register for updates of the display output.
 * 
 * \param instance Instance of \ref displayinfo_type.
 * \param callback Callback that needs to be called if a chaged is deteced.
 * \param userdata The user data to be passed back to the \ref displayinfo_updated_cb callback.
 * 
 **/
EXTERNAL void displayinfo_register(struct displayinfo_type* instance, displayinfo_updated_cb callback, void* userdata);

/**
 * \brief Unregister for updates of the display output.
 * 
 * \param instance Instance of \ref displayinfo_type.
 * \param callback Callback that was used to \ref displayinfo_registet
 * 
 **/
EXTERNAL void displayinfo_unregister(struct displayinfo_type* instance, displayinfo_updated_cb callback);

/**
 * \brief Returns name of display output.
 *
 * \param instance Instance of \ref displayinfo_type.
 * \param buffer Buffer that will contain instance name.
 * \param length Size of \ref buffer.
 *
 **/
EXTERNAL void displayinfo_name(struct displayinfo_type* instance, char buffer[], const uint8_t length);

/**
 * \brief Checks if a audio passthrough is enabled.
 * 
 * \param instance Instance of \ref displayinfo_type.
 * 
 * \return true if audio passthrough is enabled, false otherwise.
 **/
EXTERNAL bool displayinfo_is_audio_passthrough(struct displayinfo_type* instance);

/**
 * \brief Checks if a display is connected to the display output.
 * 
 * \param instance Instance of \ref displayinfo_type.
 * 
 * \return true a dispplay is connected, false otherwise.
 * 
 **/
EXTERNAL bool displayinfo_connected(struct displayinfo_type* instance);

/**
 * \brief Get the heigth of the connected display  
 * 
 * \param instance Instance of \ref displayinfo_type.
 * 
 * \return The current heigth in pixels, 0 on error or invalid connection
 * 
 **/
EXTERNAL uint32_t displayinfo_width(struct displayinfo_type* instance);

/**
 * \brief Get the width of the connected display  
 * 
 * \param instance Instance of \ref displayinfo_type.
 * 
 * \return The current width in pixels, 0 on error or invalid connection
 *
 **/
EXTERNAL uint32_t displayinfo_height(struct displayinfo_type* instance);

/**
 * \brief Get the vertical refresh rate ("v-sync")  
 * 
 * \param instance Instance of \ref displayinfo_type.
 * 
 * \return The vertical refresh rate
 *
 **/
EXTERNAL uint32_t displayinfo_vertical_frequency(struct displayinfo_type* instance);

/**
 * \brief Get the current HDR system of the connected display  
 * 
 * \param instance Instance of \ref displayinfo_type.
 * 
 * \return The current enabled HDR system, DISPLAYINFO_HDR_UNKNOWN on error or invalid connection
 * 
 **/
EXTERNAL displayinfo_hdr_t displayinfo_hdr(struct displayinfo_type* instance);

/**
 * \brief Get the current HDCP protection level of the connected display  
 * 
 * \param instance Instance of \ref displayinfo_type.
 * 
 * \return The current enabled HDCP level, DISPLAYINFO_HDCP_UNKNOWN on error or invalid connection
 * 
 **/
EXTERNAL displayinfo_hdcp_protection_t displayinfo_hdcp_protection(struct displayinfo_type* instance);

/**
 * \brief Get the total available GPU RAM space in bytes.
 * 
 * \param instance Instance of \ref displayinfo_type.
 * \return The total amount of GPU RAM available on the device.
 */
EXTERNAL uint64_t displayinfo_total_gpu_ram(struct displayinfo_type* instance);

/**
 * \brief Get the currently available GPU RAM in bytes.
 * 
 * \param instance Instance of \ref displayinfo_type.
 * \return The current amount of available GPU RAM memory.
 */
EXTERNAL uint64_t displayinfo_free_gpu_ram(struct displayinfo_type* instance);

/**
 * \brief Returns EDID data of a connected display.
 *
 * \param instance Instance of \ref displayinfo_type.
 * \param buffer Buffer that will contain the data.
 * \param length Size of \ref buffer. On success it'll be set to the length of the actuall data in \ref buffer.
 *
 **/
EXTERNAL uint32_t displayinfo_edid(struct displayinfo_type* instance, uint8_t buffer[], uint16_t* length);

/**
 * \brief Get the heigth of the connected display in centimaters
 *
 * \param instance Instance of \ref displayinfo_type.
 *
 * \return The current heigth in centimeters, 0 on error or invalid connection
 *
 **/
EXTERNAL uint8_t displayinfo_width_in_centimeters(struct displayinfo_type* instance);

/**
 * \brief Get the width of the connected display in centimeters
 *
 * \param instance Instance of \ref displayinfo_type.
 *
 * \return The current width in centimeters, 0 on error or invalid connection
 *
 **/
EXTERNAL uint8_t displayinfo_height_in_centimeters(struct displayinfo_type* instance);

#ifdef __cplusplus
} // extern "C"
#endif
