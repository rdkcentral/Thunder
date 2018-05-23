#include "../core/core.h"

#include <provision/DRMInfo.h>

#include "IPCProvision.h"

using namespace WPEFramework;

static string GetEndPoint ()
{
        TCHAR* value = ::getenv(_T("PROVISION_PATH"));

	return (value == nullptr ? _T("/tmp/provision") : value);
}

extern "C" {

Core::IPCChannelClientType<Core::Void, false, true> _channel(Core::NodeId(GetEndPoint().c_str()), 2048);
Core::ProxyType<IPC::Provisioning::DrmIdData> _drmId (Core::ProxyType<IPC::Provisioning::DrmIdData>::Create());
Core::ProxyType<IPC::Provisioning::DeviceIdData> _deviceId (Core::ProxyType<IPC::Provisioning::DeviceIdData>::Create());

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
int GetDeviceId(unsigned short MaxIdLength, char Id[])
{
    int result = -1;

    if (_channel.Open (1000) == Core::ERROR_NONE) { // Wait for 1 Second.

        Core::ProxyType<Core::IIPC> message (Core::proxy_cast<Core::IIPC>(_deviceId));
        uint32_t error  = _channel.Invoke (message, Core::infinite);

        result = -error;

        if (error == Core::ERROR_NONE)
        {
            result = _deviceId->Response().Length();

            if (result <= MaxIdLength)
            {
                ::memcpy (Id, _deviceId->Response().Value(), result);
                printf("%s:%d [%s] Received deviceId '%s'.\n", __FILE__, __LINE__, __func__, string(Id, result).c_str());
            }
            else
            {
                printf("%s:%d [%s] Received deviceId '%s' is too long.\n", __FILE__, __LINE__, __func__,
                       _deviceId->Response().Value());
                result = -result;
            }
        }
    }
    else {
        printf("%s:%d [%s] Could not open link. error=%d\n", __FILE__, __LINE__, __func__, result);
    }

    _channel.Close(1000); // give it 1S again to close...

    return (result);
}

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

int GetDRMId(const char label[], const unsigned short MaxIdLength, char Id[])
{
    int result = -1;

    if (_channel.Open (1000) == Core::ERROR_NONE) { // Wait for 1 Second.

        _drmId->Clear();
        _drmId->Parameters() = string(label);
        Core::ProxyType<Core::IIPC> message (Core::proxy_cast<Core::IIPC>(_drmId));

        uint32_t error  = _channel.Invoke (message, Core::infinite);
	result = -error;

        if (error == Core::ERROR_NONE) {

            // This is a huge encrypted blob, convert it to an uencrypted required info
            result = ClearBlob(_drmId->Response().Length(), reinterpret_cast<const char*>(_drmId->Response().Frame()), MaxIdLength, Id);

            if (result > 0) {
                printf("%s:%d [%s] Received Provision Info for '%s' with length [%d].\n", __FILE__, __LINE__, __func__, label, result);
            }
            else {
                printf ("%s:%d [%s] Provisioning for %s too big. Length: %d - %d.\n", __FILE__, __LINE__, __func__, label, -result, MaxIdLength);
            }

            // Security, we want to get ride of the data.
            _drmId->Response().Clear();
        }
        else {
            printf ("%s:%d [%s] Failed to extract %s provisioning. Error code %d.\n", __FILE__, __LINE__, __func__, label, error);
        }
    }
    else {
        printf ("%s:%d [%s] Could not open the provisioning link for %s.\n", __FILE__, __LINE__, __func__, label);
    }

    _channel.Close(1000); // give it 1S again to close...

    return (result);
}

} // extern "C"
