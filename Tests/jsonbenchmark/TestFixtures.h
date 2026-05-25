/*
 * Copyright 2024 Metrological
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

// ============================================================================
// TestFixtures.h
//
// All JSON test data (embedded strings + programmatic generators) used across
// every benchmark file, plus the Thunder Container schema types that correspond
// to each fixture.
//
// Fixture overview:
//   SMALL_CONFIG     ~230 bytes   Typical Thunder plugin configuration object
//   MEDIUM_RESPONSE  ~3.5 KB      JSON-RPC 2.0 response with 20 device records
//   LARGE_ARRAY      ~100 KB      1 000 device records (generated at first use)
//   DEEPLY_NESTED    ~500 bytes   30-level nesting depth
//   UNICODE_DATA     ~300 bytes   UTF-8 multi-byte characters + JSON escapes
//   REAL_WORLD       ~1 KB        Realistic WPEFramework controller status blob
// ============================================================================

#include <core/JSON.h>
#include <string>
#include <sstream>
#include <iomanip>

namespace JsonBenchmark {
namespace Fixtures {

// ============================================================================
// Raw JSON strings
// ============================================================================

// ~230 bytes — typical Thunder plugin config
inline const char* SMALL_CONFIG()
{
    return R"({"callsign":"TestPlugin","classname":"TestPlugin","autostart":true,"configuration":{"delay":5,"maxretries":3,"endpoint":"http://localhost:80","loglevel":2,"token":"abc123XYZ"}})";
}

// ~3.5 KB — JSON-RPC 2.0 response containing 20 network device records
inline const char* MEDIUM_RESPONSE()
{
    static std::string s;
    if (s.empty()) {
        std::ostringstream oss;
        oss << R"({"jsonrpc":"2.0","id":42,"result":{"status":0,"devices":[)";
        for (int i = 0; i < 20; ++i) {
            if (i) oss << ",";
            oss << R"({"id":)" << (i + 1)
                << R"(,"name":"eth)" << i << '"'
                << R"(,"mac":")" << std::hex << std::setfill('0')
                << std::setw(2) << i << ":11:22:33:44:" << std::setw(2) << (i + 1) << '"'
                << std::dec
                << R"(,"ip":"192.168.)" << (i / 10) << '.' << (i % 10 + 1) << '"'
                << R"(,"active":)" << (i % 3 != 0 ? "true" : "false")
                << R"(,"speed":)" << (i % 2 == 0 ? 1000 : 100)
                << R"(,"vendor":"VendorCorp)";
            oss << std::setfill('0') << std::setw(3) << (i + 1) << "\"}" ;
        }
        oss << R"(]}})";
        s = oss.str();
    }
    return s.c_str();
}

// ~100 KB — 1 000 device records (similar shape to MEDIUM, no JSON-RPC wrapper)
inline const std::string& LARGE_ARRAY()
{
    static std::string s;
    if (s.empty()) {
        std::ostringstream oss;
        oss << "[";
        for (int i = 0; i < 1000; ++i) {
            if (i) oss << ",";
            oss << R"({"id":)" << (i + 1)
                << R"(,"name":"Device)"
                << std::setfill('0') << std::setw(4) << (i + 1) << '"'
                << R"(,"mac":")" << std::hex << std::setfill('0')
                << std::setw(2) << (i & 0xFF) << ":22:33:44:55:"
                << std::setw(2) << ((i >> 8) & 0xFF) << '"'
                << std::dec
                << R"(,"ip":"10.)" << (i / 256) << '.' << (i % 256) << ".1\""
                << R"(,"active":)" << (i % 5 != 0 ? "true" : "false")
                << R"(,"speed":)" << ((i % 4 + 1) * 100)
                << R"(,"vendor":"Vendor)"
                << std::setfill('0') << std::setw(3) << (i % 50 + 1) << "\"}";
        }
        oss << "]";
        s = oss.str();
    }
    return s;
}

// ~500 bytes — 30 levels of object nesting
inline const char* DEEPLY_NESTED()
{
    static std::string s;
    if (s.empty()) {
        for (int i = 0; i < 30; ++i) {
            s += R"({"level)";
            s += std::to_string(i);
            s += R"(":)";
        }
        s += R"("deepvalue")";
        for (int i = 0; i < 30; ++i) {
            s += "}";
        }
    }
    return s.c_str();
}

// ~300 bytes — multi-byte UTF-8 characters and JSON escape sequences
inline const char* UNICODE_DATA()
{
    return u8R"({
        "greeting": "Hello, \u4e16\u754c!",
        "emoji_escaped": "\uD83D\uDE00",
        "tab_newline": "line1\nline2\ttabbed",
        "unicode_key_\u00e9": "caf\u00e9",
        "backslash": "path\\to\\file",
        "quotes": "she said \"hello\""
    })";
}

// ~1 KB — realistic WPEFramework controller status
inline const char* REAL_WORLD()
{
    return R"({
        "version": "5.0.0",
        "uptime": 86400,
        "framework": "Thunder",
        "plugins": [
            {"callsign":"Controller","state":"activated","classname":"Controller","locator":"","autostart":true},
            {"callsign":"DeviceInfo","state":"activated","classname":"DeviceInfo","locator":"libWPEFrameworkDeviceInfo.so","autostart":true},
            {"callsign":"Monitor","state":"suspended","classname":"Monitor","locator":"libWPEFrameworkMonitor.so","autostart":false},
            {"callsign":"Netflix","state":"deactivated","classname":"Netflix","locator":"libWPEFrameworkNetflix.so","autostart":false}
        ],
        "subsystems": {
            "platform":true,
            "network":true,
            "security":false,
            "graphics":true,
            "internet":false
        }
    })";
}

// ============================================================================
// Thunder Container schema types
//
// Thunder's JSON parser is schema-bound: you must define Container-derived
// types with registered fields before parsing.  This is in contrast to
// nlohmann/json and RapidJSON which can parse into a generic DOM first.
//
// The schema types below correspond to the fixtures above.
// ============================================================================

// --- PluginConfig (maps to SMALL_CONFIG) ------------------------------------
struct PluginInnerConfig : public Thunder::Core::JSON::Container {
    Thunder::Core::JSON::DecUInt32 Delay;
    Thunder::Core::JSON::DecUInt32 MaxRetries;
    Thunder::Core::JSON::String    Endpoint;
    Thunder::Core::JSON::DecUInt32 LogLevel;
    Thunder::Core::JSON::String    Token;

    PluginInnerConfig()
        : Delay(0)
        , MaxRetries(0)
        , LogLevel(0)
    {
        Add("delay",      &Delay);
        Add("maxretries", &MaxRetries);
        Add("endpoint",   &Endpoint);
        Add("loglevel",   &LogLevel);
        Add("token",      &Token);
    }
};

struct PluginConfig : public Thunder::Core::JSON::Container {
    Thunder::Core::JSON::String    Callsign;
    Thunder::Core::JSON::String    Classname;
    Thunder::Core::JSON::Boolean   AutoStart;
    PluginInnerConfig              Configuration;

    PluginConfig()
        : AutoStart(false)
    {
        Add("callsign",      &Callsign);
        Add("classname",     &Classname);
        Add("autostart",     &AutoStart);
        Add("configuration", &Configuration);
    }
};

// --- DeviceRecord (one element of MEDIUM_RESPONSE / LARGE_ARRAY) -----------
struct DeviceRecord : public Thunder::Core::JSON::Container {
    Thunder::Core::JSON::DecUInt32 Id;
    Thunder::Core::JSON::String    Name;
    Thunder::Core::JSON::String    Mac;
    Thunder::Core::JSON::String    Ip;
    Thunder::Core::JSON::Boolean   Active;
    Thunder::Core::JSON::DecUInt32 Speed;
    Thunder::Core::JSON::String    Vendor;

    DeviceRecord()
        : Id(0)
        , Active(false)
        , Speed(0)
    {
        Add("id",     &Id);
        Add("name",   &Name);
        Add("mac",    &Mac);
        Add("ip",     &Ip);
        Add("active", &Active);
        Add("speed",  &Speed);
        Add("vendor", &Vendor);
    }

    // Container deletes copy+move ctors; provide move so ArrayType<DeviceRecord>::Deserialize
    // can call push_back(ELEMENT()) on libc++ (which requires a move constructor).
    DeviceRecord(DeviceRecord&& other) noexcept
        : Thunder::Core::JSON::Container()
        , Id(std::move(other.Id))
        , Name(std::move(other.Name))
        , Mac(std::move(other.Mac))
        , Ip(std::move(other.Ip))
        , Active(std::move(other.Active))
        , Speed(std::move(other.Speed))
        , Vendor(std::move(other.Vendor))
    {
        Add("id",     &Id);
        Add("name",   &Name);
        Add("mac",    &Mac);
        Add("ip",     &Ip);
        Add("active", &Active);
        Add("speed",  &Speed);
        Add("vendor", &Vendor);
    }
};

using DeviceArray = Thunder::Core::JSON::ArrayType<DeviceRecord>;

struct DeviceListResult : public Thunder::Core::JSON::Container {
    Thunder::Core::JSON::DecUInt32 Status;
    DeviceArray                    Devices;

    DeviceListResult() : Status(0) {
        Add("status",  &Status);
        Add("devices", &Devices);
    }
};

struct JsonRpcDeviceResponse : public Thunder::Core::JSON::Container {
    Thunder::Core::JSON::String    JsonRpc;
    Thunder::Core::JSON::DecUInt32 Id;
    DeviceListResult               Result;

    JsonRpcDeviceResponse() : Id(0) {
        Add("jsonrpc", &JsonRpc);
        Add("id",      &Id);
        Add("result",  &Result);
    }
};

// --- PluginStatus (one element of REAL_WORLD plugins array) ----------------
struct PluginStatus : public Thunder::Core::JSON::Container {
    Thunder::Core::JSON::String  Callsign;
    Thunder::Core::JSON::String  State;
    Thunder::Core::JSON::String  Classname;
    Thunder::Core::JSON::String  Locator;
    Thunder::Core::JSON::Boolean AutoStart;

    PluginStatus() : AutoStart(false) {
        Add("callsign",  &Callsign);
        Add("state",     &State);
        Add("classname", &Classname);
        Add("locator",   &Locator);
        Add("autostart", &AutoStart);
    }

    // Container deletes copy+move ctors; provide move so ArrayType<PluginStatus>::Deserialize
    // can call push_back(ELEMENT()) on libc++.
    PluginStatus(PluginStatus&& other) noexcept
        : Thunder::Core::JSON::Container()
        , Callsign(std::move(other.Callsign))
        , State(std::move(other.State))
        , Classname(std::move(other.Classname))
        , Locator(std::move(other.Locator))
        , AutoStart(std::move(other.AutoStart))
    {
        Add("callsign",  &Callsign);
        Add("state",     &State);
        Add("classname", &Classname);
        Add("locator",   &Locator);
        Add("autostart", &AutoStart);
    }
};

using PluginArray = Thunder::Core::JSON::ArrayType<PluginStatus>;

struct SubSystems : public Thunder::Core::JSON::Container {
    Thunder::Core::JSON::Boolean Platform;
    Thunder::Core::JSON::Boolean Network;
    Thunder::Core::JSON::Boolean Security;
    Thunder::Core::JSON::Boolean Graphics;
    Thunder::Core::JSON::Boolean Internet;

    SubSystems()
        : Platform(false), Network(false), Security(false),
          Graphics(false), Internet(false)
    {
        Add("platform", &Platform);
        Add("network",  &Network);
        Add("security", &Security);
        Add("graphics", &Graphics);
        Add("internet", &Internet);
    }
};

struct ControllerStatus : public Thunder::Core::JSON::Container {
    Thunder::Core::JSON::String    Version;
    Thunder::Core::JSON::DecUInt32 Uptime;
    Thunder::Core::JSON::String    Framework;
    PluginArray                    Plugins;
    SubSystems                     Subsystems;

    ControllerStatus() : Uptime(0) {
        Add("version",    &Version);
        Add("uptime",     &Uptime);
        Add("framework",  &Framework);
        Add("plugins",    &Plugins);
        Add("subsystems", &Subsystems);
    }
};

} // namespace Fixtures
} // namespace JsonBenchmark
