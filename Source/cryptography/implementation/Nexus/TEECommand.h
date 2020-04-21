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

#include "TEE.h"

#include <nexus_config.h>
#include <sage_srai.h>
#include <bsagelib_types.h>


namespace Implementation {

namespace Platform {

    class TEECommand {
    public:
        TEECommand(const TEECommand&) = delete;
        TEECommand& operator=(const TEECommand) = delete;

        TEECommand();
        TEECommand(const uint32_t commandId, const TEE* tee);
        virtual ~TEECommand();

    public:
        BSAGElib_InOutContainer* operator*()
        {
            return _container;
        }

        const BSAGElib_InOutContainer* operator*() const
        {
            return _container;
        }

        uint32_t Id() const
        {
            return _commandId;
        }

        uint32_t Result() const
        {
            return (Basic(0));
        }

        void Basic(const uint8_t index, const uint32_t value)
        {
            ASSERT(index < BSAGE_CONTAINER_MAX_BASICIN);
            _container->basicIn[index] = value;
        }

        uint32_t Basic(const uint8_t index) const
        {
            ASSERT(index < BSAGE_CONTAINER_MAX_BASICOUT);
            return (_container->basicOut[index]);
        }

        void Block(const uint8_t index, const uint8_t buffer[], const uint32_t bufferSize);
        void Block(const uint8_t index, const uint32_t bufferSize);
        uint32_t Block(const uint8_t index, uint8_t basicOutLenIndex, uint8_t buffer[]) const;

        uint32_t Execute();

    private:
        SRAI_ModuleHandle _handle;
        BSAGElib_InOutContainer* _container;
        uint32_t _commandId;
        uint8_t _maxBlock;
    }; // class TEECommand

    class TEECommandOld : public TEECommand {
    public:
        TEECommandOld(const TEECommandOld&) = delete;
        TEECommandOld& operator=(const TEECommandOld) = delete;

        TEECommandOld() = delete;
        TEECommandOld(const uint32_t commandId, const TEE* tee);
        ~TEECommandOld() = default;
    }; // class TEECommandOld

} // namespace Platform

} // namespace Implementation

