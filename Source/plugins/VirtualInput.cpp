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

#include "VirtualInput.h"

namespace WPEFramework {

ENUM_CONVERSION_BEGIN(PluginHost::VirtualInput::KeyMap::modifier)

    { PluginHost::VirtualInput::KeyMap::LEFTSHIFT, _TXT("shift") },
    { PluginHost::VirtualInput::KeyMap::LEFTSHIFT, _TXT("leftshift") },
    { PluginHost::VirtualInput::KeyMap::RIGHTSHIFT, _TXT("rightshift") },
    { PluginHost::VirtualInput::KeyMap::LEFTALT, _TXT("alt") },
    { PluginHost::VirtualInput::KeyMap::LEFTALT, _TXT("leftalt") },
    { PluginHost::VirtualInput::KeyMap::RIGHTALT, _TXT("rightalt") },
    { PluginHost::VirtualInput::KeyMap::LEFTCTRL, _TXT("ctrl") },
    { PluginHost::VirtualInput::KeyMap::LEFTCTRL, _TXT("leftctrl") },
    { PluginHost::VirtualInput::KeyMap::RIGHTCTRL, _TXT("rightctrl") },

ENUM_CONVERSION_END(PluginHost::VirtualInput::KeyMap::modifier)

namespace {
    class InputEvent {
    public:
        InputEvent(InputEvent&&) = delete;
        InputEvent(const InputEvent&) = delete;
        InputEvent& operator=(const InputEvent&) = delete;

        InputEvent(const IVirtualInput::KeyData& data) {
            switch (data.Action) {
            case IVirtualInput::KeyData::type::RELEASED:
                _text = Core::ToString(Core::Format(_T("KeyData: RELEASED [0x%X]"), data.Code));
                break;
            case IVirtualInput::KeyData::type::PRESSED:
                _text = Core::ToString(Core::Format(_T("KeyData: PRESSED [0x%X]"), data.Code));
                break;
            case IVirtualInput::KeyData::type::REPEAT:
                _text = Core::ToString(Core::Format(_T("KeyData: REPEAT [0x%X]"), data.Code));
                break;
            case IVirtualInput::KeyData::type::COMPLETED:
                _text = Core::ToString(Core::Format(_T("KeyData: COMPLETED [0x%X]"), data.Code));
                break;
            }
        }
        InputEvent(const IVirtualInput::MouseData& data) {
            switch (data.Action) {
            case IVirtualInput::MouseData::type::RELEASED:
                _text = Core::ToString(Core::Format(_T("MouseData: RELEASED%d (x=%d,y=%d)"), data.Button, data.Horizontal, data.Vertical));
                break;
            case IVirtualInput::MouseData::type::PRESSED:
                _text = Core::ToString(Core::Format(_T("MouseData: PRESSED%d (x=%d,y=%d)"), data.Button, data.Horizontal, data.Vertical));
                break;
            case IVirtualInput::MouseData::type::MOTION:
                _text = Core::ToString(Core::Format(_T("MouseData: MOTION (x=%d,y=%d)"), data.Horizontal, data.Vertical));
                break;
            case IVirtualInput::MouseData::type::SCROLL:
                _text = Core::ToString(Core::Format(_T("MouseData: SCROLL (x=%d,y=%d)"), data.Horizontal, data.Vertical));
                break;
            }
        }
        InputEvent(const IVirtualInput::TouchData& data) {
            switch (data.Action) {
            case IVirtualInput::TouchData::type::RELEASED:
                _text = Core::ToString(Core::Format(_T("TouchData: RELEASED (index=%d,x=%d,y=%d)"), data.Index, data.X, data.Y));
                break;
            case IVirtualInput::TouchData::type::PRESSED:
                _text = Core::ToString(Core::Format(_T("TouchData: PRESSED (index=%d,x=%d,y=%d)"), data.Index, data.X, data.Y));
                break;
            case IVirtualInput::TouchData::type::MOTION:
                _text = Core::ToString(Core::Format(_T("TouchData: MOTION (index=%d,x=%d,y=%d)"), data.Index, data.X, data.Y));
                break;
            }
        }
        ~InputEvent() = default;

    public:
        const char* Data() const
        {
            return (_text.c_str());
        }
        uint16_t Length() const
        {
            return (static_cast<uint16_t>(_text.length()));
        }

    private:
        std::string _text;
    };
}

namespace PluginHost
{
    /* static */ VirtualInput* InputHandler::_keyHandler;

    uint32_t VirtualInput::KeyMap::Import(const Core::JSON::ArrayType<KeyMapEntry>& mappingTable)
    {
        uint32_t result = Core::ERROR_ILLEGAL_STATE;

        if(mappingTable.Length() > 0) {
            std::map<uint16_t, uint16_t> previousKeys;
            std::map<uint16_t, int16_t> updatedKeys;

            result = Core::ERROR_NONE;

            while (_keyMap.size() > 0) {
                // Keep reference count of repeated keys
                previousKeys[_keyMap.begin()->second.Code]++;

                // Create an empty list
                updatedKeys[_keyMap.begin()->second.Code] = 0;

                _keyMap.erase(_keyMap.begin());
            }

            Core::JSON::ArrayType<KeyMapEntry>::ConstIterator index(mappingTable.Elements());
            while (index.Next() == true) {
                if ((index.Current().Code.IsSet()) && (index.Current().Key.IsSet())) {
                    uint32_t code(index.Current().Code.Value());
                    ConversionInfo conversionInfo;

                    conversionInfo.Code = index.Current().Key.Value();
                    conversionInfo.Modifiers = 0;

                    // Build the device info array..
                    Core::JSON::ArrayType<JSONModifier>::ConstIterator flags(index.Current().Modifiers.Elements());

                    while (flags.Next() == true) {
                        switch (flags.Current().Value()) {
                            case KeyMap::modifier::LEFTSHIFT:
                            case KeyMap::modifier::RIGHTSHIFT:
                            case KeyMap::modifier::LEFTALT:
                            case KeyMap::modifier::RIGHTALT:
                            case KeyMap::modifier::LEFTCTRL:
                            case KeyMap::modifier::RIGHTCTRL:
                                conversionInfo.Modifiers |= flags.Current().Value();
                                break;
                            default:
                                ASSERT(false);
                                break;
                        }
                    }

                    // Do not allow same device code to point multiple input keys, it is not a real use-case
                    if (_keyMap.find(code) == _keyMap.end()) {
                        _keyMap.insert(std::pair<const uint32_t, const ConversionInfo>(code, conversionInfo));

                        std::map<uint16_t, uint16_t>::iterator index = previousKeys.find(conversionInfo.Code);

                        if (index != previousKeys.end()) {
                            updatedKeys[index->first]++;
                        } else {
                            updatedKeys[conversionInfo.Code]++;
                        }
                    }
                }
            }

            std::map<uint16_t, uint16_t>::const_iterator updatedKey(previousKeys.begin());

            while (updatedKey != previousKeys.end()) {
                // Get differences with the latest key map and create delta list
                updatedKeys[updatedKey->first] -= previousKeys[updatedKey->first];
                updatedKey++;
            }

            ChangeIterator updated(updatedKeys);
            _parent.MapChanges(updated);
        }

        return (result);
    }

    void VirtualInput::KeyMap::Export(Core::JSON::ArrayType<KeyMapEntry>& mappingTable)
    {
        std::map<const uint32_t, const ConversionInfo>::const_iterator index(_keyMap.begin());

        while (index != _keyMap.end()) {

            KeyMapEntry element;
            element.Code = index->first;
            element.Key = index->second.Code;

            uint16_t flag(1);
            uint16_t modifiers(index->second.Modifiers);

            while (modifiers != 0) {

                if ((modifiers & 0x01) != 0) {
                    switch (flag) {
                    case KeyMap::modifier::LEFTSHIFT:
                    case KeyMap::modifier::RIGHTSHIFT:
                    case KeyMap::modifier::LEFTALT:
                    case KeyMap::modifier::RIGHTALT:
                    case KeyMap::modifier::LEFTCTRL:
                    case KeyMap::modifier::RIGHTCTRL: {
                        JSONModifier& jsonRef = element.Modifiers.Add();
                        jsonRef = static_cast<KeyMap::modifier>(flag);
                        break;
                    }
                    default:
                        ASSERT(false);
                        break;
                    }
                }

                flag = flag << 1;
                modifiers = modifiers >> 1;
            }

            mappingTable.Add(element);
            index++;
        }
    }

PUSH_WARNING(DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST)
    VirtualInput::VirtualInput()
        : _lock()
        , _repeatKey(this)
        , _modifiers(0)
        , _defaultMap(nullptr)
        , _notifierMap()
        , _postLookupParent()
        , _postLookupTable()
        , _keyTable()
        , _pressedCode(~0)
        , _repeatCounter(0)
        , _repeatLimit(0)
    {
        // The derived class shoud set, the initial value of the modifiers...
        _repeatKey.AddRef();
        _repeatKey.AddReference();
    }
POP_WARNING()

    VirtualInput::~VirtualInput()
    {
        _mappingTables.clear();
        _repeatKey.DropReference();
        _repeatKey.CompositRelease();
    }

    void VirtualInput::Register(INotifier * callback, const uint32_t keyCode)
    {
        // Register is only usefull with actuall callbacks !!
        ASSERT(callback != nullptr);

        if (keyCode == static_cast<uint32_t>(~0)) {

            _lock.Lock();

            // Only register a callback once !!
            ASSERT(std::find(_notifierList.begin(), _notifierList.end(), callback) == _notifierList.end());

            _notifierList.push_back(callback);

            _lock.Unlock();

        } else {
            _lock.Lock();

            NotifierList& notifierList(_notifierMap[keyCode]);

            // Only register a callback once !!
            ASSERT(std::find(notifierList.begin(), notifierList.end(), callback) == notifierList.end());

            notifierList.push_back(callback);

            _lock.Unlock();
        }
    }

    void VirtualInput::Unregister(const INotifier* callback, const uint32_t keyCode)
    {
        // Unregister is only usefull with actuall callbacks !!
        ASSERT(callback != nullptr);

        if (keyCode == static_cast<uint32_t>(~0)) {

            _lock.Lock();

            NotifierList::iterator position = std::find(_notifierList.begin(), _notifierList.end(), callback);

            // Do not unregister something you did not register !!!
            ASSERT(position != _notifierList.end());

            if (position != _notifierList.end()) {
                _notifierList.erase(position);
            }

            _lock.Unlock();
        } else {

            _lock.Lock();

            NotifierMap::iterator it(_notifierMap.find(keyCode));

            // Do not unregister something you did not register !!!
            ASSERT(it != _notifierMap.end());

            if (it != _notifierMap.end()) {
                NotifierList& notifierList(it->second);
                NotifierList::iterator position = std::find(notifierList.begin(), notifierList.end(), callback);

                // Do not unregister something you did not register !!!
                ASSERT(position != notifierList.end());

                if (position != notifierList.end()) {
                    notifierList.erase(position);

                    if (notifierList.empty() == true) {
                        _notifierMap.erase(it);
                    }
                }
            }

            _lock.Unlock();
        }
    }

    uint32_t VirtualInput::AxisEvent(const int16_t x, const int16_t y)
    {
        IVirtualInput::MouseData event;
        event.Action     = IVirtualInput::MouseData::SCROLL;
        event.Horizontal = x;
        event.Vertical   = y;
        Send(event);

        return (Core::ERROR_NONE);
    }

    uint32_t VirtualInput::PointerMotionEvent(const int16_t x, const int16_t y)
    {
        IVirtualInput::MouseData event;
        event.Action     = IVirtualInput::MouseData::MOTION;
        event.Horizontal = x;
        event.Vertical   = y;
        Send(event);

        return (Core::ERROR_NONE);
    }

    uint32_t VirtualInput::PointerButtonEvent(const bool pressed, const uint8_t button)
    {
        IVirtualInput::MouseData event;
        event.Action = (pressed ? IVirtualInput::MouseData::PRESSED : IVirtualInput::MouseData::RELEASED);
        event.Button = button;
        Send(event);
 
        return (Core::ERROR_NONE);
    }

    uint32_t VirtualInput::TouchEvent(const uint8_t index, uint16_t state, uint16_t x, uint16_t y)
    {
        IVirtualInput::TouchData event;
        event.Action = (state == 0 ? IVirtualInput::TouchData::MOTION: 
                       (state == 1 ? IVirtualInput::TouchData::RELEASED : IVirtualInput::TouchData::PRESSED));
        event.Index = index;
        event.X = x;
        event.Y = y;
        Send(event);

        return (Core::ERROR_NONE);
    }

    uint32_t VirtualInput::KeyEvent(const bool pressed, const uint32_t code, const string& table)
    {
        uint32_t result = Core::ERROR_UNKNOWN_TABLE;

        _lock.Lock();

        TableMap::const_iterator index(_mappingTables.find(table));
        const KeyMap* conversionTable = nullptr;

        if (index == _mappingTables.end()) {
            conversionTable = _defaultMap;
        } else {
            _keyTable = table;
            conversionTable = &(index->second);
        }

        if (conversionTable != nullptr) {
            uint32_t sendCode = ~0;

            result = Core::ERROR_NONE;

            // See if we find the code in the table..
            const KeyMap::ConversionInfo* element = (*conversionTable)[code];

            if (element == nullptr) {
                if (conversionTable->PassThrough() == false) {
                    result = Core::ERROR_UNKNOWN_KEY;
                } else {
                    sendCode = code;
                }
            } else {
                sendCode = element->Code | (element->Modifiers << 16);
            }

            // Modifiers can be pressed when there is a new press for another key.
            if ((pressed == false) && (_pressedCode != sendCode) && !Modifier(sendCode)) {
                result = Core::ERROR_ALREADY_RELEASED;
            }
            else if (sendCode != static_cast<uint32_t>(~0)) {
                if ( (pressed == true) && (_pressedCode != static_cast<uint32_t>(~0)) && !Modifier(_pressedCode)) {
                    DispatchRegisteredKey(IVirtualInput::KeyData::RELEASED, _pressedCode);
                }

                DispatchRegisteredKey(
                        (pressed ? IVirtualInput::KeyData::PRESSED : IVirtualInput::KeyData::RELEASED),
                         sendCode);
            }
        }

        _lock.Unlock();

        return (result);
    }

    void VirtualInput::RepeatKey(const uint16_t code)
    {
        IVirtualInput::KeyData event;
        event.Action = IVirtualInput::KeyData::REPEAT;
        event.Code = code;
        Send(event);
        if (--_repeatCounter == 0) {
             _lock.Lock();
            DispatchRegisteredKey(IVirtualInput::KeyData::RELEASED, _pressedCode);
             _lock.Unlock();
        }
    }

    bool VirtualInput::SendModifier(const IVirtualInput::KeyData::type type, const enumModifier mode)
    {
        bool trigger = false;

        // Reference count all modifiers. (up to 15 references can be held). If this is the
        // first reference, send the key (return true) on the pressed and if it is the last
        // reference on the release, also send the key (return true). Otherwise just increment
        // or decrement.
        uint8_t count((_modifiers >> mode) & 0xF);

        if (type == IVirtualInput::KeyData::PRESSED) {
            // Do we need to send the key, trigger required?
            trigger = (count == 0);

            // First clear our value.
            _modifiers &= ~(0x0f << mode);

            ASSERT(count < 0xF);

            // Then write the incremented value
            _modifiers |= ((count + 1) << mode);
        } else if (type == IVirtualInput::KeyData::RELEASED) {
            // Do we need to send the key, trigger required?
            trigger = (count == 1);

            // First clear our value.
            _modifiers &= ~(0x0f << mode);

            ASSERT(count > 0x0);

            // Then write the decremented value
            _modifiers |= ((count - 1) << mode);
        }

        return (trigger);
    }

    void VirtualInput::ModifierKey(const IVirtualInput::KeyData::type type, const uint16_t modifiers)
    {
        IVirtualInput::KeyData event;
        event.Action = type;

        if (((modifiers & KeyMap::modifier::LEFTSHIFT) != 0) && (SendModifier(type, enumModifier::LEFTSHIFT) == true)) {
            event.Code = KEY_LEFTSHIFT;
            Send(event);
        }
        if (((modifiers & KeyMap::modifier::RIGHTSHIFT) != 0) && (SendModifier(type, enumModifier::RIGHTSHIFT) == true)) {
            event.Code = KEY_RIGHTSHIFT;
            Send(event);
        }
        if (((modifiers & KeyMap::modifier::LEFTALT) != 0) && (SendModifier(type, enumModifier::LEFTALT) == true)) {
            event.Code = KEY_LEFTALT;
            Send(event);
        }
        if (((modifiers & KeyMap::modifier::RIGHTALT) != 0) && (SendModifier(type, enumModifier::RIGHTALT) == true)) {
            event.Code = KEY_RIGHTALT;
            Send(event);
        }
        if (((modifiers & KeyMap::modifier::LEFTCTRL) != 0) && (SendModifier(type, enumModifier::LEFTCTRL) == true)) {
            event.Code = KEY_LEFTCTRL;
            Send(event);
        }
        if (((modifiers & KeyMap::modifier::RIGHTCTRL) != 0) && (SendModifier(type, enumModifier::RIGHTCTRL) == true)) {
            event.Code = KEY_RIGHTCTRL;
            Send(event);
        }
    }

    void VirtualInput::DispatchRegisteredKey(const IVirtualInput::KeyData::type type, const uint32_t code)
    {
        uint32_t sendCode = code;

        // Check in the Parent Table if we really need to dispatch this..
        if ( (type == IVirtualInput::KeyData::PRESSED) && (_postLookupParent.size() > 0) ) {
            PostLookupEntries::iterator index (_postLookupParent.find(sendCode));
            if (index != _postLookupParent.end()) {
                sendCode = index->second;
            }
        }

        if (sendCode != static_cast<uint32_t>(~0)) {
            if (type == IVirtualInput::KeyData::PRESSED) {
                TRACE_L1("Pressed: keyCode: %d, sending: %d", code, sendCode);
                _repeatCounter = _repeatLimit;
                _repeatKey.Arm(sendCode);
                _pressedCode = code;

                uint16_t modifiers = static_cast<uint16_t>((sendCode >> 16) & 0xFFFF);

                if (modifiers != 0) {
                    ModifierKey(IVirtualInput::KeyData::PRESSED, modifiers);
                }
            } else {
                ASSERT (_pressedCode == code);
                _repeatKey.Reset();
                _pressedCode = ~0;
                TRACE_L1("Released: keyCode: %d, sending: %d", code, sendCode);
            }

            IVirtualInput::KeyData event;
            event.Action = type;
            event.Code = (sendCode & 0xFFFF);
            Send(event);

            if (type == IVirtualInput::KeyData::RELEASED) {
                uint16_t modifiers = static_cast<uint16_t>((sendCode >> 16) & 0xFFFF);
                if (modifiers != 0) {
                    ModifierKey(IVirtualInput::KeyData::RELEASED, modifiers);
                }
            }

            event.Action = IVirtualInput::KeyData::COMPLETED;
            Send(event);
        }

        for (INotifier* element : _notifierList) {
            element->Dispatch(type, code);
        }

        NotifierMap::iterator it(_notifierMap.find(code));

        if (it != _notifierMap.end()) {
            const NotifierList& notifierList = it->second;
            for (INotifier* element : notifierList) {
                element->Dispatch(type, code);
            }
        }
    }

#if !defined(__WINDOWS__) && !defined(__APPLE__)

    static const string ConsumerName (_T("LinuxKeyboard"));

    LinuxKeyboardInput::LinuxKeyboardInput(const string& source, const string& inputName, const bool defaultEnabled)
        : VirtualInput()
        , _eventDescriptor(-1)
        , _source(source)
        , _deviceKeys()
        , _enabled(defaultEnabled)
    {
        memset(&_uidev, 0, sizeof(_uidev));

        strncpy(_uidev.name, inputName.c_str(), UINPUT_MAX_NAME_SIZE);
        _uidev.id.bustype = BUS_USB;
        _uidev.id.vendor = 0x1234;
        _uidev.id.product = 0xfedc;
        _uidev.id.version = 1;
    }

    /* virtual */ LinuxKeyboardInput::~LinuxKeyboardInput()
    {
        ClearKeyMap();
        Close();
    }

    /* virtual */ VirtualInput::Iterator LinuxKeyboardInput::Consumers() const
    {
        return (VirtualInput::Iterator(ConsumerName));
    }

    /* virtual */ bool LinuxKeyboardInput::Consumer(const string& name) const 
    {
        return (name == ConsumerName ? _enabled : false);
    }

    /* virtual */ void LinuxKeyboardInput::Consumer(const string& name, const bool enabled)
    {
        if (name == ConsumerName) {
            _enabled = enabled;
        }
    }

    /* virtual */ uint32_t LinuxKeyboardInput::Open()
    {
        uint32_t result = Core::ERROR_NONE;

        if (_eventDescriptor == -1) {
            result = Core::ERROR_OPENING_FAILED;
            // open filedescriptor for uinput kernel module
            _eventDescriptor = open(_source.c_str(), O_WRONLY | O_NONBLOCK);

            if (_eventDescriptor != -1) {
                result = Core::ERROR_NONE;
                ioctl(_eventDescriptor, UI_SET_EVBIT, EV_KEY);
                ioctl(_eventDescriptor, UI_SET_EVBIT, EV_SYN);

                // Make sure we can send the modifiers...
                ioctl(_eventDescriptor, UI_SET_KEYBIT, KEY_LEFTSHIFT);
                ioctl(_eventDescriptor, UI_SET_KEYBIT, KEY_RIGHTSHIFT);
                ioctl(_eventDescriptor, UI_SET_KEYBIT, KEY_LEFTALT);
                ioctl(_eventDescriptor, UI_SET_KEYBIT, KEY_RIGHTALT);
                ioctl(_eventDescriptor, UI_SET_KEYBIT, KEY_LEFTCTRL);
                ioctl(_eventDescriptor, UI_SET_KEYBIT, KEY_RIGHTCTRL);
            }
        }

        return (result);
    }

    /* virtual */ uint32_t LinuxKeyboardInput::Close()
    {
        if (_eventDescriptor != -1) {

            ioctl(_eventDescriptor, UI_DEV_DESTROY);
            close(_eventDescriptor);
            _eventDescriptor = -1;
        }

        return (Core::ERROR_NONE);
    }

    /* virtual */ void LinuxKeyboardInput::LookupChanges(const string& )
    {
    }

    /* virtual */ void LinuxKeyboardInput::MapChanges(ChangeIterator& updated)
    {
        _lock.Lock();

        if (Updated(updated) == true) {

            // Close virtual device if it has been already opened!
            uint32_t VARIABLE_IS_NOT_USED result = Close();
            ASSERT(result == Core::ERROR_NONE);

            if (_deviceKeys.size() > 0) {
                // Open a new virtual device
                result = Open();
                ASSERT(result == Core::ERROR_NONE);

                // Set filter keys for virtual device
                for_each(_deviceKeys.begin(), _deviceKeys.end(), [&](const std::pair<uint16_t, uint16_t>& key) {
                    ioctl(_eventDescriptor, UI_SET_KEYBIT, key.first);
                });

                // We are ready! Create that device!
                PUSH_WARNING(DISABLE_WARNING_UNUSED_RESULT);
                write(_eventDescriptor, &_uidev, sizeof(_uidev));
                POP_WARNING();

                ioctl(_eventDescriptor, UI_DEV_CREATE);
            }
        }
        _lock.Unlock();
    }

    /* virtual */ void LinuxKeyboardInput::Send(const IVirtualInput::KeyData& data)
    {
        if ((_enabled == true) && (_eventDescriptor > 0)) {

            struct input_event ev;

            memset(&ev, 0, sizeof(ev));

            ev.type  = ((data.Action == IVirtualInput::KeyData::COMPLETED) ? EV_SYN : EV_KEY);
            ev.value = ((data.Action == IVirtualInput::KeyData::COMPLETED) ? SYN_REPORT : 
                       ((data.Action == IVirtualInput::KeyData::PRESSED) ? 1 : 
                        (data.Action == IVirtualInput::KeyData::RELEASED ? 0 : 2)));
            ev.code  = ((data.Action == IVirtualInput::KeyData::COMPLETED) ? 0 : data.Code);

            TRACE_L1("Inserted a keycode: %d", data.Code);

            PUSH_WARNING(DISABLE_WARNING_UNUSED_RESULT);
            write(_eventDescriptor, &ev, sizeof(ev));
            POP_WARNING();
        }
    }

    /* virtual */ void LinuxKeyboardInput::Send(const IVirtualInput::MouseData&)
    {
    }

    /* virtual */ void LinuxKeyboardInput::Send(const IVirtualInput::TouchData&)
    {
    }


    bool LinuxKeyboardInput::Updated(ChangeIterator & updated)
    {
        bool updateTable = false;

        updated.Reset(0);

        while (updated.Next() == true) {

            std::map<uint16_t, uint16_t>::iterator current(_deviceKeys.find(updated.Key()));
            if (current != _deviceKeys.end()) {

                // If the ref count is changed or not (increased, decreased or unchanged)
                // <list>       <delta>
                // --------------------
                // 'KEY_1'      '+2'    add 2 refs
                // 'KEY_UP'     '-1'    subtract 1 ref
                // 'KEY_LEFT'   '0'     unchanged
                if (*updated != 0) {

                    // It is changed, add or subtract reference counts
                    current->second += *updated;

                    // There is no ref count anymore, remove key
                    if (current->second == 0) {
                        _deviceKeys.erase(current);
                    }
                    updateTable = true;
                }
            } else {
                updateTable = true;
                _deviceKeys.insert(std::pair<uint16_t, uint16_t>(updated.Key(), *updated));
            }
        }

        return (updateTable);
    }

#endif

    // Keyboard input
PUSH_WARNING(DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST)
    IPCUserInput::IPCUserInput(const Core::NodeId& sourceName, const bool defaultEnabled)
        : _service(*this, sourceName)
        , _defaultEnabled(defaultEnabled)
    {
        TRACE_L1("Constructing IPCUserInput for %s on %s", sourceName.HostAddress().c_str(), sourceName.HostName().c_str());
    }
POP_WARNING()

    /* virtual */ IPCUserInput::~IPCUserInput()
    {
        ClearKeyMap();
        _service.Cleanup();
    }

    /* virtual */ VirtualInput::Iterator IPCUserInput::Consumers() const
    {
        std::list<string> container;
            
        _service.Visit(
            [ &container ](const VirtualInputChannelServer::Client& element)
            {
                string result = element.Extension().Name();
                container.push_back(result);
            }
        );

        return (VirtualInput::Iterator(std::move(container)));
    }

    /* virtual */ bool IPCUserInput::Consumer(const string& name) const {
        Core::ProxyType<const VirtualInputChannelServer::Client> result = _service.Find(
            [name] (const VirtualInputChannelServer::Client& element)
            {
                return (element.Extension().Name() == name);
            }
        );

        return(result.IsValid() && result->Extension().Enable());
    }

    /* virtual */ void IPCUserInput::Consumer(const string& name, const bool enabled) {
        Core::ProxyType<VirtualInputChannelServer::Client> result = _service.Find(
            [name](const VirtualInputChannelServer::Client& element)
            {
                return (element.Extension().Name() == name);
            }
        );

        if (result.IsValid() == true) {
            result->Extension().Enable(enabled);
        }
    }

    /* virtual */ uint32_t IPCUserInput::Open()
    {
        return (_service.Open(2000));
    }

    /* virtual */ uint32_t IPCUserInput::Close()
    {
        _service.Close(2000);
        return (Core::ERROR_NONE);
    }

    /* virtual */ void IPCUserInput::Send(const IVirtualInput::KeyData& data)
    {
        static Core::ProxyType<IVirtualInput::KeyMessage> message(Core::ProxyType<IVirtualInput::KeyMessage>::Create());

        message->Parameters() = data;
        Core::ProxyType<Core::IIPC> base(message);
        TRACE(InputEvent, (data));
        _service.Invoke(base, RPC::CommunicationTimeOut);
    }

    /* virtual */ void IPCUserInput::Send(const IVirtualInput::MouseData& data)
    {
        static Core::ProxyType<IVirtualInput::MouseMessage> message(Core::ProxyType<IVirtualInput::MouseMessage>::Create());

        message->Parameters() = data;
        Core::ProxyType<Core::IIPC> base(message);
        TRACE(InputEvent, (data));
        _service.Invoke(base, RPC::CommunicationTimeOut);
    }

    /* virtual */ void IPCUserInput::Send(const IVirtualInput::TouchData& data)
    {
        static Core::ProxyType<IVirtualInput::TouchMessage> message(Core::ProxyType<IVirtualInput::TouchMessage>::Create());

        message->Parameters() = data;
        Core::ProxyType<Core::IIPC> base(message);
        TRACE(InputEvent, (data));
        _service.Invoke(base, RPC::CommunicationTimeOut);
    }

    /* virtual */ void IPCUserInput::MapChanges(ChangeIterator&) {}

    /* virtual */ void IPCUserInput::LookupChanges(const string& linkName)
    {
        _service.Visit(
            [ linkName ](VirtualInputChannelServer::Client& element)
            {
                if (element.Extension().Name() == linkName) {
                    element.Extension().Reload();
                }
            }
        );
    }

} // Namespace PluginHost
}


