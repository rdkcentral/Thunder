## Why

This user story was raised to allow plugins to add, update, and remove key-value
pairs in a `JSON::Container` at runtime. After peer review and analysis of the
existing codebase, the requirement is **withdrawn as a new API request** — the
capability already exists through direct member access and `VariantContainer`.

## Finding: No New API Required

### Typed Container — direct member access
A typed `Container` subclass declares its fields as **public member variables**.
Those members are live C++ objects that can be read and written directly at any
time. There is no need for a `Set(label, value)` method because the developer
already holds a typed reference to the field.

```cpp
struct PluginStatus : public Core::JSON::Container {
    PluginStatus() : state(), version(), activeClients(0) {
        Add(_T("state"),   &state);
        Add(_T("version"), &version);
        Add(_T("clients"), &activeClients);
    }
    Core::JSON::EnumType<PluginState> state;
    Core::JSON::String                version;
    Core::JSON::DecUInt32             activeClients;
};

PluginStatus status;

// Add / update fields — direct assignment, no helper needed.
status.state         = PluginState::ACTIVATED;
status.version       = _T("R4.4.7");
status.activeClients = 3;

// Remove a field from serialised output — call Clear() on the member.
status.version.Clear();   // IsSet()==false → omitted from ToString()
```

If a Container is declared with **private member variables**, the developer is
responsible for adding typed accessor methods — this is standard C++ practice
and requires no framework change.

### VariantContainer — existing `Set()`, `Get()`, `Remove()`, `operator[]`
`VariantContainer` (aliased as `JsonObject`) already provides a full dynamic
mutation API:

```cpp
JsonObject obj;

// Add
obj.Set(_T("model"),   JsonValue(string("ES1-A")));
obj.Set(_T("clients"), JsonValue(3));

// Update (same call — Set overwrites if key exists)
obj.Set(_T("model"), JsonValue(string("ES1-B")));

// Read
string model = obj.Get(_T("model")).String();    // "ES1-B"
JsonValue& ref = obj[_T("clients")];              // reference to live value
ref = JsonValue(5);                               // update in place

// Remove
obj.Remove(_T("clients"));
```

## Outcome

No code changes are required for this user story. The design document and task
list for this change are closed as **Not Needed — Existing Capability**.

The documentation in `docs/json-type-system.md` already covers both patterns
(typed Container direct access and VariantContainer mutation API).
