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

#include "../../Module.h"

#include "TEE.h"
#include "TEECommand.h"

#include <bstd.h>
#include <bkni.h>
#include <bkni_multi.h>
#include <blst_list.h>

#include <sage_srai.h>
#include <bsagelib_types.h>


namespace Implementation {

namespace Platform {

    TEECommand::TEECommand()
        : _handle(0)
        , _container(nullptr)
        , _commandId(0)
    {
        _container = SRAI_Container_Allocate();
        ASSERT(_container != nullptr);

        if (_container == nullptr) {
            TRACE_L1(_T("SRAI_Container_Allocate() failed!"));
        }
    }

    TEECommand::TEECommand(const uint32_t commandId, const TEE* tee)
        : TEECommand()
    {
        ASSERT(tee != nullptr);
        ASSERT(commandId != 0);
        ASSERT(commandId <= 32);

        _commandId = commandId;

        if (_container != nullptr) {
            _handle = reinterpret_cast<SRAI_ModuleHandle>(tee->ModuleHandle());
            ASSERT(_handle != 0);

            Basic(0, tee->DRMVersion());
            Basic(1, tee->DRMHandle());
        }
    }

    TEECommand::~TEECommand()
    {
        if (_container != nullptr) {
            for (uint8_t i = 0; i < BSAGE_CONTAINER_MAX_SHARED_BLOCKS; i++) {
                if (_container->blocks[i].data.ptr != nullptr) {
                    SRAI_Memory_Free(_container->blocks[i].data.ptr);
                }
            }

            SRAI_Container_Free(_container);
        }
    }

    void TEECommand::Block(const uint8_t index, const uint8_t buffer[], const uint32_t bufferSize)
    {
        ASSERT(buffer != nullptr);
        ASSERT(index < BSAGE_CONTAINER_MAX_SHARED_BLOCKS);
        ASSERT(_container->blocks[index].data.ptr == nullptr);

        _container->blocks[index].data.ptr = SRAI_Memory_Allocate(bufferSize, SRAI_MemoryType_Shared);
        ASSERT(_container->blocks[index].data.ptr != nullptr);

        if (_container->blocks[index].data.ptr != nullptr) {
            _container->blocks[index].len = bufferSize;

            BKNI_Memcpy(_container->blocks[index].data.ptr, buffer, bufferSize);
        } else {
            TRACE_L1(_T("SRAI_Memory_Allocate() failed!"));
        }
    }

    void TEECommand::Block(const uint8_t index, const uint32_t bufferSize)
    {
        ASSERT(index < BSAGE_CONTAINER_MAX_SHARED_BLOCKS);
        ASSERT(_container->blocks[index].data.ptr == nullptr);

        _container->blocks[index].data.ptr = SRAI_Memory_Allocate(bufferSize, SRAI_MemoryType_Shared);
        ASSERT(_container->blocks[index].data.ptr != nullptr);

        if (_container->blocks[index].data.ptr != nullptr) {
            _container->blocks[index].len = bufferSize;

            BKNI_Memset(_container->blocks[index].data.ptr, 0, bufferSize);
        } else {
            TRACE_L1(_T("SRAI_Memory_Allocate() failed!"));
        }
    }

    uint32_t TEECommand::Block(const uint8_t index, uint8_t basicOutLenIndex, uint8_t buffer[]) const
    {
        ASSERT(buffer != nullptr);
        ASSERT(index < BSAGE_CONTAINER_MAX_SHARED_BLOCKS);
        ASSERT(basicOutLenIndex < BSAGE_CONTAINER_MAX_BASICOUT);
        ASSERT(_container->blocks[index].data.ptr != nullptr);
        ASSERT(static_cast<uint32_t>(_container->basicOut[basicOutLenIndex]) <= _container->blocks[index].len);

        uint32_t result = 0;

        if (static_cast<uint32_t>(_container->basicOut[basicOutLenIndex]) <= _container->blocks[index].len) {
            // Not an error: actual output length is in basicOut instead of block.len
            BKNI_Memcpy(buffer, _container->blocks[index].data.ptr, _container->basicOut[basicOutLenIndex]);
            result = _container->basicOut[basicOutLenIndex];
        }

        return (result);
    }

    uint32_t TEECommand::Execute()
    {
        uint32_t result = 0;

        BERR_Code err = SRAI_Module_ProcessCommand(_handle, _commandId, _container);

        /* BSAGE_ERR_INTERNAL is not really a SRAI error, comms succeeded but the sage-side TA failed. */
        if ((err != BERR_SUCCESS) && (err != BSAGE_ERR_INTERNAL)) {
            TRACE_L1(_T("SRAI_Module_ProcessCommand() for command %i failed [0x%08x]"), _commandId, err);
            result = -1;
        };

        return (result);
    }

    // pre NRD 4.1 command

    TEECommandOld::TEECommandOld(const uint32_t commandId, const TEE* tee)
        : TEECommand(commandId, tee)
    {
        if (Id() != 0) {
            Basic(0, tee->DRMHandle());
        }
    }

} // namespace Platform

} // namespace Implementation



