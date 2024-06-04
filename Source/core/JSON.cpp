 /*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
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

#include "JSON.h"
#include <iomanip>
#include <sstream>

namespace Thunder {
namespace Core {
    namespace JSON {

        string ErrorDisplayMessage(const Error& err)
        {
            string msg;
            msg += err.Message();
            string context = err.Context();
            if (!context.empty())
                msg += "\nAt character " + Core::NumberType<size_t>(err.Position()).Text() + ": " + context;

            return msg;
        }


        /* static */ char IElement::NullTag[5] = { 'n', 'u', 'l', 'l', '\0' };
        /* static */ char IElement::TrueTag[5] = { 't', 'r', 'u', 'e', '\0' };
        /* static */ char IElement::FalseTag[6] = { 'f', 'a', 'l', 's', 'e', '\0' };

        string Variant::GetDebugString(const TCHAR name[], int indent, int arrayIndex) const
        {
            std::stringstream ss;
            if (indent > 0)
                ss << std::setw(indent * 4) << " " << std::setw(1);
            if (arrayIndex >= 0)
                ss << "[" << arrayIndex << "] ";
            if (name)
                ss << "name=" << name << " ";
            if (_type == type::EMPTY)
                ss << "type=Empty value=" << String() << std::endl;
            else if (_type == type::BOOLEAN)
                ss << "type=Boolean value=" << (Boolean() ? "true" : "false") << std::endl;
            else if (_type == type::NUMBER)
                ss << "type=Number value=" << Number() << std::endl;
            else if (_type == type::STRING)
                ss << "type=String value=" << String() << std::endl;
            else if (_type == type::ARRAY) {
                ss << "type=Array value=[" << std::endl;
                for (int i = 0; i < Array().Length(); ++i)
                    ss << Array()[i].GetDebugString(nullptr, indent + 1, i);
                ss << std::setw(indent * 4) << ']' << std::setw(1) << std::endl;
            } else if (_type == type::OBJECT) {
                ss << "type=Object value={" << std::endl;
                ss << Object().GetDebugString(indent + 1);
                ss << std::setw(indent * 4) << '}' << std::setw(1) << std::endl;
            }
            return ss.str();
        }
        string VariantContainer::GetDebugString(int indent) const
        {
            std::stringstream ss;
            Iterator iterator = Variants();
            while (iterator.Next())
                ss << iterator.Current().GetDebugString(iterator.Label(), indent);
            return ss.str();
        }
    }
}

} //namespace Core::JSON
