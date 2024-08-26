/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2021 Metrological
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
#include "Module.h"

namespace Thunder {
namespace Core {

    template <TCHAR SEPARATOR, bool REMOVE_WHITESPACES>
    class TokenizedStringList {
    public:
        TokenizedStringList() = default;
        ~TokenizedStringList() = default;
        TokenizedStringList(TokenizedStringList&&) = default;
        TokenizedStringList(const TokenizedStringList&) = default;
        TokenizedStringList& operator=(TokenizedStringList&&) = default;
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
