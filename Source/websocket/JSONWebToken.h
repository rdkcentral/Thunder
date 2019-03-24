#pragma once

#include "Module.h"
#include "URL.h"

namespace WPEFramework {
namespace Web {

	class JSONWebToken {
    private:
        JSONWebToken() = delete;
        JSONWebToken(const JSONWebToken&) = delete;
        JSONWebToken& operator= (const JSONWebToken&) = delete;

	public:
		enum mode : uint8_t {
			SHA256
		};
		
		JSONWebToken(const mode type, const uint8_t length, const uint8_t key[]);
        ~JSONWebToken();

	public:
        string Encode(const string& payload) const;
        string Decode(const string& token) const;
        bool ValidSignature(const mode type, const string& token) const;

    private:
        mode _mode;
        string _header; 
		string _key;
    };

} } // namespace WPEFramework::Web