#!/usr/bin/env python3

# If not stated otherwise in this file or this component's license file the
# following copyright and licenses apply:
#
# Copyright 2020 Metrological
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import argparse
import sys
import re
import os
import json
import posixpath
import urllib
import glob
import copy
import tempfile
from collections import OrderedDict
from enum import Enum

sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), os.pardir))

import ProxyStubGenerator.CppParser
import ProxyStubGenerator.Interface
import ProxyStubGenerator.Log as Log


NAME = "JsonGenerator"
DEFAULT_DEFINITIONS_FILE = "../ProxyStubGenerator/default.h"
FRAMEWORK_NAMESPACE = "WPEFramework"
INTERFACE_NAMESPACE = FRAMEWORK_NAMESPACE + "::Exchange"
INTERFACES_SECTION = True
INTERFACE_SOURCE_LOCATION = None
INTERFACE_SOURCE_REVISION = None
DEFAULT_INTERFACE_SOURCE_REVISION = "main"
GENERATED_JSON = False
SHOW_WARNINGS = True
DOC_ISSUES = True

class RpcFormat(Enum):
    COMPLIANT = "compliant"
    EXTENDED = "uncompliant-extended"
    COLLAPSED = "uncompliant-collapsed"

RPC_FORMAT = RpcFormat.COMPLIANT
RPC_FORMAT_FORCED = False

log = None

temp_files = []

try:
    import jsonref
except:
    log.Error("Install jsonref first")
    log.Print("e.g. try 'pip3 install jsonref'")
    sys.exit(1)

INDENT_SIZE = 4
ALWAYS_EMIT_COPY_CTOR = False
KEEP_EMPTY = False
CLASSNAME_FROM_REF = True
DEFAULT_EMPTY_STRING = ""
DEFAULT_INT_SIZE = 32
CPP_INTERFACE_PATH = "interfaces" + os.sep
JSON_INTERFACE_PATH = CPP_INTERFACE_PATH + "json"
DUMP_JSON = False
FORCE = False

GLOBAL_DEFINITIONS = "global.json"
DATA_NAMESPACE = "JsonData"
PLUGIN_NAMESPACE = "Plugin"
TYPE_PREFIX = "Core::JSON"
OBJECT_SUFFIX = "Data"
COMMON_OBJECT_SUFFIX = "Info"
ARRAY_SUFFIX = "Array"
ENUM_SUFFIX = "Type"
IMPL_ENDPOINT_PREFIX = "endpoint_"
IMPL_EVENT_PREFIX = "event_"


##############################################################################
#
# JSON SCHEMA PARSER
#

class JsonParseError(RuntimeError):
    pass


class CppParseError(RuntimeError):
    def __init__(self, obj, msg):
        try:
            msg = "%s(%s): %s (see '%s')" % (obj.parser_file, obj.parser_line, msg, obj.Proto())
            super(CppParseError, self).__init__(msg)
        except:
            super(CppParseError, self).__init__("unknown parsing failure: %s(%i): %s" % (obj.parser_file, obj.parser_line, msg))


def TypePrefix(type):
    return (TYPE_PREFIX + "::" + type)

def MakeObject(type):
    return (type + OBJECT_SUFFIX)

def MakeArray(type):
    return (type + ARRAY_SUFFIX)

def MakeEnum(type):
    return (type + ENUM_SUFFIX)


class JsonType():
    def __init__(self, name, parent, schema, included=None):
        is_generated = name.startswith("@_generated_")
        if is_generated:
            name = name.replace("@_generated_", "")
        self.new_name = None
        self.is_renamed = False
        self.name = name
        self.original_name = schema["original_name"] if "original_name" in schema else None
        self.schema = schema
        self.parent = parent
        self.description = schema["description"] if "description" in schema else None
        self.included_from = included
        self.is_duplicate = False
        self.iterator = schema["iterator"] if "iterator" in schema else None
        self.original_type = schema["original_type"] if "original_type" in schema else None
        self.do_create = (self.original_type == None)

        # Do some sanity check on the type name
        if parent:
            if not self.name.replace("_","").isalnum():
                raise JsonParseError("'%s': invalid characters in identifier name" % self.name)
            if not is_generated:
                if self.name[0] == "_":
                    raise JsonParseError("'%s': identifiers must not start with an underscore (reserved by the generator)" % self.name)
                if not self.name.islower():
                    log.Warn("'%s': mixedCase identifiers are supported, however all-lowercase names are recommended " % self.name)
                elif "_" in self.name:
                    log.Warn("'%s': snake_case identifiers are supported, however flatcase names are recommended " % self.name)
            if self.original_name: #identifier comming from C++ world
                if self.original_name[0] == "_":
                    raise JsonParseError("'%s': identifiers must not start with an underscore (reserved by the generator)" % self.original_name)
                elif "_" in self.original_name:
                    log.Warn("'%s': snake_case identifiers are supported, however mixedCase names are recommended " % self.original_name)

        # Do some sanity check on the description text
        if self.description and not isinstance(self, JsonMethod):
            if self.description.endswith("."):
                log.DocIssue("'%s': use sentence case capitalization and no period for parameter descriptions (\"%s\")" % (self.name, log.Ellipsis(self.description, False)))
            if self.description.endswith(" ") or self.description.startswith(" "):
                log.DocIssue("'%s': parameter description has leading or trailing whitespace" % self.name)
            if not self.description[0].isupper() and self.description[0].isalpha():
                log.DocIssue("'%s': use sentence case capitalization and no period for parameter descriptions (\"%s\")" % (self.name,log.Ellipsis(self.description)))

    def TempName(self, prefix = ""):
        return ("_" + prefix + self.json_name)

    def Rename(self, new_name):
        self.new_name = new_name.lower()
        self.is_renamed = True

    @property
    def objects(self):
        return []

    @property
    def properties(self):
        return []

    @property
    def json_name(self):
        if self.new_name:
            return self.new_name
        else:
            return self.name

    @property
    def local_name(self):
        if self.new_name:
            return self.new_name
        elif self.original_name:
            return self.original_name.lower()
        else:
            return self.name

    @property
    def actual_name(self):
        if self.new_name:
            return self.new_name
        elif self.original_name:
            return self.original_name
        else:
            return self.name

    @property
    def cpp_name(self): # C++ name of the object
        if self.new_name:
            return (self.new_name[0].upper() + self.new_name[1:])
        elif self.original_name:
            return (self.original_name[0].upper() + self.original_name[1:])
        else:
            return (self.name[0].upper() + self.name[1:])

    @property
    def cpp_type(self): # C++ type of the object (e.g. may be array)
        return self.cpp_class

    @property
    def cpp_class(self): # C++ class type of the object
        raise RuntimeError("can't instantiate %s" % self.name)

    @property
    def cpp_native_type(self):
        assert False, "cpp_native_type accessed on JsonType"

    @property
    def is_void(self):
        return (self.cpp_type == "void")

    @property
    def cpp_def_value(self): # Value to initialize with in C++
        return ""

    @property
    def root(self):
        return self.parent.root

    # Whether a copy constructor would be needed if this type is a member of a class
    @property
    def is_copy_ctor_needed(self):
        return False

class JsonNative:
    pass

class JsonNull(JsonNative, JsonType):
    @property
    def cpp_type(self):
        return self.cpp_native_type

    @property
    def cpp_native_type(self):
        return "void"

class JsonBoolean(JsonNative, JsonType):
    @property
    def cpp_class(self):
        return TypePrefix("Boolean")

    @property
    def cpp_native_type(self):
        return "bool"


class JsonNumber(JsonNative, JsonType):
    def __init__(self, name, parent, schema, size = DEFAULT_INT_SIZE, signed = False):
        JsonType.__init__(self, name, parent, schema)
        self.size = schema["size"] if "size" in schema else size
        self.signed = schema["signed"] if "signed" in schema else signed
        self._cpp_class = TypePrefix("Dec%sInt%i" % ("S" if self.signed else "U", self.size))
        self._cpp_native_type = "%sint%i_t" % ("" if self.signed else "u", self.size)

    @property
    def cpp_class(self):
        return self._cpp_class

    @property
    def cpp_native_type(self):
        return self._cpp_native_type


class JsonInteger(JsonNumber):
    # Identical as Number
    pass


class AuxJsonInteger(JsonInteger):
    def __init__(self, name, size = DEFAULT_INT_SIZE, signed = False):
        JsonInteger.__init__(self, "@_generated_" + name, None, {}, size, signed)


class JsonFloat(JsonNative, JsonType):
    @property
    def cpp_class(self):
        return TypePrefix("Float")

    @property
    def cpp_native_type(self):
        return "float"


class JsonDouble(JsonNative, JsonType):
    @property
    def cpp_class(self):
        return TypePrefix("Double")

    @property
    def cpp_native_type(self):
        return "double"


class JsonString(JsonNative, JsonType):
    @property
    def cpp_class(self):
        return TypePrefix("String")

    @property
    def cpp_native_type(self):
        return "string"


class JsonEnum(JsonType):
    def __init__(self, name, parent, schema, enumType, included=None):
        JsonType.__init__(self, name, parent, schema, included)
        self.print_name = parent.print_name + "/" + name
        if enumType != "string":
            raise JsonParseError("Only strings are supported in enums")
        self.type = enumType
        if self.do_create:
            self.enum_name = MakeEnum(self.original_name.capitalize() if self.original_name else self.name.capitalize())
        self.enumerators = schema["enum"]
        self.cpp_enumerator_values = schema["values"] if "values" in schema else []
        if not self.cpp_enumerator_values:
            self.cpp_enumerator_values = schema["enumvalues"] if "enumvalues" in schema else []
        if self.cpp_enumerator_values:
            same = True
            biggest = 0;
            for idx, e in enumerate(self.cpp_enumerator_values):
                if idx != e:
                    same = False
                if e > biggest:
                    biggest = e
            if same:
                log.Warn("'%s': specified enum values are same as automatic" % name)
                self.cpp_enumerator_values = []
            self.size = 32 if biggest > 65536 else 16 if biggest > 256 else 8
        else:
            self.size = 16 if len(self.enumerators) > 256 else 8
        if "size" in schema and isinstance(schema["size"], int):
            self.size = schema["size"]
        if self.size not in [8, 16, 32, 64]:
            raise JsonParseError("Invalid enum size value (%i)" % self.size)
        self.cpp_enumerators = []
        if "enumids" in self.schema:
            self.cpp_enumerators = self.schema["enumids"]
        elif "ids" in self.schema:
            self.cpp_enumerators = self.schema["ids"]
        else:
            if "case" in self.schema and self.schema["case"] == "snake" :
                self.cpp_enumerators = list(map(lambda x: re.sub(r'(?<!^)(?=[A-Z])', '_', x).upper(), self.enumerators))
            else:
                self.cpp_enumerators = list(map(lambda x: ("E" if x[0].isdigit() else "") + x.upper(), self.enumerators))
        if self.cpp_enumerator_values and (len(self.enumerators) != len(self.cpp_enumerator_values)):
            raise JsonParseError("Mismatch in enumeration values in enum '%s'" % self.json_name)
        self.is_scoped = schema["scoped"] if "scoped" in schema else schema["enumtyped"] if "enumtyped" in schema else True
        self.ref_destination = None
        self.refs = []
        self.AddRef(self)
        obj = enumTracker.Add(self)
        if obj:
            self.is_duplicate = True
            self.ref_destination = obj
            if ((obj.parent != self.parent) and not IsInCustomRef(self)):
                obj.AddRef(self)

    def AddRef(self, obj):
        self.refs.append(obj)

    def RefCount(self):
        return len(self.refs)

    @property
    def cpp_type(self):
        return TypePrefix("EnumType<%s>" % self.cpp_class)

    @property
    def cpp_class(self):
        if self.is_duplicate:
            # Use the original (ie. first seen) name
            return self.ref_destination.cpp_class
        else:
            classname = ""
            if not self.do_create:
                # Override with real class name, this is likely comming from C++ header
                classname = self.original_type
            elif "class" in self.schema:
                # Override class name if "class" property present
                classname = self.schema["class"].capitalize
            elif "hint" in self.schema:
                # Override class name if "hint" property present
                classname = self.schema["hint"]
            elif CLASSNAME_FROM_REF and ("@ref" in self.schema):
                # NOTE: Abuse the ref feature to construct a name for the enum!
                classname = MakeEnum(self.schema["@ref"].rsplit(posixpath.sep, 1)[1].capitalize())
            elif isinstance(self.parent, JsonProperty):
                classname = MakeEnum(self.parent.name.capitalize())
            else:
                classname = self.enum_name
            return classname

    @property
    def cpp_native_type(self):
        return self.cpp_class

class JsonObject(JsonType):
    def __init__(self, name, parent, schema, print_name=None, included=None):
        self.print_name = parent.print_name + "/" + name
        JsonType.__init__(self, name, parent, schema, included)
        self._properties = []
        self._objects = []
        self._enums = []
        self.ref_destination = None
        self.refs = []
        self.AddRef(self)

        # Handle duplicate objects...
        if "properties" in schema:
            obj = objTracker.Add(self)
            if obj:
                if "original_type" in self.schema and "original_type" not in obj.schema:
                    # Very unlucky scenario when duplicate class carries more information than the original.
                    # Swap the classes in duplicate lists so we take the better one for generating code.
                    objTracker.Remove(obj)
                    objTracker.Remove(self)
                    objTracker.Add(self)
                    self.refs += obj.refs[1:]
                    obj.is_duplicate = True
                    obj.ref_destination = self
                    obj.refs = [obj]
                    objTracker.Add(obj)
                    if obj.parent != self.parent and not IsInCustomRef(obj):
                        self.AddRef(obj)
                else:
                    self.is_duplicate = True
                    self.ref_destination = obj
                    if obj.parent != self.parent and not IsInCustomRef(self):
                        obj.AddRef(self)

            for prop_name, prop in schema["properties"].items():
                new_obj = JsonItem(prop_name, self, prop, included=included)
                self._properties.append(new_obj)
                # Handle aggregate objects
                if isinstance(new_obj, JsonObject):
                    self._objects.append(new_obj)
                elif isinstance(new_obj, JsonArray):
                    # Also add nested objects within arrays
                    o = new_obj.items
                    while isinstance(o, JsonArray):
                        o = o.items
                    if isinstance(o, JsonObject):
                        self._objects.append(o)
                    if isinstance(o, JsonEnum):
                        self._enums.append(new_obj.items)
                elif isinstance(new_obj, JsonEnum):
                    self._enums.append(new_obj)

        if not self.properties:
            log.Info("No properties in object %s" % self.print_name)

    def AddRef(self, obj):
        self.refs.append(obj)

    def RefCount(self):
        return len(self.refs)

    @property
    def cpp_name(self):
        # NOTE: Special cases for names for Methods and Arrays
        if self.is_renamed:
            return super().cpp_name
        elif isinstance(self.parent, JsonMethod):
            return self.parent.cpp_name + super().cpp_name
        elif isinstance(self.parent, JsonArray):
            return self.parent.cpp_name
        else:
            return super().cpp_name

    @property
    def cpp_class(self):
        if self.is_duplicate:
            # Use the original (ie. first seen) class name
            return self.ref_destination.cpp_class
        else:
            classname = ""
            if "class" in self.schema:
                # Override class name if "class" property present
                classname = self.schema["class"].capitalize()
            elif "hint" in self.schema:
                # Override class name if "class" property present
                classname = self.schema["hint"]
            else:
                if not self.properties:
                    return TypePrefix("Container")
                elif CLASSNAME_FROM_REF and ("@ref" in self.schema):
                    classname = MakeObject(self.schema["@ref"].rsplit(posixpath.sep, 1)[1].capitalize())
                else:
                    # Make the name out of properties, but not for params/result types
                    if self.original_type:
                        classonly = self.original_type.split("::")[-1]
                        classname = MakeObject(classonly[0].upper() + classonly[1:])
                    elif len(self.properties) == 1 and not isinstance(self.parent, JsonMethod):
                        classname = MakeObject(self.properties[0].cpp_name)
                    elif isinstance(self.parent, JsonProperty):
                        classname = MakeObject(self.parent.cpp_name)
                    elif self.root.rpc_format != RpcFormat.COMPLIANT and isinstance(self.parent, JsonArray) and isinstance(self.parent.parent, JsonProperty):
                        classname = MakeObject(self.parent.parent.cpp_name)
                    elif self.root.rpc_format == RpcFormat.COMPLIANT and isinstance(self.parent, JsonArray):
                        classname = (MakeObject(self.parent.cpp_name) + "Elem")
                    else:
                        classname = MakeObject(self.cpp_name)
                # For common classes append special suffix
                if self.RefCount() > 1:
                    classname = classname.replace(OBJECT_SUFFIX, COMMON_OBJECT_SUFFIX)
            return classname

    @property
    def cpp_native_type(self):
        if not self.do_create:
            return self.original_type
        else:
            return self.cpp_class

    @property
    def json_name(self):
        return self.name

    @property
    def properties(self):
        return self._properties

    @property
    def objects(self):
        return self._objects

    @property
    def enums(self):
        return self._enums

    @property
    def is_copy_ctor_needed(self):
        if ALWAYS_EMIT_COPY_CTOR or self.parent.is_copy_ctor_needed:
            return True
        # Check if a copy constructor is needed by scanning all duplicate classes
        for obj in self.refs:
            if (obj != self) and obj.parent.is_copy_ctor_needed:
                return True
        return False


class JsonArray(JsonType):
    def __init__(self, name, parent, schema, print_name=None, included=None):
        JsonType.__init__(self, name, parent, schema, included)
        self.print_name = parent.print_name = "/{array}"
        self._items = None
        if "items" in schema:
            if "class" in schema["items"]:
                name = "@_generated_" + schema["items"]["class"] + "Element"
            self._items = JsonItem(name, self, schema["items"], print_name, included)
        else:
            raise JsonParseError("no items in array '%s'" % name)

        self.is_duplicate = self.items.is_duplicate

    def RefCount(self):
        return self.items.RefCount()

    @property
    def cpp_name(self):
        if self.is_renamed:
            return super().cpp_name
        # Take the name of the array from the method if this array is result type
        elif isinstance(self.parent, JsonMethod):
            return self.parent.cpp_name + super().cpp_name
        else:
            return super().cpp_name

    @property
    def is_copy_ctor_needed(self):
        # Important
        return True

    @property
    def items(self):
        return self._items

    # Delegate all other methods to the underlying type
    @property
    def cpp_type(self):
        return TypePrefix("ArrayType<%s>" % self._items.cpp_type)

    @property
    def cpp_class(self):
        return self.items.cpp_class

    @property
    def cpp_native_type(self):
        return "/* TODO */"

    @property
    def enums(self):
        return self.items.enums

    @property
    def objects(self):
        return self.items.objects

    @property
    def properties(self):
        return self.items.properties


class JsonMethod(JsonObject):
    def __init__(self, name, parent, schema, included=None, property=False):
        if '.' in name:
            log.Warn("'%s': method names containing full designator are deprecated (include name only)" % name)
            name = name.rsplit(".", 1)[1]
        elif "original_name" in schema:
            # In case of a method always take the original name if available
            name = schema["original_name"].lower()

        # Mimic a JSON object to fit rest of the parsing...
        self.errors = schema["errors"] if "errors" in schema else OrderedDict()
        props = OrderedDict()
        props["params"] = schema["params"] if "params" in schema else {"type": "null"}
        props["result"] = schema["result"] if "result" in schema else {"type": "null"}
        method_schema = {"type": "object", "properties": props}
        if "original_name" in schema:
            method_schema["original_name"] = schema["original_name"]

        JsonObject.__init__(self, name, parent, method_schema, included=included)

        self.summary = schema["summary"] if "summary" in schema else None
        self.deprecated = "deprecated" in schema and schema["deprecated"];
        self.obsolete = "obsolete" in schema and schema["obsolete"];
        self.endpoint_name = (IMPL_ENDPOINT_PREFIX + super().json_name)

        if (self.rpc_format == RpcFormat.COMPLIANT) and not isinstance(self.params, (JsonObject, JsonNull)):
            raise RuntimeError("In 'compliant' format parameters to a method or event need to be an object: '%s'" % self.json_name)
        elif (self.rpc_format == RpcFormat.EXTENDED) and not property and not isinstance(self.params, (JsonObject, JsonArray, JsonNull)):
            raise RuntimeError("In 'extended' format parameters to a method or event need to be an object or an array: '%s'" % self.json_name)
        elif (self.rpc_format == RpcFormat.COLLAPSED) and isinstance(self.params, JsonObject) and (len(self.params.properties) == 1):
            log.Warn("'%s': in 'collapsed' format methods and events with one parameter ought not to have an outer object" % self.json_name)

    @property
    def rpc_format(self):
        return self.root.rpc_format

    @property
    def params(self):
        return self.properties[0]

    @property
    def result(self):
        return self.properties[1]

    def Headline(self):
        return "%s%s%s" % (self.json_name, (" - " + self.summary.split(".", 1)[0]) if self.summary else "",
                           " (DEPRECATED)" if self.deprecated else " (OBSOLETE)" if self.obsolete else "")


class JsonNotification(JsonMethod):
    def __init__(self, name, parent, schema, included=None):
        JsonMethod.__init__(self, name, parent, schema, included)
        if "id" in schema and "type" not in schema["id"]:
            schema["id"]["type"] = "string"
        self.sendif_type = JsonItem("id", self, schema["id"]) if "id" in schema else None
        self.is_status_listener = schema["statuslistener"] if "statuslistener" in schema else False
        self.endpoint_name = (IMPL_EVENT_PREFIX + self.json_name)
        for param in self.params.properties:
            if not isinstance(param,JsonNative) and param.do_create:
                log.Info("'%s': notification parameter '%s' refers to generated JSON objects" % (name, param.name))
                break


class JsonProperty(JsonMethod):
    def __init__(self, name, parent, schema, included=None):
        self.readonly = "readonly" in schema and schema["readonly"]
        self.writeonly = "writeonly" in schema and schema["writeonly"]
        if ("params" not in schema) and ("result" not in schema):
            raise RuntimeError("No parameters defined for property: %s" % name)
        if ("index" in schema) and ("type" not in schema["index"]):
            schema["index"]["type"] = "string"
        JsonMethod.__init__(self, name, parent, schema, included, property=True)
        self.index = JsonItem("index", self, schema["index"]) if "index" in schema else None
        self.endpoint_set_name = (IMPL_ENDPOINT_PREFIX + "set_" + self.json_name)
        self.endpoint_get_name = (IMPL_ENDPOINT_PREFIX + "get_" + self.json_name)


class JsonRpcSchema(JsonType):
    def __init__(self, name, schema):
        JsonType.__init__(self, name, None, schema)
        self.info = None
        self.base_schema = None
        self.jsonrpc_version = None
        self.methods = []
        self.includes = []
        self.rpc_format = RPC_FORMAT
        self.print_name = ""
        if "interface" in schema:
            schema = schema["interface"]
        if "$schema" in schema:
            self.base_schema = schema["$schema"]
        if "jsonrpc" in schema:
            self.jsonrpc_version = schema["jsonrpc"]
        if "info" in schema:
            self.info = schema["info"]
            if "format" in self.info and not RPC_FORMAT_FORCED:
                self.rpc_format = RpcFormat(self.info["format"])
        if "include" in schema:
            for name, included_schema in schema["include"].items():
                include = included_schema["info"]["class"]
                self.includes.append(include)
                if "methods" in included_schema:
                    for name, method in included_schema["methods"].items():
                        self.methods.append(JsonMethod(name, self, method, include))
                if "properties" in included_schema:
                    for name, method in included_schema["properties"].items():
                        self.methods.append(JsonProperty(name, self, method, include))
                if "events" in included_schema:
                    for name, method in included_schema["events"].items():
                        self.methods.append(JsonNotification(name, self, method, include))

        method_list = list(map(lambda x: x.name, self.methods))

        def _AddMethods(section, schema, ctor):
            if section in schema:
                for name, method in schema[section].items():
                    if name in method_list:
                        del self.methods[method_list.index(name)]
                        method_list.remove(name)
                    if method != None:
                        self.methods.append(ctor(name, self, method))

        _AddMethods("methods", schema, lambda name, obj, method: JsonMethod(name, obj, method))
        _AddMethods("properties", schema, lambda name, obj, method: JsonProperty(name, obj, method))
        _AddMethods("events", schema, lambda name, obj, method: JsonNotification(name, obj, method))

        if not self.methods:
            raise JsonParseError("no methods, properties or events defined in %s" % name)

    @property
    def root(self):
        return self

    @property
    def cpp_class(self):
        return super().cpp_name

    @property
    def properties(self):
        return self.methods

    @property
    def objects(self):
        return self.properties

    def RefCount(self):
        return 1


def JsonItem(name, parent, schema, print_name=None, included=None):
    # Create the appropriate Python object based on the JSON type
    if "type" in schema:
        if schema["type"] == "object":
            return JsonObject(name, parent, schema, print_name, included)
        elif schema["type"] == "array":
            return JsonArray(name, parent, schema, print_name, included)
        elif schema["type"] == "null":
            return JsonNull(name, parent, schema)
        elif schema["type"] == "boolean":
            return JsonBoolean(name, parent, schema)
        elif "enum" in schema:
            return JsonEnum(name, parent, schema, schema["type"], included)
        elif schema["type"] == "string":
            return JsonString(name, parent, schema)
        elif schema["type"] == "integer":
            return JsonInteger(name, parent, schema)
        elif schema["type"] == "number":
            return JsonNumber(name, parent, schema)
        elif schema["type"] == "float":
            return JsonFloat(name, parent, schema)
        elif schema["type"] == "double":
            return JsonDouble(name, parent, schema)
        else:
            raise JsonParseError("unsupported JSON type: %s" % schema["type"])
    else:
        raise JsonParseError("undefined type for item: %s" % name)


def LoadSchema(file, include_path, cpp_include_path, header_include_paths):
    additional_includes = []

    def Adjust(schema):
        def AdjustByFormat(schema):
            # Fixes properties to conform to params/result scheme
            # Also automatically enclose params in object for COMPLIANT mode
            def AdjustProperties(schema, rpc_format):
                if "include" in schema:
                    for _, prop in schema["include"].items():
                        AdjustProperties(prop, rpc_format)
                if "properties" in schema:
                    for _, prop in schema["properties"].items():
                        if "params" in prop:
                            if (rpc_format == RpcFormat.COMPLIANT) and ("type" in prop["params"]) and (prop["params"]["type"] != "object") \
                                    and ("readonly" not in prop or not prop["readonly"]):
                                prop["params"] = { "type": "object", "properties" : { "value" : prop["params"] } }
                            if ("result" not in prop) and ("writeonly" not in prop or not prop["writeonly"]):
                                if "readonly" in prop:
                                    prop["result"] = prop["params"]
                                    del prop["params"]
                                else:
                                    prop["result"] = copy.deepcopy(prop["params"])
                                    prop["result"]["@ref"] = "#../params"

            rpc_format = RPC_FORMAT
            if (not RPC_FORMAT_FORCED) and ("info" in schema) and ("format" in schema["info"]):
                rpc_format = RpcFormat(schema["info"]["format"])

            AdjustProperties(schema, rpc_format)

        AdjustByFormat(schema)

        if "interface" in schema:
            if isinstance(schema["interface"], list):
                for face in schema["interface"]:
                    AdjustByFormat(face)
            else:
                AdjustByFormat(schema["interface"])

    def MarkRefs(schema, parent_name, parent, root, idx=None):
        def _Find(element, root):
            paths = element.split("/")
            data = root
            for i in range(0, len(paths)):
                if paths[i] and paths[i] != '#':
                    if paths[i] in data:
                        data = data[paths[i]]
                    else:
                        return None
            return data

        def _FindCpp(element, schema):
            if isinstance(schema, OrderedDict):
                if "original_type" in schema:
                    if schema["original_type"] == element:
                        return schema
                for _, item in schema.items():
                    found = _FindCpp(element, item)
                    if found:
                        return found
            return None

        # Tags all objects that used to be $references
        if isinstance(schema, jsonref.JsonRef):
            if "description" in schema.__reference__ or "example" in schema.__reference__ or "default" in schema.__reference__:
                # Need a copy, there an override on one of the properites
                if idx == None:
                    parent[parent_name] = copy.deepcopy(schema)
                    new_schema = parent[parent_name]
                else:
                    parent[parent_name][idx] = copy.deepcopy(schema)
                    new_schema = parent[parent_name][idx]
                new_schema["@ref"] = schema.__reference__["$ref"]
                if "description" in schema.__reference__:
                    new_schema["description"] = schema.__reference__["description"]
                if "example" in schema.__reference__:
                    new_schema["example"] = schema.__reference__["example"]
                if "default" in schema.__reference__:
                    new_schema["default"] = schema.__reference__["default"]
                schema = new_schema
            else: #isinstance(schema.__reference__["$ref"], dict):
                schema["@ref"] = schema.__reference__["$ref"]

        if isinstance(schema, OrderedDict):
            for elem, item in schema.items():
                if isinstance(item, list):
                    for i, e in enumerate(item):
                        MarkRefs(e, elem, schema, root, i)
                else:
                    MarkRefs(item, elem, schema, root, None)
            if "@dataref" in schema:
                splitted_path = schema["@dataref"].split('/')
                json_path = '/'.join(splitted_path[:-1])
                cpp_path = splitted_path[-1]
                obj = _Find(json_path, root)
                if not obj and "interface" in root:
                    obj = _Find(json_path, root["interface"])
                if not obj and "interfaces" in root:
                    obj = _Find(json_path, root["interfaces"])
                if obj:
                    found = False
                    for o in obj:
                        if cpp_path.startswith(FRAMEWORK_NAMESPACE + "::"):
                            cpp_path = cpp_path.replace(FRAMEWORK_NAMESPACE + "::","")
                        cpp_obj = _FindCpp(cpp_path, o)
                        if cpp_obj:
                            parent[parent_name] = copy.deepcopy(cpp_obj)
                            parent[parent_name]["@ref"] = "@" + json_path + "/" + cpp_obj["original_name"]
                            if "description" in schema:
                                parent[parent_name]["description"] = schema["description"]
                            if "example" in schema:
                                parent[parent_name]["example"] = schema["example"]
                            if "default" in schema:
                                parent[parent_name]["default"] = schema["default"]
                            found = True
                            break
                    if not found:
                        raise RuntimeError("Failed to find $ref path '%s' (C++ part)" % schema["@dataref"])
                else:
                    raise RuntimeError("Failed to find $ref path '%s' (JSON part)" % schema["@dataref"])

    with open(file, "r") as json_file:
        def Preprocess(pairs):
            def Scan(pairs):
                for i, c in enumerate(pairs):
                    if isinstance(c, tuple):
                        k = c[0]
                        v = c[1]
                        if isinstance(v, list):
                            Scan(v)
                        elif k == "$ref":
                            if ".json" in v:
                                # Need to prepend with 'file:' for jsonref to load an external file..
                                ref = v.split("#") if "#" in v else [v,""]
                                if ("{interfacedir}" in ref[0]) and include_path:
                                    ref_file = ref[0].replace("{interfacedir}", include_path)
                                    if os.path.exists(ref_file):
                                        pairs[i] = (k, ("file:" + ref_file + "#" + ref[1]))
                                    else:
                                        raise RuntimeError("$ref file %s not found" % ref_file)
                                else:
                                    ref_file = os.path.abspath(os.path.dirname(file)) + os.sep + ref[0]
                                    if not os.path.exists(ref_file):
                                        ref_file = v
                                    else:
                                        pairs[i] = (k, ("file:" + ref_file + "#" + ref[1]))
                            elif v.endswith(".h") or v.endswith(".h#"):
                                ref = v.replace("#", "")
                                if ("{interfacedir}" in ref or "{cppinterfacedir}" in ref) and cpp_include_path:
                                    ref_file = ref.replace("{interfacedir}", cpp_include_path).replace("{cppinterfacedir}", cpp_include_path)
                                    if os.path.exists(ref_file):
                                        cppif, _ = LoadInterface(ref_file, True, header_include_paths)
                                        if cppif:
                                            if ref_file not in additional_includes:
                                                additional_includes.append(ref_file)
                                            with tempfile.NamedTemporaryFile(mode="w", delete=False) as temp_json_file:
                                                temp_json_file.write(json.dumps(cppif))
                                                pairs[i] = (k, "file:" + temp_json_file.name)
                                                temp_files.append(temp_json_file.name)
                                    else:
                                        raise RuntimeError("$ref file %s not found" % ref_file)
                            elif "::" in v:
                                pairs[i] = ("@dataref", v)

            Scan(pairs)
            d = OrderedDict()
            for k,v in pairs:
                d[k] = v
            return d

        #json_pre = PreprocessJson(file, json_file.read(), include_path, cpp_include_path, header_include_paths)
        json_resolved = jsonref.loads(json_file.read(), object_pairs_hook=Preprocess)
        Adjust(json_resolved)
        MarkRefs(json_resolved, None, None, json_resolved)
        return [json_resolved], additional_includes

    return [], []


########################################################
#
# C++ HEADER TO JSON CONVERTER
#

def LoadInterface(file, all = False, includePaths = []):
    tree = ProxyStubGenerator.CppParser.ParseFiles([os.path.join(os.path.dirname(os.path.realpath(__file__)),
                posixpath.normpath(DEFAULT_DEFINITIONS_FILE)), file], includePaths, log)
    interfaces = [i for i in ProxyStubGenerator.Interface.FindInterfaceClasses(tree, INTERFACE_NAMESPACE, file) if (i.obj.is_json or (all and not i.obj.is_event))]

    def Build(face):
        def _EvaluateRpcFormat(obj):
            rpc_format = RPC_FORMAT
            if not RPC_FORMAT_FORCED:
                if obj.is_collapsed:
                    rpc_format = RpcFormat.COLLAPSED
                elif obj.is_extended:
                    rpc_format = RpcFormat.EXTENDED
                elif obj.is_compliant:
                    rpc_format = RpcFormat.COMPLIANT
            return rpc_format

        schema = OrderedDict()
        methods = OrderedDict()
        properties = OrderedDict()
        events = OrderedDict()

        schema["$schema"] = "interface.json.schema"
        schema["jsonrpc"] = "2.0"
        schema["@generated"] = True
        if face.obj.is_json:
            schema["@dorpc"] = True
            rpc_format = _EvaluateRpcFormat(face.obj)
        else:
            rpc_format = RpcFormat.COLLAPSED

        verify = face.obj.is_json or face.obj.is_event

        schema["@interfaceonly"] = True
        schema["configuration"] = { "nodefault" : True }


        info = dict()
        info["format"] = rpc_format.value
        if not face.obj.parent.full_name.endswith(INTERFACE_NAMESPACE):
            info["namespace"] = face.obj.parent.name
        info["class"] = face.obj.name[1:] if face.obj.name[0] == "I" else face.obj.name
        scoped_face = face.obj.full_name.split("::")[1:]
        if scoped_face[0] == FRAMEWORK_NAMESPACE:
            scoped_face = scoped_face[1:]
        info["interface"] = "::".join(scoped_face)
        info["sourcefile"] = os.path.basename(file)
        if face.obj.sourcelocation:
            info["sourcelocation"] = face.obj.sourcelocation
        if face.obj.json_version:
            try:
                info["version"] = [int(x) for x in face.obj.json_version.split(".")]
            except:
                raise CppParseError(face.obj, "Interface version must be provided in major[.minor[.patch]] format")

        info["title"] = info["class"] + " API"
        info["description"] = info["class"] + " JSON-RPC interface"
        schema["info"] = info

        event_interfaces = set()

        for method in face.obj.methods:

            def ResolveTypedef(type):
                return type.Resolve()

            def StripFrameworkNamespace(identifier):
                return str(identifier).replace("::" + FRAMEWORK_NAMESPACE + "::", "")

            def StripInterfaceNamespace(identifier):
                return str(identifier).replace(INTERFACE_NAMESPACE + "::", "").replace("::" + FRAMEWORK_NAMESPACE + "::", "")

            def ConvertType(var):
                var_type = ResolveTypedef(var.type)
                cppType = var_type.Type()
                is_iterator = (isinstance(cppType, ProxyStubGenerator.CppParser.Class) and cppType.is_iterator)
                # Pointers
                if var_type.IsPointer() and (is_iterator or (var.meta.length and var.meta.length != ["void"])):
                    # Special case for serializing C-style buffers, that will be converted to base64 encoded strings
                    if isinstance(cppType, ProxyStubGenerator.CppParser.Integer) and cppType.size == "char":
                        props = dict()
                        if var.meta.maxlength:
                            props["length"] = " ".join(var.meta.maxlength)
                        elif var.meta.length:
                            props["length"] = " ".join(var.meta.length)
                        if "length" in props:
                            props["cpptype"] = cppType.type
                        props["encode"] = cppType.type != "char"
                        return "string", props if props else None
                    # Special case for iterators, that will be converted to JSON arrays
                    elif is_iterator and len(cppType.args) == 2:
                        # Take element type from return value of the Current() method
                        currentMethod = next((x for x in cppType.methods if x.name == "Current"), None)
                        if currentMethod == None:
                            raise CppParseError(var, "%s does not appear to a be an @iterator type" % cppType.type)
                        result = ["array", { "items": ConvertParameter(currentMethod.retval), "iterator": StripInterfaceNamespace(cppType.type) } ]
                        if var_type.IsPointerToPointer():
                            result[1]["ptr"] = True
                        if var_type.IsPointerToConst():
                            raise CppParseError(var, "passed iterators must not be constant")
                        return result
                    # All other pointer types are not supported
                    else:
                        raise CppParseError(var, "unable to convert C++ type to JSON type: %s - input pointer allowed only on interator or C-style buffer" % cppType.type)
                # Primitives
                else:
                    result = None
                    # String
                    if isinstance(cppType, ProxyStubGenerator.CppParser.String):
                        result = [ "string", {} ]
                    # Boolean
                    elif isinstance(cppType, ProxyStubGenerator.CppParser.Bool):
                        result = ["boolean", {} ]
                    # Integer
                    elif isinstance(cppType, ProxyStubGenerator.CppParser.Integer):
                        size = 8 if cppType.size == "char" else 16 if cppType.size == "short" else \
                            32 if cppType.size == "int" or cppType.size == "long" else 64 if cppType.size == "long long" else 32
                        result = [ "integer", { "size": size, "signed": cppType.signed } ]
                    # Float
                    elif isinstance(cppType, ProxyStubGenerator.CppParser.Float):
                        result = [ "float", { "size": 32 if cppType.type == "float" else 64 if cppType.type == "double" else 128 } ]
                    # Null
                    elif isinstance(cppType, ProxyStubGenerator.CppParser.Void):
                        result = [ "null", {} ]
                    # Enums
                    elif isinstance(cppType, ProxyStubGenerator.CppParser.Enum):
                        if len(cppType.items) > 1:
                            enumValues = [e.autoValue for e in cppType.items]
                            for i, e in enumerate(cppType.items, 0):
                                if enumValues[i - 1] != enumValues[i]:
                                    raise CppParseError(var, "enumerator values in an enum must all be explicit or all be implied")
                        enumSpec = { "enum": [e.meta.text if e.meta.text else e.name.replace("_"," ").title().replace(" ","") for e in cppType.items], "scoped": var.type.Type().scoped  }
                        enumSpec["ids"] = [e.name for e in cppType.items]
                        enumSpec["hint"] = var.type.Type().name
                        if not cppType.items[0].autoValue:
                            enumSpec["values"] = [e.value for e in cppType.items]
                        result = [ "string", enumSpec ]
                    # POD objects
                    elif isinstance(cppType, ProxyStubGenerator.CppParser.Class):
                        def GenerateObject(ctype):
                            properties = dict()
                            for p in ctype.vars:
                                name = p.name.lower()
                                if isinstance(ResolveTypedef(p.type).Type(), ProxyStubGenerator.CppParser.Class):
                                    _, props = GenerateObject(ResolveTypedef(p.type).Type())
                                    properties[name] = props
                                    properties[name]["type"] = "object"
                                    properties[name]["original_type"] = StripFrameworkNamespace(p.type.Type().full_name)
                                else:
                                    properties[name] = ConvertParameter(p)
                                properties[name]["original_name"] = p.name
                            return "object", { "properties": properties, "required": list(properties.keys()) }
                        result = GenerateObject(cppType)
                    # All other types are not supported
                    else:
                        raise CppParseError(var, "unable to convert C++ type to JSON type: %s" % cppType.type)
                    if var_type.IsPointer():
                        result[1]["ptr"] = True
                    if var_type.IsReference():
                        result[1]["ref"] = True
                    if var_type.IsPointerToConst():
                        result[1]["ptrtoconst"] = True
                    return result

            def ExtractExample(var):
                egidx = var.meta.brief.index("(e.g.") if "(e.g." in var.meta.brief else None
                if egidx != None and ")" in var.meta.brief[egidx + 1:]:
                    example = var.meta.brief[egidx + 5:var.meta.brief.rfind(")")].strip()
                    description = var.meta.brief[0:egidx].strip()
                    return [example, description]
                else:
                    return None

            def ConvertParameter(var):
                jsonType, args = ConvertType(var)
                properties = {"type": jsonType}
                if args != None:
                    properties.update(args)
                    try:
                        properties["original_type"] = StripFrameworkNamespace(var.type.Type().full_name)
                    except:
                        pass
                if var.meta.brief:
                    # Also attempt to craft some description
                    pair = ExtractExample(var)
                    if pair:
                        properties["example"] = pair[0]
                        properties["description"] = pair[1]
                    else:
                        properties["description"] = var.meta.brief
                return properties

            def EventParameters(vars):
                events = []
                for var in vars:
                    def ResolveTypedef(resolved, events, type):
                        if not isinstance(type, list):
                            if isinstance(type.Type(), ProxyStubGenerator.CppParser.Typedef):
                                if type.Type().is_event:
                                    events.append(type)
                                ResolveTypedef(resolved, events, type.Type())
                            else:
                                if isinstance(type.Type(), ProxyStubGenerator.CppParser.Class) and type.Type().is_event:
                                    events.append(type)
                        else:
                            raise CppParseError(var, "undefined type: '%s'" % " ".join(type))
                        resolved.append(type)
                        return events

                    resolved = []
                    events = ResolveTypedef(resolved, events, var.type)
                return events

            def BuildParameters(vars, rpc_format, is_property=False, test=False):
                params = {"type": "object"}
                properties = OrderedDict()
                required = []
                for var in vars:
                    if var.meta.input or not var.meta.output:
                        if (not var.type.IsConst() or (var.type.IsPointer() and not var.type.IsPointerToConst())) and verify:
                            if not var.meta.input:
                                log.WarnLine(var, "'%s': non-const parameter assumed to be input (forgot 'const'?)" % var.name)
                            elif not var.meta.output:
                                log.WarnLine(var, "'%s': non-const parameter marked with @in tag (forgot 'const'?)" % var.name)
                        var_name = "value" if is_property else (var.meta.text if var.meta.text else var.name.lower())
                        if var_name.startswith("__unnamed") and not test:
                            raise CppParseError(var, "unnamed parameter, can't deduce parameter name (*1)")
                        properties[var_name] = ConvertParameter(var)
                        if not is_property and not var.name.startswith("@_") and not var.name.startswith("__unnamed"):
                            properties[var_name]["original_name"] = var.name
                        properties[var_name]["position"] = vars.index(var)
                        if not is_property and "description" not in properties[var_name] and verify:
                            log.DocIssue("'%s': parameter is missing description" % var_name)
                        required.append(var_name)
                        if properties[var_name]["type"] == "string" and not var.type.IsReference() and not var.type.IsPointer() and not "enum" in properties[var_name]:
                            log.WarnLine(var, "'%s': passing string by value (forgot &?)" % var.name)
                params["properties"] = properties
                params["required"] = required
                if is_property and ((rpc_format == RpcFormat.EXTENDED) or (rpc_format == RpcFormat.COLLAPSED)):
                    if len(properties) == 1:
                        return list(properties.values())[0]
                    elif len(properties) > 1:
                        params["required"] = required
                        return params
                    else:
                        return None
                else:
                    if (len(properties) == 0):
                        return {}
                    elif (len(properties) == 1) and (rpc_format == RpcFormat.COLLAPSED):
                        # collapsed format: if only one parameter present then omit the outer object
                        return list(properties.values())[0]
                    else:
                        return params

            def BuildIndex(var, test=False):
                return BuildParameters([var], True, True, True, test)

            def BuildResult(vars, is_property=False):
                params = {"type": "object"}
                properties = OrderedDict()
                required = []
                for var in vars:
                    var_type = ResolveTypedef(var.type)
                    if var.meta.output:
                        if var_type.IsValue():
                            raise CppParseError(var, "parameter marked with @out tag must be either a reference or a pointer")
                        if var_type.IsConst():
                            raise CppParseError(var, "parameter marked with @out tag must not be const")
                        var_name = "value" if is_property else (var.meta.text if var.meta.text else var.name.lower())
                        if var_name.startswith("__unnamed"):
                            raise CppParseError(var, "unnamed parameter, can't deduce parameter name (*2)")
                        properties[var_name] = ConvertParameter(var)
                        if not is_property and not var.name.startswith("@_") and not var.name.startswith("__unnamed"):
                           properties[var_name]["original_name"] = var.name
                        properties[var_name]["position"] = vars.index(var)
                        required.append(var_name)
                params["properties"] = properties
                if len(properties) == 1:
                    return list(properties.values())[0]
                elif len(properties) > 1:
                    params["required"] = required
                    return params
                else:
                    void = {"type": "null"}
                    void["description"] = "Always null"
                    return void

            if method.is_excluded:
                continue

            prefix = (face.obj.parent.name.lower() + "_") if face.obj.parent.full_name != INTERFACE_NAMESPACE else ""
            method_name = method.retval.meta.text if method.retval.meta.text else method.name
            method_name_lower = method_name.lower()

            event_params = EventParameters(method.vars)
            for e in event_params:
                exists = any(x.obj.type == e.type.type for x in event_interfaces)
                if not exists:
                    event_interfaces.add(ProxyStubGenerator.Interface.Interface(ResolveTypedef(e).type, 0, file))

            obj = None

            if method.retval.meta.is_property or (prefix + method_name_lower) in properties:
                try:
                    obj = properties[prefix + method_name_lower]
                    obj["original_name"] = method.name
                except:
                    obj = OrderedDict()
                    obj["original_name"] = method.name
                    properties[prefix + method_name_lower] = obj

                indexed_property = (len(method.vars) == 2 and method.vars[0].meta.is_index)

                if len(method.vars) == 1 or (len(method.vars) == 2 and indexed_property):
                    if indexed_property:
                        if method.vars[0].type.IsPointer():
                            raise CppParseError(method.vars[0], "index to a property must not be pointer")
                        if not method.vars[0].type.IsConst() and method.vars[0].type.IsReference():
                            raise CppParseError(method.vars[0], "index to a property must be an input parameter")
                        if "index" not in obj:
                            obj["index"] = BuildIndex(method.vars[0])
                            obj["index"]["name"] = method.vars[0].name
                            if "enum" in obj["index"]:
                                obj["index"]["example"] = obj["index"]["enum"][0]
                            if "example" not in obj["index"]:
                                # example not specified, let's invent something...
                                obj["index"]["example"] = ("0" if obj["index"]["type"] == "integer" else "xyz")
                            if obj["index"]["type"] not in ["integer", "string"]:
                                raise CppParseError(method.vars[0], "index to a property must be integer, enum or string type")
                        else:
                            test = BuildIndex(method.vars[0], True)
                            if not test:
                                raise CppParseError(method.vars[value], "property index must be an input parameter")
                            if obj["index"]["type"] != test["type"]:
                                raise CppParseError(method.vars[0], "setter and getter of the same property must have same index type")
                        if method.vars[1].meta.is_index:
                            raise CppParseError(method.vars[0], "index must be the first parameter to property method")

                        value = 1
                    else:
                        value = 0 # no index

                    if "const" in method.qualifiers:
                        # getter
                        if method.vars[value].type.IsConst():
                            raise CppParseError(method.vars[value], "property getter method must not use const parameter")
                        else:
                            if rpc_format == RpcFormat.COLLAPSED:
                                result_name = "params"
                            else:
                                result_name = "result"
                            if "writeonly" in obj:
                                del obj["writeonly"]
                            else:
                                obj["readonly"] = True
                            if result_name not in obj:
                                obj[result_name] = BuildResult([method.vars[value]], True)
                            else:
                                test = BuildResult([method.vars[value]], True)
                                if not test:
                                    raise CppParseError(method.vars[value], "property getter method must have one output parameter")
                                if obj[result_name]["type"] != test["type"]:
                                    raise CppParseError(method.vars[value], "setter and getter of the same property must have same type (*1)")
                                if "ptr" in test and test["ptr"]:
                                    obj[result_name]["ptr"] = True
                                if "ptrtoconst" in test and test["ptrtoconst"]:
                                    obj[result_name]["ptrtoconst"] = True
                            if obj[result_name] == None:
                                raise CppParseError(method.vars[value], "property getter method must have one output parameter")
                    else:
                        # setter
                        if not method.vars[value].type.IsConst():
                            raise CppParseError(method.vars[value], "property setter method must use a const parameter")
                        else:
                            if "readonly" in obj:
                                del obj["readonly"]
                            else:
                                obj["writeonly"] = True
                            if "params" not in obj:
                                obj["params"] = BuildParameters([method.vars[value]], rpc_format, True)
                            else:
                                test = BuildParameters([method.vars[value]], rpc_format, True, True)
                                if not test:
                                    raise CppParseError(method.vars[value], "property setter method must have one input parameter")
                                if obj["params"]["type"] != test["type"]:
                                    raise CppParseError(method.vars[value], "setter and getter of the same property must have same type (*2)")
                                if "ref" in test and test["ref"]:
                                    obj["params"]["ref"] = True
                            if obj["params"] == None:
                                raise CppParseError(method.vars[value], "property setter method must have one input parameter")
                else:
                    raise CppParseError(method, "property method must have one parameter")

            elif method.IsPureVirtual() and not event_params:
                if method.retval.type and ((isinstance(method.retval.type.Type(), ProxyStubGenerator.CppParser.Integer) and (method.retval.type.Type().size == "long")) or not verify):
                    obj = OrderedDict()
                    params = BuildParameters(method.vars, rpc_format)
                    if "properties" in params and params["properties"]:
                        if method.name.lower() in [x.lower() for x in params["required"]]:
                            raise CppParseError(method, "parameters must not use the same name as the method")
                    if params:
                        obj["params"] = params
                    obj["result"] = BuildResult(method.vars)
                    obj["original_name"] = method_name
                    methods[prefix + method_name_lower] = obj
                else:
                    raise CppParseError(method, "method return type must be uint32_t (error code), i.e. pass other return values by a reference")

            if obj:
                if method.retval.meta.is_deprecated:
                    obj["deprecated"] = True
                elif method.retval.meta.is_obsolete:
                    obj["obsolete"] = True
                if method.retval.meta.brief:
                    obj["summary"] = method.retval.meta.brief
                elif (prefix + method_name_lower) not in properties and verify:
                    log.DocIssue("'%s': %s is missing brief description" % (method.name, "property" if method.retval.meta.is_property else "method"))
                if method.retval.meta.details:
                    obj["description"] = method.retval.meta.details
                if method.retval.meta.retval:
                    errors = []
                    for err in method.retval.meta.retval:
                        errEntry = OrderedDict()
                        errEntry["description"] = method.retval.meta.retval[err]
                        errEntry["message"] = err
                        errors.append(errEntry)
                    if errors:
                        obj["errors"] = errors

        for f in event_interfaces:
            rpc_format = _EvaluateRpcFormat(f.obj)
            for method in f.obj.methods:
                EventParameters(method.vars) # just to check for undefined types...
                if method.IsPureVirtual() and method.is_excluded == False:
                    obj = OrderedDict()
                    obj["original_name"] = method.name
                    varsidx = 0
                    if len(method.vars) > 0:
                        for v in method.vars[1:]:
                            if v.meta.is_index:
                                log.WarnLine(method,"@index ignored on non-first parameter of %s" % method.name)
                        if method.vars[0].meta.is_index:
                            if method.vars[0].type.IsPointer():
                                raise CppParseError(method.vars[0], "index to a notification must not be pointer")
                            if not method.vars[0].type.IsConst() and method.vars[0].type.IsReference():
                                raise CppParseError(method.vars[0], "index to a notification must be an input parameter")
                            obj["id"] = BuildParameters([method.vars[0]], True, True, False)
                            obj["id"]["name"] = method.vars[0].name
                            if "example" not in obj["id"]:
                                obj["id"]["example"] = "0" if obj["id"]["type"] == "integer" else "abc"
                            if obj["id"]["type"] not in ["integer", "string"]:
                                raise CppParseError(method.vars[0], "index to a notification must be integer, enum or string type")
                            varsidx = 1
                    if method.retval.meta.is_listener:
                        obj["statuslistener"] = True
                    params = BuildParameters(method.vars[varsidx:], rpc_format, False)
                    if method.retval.meta.is_deprecated:
                        obj["deprecated"] = True
                    elif method.retval.meta.is_obsolete:
                        obj["obsolete"] = True
                    if method.retval.meta.brief:
                        obj["summary"] = method.retval.meta.brief
                    else:
                        log.DocIssue("'%s': event is missing brief description" % method.name)
                    if method.retval.meta.details:
                        obj["description"] = method.retval.meta.details
                    if params:
                        obj["params"] = params
                    method_name = method.retval.meta.text if method.retval.meta.text else method.name
                    events[prefix + method_name.lower()] = obj

        if methods:
            schema["methods"] = methods
        if properties:
            schema["properties"] = properties
        if events:
            schema["events"] = events

        if DUMP_JSON:
            print("\n// JSON interface for {} -----------".format(face.obj.name))
            print(json.dumps(schema, indent=2))
            print("// ----------------\n")
        return schema

    schemas = []
    if interfaces:
        for face in interfaces:
            schema = Build(face)
            if schema:
                schemas.append(schema)
    else:
        log.Info("No interfaces found")

    return schemas, []



########################################################
#
# JSON OBJECT TRACKER
#

def SortByDependency(objects):
    sorted_objects = []

    # This will order objects by their relations
    for obj in sorted(objects, key=lambda x: x.cpp_class, reverse=False):
        found = filter(lambda sorted_obj: obj.cpp_class in map(lambda x: x.cpp_class, sorted_obj.objects), sorted_objects)
        try:
            index = min(map(lambda x: sorted_objects.index(x), found))
            movelist = filter(lambda x: x.cpp_class in map(lambda x: x.cpp_class, sorted_objects), obj.objects)
            sorted_objects.insert(index, obj)
            for m in movelist:
                if m in sorted_objects:
                    sorted_objects.insert(index, sorted_objects.pop(sorted_objects.index(m)))
        except ValueError:
            sorted_objects.append(obj)

    return sorted_objects

def IsInRef(obj):
    while obj:
        if "@ref" in obj.schema:
            return True
        obj = obj.parent
    return False

def IsInCustomRef(obj):
    while obj:
        if "@ref" in obj.schema and obj.schema["@ref"] == "#../params":
            return True
        obj = obj.parent
    return False

class ObjectTracker:
    def __init__(self):
        self.objects = []
        self.Reset()

    def Add(self, newObj):
        def _CompareObject(lhs, rhs):
            def _CompareType(lhs, rhs):
                if rhs["type"] != lhs["type"]:
                    return False
                if "size" in lhs:
                    if "size" in rhs:
                        if lhs["size"] != rhs["size"]:
                            return False
                    elif "size" != 32:
                        return False
                elif "size" in rhs:
                    if rhs["size"] != 32:
                        return False
                if "signed" in lhs:
                    if "signed" in rhs:
                        if lhs["signed"] != rhs["signed"]:
                            return False
                    elif lhs["signed"] != False:
                        return False
                elif "signed" in rhs:
                    if rhs["signed"] != False:
                        return False
                if "enum" in lhs:
                    if "enum" in rhs:
                        if lhs["enum"] != rhs["enum"]:
                            return False
                    else:
                        return False
                    if "enumvalues" in lhs:
                        if "enumvalues" in rhs:
                            if lhs["enumvalues"] != rhs["enumvalues"]:
                                return False
                        else:
                            return False
                    elif "enumvalues" in rhs:
                        return False
                    if "values" in lhs:
                        if "values" in rhs:
                            if lhs["values"] != rhs["values"]:
                                return False
                        else:
                            return False
                    elif "values" in rhs:
                        return False
                    if "enumids" in lhs:
                        if "enumids" in rhs:
                            if lhs["enumids"] != rhs["enumids"]:
                                return False
                        else:
                            return False
                    elif "enumids" in rhs:
                        return False
                    if "ids" in lhs:
                        if "ids" in rhs:
                            if lhs["ids"] != rhs["ids"]:
                                return False
                        else:
                            return False
                    elif "ids" in rhs:
                        return False
                elif "enum" in rhs:
                    return False
                if "items" in lhs:
                    if "items" in rhs:
                        if not _CompareType(lhs["items"], rhs["items"]):
                            return False
                    else:
                        return False
                elif "items" in rhs:
                    return False
                if "properties" in lhs:
                    if "properties" in rhs:
                        if not _CompareObject(lhs["properties"], rhs["properties"]):
                            return False
                    else:
                        return False
                return True

            # NOTE: Two objects are considered identical if they have the same property names and types only!
            for name, prop in lhs.items():
                if name not in rhs:
                    return False
                else:
                    if not _CompareType(prop, rhs[name]):
                        return False
            for name, prop in rhs.items():
                if name not in lhs:
                    return False
                else:
                    if not _CompareType(prop, lhs[name]):
                        return False
            return True

        if "properties" in newObj.schema and not isinstance(newObj, JsonMethod):
            self.objects.append(newObj)
            is_ref = IsInRef(newObj)
            props = newObj.schema["properties"]
            for obj in self.objects[:-1]:
                if _CompareObject(obj.schema["properties"], props):
                    if not GENERATED_JSON and not NO_DUP_WARNINGS and (not is_ref and not IsInRef(obj)):
                        warning = "'%s': duplicate object (same as '%s') - consider using $ref" % (newObj.print_name, obj.print_name)
                        if len(props) > 2:
                            log.Warn(warning)
                        else:
                            log.Info(warning)
                    return obj
        return None

    def Remove(self, obj):
        self.objects.remove(obj)

    def Reset(self):
        self.objects = []

    def CommonObjects(self):
        return SortByDependency(filter(lambda obj: obj.RefCount() > 1, self.objects))


class EnumTracker(ObjectTracker):
    def _IsTopmost(self, obj):
        while isinstance(obj.parent, JsonArray):
            obj = obj.parent
        return isinstance(obj.parent, JsonMethod)

    def Add(self, newObj):
        def __Compare(lhs, rhs):
            # NOTE: Two enums are considered identical if they have the same enumeration names and types
            if (lhs["enum"] == rhs["enum"]) and (lhs["type"] == rhs["type"]):
                if "enumvalues" in lhs:
                    if "enumvalues" not in rhs:
                        return False
                    else:
                        return lhs["enumvalues"] == rhs["enumvalues"]
                elif "enumvalues" in rhs:
                    return False
                if "enumids" in lhs:
                    if "enumids" not in rhs:
                        return False
                    else:
                        return lhs["enumids"] == rhs["enumids"]
                elif "enumids" in rhs:
                    return False
                if "values" in lhs:
                    if "values" not in rhs:
                        return False
                    else:
                        return lhs["values"] == rhs["values"]
                elif "values" in rhs:
                    return False
                if "ids" in lhs:
                    if "ids" not in rhs:
                        return False
                    else:
                        return lhs["ids"] == rhs["ids"]
                elif "ids" in rhs:
                    return False
                return True
            else:
                return False

        if "enum" in newObj.schema and not isinstance(newObj, JsonMethod):
            self.objects.append(newObj)
            is_ref = IsInRef(newObj)
            for obj in self.objects[:-1]:
                if __Compare(obj.schema, newObj.schema):
                    if not GENERATED_JSON and not NO_DUP_WARNINGS and (not is_ref and not IsInRef(obj)):
                        log.Warn("%s: duplicate enums (same as '%s') - consider using $ref" %
                                   (newObj.print_name, obj.print_name))
                    return obj
            return None

    def CommonObjects(self):
        return SortByDependency(filter(lambda obj: ((obj.RefCount() > 1) or self._IsTopmost(obj)), self.objects))


##############################################################################
#
# CODE EMITTER
#

class Emitter():
    def __init__(self, file, indentSize):
        self.indent_size = indentSize
        self.indent = 0
        self.file = file

    def Line(self, text=""):
        if text != "":
            self.file.write(" " * self.indent + str(text) + "\n")
        else:
            self.file.write("\n")

    def Indent(self):
        self.indent += self.indent_size

    def Unindent(self):
        if self.indent >= self.indent_size:
            self.indent -= self.indent_size
        else:
            self.indent = 0


##############################################################################
#
# C++ ENUMERATION GENERATOR
#

def GetNamespace(root, obj, full=True):
    namespace = ""
    if obj.do_create and isinstance(obj, (JsonObject, JsonEnum, JsonArray)):
        if isinstance(obj, JsonObject) and not obj.properties:
            return namespace
        fullname = ""
        e = obj
        while e.parent and not isinstance(e, JsonMethod) and (not e.is_duplicate or e.RefCount() > 1):
            if not isinstance(e, (JsonEnum, JsonArray)):
                fullname = e.cpp_class + "::" + fullname
            if e.RefCount() > 1:
                break
            e = e.parent
        if full:
            namespace = "%s::%s::" % (DATA_NAMESPACE, root.cpp_name)
        namespace = namespace + "%s" % fullname
    return namespace


def EmitEnumRegs(root, emit, header_file, if_file):
    def EmitEnumRegistration(root, enum, full=True):
        fullname = (GetNamespace(root, enum) if full else "%s::%s::" %
                    (DATA_NAMESPACE, root.cpp_name)) + enum.cpp_class
        emit.Line("ENUM_CONVERSION_BEGIN(%s)" % fullname)
        emit.Indent()
        for c, item in enumerate(enum.enumerators):
            emit.Line("{ %s::%s, _TXT(\"%s\") }," % (fullname, enum.cpp_enumerators[c], enum.enumerators[c]))
        emit.Unindent()
        emit.Line("ENUM_CONVERSION_END(%s);" % fullname)

    # Enumeration conversion code
    emit.Line("#include \"definitions.h\"")
    if if_file.endswith(".h"):
        emit.Line("#include <%s%s>" % (CPP_INTERFACE_PATH, if_file))
    emit.Line("#include <core/Enumerate.h>")
    emit.Line("#include \"%s_%s.h\"" % (DATA_NAMESPACE, header_file))
    emit.Line()
    emit.Line("namespace %s {" % FRAMEWORK_NAMESPACE)
    count = 0
    for obj in enumTracker.objects:
        if not obj.is_duplicate and not obj.included_from:
            emit.Line()
            EmitEnumRegistration(root, obj, obj.RefCount() == 1 or not obj.do_create)
            count += 1
    emit.Line()
    emit.Line("}")
    return count


##############################################################################
#
# JSON-RPC CODE GENERATOR
#

def EmitEvent(emit, root, event, static=False):
    module = "_module"
    emit.Line("// Event: %s" % event.Headline())
    params = event.params.cpp_type
    par = ""
    if params != "void":
        par = ("const %s& id, " % event.sendif_type.cpp_native_type) if event.sendif_type else ""
        if event.params.properties and event.params.do_create:
            par = par + ", ".join(map(lambda x: "const " + (GetNamespace(root, x, False) if not static else "") + x.cpp_native_type + "& " + x.local_name, event.params.properties))
        else:
            par = par + "const " + (GetNamespace(root, event.params, False) if not static else "") + event.params.cpp_native_type + "& " + event.params.local_name
    if not static:
        line = "void %s::%s(%s)" % (root.cpp_name, event.endpoint_name, par)
    else:
        line = "static void %s(const PluginHost::JSONRPC& %s%s%s)" % (event.cpp_name, module, ", " if par else "", par)
    if event.included_from:
        line += " /* %s */" % event.included_from
    emit.Line(line)
    emit.Line("{")
    emit.Indent()

    if params != "void":
        emit.Line("%s _params;" % params)
        if event.params.properties and event.params.do_create:
            for p in event.params.properties:
                if isinstance(p, JsonEnum):
                    emit.Line("_params.%s = static_cast<%s>(%s);" % (p.cpp_name, GetNamespace(root, p, False) + p.cpp_class, p.local_name))
                else:
                    emit.Line("_params.%s = %s;" % (p.cpp_name, p.local_name))
        else:
            emit.Line("_params = %s;" % event.params.local_name)
        emit.Line()

    if event.sendif_type:
        index_var = "designatorId"
        emit.Line('%sNotify(_T("%s")%s, [&id](const string& designator) -> bool {' %
                  (("%s." % module) if static else "", event.json_name, ", _params" if params != "void" else ""))
        emit.Indent()
        emit.Line("const string %s = designator.substr(0, designator.find('.'));" % index_var)
        if isinstance(event.sendif_type, JsonInteger):
            index_var = "_designatorIdInt"
            type = event.sendif_type.cpp_native_type
            emit.Line("%s %s{};" % (type, index_var))
            emit.Line("if (Core::FromString(%s, %s) == false) {" % ("designatorId", index_var))
            emit.Indent()
            emit.Line("return (false);")
            emit.Unindent()
            emit.Line("} else {")
            emit.Indent()
        elif isinstance(event.sendif_type, JsonEnum):
            index_var = "_designatorIdEnum"
            type = event.sendif_type.cpp_native_type
            emit.Line("Core::EnumerateType<%s> _value(%s.c_str());" % (type, "designatorId"))
            emit.Line("const %s %s = _value.Value();" % ( type, index_var))
            emit.Line("if (_value.IsSet() == false) {")
            emit.Indent()
            emit.Line("return (false);")
            emit.Unindent()
            emit.Line("} else {")
            emit.Indent()
        emit.Line("return (id == %s);" % index_var)
        if not isinstance(event.sendif_type, JsonString):
            emit.Unindent()
            emit.Line("}")
        emit.Unindent()
        emit.Line("});")
    else:
        emit.Line('%sNotify(_T("%s")%s);' %
                  (("%s." % module) if static else "", event.json_name, ", _params" if params != "void" else ""))
    emit.Unindent()
    emit.Line("}")
    emit.Line()

def _EmitRpcPrologue(root, emit, header_file, source_file, data_emitted, cpp = False):
    struct = "J" + root.json_name
    emit.Line("// Generated automatically from '%s'. DO NOT EDIT." % os.path.basename(source_file))
    emit.Line()
    emit.Line("#pragma once")
    emit.Line()
    emit.Line("#include \"Module.h\"")
    if data_emitted:
        emit.Line("#include \"%s_%s.h\"" % (DATA_NAMESPACE, header_file))
    if cpp:
        emit.Line("#include <%s%s>" % (CPP_INTERFACE_PATH, source_file))
    emit.Line()
    emit.Line("namespace %s {" % FRAMEWORK_NAMESPACE)
    emit.Line()
    emit.Line("namespace %s {" % "Exchange")
    emit.Indent()
    emit.Line()
    namespace = root.json_name
    if "info" in root.schema and "namespace" in root.schema["info"]:
        namespace = root.schema["info"]["namespace"] + "::" + namespace
        emit.Line("namespace %s {" % root.schema["info"]["namespace"])
        emit.Indent()
        emit.Line()
    namespace = DATA_NAMESPACE + "::" + namespace
    emit.Line("namespace %s {" % struct)
    emit.Indent()
    emit.Line()
    return namespace

def _EmitVersionCode(emit, version):
    emit.Line("namespace Version {")
    emit.Indent()
    emit.Line()
    emit.Line("constexpr uint8_t Major = %u;" % version[0])
    emit.Line("constexpr uint8_t Minor = %u;" % version[1])
    emit.Line("constexpr uint8_t Patch = %u;" % version[2])
    emit.Line()
    emit.Unindent()
    emit.Line("} // namespace Version")

def EmitVersionCode(root, emit, header_file, source_file, data_emitted):
    struct = "J" + root.json_name
    _EmitRpcPrologue(root, emit, header_file, source_file, data_emitted)
    _EmitVersionCode(emit, GetVersion(root.schema["info"] if "info" in root.schema else dict()))
    emit.Line()
    emit.Unindent()
    emit.Line("} // namspace %s" % struct)
    emit.Unindent()
    emit.Line()
    emit.Line("} // namespace Exchange")
    emit.Line()
    emit.Line("}")

def EmitRpcCode(root, emit, header_file, source_file, data_emitted):
    module = "_module"
    namespace = _EmitRpcPrologue(root, emit, header_file, source_file, data_emitted, True)
    _EmitVersionCode(emit, GetVersion(root.schema["info"] if "info" in root.schema else dict()))
    struct = "J" + root.json_name
    face = "I" + root.json_name
    destination_var = "_destination"
    emit.Line()
    if data_emitted:
        emit.Line("using namespace %s;" % namespace)
        emit.Line()
    emit.Line("static void Register(PluginHost::JSONRPC& %s, %s* %s)" % (module, face, destination_var))
    emit.Line("{")
    emit.Indent()
    emit.Line("ASSERT(%s != nullptr);" % destination_var)
    emit.Line()
    emit.Line("%s.RegisterVersion(_T(\"%s\"), Version::Major, Version::Minor, Version::Patch);" % (module, struct))
    emit.Line();

    events = []

    for m in root.properties:
        if not isinstance(m, JsonNotification):

            indexed = isinstance(m, JsonProperty) and m.index
            if indexed:
                index_converted = False

            # Emit method prologue
            if isinstance(m, JsonProperty):
                if m.properties[1].is_void and not m.writeonly:
                    # try to detect the uncompliant format
                    params = copy.deepcopy(m.properties[0] if not m.readonly else m.properties[1])
                    response = copy.deepcopy(m.properties[0] if not m.writeonly else m.properties[1])
                else:
                    params = copy.deepcopy(m.properties[0])
                    response = copy.deepcopy(m.properties[1])
                params.Rename("params")
                response.Rename("result")
                emit.Line("// %sProperty: %s%s" % ("Indexed " if indexed else "", m.Headline(), " (r/o)" if m.readonly else (" (w/o)" if m.writeonly else "")))
            else:
                params = copy.deepcopy(m.properties[0])
                response = copy.deepcopy(m.properties[1])
                emit.Line("// Method: %s" % m.Headline())

            function_def = ""
            if indexed:
                function_def =", std::function<uint32_t(const std::string&, %s%s)>" % ("" if params.is_void else ("const " + params.cpp_type + "&"), "" if response.is_void else (("" if params.is_void else ", ") + response.cpp_type + "&"))
            line = '%s.Register<%s, %s%s>(_T("%s"),' % (module, params.cpp_type, response.cpp_type, function_def, m.json_name)
            emit.Line(line)
            emit.Indent()
            line = '[%s](' % destination_var
            if indexed:
                line = line + "const string& %s, " % m.index.cpp_name
            if not params.is_void:
                line = line + ("const " + params.cpp_type + "& " + params.cpp_name)
            if not params.is_void and not response.is_void:
                line = line + ", "
            if not response.is_void:
                line = line + (response.cpp_type + "& " + response.cpp_name)
            line = line + ') -> uint32_t {'
            emit.Line(line)

            # Emit the function body
            emit.Indent()
            error_code = AuxJsonInteger("errorCode", 32)
            emit.Line("uint32_t %s;" % error_code.TempName())

            READ_ONLY = 0
            READ_WRITE = 1
            WRITE_ONLY = 2

            def Invoke(params, response, use_prefix = True, const_cast=False, parent = None):
                vars = OrderedDict()

                # Build param/response dictionaries (dictionaries will ensure they do not repeat)
                if params and not params.is_void:
                    if isinstance(params, JsonObject) and params.do_create:
                        for param in params.properties:
                            vars[param.json_name] = [param, READ_ONLY]
                    else:
                        vars[params.json_name] = [params, READ_ONLY]

                if response and not response.is_void:
                    if isinstance(response, JsonObject) and response.do_create:
                        for resp in response.properties:
                            if resp.json_name not in vars:
                                vars[resp.json_name] = [resp, WRITE_ONLY]
                            else:
                               vars[resp.json_name][1] = READ_WRITE
                    else:
                        if response.json_name not in vars:
                            vars[response.json_name] = [response, WRITE_ONLY]
                        else:
                            vars[response.json_name][1] = READ_WRITE

                for _, _record in vars.items():
                    arg = _record[0]
                    arg_type = _record[1]
                    if isinstance(arg, JsonString) and "length" in arg.schema:
                        for var, q in vars.items():
                            arg2 = q[0]
                            arg2_type = q[1]
                            if var == arg.schema["length"]:
                                # Have to handle any order of length/buffer params
                                arg2.schema["bufferlength"] = True
                                if arg2_type == WRITE_ONLY:
                                    raise RuntimeError("'%s': parameter marked pointed to by @length is output only" % arg2.name)

                # Emit temporary variables and deserializing of JSON data
                for _, _record in sorted(vars.items(), key=lambda x: x[1][0].schema["position"]):
                    arg = _record[0]
                    arg_type = _record[1]
                    if "bufferlength" in arg.schema:
                        arg.release = False
                        arg.cast = None
                        arg.prefix = ""
                        continue
                    # C-style buffers
                    arg.release = False
                    arg.cast = None
                    arg.prefix = "&" if ("ptr" in arg.schema and arg.schema["ptr"]) and use_prefix else ""

                    if isinstance(arg, JsonString) and "length" in arg.schema:
                        for var, q in vars.items():
                            arg2 = q[0]
                            arg2_type = q[1]
                            if var == arg.schema["length"]:
                                emit.Line("%s %s{%s};" % (arg2.cpp_native_type, arg2.TempName(), "%s%s.Value()" % (parent if parent else "", arg2.cpp_name) if arg2_type != WRITE_ONLY else ""))
                                break
                        encode = "encode" in arg.schema and arg.schema["encode"]
                        if arg_type == READ_ONLY and not encode:
                            emit.Line("const %s* %s{%s%s.Value().data()};" % (arg.schema["cpptype"], arg.TempName(),  parent if parent else "", arg.cpp_name))
                        else:
                            emit.Line("%s* %s = nullptr;" % (arg.schema["cpptype"], arg.TempName()))
                            emit.Line("if (_%s != 0) {" % arg.schema["length"])
                            emit.Indent()
                            emit.Line("%s = reinterpret_cast<%s*>(ALLOCA(_%s));" % (arg.TempName(), arg.schema["cpptype"], arg.schema["length"]))
                            emit.Line("ASSERT(%s != nullptr);" % arg.TempName())
                        if arg_type != WRITE_ONLY:
                            if encode:
                                emit.Line("// Decode base64-encoded JSON string")
                                emit.Line("Core::FromString(%s%s.Value(), %s, %s, nullptr);" % (parent if parent else "", arg.cpp_name, arg.TempName(), arg.schema["length"]))
                            elif arg_type != READ_ONLY:
                                emit.Line("::memcpy(%s, %s%s.Value().data(), %s);" % (arg.TempName(), parent if parent else "", arg.cpp_name, arg.schema["length"]))
                        if arg_type != READ_ONLY or encode:
                            emit.Unindent()
                            emit.Line("}")
                    # Iterators
                    elif isinstance(arg, JsonArray):
                        if arg.iterator:
                            if arg_type == READ_ONLY:
                                emit.Line("std::list<%s> _elements;" %(arg.items.cpp_native_type))
                                emit.Line("auto _Iterator = %s.Elements();" % ((parent if parent else "") + arg.cpp_name))
                                emit.Line("while (_Iterator.Next() == true) {")
                                emit.Indent()
                                emit.Line("_elements.push_back(_Iterator.Current()%s);" % (".Value()" if not isinstance(arg.items, JsonObject) else ""))
                                emit.Unindent()
                                emit.Line("}")
                                impl = arg.iterator[:arg.iterator.index('<')].replace("IIterator", "Iterator") + "<%s>" % arg.iterator
                                initializer = "Core::Service<%s>::Create<%s>(_elements)" % (impl, arg.iterator)
                                emit.Line("%s* %s{%s};" % (arg.iterator, arg.TempName(), initializer))
                                arg.release = True
                                if "ref" in arg.schema and arg.schema["ref"]:
                                    arg.cast = "static_cast<%s* const&>(%s)" % (arg.iterator, arg.TempName())
                            elif arg_type == WRITE_ONLY:
                                emit.Line("%s%s* %s{};" % ("const " if "ptrtoconst" in arg.schema else "", arg.iterator, arg.TempName()))
                            else:
                                raise RuntimeError("Read/write arrays are not supported: %s" % arg.json_name)
                        else:
                            raise RuntimeError("Arrays need to be iterators: %s" % arg.json_name)
                    # PODs
                    elif isinstance(arg, JsonObject):
                        emit.Line("%s%s %s%s;" % ("const " if (arg_type == READ_ONLY )else "", arg.cpp_native_type, arg.TempName(), "(%s%s)" % (parent if parent and not isinstance(arg.parent, JsonMethod) else "", arg.cpp_name) if arg_type != WRITE_ONLY else "{}"))
                    # All Other
                    else:
                        emit.Line("%s%s %s{%s};" % ("const " if (arg_type == READ_ONLY) else "", arg.cpp_native_type, arg.TempName(), "%s%s.Value()" % (parent if parent else "", arg.cpp_name) if arg_type != WRITE_ONLY else ""))

                cond = ""
                for _, _record in vars.items():
                    arg = _record[0]
                    if arg.release:
                        cond += "(%s != nullptr) &&" % arg.TempName()
                if cond:
                    emit.Line("if (%s) {" % cond[:-3])
                    emit.Indent()

                # Emit call the API
                if const_cast:
                    line = "%s = (static_cast<const %s*>(%s))->%s(" % (error_code.TempName(), face, destination_var, m.cpp_name)
                else:
                    line = "%s = %s->%s(" % (error_code.TempName(), destination_var, m.cpp_name)
                if indexed:
                    line = line + m.index.TempName() + ", "
                for _, _record in sorted(vars.items(), key=lambda x: x[1][0].schema["position"]):
                    arg = _record[0]
                    line = line + ("%s%s, " % (arg.prefix, arg.cast if arg.cast else arg.TempName()))
                if line.endswith(", "):
                    line = line[:-2]
                line = line + ");"
                emit.Line(line)

                if cond:
                    for _, _record in vars.items():
                        arg = _record[0]
                        if arg.release:
                            emit.Line("%s->Release();" % arg.TempName())
                    emit.Unindent()
                    emit.Line("} else {")
                    emit.Indent()
                    emit.Line("%s = Core::ERROR_GENERAL;" % error_code.TempName())
                    emit.Unindent()
                    emit.Line("}")

                # Emit result handling and serializing JSON data
                if response and not response.is_void:
                    emit.Line("if (%s == Core::ERROR_NONE) {" % error_code.TempName())
                    emit.Indent()

                    def EmitResponse(elem, cppParent = "", parent = ""):
                        if isinstance(elem, JsonObject) and elem.do_create:
                            for p in elem.properties:
                                EmitResponse(p, cppParent + (((elem.cpp_name if cppParent else elem.TempName()) + ".") if not elem.do_create else ""), parent + elem.cpp_name + "." )
                        # C-style buffers disguised as base64-encoded strings
                        elif isinstance(elem, JsonString) and "length" in elem.schema:
                            emit.Line("if (_%s != 0) {" % elem.schema["length"])
                            emit.Indent()
                            if "encode" in elem.schema and elem.schema["encode"]:
                                emit.Line("// Convert the C-style buffer to a base64-encoded JSON string")
                                emit.Line("%s %s;" % (elem.cpp_native_type, elem.TempName("encoded")))
                                emit.Line("Core::ToString(%s, _%s, true, %s);" % (elem.TempName(), elem.schema["length"], elem.TempName("encoded")))
                                emit.Line("%s%s = %s;" % (parent, elem.cpp_name, elem.TempName("encoded")))
                            else:
                                emit.Line("%s%s = string(%s, %s);" % (parent, elem.cpp_name, elem.TempName(), elem.schema["length"]))
                            emit.Unindent()
                            emit.Line("}")
                        # Iterators disguised as arrays
                        elif isinstance(elem, JsonArray):
                            if elem.iterator:
                                emit.Line("if (%s != nullptr) {" % elem.TempName())
                                emit.Indent()
                                emit.Line("// Convert the iterator to a JSON array")
                                emit.Line("%s %s{};" % ((elem.items.cpp_native_type, elem.items.TempName("item"))))
                                emit.Line("while (%s->Next(%s) == true) {" % (elem.TempName(), elem.items.TempName("item")))
                                emit.Indent()
                                emit.Line("%s& _Element(%s.Add());" % (elem.items.cpp_type, parent + elem.cpp_name))
                                emit.Line("_Element = %s;" % elem.items.TempName("item"))
                                emit.Unindent()
                                emit.Line("}")
                                emit.Line("%s->Release();" % elem.TempName())
                                emit.Unindent()
                                emit.Line("}")
                            else:
                                raise RuntimeError("unable to serialize a non-iterator array: %s" % elem.json_name)
                        # Enums
                        elif isinstance(elem, JsonEnum):
                            if elem.do_create:
                                emit.Line("%s%s = static_cast<%s>(%s%s);" % (parent, elem.cpp_name, elem.cpp_class, cppParent, elem.TempName()))
                            else:
                                emit.Line("%s%s = %s%s;" % (parent, elem.cpp_name, cppParent, elem.TempName()))
                        # All other primitives and PODs
                        else:
                            emit.Line("%s%s = %s%s;" % (parent, elem.cpp_name, cppParent, elem.TempName() if not cppParent else elem.cpp_name))

                    EmitResponse(response)

                    emit.Unindent()
                    emit.Line("}")

            if isinstance(m, JsonProperty):
                if indexed and isinstance(m.index, JsonInteger):
                    index_converted = True
                    emit.Line("%s %s{};" % (m.index.cpp_native_type, m.index.TempName()))
                    emit.Line("if (Core::FromString(%s, %s) == false) {" % (m.index.cpp_name, m.index.TempName()))
                    emit.Indent()
                    emit.Line("// failed to convert the index")
                    emit.Line("%s = Core::ERROR_UNKNOWN_KEY;" % error_code.TempName())
                    if not m.writeonly and not m.readonly:
                        emit.Line("%s%s.Null(true);" % ("// " if isinstance(response, (JsonArray, JsonObject)) else "", response.cpp_name)) # FIXME
                    emit.Unindent()
                    emit.Line("} else {")
                    emit.Indent()
                elif indexed and isinstance(m.index, JsonEnum):
                    index_converted = True
                    emit.Line("Core::EnumerateType<%s> _value(%s.c_str());" % (m.index.cpp_native_type, m.index.cpp_name))
                    emit.Line("const %s %s = _value.Value();" % (m.index.cpp_native_type, m.index.TempName()))
                    emit.Line("if (_value.IsSet() == false) {")
                    emit.Indent()
                    emit.Line("// failed enum look-up")
                    emit.Line("%s = Core::ERROR_UNKNOWN_KEY;" % error_code.TempName())
                    if not m.writeonly and not m.readonly:
                        emit.Line("%s%s.Null(true);" % ("// " if isinstance(response, (JsonArray, JsonObject)) else "", response.cpp_name)) # FIXME
                    emit.Unindent()
                    emit.Line("} else {")
                    emit.Indent()
                if not m.readonly and not m.writeonly:
                    emit.Line("if (%s.IsSet() == false) {" % ((params.cpp_name + '.' + params.properties[0].cpp_name) if isinstance(params, JsonObject) else params.cpp_name))
                    emit.Indent()
                    emit.Line("// property get")
                elif m.readonly:
                    emit.Line("// read-only property get")
                if not m.writeonly:
                    Invoke(None, response, True, not m.readonly)
            else:
                Invoke(params, response, True, False, (params.cpp_name + '.') if isinstance(params, JsonObject) else None)

            if isinstance(m, JsonProperty) and not m.readonly:
                if not m.writeonly:
                    emit.Unindent()
                    emit.Line("} else {")
                    emit.Indent()
                    emit.Line("// property set")
                if m.writeonly:
                    emit.Line("// write-only property set")
                Invoke(params, None, False, False, ((params.cpp_name + '.') if isinstance(params, JsonObject) else None))
                if not m.writeonly:
                    emit.Line("%s%s.Null(true);" % ("// " if isinstance(response, (JsonArray, JsonObject)) else "", response.cpp_name)) # FIXME
                    emit.Unindent()
                    emit.Line("}")

            if indexed and index_converted:
                emit.Unindent()
                emit.Line("}")

            # Emit method epilogue
            emit.Line("return (%s);" % error_code.TempName())
            emit.Unindent()
            emit.Line("});")
            emit.Unindent()
            emit.Line()
        elif isinstance(m, JsonNotification):
            events.append(m)

    emit.Unindent()
    emit.Line("}")
    emit.Line()

    emit.Line("static void Unregister(PluginHost::JSONRPC& %s)" % module)
    emit.Line("{")
    emit.Indent()

    for m in root.properties:
        if isinstance(m, JsonMethod) and not isinstance(m, JsonNotification):
            emit.Line("%s.Unregister(_T(\"%s\"));" % (module, m.json_name))

    emit.Unindent()
    emit.Line("}")
    emit.Line()

    if events:
        emit.Line("namespace Event {")
        emit.Indent()
        emit.Line()
        for event in events:
            EmitEvent(emit, root, event, True)
        emit.Unindent()
        emit.Line("}; // namespace Event")
        emit.Line()

    emit.Unindent()
    emit.Line("}; // namespace %s" % struct)
    emit.Line()
    if "info" in root.schema and "namespace" in root.schema["info"]:
        emit.Unindent()
        emit.Line("} // namespace %s" % root.schema["info"]["namespace"])
        emit.Line()
    emit.Unindent()
    emit.Line("} // namespace %s" % "Exchange")
    emit.Line()
    emit.Line("}")
    emit.Line()


##############################################################################
#
# JSON-RPC STUB GENERATOR
#

def EmitHelperCode(root, emit, header_file):
    if DUMP_JSON:
        print("\n// JSON interface -----------")
        print(json.dumps(root.schema, indent=2))
        print("// ----------------\n")

    if root.objects:
        namespace = DATA_NAMESPACE + "::" + root.json_name

        def _ScopedName(obj):
            ns = DATA_NAMESPACE + "::" + (root.json_name if not obj.included_from else obj.included_from)
            obj_name = obj.cpp_type
            if obj_name != "void":
                if not obj_name.startswith(TYPE_PREFIX):
                    obj_name = ns + "::" + obj_name
                p = obj_name.find("<", 0)
                while p != -1:
                    if not obj_name.startswith(TYPE_PREFIX, p + 1):
                        obj_name = obj_name[:p + 1] + ns + "::" + obj_name[p + 1:]
                    p = obj_name.find("<", p + 1)
            return obj_name

        emit.Line("#include \"Module.h\"")
        emit.Line("#include \"%s.h\"" % root.json_name)
        emit.Line("#include <%s%s>" % (JSON_INTERFACE_PATH, header_file))
        for inc in root.includes:
            emit.Line("#include <%s%s_%s.h>" % (JSON_INTERFACE_PATH, DATA_NAMESPACE, inc))
        emit.Line()

        # Registration prototypes
        has_statuslistener = False
        for method in root.properties:
            if isinstance(method, JsonNotification) and method.is_status_listener:
                has_statuslistener = True
                break

        log.Info("Emitting registration code...")
        emit.Line("/*")
        emit.Indent()
        emit.Line("// Copy the code below to %s class definition" % root.json_name)
        emit.Line("// Note: The %s class must inherit from PluginHost::JSONRPC%s" %
                  (root.json_name, "SupportsEventStatus" if has_statuslistener else ""))
        emit.Line()
        emit.Line("private:")
        emit.Indent()
        emit.Line("void RegisterAll();")
        emit.Line("void UnregisterAll();")
        for method in root.properties:
            if not isinstance(method, JsonProperty) and not isinstance(method, JsonNotification):
                params = _ScopedName(method.params)
                response = _ScopedName(method.result)
                line = ("uint32_t %s(%s%s%s);" % (method.endpoint_name,
                                                  ("const " + params + "& params") if params != "void" else "",
                                                  ", " if params != "void" and response != "void" else "",
                                                  (response + "& response") if response != "void" else ""))
                if method.included_from:
                    line += " // %s" % method.included_from
                emit.Line(line)

        for method in root.properties:
            if isinstance(method, JsonProperty):
                if not method.writeonly:
                    response = _ScopedName(method.result)
                    line = "uint32_t %s(%s%s& response) const;" % (method.endpoint_get_name, ("const string& index, " if method.index else ""), response)
                    if method.included_from:
                        line += " // %s" % method.included_from
                    emit.Line(line)
                if not method.readonly:
                    params = _ScopedName(method.params)
                    line = "uint32_t %s(%sconst %s& param);" % (method.endpoint_set_name, ("const string& index, " if method.index else ""), params)
                    if method.included_from:
                        line += " // %s" % method.included_from
                    emit.Line(line)

        for method in root.properties:
            if isinstance(method, JsonNotification):
                params = _ScopedName(method.params)
                par = ""
                if params != "void":
                    if method.params.properties:
                        par = ", ".join(map(lambda x: "const " + GetNamespace(root, x) + x.cpp_native_type + "& " + x.json_name, method.params.properties))
                    else:
                        par = "const " + GetNamespace(root, method.params) + method.params.cpp_native_type + "& " + method.params.json_name
                line = ('void %s(%s%s);' % (method.endpoint_name, "const string& id, " if method.sendif_type  else "", par))
                if method.included_from:
                    line += " // %s" % method.included_from
                emit.Line(line)

        emit.Unindent()
        emit.Unindent()
        emit.Line("*/")
        emit.Line()

        # Registration code
        emit.Line("namespace %s {" % FRAMEWORK_NAMESPACE)
        emit.Line()
        emit.Line("namespace %s {" % PLUGIN_NAMESPACE)
        emit.Indent()
        emit.Line()
        emit.Line("using namespace %s;" % namespace)
        for inc in root.includes:
            emit.Line("using namespace %s::%s;" % (DATA_NAMESPACE, inc))
        emit.Line()
        emit.Line("// Registration")
        emit.Line("//")
        emit.Line()
        emit.Line("void %s::RegisterAll()" % root.json_name)
        emit.Line("{")
        emit.Indent()
        for method in root.properties:
            if isinstance(method, JsonNotification) and method.is_status_listener:
                emit.Line("RegisterEventStatusListener(_T(\"%s\"), [this](const string& client, Status status) {" % method.json_name)
                emit.Indent()
                emit.Line("const string id = client.substr(0, client.find('.'));")
                emit.Line("// TODO...")
                emit.Unindent()
                emit.Line("});")
                emit.Line()
        for method in root.properties:
            if not isinstance(method, JsonNotification) and not isinstance(method, JsonProperty):
                line = 'Register<%s,%s>(_T("%s"), &%s::%s, this);' % (
                    method.params.cpp_type, method.result.cpp_type, method.json_name, root.json_name, method.endpoint_name)
                if method.included_from:
                    line += " /* %s */" % method.included_from
                emit.Line(line)
        for method in root.properties:
            if isinstance(method, JsonProperty):
                line = ""
                if root.rpc_format != RpcFormat.COMPLIANT or method.readonly or method.writeonly:
                    line += 'Property<%s>(_T("%s")' % (method.params.cpp_type if not method.readonly else method.result.cpp_type, method.json_name)
                    line += ", &%s::%s" % (root.json_name, method.endpoint_get_name) if not method.writeonly else ", nullptr"
                    line += ", &%s::%s" % (root.json_name, method.endpoint_set_name) if not method.readonly else ", nullptr"
                    line += ', this);'
                else:
                    line = 'Register<%s,%s>(_T("%s"), ([this](const %s& Params, %s& Response) { if (Params.IsSet() == true) return(%s(Params)); else return(%s(Response); }), this);' % (
                        method.params.cpp_type, method.result.cpp_type, method.json_name,
                        method.params.cpp_type, method.result.cpp_type,
                        method.endpoint_set_name, method.endpoint_get_name)
                if method.included_from:
                    line += " /* %s */" % method.included_from
                emit.Line(line)
        emit.Unindent()
        emit.Line("}")
        emit.Line()
        emit.Line("void %s::UnregisterAll()" % root.json_name)
        emit.Line("{")
        emit.Indent()
        for method in reversed(root.properties):
            if not isinstance(method, JsonNotification) and not isinstance(method, JsonProperty):
                emit.Line('Unregister(_T("%s"));' % method.json_name)
        for method in reversed(root.properties):
            if isinstance(method, JsonProperty):
                emit.Line('Unregister(_T("%s"));' % method.json_name)
        for method in reversed(root.properties):
            if isinstance(method, JsonNotification) and method.is_status_listener:
                emit.Line("UnregisterEventStatusListener(_T(\"%s\");" % method.json_name)

        emit.Unindent()
        emit.Line("}")
        emit.Line()

        # Method/property/event stubs
        log.Info("Emitting stubs...")
        emit.Line("// API implementation")
        emit.Line("//")
        emit.Line()
        for method in root.properties:
            if not isinstance(method, JsonNotification) and not isinstance(method, JsonProperty):
                log.Info("Emitting method '{}'".format(method.json_name))
                params = method.params.cpp_type
                emit.Line("// Method: %s" % method.Headline())
                emit.Line("// Return codes:")
                emit.Line("//  - ERROR_NONE: Success")
                for e in method.errors:
                    description = e["description"] if "description" in e else ""
                    emit.Line("//  - %s: %s" % (e["message"], description))
                response = method.properties[1].cpp_type
                line = ("uint32_t %s::%s(%s%s%s)" % (root.json_name, method.endpoint_name,
                                                     ("const " + params + "& params") if params != "void" else "",
                                                     ", " if params != "void" and response != "void" else "",
                                                     (response + "& response") if response != "void" else ""))
                if method.included_from:
                    line += " /* %s */" % method.included_from
                emit.Line(line)
                emit.Line("{")
                emit.Indent()
                emit.Line("uint32_t result = Core::ERROR_NONE;")
                if params != "void":
                    for p in method.params.properties:
                        if not isinstance(p, (JsonObject, JsonArray)):
                            emit.Line("const %s& %s = params.%s.Value();" %
                                      (p.cpp_native_type, p.json_name, p.cpp_name))
                        else:
                            emit.Line("// params.%s ..." % p.cpp_name)
                emit.Line()
                emit.Line("// TODO...")
                emit.Line()
                if response != "void":
                    for p in method.properties[1].properties:
                        emit.Line("// response.%s = ..." % (p.cpp_name))
                    emit.Line()
                emit.Line("return result;")
                emit.Unindent()
                emit.Line("}")
                emit.Line()

        for method in root.properties:
            if isinstance(method, JsonProperty):

                def _EmitProperty(method, name, getter):
                    params = method.params.cpp_type if not getter else (method.result.cpp_type if not method.result.is_void else method.params.cpp_type)
                    emit.Line("// Property: %s" % method.Headline())
                    emit.Line("// Return codes:")
                    emit.Line("//  - ERROR_NONE: Success")
                    for e in method.errors:
                        description = e["description"] if "description" in e else ""
                        emit.Line("//  - %s: %s" % (e["message"], description))
                    line = "uint32_t %s::%s(%s%s%s& %s)%s" % (
                        root.json_name, name, "const string& index, " if method.index else "", "const " if not getter else "",
                            params, "response" if getter else "params", " const" if getter else "")
                    if method.included_from:
                        line += " /* %s */" % method.included_from
                    emit.Line(line)
                    emit.Line("{")
                    emit.Indent()

                    emit.Line("uint32_t result = Core::ERROR_NONE;")

                    if not getter and params != "void":
                        for p in method.params.properties:
                            if not isinstance(p, (JsonObject, JsonArray)):
                                emit.Line("const %s& %s = params.%s.Value();" % (p.cpp_native_type, p.json_name, p.cpp_name))
                            else:
                                emit.Line("// params.%s ..." % p.cpp_name)
                    emit.Line()
                    emit.Line("// TODO...")
                    if getter:
                        emit.Line("// response = ...")

                    emit.Line()
                    emit.Line("return result;")
                    emit.Unindent()
                    emit.Line("}")
                    emit.Line()

                propType = ' (write-only)' if method.writeonly else (' (read-only)' if method.readonly else '')
                log.Info("Emitting property '{}' {}".format(method.json_name, propType))
                if not method.writeonly:
                    _EmitProperty(method, method.endpoint_get_name, True)
                if not method.readonly:
                    _EmitProperty(method, method.endpoint_set_name, False)

        for method in root.properties:
            if isinstance(method, JsonNotification):
                log.Info("Emitting notification '{}'".format(method.json_name))
                EmitEvent(emit, root, method)

        emit.Unindent()
        emit.Line("} // namespace %s" % PLUGIN_NAMESPACE)
        emit.Line()
        emit.Line("}")
        emit.Line()


##############################################################################
#
# C++ OBJECT GENERATOR
#

def EmitObjects(root, emit, if_file, additional_includes, emitCommon=False):
    global emittedItems
    emittedItems = 0

    def EmitEnumConversionHandler(root, enum):
        fullname = GetNamespace(root, enum) + enum.cpp_class
        emit.Line("ENUM_CONVERSION_HANDLER(%s);" % fullname)

    def EmitEnum(enum):
        global emittedItems
        emittedItems += 1
        log.Info("Emitting enum {}".format(enum.cpp_class))
        root = enum.parent.parent
        while root.parent:
            root = root.parent
        if enum.description:
            emit.Line("// " + enum.description)
        emit.Line("enum%s %s : uint%i_t {" % (" class" if enum.is_scoped else "", enum.cpp_class, enum.size))
        emit.Indent()
        for c, item in enumerate(enum.cpp_enumerators):
            emit.Line("%s%s%s" % (item.upper(),
                                  (" = " + str(enum.cpp_enumerator_values[c])) if enum.cpp_enumerator_values else "",
                                  "," if not c == len(enum.cpp_enumerators) - 1 else ""))
        emit.Unindent()
        emit.Line("};")
        emit.Line()

    def EmitClass(json_obj, allow_duplicates=False):
        def EmitInit(json_object):
            for prop in json_obj.properties:
                emit.Line("Add(_T(\"%s\"), &%s);" % (prop.json_name, prop.cpp_name))

        def EmitCtor(json_obj, no_init_code=False, copy_ctor=False, conversion_ctor=False):
            if copy_ctor:
                emit.Line("%s(const %s& _other)" % (json_obj.cpp_class, json_obj.cpp_class))
            elif conversion_ctor:
                emit.Line("%s(const %s& _other)" % (json_obj.cpp_class, json_obj.cpp_native_type))
            else:
                emit.Line("%s()" % (json_obj.cpp_class))
            emit.Indent()
            emit.Line(": %s()" % TypePrefix("Container"))
            for prop in json_obj.properties:
                if copy_ctor:
                    emit.Line(", %s(_other.%s)" % (prop.cpp_name, prop.cpp_name))
                elif prop.cpp_def_value != '""' and prop.cpp_def_value != "":
                    emit.Line(", %s(%s)" % (prop.cpp_name, prop.cpp_def_value))
            emit.Unindent()
            emit.Line("{")
            emit.Indent()
            if conversion_ctor:
                for prop in json_obj.properties:
                    emit.Line("%s = _other.%s;" % (prop.cpp_name, prop.actual_name))
            if no_init_code:
                emit.Line("_Init();")
            else:
                EmitInit(json_obj)

            emit.Unindent()
            emit.Line("}")

        def EmitAssignmentOperator(json_obj, copy_ctor=False, conversion_ctor=False):
            if copy_ctor:
                emit.Line("%s& operator=(const %s& _rhs)" % (json_obj.cpp_class, json_obj.cpp_class))
            elif conversion_ctor:
                emit.Line("%s& operator=(const %s& _rhs)" % (json_obj.cpp_class, json_obj.cpp_native_type))
            emit.Line("{")
            emit.Indent()
            for prop in json_obj.properties:
                if copy_ctor:
                    emit.Line("%s = _rhs.%s;" % (prop.cpp_name, prop.cpp_name))
                elif conversion_ctor:
                    emit.Line("%s = _rhs.%s;" % (prop.cpp_name, prop.actual_name))
            emit.Line("return (*this);")
            emit.Unindent()
            emit.Line("}")

        def EmitConversionOperator(json_obj):
            emit.Line("operator %s() const" % (json_obj.cpp_native_type))
            emit.Line("{")
            emit.Indent();
            emit.Line("%s _value{};" % (json_obj.cpp_native_type))
            for prop in json_obj.properties:
                emit.Line("_value.%s = %s%s;" % ( prop.actual_name, prop.cpp_name, ".Value()" if not isinstance(prop, JsonObject) else ""))
            emit.Line("return (_value);")
            emit.Unindent()
            emit.Line("}")

        # Bail out if a duplicated class!
        if isinstance(json_obj, JsonObject) and not json_obj.properties:
            return
        if  json_obj.is_duplicate or (not allow_duplicates and json_obj.RefCount() > 1):
            return
        if not isinstance(json_obj, (JsonRpcSchema, JsonMethod)):
            log.Info("Emitting class '{}' (source: '{}')".format(json_obj.cpp_class, json_obj.print_name))
            emit.Line("class %s : public %s {" % (json_obj.cpp_class, TypePrefix("Container")))
            emit.Line("public:")
            for enum in json_obj.enums:
                if (enum.do_create and not enum.is_duplicate and (enum.RefCount() == 1)):
                    emit.Indent()
                    EmitEnum(enum)
                    emit.Unindent()
            emit.Indent()
        else:
            if isinstance(json_obj, JsonMethod):
                if json_obj.included_from:
                    return

        # Handle nested classes!
        for obj in SortByDependency(json_obj.objects):
            EmitClass(obj)

        if not isinstance(json_obj, (JsonRpcSchema, JsonMethod)):
            global emittedItems
            emittedItems += 1
            EmitCtor(json_obj, json_obj.is_copy_ctor_needed or "original_type" in json_obj.schema, False, False)
            if json_obj.is_copy_ctor_needed:
                emit.Line()
                EmitCtor(json_obj, True, True, False)
                emit.Line()
                EmitAssignmentOperator(json_obj, True, False)
            if "original_type" in json_obj.schema:
                emit.Line()
                EmitCtor(json_obj, True, False, True)
                emit.Line()
                EmitAssignmentOperator(json_obj, False, True)
                emit.Line()
                EmitConversionOperator(json_obj)
            if json_obj.is_copy_ctor_needed or ("original_type" in json_obj.schema):
                emit.Unindent()
                emit.Line()
                emit.Line("private:")
                emit.Indent()
                emit.Line("void _Init()")
                emit.Line("{")
                emit.Indent()
                EmitInit(json_obj)
                emit.Unindent()
                emit.Line("}")
                emit.Line()
            else:
                emit.Line()
                emit.Line("%s(const %s&) = delete;" % (json_obj.cpp_class, json_obj.cpp_class))
                emit.Line("%s& operator=(const %s&) = delete;" % (json_obj.cpp_class, json_obj.cpp_class))
                emit.Line()

            emit.Unindent()
            emit.Line("public:")
            emit.Indent()
            for prop in json_obj.properties:
                comment = prop.print_name if isinstance(prop, JsonMethod) else prop.description
                emit.Line("%s %s;%s" % (prop.cpp_type, prop.cpp_name, (" // " + comment) if comment else ""))
            emit.Unindent()
            emit.Line("}; // class %s" % json_obj.cpp_class)
            emit.Line()

    count = 0
    if enumTracker.objects:
        count = 0
        for obj in enumTracker.objects:
            if obj.do_create and not obj.is_duplicate and not obj.included_from:
                count += 1

    emit.Line("#pragma once")
    emit.Line()
    emit.Line("#include <core/JSON.h>")
    if if_file.endswith(".h"):
        emit.Line("#include <%s%s>" % (CPP_INTERFACE_PATH, if_file))
    for ai in additional_includes:
        emit.Line("#include <%s%s>" % (CPP_INTERFACE_PATH, os.path.basename(ai)))
    if count:
        emit.Line("#include <core/Enumerate.h>")
    emit.Line()
    emit.Line("namespace %s {" % FRAMEWORK_NAMESPACE)
    emit.Line()
    emit.Line("namespace %s {" % DATA_NAMESPACE)
    emit.Indent()
    emit.Line()
    if "info" in root.schema and "namespace" in root.schema["info"]:
        emit.Line("namespace %s {" % root.schema["info"]["namespace"])
        emit.Indent()
        emit.Line()
    emit.Line("namespace %s {" % root.json_name)
    emit.Indent()
    emit.Line()
    if emitCommon and enumTracker.CommonObjects():
        emittedPrologue = False
        for obj in enumTracker.CommonObjects():
            if obj.do_create and not obj.is_duplicate and not obj.included_from:
                if not emittedPrologue:
                    log.Info("Emitting common enums...")
                    emit.Line("// Common enums")
                    emit.Line("//")
                    emit.Line()
                    emittedPrologue = True
                EmitEnum(obj)
    if emitCommon and objTracker.CommonObjects():
        log.Info("Emitting common classes...")
        emittedPrologue = False
        for obj in objTracker.CommonObjects():
            if not obj.included_from:
                if not emittedPrologue:
                    emit.Line("// Common classes")
                    emit.Line("//")
                    emit.Line()
                    emittedPrologue = True
                EmitClass(obj, True)
    if root.objects:
        log.Info("Emitting params/result classes...")
        emit.Line("// Method params/result classes")
        emit.Line("//")
        emit.Line()
        EmitClass(root)
    emit.Unindent()
    emit.Line("} // namespace %s" % root.json_name)
    emit.Line()
    if "info" in root.schema and "namespace" in root.schema["info"]:
        emit.Unindent()
        emit.Line("} // namespace %s" % root.schema["info"]["namespace"])
        emit.Line()
    emit.Unindent()
    emit.Line("} // namespace %s" % DATA_NAMESPACE)
    emit.Line()
    emittedPrologue = False
    for obj in enumTracker.objects:
        if not obj.is_duplicate and not obj.included_from:
            if not emittedPrologue:
                emit.Line("// Enum conversion handlers")
                emittedPrologue = True
            EmitEnumConversionHandler(root, obj)
            emittedItems += 1
    emit.Line()
    emit.Line("}")
    emit.Line()
    return emittedItems



##############################################################################
#
# DOCUMENTATION GENERATOR
#

def GetVersion(info):
    if "version" in info:
        if isinstance(info["version"], list):
            version = info["version"]
            if len(version) == 0:
                version.apend(1)
            if len(version) == 1:
                version.append(0)
            if len(version) == 2:
                version.append(0)
        else:
            raise RuntimeError("version needs to be a [major, minor, patch] list of integers")
    else:
        log.Warn("No version provided for %s interface, using 1.0.0" % info["class"])
        version = [1, 0, 0]

    return version

def GetVersionString(info):
    return ".".join(str(x) for x in GetVersion(info))

def CreateDocument(schema, path):
    input_basename = os.path.basename(path)
    output_path = os.path.dirname(path) + "/" + input_basename.replace(".json", "") + ".md"
    with open(output_path, "w") as output_file:
        emit = Emitter(output_file, INDENT_SIZE)

        def bold(string):
            return "**%s**" % string

        def italics(string):
            return "*%s*" % string

        def link(string):
            return "[%s](#%s)" % (string.split(".", 1)[1].replace("_", " "), string)

        def weblink(string, link):
            return "[%s](%s)" % (string,link)

        def MdBr():
            emit.Line()

        def MdHeader(string, level=1, id="head", section=None):
            if level < 3:
                emit.Line("<a name=\"%s\"></a>" % (id + "." + string.replace(" ", "_")))
            if id != "head":
                string += " [<sup>%s</sup>](#head.%s)" % (id, section)
            emit.Line("%s %s" % ("#" * level, "*%s*" % string if id != "head" else string))
            MdBr()

        def MdBody(string=""):
            emit.Line(string)

        def MdParagraph(string):
            MdBody(string)
            MdBr()

        def MdCode(string, lang):
            emit.Line("```" + lang)
            emit.Line(string)
            emit.Line("```")
            emit.Line()

        def MdRow(cols):
            row = "|"
            for c in cols:
                row += " %s |" % c
            emit.Line(row)

        def MdTableHeader(columns):
            MdRow(columns)
            MdRow([":--------"] * len(columns))

        def ParamTable(name, object):
            MdTableHeader(["Name", "Type", "Description"])

            def _TableObj(name, obj, parentName="", parent=None, prefix="", parentOptional=False):
                # determine if the attribute is optional
                optional = parentOptional or (obj["optional"] if "optional" in obj else False)
                deprecated = obj["deprecated"] if "deprecated" in obj else False
                obsolete = obj["obsolete"] if "obsolete" in obj else False
                if parent and not optional:
                    if parent["type"] == "object":
                        optional = ("required" not in parent and len(parent["properties"]) > 1) or (
                            "required" in parent
                            and name not in parent["required"]) or ("required" in parent and len(parent["required"]) == 0)

                name = (name if not "original_name" in obj else obj["original_name"].lower())
                # include information about enum values in description
                enum = ' (must be one of the following: %s)' % (", ".join(
                    '*{0}*'.format(w) for w in obj["enum"])) if "enum" in obj else ""
                if parent and prefix and parent["type"] == "object":
                    prefix += "?." if optional else "."
                prefix += name
                description = obj["description"] if "description" in obj else obj["summary"] if "summary" in obj else ""

                if name or prefix:
                    if "type" not in obj:
                        raise RuntimeError("missing 'type' for object %s" % (parentName + "/" + name))
                    row = (("<sup>" + italics("(optional)") + "</sup>" + " ") if optional else "")
                    if deprecated:
                        row = "<sup>" + italics("(deprecated)") + "</sup> " + row
                    if obsolete:
                        row = "<sup>" + italics("(obsolete)") + "</sup> " + row
                    row += description + enum
                    if row.endswith('.'):
                        row = row[:-1]
                    if optional and "default" in obj:
                        row += " (default: " + (italics("%s") % str(obj["default"]) + ")")
                    MdRow([prefix, obj["type"], row])

                if obj["type"] == "object":
                    if "required" not in obj and name and len(obj["properties"]) > 1:
                        log.Warn("'%s': no 'required' field present (assuming all members optional)" % name)
                    for pname, props in obj["properties"].items():
                        _TableObj(pname, props, parentName + "/" + (name if not "original_name" in props else props["original_name"].lower()), obj, prefix, False)
                elif obj["type"] == "array":
                    _TableObj("", obj["items"], parentName + "/" + name, obj, (prefix + "[#]") if name else "", optional)

            _TableObj(name, object, "")
            MdBr()

        def ErrorTable(obj):
            MdTableHeader(["Code", "Message", "Description"])
            for err in obj:
                description = err["description"] if "description" in err else ""
                MdRow([err["code"] if "code" in err else "", "```" + err["message"] + "```", description])
            MdBr()

        def PlainTable(obj, columns, ref="ref"):
            MdTableHeader(columns)
            for prop, val in sorted(obj.items()):
                MdRow(["<a name=\"%s.%s\">%s</a>" % (ref, (prop.split("]", 1)[0][1:]) if "]" in prop else prop, prop), val])
            MdBr()

        def ExampleObj(name, obj, root=False):
            if "deprecated" in obj and obj["deprecated"]:
                return "$deprecated"
            obj_type = obj["type"]
            default = obj["example"] if "example" in obj else obj["default"] if "default" in obj else ""
            if not default and "enum" in obj:
                default = obj["enum"][0]
            json_data = '"%s": ' % name if name else ''
            if obj_type == "string":
                json_data += '"%s"' % (default if default else "...")
            elif obj_type in ["integer", "number"]:
                json_data += '%s' % (default if default else 0)
            elif obj_type in ["float", "double"]:
                json_data += '%s' % (default if default else 0.0)
            elif obj_type == "boolean":
                json_data += '%s' % str(default if default else False).lower()
            elif obj_type == "null":
                json_data += 'null'
            elif obj_type == "array":
                json_data += str(default if default else ('[ %s ]' % (ExampleObj("", obj["items"]))))
            elif obj_type == "object":
                json_data += "{ %s }" % ", ".join(
                    list(map(lambda p: ExampleObj(p, obj["properties"][p]),
                             obj["properties"]))[0:obj["maxProperties"] if "maxProperties" in obj else None])
                json_data = json_data.replace("$deprecated, ", "")
            return json_data

        def MethodDump(method, props, classname, section, is_notification=False, is_property=False, include=None):
            method = (method.rsplit(".", 1)[1] if "." in method else method)
            type =  "property" if is_property else "event" if is_notification else "method"
            log.Info("Emitting documentation for %s '%s'..." % (type, method))
            MdHeader(method, 2, type, section)
            readonly = False
            writeonly = False
            if "summary" in props:
                text = props["summary"]
                if is_property:
                    text = "Provides access to the " + \
                        (text[0].lower() if text[1].islower()
                         else text[0]) + text[1:]
                if not text.endswith('.'):
                    text += '.'
                MdParagraph(text)
            if is_property:
                if "readonly" in props and props["readonly"] == True:
                    MdParagraph("> This property is **read-only**.")
                    readonly = True
                elif "writeonly" in props and props["writeonly"] == True:
                    writeonly = True
                    MdParagraph("> This property is **write-only**.")
            if "deprecated" in props:
                MdParagraph("> This API is **deprecated** and may be removed in the future. It is no longer recommended for use in new implementations.")
            elif "obsolete" in props:
                MdParagraph("> This API is **obsolete**. It is no longer recommended for use in new implementations.")
            if "description" in props:
                MdHeader("Description", 3)
                MdParagraph(props["description"])
            if "events" in props:
                events = [props["events"]] if isinstance(props["events"], str) else props["events"]
                MdParagraph("Also see: " + (", ".join(map(lambda x: link("event." + x), events))))
            if is_property:
                MdHeader("Value", 3)
                if "params" in props:
                    if not "description" in props["params"]:
                        if "summary" in props:
                            props["params"]["description"] = props["summary"]
                    ParamTable("(property)", props["params"])
                elif "result" in props:
                    if not "description" in props["result"]:
                        if "summary" in props:
                            props["result"]["description"] = props["summary"]
                if "index" in props:
                    if "name" not in props["index"] or "example" not in props["index"]:
                        raise RuntimeError("in %s: index field needs 'name' and 'example' properties" % method)
                    extra_paragraph = "> The *%s* argument shall be passed as the index to the property, e.g. *%s.1.%s@%s*.%s" % (
                        props["index"]["name"].lower(), classname, method, props["index"]["example"],
                        (" " + props["index"]["description"]) if "description" in props["index"] else "")
                    if not extra_paragraph.endswith('.'):
                        extra_paragraph += '.'
                    MdParagraph(extra_paragraph)
            else:
                MdHeader("Parameters", 3)
                if "params" in props:
                    ParamTable("params", props["params"])
                else:
                    if is_notification:
                        MdParagraph("This event carries no parameters.")
                    else:
                        MdParagraph("This method takes no parameters.")
                if is_notification:
                    if "id" in props:
                        if "name" not in props["id"] or "example" not in props["id"]:
                            raise RuntimeError("in %s: id field needs 'name' and 'example' properties" % method)
                        MdParagraph("> The *%s* argument shall be passed within the designator, e.g. *%s.client.events.1*." %
                                    (props["id"]["name"], props["id"]["example"]))

            if "result" in props:
                MdHeader("Result", 3)
                ParamTable("result", props["result"])
            if not is_notification and "errors" in props:
                MdHeader("Errors", 3)
                ErrorTable(props["errors"])

            MdHeader("Example", 3)

            if is_notification:
                method = "client.events.1." + method
            elif is_property:
                method = "%s.1.%s%s" % (classname, method, ("@" + props["index"]["example"]) if "index" in props and "example" in props["index"] else "")
            else:
                method = "%s.1.%s" % (classname, method)
            if "id" in props and "example" in props["id"]:
                method = props["id"]["example"] + "." + method

            if is_property:
                if not writeonly:
                    MdHeader("Get Request", 4)
                    jsonRequest = json.dumps(json.loads('{ "jsonrpc": "2.0", "id": 42, "method": "%s" }' % method,
                                                        object_pairs_hook=OrderedDict),
                                            indent=4)
                    MdCode(jsonRequest, "json")
                    MdHeader("Get Response", 4)
                    parameters = (props["result"] if "result" in props else (props["params"] if "params" in props else None))
                    jsonResponse = json.dumps(json.loads('{ "jsonrpc": "2.0", "id": 42, %s }' % ExampleObj("result", parameters, True),
                                                         object_pairs_hook=OrderedDict),
                                              indent=4)
                    MdCode(jsonResponse, "json")

            if not readonly:
                if not is_notification:
                    if is_property:
                        MdHeader("Set Request", 4)
                    else:
                        MdHeader("Request", 4)

                jsonRequest = json.dumps(json.loads('{ "jsonrpc": "2.0", %s"method": "%s"%s }' %
                                                    ('"id": 42, ' if not is_notification else "", method,
                                                    (", " + ExampleObj("params", props["params"], True)) if "params" in props else ""),
                                                    object_pairs_hook=OrderedDict),
                                         indent=4)
                MdCode(jsonRequest, "json")

                if not is_notification and not is_property:
                    if "result" in props:
                        MdHeader("Response", 4)
                        jsonResponse = json.dumps(json.loads('{ "jsonrpc": "2.0", "id": 42, %s }' % ExampleObj("result", props["result"], True),
                                                             object_pairs_hook=OrderedDict),
                                                  indent=4)
                        MdCode(jsonResponse, "json")
                    elif "noresult" not in props or not props["noresult"]:
                        raise RuntimeError("missing 'result' in %s" % method)

                if is_property:
                    MdHeader("Set Response", 4)
                    jsonResponse = json.dumps(json.loads('{ "jsonrpc": "2.0", "id": 42, "result": "null" }',
                                                         object_pairs_hook=OrderedDict),
                                              indent=4)
                    MdCode(jsonResponse, "json")

        commons = dict()
        with open(os.path.join(os.path.dirname(os.path.realpath(__file__)), GLOBAL_DEFINITIONS)) as f:
            commons = json.load(f)

        global_rpc_format = RPC_FORMAT
        if (not RPC_FORMAT_FORCED) and ("info" in schema) and ("format" in schema["info"]):
            global_rpc_format = RpcFormat(schema["info"]["format"])

        # The interfaces defined can be a single item, a list, or a list of list.
        # So first flatten the structure and make it consistent to be always a list.
        outer_interfaces = schema
        if "interface" in schema:
            outer_interfaces = schema["interface"]
        if not isinstance(outer_interfaces, list):
            outer_interfaces = [outer_interfaces]

        interfaces = []
        for interface in outer_interfaces:
            rpc_format = RPC_FORMAT
            if (not RPC_FORMAT_FORCED) and ("info" in interface) and ("format" in interface["info"]):
                rpc_format = RpcFormat(interface["info"]["format"])

            if isinstance(interface, list):
                interfaces.extend(interface)
                for face in interface:
                    if "include" in face:
                        if isinstance(face["include"], list):
                            interfaces.extend(face["include"])
                        else:
                            interfaces.append(face["include"])
                    interfaces[-1]["info"]["format"] = rpc_format.value
            else:
                interfaces.append(interface)
                if "include" in interface:
                    for face in interface["include"]:
                        interfaces.append(interface["include"][face])
                        interfaces[-1]["info"]["format"] = rpc_format.value

        # Don't consider all interfaces for processing
        interfaces = [interface for interface in interfaces if "@generated" not in interface or "@dorpc" in interface]

        version = "1.0" # Plugin version, not interface version

        if "info" in schema:
            info = schema["info"]
        else:
            info = dict()

        # Count the total number of methods, properties and events.
        method_count = 0
        property_count = 0
        event_count = 0
        for interface in interfaces:
            if "methods" in interface:
                method_count += len(interface["methods"])
            if "properties" in interface:
                property_count += len(interface["properties"])
            if "events" in interface:
                event_count += len(interface["events"])

        status = info["status"] if "status" in info else "alpha"

        rating = 0
        if status == "dev" or status == "development":
            rating = 0
        elif status == "alpha":
            rating = 1
        elif status == "beta":
            rating = 2
        elif status == "production" or status == "prod":
            rating = 3
        else:
            raise RuntimeError("invalid status")

        plugin_class = None
        if "callsign" in info:
            plugin_class = info["callsign"]
        elif "class" in info:
            plugin_class = info["class"]
        else:
            raise RuntimeError("missing class in 'info'")

        def SourceRevision(face):
            sourcerevision = DEFAULT_INTERFACE_SOURCE_REVISION
            if INTERFACE_SOURCE_REVISION:
                sourcerevision = INTERFACE_SOURCE_REVISION
            elif "sourcerevision" in face["info"]:
                sourcerevision = face["info"]["sourcerevision"]
            elif "sourcerevision" in info:
                sourcerevision = info["sourcerevision"]
            elif "common" in face and "sourcerevision" in face["common"]:
                sourcerevision = face["common"]["sourcerevision"]
            elif "sourcerevision" in commons:
                sourcerevision = commons["sourcerevision"]
            return sourcerevision

        def SourceLocation(face):
            sourcelocation = None
            if INTERFACE_SOURCE_LOCATION:
                sourcelocation = INTERFACE_SOURCE_LOCATION
            elif "sourcelocation" in face["info"]:
                sourcelocation = face["info"]["sourcelocation"]
            elif "sourcelocation" in info:
                sourcelocation = info["sourcelocation"]
            elif "common" in face and "sourcelocation" in face["common"]:
                sourcelocation = face["common"]["sourcelocation"]
            elif "sourcelocation" in commons:
                sourcelocation = commons["sourcelocation"]
            if sourcelocation:
                sourcelocation = sourcelocation.replace("{interfacefile}", face["info"]["sourcefile"] if "sourcefile" in face["info"]
                                        else ((face["info"]["class"] if "class" in face["info"] else input_basename) + ".json"))
                sourcerevision = SourceRevision(face)
                if sourcerevision:
                    sourcelocation = sourcelocation.replace("{revision}", sourcerevision)
            return sourcelocation

        document_type = "plugin"
        if ("@interfaceonly" in schema and schema["@interfaceonly"]) or ("$schema" in schema and schema["$schema"] == "interface.schema.json"):
            document_type = "interface"
            version = GetVersionString(info)

        MdBody("<!-- Generated automatically, DO NOT EDIT! -->")

        # Emit title bar
        if "title" in info:
            MdHeader(info["title"])
        MdParagraph(bold("Version: " + version))
        MdParagraph(bold("Status: " + rating * ":black_circle:" + (3 - rating) * ":white_circle:"))
        MdParagraph("%s %s for Thunder framework." % (plugin_class, document_type))

        if document_type == "interface":
            face = interfaces[0]
            sourcelocation = SourceLocation(interfaces[0])
            iface = (face["info"]["interface"] if "interface" in face["info"] else (face["info"]["class"] + ".json"))
            if sourcelocation:
                if "sourcefile" in face["info"]:
                    wl = "with " + iface + " in " + weblink(face["info"]["sourcefile"], sourcelocation)
                else:
                    wl = "by " + weblink(iface, sourcelocation)
            else:
                wl = iface
            MdBody("(Defined %s)" % wl)
            MdBr()

        # Emit TOC
        MdHeader("Table of Contents", 3)
        MdBody("- " + link("head.Introduction"))
        if "description" in info:
            MdBody("- " + link("head.Description"))
        if document_type == "plugin":
            MdBody("- " + link("head.Configuration"))
        if INTERFACES_SECTION and (document_type == "plugin") and (method_count or property_count or event_count):
            MdBody("- " + link("head.Interfaces"))
        if method_count:
            MdBody("- " + link("head.Methods"))
        if property_count:
            MdBody("- " + link("head.Properties"))
        if event_count:
            MdBody("- " + link("head.Notifications"))
        MdBr()

        def mergedict(d1, d2, prop):
            tmp = dict()
            if prop in d1:
                tmp.update(d1[prop])
            if prop in d2:
                tmp.update(d2[prop])
            return tmp


        MdHeader("Introduction")
        MdHeader("Scope", 2)
        if "scope" in info:
            MdParagraph(info["scope"])
        elif "title" in info:
            extra = ""
            if document_type == 'plugin':
                extra = "configuration"
            if method_count and property_count and event_count:
                if extra:
                    extra += ", "
                extra += "methods and properties provided, as well as notifications sent"
            elif method_count and property_count:
                if extra:
                    extra += ", "
                extra += "methods and properties provided"
            elif method_count and event_count:
                if extra:
                    extra += ", "
                extra += "methods provided and notifications sent"
            elif property_count and event_count:
                if extra:
                    extra += ", "
                extra += "properties provided and notifications sent"
            elif method_count:
                if extra:
                    extra += " and "
                extra += "methods provided"
            elif property_count:
                if extra:
                    extra += " and "
                extra += "properties provided"
            elif event_count:
                if extra:
                    extra += " and "
                extra += "notifications sent"
            if extra:
                extra = " It includes detailed specification about its " + extra + "."
            MdParagraph("This document describes purpose and functionality of the %s %s%s.%s" %
                (plugin_class, document_type, (" (version %s)" % version if document_type == "interface" else ""), extra))

        MdHeader("Case Sensitivity", 2)
        MdParagraph((
            "All identifiers of the interfaces described in this document are case-sensitive. "
            "Thus, unless stated otherwise, all keywords, entities, properties, relations and actions should be treated as such."
        ))

        if "acronyms" in info or "acronyms" in commons or "terms" in info or "terms" in commons:
            MdHeader("Acronyms, Abbreviations and Terms", 2)
            if "acronyms" in info or "acronyms" in commons:
                MdParagraph("The table below provides and overview of acronyms used in this document and their definitions.")
                PlainTable(mergedict(commons, info, "acronyms"), ["Acronym", "Description"], "acronym")
            if "terms" in info or "terms" in commons:
                MdParagraph("The table below provides and overview of terms and abbreviations used in this document and their definitions.")
                PlainTable(mergedict(commons, info, "terms"), ["Term", "Description"], "term")

        if "standards" in info:
            MdHeader("Standards", 2)
            MdParagraph(info["standards"])

        if "references" in commons or "references" in info:
            MdHeader("References", 2)
            PlainTable(mergedict(commons, info, "references"), ["Ref ID", "Description"])

        if "description" in info:
            MdHeader("Description")
            description = " ".join(info["description"]) if isinstance(info["description"], list) else info["description"]
            if not description.endswith('.'):
                description += '.'
            MdParagraph(description)
            if document_type == "plugin":
                MdParagraph(("The plugin is designed to be loaded and executed within the Thunder framework. "
                            "For more information about the framework refer to [[Thunder](#ref.Thunder)]."))

        if document_type == "plugin":
            MdHeader("Configuration")
            commonConfig = OrderedDict()
            if "configuration" in schema and "nodefault" in schema["configuration"] and schema["configuration"]["nodefault"] and "properties" not in schema["configuration"]:
                MdParagraph("The plugin does not take any configuration.")
            else:
                MdParagraph("The table below lists configuration options of the plugin.")
                if "configuration" not in schema or ("nodefault" not in schema["configuration"] or not schema["configuration"]["nodefault"]):
                    if "callsign" in info:
                        commonConfig["callsign"] = {
                            "type": "string",
                            "description": 'Plugin instance name (default: *%s*)' % info["callsign"]
                        }
                    if plugin_class:
                        commonConfig["classname"] = {"type": "string", "description": 'Class name: *%s*' % plugin_class}
                    if "locator" in info:
                        commonConfig["locator"] = {"type": "string", "description": 'Library name: *%s*' % info["locator"]}
                    commonConfig["autostart"] = {
                        "type": "boolean",
                        "description": "Determines if the plugin shall be started automatically along with the framework"
                    }

                required = []
                if "configuration" in schema:
                    commonConfig2 = OrderedDict(list(commonConfig.items()) + list(schema["configuration"]["properties"].items()))
                    required = schema["configuration"]["required"] if "required" in schema["configuration"] else []
                else:
                    commonConfig2 = commonConfig

                totalConfig = OrderedDict()
                totalConfig["type"] = "object"
                totalConfig["properties"] = commonConfig2
                if "configuration" not in schema or ("nodefault" not in schema["configuration"] or not schema["configuration"]["nodefault"]):
                    totalConfig["required"] = ["callsign", "classname", "locator", "autostart"] + required

                ParamTable("", totalConfig)

            if INTERFACES_SECTION and (document_type == "plugin") and (method_count or property_count or event_count):
                MdHeader("Interfaces")
                MdParagraph("This plugin implements the following interfaces:")
                for face in interfaces:
                    iface = (face["info"]["interface"] if "interface" in face["info"] else (face["info"]["class"] + ".json"))
                    sourcelocation = SourceLocation(face)
                    if sourcelocation:
                        if "sourcefile" in face["info"]:
                            wl = iface + " (" + weblink(face["info"]["sourcefile"], sourcelocation) + ")"
                        else:
                            wl = weblink(iface, sourcelocation)
                    else:
                        wl = iface
                    format = face["info"]["format"] if "format" in face["info"] else global_rpc_format.value
                    MdBody("- %s (version %s) (%s format)" % (wl, GetVersionString(face["info"]), format))
                MdBr()

        def SectionDump(section_name, section, header, description=None, description2=None, event=False, prop=False):
            skip_list = []

            def InterfaceDump(interface, section, header):
                head = False
                emitted = False
                if section in interface:
                    for method, contents in interface[section].items():
                        if contents and method not in skip_list:
                            ns = interface["info"]["namespace"] if "namespace" in interface["info"] else ""
                            if not head:
                                MdParagraph("%s interface %s:" % (((ns + " ") if ns else "") + interface["info"]["class"], section))
                                MdTableHeader([header.capitalize(), "Description"])
                                head = True
                            access = ""
                            if "readonly" in contents and contents["readonly"] == True:
                                access = "RO"
                            elif "writeonly" in contents and contents["writeonly"] == True:
                                access = "WO"
                            if access:
                                access = " <sup>%s</sup>" % access
                            tags = ""
                            if "obsolete" in contents and contents["obsolete"]:
                                tags += "<sup>obsolete</sup> "
                            if "deprecated" in contents and contents["deprecated"]:
                                tags += "<sup>deprecated</sup> "
                            descr = ""
                            if "summary" in contents:
                                descr = contents["summary"]
                                if "e.g" in descr:
                                    descr = descr[0:descr.index("e.g") - 1]
                                if "i.e" in descr:
                                    descr = descr[0:descr.index("i.e") - 1]
                                descr = descr.split(".", 1)[0] if "." in descr else descr
                            MdRow([tags + link(header + "." + (method.rsplit(".", 1)[1] if "." in method else method)) + access, descr])
                            emitted = True
                        skip_list.append(method)
                    if emitted:
                        MdBr()

            MdHeader(section_name)
            if description:
                MdParagraph(description)

            MdParagraph("The following %s are provided by the %s %s:" % (section, plugin_class, document_type))

            for interface in interfaces:
                InterfaceDump(interface, section, header)

            MdBr()
            if description2:
                MdParagraph(description2)
                MdBr()

            skip_list = []

            for interface in interfaces:
                if section in interface:
                    for method, props in interface[section].items():
                        if props and method not in skip_list:
                            MethodDump(method, props, plugin_class, section_name, event, prop)
                        skip_list.append(method)

        if method_count:
            SectionDump("Methods", "methods", "method")

        if property_count:
            SectionDump("Properties", "properties", "property", prop=True)

        if event_count:
            SectionDump("Notifications",
                        "events",
                        "event",
                        ("Notifications are autonomous events, triggered by the internals of the implementation, "
                         "and broadcasted via JSON-RPC to all registered observers. "
                         "Refer to [[Thunder](#ref.Thunder)] for information on how to register for a notification."),
                        event=True)

        log.Success("Document created: %s" % output_path)


##############################################################################
#
# Entry point
#

def ParseJsonRpcSchema(schema):
    objTracker.Reset()
    enumTracker.Reset()
    if "interface" in schema:
        schema = schema["interface"]
    if "info" in schema:
        if "class" in schema["info"]:
            pluginClass = schema["info"]["class"]
        else:
            pluginClass = "undefined_class"
            log.Error("no \"class\" defined in \"info\"")
        return JsonRpcSchema(pluginClass, schema)
    else:
        return None

def CreateCode(schema, source_file, path, additional_includes, generateClasses, generateStubs, generateRpc):
    directory = os.path.dirname(path)
    filename = (schema["info"]["namespace"]) if "info" in schema and "namespace" in schema["info"] else ""
    filename += (schema["info"]["class"]) if "info" in schema and "class" in schema["info"] else ""
    if len(filename) == 0:
        filename = os.path.basename(path.replace("Plugin", "").replace(".json", "").replace(".h", ""))
    rpcObj = ParseJsonRpcSchema(schema)
    if rpcObj:
        header_file = os.path.join(directory, DATA_NAMESPACE + "_" + filename + ".h")
        enum_file = os.path.join(directory, "JsonEnum_" + filename + ".cpp")

        data_emitted = 0
        if generateClasses:
            if not FORCE and (os.path.exists(header_file) and (os.path.getmtime(source_file) < os.path.getmtime(header_file))):
                log.Success("skipping file %s, up-to-date" % os.path.basename(header_file))
                data_emitted = 1
            else:
                with open(header_file, "w") as output_file:
                    emitter = Emitter(output_file, INDENT_SIZE)
                    emitter.Line()
                    emitter.Line("// C++ classes for %s JSON-RPC API." % rpcObj.info["title"].replace("Plugin", "").strip())
                    emitter.Line("// Generated automatically from '%s'. DO NOT EDIT." % os.path.basename(source_file))
                    emitter.Line()
                    emitter.Line("// Note: This code is inherently not thread safe. If required, proper synchronisation must be added.")
                    emitter.Line()
                    data_emitted = EmitObjects(rpcObj, emitter, os.path.basename(source_file), additional_includes, True)
                    if data_emitted:
                        log.Success("JSON data classes generated in %s" % os.path.basename(output_file.name))
                    else:
                        log.Info("No JSON data classes generated for %s" % os.path.basename(filename))
                if not data_emitted and not KEEP_EMPTY:
                    try:
                        os.remove(header_file)
                    except:
                        pass

            if not FORCE and (os.path.exists(enum_file) and (os.path.getmtime(source_file) < os.path.getmtime(enum_file))):
                log.Success("skipping file %s, up-to-date" % os.path.basename(enum_file))
            else:
                enum_emitted = 0
                with open(enum_file, "w") as output_file:
                    emitter = Emitter(output_file, INDENT_SIZE)
                    emitter.Line()
                    emitter.Line("// Enumeration code for %s JSON-RPC API." %
                                rpcObj.info["title"].replace("Plugin", "").strip())
                    emitter.Line("// Generated automatically from '%s'." % os.path.basename(source_file))
                    emitter.Line()
                    enum_emitted = EmitEnumRegs(rpcObj, emitter, filename, os.path.basename(source_file))
                    if enum_emitted:
                        log.Success("JSON enumeration code generated in %s" % os.path.basename(output_file.name))
                    else:
                        log.Info("No JSON enumeration code generated for %s" % os.path.basename(filename))
                if not enum_emitted and not KEEP_EMPTY:
                    try:
                        os.remove(enum_file)
                    except:
                        pass

            if not generateRpc or "@dorpc" not in rpcObj.schema:
                rpc_file = os.path.join(directory, "J" + filename + ".h")
                if not FORCE and (os.path.exists(rpc_file) and (os.path.getmtime(source_file) < os.path.getmtime(rpc_file))):
                    log.Success("skipping file %s, up-to-date" % os.path.basename(rpc_file))
                else:
                    with open(rpc_file, "w") as output_file:
                        emitter = Emitter(output_file, INDENT_SIZE)
                        EmitVersionCode(rpcObj, emitter, filename, os.path.basename(source_file), data_emitted)
                        log.Success("JSON-RPC version information generated in %s" % os.path.basename(output_file.name))

        if generateStubs:
            with open(os.path.join(directory, filename + "JsonRpc.cpp"), "w") as output_file:
                emitter = Emitter(output_file, INDENT_SIZE)
                emitter.Line()
                EmitHelperCode(rpcObj, emitter, os.path.basename(header_file))
                log.Success("JSON-RPC stubs generated in %s" % os.path.basename(output_file.name))

        if generateRpc and "@dorpc" in rpcObj.schema and rpcObj.schema["@dorpc"]:
            output_filename = os.path.join(directory, "J" + filename + ".h")
            if not FORCE and (os.path.exists(output_filename) and (os.path.getmtime(source_file) < os.path.getmtime(output_filename))):
               log.Success("skipping file %s, up-to-date" % os.path.basename(output_filename))
            else:
                with open(output_filename, "w") as output_file:
                    emitter = Emitter(output_file, INDENT_SIZE)
                    emitter.Line()
                    EmitRpcCode(rpcObj, emitter, filename, os.path.basename(source_file), data_emitted)
                    log.Success("JSON-RPC implementation generated in %s" % os.path.basename(output_file.name))

    else:
        log.Info("No code to generate.")


objTracker = ObjectTracker()
enumTracker = EnumTracker()

if __name__ == "__main__":
    argparser = argparse.ArgumentParser(
        description='Generate JSON C++ classes, stub code and API documentation from JSON definition files and C++ header files',
        epilog="For information about custom tags supprted in C++ code please see StubGenerator help (--help-tags).",
        formatter_class=argparse.RawTextHelpFormatter)
    argparser.add_argument('path',
            nargs="*",
            help="JSON file(s) and/or C++ header files, wildcards are allowed")
    argparser.add_argument("-d",
            "--docs",
            dest="docs",
            action="store_true",
            default=False,
            help="generate documentation")
    argparser.add_argument("-c",
            "--code",
            dest="code",
            action="store_true",
            default=False,
            help="generate C++ code building JSON classes and complete JSON-RPC functionality (the latter only if applicable)")
    argparser.add_argument("-s",
            "--stubs",
            dest="stubs",
            action="store_true",
            default=False,
            help="generate C++ stub code for JSON-RPC (i.e. *JsonRpc.cpp file to fill in manually)")
    argparser.add_argument("-o",
            "--output",
            dest="output_dir",
            metavar="DIR",
            action="store",
            default=None,
            help="output directory, absolute path or directory relative to output file (default: output in the same directory as the source file)")
    argparser.add_argument(
            "--force",
            dest="force",
            action="store_true",
            default=False,
            help= "force code generation even if destination appears up-to-date (default: force disabled)")

    json_group = argparser.add_argument_group("JSON parser arguments (optional)")
    json_group.add_argument("-i",
            dest="if_dir",
            metavar="DIR",
            action="store",
            type=str,
            default=None,
            help=
            "a directory with JSON API interfaces that will substitute the {interfacedir} tag (default: same directory as source file)")
    json_group.add_argument("--no-ref-names",
            dest="no_ref_names",
            action="store_true",
            default=False,
            help="do not derive class names from $refs (default: derive class names from $ref)")
    json_group.add_argument("--no-duplicates-warnings",
            dest="no_duplicates_warnings",
            action="store_true",
            default=not SHOW_WARNINGS,
            help="suppress duplicate object warnings (default: show all duplicate object warnings)")

    cpp_group = argparser.add_argument_group("C++ parser arguments (optional)")
    cpp_group.add_argument("-j",
            dest="cppif_dir",
            metavar="DIR",
            action="store",
            type=str,
            default=None,
            help=
            "a directory with C++ API interfaces that will substitute the {cppinterfacedir} tag (default: same directory as source file)")
    cpp_group.add_argument('-I',
            dest="includePaths",
            metavar="INCLUDE_DIR",
            action='append',
            default=[],
            type=str,
            help='add an include path (can be used multiple times)')
    cpp_group.add_argument("--include",
            dest="extra_include",
            metavar="FILE",
            action="store",
            default=DEFAULT_DEFINITIONS_FILE,
            help="include a C++ header file with common types (default: include '%s')" %
            DEFAULT_DEFINITIONS_FILE)
    cpp_group.add_argument("--namespace",
            dest="if_namespace",
            metavar="NS",
            type=str,
            action="store",
            default=INTERFACE_NAMESPACE,
            help="set namespace to look for interfaces in (default: %s)" %
            INTERFACE_NAMESPACE)
    cpp_group.add_argument("--format",
            dest="format",
            type=str,
            action="store",
            default="flexible",
            choices=["default-compliant", "force-compliant", "default-uncompliant-extended", "force-uncompliant-extended", "default-uncompliant-collapsed", "force-uncompliant-collapsed"],
            help="select JSON-RPC data format (default: default-compliant)")

    data_group = argparser.add_argument_group("C++ output arguments (optional)")
    data_group.add_argument("-p",
            "--data-path",
            dest="if_path",
            metavar="PATH",
            action="store",
            type=str,
            default=JSON_INTERFACE_PATH,
            help="relative path for #include'ing JsonData header file (default: 'interfaces/json', '.' for no path)")
    data_group.add_argument("--copy-ctor",
            dest="copy_ctor",
            action="store_true",
            default=False,
            help="always emit a copy constructor and assignment operator (default: emit only when needed)")
    data_group.add_argument("--def-string",
            dest="def_string",
            metavar="STRING",
            type=str,
            action="store",
            default=DEFAULT_EMPTY_STRING,
            help="default string initialisation (default: \"%s\")" % DEFAULT_EMPTY_STRING)
    data_group.add_argument("--def-int-size",
            dest="def_int_size",
            metavar="SIZE",
            type=int,
            action="store",
            default=DEFAULT_INT_SIZE,
            help="default integer size in bits (default: %i)" % DEFAULT_INT_SIZE)
    data_group.add_argument("--indent",
            dest="indent_size",
            metavar="SIZE",
            type=int,
            action="store",
            default=INDENT_SIZE,
            help="code indentation in spaces (default: %i)" % INDENT_SIZE)

    doc_group = argparser.add_argument_group("Documentation output arguments (optional)")
    doc_group.add_argument("--no-style-warnings",
            dest="no_style_warnings",
            action="store_true",
            default=not DOC_ISSUES,
            help="suppress documentation issues (default: show all documentation issues)")
    doc_group.add_argument("--no-interfaces-section",
            dest="no_interfaces_section",
            action="store_true",
            default=False,
            help="do not include 'Interfaces' section (default: include interface section)")
    doc_group.add_argument("--source-location",
            dest="source_location",
            metavar="LN",
            type=str,
            action="store",
            default=INTERFACE_SOURCE_LOCATION,
            help="override interface source file location to the link specified")
    doc_group.add_argument("--source-revision",
            dest="source_revision",
            metavar="ID",
            type=str,
            action="store",
            default=None,
            help="override interface source file revision to the commit id specified")

    ts_group = argparser.add_argument_group("Troubleshooting arguments (optional)")
    ts_group.add_argument("--verbose",
            dest="verbose",
            action="store_true",
            default=False,
            help="enable verbose logging")
    ts_group.add_argument("--keep-empty",
            dest="keep_empty",
            action="store_true",
            default=False,
            help="keep generated files that have no code")
    ts_group.add_argument("--dump-json",
            dest="dump_json",
            action="store_true",
            default=False,
            help="dump the intermediate JSON file created while parsing a C++ header")


    args = argparser.parse_args(sys.argv[1:])

    log = Log.Log(NAME, args.verbose, SHOW_WARNINGS, DOC_ISSUES)

    DOC_ISSUES = not args.no_style_warnings
    log.doc_issues = DOC_ISSUES
    NO_DUP_WARNINGS = args.no_duplicates_warnings
    INDENT_SIZE = args.indent_size
    ALWAYS_EMIT_COPY_CTOR = args.copy_ctor
    KEEP_EMPTY = args.keep_empty
    CLASSNAME_FROM_REF = not args.no_ref_names
    DEFAULT_EMPTY_STRING = args.def_string
    DEFAULT_INT_SIZE = args.def_int_size
    DUMP_JSON = args.dump_json
    FORCE = args.force
    DEFAULT_DEFINITIONS_FILE = args.extra_include
    INTERFACE_NAMESPACE = "::" + args.if_namespace if args.if_namespace.find("::") != 0 else args.if_namespace
    INTERFACES_SECTION = not args.no_interfaces_section
    INTERFACE_SOURCE_LOCATION = args.source_location
    INTERFACE_SOURCE_REVISION = args.source_revision
    if RpcFormat.EXTENDED.value in args.format:
        RPC_FORMAT = RpcFormat.EXTENDED
    elif RpcFormat.COLLAPSED.value in args.format:
        RPC_FORMAT = RpcFormat.COLLAPSED
    else:
        RPC_FORMAT = RpcFormat.COMPLIANT
    if "force" in args.format:
        RPC_FORMAT_FORCED = True

    if args.if_path and args.if_path != ".":
        JSON_INTERFACE_PATH = args.if_path
    JSON_INTERFACE_PATH = posixpath.normpath(JSON_INTERFACE_PATH) + os.sep

    if args.if_dir:
        args.if_dir = os.path.abspath(os.path.normpath(args.if_dir))
    if args.cppif_dir:
        args.cppif_dir = os.path.abspath(os.path.normpath(args.cppif_dir))

    generateCode = args.code
    generateRpc = args.code
    generateDocs = args.docs
    generateStubs = args.stubs

    if not args.path or (not generateCode and not generateRpc and not generateStubs and not generateDocs):
        argparser.print_help()
    else:
        files = []
        for p in args.path:
            if "*" in p or "?" in p:
                files.extend(glob.glob(p))
            else:
                files.append(p)
        for path in files:
            try:
                log.Header(path)
                if path.endswith(".h"):
                    schemas, additional_includes = LoadInterface(path, False, args.includePaths)
                else:
                    schemas, additional_includes = LoadSchema(path, args.if_dir, args.cppif_dir, args.includePaths)
                for schema in schemas:
                    if schema:
                        warnings = GENERATED_JSON
                        GENERATED_JSON = "@generated" in schema
                        output_path = path
                        if args.output_dir:
                            if (args.output_dir[0]) == '/':
                                output_path = os.path.join(args.output_dir, os.path.basename(output_path))
                            else:
                                dir = os.path.join(os.path.dirname(output_path), args.output_dir)
                                if not os.path.exists(dir):
                                    os.makedirs(dir)
                                output_path = os.path.join(dir, os.path.basename(output_path))
                        if generateCode or generateStubs or generateRpc:
                            CreateCode(schema, path, output_path, additional_includes, generateCode, generateStubs, generateRpc)
                        if generateDocs:
                            if "$schema" in schema:
                                if "info" in schema:
                                    title = schema["info"]["title"] if "title" in schema["info"] \
                                            else schema["info"]["class"] if "class" in schema["info"] \
                                            else os.path.basename(output_path)
                                else:
                                    title = os.path.basename(output_path)
                                CreateDocument(schema, os.path.join(os.path.dirname(output_path), title.replace(" ", "")))
                            else:
                                log.Warn("Skiping file; not a JSON-RPC definition document")
                        GENERATED_JSON = warnings
            except JsonParseError as err:
                log.Error(str(err))
            except RuntimeError as err:
                log.Error(str(err))
            except IOError as err:
                log.Error(str(err))
            except ValueError as err:
                log.Error(str(err))
            except jsonref.JsonRefError as err:
                log.Error(str(err))

        log.Info("JsonGenerator: All done, {} files parsed, {} error{}.".format(len(files), len(log.errors) if log.errors else 'no',
                                                              '' if len(log.errors) == 1 else 's'))

        for tf in temp_files:
            os.remove(tf)

        if log.errors:
            sys.exit(1)

