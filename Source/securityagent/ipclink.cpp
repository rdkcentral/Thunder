#include "securityagent.h"
#include "IPCSecurityToken.h"

using namespace WPEFramework;

static string GetEndPoint()
{
    TCHAR* value = ::getenv(_T("SECURITYAGENT_PATH"));

    return (value == nullptr ? _T("/tmp/securityagent") : value);
}

extern "C" {

Core::ProxyType<IPC::SecurityAgent::TokenData> _tokenId(Core::ProxyType<IPC::SecurityAgent::TokenData>::Create());

/*
 * GetToken - function to obtain a token from the SecurityAgent
 *
 * Parameters
 *  MaxIdLength - holds the maximal uint8_t length of the token
 *  Id          - Buffer holds the data to tokenize on its way in, and returns in the same buffer the token.
 *
 * Return value
 *  < 0 - failure, absolute value returned is the length required to store the token
 *  > 0 - success, char length of the returned token
 *
 * Post-condition; return value 0 should not occur
 *
 */
int GetToken(unsigned short maxLength, unsigned short inLength, unsigned char buffer[])
{
    Core::IPCChannelClientType<Core::Void, false, true> channel(Core::NodeId(GetEndPoint().c_str()), 2048);
    int result = -1;

    if (channel.Open(1000) == Core::ERROR_NONE) { // Wait for 1 Second.

        // Prepare the data we have to send....
        _tokenId->Clear();
        _tokenId->Parameters().Set(inLength, buffer);

        Core::ProxyType<Core::IIPC> message(Core::proxy_cast<Core::IIPC>(_tokenId));
        uint32_t error = channel.Invoke(message, IPC::CommunicationTimeOut);

        if (error == Core::ERROR_NONE) {
            result = _tokenId->Response().Length();

            if (result <= maxLength) {
                ::memcpy(buffer, _tokenId->Response().Value(), result);
            } else {
                printf("%s:%d [%s] Received token is too long [%d].\n", __FILE__, __LINE__, __func__, result);
                result = -result;
            }
        }
        else {
            result = error;
            result = -result;
        }
    } else {
        printf("%s:%d [%s] Could not open link. error=%d\n", __FILE__, __LINE__, __func__, result);
    }

    channel.Close(1000); // give it 1S again to close...

    return (result);
}

}
