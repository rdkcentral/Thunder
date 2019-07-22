#ifndef __VIRTUAL_INPUT__
#define __VIRTUAL_INPUT__

#include "Module.h"

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
                _parent.MapChanges(removed);
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
        enum actiontype {
            RELEASED = 0,
            PRESSED = 1,
            REPEAT = 2,
            COMPLETED = 3,
            KEY_RELEASED = RELEASED,
            KEY_PRESSED = PRESSED,
            KEY_REPEAT = REPEAT,
            KEY_COMPLETED = COMPLETED,
            AXIS_HORIZONTAL_MOTION = 4,
            AXIS_VERTICAL_MOTION = 5,
            POINTER_RELEASED = 6,
            POINTER_PRESSED = 7,
            POINTER_HORIZONTAL_MOTION = 8,
            POINTER_VERTICAL_MOTION = 9,
            TOUCH_RELEASED = 10,
            TOUCH_PRESSED = 11,
            TOUCH_HORIZONTAL_POSITION = 12,
            TOUCH_VERTICAL_POSITION = 13,
            TOUCH_INDEX = 14,
            TOUCH_COMPLETED = 15
        };

        struct INotifier {
            virtual ~INotifier() {}
            virtual void Dispatch(const actiontype action, const uint32_t code) = 0;
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
        void ModifierKey(const actiontype type, const uint16_t modifiers);
        bool SendModifier(const actiontype type, const enumModifier mode);
        void AdministerAndSendKey(const actiontype type, const uint32_t code);
        void DispatchRegisteredKey(const actiontype type, uint32_t code);

        virtual void SendKey(const actiontype type, const uint32_t code) = 0;

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
        virtual void SendKey(const actiontype type, const uint32_t code);
        bool Updated(ChangeIterator& updated);
        virtual void LookupChanges(const string&);

    private:
        struct uinput_user_dev _uidev;
        int _eventDescriptor;
        const string _source;
        std::map<uint16_t, uint16_t> _deviceKeys;
    };
#endif

    class EXTERNAL IPCKeyboardInput : public VirtualInput {
    private:
        struct KeyData {
            uint32_t Action;
            uint32_t Code;
        };

        typedef Core::IPCMessageType<0, KeyData, Core::Void> KeyMessage;
        typedef Core::IPCMessageType<1, Core::Void, Core::IPC::Text<20>> NameMessage;

        IPCKeyboardInput(const IPCKeyboardInput&) = delete;
        IPCKeyboardInput& operator=(const IPCKeyboardInput&) = delete;

    public:
        class KeyboardLink : public Core::IDispatchType<Core::IIPC> {
        private:
            KeyboardLink(const KeyboardLink&) = delete;
            KeyboardLink& operator=(const KeyboardLink&) = delete;

        public:
            KeyboardLink(Core::IPCChannelType<Core::SocketPort, KeyboardLink>*)
                : _enabled(false)
                , _name()
                , _parent(nullptr)
                , _postLookup(nullptr)
                , _replacement(Core::ProxyType<KeyMessage>::Create())
            {
            }
            virtual ~KeyboardLink()
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

                if (_enabled == true) {
                    if (_postLookup == nullptr) {
                        result = element;
                    } else {
                        KeyMessage& copy(static_cast<KeyMessage&>(*element));

                        ASSERT(dynamic_cast<KeyMessage*>(&(*element)) != nullptr);

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
            inline void Parent(IPCKeyboardInput& parent)
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
            virtual void Dispatch(Core::IIPC& element) override
            {
                ASSERT(dynamic_cast<NameMessage*>(&element) != nullptr);

                _name = (static_cast<NameMessage&>(element).Response().Value());
                _enabled = true;
                _postLookup = _parent->FindPostLookup(_name);
            }

        private:
            bool _enabled;
            string _name;
            IPCKeyboardInput* _parent;
            const PostLookupEntries* _postLookup;
            Core::ProxyType<KeyMessage> _replacement;
        };

        class VirtualInputChannelServer : public Core::IPCChannelServerType<KeyboardLink, true> {
        private:
            typedef Core::IPCChannelServerType<KeyboardLink, true> BaseClass;

        public:
            VirtualInputChannelServer(IPCKeyboardInput& parent, const Core::NodeId& sourceName)
                : BaseClass(sourceName, 32)
                , _parent(parent)
            {
            }

            virtual void Added(Core::ProxyType<Client>& client) override
            {
                TRACE_L1("VirtualInputChannelServer::Added -- %d", __LINE__);

                Core::ProxyType<Core::IIPC> message(Core::ProxyType<NameMessage>::Create());

                // TODO: The reference to this should be held by the IPC mechanism.. Testing showed it did
                //       not, to be further investigated..
                message.AddRef();

                client->Extension().Parent(_parent);
                client->Invoke(message, &(client->Extension()));
            }

        private:
            IPCKeyboardInput& _parent;
        };

    public:
        IPCKeyboardInput(const Core::NodeId& sourceName);
        virtual ~IPCKeyboardInput();

        virtual uint32_t Open();
        virtual uint32_t Close();
        virtual void MapChanges(ChangeIterator& updated);
        virtual void LookupChanges(const string&);

    private:
        virtual void SendKey(const actiontype type, const uint32_t code);

    private:
        VirtualInputChannelServer _service;
    };

    class EXTERNAL IPCMouseInput : public VirtualInput {
    private:
        enum class mouseactiontype : uint32_t {
            MOUSE_RELEASED = 0,
            MOUSE_PRESSED = 1,
            MOUSE_MOTION = 2,
            MOUSE_SCROLL = 3
        };

        struct MouseData {
            mouseactiontype Action;
            uint16_t Button;
            int16_t Horizontal;
            int16_t Vertical;
        };

        typedef Core::IPCMessageType<0, MouseData, Core::Void> MouseMessage;
        typedef Core::IPCMessageType<1, Core::Void, Core::IPC::Text<20>> NameMessage;

        IPCMouseInput(const IPCMouseInput&) = delete;
        IPCMouseInput& operator=(const IPCMouseInput&) = delete;

    public:
        class MouseLink : public Core::IDispatchType<Core::IIPC> {
        private:
            MouseLink(const MouseLink&) = delete;
            MouseLink& operator=(const MouseLink&) = delete;

        public:
            MouseLink(Core::IPCChannelType<Core::SocketPort, MouseLink>*)
                : _enabled(false)
                , _name()
                , _parent()
            {
            }
            virtual ~MouseLink()
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

                if (_enabled == true) {
                    result = element;
                }
                return (result);
            }
            inline const string& Name() const
            {
                return (_name);
            }
            inline void Parent(IPCMouseInput& parent)
            {
                ASSERT(_parent == nullptr);
                _parent = &parent;
            }

        private:
            virtual void Dispatch(Core::IIPC& element) override
            {
                ASSERT(dynamic_cast<NameMessage*>(&element) != nullptr);
                _name = (static_cast<NameMessage&>(element).Response().Value());
                _enabled = true;
            }

        private:
            bool _enabled;
            string _name;
            IPCMouseInput* _parent;
        };

        class VirtualInputChannelServer : public Core::IPCChannelServerType<MouseLink, true> {
        private:
            typedef Core::IPCChannelServerType<MouseLink, true> BaseClass;

        public:
            VirtualInputChannelServer(IPCMouseInput& parent, const Core::NodeId& sourceName)
                : BaseClass(sourceName, 32)
                , _parent(parent)
            {
            }

            virtual void Added(Core::ProxyType<Client>& client) override
            {
                Core::ProxyType<Core::IIPC> message(Core::ProxyType<NameMessage>::Create());
                message.AddRef();
                client->Extension().Parent(_parent);
                client->Invoke(message, &(client->Extension()));
            }

        private:
            IPCMouseInput& _parent;
        };

    public:
        IPCMouseInput(const Core::NodeId& sourceName);
        virtual ~IPCMouseInput();

        uint32_t Open() override;
        uint32_t Close() override;
        void MapChanges(ChangeIterator& updated) override { }
        void LookupChanges(const string&) override { }

    private:
        virtual void SendKey(const actiontype type, const uint32_t code) override;

    private:
        VirtualInputChannelServer _service;
    };

    class EXTERNAL IPCTouchScreenInput : public VirtualInput {
    private:
        enum class touchactiontype : uint32_t {
            TOUCH_RELEASED = 0,
            TOUCH_PRESSED = 1,
            TOUCH_MOTION = 2
        };

        struct TouchData {
            touchactiontype Action;
            uint16_t Index;
            uint16_t X;
            uint16_t Y;
        };

        typedef Core::IPCMessageType<0, TouchData, Core::Void> TouchMessage;
        typedef Core::IPCMessageType<1, Core::Void, Core::IPC::Text<20>> NameMessage;

        IPCTouchScreenInput(const IPCTouchScreenInput&) = delete;
        IPCTouchScreenInput& operator=(const IPCTouchScreenInput&) = delete;

    public:
        class TouchScreenLink : public Core::IDispatchType<Core::IIPC> {
        private:
            TouchScreenLink(const TouchScreenLink&) = delete;
            TouchScreenLink& operator=(const TouchScreenLink&) = delete;

        public:
            TouchScreenLink(Core::IPCChannelType<Core::SocketPort, TouchScreenLink>*)
                : _enabled(false)
                , _name()
                , _parent()
            {
            }
            virtual ~TouchScreenLink()
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

                if (_enabled == true) {
                    result = element;
                }
                return (result);
            }
            inline const string& Name() const
            {
                return (_name);
            }
            inline void Parent(IPCTouchScreenInput& parent)
            {
                ASSERT(_parent == nullptr);
                _parent = &parent;
            }

        private:
            virtual void Dispatch(Core::IIPC& element) override
            {
                ASSERT(dynamic_cast<NameMessage*>(&element) != nullptr);
                _name = (static_cast<NameMessage&>(element).Response().Value());
                _enabled = true;
            }

        private:
            bool _enabled;
            string _name;
            IPCTouchScreenInput* _parent;
        };

        class VirtualInputChannelServer : public Core::IPCChannelServerType<TouchScreenLink, true> {
        private:
            typedef Core::IPCChannelServerType<TouchScreenLink, true> BaseClass;

        public:
            VirtualInputChannelServer(IPCTouchScreenInput& parent, const Core::NodeId& sourceName)
                : BaseClass(sourceName, 32)
                , _parent(parent)
            {
            }

            virtual void Added(Core::ProxyType<Client>& client) override
            {
                Core::ProxyType<Core::IIPC> message(Core::ProxyType<NameMessage>::Create());
                message.AddRef();
                client->Extension().Parent(_parent);
                client->Invoke(message, &(client->Extension()));
            }

        private:
            IPCTouchScreenInput& _parent;
        };

    public:
        IPCTouchScreenInput(const Core::NodeId& sourceName);
        virtual ~IPCTouchScreenInput();

        uint32_t Open() override;
        uint32_t Close() override;
        void MapChanges(ChangeIterator& updated) override { }
        void LookupChanges(const string&) override { }

    private:
        virtual void SendKey(const actiontype type, const uint32_t code) override;

    private:
        VirtualInputChannelServer _service;
        uint16_t _latch_index;
        touchactiontype _latch_action;
        uint32_t _latch_x;
        uint32_t _latch_y;
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
        void InitializeKeyboard(const type t, const string& locator)
        {
            ASSERT(_keyHandler == nullptr);
#if defined(__WIN32__) || defined(__APPLE__)
            ASSERT(t == VIRTUAL)
            _inputHandler = new PluginHost::IPCKeyboardInput(Core::NodeId(locator.c_str()));
            TRACE_L1("Creating a IPC Channel for key communication. %d", 0);
#else
            if (t == VIRTUAL) {
                _keyHandler = new PluginHost::IPCKeyboardInput(Core::NodeId(locator.c_str()));
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

        void InitializeMouse(const type t, const string& locator)
        {
            ASSERT(_mouseHandler == nullptr);
            if (t == VIRTUAL) {
                _mouseHandler = new PluginHost::IPCMouseInput(Core::NodeId(locator.c_str()));
                TRACE_L1("Creating a IPC Channel for mouse communication. %d", 0);
            }
            if (_mouseHandler != nullptr) {
                TRACE_L1("Opening VirtualInput %s", locator.c_str());
                if (_mouseHandler->Open() != Core::ERROR_NONE) {
                    TRACE_L1("ERROR: Could not open VirtualInput %s.", locator.c_str());
                }
            } else {
                TRACE_L1("ERROR: Could not create '%s' as mouse communication channel.", locator.c_str());
            }
        }

        void InitializeTouchScreen(const type t, const string& locator)
        {
            ASSERT(_touchHandler == nullptr);
            if (t == VIRTUAL) {
                _touchHandler = new PluginHost::IPCTouchScreenInput(Core::NodeId(locator.c_str()));
                TRACE_L1("Creating a IPC Channel for touch communication. %d", 0);
            }
            if (_touchHandler != nullptr) {
                TRACE_L1("Opening VirtualInput %s", locator.c_str());
                if (_touchHandler->Open() != Core::ERROR_NONE) {
                    TRACE_L1("ERROR: Could not open VirtualInput %s.", locator.c_str());
                }
            } else {
                TRACE_L1("ERROR: Could not create '%s' as touch communication channel.", locator.c_str());
            }
        }

        void DeinitializeKeyboard()
        {
            if (_keyHandler != nullptr) {
                _keyHandler->Close();
                delete (_keyHandler);
                _keyHandler = nullptr;
            }
        }

        void DeinitializeMouse()
        {
            if (_mouseHandler != nullptr) {
                _mouseHandler->Close();
                delete (_mouseHandler);
                _mouseHandler = nullptr;
            }
        }

        void DeinitializeTouchScreen()
        {
            if (_touchHandler != nullptr) {
                _touchHandler->Close();
                delete (_touchHandler);
                _touchHandler = nullptr;
            }
        }

        static VirtualInput* KeyHandler()
        {
            return _keyHandler;
        }

        static VirtualInput* MouseHandler()
        {
            return _mouseHandler;
        }

        static VirtualInput* TouchHandler()
        {
            return _touchHandler;
        }

    private:
        static VirtualInput* _keyHandler;
        static VirtualInput* _mouseHandler;
        static VirtualInput* _touchHandler;
    };

} // PluginHost

namespace Core {

    template <>
    EXTERNAL /* static */ const EnumerateConversion<PluginHost::VirtualInput::KeyMap::modifier>*
    EnumerateType<PluginHost::VirtualInput::KeyMap::modifier>::Table(const uint16_t);

} // namespace Core

} // namespace WPEFramework

#endif // __VIRTUAL_INPUT__
