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

#pragma once

#include "Module.h"

namespace Thunder {
namespace PluginHost {

    class CallstackData : public Core::JSON::Container {
    public:
        CallstackData()
            : Core::JSON::Container()
            , Address()
            , Function()
            , Module()
            , Line()
        {
            Add(_T("address"), &Address);
            Add(_T("function"), &Function);
            Add(_T("module"), &Module);
            Add(_T("line"), &Line);
        }
        CallstackData(const Core::callstack_info& source)
            : Core::JSON::Container()
            , Address()
            , Function()
            , Module()
            , Line()
        {
            Add(_T("address"), &Address);
            Add(_T("function"), &Function);
            Add(_T("module"), &Module);
            Add(_T("line"), &Line);

            Address = reinterpret_cast<Core::instance_id>(source.address);
            Function = source.function;
            if (source.module.empty() == false) {
                Module = source.module;
            }
            if (source.line != static_cast<uint32_t>(~0)) {
                Line = source.line;
            }
        }
        CallstackData(const CallstackData& copy)
            : Core::JSON::Container()
            , Address(copy.Address)
            , Function(copy.Function)
            , Module(copy.Module)
            , Line(copy.Line)
        {
            Add(_T("address"), &Address);
            Add(_T("function"), &Function);
            Add(_T("module"), &Module);
            Add(_T("line"), &Line);
        }
        CallstackData(CallstackData&& move)
            : Core::JSON::Container()
            , Address(std::move(move.Address))
            , Function(std::move(move.Function))
            , Module(std::move(move.Module))
            , Line(std::move(move.Line))
        {
            Add(_T("address"), &Address);
            Add(_T("function"), &Function);
            Add(_T("module"), &Module);
            Add(_T("line"), &Line);
        }

        ~CallstackData() override = default;

        CallstackData& operator=(const CallstackData& RHS)
        {
            Address = RHS.Address;
            Function = RHS.Function;
            Module = RHS.Module;
            Line = RHS.Line;

            return (*this);
        }

        CallstackData& operator=(CallstackData&& move)
        {
            if (this != &move) {
                Address = std::move(move.Address);
                Function = std::move(move.Function);
                Module = std::move(move.Module);
                Line = std::move(move.Line);
            }
            return (*this);
        }

    public:
        Core::JSON::Pointer   Address;
        Core::JSON::String    Function;
        Core::JSON::String    Module;
        Core::JSON::DecUInt32 Line;
    };

    
    class PostMortemData : public Core::JSON::Container {
    public:
        class Callstack : public Core::JSON::Container {
        public:
            Callstack& operator=(const Callstack& copy) = delete;

            Callstack() 
                : Core::JSON::Container()
                , Id(0)
                , Data() {
                Add(_T("id"), &Id);
                Add(_T("stack"), &Data);
            }
            Callstack(Callstack&& move) 
                : Core::JSON::Container()
                , Id(move.Id)
                , Data(move.Data) {
                Add(_T("id"), &Id);
                Add(_T("stack"), &Data);
            }
            Callstack(const Callstack& copy) 
                : Core::JSON::Container()
                , Id(copy.Id)
                , Data(copy.Data) {
                Add(_T("id"), &Id);
                Add(_T("stack"), &Data);
            }
            ~Callstack() override = default;

        public:
            Core::JSON::Pointer                   Id;
            Core::JSON::ArrayType<CallstackData>  Data;
        };

    public:
        PostMortemData(PostMortemData&&) = delete;
        PostMortemData(const PostMortemData&) = delete;
        PostMortemData& operator=(PostMortemData&&) = delete;
        PostMortemData& operator=(const PostMortemData&) = delete;

        PostMortemData()
            : Core::JSON::Container()
            , WorkerPool()
            , Callstacks() {
            Add(_T("workerpool"), &WorkerPool);
            Add(_T("stacks"), &Callstacks);
        }
        ~PostMortemData() override = default;

    public:
        Metadata::Server WorkerPool;
        Core::JSON::ArrayType<Callstack> Callstacks;
    };

} }


 

