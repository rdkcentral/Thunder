#ifndef __VIRTUAL_INPUT__
#define __VIRTUAL_INPUT__

#include "Module.h"
#include "../virtualinput/IVirtualInput.h"

namespace WPEFramework {
namespace PluginHost {

    class EXTERNAL VirtualInput {
    private:
        VirtualInput(const VirtualInput&) = delete;
        VirtualInput& operator=(const VirtualInput&) = delete;

        class RepeatKeyTimer : Core::WatchDogType<RepeatKeyTimer&> {
        private:
            RepeatKeyTimer() = delete;
            RepeatKeyTimer(const RepeatKeyTimer&) = delete;
            RepeatKeyTimer& operator=(const RepeatKeyTimer&) = delete;

            typedef Core::WatchDogType<RepeatKeyTimer&> BaseClass;
            static constexpr uint32_t RepeatStackSize = 64 * 1024;

        public:
#ifdef __WIN32__
#pragma warning(disable : 4355)
#endif
            RepeatKeyTimer(VirtualInput& parent)
                : BaseClass(RepeatStackSize, _T("RepeatKeyTimer"), *this)
                , _parent(parent)
                , _startTime(0)
                , _intervalTime(0)
                , _code(0)
            {
            }
#ifdef __WIN32__
#pragma warning(default : 4355)
#endif
            ~RepeatKeyTimer()
            {
            }

        public:
            void Interval(const uint16_t startTime, const uint16_t intervalTime)
            {
                _startTime = startTime;
                _intervalTime = intervalTime;
            }
            void Arm(const uint16_t code)
            {
                if (_startTime != 0) {
                    BaseClass::Lock();

                    BaseClass::Arm(_startTime);
                    _code = code;

                    BaseClass::Unlock();
                }
            }
            void Reset()
            {
                if (_startTime != 0) {
                    BaseClass::Reset();
                }
            }
            uint32_t Expired()
            {
                _parent.RepeatKey(_code);

                // Let's retrigger at the same time..
                return (_intervalTime);
            }

            virtual void SelectListener(const string& /* listener */)
            {
            }

        private:
            VirtualInput& _parent;
            uint16_t _startTime;
            uint16_t _intervalTime;
            uint16_t _code;
        };

        enum enumModifier {
            LEFTSHIFT = 0, //  0..3  bits are for LeftShift reference counting (max 15)
            RIGHTSHIFT = 4, //  4..7  bits are for RightShift reference counting (max 15)
            LEFTALT = 8, //  8..11 bits are for LeftAlt reference counting (max 15)
            RIGHTALT = 12, // 12..15 bits are for RightAlt reference counting (max 15)
            LEFTCTRL = 16, // 16..19 bits are for LeftCtrl reference counting (max 15)
            RIGHTCTRL = 20 // 20..23 bits are for RightCtrl reference counting (max 15)
        };

    public:
        class EXTERNAL KeyMap {
        public:
            enum modifier {
                LEFTSHIFT = 0x01,
                RIGHTSHIFT = 0x02,
                LEFTALT = 0x04,
                RIGHTALT = 0x08,
                LEFTCTRL = 0x10,
                RIGHTCTRL = 0x20
            };

        private:
            typedef Core::JSON::EnumType<modifier> JSONModifier;

            KeyMap(const KeyMap&) = delete;
            KeyMap& operator=(const KeyMap&) = delete;

        public:
            class KeyMapEntry : public Core::JSON::Container {
            private:
                KeyMapEntry& operator=(const KeyMapEntry&) = delete;

            public:
                inline KeyMapEntry()
                    : Core::JSON::Container()
                {
                    Add(_T("code"), &Code);
                    Add(_T("key"), &Key);
                    Add(_T("modifiers"), &Modifiers);
                }
                inline KeyMapEntry(const KeyMapEntry& copy)
                    : Core::JSON::Container()
                    , Code(copy.Code)
                    , Key(copy.Key)
                    , Modifiers(copy.Modifiers)
                {
                    Add(_T("code"), &Code);
                    Add(_T("key"), &Key);
                    Add(_T("modifiers"), &Modifiers);
                }
                virtual ~KeyMapEntry()
                {
                }

            public:
                Core::JSON::HexUInt32 Code;
                Core::JSON::DecUInt16 Key;
                Core::JSON::ArrayType<JSONModifier> Modifiers;
            };

        public:
            struct ConversionInfo {
                uint16_t Code;
                uint16_t Modifiers;
            };

        private:
            typedef std::map<const uint32_t, const ConversionInfo> LookupMap;

        public:
            typedef Core::IteratorMapType<const LookupMap, const ConversionInfo&, uint32_t, LookupMap::const_iterator> Iterator;

        public:
            KeyMap(KeyMap&&) = default;
            KeyMap(VirtualInput& parent)
                : _parent(parent)
                , _passThrough(false)
            {
            }
            ~KeyMap()
            {

                std::map<uint16_t, int16_t> removedKeys;

                while (_keyMap.size() > 0) {

                    // Negative reference counts
                    removedKeys[_keyMap.begin()->second.Code]--;

                    _keyMap.erase(_keyMap.begin());
                }

                ChangeIterator removed(removedKeys);
            }

        public:
            inline bool PassThrough() const
            {
                return (_passThrough);
            }
            inline void PassThrough(const bool enabled)
            {
                _passThrough = enabled;
            }
            uint32_t Load(const string& mappingFile);
            uint32_t Save(const string& mappingFile);

            inline const ConversionInfo* operator[](const uint32_t code) const
            {
                std::map<const uint32_t, const ConversionInfo>::const_iterator index(_keyMap.find(code));

                return (index != _keyMap.end() ? &(index->second) : nullptr);
            }
            inline bool Add(const uint32_t code, const uint16_t key, const uint16_t modifiers)
            {
                bool added = false;
                std::map<const uint32_t, const ConversionInfo>::const_iterator index(_keyMap.find(code));

                if (index == _keyMap.end()) {
                    ConversionInfo element;
                    element.Code = key;
                    element.Modifiers = modifiers;

                    _keyMap.insert(std::pair<const uint32_t, const ConversionInfo>(code, element));
                    added = true;
                }
                return (added);
            }
            inline void Delete(const uint32_t code)
            {
                std::map<const uint32_t, const ConversionInfo>::const_iterator index(_keyMap.find(code));

                if (index != _keyMap.end()) {
                    _keyMap.erase(index);
                }
            }

            inline bool Modify(const uint32_t code, const uint16_t key, const uint16_t modifiers)
            {
                // Delete if exist
                Delete(code);

                return (Add(code, key, modifiers));
            }

        private:
            VirtualInput& _parent;
            LookupMap _keyMap;
            bool _passThrough;
        };

    public:
        struct INotifier {
            virtual ~INotifier() {}
            virtual void Dispatch(const IVirtualInput::KeyData::type type, const uint32_t code) = 0;
        };
        typedef std::map<const uint32_t, const uint32_t> PostLookupEntries;

    private:
        class PostLookupTable : public Core::JSON::Container {
        private:
            PostLookupTable(const PostLookupTable&) = delete;
            PostLookupTable& operator=(const PostLookupTable&) = delete;

        public:
            class Conversion : public Core::JSON::Container {
            private:
                Conversion& operator=(const Conversion&) = delete;

            public:
                class KeyCode : public Core::JSON::Container {
                private:
                    KeyCode& operator=(const KeyCode&) = delete;

                public:
                    KeyCode()
                        : Core::JSON::Container()
                        , Code()
                        , Mods()
                    {
                        Add(_T("code"), &Code);
                        Add(_T("mods"), &Mods);
                    }
                    KeyCode(const KeyCode& copy)
                        : Core::JSON::Container()
                        , Code()
                        , Mods()
                    {
                        Add(_T("code"), &Code);
                        Add(_T("mods"), &Mods);
                    }
                    virtual ~KeyCode()
                    {
                    }

                public:
                    Core::JSON::DecUInt16 Code;
                    Core::JSON::ArrayType<Core::JSON::EnumType<KeyMap::modifier>> Mods;
                };

            public:
                Conversion()
                    : Core::JSON::Container()
                    , In()
                    , Out()
                {
                    Add(_T("in"), &In);
                    Add(_T("out"), &Out);
                }
                Conversion(const Conversion& copy)
                    : Core::JSON::Container()
                    , In()
                    , Out()
                {
                    Add(_T("in"), &In);
                    Add(_T("out"), &Out);
                }
                virtual ~Conversion()
                {
                }

            public:
                KeyCode In;
                KeyCode Out;
            };

        public:
            PostLookupTable()
                : Core::JSON::Container()
                , Conversions()
            {
                Add(_T("conversion"), &Conversions);
            }
            ~PostLookupTable()
            {
            }

        public:
            Core::JSON::ArrayType<Conversion> Conversions;
        };

        typedef std::map<const string, KeyMap> TableMap;
        typedef std::vector<INotifier*> NotifierList;
        typedef std::map<uint32_t, NotifierList> NotifierMap;
        typedef std::map<const string, PostLookupEntries> PostLookupMap;

    public:
        VirtualInput();
        virtual ~VirtualInput();

    public:
        inline void Interval(const uint16_t startTime, const uint16_t intervalTime)
        {
            _repeatKey.Interval(startTime, intervalTime);
        }

        inline void RepeatLimit(uint16_t limit)
        {
            _repeatLimit = limit;
        }

        virtual uint32_t Open() = 0;
        virtual uint32_t Close() = 0;
        void Default(const string& table)
        {

            if (table.empty() == true) {

                _defaultMap = nullptr;
            } else {
                TableMap::iterator index(_mappingTables.find(table));

                ASSERT(index != _mappingTables.end());

                if (index != _mappingTables.end()) {
                    _defaultMap = &(index->second);
                }
            }
        }

        KeyMap& Table(const string& table)
        {
            TableMap::iterator index(_mappingTables.find(table));
            if (index == _mappingTables.end()) {
                std::pair<TableMap::iterator, bool> result = _mappingTables.insert(std::make_pair(table, KeyMap(*this)));
                index = result.first;
            }

            ASSERT(index != _mappingTables.end());

            return (index->second);
        }

        inline void ClearTable(const std::string& name)
        {
            TableMap::iterator index(_mappingTables.find(name));

            if (index != _mappingTables.end()) {
                _mappingTables.erase(index);
            }
        }

        void Register(INotifier* callback, const uint32_t keyCode = ~0);
        void Unregister(const INotifier* callback, const uint32_t keyCode = ~0);

        // -------------------------------------------------------------------------------------------------------
        // Whenever a key is pressed or released, let this object know, it will take the proper arrangements and timings
        // to announce this key event to the linux system. Repeat event is triggered by the watchdog implementation
        // in this plugin. No need to signal this.
        uint32_t KeyEvent(const bool pressed, const uint32_t code, const string& tablename);
        uint32_t AxisEvent(const int16_t x, const int16_t y);
        uint32_t PointerMotionEvent(const int16_t x, const int16_t y);
        uint32_t PointerButtonEvent(const bool pressed, const uint8_t button);
        uint32_t TouchEvent(const uint8_t index, const uint16_t state, const uint16_t x, const uint16_t y);

        typedef Core::IteratorMapType<const std::map<uint16_t, int16_t>, uint16_t, int16_t, std::map<uint16_t, int16_t>::const_iterator> ChangeIterator;

        void PostLookup(const string& linkName, const string& tableName)
        {
            Core::File data(tableName);
            if (data.Open(true) == true) {
                PostLookupTable info;
                info.FromFile(data);
                Core::JSON::ArrayType<PostLookupTable::Conversion>::Iterator index(info.Conversions.Elements());

                _lock.Lock();

                PostLookupMap::iterator postMap(_postLookupTable.find(linkName));
                if (postMap != _postLookupTable.end()) {
                    postMap->second.clear();
                } else {
                    auto newElement = _postLookupTable.emplace(std::piecewise_construct,
                        std::make_tuple(linkName),
                        std::make_tuple());
                    postMap = newElement.first;
                }
                while (index.Next() == true) {
                    if ((index.Current().In.IsSet() == true) && (index.Current().Out.IsSet() == true)) {
                        uint32_t from = index.Current().In.Code.Value();
                        uint32_t to = index.Current().Out.Code.Value();

                        from |= (Modifiers(index.Current().In.Mods) << 16);
                        to |= (Modifiers(index.Current().In.Mods) << 16);

                        postMap->second.insert(std::pair<const uint32_t, const uint32_t>(from, to));
                    }
                }

                if (postMap->second.size() == 0) {
                    _postLookupTable.erase(postMap);
                }

                LookupChanges(linkName);

                _lock.Unlock();
            }
        }
        inline const PostLookupEntries* FindPostLookup(const string& linkName) const
        {
            PostLookupMap::const_iterator linkMap(_postLookupTable.find(linkName));

            return (linkMap != _postLookupTable.end() ? &(linkMap->second) : nullptr);
        }

    private:
        virtual void MapChanges(ChangeIterator& updated) = 0;
        virtual void LookupChanges(const string&) = 0;

        void RepeatKey(const uint32_t code);
        void ModifierKey(const IVirtualInput::KeyData::type type, const uint16_t modifiers);
        bool SendModifier(const IVirtualInput::KeyData::type type, const enumModifier mode);
        void DispatchRegisteredKey(const IVirtualInput::KeyData::type type, uint32_t code);

        virtual void Send(const IVirtualInput::KeyData& data) = 0;
        virtual void Send(const IVirtualInput::MouseData& data) = 0;
        virtual void Send(const IVirtualInput::TouchData& data) = 0;

        inline uint16_t Modifiers(const Core::JSON::ArrayType<Core::JSON::EnumType<KeyMap::modifier>>& modifiers) const
        {
            uint16_t result = 0;
            Core::JSON::ArrayType<Core::JSON::EnumType<KeyMap::modifier>>::ConstIterator index(modifiers.Elements());

            while (index.Next() == true) {
                KeyMap::modifier element(index.Current());
                result |= element;
            }

            return (result);
        }

    protected:
        Core::CriticalSection _lock;

    private:
        RepeatKeyTimer _repeatKey;
        uint32_t _modifiers;
        std::map<const string, KeyMap> _mappingTables;
        KeyMap* _defaultMap;
        NotifierList _notifierList;
        NotifierMap _notifierMap;
        PostLookupMap _postLookupTable;
        string _keyTable;
        uint32_t _pressedCode;
        uint16_t _repeatCounter;
        uint16_t _repeatLimit;
    };

#if !defined(__WIN32__) && !defined(__APPLE__)
    class EXTERNAL LinuxKeyboardInput : public VirtualInput {
    private:
        LinuxKeyboardInput(const LinuxKeyboardInput&) = delete;
        LinuxKeyboardInput& operator=(const LinuxKeyboardInput&) = delete;

    public:
        LinuxKeyboardInput(const string& source, const string& inputName);
        virtual ~LinuxKeyboardInput();

        virtual uint32_t Open();
        virtual uint32_t Close();
        virtual void MapChanges(ChangeIterator& updated);

    private:
        virtual void Send(const IVirtualInput::KeyData& data) override;
        virtual void Send(const IVirtualInput::MouseData& data) override;
        virtual void Send(const IVirtualInput::TouchData& data) override;
        bool Updated(ChangeIterator& updated);
        virtual void LookupChanges(const string&);

    private:
        struct uinput_user_dev _uidev;
        int _eventDescriptor;
        const string _source;
        std::map<uint16_t, uint16_t> _deviceKeys;
    };
#endif

    class EXTERNAL IPCUserInput : public VirtualInput {
    private:
        IPCUserInput(const IPCUserInput&) = delete;
        IPCUserInput& operator=(const IPCUserInput&) = delete;

    public:
        class InputDataLink : public Core::IDispatchType<Core::IIPC> {
        private:
            InputDataLink(const InputDataLink&) = delete;
            InputDataLink& operator=(const InputDataLink&) = delete;

        public:
            InputDataLink(Core::IPCChannelType<Core::SocketPort, InputDataLink>*)
                : _enabled(false)
                , _name()
                , _mode(0)
                , _parent(nullptr)
                , _postLookup(nullptr)
                , _replacement(Core::ProxyType<IVirtualInput::KeyMessage>::Create())
            {
            }
            virtual ~InputDataLink()
            {
            }

        public:
            inline void Enable(const bool enabled)
            {
                _enabled = enabled;
            }
            inline Core::ProxyType<Core::IIPC> InvokeAllowed(const Core::ProxyType<Core::IIPC>& element) const
            {
                Core::ProxyType<Core::IIPC> result;

                if ( (_enabled == true) && (Subscribed(element->Label()) == true) ) {
                    if ( (element->Label() != IVirtualInput::KeyMessage::Id()) || (_postLookup == nullptr) ) {
                        result = element;
                    } else {
                        IVirtualInput::KeyMessage& copy(static_cast<IVirtualInput::KeyMessage&>(*element));

                        ASSERT(dynamic_cast<IVirtualInput::KeyMessage*>(&(*element)) != nullptr);

                        // See if we need to convert this keycode..
                        PostLookupEntries::const_iterator index(_postLookup->find(copy.Parameters().Code));
                        if (index == _postLookup->end()) {
                            result = element;
                        } else {

                            _replacement->Parameters().Action = copy.Parameters().Action;
                            _replacement->Parameters().Code = index->second;
                            result = Core::ProxyType<Core::IIPC>(_replacement);
                        }
                    }
                }
                return (result);
            }
            inline const string& Name() const
            {
                return (_name);
            }
            inline void Parent(IPCUserInput& parent)
            {
                // We assume it will only be set, if the client reports it self in, once !
                ASSERT(_parent == nullptr);
                _parent = &parent;
            }
            inline void Reload()
            {
                _postLookup = _parent->FindPostLookup(_name);
            }

        private:
            bool Subscribed(const uint32_t id) const {
                uint8_t index (id == IVirtualInput::KeyMessage::Id()   ? IVirtualInput::INPUT_KEY   :
                              (id == IVirtualInput::MouseMessage::Id() ? IVirtualInput::INPUT_MOUSE :
                              (id == IVirtualInput::TouchMessage::Id() ? IVirtualInput::INPUT_TOUCH : 0)));

                return ((index & _mode) != 0);
            }
            virtual void Dispatch(Core::IIPC& element) override
            {
                ASSERT(dynamic_cast<IVirtualInput::NameMessage*>(&element) != nullptr);

                _name = (static_cast<IVirtualInput::NameMessage&>(element).Response().Name);
                _mode = (static_cast<IVirtualInput::NameMessage&>(element).Response().Mode);
                _enabled = true;
                _postLookup = _parent->FindPostLookup(_name);
            }

        private:
            bool _enabled;
            string _name;
            uint8_t _mode;
            IPCUserInput* _parent;
            const PostLookupEntries* _postLookup;
            Core::ProxyType<IVirtualInput::KeyMessage> _replacement;
        };

        class VirtualInputChannelServer : public Core::IPCChannelServerType<InputDataLink, true> {
        private:
            typedef Core::IPCChannelServerType<InputDataLink, true> BaseClass;

        public:
            VirtualInputChannelServer(IPCUserInput& parent, const Core::NodeId& sourceName)
                : BaseClass(sourceName, 32)
                , _parent(parent)
            {
            }

            virtual void Added(Core::ProxyType<Client>& client) override
            {
                TRACE_L1("VirtualInputChannelServer::Added -- %d", __LINE__);

                Core::ProxyType<Core::IIPC> message(Core::ProxyType<IVirtualInput::NameMessage>::Create());

                // TODO: The reference to this should be held by the IPC mechanism.. Testing showed it did
                //       not, to be further investigated..
                message.AddRef();

                client->Extension().Parent(_parent);
                client->Invoke(message, &(client->Extension()));
            }

        private:
            IPCUserInput& _parent;
        };

    public:
        IPCUserInput(const Core::NodeId& sourceName);
        virtual ~IPCUserInput();

        virtual uint32_t Open();
        virtual uint32_t Close();
        virtual void MapChanges(ChangeIterator& updated);
        virtual void LookupChanges(const string&);

    private:
        virtual void Send(const IVirtualInput::KeyData& data) override;
        virtual void Send(const IVirtualInput::MouseData& data) override;
        virtual void Send(const IVirtualInput::TouchData& data) override;

    private:
        VirtualInputChannelServer _service;
    };

    class EXTERNAL InputHandler {
    private:
        InputHandler(const InputHandler&) = delete;
        InputHandler& operator=(const InputHandler&) = delete;

    public:
        InputHandler()
        {
        }
        ~InputHandler()
        {
        }

        enum type {
            DEVICE,
            VIRTUAL
        };

    public:
        void Initialize(const type t, const string& locator)
        {
            ASSERT(_keyHandler == nullptr);
#if defined(__WIN32__) || defined(__APPLE__)
            ASSERT(t == VIRTUAL)
            _inputHandler = new PluginHost::IPCUserInput(Core::NodeId(locator.c_str()));
            TRACE_L1("Creating a IPC Channel for key communication. %d", 0);
#else
            if (t == VIRTUAL) {
                _keyHandler = new PluginHost::IPCUserInput(Core::NodeId(locator.c_str()));
                TRACE_L1("Creating a IPC Channel for key communication. %d", 0);
            } else {
                if (Core::File(locator, false).Exists() == true) {
                    TRACE_L1("Creating a /dev/input device for key communication. %d", 0);

                    // Seems we have a possibility to use /dev/input, create it.
                    _keyHandler = new PluginHost::LinuxKeyboardInput(locator, _T("remote_input"));
                }
            }
#endif
            if (_keyHandler != nullptr) {
                TRACE_L1("Opening VirtualInput %s", locator.c_str());
                if (_keyHandler->Open() != Core::ERROR_NONE) {
                    TRACE_L1("ERROR: Could not open VirtualInput %s.", locator.c_str());
                }
            } else {
                TRACE_L1("ERROR: Could not create '%s' as key communication channel.", locator.c_str());
            }
        }

        void Deinitialize()
        {
            if (_keyHandler != nullptr) {
                _keyHandler->Close();
                delete (_keyHandler);
                _keyHandler = nullptr;
            }
        }

        static VirtualInput* Handler()
        {
            return _keyHandler;
        }

    private:
        static VirtualInput* _keyHandler;
    };

} // PluginHost

namespace Core {

    template <>
    EXTERNAL /* static */ const EnumerateConversion<PluginHost::VirtualInput::KeyMap::modifier>*
    EnumerateType<PluginHost::VirtualInput::KeyMap::modifier>::Table(const uint16_t);

} // namespace Core

} // namespace WPEFramework

#endif // __VIRTUAL_INPUT__
