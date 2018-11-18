#include "ProgramTable.h"

namespace WPEFramework {

    ENUM_CONVERSION_BEGIN(Broadcast::ITuner::DTVStandard)
    { Broadcast::ITuner::DVB,     _TXT("DVB")       },
    { Broadcast::ITuner::ATSC,    _TXT("ATSC")      },
    ENUM_CONVERSION_END(Broadcast::ITuner::DTVStandard)

    ENUM_CONVERSION_BEGIN(Broadcast::ITuner::Annex)
    { Broadcast::ITuner::A,     _TXT("A")       },
    { Broadcast::ITuner::B,     _TXT("B")       },
    { Broadcast::ITuner::C,     _TXT("C")       },
    ENUM_CONVERSION_END(Broadcast::ITuner::Annex)

    ENUM_CONVERSION_BEGIN(Broadcast::ITuner::SpectralInversion)
    { Broadcast::ITuner::Auto,     _TXT("Auto")     },
    { Broadcast::ITuner::Normal,   _TXT("Normal")   },
    { Broadcast::ITuner::Inverted, _TXT("Inverted") },
    ENUM_CONVERSION_END(Broadcast::ITuner::SpectralInversion)

    ENUM_CONVERSION_BEGIN(Broadcast::ITuner::Modulation)
    { Broadcast::ITuner::QAM16,    _TXT("QAM16")     },
    { Broadcast::ITuner::QAM32,    _TXT("QAM32")     },
    { Broadcast::ITuner::QAM64,    _TXT("QAM64")     },
    { Broadcast::ITuner::QAM128,   _TXT("QAM128")    },
    { Broadcast::ITuner::QAM256,   _TXT("QAM256")    },
    { Broadcast::ITuner::QAM512,   _TXT("QAM512")    },
    { Broadcast::ITuner::QAM1024,  _TXT("QAM1024")   },
    { Broadcast::ITuner::QAM2048,  _TXT("QAM2048")   },
    { Broadcast::ITuner::QAM4096,  _TXT("QAM4096")   },
    ENUM_CONVERSION_END(Broadcast::ITuner::Modulation)

namespace Broadcast {

    /* static */ ProgramTable ProgramTable::_instance;
    /* static */ Core::ProxyPoolType<Core::DataStore> ProgramTable::_storeFactory(2);

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

                        if (index.Pid() != 0x10) {
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

                    if (_parent.AddProgram(_frequency, _table) == true) {
                        // do not forget to get a new storage space, the previous one is now
                        // with the AddProgram !!!
                        _table.Storage(_storeFactory.Element());

                        TRACE_L1("PMT for %d loaded. PID load for PMTs completed: %s", _table.Extension(), completedStep ? _T("true") : _T("false"));
                    }
                }
            }

            if (completedStep == true) {
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
} // namespace WPEFramework
