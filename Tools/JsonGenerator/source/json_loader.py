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

import os
import sys
import re
import copy
import posixpath
import tempfile
import json
from collections import OrderedDict

try:
    import jsonref
except:
    print("Install jsonref first")
    sys.exit(1)

import config
import header_loader
import trackers


log = None
def SetLogger(logger):
    global log
    log = logger

def Tstring(string):
    return '_T(\"%s\")' % string

def Scoped(root, obj, full = True):
    scope = ""

    if isinstance(obj, (JsonObject, JsonEnum, JsonArray)):
        o = obj

        # Go down the class hierarchy...
        while o.parent and not isinstance(o.parent, JsonMethod) and (not o.is_duplicate and o.RefCount() == 1):
            if o.parent.cpp_class:
                scope = o.parent.cpp_class + "::" + scope

            o = o.parent

        # Prepend with the outer namespace
        if o.parent and o.parent.included_from:
            scope = "%s::" % o.parent.included_from + scope
        else:
            scope = "%s::" % root.cpp_class + scope

        if full:
            scope = "%s::" % (config.DATA_NAMESPACE) + scope

    return scope


class JsonParseError(RuntimeError):
    pass


def CoreJson(type):
    return (config.TYPE_PREFIX + "::" + type)

def MakeObject(type):
    return (type + config.OBJECT_SUFFIX)

def MakeEnum(type):
    return (type + config.ENUM_SUFFIX)


class JsonType():
    KEYWORDS = ["asm", "auto", "bool", "break", "catch", "class", "const", "continue",
                "delete", "do", "double", "enum", "explicit", "export", "extern", "float", "for"
                "friend", "goto", "long", "mutable", "mamespace", "new", "operator", "private", "protected",
                "public", "register", "return", "short", "signed", "static", "struct", "switch", "template",
                "this", "throw", "true", "try", "union", "using", "virtual", "volatile", "while"]

    def __init__(self, name, parent, schema, included=None):
        is_generated = name.startswith("@_generated_")

        if is_generated:
            name = name.replace("@_generated_", "")

        self.new_name = None
        self.is_renamed = False
        self.schema = schema
        self.parent = parent
        self.name = name
        self.original_name = schema.get("original_name")
        self.description = schema.get("description")
        self.iterator = schema.get("iterator")
        self.original_type = schema.get("original_type")
        self.do_create = (self.original_type == None)
        self.included_from = included
        self.is_duplicate = False

        # Do some sanity check on the type name
        if parent and not isinstance(parent, JsonArray):
            if not is_generated:
                if not self.name.replace("_", "").isalnum():
                    raise JsonParseError("Invalid characters in identifier name: '%s'" % self.print_name)

                if self.name[0] == "_":
                    raise JsonParseError("Identifiers must not start with an underscore (reserved by the generator): '%s'" % self.print_name)

                if not self.name.islower():
                    log.Warn("'%s': mixedCase identifiers are supported, however all-lowercase names are recommended" % self.print_name)
                elif "_" in self.name:
                    log.Warn("'%s': snake_case identifiers are supported, however flatcase names are recommended" % self.print_name)

            if self.original_name: # identifier comming from the C++ world
                if self.original_name[0] == "_":
                    raise JsonParseError("'%s': identifiers must not start with an underscore (reserved by the generator)" % self.original_name)

                if "_" in self.original_name:
                    log.Warn("'%s': snake_case identifiers are supported, however mixedCase names are recommended " % self.original_name)

        # Do some sanity check on the description text
        if self.description and not isinstance(self, JsonMethod):
            if self.description.endswith(" ") or self.description.startswith(" "):
                log.DocIssue("'%s': parameter description has leading or trailing whitespaces" % self.print_name)

            if not self.description[0].isupper() and self.description[0].isalpha():
                log.DocIssue("'%s': sentence-case capitalization is recommended for parameter descriptions ('%s')"
                    % (self.print_name ,log.Ellipsis(self.description)))

    def TempName(self, postfix = ""):
        name = self.local_name.strip("_")
        return ("_" + name[0].lower() + name[1:] + (postfix[0].upper() + postfix[1:] if postfix else ""))

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
    def print_name(self):
        return (self.parent.print_name + "/" + self.name)

    @property
    def json_name(self):
        if self.new_name:
            return self.new_name
        else:
            return self.name

    @property
    def local_name(self):
        if self.new_name:
            name = self.new_name
        elif self.original_name:
            name = self.original_name
        else:
            name = self.name

        if name in self.KEYWORDS:
            name += "_"

        return name

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
        return (Scoped(self.root, self) + self.cpp_class)

    @property
    def short_cpp_type(self):
        return self.cpp_type.replace("%s::%s::" % (config.DATA_NAMESPACE, self.root.cpp_class), "")

    @property
    def cpp_class(self): # C++ class type of the object
        raise RuntimeError("Can't instantiate '%s'" % self.print_name)

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
        return CoreJson("Boolean")

    @property
    def cpp_native_type(self):
        return "bool"


class JsonNumber(JsonNative, JsonType):
    def __init__(self, name, parent, schema, size = config.DEFAULT_INT_SIZE, signed = False):
        JsonType.__init__(self, name, parent, schema)
        self.size = schema["size"] if "size" in schema else size
        self.signed = schema["signed"] if "signed" in schema else signed
        self._cpp_class = CoreJson("Dec%sInt%i" % ("S" if self.signed else "U", self.size))
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
    def __init__(self, name, size = config.DEFAULT_INT_SIZE, signed = False):
        JsonInteger.__init__(self, "@_generated_" + name, None, {}, size, signed)


class AuxJsonAuto(JsonInteger):
    def __init__(self, name, size = config.DEFAULT_INT_SIZE, signed = False):
        JsonType.__init__(self, "@_generated_" + name, None, {})

    @property
    def cpp_native_type(self):
        return "auto"


class JsonFloat(JsonNative, JsonType):
    @property
    def cpp_class(self):
        return CoreJson("Float")

    @property
    def cpp_native_type(self):
        return "float"


class JsonDouble(JsonNative, JsonType):
    @property
    def cpp_class(self):
        return CoreJson("Double")

    @property
    def cpp_native_type(self):
        return "double"


class JsonString(JsonNative, JsonType):
    @property
    def cpp_class(self):
        return CoreJson("String")

    @property
    def cpp_native_type(self):
        return "string"


class JsonRefCounted():
    def __init__(self):
        self.ref_destination = None
        self.refs = []
        self.AddRef(self)

    def AddRef(self, obj):
        self.refs.append(obj)

    def RefCount(self):
        return len(self.refs)


class JsonEnum(JsonRefCounted, JsonType):
    def __init__(self, name, parent, schema, enum_type, included = None):
        JsonRefCounted.__init__(self)
        JsonType.__init__(self, name, parent, schema, included)

        if enum_type != "string":
            raise JsonParseError("Only strings are supported in enums: '%s'" % self.print_name)

        self.type = enum_type
        self.bitmask = schema.get("bitmask")
        self.enumerators = schema.get("enum")
        self.cpp_enumerator_values = schema.get("values")

        self.class_name = self.__DetermineClassName()

        if not self.cpp_enumerator_values:
            self.cpp_enumerator_values = schema.get("enumvalues") # deprecated

        self.cpp_enumerators = []

        if "ids" in self.schema:
            self.cpp_enumerators = self.schema["ids"]
        elif "enumids" in self.schema: # deprecated
            self.cpp_enumerators = self.schema["enumids"]
        else:
            if self.schema.get("case") == "snake" :
                self.cpp_enumerators = list(map(lambda x: re.sub(r'(?<!^)(?=[A-Z])', '_', x).upper(), self.enumerators))
            else:
                self.cpp_enumerators = list(map(lambda x: ("E" if x[0].isdigit() else "") + x.upper(), self.enumerators))

        self.__DetermineSize()

        if self.cpp_enumerator_values and (len(self.enumerators) != len(self.cpp_enumerator_values)):
            raise JsonParseError("Mismatch in enumeration values in enum '%s'" % self.print_name)

        self.is_scoped = schema["scoped"] if "scoped" in schema else schema["enumtyped"] if "enumtyped" in schema else True

        obj = trackers.enum_tracker.Add(self)
        if obj:
            self.is_duplicate = True
            self.ref_destination = obj
            if ((obj.parent != self.parent) and not trackers.IsInCustomRef(self)):
                obj.AddRef(self)

    def __DetermineSize(self):
        if "size" in self.schema and isinstance(self.schema["size"], int):
            self.size = self.schema["size"]

        elif self.cpp_enumerator_values:
            same = True
            is_bitmap = True
            biggest = 0

            for idx, e in enumerate(self.cpp_enumerator_values):
                if (e & (e-1) != 0) and (e != 0):
                    is_bitmap = False

                if idx != e:
                    same = False

                if e > biggest:
                    biggest = e

            if same:
                log.Warn("'%s': specified enum values are same as implicit" % self.print_name)
                self.cpp_enumerator_values = []

            if self.bitmask and not is_bitmap:
                raise JsonParseError("Enum tagged @bitmask but values are not a power of 2: '%s'" % self.print_name);

            self.size = 32 if biggest >= 65536 else 16 if biggest >= 256 else 8

        else:
            self.size = 16 if len(self.enumerators) >= 256 else 8
            if self.bitmask:
                raise JsonParseError("Enum tagged @bitmask but no explicit values are provided: '%s'" % self.print_name);

        if self.size not in [8, 16, 32, 64]:
            raise JsonParseError("Invalid enum size value (%i): %s" % (self.size, self.print_name))

    def __DetermineClassName(self):
        classname = ""

        if self.original_type:
            # Override with real class name, this is likely comming from C++ header
            classname = self.original_type
        elif "class" in self.schema:
            # Override class name if "class" property present
            classname = self.schema["class"].capitalize
        elif "hint" in self.schema:
            # Override class name if "hint" property present
            classname = self.schema["hint"]
        elif config.CLASSNAME_FROM_REF and ("@ref" in self.schema):
            # NOTE: Abuse the ref feature to construct a name for the enum!
            classname = MakeEnum(self.schema["@ref"].rsplit(posixpath.sep, 1)[1].capitalize())
        elif isinstance(self.parent, JsonProperty):
            classname = MakeEnum(self.parent.name.capitalize())
        else:
            classname = MakeEnum(self.name.capitalize())

        return classname

    @property
    def cpp_type(self):
        return CoreJson("EnumType<%s>" % (self.cpp_native_type))

    @property
    def cpp_native_type(self):
        return self.original_type if self.original_type else (Scoped(self.root, self) + self.cpp_class)

    @property
    def cpp_class(self):
        if self.is_duplicate:
            # Use the original (ie. first seen) name
            return self.ref_destination.cpp_class
        else:
            return self.class_name


class JsonObject(JsonRefCounted, JsonType):
    def __init__(self, name, parent, schema, included = None):
        JsonRefCounted.__init__(self)

        JsonType.__init__(self, name, parent, schema, included)
        self._properties = []
        self._objects = []
        self._enums = []

        # Handle duplicate objects...
        if "properties" in schema:
            obj = trackers.object_tracker.Add(self)
            if obj:
                if "original_type" in self.schema and "original_type" not in obj.schema:
                    # Very unlucky scenario when duplicate class carries more information than the original.
                    # Swap the classes in duplicate lists so we take the better one for generating code.
                    trackers.object_tracker.Remove(obj)
                    trackers.object_tracker.Remove(self)
                    trackers.object_tracker.Add(self)
                    self.refs += obj.refs[1:]
                    obj.is_duplicate = True
                    obj.ref_destination = self
                    obj.refs = [obj]
                    trackers.object_tracker.Add(obj)
                    if obj.parent != self.parent and not trackers.IsInCustomRef(obj):
                        self.AddRef(obj)
                else:
                    self.is_duplicate = True
                    self.ref_destination = obj
                    if obj.parent != self.parent and not trackers.IsInCustomRef(self):
                        obj.AddRef(self)

            idx = 0
            for prop_name, prop in schema["properties"].items():
                new_obj = JsonItem(prop_name, self, prop, included=included)
                if "position" not in new_obj.schema:
                    new_obj.schema["position"] = idx

                idx += 1
                self._properties.append(new_obj)

                if not new_obj.description and not isinstance(self, JsonMethod):
                    log.DocIssue("'%s': element missing description" % new_obj.print_name)

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
                    return CoreJson("Container")
                elif config.CLASSNAME_FROM_REF and ("@ref" in self.schema):
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
                    elif self.root.rpc_format != config.RpcFormat.COMPLIANT and isinstance(self.parent, JsonArray) and isinstance(self.parent.parent, JsonProperty):
                        classname = MakeObject(self.parent.parent.cpp_name)
                    elif self.root.rpc_format == config.RpcFormat.COMPLIANT and isinstance(self.parent, JsonArray):
                        classname = (MakeObject(self.parent.cpp_name) + "Elem")
                    else:
                        classname = MakeObject(self.cpp_name)

                # For common classes append special suffix
                if self.RefCount() > 1:
                    classname = classname.replace(config.OBJECT_SUFFIX, config.COMMON_OBJECT_SUFFIX)

            return classname

    @property
    def json_name(self):
        return self.name

    @property
    def cpp_type(self):
        return Scoped(self.root, self) + self.cpp_class

    @property
    def cpp_native_type(self):
        return self.original_type if self.original_type else self.cpp_type

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
        if config.ALWAYS_EMIT_COPY_CTOR or self.parent.is_copy_ctor_needed:
            return True

        # Check if a copy constructor is needed by scanning all duplicate classes
        for obj in self.refs:
            if (obj != self) and obj.parent.is_copy_ctor_needed:
                return True

        return False


class JsonArray(JsonType):
    def __init__(self, name, parent, schema, included=None):
        JsonType.__init__(self, name, parent, schema, included)
        self._items = None

        if "items" in schema:
            if "class" in schema["items"]:
                name = "@_generated_" + schema["items"]["class"] + "Element"

            self._items = JsonItem(name, self, schema["items"], included)
        else:
            raise JsonParseError("No items in array '%s'" % self.print_name)

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
        return CoreJson("ArrayType<%s>" % self._items.cpp_type)

    @property
    def cpp_native_type(self):
        return "std::list<%s>" % self._items.cpp_native_type

    @property
    def cpp_class(self):
        return "" # This is not an error

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
            log.Warn("'%s': method names containing full designator are deprecated" % name)
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

        if "hint" in schema:
            method_schema["hint"] = schema["hint"]

        JsonObject.__init__(self, name, parent, method_schema, included=included)

        self.summary = schema.get("summary")
        self.deprecated = schema.get("deprecated")
        self.obsolete = schema.get("obsolete")

        if not self.summary:
            log.DocIssue("'%s': method is missing summary" % self.print_name)

        self.endpoint_name = (config.IMPL_ENDPOINT_PREFIX + super().json_name)

        if (self.rpc_format == config.RpcFormat.COMPLIANT) and not isinstance(self.params, (JsonObject, JsonNull)):
            raise JsonParseError("With 'compliant' format parameters to a method or event need to be an object: '%s'" % self.print_name)
        elif (self.rpc_format == config.RpcFormat.EXTENDED) and not property and not isinstance(self.params, (JsonObject, JsonArray, JsonNull)):
            raise JsonParseError("With 'extended' format parameters to a method or event need to be an object or an array: '%s'" % self.print_name)
        elif (self.rpc_format == config.RpcFormat.COLLAPSED) and isinstance(self.params, JsonObject) and (len(self.params.properties) == 1):
            log.Warn("'%s': with 'collapsed' format methods and events with one parameter should not have an outer object" % self.print_name)

    @property
    def rpc_format(self):
        return self.root.rpc_format

    @property
    def params(self):
        return self.properties[0]

    @property
    def result(self):
        return self.properties[1]

    @property
    def function_name(self):
        if "hint" in self.schema:
            return self.schema["hint"]
        else:
            return self.cpp_name

    def Headline(self):
        return "%s%s%s" % (self.json_name, (" - " + self.summary.split(".", 1)[0]) if self.summary else "",
                           " (DEPRECATED)" if self.deprecated else " (OBSOLETE)" if self.obsolete else "")


class JsonNotification(JsonMethod):
    def __init__(self, name, parent, schema, included=None):
        JsonMethod.__init__(self, name, parent, schema, included)

        if "id" in schema and "type" not in schema["id"]:
            schema["id"]["type"] = "string"

        self.sendif_type = JsonItem("id", self, schema["id"]) if "id" in schema else None
        self.is_status_listener = schema.get("statuslistener")

        self.endpoint_name = (config.IMPL_EVENT_PREFIX + self.json_name)

        for param in self.params.properties:
            if not isinstance(param,JsonNative) and param.do_create:
                log.Info("'%s': notification parameter '%s' refers to generated JSON objects" % (name, param.name))
                break


class JsonProperty(JsonMethod):
    def __init__(self, name, parent, schema, included=None):
        self.readonly = "readonly" in schema and schema["readonly"]
        self.writeonly = "writeonly" in schema and schema["writeonly"]

        if ("params" not in schema) and ("result" not in schema):
            raise JsonParseError("No parameters defined for property: '%s'" % self.print_name)

        if ("index" in schema) and ("type" not in schema["index"]):
            schema["index"]["type"] = "string"

        JsonMethod.__init__(self, name, parent, schema, included, property=True)
        self.index = JsonItem("index", self, schema["index"]) if "index" in schema else None

        self.endpoint_set_name = (config.IMPL_ENDPOINT_PREFIX + "set_" + self.json_name)
        self.endpoint_get_name = (config.IMPL_ENDPOINT_PREFIX + "get_" + self.json_name)


class JsonRpcSchema(JsonType):
    def __init__(self, name, schema):
        JsonType.__init__(self, name, None, schema)
        self.info = None
        self.base_schema = schema.get("$schema")
        self.jsonrpc_version = schema.get("jsonrpc")
        self.namespace = None
        self.methods = []
        self.includes = []
        self.rpc_format = config.RPC_FORMAT

        if "interface" in schema:
            schema = schema["interface"]

        if "info" in schema:
            self.info = schema["info"]

            self.namespace = self.info.get("namespace")

            if "class" not in self.info:
                raise JsonParseError("No 'class' defintion in 'info' block")

            if "format" in self.info and not config.RPC_FORMAT_FORCED:
                self.rpc_format = config.RpcFormat(self.info["format"])

            if "version" not in self.info:
                log.Warn("No version provided for %s interface, using 1.0.0" % self.info["class"])
        else:
            raise JsonParseError("No 'info' block in metadata")

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
            raise JsonParseError("no methods, properties or events defined in %s" % self.print_name)

    @property
    def root(self):
        return self

    @property
    def print_name(self):
        return ""

    @property
    def cpp_class(self):
        return ((self.namespace + "::") if self.namespace else "") + super().cpp_name

    @property
    def properties(self):
        return self.methods

    @property
    def objects(self):
        return self.properties

    def RefCount(self):
        return 1


def JsonItem(name, parent, schema, included=None):
    # Create the appropriate Python object based on the JSON type
    if "type" in schema:
        if schema["type"] == "object":
            return JsonObject(name, parent, schema, included)
        elif schema["type"] == "array":
            return JsonArray(name, parent, schema, included)
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
            raise JsonParseError("unsupported JSON type: '%s'" % schema["type"])
    else:
        raise JsonParseError("missing 'type' for item: '%s'" % name)


def LoadSchema(file, include_path, cpp_include_path, header_include_paths):
    additional_includes = []

    def Adjust(schema):
        def AdjustByFormat(schema):

            # Unifies properties to always have params and result objects
            # Also automatically enclose params in object for COMPLIANT mode
            def AdjustProperties(schema, rpc_format):
                if "include" in schema:
                    for _, prop in schema["include"].items():
                        AdjustProperties(prop, rpc_format)

                if "properties" in schema:
                    for _, prop in schema["properties"].items():
                        if "params" in prop:
                            compliance_adjusted = False

                            if (rpc_format == config.RpcFormat.COMPLIANT) and (prop["params"].get("type") != "object") and (not prop.get("readonly")):
                                prop["params"] = { "type": "object", "properties" : { "value" : prop["params"] } }
                                compliance_adjusted = True

                            if ("result" not in prop) and (not prop.get("writeonly")):
                                if prop.get("readonly"):
                                    prop["result"] = prop["params"]
                                    del prop["params"]
                                else:
                                    prop["result"] = copy.deepcopy(prop["params"]["properties"]["value"]) if compliance_adjusted else copy.deepcopy(prop["params"])
                                    prop["result"]["@ref"] = "#../params"

            rpc_format = config.RPC_FORMAT

            if (not config.RPC_FORMAT_FORCED) and ("info" in schema) and ("format" in schema["info"]):
                rpc_format = config.RpcFormat(schema["info"]["format"])

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
            if isinstance(schema, dict):
                if "original_type" in schema:
                    if schema["original_type"] == element:
                        return schema

                for _, item in schema.items():
                    found = _FindCpp(element, item)
                    if found:
                        return found

            return None

        # Tags all objects that used to be $references
        if isinstance(schema, jsonref.JsonRef) and isinstance(schema, dict):
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
            else:
                schema["@ref"] = schema.__reference__["$ref"]

        if isinstance(schema, dict):
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
                        if cpp_path.startswith(config.FRAMEWORK_NAMESPACE + "::"):
                            cpp_path = cpp_path.replace(config.FRAMEWORK_NAMESPACE + "::","")

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
                        raise IOError("Failed to find $ref path '%s' (C++ part)" % schema["@dataref"])
                else:
                    raise IOError("Failed to find $ref path '%s' (JSON part)" % schema["@dataref"])

    temp_files = []

    with open(file, "r") as json_file:
        def Preprocess(pairs):
            def Scan(pairs):
                for i, c in enumerate(pairs):
                    if isinstance(c, tuple):

                        k, v = c

                        if k == "$cppref":
                            k = "$ref"

                        if isinstance(v, list):
                            Scan(v)

                        elif k == "$ref":
                            if ".json" in v:
                                # Need to prepend with 'file:' for jsonref to load an external file..
                                ref = v.split("#") if "#" in v else [v,""]
                                if ("{interfacedir}" in ref[0]) and include_path:
                                    ref_file = ref[0].replace("{interfacedir}", include_path)

                                    if os.path.exists(ref_file):
                                        pairs[i] = (k, ("file://" + ref_file + "#" + ref[1]))
                                    else:
                                        raise IOError("$ref file '%s' not found" % ref_file)
                                else:
                                    ref_file = os.path.abspath(os.path.dirname(file)) + os.sep + ref[0]

                                    if not os.path.exists(ref_file):
                                        ref_file = v
                                    else:
                                        pairs[i] = (k, ("file://" + ref_file + "#" + ref[1]))

                            elif v.endswith(".h") or v.endswith(".h#"):
                                ref_file = v.replace("#", "")

                                if ("{interfacedir}" in ref_file or "{cppinterfacedir}" in ref_file) and cpp_include_path:
                                    ref_file = ref_file.replace("{interfacedir}", cpp_include_path).replace("{cppinterfacedir}", cpp_include_path)

                                if os.path.exists(ref_file):
                                    cppif, _ = header_loader.LoadInterface(ref_file, log, True, header_include_paths)

                                    if cppif:
                                        if ref_file not in additional_includes:
                                            additional_includes.append(ref_file)

                                        with tempfile.NamedTemporaryFile(mode="w", delete=False) as temp_json_file:
                                            temp_json_file.write(json.dumps(cppif))
                                            pairs[i] = (k, "file://" + temp_json_file.name + "#")
                                            temp_files.append(temp_json_file.name)
                                else:
                                    raise IOError("$ref file '%s' not found" % ref_file)

                            elif "::" in v:
                                pairs[i] = ("@dataref", v)

            Scan(pairs)

            d = OrderedDict()

            for k, v in pairs:
                d[k] = v

            return d

        json_resolved = jsonref.loads(json_file.read(), object_pairs_hook=Preprocess)

        Adjust(json_resolved)
        MarkRefs(json_resolved, None, None, json_resolved)

        return [json_resolved], additional_includes, temp_files

    return [], [], temp_files

def Load(log, path, if_dir = None, cpp_if_dir = None, include_paths = []):
    temp_files = []

    if path.endswith(".h"):
        schemas, additional_includes = header_loader.LoadInterface(path, log, False, include_paths)
    else:
        schemas, additional_includes, temp_files = LoadSchema(path, if_dir, cpp_if_dir, include_paths)

    return schemas, additional_includes, temp_files
