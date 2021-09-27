#pragma once
#include "Module.h"

namespace WPEFramework {
namespace Core {

    template <TCHAR SEPARATOR, bool REMOVE_WHITESPACES>
    class TokenizedStringList {
    public:
        TokenizedStringList() = default;
        ~TokenizedStringList() = default;
        TokenizedStringList(const TokenizedStringList&) = default;
        TokenizedStringList& operator=(const TokenizedStringList&) = default;

        explicit TokenizedStringList(const string& buffer)
        {
            Tokenize(buffer);
        }

        void Tokenize(const string& buffer)
        {

            if (!buffer.empty()) {
                //remove spaces and tokenize by SEPARATOR
                if (REMOVE_WHITESPACES) {
                    string result = buffer;
                    result.erase(std::remove_if(result.begin(), result.end(), ::isspace), result.end());
                    std::istringstream iss(result);
                }
                std::istringstream iss(buffer);

                string substring;
                while (iss.good()) {
                    getline(iss, substring, SEPARATOR);
                    _parts.push_back(substring);
                }
            }
        }

        void Clear()
        {
            _parts.clear();
        }

        const string& First() const
        {
            return _parts.front();
        }

        string All() const
        {
            string result;

            if (!_parts.empty()) {

                for (const auto& part : _parts) {
                    result += part;
                    result += SEPARATOR;
                }

                result.erase(result.rfind(SEPARATOR));
            }

            return result;
        }

        bool Empty() const
        {
            return _parts.empty();
        }

        std::size_t Size() const
        {
            return _parts.size();
        }

        std::vector<string>::iterator begin()
        {
            return _parts.begin();
        }

        std::vector<string>::iterator end()
        {
            return _parts.end();
        }

        std::vector<string>::const_iterator begin() const
        {
            return _parts.cbegin();
        }

        std::vector<string>::const_iterator end() const
        {
            return _parts.cend();
        }

        std::string& operator[](int index)
        {
            return _parts[index];
        }

        const std::string& operator[](int index) const
        {
            return _parts[index];
        }

    private:
        std::vector<string> _parts;
    };
}
}