#include "JSONWebToken.h"

namespace WPEFramework {

ENUM_CONVERSION_BEGIN(Web::JSONWebToken::mode)

    { Web::JSONWebToken::mode::SHA256, _TXT("HS256") },

    ENUM_CONVERSION_END(Web::JSONWebToken::mode)

        namespace Web
{

    JSONWebToken::JSONWebToken(const mode type, const uint8_t length, const uint8_t key[])
        : _mode(type)
        , _key(string(reinterpret_cast<const char*>(key), length))
    {
        Core::EnumerateType<mode> modeData(type);
        string sourceBuffer(_T("{\"alg\":\"") + string(modeData.Data()) + _T("\",\"typ\":\"JWT\"}"));
        uint16_t sourceLength = static_cast<uint16_t>(sourceBuffer.length());
        uint16_t destinationLength = (((sourceLength * 8) / 6) + 4) * sizeof(TCHAR);
        TCHAR* destinationBuffer = reinterpret_cast<TCHAR*>(ALLOCA(destinationLength * sizeof(TCHAR)));
        uint16_t convertedLength = Core::URL::Base64Encode(
            reinterpret_cast<const uint8_t*>(sourceBuffer.c_str()),
            sourceLength * sizeof(TCHAR),
            destinationBuffer,
            destinationLength / sizeof(TCHAR));
        ASSERT(convertedLength < destinationLength);
        _header = string(destinationBuffer, convertedLength);
    }
    JSONWebToken::~JSONWebToken()
    {
    }
    string JSONWebToken::Encode(const string& payload) const
    {
        uint16_t sourceLength = static_cast<uint16_t>(payload.length());
        uint16_t destinationLength = (((sourceLength * 8) / 6) + 4) * sizeof(TCHAR);
        TCHAR* destinationBuffer = reinterpret_cast<TCHAR*>(ALLOCA(destinationLength * sizeof(TCHAR)));
        uint16_t convertedLength = Core::URL::Base64Encode(
            reinterpret_cast<const uint8_t*>(payload.c_str()),
            sourceLength * sizeof(TCHAR),
            destinationBuffer,
            destinationLength / sizeof(TCHAR));

		string result = (_header + '.' + string(destinationBuffer, convertedLength));

		if (_mode == JSONWebToken::SHA256) {
            TCHAR signature[((Crypto::SHA256HMAC::Length * 8) / 6) + 4];
            Crypto::SHA256HMAC hash(_key);
            hash.Input(reinterpret_cast<const uint8_t*>(result.c_str()), static_cast<uint16_t>(result.length() * sizeof(TCHAR)));
            convertedLength = Core::URL::Base64Encode(hash.Result(), hash.Length, signature, sizeof(signature), false);
            result += '.' + signature;
		}

        return (result);
    }
    string JSONWebToken::Decode(const string& token) const
    {
        string result;

		// Check if the Hash is correct..
        size_t pos = token.find_last_of('.', 0);

		if (pos != string::npos) {
			// Extract the signature and convert it to a binary string.
            uint8_t signature[Crypto::SHA256HMAC::Length];
            Core::URL::Base64Decode(token.substr(pos + 1).c_str(), static_cast<uint16_t>(token.length() - pos - 1), signature, sizeof(signature), nullptr);

			// Now calculate what we think it should be..
            Crypto::SHA256HMAC hash(_key);
            string payload = token.substr(0, pos);

            hash.Input(reinterpret_cast<const uint8_t*>(payload.c_str()), static_cast<uint16_t>(payload.length() * sizeof(TCHAR)));
            if (::memcmp(hash.Result(), signature, sizeof(signature)) == 0) {

                // Check if the Hash is correct..
                pos = payload.find_last_of('.', 0);

				if (pos != string::npos) {
                    uint16_t length = static_cast<uint16_t>(payload.length() - pos);
                    TCHAR* output = reinterpret_cast<TCHAR*>(ALLOCA(length * sizeof(TCHAR)));
                    // Oke, this is a valid frame, let extract the payload..
                    length = Core::URL::Base64Decode(payload.substr(pos + 1).c_str(), length - 1, reinterpret_cast<uint8_t*>(output), length * sizeof(TCHAR), nullptr);
                    result = string(output, length); 
				}
			}
        }

        return (result);
    }
}
} // namespace WPEFramework::Web