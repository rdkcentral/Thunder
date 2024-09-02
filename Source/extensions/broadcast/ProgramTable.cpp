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

#include "ProgramTable.h"

namespace Thunder {

namespace Broadcast {

    /* static */ Core::ProxyPoolType<Core::DataStore> ProgramTable::_storeFactory(2);

    /* static */ ProgramTable& ProgramTable::Instance()
    {
        static ProgramTable _instance;
        return (_instance);
    }

    /* virtual */ void
    ProgramTable::Observer::Handle(const MPEG::Section& section)
    {
        bool completedStep = false;

        if (section.IsValid() == true) {
            if (section.TableId() == MPEG::PAT::ID) {
                _table.AddSection(section);

                if (_table.IsValid() == true) {
                    // Iterator over this table and find all program Pids
                    MPEG::PAT patTable(_table);
                    MPEG::PAT::ProgramIterator index(patTable.Programs());
                    while (index.Next() == true) {

                        if (index.ProgramNumber() == 0) {
                            // ProgramNumber == 0 is reserved for the NIT pid
                            _parent._nitPids[_keyId] = index.Pid();
                        } else {
                            _entries.push_back(index.Pid() | (index.ProgramNumber() << 16));
                            TRACE_L1("ProgramNumber: %d on PID: %d", index.ProgramNumber(), index.Pid());
                        }
                    }
                    completedStep = true;
                }
            } else if (section.TableId() == MPEG::PMT::ID) {
                _table.AddSection(section);

                if (_table.IsValid() == true) {

                    completedStep = true;

                    uint32_t key = static_cast<uint16_t>(_entries.front() & 0xFFFF) | (_table.Extension() << 16);

                    ScanMap::iterator index(_entries.begin());

                    while (index != _entries.end()) {
                        if (*index == key) {
                            index = _entries.erase(index);
                        } else {
                            completedStep = completedStep && ((*index & 0xFFFF) != (key & 0xFFFF));
                            index++;
                        }
                    }

                    if (_parent.AddProgram(_keyId, _table) == true) {
                        // do not forget to get a new storage space, the previous one is now
                        // with the AddProgram !!!
                        _table.Storage(_storeFactory.Element());

                        TRACE_L1("PMT for %d loaded. PID load for PMTs completed: %s", _table.Extension(), completedStep ? _T("true") : _T("false"));
                    }
                }
            }

            if (completedStep == true) {
                _table.Clear();
                if (_entries.size() > 0) {
                    _callback->ChangePid(static_cast<uint16_t>(_entries.front() & 0xFFFF),
                        this);
                } else {
                    _callback->ChangePid(0xFFFF, this);
                }
            }
        }
    }

} // namespace Broadcast
} // namespace Thunder
