#include "VirtualInput.h"

namespace WPEFramework
{

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

    namespace PluginHost {
       /*static */ VirtualInput* InputHandler::_inputHandler;

        uint32_t VirtualInput::KeyMap::Load(const string& keyMap)
        {
            uint32_t result = Core::ERROR_ILLEGAL_STATE;

            if ((keyMap.empty() == false) && (Core::File(keyMap).Exists() == true)) {
                result = Core::ERROR_OPENING_FAILED;

                std::map<uint16_t,uint16_t> previousKeys;
                std::map<uint16_t,int16_t> updatedKeys;

                while (_keyMap.size() > 0) {

                    // Keep reference count of repeated keys
                    previousKeys[_keyMap.begin()->second.Code]++;

                    // Create an empty list
                    updatedKeys[_keyMap.begin()->second.Code] = 0;

                    _keyMap.erase(_keyMap.begin());
                }

                Core::File mappingFile(keyMap);
                Core::JSON::ArrayType<KeyMapEntry> mappingTable;

                if (mappingFile.Open(true) == true) {
                    result = Core::ERROR_NONE;

                    mappingTable.FromFile(mappingFile);

                    // Build the device info array..
                    Core::JSON::ArrayType<KeyMapEntry>::Iterator index(mappingTable.Elements());

                    while (index.Next() == true) {
                        if ((index.Current().Code.IsSet()) && (index.Current().Key.IsSet())) {
                            uint32_t code(index.Current().Code.Value());
                            ConversionInfo conversionInfo;

                            conversionInfo.Code = index.Current().Key.Value();
                            conversionInfo.Modifiers = 0;

                            // Build the device info array..
                            Core::JSON::ArrayType<JSONModifier>::Iterator flags(index.Current().Modifiers.Elements());

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

                                std::map<uint16_t,uint16_t>::iterator index = previousKeys.find(conversionInfo.Code);

                                if (index != previousKeys.end()) {
                                    updatedKeys[index->first]++;
                                }
                                else {
                                    updatedKeys[conversionInfo.Code]++;
                                }
                            }
                        }
                    }

                    std::map<uint16_t,uint16_t >::const_iterator updatedKey(previousKeys.begin());

                    while(updatedKey != previousKeys.end()) {

                        // Get differences with the latest key map and create delta list
                        updatedKeys[updatedKey->first] -= previousKeys[updatedKey->first];
                        updatedKey++;
                    }

                    ChangeIterator updated(updatedKeys);
                    _parent.MapChanges(updated);
                }
            }

            return (result);
        }

        uint32_t VirtualInput::KeyMap::Save(const string& keyMap)
        {
            uint32_t result = Core::ERROR_ILLEGAL_STATE;

            Core::File mappingFile(keyMap, true);
            Core::JSON::ArrayType<KeyMapEntry> mappingTable;

            if (mappingFile.Create() == true) {
                mappingFile.SetSize(0);

                result = Core::ERROR_NONE;

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
                                JSONModifier &jsonRef = element.Modifiers.Add();
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
                mappingTable.ToFile(mappingFile);
            }

            return (result);
        }

		#ifdef __WIN32__
		#pragma warning( disable : 4355 )
		#endif
        VirtualInput::VirtualInput()
            : _lock()
            , _repeatKey(*this)
            , _modifiers(0)
            , _defaultMap(nullptr)
            , _notifierMap()
        {
            // The derived class shoud set, the initial value of the modifiers...
        }
		#ifdef __WIN32__
		#pragma warning( default : 4355 )
		#endif

        VirtualInput::~VirtualInput()
        {

            _mappingTables.clear();
        }

        void VirtualInput::Register(const uint32_t keyCode, Notifier* callback)
        {
            // Register is only usefull with actuall callbacks !!
            ASSERT (callback != nullptr);

            _lock.Lock();

            NotifierList& notifierList (_notifierMap[keyCode]);

            // Only register a callback once !!
            ASSERT (std::find(notifierList.begin(), notifierList.end(), callback) == notifierList.end());

            notifierList.push_back(callback);

            _lock.Unlock();
        }

        void VirtualInput::Unregister(const uint32_t keyCode, const Notifier* callback)
        {
            // Unregister is only usefull with actuall callbacks !!
            ASSERT (callback != nullptr);

            // Do not unregister something you did not register !!!
            ASSERT (_notifierMap.empty() == false);

            _lock.Lock();

            NotifierMap::iterator it(_notifierMap.find(keyCode));

            if (it != _notifierMap.end()) {
                NotifierList& notifierList (it->second);
                NotifierList::iterator position = std::find(notifierList.begin(), notifierList.end(), callback);

                if (position != notifierList.end()) {
                    notifierList.erase(position);
                }
                else {
                    // Do not unregister something you did not register !!!
                    ASSERT (false);
                }

                if (notifierList.empty() == true) {
                    _notifierMap.erase(it);
                }
 
            }
            else {
                // Do not unregister something you did not register !!!
                ASSERT (false);
            }

            _lock.Unlock();
        }

        uint32_t VirtualInput::KeyEvent(const bool pressed, const uint32_t code, const string& table)
        {
            uint32_t result = Core::ERROR_UNKNOWN_TABLE;

            _lock.Lock();

            TableMap::const_iterator index(_mappingTables.find(table));
            const KeyMap* conversionTable = nullptr;

            if (index == _mappingTables.end()) {
                conversionTable = _defaultMap;
            }
            else {
                conversionTable = &(index->second);
            }

            if (conversionTable == nullptr) {
                _lock.Unlock();
            }
            else {
                uint32_t sendCode = 0;
                uint16_t sendModifiers = 0;

                result = Core::ERROR_NONE;

                // See if we find the code in the table..
                const KeyMap::ConversionInfo* element = (*conversionTable)[code];

                if (element == nullptr) {
                    if (conversionTable->PassThrough() == false) {
                        result = Core::ERROR_UNKNOWN_KEY;
                    }
                    else {
                        result = Core::ERROR_UNKNOWN_KEY_PASSED;
                        sendCode = code;
                        sendModifiers = 0;
                    }
                }
                else {
                    sendCode = element->Code;
                    sendModifiers = element->Modifiers;
                }

                _lock.Unlock();

                if ( (result == Core::ERROR_NONE) || (result == Core::ERROR_UNKNOWN_KEY_PASSED) ) {
                    if (pressed == true) {
                        TRACE_L1("Ingested keyCode: %d pressed", sendCode);
                        _repeatKey.Arm(sendCode);

                        if (sendModifiers != 0) {
                            ModifierKey(PRESSED, sendModifiers);
                        }
                    }
                    else {
                        TRACE_L1("Ingested keyCode: %d Released", sendCode);
                        _repeatKey.Reset();
                    }

                    AdministerAndSendKey((pressed ? PRESSED : RELEASED), sendCode);

                    if (pressed == false) {
                        if (sendModifiers != 0) {
                            ModifierKey(RELEASED, sendModifiers);
                        }

                        DispatchRegisteredKey(sendCode);
                    }

                    SendKey(COMPLETED, sendCode);
                }
            }

            return (result);
        }

        void VirtualInput::RepeatKey(const uint32_t code)
        {
            AdministerAndSendKey(REPEAT, code);
        }

        void VirtualInput::AdministerAndSendKey(const actiontype type, const uint32_t code)
        {
            bool trigger;

            if (code == KeyMap::modifier::LEFTSHIFT) {
                trigger = SendModifier(type, enumModifier::LEFTSHIFT);
            }
            else if (code == KeyMap::modifier::RIGHTSHIFT) {
                trigger = SendModifier(type, enumModifier::RIGHTSHIFT);
            }
            else if (code == KeyMap::modifier::LEFTALT) {
                trigger = SendModifier(type, enumModifier::LEFTALT);
            }
            else if (code == KeyMap::modifier::RIGHTALT) {
                trigger = SendModifier(type, enumModifier::RIGHTALT);
            }
            else if (code == KeyMap::modifier::LEFTCTRL) {
                trigger = SendModifier(type, enumModifier::LEFTCTRL);
            }
            else if (code == KeyMap::modifier::RIGHTCTRL) {
                trigger = SendModifier(type, enumModifier::RIGHTCTRL);
            }
            else {
                trigger = true;
            }

            if (trigger == true) {
                SendKey(type, code);
            }
        }

        bool VirtualInput::SendModifier(const actiontype type, const enumModifier mode)
        {
            bool trigger = false;

            // Reference count all modifiers. (up to 15 references can be held). If this is the
            // first reference, send the key (return true) on the pressed and if it is the last
            // reference on the release, also send the key (return true). Otherwise just increment
            // or decrement.
            uint8_t count((_modifiers >> mode) & 0xF);

            if (type == PRESSED) {
                // Do we need to send the key, trigger required?
                trigger = (count == 0);

                // First clear our value.
                _modifiers &= ~(0x0f << mode);

                ASSERT(count < 0xF);

                // Then write the incremented value
                _modifiers |= ((count + 1) << mode);
            }
            else if (type == RELEASED) {
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

        void VirtualInput::ModifierKey(const actiontype type, const uint16_t modifiers)
        {
            if (((modifiers & KeyMap::modifier::LEFTSHIFT) != 0) && (SendModifier(type, enumModifier::LEFTSHIFT) == true)) {
                SendKey(type, KEY_LEFTSHIFT);
            }
            if (((modifiers & KeyMap::modifier::RIGHTSHIFT) != 0) && (SendModifier(type, enumModifier::RIGHTSHIFT) == true)) {
                SendKey(type, KEY_RIGHTSHIFT);
            }
            if (((modifiers & KeyMap::modifier::LEFTALT) != 0) && (SendModifier(type, enumModifier::LEFTALT) == true)) {
                SendKey(type, KEY_LEFTALT);
            }
            if (((modifiers & KeyMap::modifier::RIGHTALT) != 0) && (SendModifier(type, enumModifier::RIGHTALT) == true)) {
                SendKey(type, KEY_RIGHTALT);
            }
            if (((modifiers & KeyMap::modifier::LEFTCTRL) != 0) && (SendModifier(type, enumModifier::LEFTCTRL) == true)) {
                SendKey(type, KEY_LEFTCTRL);
            }
            if (((modifiers & KeyMap::modifier::RIGHTCTRL) != 0) && (SendModifier(type, enumModifier::RIGHTCTRL) == true)) {
                SendKey(type, KEY_RIGHTCTRL);
            }
        }

        void VirtualInput::DispatchRegisteredKey(uint32_t code)
        {
            if (_notifierMap.empty() == false) {

                _lock.Lock();

                NotifierMap::iterator it(_notifierMap.find(code));

                if (it != _notifierMap.end()) {
                    const NotifierList& notifierList = it->second;
                    for (Notifier* element : notifierList) {
                        element->Dispatch(code);
                    }
                }

                _lock.Unlock();
            }
        }

#if !defined(__WIN32__) && !defined(__APPLE__)

        LinuxKeyboardInput::LinuxKeyboardInput(const string& source, const string& inputName)
            : VirtualInput()
            , _eventDescriptor(-1)
            , _source(source)
            , _deviceKeys()
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
            Close();
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

        /* virtual */ void LinuxKeyboardInput::MapChanges(ChangeIterator& updated) {

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
                    (void) write(_eventDescriptor, &_uidev, sizeof(_uidev));
                    ioctl(_eventDescriptor, UI_DEV_CREATE);
                }
            }
            _lock.Unlock();
        }

        /* virtual */ void LinuxKeyboardInput::SendKey(const actiontype type, const uint32_t code)
        {
            if (_eventDescriptor > 0) {
                struct input_event ev;

                memset(&ev, 0, sizeof(ev));

                ev.type = ((type == COMPLETED) ? EV_SYN : EV_KEY);
                ev.value = ((type == COMPLETED) ? SYN_REPORT : ((type == PRESSED) ? 1 : (type == RELEASED ? 0 : 2)));
                ev.code = ((type == COMPLETED) ? 0 : code);

                TRACE_L1("Inserted a keycode: %d", code);
                (void)write(_eventDescriptor, &ev, sizeof(ev));
            }
        }

        bool LinuxKeyboardInput::Updated(ChangeIterator& updated)
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

        IPCKeyboardInput::IPCKeyboardInput(const Core::NodeId& sourceName)
            : _service(sourceName)
        {
            TRACE_L1("Constructing IPCKeyboardInput for %s on %s", sourceName.HostAddress().c_str(), sourceName.HostName().c_str());
        }

        /* virtual */ IPCKeyboardInput::~IPCKeyboardInput()
        {
        }

        /* virtual */ uint32_t IPCKeyboardInput::Open()
        {
            return (_service.Open(2000));
        }

        /* virtual */ uint32_t IPCKeyboardInput::Close()
        {
            _service.Close(2000);
            return (Core::ERROR_NONE);
        }

        /* virtual */ void IPCKeyboardInput::SendKey(const actiontype type, const uint32_t code)
        {
            static Core::ProxyType<KeyMessage> message(Core::ProxyType<KeyMessage>::Create());

            message->Parameters().Action = type;
            message->Parameters().Code = code;

            Core::ProxyType<Core::IIPC> base(Core::proxy_cast<Core::IIPC>(message));

            TRACE_L1("Sending keycode to all clients: %d", code);
            _service.Invoke(base, Core::infinite);
        }

        /* virtual */ void IPCKeyboardInput::MapChanges(ChangeIterator&) {}
    }
} // Namespace PluginHost
