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
from collections import OrderedDict


sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), os.pardir))

import ProxyStubGenerator.CppParser
import ProxyStubGenerator.Interface
import ProxyStubGenerator.Log as Log

NAME = "JsonGenerator"
DEFAULT_DEFINITIONS_FILE = "../ProxyStubGenerator/default.h"
FRAMEWORK_NAMESPACE = "WPEFramework"
INTERFACE_NAMESPACE = FRAMEWORK_NAMESPACE + "::Exchange"
INTERFACES_SECTION = True
VERBOSE = False
GENERATED_JSON = False
SHOW_WARNINGS = True
DOC_ISSUES = True


log = Log.Log(NAME, VERBOSE, SHOW_WARNINGS, DOC_ISSUES)

try:
    import jsonref
except:
    log.Error("Install jsonref first")
    log.Print("e.g. try 'pip3 install jsonref'")
    sys.exit(1)

INDENT_SIZE = 4

ALWAYS_COPYCTOR = False
KEEP_EMPTY = False
CLASSNAME_FROM_REF = True
DEFAULT_EMPTY_STRING = ""
DEFAULT_INT_SIZE = 32
CPP_IF_PATH = "interfaces" + os.sep
IF_PATH = CPP_IF_PATH + "json"
DUMP_JSON = False

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
        self.name = schema["original"] if "original" in schema else name
        if parent:
            if not self.name.replace("_","").isalnum():
                raise JsonParseError("'%s': invalid characters in identifier name" % self.name)
            if self.name[0] == "_":
                raise JsonParseError("'%s': identifiers must not start with an underscore (reserved by the generator)" % self.name)
            if not self.name.islower():
                log.Warn("'%s': mixed case identifiers are supported, however all-lowercase names are recommended " % self.name)
            elif "_" in self.name and not GENERATED_JSON:
                log.Warn("'%s': snake_case identifiers are supported, however flat case names are recommended " % self.name)
        self.true_name = name
        self.schema = schema
        self.duplicate = False
        self.parent = parent
        self.description = None
        self.default = None
        self.included_from = included
        self.create = "typename" not in schema
        if not self.create:
            self.cpptype = schema["typename"]
            self.enumName = schema["typename"]

        if "description" in schema:
            self.description = schema["description"]
        # BUG: Due to a bug in JsonRef, need to pick up the description from the original JSON
        if isinstance(schema, jsonref.JsonRef) and "description" in schema.__reference__:
            self.description = schema.__reference__["description"]
        # do some sanity check on the description text
        if self.description and not isinstance(self, JsonMethod):
            if self.description.endswith("."):
                log.DocIssue("'%s': use sentence case capitalization and no period for parameter descriptions (\"%s\")" % (self.name, log.Ellipsis(self.description, False)))
            if self.description.endswith(" ") or self.description.startswith(" "):
                log.DocIssue("'%s': parameter description has leading or trailing whitespace" % self.name)
            if not self.description[0].isupper() and self.description[0].isalpha():
                log.DocIssue("'%s': use sentence case capitalization and no period for parameter descriptions (\"%s\")" % (self.name,log.Ellipsis(self.description)))
        if "default" in schema:
            self.default = schema["default"]

    def Create(self):
        return self.create

    def IsDuplicate(self): # Whether this object is a duplicate of another
        return self.duplicate

    def Schema(self): # Get the original schema
        return self.schema

    def Description(self): # Item description
        return self.description

    def JsonName(self): # Name as in JSON
        return self.name

    def Properties(self): # Class attributes
        return []

    def Objects(self): # Class aggregate objects
        return []

    def CppType(self): # C++ type of the object (e.g. may be array)
        return self.CppClass()

    def CppClass(self): # C++ class type of the object
        raise RuntimeError("can't instantiate %s" % self.name)

    def CppName(self): # C++ name of the object
        return self.true_name[0].upper() + self.true_name[1:]

    def CppDefValue(self): # Value to instantiate with in C++
        return ""

    # Whether a copy constructor would be needed if this type is a member of a class
    def NeedsCopyCtor(self):
        return False

    def TrueName(self):
        return self.true_name


class JsonNull(JsonType):
    def CppDefValue(self):
        return "nullptr"

    def CppType(self):
        return "void"


class JsonBoolean(JsonType):
    def CppClass(self):
        return TypePrefix("Boolean")

    def CppStdClass(self):
        return "bool"


class JsonNumber(JsonType):
    def __init__(self, name, parent, schema):
        JsonType.__init__(self, name, parent, schema)
        self.size = DEFAULT_INT_SIZE
        self.signed = False
        # NOTE: Take a hint on the size and signedness of the number/integer
        if "size" in schema:
            self.size = schema["size"]
        if "signed" in schema:
            self.signed = schema["signed"]

    def CppClass(self):
        return TypePrefix("Dec%sInt%i" % ("S" if self.signed else "U", self.size))

    def CppStdClass(self):
        return "%sint%i_t" % ("" if self.signed else "u", self.size)


class JsonInteger(JsonNumber):
    pass      # Identical as Number

class JsonFloat(JsonType):
    def CppClass(self):
        return TypePrefix("Float")

    def CppStdClass(self):
            return "float"

class JsonDouble(JsonType):
    def CppClass(self):
        return TypePrefix("Double")

    def CppStdClass(self):
        return "double"


class JsonString(JsonType):
    def CppClass(self):
        return TypePrefix("String")

    def CppStdClass(self):
        return "string"


class JsonEnum(JsonType):
    def __init__(self, name, parent, schema, enumType, included=None):
        self.cpptype = "undefined"
        JsonType.__init__(self, name, parent, schema, included)
        if enumType != "string":
            raise JsonParseError("Only strings are supported in enums")
        self.type = enumType
        if self.Create():
            self.enumName = MakeEnum(self.name.capitalize())
        self.enumerators = schema["enum"]
        self.values = schema["enumvalues"] if "enumvalues" in schema else []
        if self.values and (len(self.enumerators) != len(self.values)):
            raise JsonParseError("Mismatch in enumeration values in enum '%s'" % self.JsonName())
        self.strongly_typed = schema["enumtyped"] if "enumtyped" in schema else True
        self.default = self.CppClass() + "::" + self.CppEnumerators()[0]
        self.duplicate = False
        self.origRef = None
        self.refs = []
        self.AddRef(self)
        obj = enumTracker.Add(self)
        if obj:
            self.duplicate = True
            self.origRef = obj
            if obj.parent != parent:
                obj.AddRef(self)

    def CppType(self):
        return TypePrefix("EnumType<%s>" % self.CppClass())

    def CppClass(self):
        if self.IsDuplicate():
            # Use the original (ie. first seen) name
            return self.origRef.CppClass()
        else:
            classname = ""
            if not self.Create():
                # Override with real class name, this is likely comming from C++ header
                classname = self.cpptype
            elif "class" in self.schema:
                # Override class name if "class" property present
                classname = self.schema["class"].capitalize()
            elif CLASSNAME_FROM_REF and isinstance(self.schema, jsonref.JsonRef):
                # NOTE: Abuse the ref feature to construct a name for the enum!
                classname = MakeEnum(self.schema.__reference__["$ref"].rsplit(posixpath.sep, 1)[1].capitalize())
            elif isinstance(self.parent, JsonProperty):
                classname = MakeEnum(self.parent.name.capitalize())
            else:
                classname = self.enumName
            return classname

    def CppEnumerators(self):
        if self.Create() and "enumids" not in self.schema:
            return list(map(lambda x: ("E" if x[0].isdigit() else "") + x.upper(), self.enumerators))
        else:
            return self.schema["enumids"]

    def StringEnumerators(self):
        return self.enumerators

    def CppEnumeratorValues(self):
        return self.values

    def IsDuplicate(self):
        return self.duplicate

    def OrigName(self):
        return self.JsonName()

    def AddRef(self, obj):
        self.refs.append(obj)

    def RefCount(self):
        return len(self.refs)

    def CppStdClass(self):
        return self.CppClass()

    def IsStronglyTyped(self):
        return self.strongly_typed


class JsonObject(JsonType):
    def __init__(self, name, parent, schema, origName=None, included=None):
        self.origName = ((parent.JsonName() + ".") if parent.JsonName() else "") + name
        JsonType.__init__(self, name, parent, schema, included)
        self.properties = []
        self.objects = []
        self.enums = []
        self.duplicate = False
        self.origRef = None
        self.refs = []
        self.AddRef(self)
        # Handle duplicate objects...
        if "properties" in schema:
            obj = objTracker.Add(self)
            if obj:
                self.duplicate = True
                self.origRef = obj
                if obj.parent != parent:
                    obj.AddRef(self)
            for prop_name, prop in schema["properties"].items():
                newObject = JsonItem(prop_name, self, prop, included=included)
                self.properties.append(newObject)
                # Handle aggregate objects
                if isinstance(newObject, JsonObject):
                    self.objects.append(newObject)
                elif isinstance(newObject, JsonArray):
                    # Also add nested objects within arrays
                    o = newObject.Items()
                    while isinstance(o, JsonArray):
                        o = o.Items()
                    if isinstance(o, JsonObject):
                        self.objects.append(o)
                    if isinstance(o, JsonEnum):
                        self.enums.append(newObject.Items())
                elif isinstance(newObject, JsonEnum):
                    self.enums.append(newObject)
        if not self.Properties():
            log.Info("No properties in object %s" % self.origName)

    def CppName(self):
        # NOTE: Special cases for names for Methods and Arrays
        if isinstance(self.parent, JsonMethod):
            return self.parent.CppName() + JsonType.CppName(self)
        elif isinstance(self.parent, JsonArray):
            return self.parent.CppName()
        else:
            return JsonType.CppName(self)

    def CppClass(self):
        if self.IsDuplicate():
            # Use the original (ie. first seen) class name
            return self.origRef.CppClass()
        else:
            classname = ""
            if "class" in self.schema:
                # Override class name if "class" property present
                classname = self.schema["class"].capitalize()
            else:
                if not self.properties:
                    return TypePrefix("Container")
                elif CLASSNAME_FROM_REF and isinstance(self.schema, jsonref.JsonRef):
                    # NOTE: Abuse the ref feature to construct a name for the class!
                    classname = MakeObject(self.schema.__reference__["$ref"].rsplit(posixpath.sep, 1)[1].capitalize())
                else:
                    # Make the name out of properties, but not for params/result types
                    if len(self.Properties()) == 1 and not isinstance(self.parent, JsonMethod):
                        classname = MakeObject(self.Properties()[0].CppName())
                    elif isinstance(self.parent, JsonProperty):
                        classname = MakeObject(self.parent.CppName())
                    elif self.parent.parent and isinstance(self.parent.parent, JsonProperty):
                        classname = MakeObject(self.parent.parent.CppName())
                    elif "typename" in self.schema:
                        classname = self.schema["typename"].split("::")[-1]
                        return classname[0].upper() + classname[1:]
                    else:
                        classname = MakeObject(self.CppName())
            # For common classes append special suffix
            if self.RefCount() > 1:
                classname = classname.replace(OBJECT_SUFFIX, COMMON_OBJECT_SUFFIX)
            return classname

    def JsonName(self):
        return self.name

    def Objects(self):
        return self.objects

    def Enums(self):
        return self.enums

    def Properties(self):
        return self.properties

    def NeedsCopyCtor(self):
        # Check if a copy constructor is needed by scanning all duplicate classes
        filteredClasses = filter(lambda obj: obj.parent.NeedsCopyCtor() if self != obj else False, self.refs)
        foundInDuplicate = next(filteredClasses, None)
        return ALWAYS_COPYCTOR or self.parent.NeedsCopyCtor() or foundInDuplicate is not None

    def AddRef(self, obj):
        self.refs.append(obj)

    def RefCount(self):
        return len(self.refs)

    def IsDuplicate(self):
        return self.duplicate

    def OrigName(self):
        return self.origName

    def CppStdClass(self):
        return self.cpptype


class JsonArray(JsonType):
    def __init__(self, name, parent, schema, origName=None, included=None):
        JsonType.__init__(self, name, parent, schema, included)
        self.items = None
        if "items" in schema:
            self.items = JsonItem("element" if name == "params" else name , self, schema["items"], origName, included)
        else:
            raise JsonParseError("no items in array '%s'" % name)

    def CppName(self):
        # Take the name of the array from the method if this array is result type
        if isinstance(self.parent, JsonMethod):
            return self.parent.CppName() + JsonType.CppName(self)
        else:
            return JsonType.CppName(self)

    def CppType(self):
        return TypePrefix("ArrayType<%s>" % self.items.CppType())

    def NeedsCopyCtor(self):
        # Important, blame it on the arrays!
        return True

    def Items(self):
        return self.items

    # Delegate all other methods to the underlying type

    def CppClass(self):
        return self.Items().CppClass()

    def Enums(self):
        return self.Items().Enums()

    def Objects(self):
        return self.Items().Objects()

    def Properties(self):
        return self.Items().Properties()

    def IsDuplicate(self):
        return self.Items().IsDuplicate()

    def RefCount(self):
        return self.Items().RefCount()

    def CppStdClass(self):
        return "/* TODO */"


class JsonMethod(JsonObject):
    def __init__(self, name, parent, schema, included=None):
        if '.' in name:
            log.Warn("'%s': method names containing full designator are deprecated (include name only)" % name)
            objName = name.rsplit(".", 1)[1]
        else:
            objName = name
        # Mimic a JSON object to fit rest of the parsing...
        self.errors = schema["errors"] if "errors" in schema else OrderedDict()
        newschema = {"type": "object"}
        props = OrderedDict()
        props["params"] = schema["params"] if "params" in schema else {"type": "null"}
        props["result"] = schema["result"] if "result" in schema else {"type": "null"}
        newschema["properties"] = props
        JsonObject.__init__(self, objName, parent, newschema, included=included)
        self.true_name = schema["cppname"] if "cppname" in schema else objName
        self.summary = None
        self.tags = []
        if "summary" in schema:
            self.summary = schema["summary"]
        if "tags" in schema:
            self.tags = schema["tags"]
        self.deprecated = "deprecated" in schema and schema["deprecated"];
        self.obsolete = "obsolete" in schema and schema["obsolete"];

    def Errors(self):
        return self.errors

    def MethodName(self):
        return IMPL_ENDPOINT_PREFIX + JsonObject.JsonName(self)

    def Headline(self):
        return "%s%s%s" % (self.JsonName(), (" - " + self.summary.split(".", 1)[0]) if self.summary else "", " (DEPRECATED)" if self.deprecated else " (OBSOLETE)" if self.obsolete else "")


class JsonNotification(JsonMethod):
    def __init__(self, name, parent, schema, included=None):
        JsonMethod.__init__(self, name, parent, schema, included)
        if "id" in schema and "type" not in schema["id"]:
            schema["id"]["type"] = "string"
        self.sendif = JsonItem("id", self, schema["id"]) if "id" in schema else None
        self.statuslistener = schema["statuslistener"] if "statuslistener" in schema else False

    def HasSendif(self):
        return self.sendif != None

    def StatusListener(self):
        return self.statuslistener

    def MethodName(self):
        return IMPL_EVENT_PREFIX + JsonObject.JsonName(self)


class JsonProperty(JsonMethod):
    def __init__(self, name, parent, schema, included=None):
        JsonMethod.__init__(self, name, parent, schema, included)
        self.readonly = "readonly" in schema and schema["readonly"] == True
        self.writeonly = "writeonly" in schema and schema["writeonly"] == True
        if "index" in schema and "type" not in schema["index"]:
            schema["index"]["type"] = "string"
        self.index = JsonItem("index", self, schema["index"]) if "index" in schema else None

    def SetMethodName(self):
        return "set_" + JsonObject.JsonName(self)

    def GetMethodName(self):
        return "get_" + JsonObject.JsonName(self)

    def HasIndex(self):
        return self.index != None


class JsonRpcSchema(JsonType):
    def __init__(self, name, schema):
        JsonType.__init__(self, name, None, schema)
        self.info = None
        self.base_schema = None
        self.jsonrpc_version = None
        self.methods = []
        self.includes = []
        if "$schema" in schema:
            self.base_schema = schema["$schema"]
        if "jsonrpc" in schema:
            self.jsonrpc_version = schema["jsonrpc"]
        if "info" in schema:
            self.info = schema["info"]
        if "interface" in schema:
            schema = schema["interface"]
        if "include" in schema:
            for name, s in schema["include"].items():
                include = s["info"]["class"]
                self.includes.append(include)
                if "methods" in s:
                    for name, method in s["methods"].items():
                        newMethod = JsonMethod(name, self, method, include)
                        self.methods.append(newMethod)
                if "properties" in s:
                    for name, method in s["properties"].items():
                        newMethod = JsonProperty(name, self, method, include)
                        self.methods.append(newMethod)
                if "events" in s:
                    for name, method in s["events"].items():
                        newMethod = JsonNotification(name, self, method, include)
                        self.methods.append(newMethod)

        method_list = list(map(lambda x: x.name, self.methods))

        def __AddMethods(section, schema, ctor):
            if section in schema:
                for name, method in schema[section].items():
                    if name in method_list:
                        del self.methods[method_list.index(name)]
                        method_list.remove(name)
                    if method != None:
                        newMethod = ctor(name, self, method)
                        self.methods.append(newMethod)

        __AddMethods("methods", schema, lambda name, obj, method: JsonMethod(name, obj, method))
        __AddMethods("properties", schema, lambda name, obj, method: JsonProperty(name, obj, method))
        __AddMethods("events", schema, lambda name, obj, method: JsonNotification(name, obj, method))

        if not self.methods:
            raise JsonParseError("no methods, properties or events defined in '%s'" % name)

    def CppClass(self):
        return JsonType.CppName(self)

    def Properties(self):
        return self.methods

    def Objects(self):
        return self.Properties()

    def NeedsCopyCtor(self):
        return False

    def RefCount(self):
        return 1


def JsonItem(name, parent, schema, origName=None, included=None):
    # Create the appropriate Python object based on the JSON type
    if "type" in schema:
        if schema["type"] == "object":
            return JsonObject(name, parent, schema, origName, included)
        elif schema["type"] == "array":
            return JsonArray(name, parent, schema, origName, included)
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
    def PreprocessJson(file, string, include_path=None, cpp_include_path=None, header_include_paths = []):
        def __Tokenize(contents):
            # Tokenize the JSON first to be able to preprocess it easier
            formula = (
                                        # multi-line comments
                r"(/\*(.|[\r\n])*?\*/)"
                                        # single line comments
                r"|(//.*)"
                                        # double quotes
                r'|("(?:[^\\"]|\\.)*")'
                                        # singe quotes
                r"|('(?:[^\\']|\\.)*')"
                                        # single-char operators
                r"|([~,:;?=^/*-\+&<>\{\}\(\)\[\]])")
            tokens = [s.strip() for s in re.split(formula, contents, flags=re.MULTILINE) if s]
                                        # Remove comments from the JSON
            tokens = [s for s in tokens if (s and (s[:2] != '/*' and s[:2] != '//'))]
            return tokens

        path = os.path.abspath(os.path.dirname(file))
        tokens = __Tokenize(string)
        for c, t in enumerate(tokens):
            # BUG?: jsonref (urllib) needs file:// and absolute path to a ref'd file
            if t == '"$ref"' and tokens[c + 1] == ":" and tokens[c + 2][:2] != '"#':
                ref_file = tokens[c + 2].strip('"')
                ref_tok = ref_file.split("#", 1) if "#" in ref_file else [ref_file, ""]
                if "{interfacedir}/" in ref_file:
                    ref_tok[0] = ref_tok[0].replace("{interfacedir}/", (include_path + os.sep) if include_path else "")
                    if not include_path:
                        ref_tok[0] = os.path.join(path, ref_tok[0])
                else:
                    if os.path.isfile(os.path.join(path, ref_tok[0])):
                        ref_tok[0] = os.path.join(path, ref_tok[0])
                if not os.path.isfile(ref_tok[0]):
                    raise RuntimeError("$ref file '%s' not found" % ref_tok[0])
                ref_file = '"file:%s#%s"' % (urllib.request.pathname2url(ref_tok[0]), ref_tok[1])
                tokens[c + 2] = ref_file
            elif t == '"$cppref"' and tokens[c + 1] == ":" and tokens[c + 2][:2] != '"#':
                ref_file = tokens[c + 2].strip('"')
                ref_tok = ref_file.split("#", 1) if "#" in ref_file else [ref_file, ""]
                if "{cppinterfacedir}/" in ref_file:
                    ref_tok[0] = ref_tok[0].replace("{cppinterfacedir}/", (cpp_include_path + os.sep) if cpp_include_path else "")
                    if not cpp_include_path:
                        ref_tok[0] = os.path.join(path, ref_tok[0])
                else:
                    if os.path.isfile(os.path.join(path, ref_tok[0])):
                        ref_tok[0] = os.path.join(path, ref_tok[0])
                if not os.path.isfile(ref_tok[0]):
                    raise RuntimeError("$cppref file '%s' not found" % ref_tok[0])
                cppif = LoadInterface(ref_tok[0], header_include_paths)
                if cppif:
                    tokens[c] = json.dumps(cppif)
                    tokens[c - 1] = ""
                    tokens[c + 1] = ""
                    tokens[c + 2] = ""
                    if tokens[c + 4] == '"include"':
                        log.Warn("Using 'include' in 'interface' is deprecated, use a list of interfaces instead")
                        tokens[c + 16] = ""
                    else:
                        tokens[c + 3] = ""
                else:
                    raise RuntimeError("failed to parse C++ header '%s'" % ref_tok[0])
        # Return back the preprocessed JSON as a string
        return " ".join(tokens)

    with open(file, "r") as json_file:
        jsonPre = PreprocessJson(file, json_file.read(), include_path, cpp_include_path, header_include_paths)
        return jsonref.loads(jsonPre, object_pairs_hook=OrderedDict)



########################################################
#
# C++ TO JSON CONVERTER
#

def LoadInterface(file, includePaths = []):
    tree = ProxyStubGenerator.CppParser.ParseFiles([os.path.join(os.path.dirname(os.path.realpath(__file__)), posixpath.normpath(DEFAULT_DEFINITIONS_FILE)), file], includePaths)
    interfaces = [i for i in ProxyStubGenerator.Interface.FindInterfaceClasses(tree, INTERFACE_NAMESPACE, file) if i.obj.is_json]

    def Build(face):
        schema = OrderedDict()
        methods = OrderedDict()
        properties = OrderedDict()
        events = OrderedDict()

        schema["$schema"] = "interface.json.schema"
        schema["jsonrpc"] = "2.0"
        schema["dorpc"] = True
        schema["interfaceonly"] = True
        schema["fromheader"] = True
        schema["configuration"] = { "nodefault" : True }

        info = dict()
        if not face.obj.parent.full_name.endswith(INTERFACE_NAMESPACE):
            info["namespace"] = face.obj.parent.name
        info["class"] = face.obj.name[1:] if face.obj.name[0] == "I" else face.obj.name
        qualified_face = face.obj.full_name.split("::")[1:]
        if qualified_face[0] == FRAMEWORK_NAMESPACE:
            qualified_face = qualified_face[1:]
        info["interface"] = "::".join(qualified_face)
        info["sourcefile"] = os.path.basename(file)
        if face.obj.sourcelocation:
            info["sourcelocation"] = face.obj.sourcelocation
        info["title"] = info["class"] + " API"
        info["description"] = info["class"] + " JSON-RPC interface"
        schema["info"] = info

        event_interfaces = set()

        for method in face.obj.methods:
            def ResolveTypedef(type):
                if isinstance(type.Type(), ProxyStubGenerator.CppParser.Typedef):
                    return ResolveTypedef(type.Type().type)
                else:
                    return type

            def StripFrameworkNamespace(identifier):
                return str(identifier).replace("::" + FRAMEWORK_NAMESPACE + "::", "")

            def StripInterfaceNamespace(identifier):
                return str(identifier).replace(INTERFACE_NAMESPACE + "::", "").replace("::" + FRAMEWORK_NAMESPACE + "::", "")

            def ConvertType(var):
                cppType = ResolveTypedef(var.type).Type()
                # Pointers
                if var.type.IsPointer():
                    # Special case for serializing C-style buffers, that will be converted to base64 encvoded strings
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
                    elif isinstance(cppType, ProxyStubGenerator.CppParser.Class) and cppType.is_iterator and len(cppType.args) == 2:
                        # Take element type from return value of the Current() method
                        currentMethod = next((x for x in cppType.methods if x.name == "Current"), None)
                        if currentMethod == None:
                            raise CppParseError(var, "%s does not appear to a be an @iterator type" % cppType.type)
                        return "array", { "items": ConvertParameter(currentMethod.retval), "iterator": StripInterfaceNamespace(cppType.type) }
                    # All other pointer types are not supported
                    else:
                        raise CppParseError(var, "unable to convert C++ type to JSON type: %s" % cppType.type)
                # Primitives
                else:
                    # String
                    if isinstance(cppType, ProxyStubGenerator.CppParser.String):
                        return "string", None
                    # Boolean
                    elif isinstance(cppType, ProxyStubGenerator.CppParser.Bool):
                        return "boolean", None
                    # Integer
                    elif isinstance(cppType, ProxyStubGenerator.CppParser.Integer):
                        size = 8 if cppType.size == "char" else 16 if cppType.size == "short" else \
                            32 if cppType.size == "int" or cppType.size == "long" else 64 if cppType.size == "long long" else 32
                        return "integer", { "size": size, "signed": cppType.signed }
                    # Float
                    elif isinstance(cppType, ProxyStubGenerator.CppParser.Float):
                        return "float", 32 if CppType.type == "float" else 64 if CppType.type == "double" else 128
                    # Null
                    elif isinstance(cppType, ProxyStubGenerator.CppParser.Void):
                        return "null", None
                    # Enums
                    elif isinstance(cppType, ProxyStubGenerator.CppParser.Enum):
                        if len(cppType.items) > 1:
                            enumValues = [e.autoValue for e in cppType.items]
                            for i, e in enumerate(cppType.items, 0):
                                if enumValues[i - 1] != enumValues[i]:
                                    raise CppParseError(var, "enumerator values in an enum must all be explicit or all be implied")
                        enumSpec = { "enum": [e.meta.text if e.meta.text else e.name.replace("_"," ").title().replace(" ","") for e in cppType.items], "enumtyped": var.type.Type().scoped  }
                        enumSpec["enumids"] = [e.name for e in cppType.items]
                        enumSpec["class"] = var.type.Type().name
                        if not cppType.items[0].autoValue:
                            enumSpec["enumvalues"] = [e.value for e in cppType.items]
                        return "string", enumSpec
                    # POD objects
                    elif isinstance(cppType, ProxyStubGenerator.CppParser.Class):
                        def GenerateObject(ctype):
                            properties = dict()
                            for p in ctype.vars:
                                name = p.name.lower()
                                if isinstance(ResolveTypedef(p.type).Type(), ProxyStubGenerator.CppParser.Class):
                                    _, props = GenerateObject(ResolveTypedef(p.type).Type())
                                    properties[p.name] = props
                                    properties[p.name]["type"] = "object"
                                    properties[p.name]["typename"] = StripFrameworkNamespace(p.type.Type().full_name)
                                else:
                                    properties[p.name] = ConvertParameter(p)
                                properties[p.name]["original"] = p.name.lower()
                            return "object", { "properties": properties, "required": list(properties.keys()) }
                        return GenerateObject(cppType)
                    # All other types are not supported
                    else:
                        raise CppParseError(var, "unable to convert C++ type to JSON type: %s" % cppType.type)

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
                        properties["typename"] = StripFrameworkNamespace(var.type.Type().full_name)
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
                        if isinstance(type, list):
                            raise CppParseError(var, "undefined type: '%s'" % " ".join(type))
                        resolved.append(type)
                        return events

                    resolved = []
                    events = ResolveTypedef(resolved, events, var.type)
                return events

            def BuildParameters(vars, json_extended, prop=False, test=False):
                params = {"type": "object"}
                properties = OrderedDict()
                required = []
                for var in vars:
                    if var.meta.input or not var.meta.output:
                        if not var.type.IsConst() or (var.type.IsPointer() and not var.type.IsPointerToConst()):
                            if not var.meta.input:
                                log.WarnLine(var, "'%s': non-const parameter assumed to be input (forgot 'const'?)" % var.name)
                            elif not var.meta.output:
                                log.WarnLine(var, "'%s': non-const parameter marked with @in tag (forgot 'const'?)" % var.name)
                        var_name = var.meta.text if var.meta.text else var.name.lower()
                        if var_name.startswith("__unnamed") and not test:
                            raise CppParseError(var, "unnamed parameter, can't deduce parameter name")
                        properties[var_name] = ConvertParameter(var)
                        properties[var_name]["original"] = var.name.lower()
                        properties[var_name]["position"] = vars.index(var)
                        if not prop and "description" not in properties[var_name]:
                            log.DocIssue("'%s': parameter is missing description" % var_name)
                        required.append(var_name)
                        if properties[var_name]["type"] == "string" and not var.type.IsReference() and not var.type.IsPointer() and not "enum" in properties[var_name]:
                            log.WarnLine(var, "'%s': passing string by value (forgot &?)" % var.name)
                params["properties"] = properties
                params["required"] = required
                if prop:
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
                    elif (len(properties) == 1) and not json_extended:
                        # New way of things: if only one parameter present then omit the outer object
                        return list(properties.values())[0]
                    else:
                        return params

            def BuildResult(vars, prop = False):
                params = {"type": "object"}
                properties = OrderedDict()
                required = []
                for var in vars:
                    if var.meta.output:
                        if var.type.IsValue():
                            raise CppParseError(var, "parameter marked with @out tag must be either a reference or a pointer")
                        if var.type.IsConst():
                            raise CppParseError(var, "parameter marked with @out tag must not be const")
                        var_name = var.meta.text if var.meta.text else var.name.lower()
                        if var_name.startswith("__unnamed") and len(vars) > 1:
                            raise CppParseError(var, "unnamed parameter, can't deduce parameter name")
                        properties[var_name] = ConvertParameter(var)
                        properties[var_name]["original"] = var.name.lower()
                        properties[var_name]["position"] = vars.index(var)
                        required.append(var_name)
                params["properties"] = properties
                if len(properties) == 1:
                    return list(properties.values())[0]
                elif len(properties) > 1:
                    params["required"] = required
                    return params
                elif not prop:
                    void = {"type": "null"}
                    void["description"] = "Always null"
                    return void
                else:
                    return None

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
                    obj["cppname"] = method.name
                except:
                    obj = OrderedDict()
                    obj["cppname"] = method.name
                    properties[prefix + method_name_lower] = obj

                indexed_property = (len(method.vars) == 2 and method.vars[0].meta.is_index)

                if len(method.vars) == 1 or (len(method.vars) == 2 and indexed_property):
                    if indexed_property:
                        if method.vars[0].type.IsPointer():
                            raise CppParseError(method.vars[0], "index to a property must not be pointer")
                        if not method.vars[0].type.IsConst() and method.vars[0].type.IsReference():
                            raise CppParseError(method.vars[0], "index to a property must be an input parameter")
                        if "index" not in obj:
                            obj["index"] = BuildParameters([method.vars[0]], False, True)
                            obj["index"]["name"] = method.vars[0].name
                            if "enum" in obj["index"]:
                                obj["index"]["example"] = obj["index"]["enum"][0]
                            if "example" not in obj["index"]:
                                # example not specified, let's invent something...
                                obj["index"]["example"] = ("0" if obj["index"]["type"] == "integer" else "xyz")
                            if obj["index"]["type"] not in ["integer", "string"]:
                                raise CppParseError(method.vars[0], "index to a property must be integer, enum or string type")
                        else:
                            test = BuildParameters([method.vars[0]], False, True, True)
                            if not test:
                                raise CppParseError(method.vars[value], "property index must be an input parameter")
                            if obj["index"]["type"] != test["type"]:
                                raise CppParseError(method.vars[0], "setter and getter of the same property must have same index type")
                        if method.vars[1].meta.is_index:
                            raise CppParseError(method.vars[0], "index must be the first parameter to property method")

                        value = 1
                    else:
                        value = 0

                    if "const" in method.qualifiers:
                        if method.vars[value].type.IsConst():
                            raise CppParseError(method.vars[value], "property getter method must not use const parameter")
                        else:
                            if "writeonly" in obj:
                                del obj["writeonly"]
                            else:
                                obj["readonly"] = True
                            if "params" not in obj:
                                obj["params"] = BuildResult([method.vars[value]], True)
                            else:
                                test = BuildResult([method.vars[value]], True)
                                if not test:
                                    raise CppParseError(method.vars[value], "property getter method must have one output parameter")
                                if obj["params"]["type"] != test["type"]:
                                    raise CppParseError(method.vars[value], "setter and getter of the same property must have same type")
                            if obj["params"] == None:
                                raise CppParseError(method.vars[value], "property getter method must have one output parameter")
                    else:
                        if not method.vars[value].type.IsConst():
                            raise CppParseError(method.vars[value], "property setter method must use a const parameter")
                        else:
                            if "readonly" in obj:
                                del obj["readonly"]
                            else:
                                obj["writeonly"] = True
                            if "params" not in obj:
                                obj["params"] = BuildParameters([method.vars[value]], False, True)
                            else:
                                test = BuildParameters([method.vars[value]], False, True, True)
                                if not test:
                                    raise CppParseError(method.vars[value], "property setter method must have one input parameter")
                                if obj["params"]["type"] != test["type"]:
                                    raise CppParseError(method.vars[value], "setter and getter of the same property must have same type")
                            if obj["params"] == None:
                                raise CppParseError(method.vars[value], "property setter method must have one input parameter")
                            if method.vars[value].type.IsReference():
                                obj["params"]["ref"] = True


                else:
                    raise CppParseError(method, "property method must have one parameter")

            elif method.IsPureVirtual() and not event_params:
                if method.retval.type and (isinstance(method.retval.type.Type(), ProxyStubGenerator.CppParser.Void) or (isinstance(method.retval.type.Type(), ProxyStubGenerator.CppParser.Integer) and method.retval.type.Type().size == "long")):
                    obj = OrderedDict()
                    params = BuildParameters(method.vars, face.obj.is_extended)
                    if "properties" in params and params["properties"]:
                        if method.name.lower() in [x.lower() for x in params["required"]]:
                            raise CppParseError(method, "parameters must not use the same name as the method")
                    if params:
                        obj["params"] = params
                    obj["result"] = BuildResult(method.vars)
                    obj["cppname"] = method_name
                    methods[prefix + method_name_lower] = obj
                else:
                    raise CppParseError(method, "method return type must be uint32_t (error code) or void (i.e. pass other return values by reference)")

            if obj:
                if method.retval.meta.is_deprecated:
                    obj["deprecated"] = True
                elif method.retval.meta.is_obsolete:
                    obj["obsolete"] = True
                if method.retval.meta.brief:
                    obj["summary"] = method.retval.meta.brief
                elif (prefix + method_name_lower) not in properties:
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
            for method in f.obj.methods:
                if method.IsPureVirtual() and method.is_excluded == False:
                    obj = OrderedDict()
                    obj["cppname"] = method.name
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
                            obj["id"] = BuildParameters([method.vars[0]], False)
                            obj["id"]["name"] = method.vars[0].name
                            if "example" not in obj["id"]:
                                obj["id"]["example"] = "0" if obj["id"]["type"] == "integer" else "abc"
                            if obj["id"]["type"] not in ["integer", "string"]:
                                raise CppParseError(method.vars[0], "index to a notification must be integer, enum or string type")
                            varsidx = 1
                    if method.retval.meta.is_listener:
                        obj["statuslistener"] = True
                    params = BuildParameters(method.vars[varsidx:], f.obj.is_extended)
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

    return schemas



########################################################
#
# JSON OBJECT TRACKER
#

def SortByDependency(objects):
    sortedObjects = []
    # This will order objects by their relations
    for obj in sorted(objects, key=lambda x: x.CppClass(), reverse=False):
        found = filter(lambda sortedObj: obj.CppClass() in map(lambda x: x.CppClass(), sortedObj.Objects()),
                       sortedObjects)
        try:
            index = min(map(lambda x: sortedObjects.index(x), found))
            movelist = filter(lambda x: x.CppClass() in map(lambda x: x.CppClass(), sortedObjects), obj.Objects())
            sortedObjects.insert(index, obj)
            for m in movelist:
                if m in sortedObjects:
                    sortedObjects.insert(index, sortedObjects.pop(sortedObjects.index(m)))
        except ValueError:
            sortedObjects.append(obj)
    return sortedObjects

def IsInRef(obj):
    while obj:
        if (isinstance(obj.Schema(), jsonref.JsonRef)):
            return True
        obj = obj.parent
    return False


class ObjectTracker:
    def __init__(self):
        self.objects = []
        self.Reset()

    def Add(self, newObj):
        def __CompareObject(lhs, rhs):
            def __CompareType(lhs, rhs):
                if rhs["type"] != lhs["type"]:
                    return False
                elif "size" in lhs:
                    if "size" in rhs:
                        if lhs["size"] != rhs["size"]:
                            return False
                    elif "size" != 32:
                        return False
                elif "size" in rhs:
                    if rhs["size"] != 32:
                        return False
                elif "signed" in lhs:
                    if "signed" in rhs:
                        if lhs["signed"] != rhs["signed"]:
                            return False
                    elif lhs["signed"] != False:
                        return False
                elif "signed" in rhs:
                    if rhs["signed"] != False:
                        return False
                elif "enum" in lhs:
                    if "enum" in rhs:
                        if lhs["enum"] != rhs["enum"]:
                            return False
                    else:
                        return False
                elif "enum" in rhs:
                    return False
                elif "enumvalues" in lhs:
                    if "enumvalues" in rhs:
                        if lhs["enumvalues"] != rhs["enumvalues"]:
                            return False
                    else:
                        return False
                elif "enumvalues" in rhs:
                    return False
                elif "enumids" in lhs:
                    if "enumids" in rhs:
                        if lhs["enumids"] != rhs["enumids"]:
                            return False
                    else:
                        return False
                elif "enumids" in rhs:
                    return False
                elif "items" in lhs:
                    if "items" in rhs:
                        if not __CompareType(lhs["items"], rhs["items"]):
                            return False
                    else:
                        return False
                elif "items" in rhs:
                    return False
                elif "properties" in lhs:
                    if "properties" in rhs:
                        if not __CompareObject(lhs["properties"], rhs["properties"]):
                            return False
                    else:
                        return False
                return True

            # NOTE: Two objects are considered identical if they have the same property names and types only!
            for name, prop in lhs.items():
                if name not in rhs:
                    return False
                else:
                    if not __CompareType(prop, rhs[name]):
                        return False
            for name, prop in rhs.items():
                if name not in lhs:
                    return False
                else:
                    if not __CompareType(prop, lhs[name]):
                        return False
            return True

        if "properties" in newObj.Schema() and not isinstance(newObj, JsonMethod):
            self.objects.append(newObj)
            is_ref = IsInRef(newObj)
            props = newObj.Schema()["properties"]
            for obj in self.Objects()[:-1]:
                if __CompareObject(obj.Schema()["properties"], props):
                    if not GENERATED_JSON and not NO_DUP_WARNINGS and (not is_ref or not IsInRef(obj)):
                        log.Warn("Duplicate object '%s' (same as '%s') - consider using $ref" %
                                   (newObj.OrigName(), obj.OrigName()))
                    return obj
            return None

    def Objects(self):
        return self.objects

    def Reset(self):
        self.objects = []

    def CommonObjects(self):
        return SortByDependency(filter(lambda obj: obj.RefCount() > 1, self.Objects()))


class EnumTracker(ObjectTracker):
    def __IsTopmost(self, obj):
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
                return True
            else:
                return False

        if "enum" in newObj.Schema() and not isinstance(newObj, JsonMethod):
            self.objects.append(newObj)
            is_ref = IsInRef(newObj)
            for obj in self.Objects()[:-1]:
                if __Compare(obj.Schema(), newObj.Schema()):
                    if not GENERATED_JSON and not NO_DUP_WARNINGS and (not is_ref or not IsInRef(obj)):
                        log.Warn("Duplicate enums '%s' (same as '%s') - consider using $ref" %
                                   (newObj.OrigName(), obj.OrigName()))
                    return obj
            return None

    def CommonObjects(self):
        return SortByDependency(filter(lambda obj: obj.RefCount() > 1 or self.__IsTopmost(obj), self.Objects()))


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
    if obj.Create() and isinstance(obj, (JsonObject, JsonEnum, JsonArray)):
        if isinstance(obj, JsonObject) and not obj.properties:
            return namespace
        fullname = ""
        e = obj
        while e.parent and not isinstance(e, JsonMethod) and (not e.IsDuplicate() or e.RefCount() > 1):
            if not isinstance(e, (JsonEnum, JsonArray)):
                fullname = e.CppClass() + "::" + fullname
            if e.RefCount() > 1:
                break
            e = e.parent
        if full:
            namespace = "%s::%s::" % (DATA_NAMESPACE, root.TrueName())
        namespace = namespace + "%s" % fullname
    return namespace


def EmitEnumRegs(root, emit, header_file, if_file):
    def EmitEnumRegistration(root, enum, full=True):
        fullname = (GetNamespace(root, enum) if full else "%s::%s::" %
                    (DATA_NAMESPACE, root.TrueName())) + enum.CppClass()
        emit.Line("ENUM_CONVERSION_BEGIN(%s)" % fullname)
        emit.Indent()
        for c, item in enumerate(enum.enumerators):
            emit.Line("{ %s::%s, _TXT(\"%s\") }," % (fullname, enum.CppEnumerators()[c], enum.StringEnumerators()[c]))
        emit.Unindent()
        emit.Line("ENUM_CONVERSION_END(%s);" % fullname)

    # Enumeration conversion code
    emit.Line("#include \"definitions.h\"")
    if if_file.endswith(".h"):
        emit.Line("#include <%s%s>" % (CPP_IF_PATH, if_file))
    emit.Line("#include <core/Enumerate.h>")
    emit.Line("#include \"%s_%s.h\"" % (DATA_NAMESPACE, header_file))
    emit.Line()
    emit.Line("namespace %s {" % FRAMEWORK_NAMESPACE)
    count = 0
    if enumTracker.Objects():
        for obj in enumTracker.Objects():
            if not obj.IsDuplicate() and not obj.included_from:
                emit.Line()
                EmitEnumRegistration(root, obj, obj.RefCount() == 1 or not obj.Create())
                count += 1
    emit.Line()
    emit.Line("}")
    return count


##############################################################################
#
# JSON-RPC CODE GENERATOR
#

def EmitEvent(emit, root, event, static=False):
    emit.Line("// Event: %s" % event.Headline())
    params = event.Properties()[0].CppType()
    par = ""
    if params != "void":
        par = ("const %s& id, " % event.sendif.CppStdClass()) if event.HasSendif() else ""
        if event.Properties()[0].Properties() and event.Properties()[0].Create():
            par = par + ", ".join(map(lambda x: "const " + (GetNamespace(root, x, False) if not static else "") + x.CppStdClass() + "& " + x.JsonName(), event.Properties()[0].Properties()))
        else:
            x = event.Properties()[0]
            par = par + "const " + (GetNamespace(root, x, False) if not static else "") + x.CppStdClass() + "& " + x.JsonName()
    if not static:
        line = "void %s::%s(%s)" % (root.JsonName(), event.MethodName(), par)
    else:
        line = "static void %s(PluginHost::JSONRPC& module%s%s)" % (event.TrueName(), ", " if par else "", par)
    if event.included_from:
        line += " /* %s */" % event.included_from
    emit.Line(line)
    emit.Line("{")
    emit.Indent()

    if params != "void":
        emit.Line("%s params;" % params)
        if event.Properties()[0].Properties() and event.Properties()[0].Create():
            for p in event.Properties()[0].Properties():
                if isinstance(p, JsonEnum):
                    emit.Line("params.%s = static_cast<%s>(%s);" % (p.CppName(), GetNamespace(root, p, False) + p.CppClass(), p.JsonName()))
                else:
                    emit.Line("params.%s = %s;" % (p.CppName(), p.JsonName()))
        else:
            emit.Line("params = %s;" % event.Properties()[0].JsonName())
        emit.Line()
    if event.HasSendif():
        index_var = "designatorId"
        emit.Line('%sNotify(_T("%s")%s, [&id](const string& designator) -> bool {' %
                  ("module." if static else "", event.JsonName(), ", params" if params != "void" else ""))
        emit.Indent()
        emit.Line("const string %s = designator.substr(0, designator.find('.'));" % index_var)
        if isinstance(event.sendif, JsonInteger):
            index_var = "_designatorIdInt"
            type = event.sendif.CppStdClass()
            emit.Line("%s %s{};" % (type, index_var))
            emit.Line("if (Core::FromString(%s, %s) == false) {" % ("designatorId", index_var))
            emit.Indent()
            emit.Line("return (false);")
            emit.Unindent()
            emit.Line("} else {")
            emit.Indent()
        elif isinstance(event.sendif, JsonEnum):
            index_var = "_designatorIdEnum"
            type = event.sendif.CppStdClass()
            emit.Line("Core::EnumerateType<%s> _value(%s.c_str());" % (type, "designatorId"))
            emit.Line("const %s %s = _value.Value();" % ( type, index_var))
            emit.Line("if (_value.IsSet() == false) {")
            emit.Indent()
            emit.Line("return (false);")
            emit.Unindent()
            emit.Line("} else {")
            emit.Indent()
        emit.Line("return (id == %s);" % index_var)
        if not isinstance(event.sendif, JsonString):
            emit.Unindent()
            emit.Line("}")
        emit.Unindent()
        emit.Line("});")
    else:
        emit.Line('%sNotify(_T("%s")%s);' %
                  ("module." if static else "", event.JsonName(), ", params" if params != "void" else ""))
    emit.Unindent()
    emit.Line("}")
    emit.Line()

def EmitRpcCode(root, emit, header_file, source_file, data_emitted):

    struct = "J" + root.JsonName()
    face = "I" + root.JsonName()

    emit.Line("// Generated automatically from '%s'. DO NOT EDIT." % os.path.basename(source_file))
    emit.Line()
    emit.Line("#pragma once")
    emit.Line()
    emit.Line("#include \"Module.h\"")
    if data_emitted:
        emit.Line("#include \"%s_%s.h\"" % (DATA_NAMESPACE, header_file))
    emit.Line("#include <%s%s>" % (CPP_IF_PATH, source_file))
    emit.Line()
    emit.Line("namespace %s {" % FRAMEWORK_NAMESPACE)
    emit.Line()
    emit.Line("namespace %s {" % "Exchange")
    emit.Indent()
    emit.Line()
    destination_var = "_destination"
    namespace = root.JsonName()
    if "info" in root.schema and "namespace" in root.schema["info"]:
        namespace = root.schema["info"]["namespace"] + "::" + namespace
        emit.Line("namespace %s {" % root.schema["info"]["namespace"])
        emit.Indent()
        emit.Line()
    namespace = DATA_NAMESPACE + "::" + namespace
    emit.Line("namespace %s {" % struct)
    emit.Indent()
    emit.Line()
    if data_emitted:
        emit.Line("using namespace %s;" % namespace)
        emit.Line()
    emit.Line("static void Register(PluginHost::JSONRPC& module, %s* %s)" % (face, destination_var))
    emit.Line("{")
    emit.Indent()
    emit.Line("ASSERT(%s != nullptr);" % destination_var)
    emit.Line()

    events = []

    for m in root.Properties():
        if not isinstance(m, JsonNotification):

            indexed = isinstance(m, JsonProperty) and m.HasIndex()
            index_var = "_index"
            index_param = index_var
            # Emit method prologue
            if isinstance(m, JsonProperty):
                void = m.Properties()[1]
                params = m.Properties()[0] if not m.readonly else void
                params.true_name = "params"
                params.name = params.true_name
                response = copy.deepcopy(m.Properties()[0]) if not m.writeonly else void
                response.true_name = "result"
                response.name = response.true_name
                emit.Line("// %sProperty: %s%s" % ("Indexed " if indexed else "", m.Headline(), " (r/o)" if m.readonly else (" (w/o)" if m.writeonly else "")))
            else:
                params = m.Properties()[0]
                response = m.Properties()[1]
                emit.Line("// Method: %s" % m.Headline())
            line = 'module.Register<%s, %s%s>(_T("%s"),' % (params.CppType(), response.CppType(), ", std::function<uint32_t(const std::string&, %s%s)>" % ("" if params.CppType() == "void" else ("const " + params.CppType() + "&"), "" if response.CppType() == "void" else (("" if params.CppType() == "void" else ", ") + response.CppType() + "&")) if indexed else "", m.JsonName())
            emit.Line(line)
            emit.Indent()
            line = '[%s](' % destination_var
            if indexed:
                line = line + "const string& %s, " % index_param
            line = line + (("const " + params.CppType() + "& " + params.CppName()) if params.CppType() != "void" else "") + \
                (", " if params.CppType() != "void" and response.CppType() != "void" else "") + \
                ((response.CppType() + "& " + response.CppName())
                 if response.CppType() != "void" else "")
            line = line + ') -> uint32_t {'
            emit.Line(line)
            emit.Indent()
            errorcode_var = "_errorCode"
            emit.Line("uint32_t %s;" % errorcode_var)

            READ_ONLY = 0
            READ_WRITE = 1
            WRITE_ONLY = 2

            def Invoke(params, response, const_cast=False, parent = None):
                vars = OrderedDict()

                # Build param/response dictionaries (dictionaries will ensure they do not repeat)
                if params.CppType() != "void":
                    if isinstance(params, JsonObject) and params.Create():
                        for p in params.Properties():
                            vars[p.JsonName()] = [p, READ_ONLY]
                    else:
                        vars[params.JsonName()] = [params, READ_ONLY]

                if response.CppType() != "void":
                    if isinstance(response, JsonObject) and response.Create():
                        for p in response.Properties():
                            if p.JsonName() not in vars:
                                vars[p.JsonName()] = [p, WRITE_ONLY]
                            else:
                               vars[p.JsonName()][1] = READ_WRITE
                    else:
                        if response.JsonName() not in vars:
                            vars[response.JsonName()] = [response, WRITE_ONLY]
                        else:
                            vars[response.JsonName()][1] = READ_WRITE

                for _, t in vars.items():
                    if isinstance(t[0], JsonString) and "length" in t[0].schema:
                        for w, q in vars.items():
                            if w == t[0].schema["length"]:
                                # Have to handle any order of lenght/buffer params
                                q[0].schema["bufferlength"] = True
                                if q[1] == WRITE_ONLY:
                                    raise RuntimeError("'%s': parameter marked pointed to by @length is output only" % q[0].name)

                # Emit temporary variables and deserializing of JSON data
                for _, t in sorted(vars.items(), key=lambda x: x[1][0].schema["position"]):
                    if "bufferlength" in t[0].schema:
                        t[0].release = False
                        t[0].cast = False
                        continue
                    # C-style buffers
                    t[0].release = False
                    t[0].cast = None
                    if isinstance(t[0], JsonString) and "length" in t[0].schema:
                        for w, q in vars.items():
                            if w == t[0].schema["length"]:
                                emit.Line("%s %s{%s};" % (q[0].CppStdClass(), q[0].JsonName(), "%s%s.Value()" % (parent if parent else "", q[0].CppName()) if q[1] != WRITE_ONLY else ""))
                                break
                        encode = "encode" in t[0].schema and t[0].schema["encode"]
                        if t[1] == READ_ONLY and not encode:
                            emit.Line("const %s* %s{%s%s.Value().data()};" % (t[0].schema["cpptype"], t[0].JsonName(),  parent if parent else "", t[0].CppName()))
                        else:
                            emit.Line("%s* %s = nullptr;" % (t[0].schema["cpptype"], t[0].JsonName()))
                            emit.Line("if (%s != 0) {" % t[0].schema["length"])
                            emit.Indent()
                            emit.Line("%s = reinterpret_cast<%s*>(ALLOCA(%s));" % (t[0].JsonName(), t[0].schema["cpptype"], t[0].schema["length"]))
                            emit.Line("ASSERT(%s != nullptr);" % t[0].JsonName())

                        if t[1] != WRITE_ONLY:
                            if encode:
                                emit.Line("// Decode base64-encoded JSON string")
                                emit.Line("Core::FromString(%s%s.Value(), %s, %s, nullptr);" % (parent if parent else "", t[0].CppName(), t[0].JsonName(), t[0].schema["length"]))
                            elif t[1] != READ_ONLY:
                                emit.Line("::memcpy(%s, %s%s.Value().data(), %s);" % (t[0].JsonName(), parent if parent else "", t[0].CppName(), t[0].schema["length"]))
                        if t[1] != READ_ONLY or encode:
                            emit.Unindent()
                            emit.Line("}")
                    # Iterators
                    elif isinstance(t[0], JsonArray):
                        if "iterator" in t[0].schema:
                            if t[1] == READ_ONLY:
                                emit.Line("std::list<%s> elements;" %(t[0].items.CppStdClass()))
                                emit.Line("auto iterator = %s.Elements();" % ((parent if parent else "") + t[0].CppName()))
                                emit.Line("while (iterator.Next() == true) {")
                                emit.Indent()
                                emit.Line("elements.push_back(iterator.Current()%s);" % (".Value()" if not isinstance(t[0].items, JsonObject) else ""))
                                emit.Unindent()
                                emit.Line("}")
                                impl = t[0].schema["iterator"][:t[0].schema["iterator"].index('<')].replace("IIterator", "Iterator") + "<%s>" % t[0].schema["iterator"]
                                initializer = "Core::Service<%s>::Create<%s>(elements)" % (impl, t[0].schema["iterator"])
                                emit.Line("%s* %s{%s};" % (t[0].schema["iterator"], t[0].JsonName(), initializer))
                                t[0].release = True
                                if "ref" in t[0].schema and t[0].schema["ref"]:
                                    t[0].cast = "static_cast<%s* const&>(%s)" % (t[0].schema["iterator"], t[0].JsonName())
                            elif t[1] == WRITE_ONLY:
                                emit.Line("%s* %s{};" % (t[0].schema["iterator"], t[0].JsonName()))
                            else:
                                raise RuntimeError("Read/write arrays are not supported: %s" % t[0].JsonName())

                    # PODs
                    elif isinstance(t[0], JsonObject):
                        emit.Line("%s%s %s%s;" % ("const " if t[1] == READ_ONLY else "", t[0].CppStdClass(), t[0].JsonName(), "(%s%s)" % (parent if parent and not isinstance(t[0].parent, JsonMethod) else "", t[0].CppName()) if t[1] != WRITE_ONLY else "{}"))
                        """
                        def EmitObject(obj, cpp_parent, json_parent):
                            if t[1] != WRITE_ONLY:
                                for e in obj.Properties():
                                    if isinstance(e, JsonObject):
                                        EmitObject(e, cpp_parent + e.TrueName() + ".", json_parent + e.CppName() + ".")
                                    else:
                                        emit.Line("%s%s = %s%s.Value();" % (cpp_parent, e.TrueName(), json_parent, e.CppName()))

                        if not t[0].Create():
                            emit.Line("%s %s%s;" % (t[0].CppStdClass(), t[0].JsonName(), "{}" if t[1] == WRITE_ONLY else ""))
                            EmitObject(t[0], t[0].JsonName() + ".", (parent if parent and not isinstance(t[0].parent, JsonMethod) else "") + t[0].CppName() + ".");
                        else:
                            emit.Line("%s %s%s;" % (t[0].CppType(), t[0].JsonName(), "{}" if t[1] == WRITE_ONLY else ""))
                        """
                    # All Other
                    else:
                        emit.Line("%s%s %s{%s};" % ("const " if (t[1] == READ_ONLY) else "", t[0].CppStdClass(), t[0].JsonName(), "%s%s.Value()" % (parent if parent else "", t[0].CppName()) if t[1] != WRITE_ONLY else ""))

                cond = ""
                for v, t in vars.items():
                    if t[0].release:
                        cond += "(%s != nullptr) &&" % t[0].JsonName()
                if cond:
                    emit.Line("if (%s) {" % cond[:-3])
                    emit.Indent()

                # Emit call the API
                if const_cast:
                    line = "%s = (static_cast<const %s*>(%s))->%s(" % (errorcode_var, face, destination_var, m.TrueName())
                else:
                    line = "%s = %s->%s(" % (errorcode_var, destination_var, m.TrueName())
                if indexed:
                    line = line + index_var + ", "
                for v, t in sorted(vars.items(), key=lambda x: x[1][0].schema["position"]):
                    line = line + ("%s, " % (t[0].cast if t[0].cast else t[0].JsonName()))
                if line.endswith(", "):
                    line = line[:-2]
                line = line + ");"
                emit.Line(line)

                if cond:
                    for v, t in vars.items():
                        if t[0].release:
                            emit.Line("%s->Release();" % t[0].JsonName())
                    emit.Unindent()
                    emit.Line("} else {")
                    emit.Indent()
                    emit.Line("%s = Core::ERROR_GENERAL;" % errorcode_var)
                    emit.Unindent()
                    emit.Line("}")

                # Emit result handling and serializing JSON data
                if response.CppType() != "void":
                    emit.Line("if (%s == Core::ERROR_NONE) {" % errorcode_var)
                    emit.Indent()

                    def EmitResponse(elem, cppParent = "", parent = ""):
                        # C-style buffers disguised as base64-encoded strings
                        if isinstance(elem, JsonObject) and elem.Create():
                            for p in elem.Properties():
                                EmitResponse(p, cppParent + (((elem.TrueName() if cppParent else elem.JsonName()) + ".") if not elem.Create() else ""), parent + elem.CppName() + "." )
                        # C-style buffers disguised as base64-encoded strings
                        elif isinstance(elem, JsonString) and "length" in elem.schema:
                            emit.Line("if (%s != 0) {" % elem.schema["length"])
                            emit.Indent()
                            if "encode" in elem.schema and elem.schema["encode"]:
                                emit.Line("// Convert the C-style buffer to a base64-encoded JSON string")
                                emit.Line("%s _%sEncoded;" % (elem.CppStdClass(), elem.JsonName()))
                                emit.Line("Core::ToString(%s, %s, true, _%sEncoded);" % (elem.JsonName(), elem.schema["length"], elem.JsonName()))
                                emit.Line("%s%s = _%sEncoded;" % (parent, elem.CppName(), elem.JsonName()))
                            else:
                                emit.Line("%s%s = string(%s, %s);" % (parent, elem.CppName(), elem.JsonName(), elem.schema["length"]))
                            emit.Unindent()
                            emit.Line("}")
                        # Iterators disguised as arrays
                        elif isinstance(elem, JsonArray):
                            if "iterator" in elem.schema:
                                emit.Line("if (%s != nullptr) {" % elem.JsonName())
                                emit.Indent()
                                emit.Line("// Convert the iterator to a JSON array")
                                emit.Line("%s %s{};" % (elem.items.CppStdClass(), elem.items.JsonName()))
                                emit.Line("while (%s->Next(%s) == true) {" % (elem.JsonName(), elem.items.JsonName()))
                                emit.Indent()
                                emit.Line("%s& element(%s.Add());" % (elem.items.CppType(), parent + elem.CppName()))
                                emit.Line("element = %s;" % elem.items.JsonName())
                                emit.Unindent()
                                emit.Line("}")
                                emit.Line("%s->Release();" % elem.JsonName())
                                emit.Unindent()
                                emit.Line("}")
                            else:
                                raise RuntimeError("unable to serialize a non-iterator array: %s" % elem.JsonName())
                        # Enums
                        elif isinstance(elem, JsonEnum):
                            if elem.Create():
                                emit.Line("%s%s = static_cast<%s>(%s%s);" % (parent, elem.CppName(), elem.CppClass(), cppParent, elem.JsonName()))
                            else:
                                emit.Line("%s%s = %s%s;" % (parent, elem.CppName(), cppParent, elem.JsonName()))
                        # All other primitives and PODs
                        else:
                            emit.Line("%s%s = %s%s;" % (parent, elem.CppName(), cppParent, elem.JsonName() if not cppParent else elem.TrueName()))

                    EmitResponse(response)

                    emit.Unindent()
                    emit.Line("}")

            if isinstance(m, JsonProperty):
                if indexed and isinstance(m.index, JsonInteger):
                    index_var = "_indexInt"
                    emit.Line("%s %s{};" % (m.index.CppStdClass(), index_var))
                    emit.Line("if (Core::FromString(%s, %s) == false) {" % (index_param, index_var))
                    emit.Indent()
                    emit.Line("// failed to convert the index")
                    emit.Line("%s = Core::ERROR_UNKNOWN_KEY;" % errorcode_var)
                    if not m.writeonly and not m.readonly:
                        emit.Line("%s%s.Null(true);" % ("// " if isinstance(response, (JsonArray, JsonObject)) else "", response.CppName())) # FIXME
                    emit.Unindent()
                    emit.Line("} else {")
                    emit.Indent()
                elif indexed and isinstance(m.index, JsonEnum):
                    index_var = "_indexEnum"
                    emit.Line("Core::EnumerateType<%s> _value(%s.c_str());" % (m.index.CppStdClass(), index_param))
                    emit.Line("const %s %s = _value.Value();" % (m.index.CppStdClass(), index_var))
                    emit.Line("if (_value.IsSet() == false) {")
                    emit.Indent()
                    emit.Line("// failed enum look-up")
                    emit.Line("%s = Core::ERROR_UNKNOWN_KEY;" % errorcode_var)
                    if not m.writeonly and not m.readonly:
                        emit.Line("%s%s.Null(true);" % ("// " if isinstance(response, (JsonArray, JsonObject)) else "", response.CppName())) # FIXME
                    emit.Unindent()
                    emit.Line("} else {")
                    emit.Indent()
                if not m.readonly and not m.writeonly:
                    emit.Line("if (%s.IsSet() == false) {" % params.CppName())
                    emit.Indent()
                    emit.Line("// property get")
                elif m.readonly:
                    emit.Line("// read-only property get")
                if not m.writeonly:
                    Invoke(void, response, not m.readonly)
            else:
                Invoke(params, response, False, (params.CppName() + '.') if isinstance(params, JsonObject) else None)

            if isinstance(m, JsonProperty) and not m.readonly:
                if not m.writeonly:
                    emit.Unindent()
                    emit.Line("} else {")
                    emit.Indent()
                    emit.Line("// property set")
                if m.writeonly:
                    emit.Line("// write-only property set")
                Invoke(params, void)
                if not m.writeonly:
                    emit.Line("%s%s.Null(true);" % ("// " if isinstance(response, (JsonArray, JsonObject)) else "", response.CppName())) # FIXME
                    emit.Unindent()
                    emit.Line("}")

            if index_var != index_param:
                emit.Unindent()
                emit.Line("}")

            # Emit method epilogue
            emit.Line("return (%s);" % errorcode_var)
            emit.Unindent()
            emit.Line("});")
            emit.Unindent()
            emit.Line()
        elif isinstance(m, JsonNotification):
            events.append(m)

    emit.Unindent()
    emit.Line("}")
    emit.Line()

    emit.Line("static void Unregister(PluginHost::JSONRPC& module)")
    emit.Line("{")
    emit.Indent()

    for m in root.Properties():
        if isinstance(m, JsonMethod) and not isinstance(m, JsonNotification):
            emit.Line("module.Unregister(_T(\"%s\"));" % (m.JsonName()))

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
    if root.Objects():
        namespace = DATA_NAMESPACE + "::" + root.JsonName()

        def __NsName(obj):
            ns = DATA_NAMESPACE + "::" + \
                (root.JsonName() if not obj.included_from else obj.included_from)
            objName = obj.CppType()
            if objName != "void":
                if not objName.startswith(TYPE_PREFIX):
                    objName = ns + "::" + objName
                p = objName.find("<", 0)
                while p != -1:
                    if not objName.startswith(TYPE_PREFIX, p + 1):
                        objName = objName[:p + 1] + ns + "::" + objName[p + 1:]
                    p = objName.find("<", p + 1)
            return objName

        emit.Line("#include \"Module.h\"")
        emit.Line("#include \"%s.h\"" % root.JsonName())
        emit.Line("#include <%s%s>" % (IF_PATH, header_file))
        for inc in root.includes:
            emit.Line("#include <%s%s_%s.h>" % (IF_PATH, DATA_NAMESPACE, inc))
        emit.Line()

        # Registration prototypes
        has_statuslistener = False
        for method in root.Properties():
            if isinstance(method, JsonNotification) and method.StatusListener():
                has_statuslistener = True
                break

        log.Info("Emitting registration code...")
        emit.Line("/*")
        emit.Indent()
        emit.Line("// Copy the code below to %s class definition" % root.JsonName())
        emit.Line("// Note: The %s class must inherit from PluginHost::JSONRPC%s" %
                  (root.JsonName(), "SupportsEventStatus" if has_statuslistener else ""))
        emit.Line()
        emit.Line("private:")
        emit.Indent()
        emit.Line("void RegisterAll();")
        emit.Line("void UnregisterAll();")
        for method in root.Properties():
            if not isinstance(method, JsonProperty) and not isinstance(method, JsonNotification):
                params = __NsName(method.Properties()[0])
                response = __NsName(method.Properties()[1])
                line = ("uint32_t %s(%s%s%s);" % (method.MethodName(),
                                                  ("const " + params + "& params") if params != "void" else "",
                                                  ", " if params != "void" and response != "void" else "",
                                                  (response + "& response") if response != "void" else ""))
                if method.included_from:
                    line += " // %s" % method.included_from
                emit.Line(line)

        for method in root.Properties():
            if isinstance(method, JsonProperty):
                if not method.writeonly:
                    line = "uint32_t %s(%s%s& response) const;" % (method.GetMethodName(),
                                                                   "const string& index, " if method.index else "",
                                                                   __NsName(method.Properties()[0]))
                    if method.included_from:
                        line += " // %s" % method.included_from
                    emit.Line(line)
                if not method.readonly:
                    line = "uint32_t %s(%sconst %s& param);" % (method.SetMethodName(),
                                                                "const string& index, " if method.index else "",
                                                                __NsName(method.Properties()[0]))
                    if method.included_from:
                        line += " // %s" % method.included_from
                    emit.Line(line)

        for method in root.Properties():
            if isinstance(method, JsonNotification):
                params = __NsName(method.Properties()[0])
                par = ""
                if params != "void":
                    if method.Properties()[0].Properties():
                        par = ", ".join(map(lambda x: "const " + GetNamespace(root, x) + x.CppStdClass() + "& " + x.JsonName(), method.Properties()[0].Properties()))
                    else:
                        x = method.Properties()[0]
                        par = "const " + GetNamespace(root, x) + x.CppStdClass() + "& " + x.JsonName()
                line = ('void %s(%s%s);' %
                        (method.MethodName(), "const string& id, " if method.HasSendif() else "", par))
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
        emit.Line("void %s::RegisterAll()" % root.JsonName())
        emit.Line("{")
        emit.Indent()
        for method in root.Properties():
            if isinstance(method, JsonNotification) and method.StatusListener():
                emit.Line("RegisterEventStatusListener(_T(\"%s\"), [this](const string& client, Status status) {" %
                          method.JsonName())
                emit.Indent()
                emit.Line("const string id = client.substr(0, client.find('.'));")
                emit.Line("// TODO...")
                emit.Unindent()
                emit.Line("});")
                emit.Line()
        for method in root.Properties():
            if not isinstance(method, JsonNotification) and not isinstance(method, JsonProperty):
                line = 'Register<%s,%s>(_T("%s"), &%s::%s, this);' % (
                    method.Properties()[0].CppType(), method.Properties()[1].CppType(), method.JsonName(),
                    root.JsonName(), method.MethodName())
                if method.included_from:
                    line += " /* %s */" % method.included_from
                emit.Line(line)
        for method in root.Properties():
            if isinstance(method, JsonProperty):
                line = 'Property<%s>(_T("%s")' % (method.Properties()[0].CppType(), method.JsonName())
                line += ", &%s::%s" % (root.JsonName(), method.GetMethodName()) if not method.writeonly else ", nullptr"
                line += ", &%s::%s" % (root.JsonName(), method.SetMethodName()) if not method.readonly else ", nullptr"
                line += ', this);'
                if method.included_from:
                    line += " /* %s */" % method.included_from
                emit.Line(line)
        emit.Unindent()
        emit.Line("}")
        emit.Line()
        emit.Line("void %s::UnregisterAll()" % root.JsonName())
        emit.Line("{")
        emit.Indent()
        for method in reversed(root.Properties()):
            if not isinstance(method, JsonNotification) and not isinstance(method, JsonProperty):
                emit.Line('Unregister(_T("%s"));' % method.JsonName())
        for method in reversed(root.Properties()):
            if isinstance(method, JsonProperty):
                emit.Line('Unregister(_T("%s"));' % method.JsonName())
        for method in reversed(root.Properties()):
            if isinstance(method, JsonNotification) and method.StatusListener():
                emit.Line("UnregisterEventStatusListener(_T(\"%s\");" % method.JsonName())

        emit.Unindent()
        emit.Line("}")
        emit.Line()

        # Method/property/event stubs
        log.Info("Emitting stubs...")
        emit.Line("// API implementation")
        emit.Line("//")
        emit.Line()
        for method in root.Properties():
            if not isinstance(method, JsonNotification) and not isinstance(method, JsonProperty):
                log.Info("Emitting method '{}'".format(method.JsonName()))
                params = method.Properties()[0].CppType()
                emit.Line("// Method: %s" % method.Headline())
                emit.Line("// Return codes:")
                emit.Line("//  - ERROR_NONE: Success")
                for e in method.Errors():
                    description = e["description"] if "description" in e else ""
                    if isinstance(e, jsonref.JsonRef) and "description" in e.__reference__:
                        description = e.__reference__["description"]
                    emit.Line("//  - %s: %s" % (e["message"], description))
                response = method.Properties()[1].CppType()
                line = ("uint32_t %s::%s(%s%s%s)" % (root.JsonName(), method.MethodName(),
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
                    for p in method.Properties()[0].Properties():
                        if not isinstance(p, (JsonObject, JsonArray)):
                            emit.Line("const %s& %s = params.%s.Value();" %
                                      (p.CppStdClass(), p.JsonName(), p.CppName()))
                        else:
                            emit.Line("// params.%s ..." % p.CppName())
                emit.Line()
                emit.Line("// TODO...")
                emit.Line()
                if response != "void":
                    for p in method.Properties()[1].Properties():
                        emit.Line("// response.%s = ..." % (p.CppName()))
                    emit.Line()
                emit.Line("return result;")
                emit.Unindent()
                emit.Line("}")
                emit.Line()

        for method in root.Properties():
            if isinstance(method, JsonProperty):

                def EmitPropertyFc(method, name, getter):
                    params = method.Properties()[0].CppType()
                    emit.Line("// Property: %s" % method.Headline())
                    emit.Line("// Return codes:")
                    emit.Line("//  - ERROR_NONE: Success")
                    for e in method.Errors():
                        description = e["description"] if "description" in e else ""
                        if isinstance(e, jsonref.JsonRef) and "description" in e.__reference__:
                            description = e.__reference__["description"]
                        emit.Line("//  - %s: %s" % (e["message"], description))
                    line = "uint32_t %s::%s(%s%s%s& %s)%s" % (
                        root.JsonName(), name, "const string& index, " if method.index else "", "const "
                        if not getter else "", params, "response" if getter else "param", " const" if getter else "")
                    if method.included_from:
                        line += " /* %s */" % method.included_from
                    emit.Line(line)
                    emit.Line("{")
                    emit.Indent()
                    if not getter:
                        emit.Line("uint32_t result = Core::ERROR_NONE;")
                        emit.Line()
                        emit.Line("// TODO...")
                    else:
                        emit.Line("// response = ...")
                    emit.Line()
                    emit.Line("return %s;" % ("Core::ERROR_NONE" if getter else "result"))
                    emit.Unindent()
                    emit.Line("}")
                    emit.Line()

                propType = ' (write-only)' if method.writeonly else (' (read-only)' if method.readonly else '')
                log.Info("Emitting property '{}' {}".format(method.JsonName(), propType))
                if not method.writeonly:
                    EmitPropertyFc(method, method.GetMethodName(), True)
                if not method.readonly:
                    EmitPropertyFc(method, method.SetMethodName(), False)

        for method in root.Properties():
            if isinstance(method, JsonNotification):
                log.Info("Emitting notification '{}'".format(method.JsonName()))
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

def EmitObjects(root, emit, if_file, emitCommon=False):
    global emittedItems
    emittedItems = 0

    def EmitEnumConversionHandler(root, enum):
        fullname = GetNamespace(root, enum) + enum.CppClass()
        emit.Line("ENUM_CONVERSION_HANDLER(%s);" % fullname)

    def EmitEnum(enum):
        global emittedItems
        emittedItems += 1
        log.Info("Emitting enum {}".format(enum.CppClass()))
        root = enum.parent.parent
        while root.parent:
            root = root.parent
        if enum.Description():
            emit.Line("// " + enum.Description())
        emit.Line("enum%s %s {" % (" class" if enum.IsStronglyTyped() else "", enum.CppClass()))
        emit.Indent()
        for c, item in enumerate(enum.CppEnumerators()):
            emit.Line("%s%s%s" % (item.upper(),
                                  (" = " + str(enum.CppEnumeratorValues()[c])) if enum.CppEnumeratorValues() else "",
                                  "," if not c == len(enum.CppEnumerators()) - 1 else ""))
        emit.Unindent()
        emit.Line("};")
        emit.Line()

    def EmitClass(jsonObj, allowDup=False):
        def EmitInit(jsonObject):
            for prop in jsonObj.Properties():
                emit.Line("Add(_T(\"%s\"), &%s);" % (prop.JsonName(), prop.CppName()))

        def EmitCtor(jsonObj, noInitCode=False, copyCtor=False, convCtor = False):
            if copyCtor:
                emit.Line("%s(const %s& other)" % (jsonObj.CppClass(), jsonObj.CppClass()))
            elif convCtor:
                emit.Line("%s(const %s& other)" % (jsonObj.CppClass(), jsonObj.CppStdClass()))
            else:
                emit.Line("%s()" % (jsonObj.CppClass()))
            emit.Indent()
            emit.Line(": %s()" % TypePrefix("Container"))
            for prop in jsonObj.Properties():
                if copyCtor:
                    emit.Line(", %s(other.%s)" % (prop.CppName(), prop.CppName()))
                elif prop.CppDefValue() != '""' and prop.CppDefValue() != "":
                    emit.Line(", %s(%s)" % (prop.CppName(), prop.CppDefValue()))
            emit.Unindent()
            emit.Line("{")
            emit.Indent()
            if convCtor:
                for prop in jsonObj.Properties():
                    emit.Line("%s = other.%s;" % (prop.CppName(), prop.TrueName()))
            if not noInitCode:
                EmitInit(jsonObj)
            else:
                emit.Line("Init();")

            emit.Unindent()
            emit.Line("}")

        def EmitAssignmentOperator(jsonObj, copyCtor = False, convCtor = False):
            if copyCtor:
                emit.Line("%s& operator=(const %s& rhs)" % (jsonObj.CppClass(), jsonObj.CppClass()))
            elif convCtor:
                emit.Line("%s& operator=(const %s& rhs)" % (jsonObj.CppClass(), jsonObj.CppStdClass()))
            emit.Line("{")
            emit.Indent()
            for prop in jsonObj.Properties():
                if copyCtor:
                    emit.Line("%s = rhs.%s;" % (prop.CppName(), prop.CppName()))
                elif convCtor:
                    emit.Line("%s = rhs.%s;" % (prop.CppName(), prop.TrueName()))
            emit.Line("return (*this);")
            emit.Unindent()
            emit.Line("}")

        def EmitConversionOperator(jsonObj):
            emit.Line("operator %s() const" % (jsonObj.CppStdClass()))
            emit.Line("{")
            emit.Indent();
            emit.Line("%s val;" % (jsonObj.CppStdClass()))
            for prop in jsonObj.Properties():
                emit.Line("val.%s = %s%s;" % ( prop.TrueName(), prop.CppName(), ".Value()" if not isinstance(prop, JsonObject) else ""))
            emit.Line("return (val);")
            emit.Unindent()
            emit.Line("}")

        # Bail out if a duplicated class!
        if isinstance(jsonObj, JsonObject) and not jsonObj.properties:
            return
        if  jsonObj.IsDuplicate() or (not allowDup and jsonObj.RefCount() > 1):
            return
        if not isinstance(jsonObj, (JsonRpcSchema, JsonMethod)):
            log.Info("Emitting class '{}' (source: '{}')".format(jsonObj.CppClass(), jsonObj.OrigName()))
            emit.Line("class %s : public %s {" % (jsonObj.CppClass(), TypePrefix("Container")))
            emit.Line("public:")
            if jsonObj.Enums():
                for enum in jsonObj.Enums():
                    if enum.Create() and not enum.IsDuplicate() and enum.RefCount() == 1:
                        emit.Indent()
                        EmitEnum(enum)
                        emit.Unindent()
            emit.Indent()
        else:
            if isinstance(jsonObj, JsonMethod):
                if jsonObj.included_from:
                    return

        # Handle nested classes!
        for obj in SortByDependency(jsonObj.Objects()):
            EmitClass(obj)

        if not isinstance(jsonObj, (JsonRpcSchema, JsonMethod)):
            global emittedItems
            emittedItems += 1
            EmitCtor(jsonObj, jsonObj.NeedsCopyCtor())
            if jsonObj.NeedsCopyCtor():
                emit.Line()
                EmitCtor(jsonObj, True, True, False)
                emit.Line()
                EmitAssignmentOperator(jsonObj, True, False)
            if "typename" in jsonObj.schema:
                emit.Line()
                EmitCtor(jsonObj, True, False, True)
                emit.Line()
                EmitAssignmentOperator(jsonObj, False, True)
                emit.Line()
                EmitConversionOperator(jsonObj)
            if jsonObj.NeedsCopyCtor() or "typename" in jsonObj.schema:
                emit.Unindent()
                emit.Line()
                emit.Line("private:")
                emit.Indent()
                emit.Line("void Init()")
                emit.Line("{")
                emit.Indent()
                EmitInit(jsonObj)
                emit.Unindent()
                emit.Line("}")
                emit.Line()
            else:
                emit.Line()
                emit.Line("%s(const %s&) = delete;" % (jsonObj.CppClass(), jsonObj.CppClass()))
                emit.Line("%s& operator=(const %s&) = delete;" % (jsonObj.CppClass(), jsonObj.CppClass()))
                emit.Line()

            emit.Unindent()
            emit.Line("public:")
            emit.Indent()
            for prop in jsonObj.Properties():
                comment = prop.OrigName() if isinstance(prop, JsonMethod) else prop.Description()
                emit.Line("%s %s;%s" % (prop.CppType(), prop.CppName(), (" // " + comment) if comment else ""))
            emit.Unindent()
            emit.Line("}; // class %s" % jsonObj.CppClass())
            emit.Line()

    count = 0
    if enumTracker.Objects():
        count = 0
        for obj in enumTracker.Objects():
            if obj.Create() and not obj.IsDuplicate() and not obj.included_from:
                count += 1

    emit.Line("#pragma once")
    emit.Line()
    emit.Line("#include <core/JSON.h>")
    if if_file.endswith(".h"):
        emit.Line("#include <%s%s>" % (CPP_IF_PATH, if_file))
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
    emit.Line("namespace %s {" % root.JsonName())
    emit.Indent()
    emit.Line()
    if emitCommon and enumTracker.CommonObjects():
        emittedPrologue = False
        for obj in enumTracker.CommonObjects():
            if obj.Create() and not obj.IsDuplicate() and not obj.included_from:
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
    if root.Objects():
        log.Info("Emitting params/result classes...")
        emit.Line("// Method params/result classes")
        emit.Line("//")
        emit.Line()
        EmitClass(root)
    emit.Unindent()
    emit.Line("} // namespace %s" % root.JsonName())
    emit.Line()
    if "info" in root.schema and "namespace" in root.schema["info"]:
        emit.Unindent()
        emit.Line("} // namespace %s" % root.schema["info"]["namespace"])
        emit.Line()
    emit.Unindent()
    emit.Line("} // namespace %s" % DATA_NAMESPACE)
    emit.Line()
    emittedPrologue = False
    for obj in enumTracker.Objects():
        if not obj.IsDuplicate() and not obj.included_from:
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

            def __TableObj(name, obj, parentName="", parent=None, prefix="", parentOptional=False):
                # determine if the attribute is optional
                optional = parentOptional or (obj["optional"] if "optional" in obj else False)
                if parent and not optional:
                    if parent["type"] == "object":
                        optional = ("required" not in parent and len(parent["properties"]) > 1) or (
                            "required" in parent
                            and name not in parent["required"]) or ("required" in parent
                                                                    and len(parent["required"]) == 0)
                name = (name if not "original" in obj else obj["original"])
                # include information about enum values in description
                enum = ' (must be one of the following: %s)' % (", ".join(
                    '*{0}*'.format(w) for w in obj["enum"])) if "enum" in obj else ""
                if parent and prefix and parent["type"] == "object":
                    prefix += "?." if optional else "."
                prefix += name
                description = obj["description"] if "description" in obj else obj["summary"] if "summary" in obj else ""
                if isinstance(obj, jsonref.JsonRef) and "description" in obj.__reference__:
                    description = obj.__reference__["description"]
                if name or prefix:
                    if "type" not in obj:
                        raise RuntimeError("missing 'type' for object %s" % (parentName + "/" + name))
                    row = (("<sup>" + italics("(optional)") + "</sup>" + " ") if optional else "") + description + enum
                    if row.endswith('.'):
                        row = row[:-1]
                    MdRow([prefix, obj["type"], row])
                if obj["type"] == "object":
                    if "required" not in obj and name and len(obj["properties"]) > 1:
                        log.Warn("'%s': no 'required' field present (assuming all members optional)" % name)
                    for pname, props in obj["properties"].items():
                        __TableObj(pname, props, parentName + "/" + (name if not "original" in props else props["original"]), obj, prefix, False)
                elif obj["type"] == "array":
                    __TableObj("", obj["items"], parentName + "/" + name, obj, (prefix + "[#]") if name else "",
                               optional)

            __TableObj(name, object, "")
            MdBr()

        def ErrorTable(obj):
            MdTableHeader(["Code", "Message", "Description"])
            for err in obj:
                description = err["description"] if "description" in err else ""
                if isinstance(err, jsonref.JsonRef) and "description" in err.__reference__:
                    description = err.__reference__["description"]
                MdRow([err["code"] if "code" in err else "", "```" + err["message"] + "```", description])
            MdBr()

        def PlainTable(obj, columns, ref="ref"):
            MdTableHeader(columns)
            for prop, val in sorted(obj.items()):
                MdRow([
                    "<a name=\"%s.%s\">%s</a>" % (ref, (prop.split("]", 1)[0][1:]) if "]" in prop else prop, prop), val
                ])
            MdBr()

        def __ExampleObj(name, obj, root=False):
            name = (name if not "original" in obj else obj["original"] if not root else name)
            objType = obj["type"]
            default = obj["example"] if "example" in obj else obj["default"] if "default" in obj else ""
            if not default and "enum" in obj:
                default = obj["enum"][0]
            jsonData = '"%s": ' % name if name else ''
            if objType == "string":
                jsonData += '"%s"' % (default if default else "...")
            elif objType in ["integer", "number"]:
                jsonData += '%s' % (default if default else 0)
            elif objType in ["float", "double"]:
                jsonData += '%s' % (default if default else 0.0)
            elif objType == "boolean":
                jsonData += '%s' % str(default if default else False).lower()
            elif objType == "null":
                jsonData += 'null'
            elif objType == "array":
                jsonData += str(default if default else ('[ %s ]' % (__ExampleObj("", obj["items"]))))
            elif objType == "object":
                jsonData += "{ %s }" % ", ".join(
                    list(map(lambda p: __ExampleObj(p, obj["properties"][p]),
                             obj["properties"]))[0:obj["maxProperties"] if "maxProperties" in obj else None])
            return jsonData

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
                if not "description" in props["params"]:
                    if "summary" in props:
                        props["params"]["description"] = props["summary"]
                ParamTable("(property)", props["params"])
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
                method = "%s.1.%s%s" % (classname, method, ("@" + props["index"]["example"])
                                        if "index" in props and "example" in props["index"] else "")
            else:
                method = "%s.1.%s" % (classname, method)
            if "id" in props and "example" in props["id"]:
                method = props["id"]["example"] + "." + method
            parameters = props["params"] if "params" in props else None

            if is_property:
                if not writeonly:
                    MdHeader("Get Request", 4)
                    jsonRequest = json.dumps(json.loads('{ "jsonrpc": "2.0", "id": 42, "method": "%s" }' %
                                                        method,
                                                        object_pairs_hook=OrderedDict),
                                             indent=4)
                    MdCode(jsonRequest, "json")
                    MdHeader("Get Response", 4)
                    jsonResponse = json.dumps(json.loads('{ "jsonrpc": "2.0", "id": 42, %s }' %
                                                         __ExampleObj("result", parameters, True),
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
                                                     (", " + __ExampleObj("params", parameters, True)) if parameters else ""),
                                                    object_pairs_hook=OrderedDict),
                                         indent=4)
                MdCode(jsonRequest, "json")

                if not is_notification and not is_property:
                    if "result" in props:
                        MdHeader("Response", 4)
                        jsonResponse = json.dumps(json.loads('{ "jsonrpc": "2.0", "id": 42, %s }' %
                                                             __ExampleObj("result", props["result"], True),
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

        MdBody("<!-- Generated automatically, DO NOT EDIT! -->")

        commons = dict()
        with open(os.path.join(os.path.dirname(os.path.realpath(__file__)), GLOBAL_DEFINITIONS)) as f:
            commons = json.load(f)

        # The interfaces defined can be a single item, a list, or a list of list.
        # So first flatten the structure and make it consistent to be always a list.
        tmpinterfaces = schema
        if "interface" in schema:
            tmpinterfaces = schema["interface"]
        if not isinstance(tmpinterfaces, list):
            tmpinterfaces = [tmpinterfaces]
        if "include" in schema:
            for face in schema["include"]:
                tmpinterfaces.append(schema["include"][face])

        interfaces = []
        for interface in tmpinterfaces:
            if isinstance(interface, list):
                interfaces.extend(interface)
                for face in interface:
                    if "include" in face:
                        if isinstance(face["include"], list):
                            interfaces.extend(face["include"])
                        else:
                            interfaces.append(face["include"])
            else:
                interfaces.append(interface)
                if "include" in interface:
                    for face in interface["include"]:
                        interfaces.append(interface["include"][face])

        if "info" in schema:
            info = schema["info"]
        elif interfaces and "info" in interfaces[0]:
            # as a fallback try the first interface
            info = interfaces[0]["info"]

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

        version = info["version"] if "version" in info else "1.0"

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

        def SourceLocation(face):
            sourcelocation = None
            if "sourcelocation" in face["info"]:
                sourcelocation = face["info"]["sourcelocation"]
            elif "sourcelocation" in info:
                sourcelocation = info["sourcelocation"]
            elif "common" in face and "sourcelocation" in face["common"]:
                sourcelocation = face["common"]["sourcelocation"]
            elif "sourcelocation" in commons:
                sourcelocation = commons["sourcelocation"]
            else:
                sourcelocation = None

            if sourcelocation:
                sourcelocation = sourcelocation.replace("{interfacefile}", face["info"]["sourcefile"] if "sourcefile" in face["info"] else ((face["info"]["class"] if "class" in face["info"] else input_basename) + ".json"))
            return sourcelocation

        document_type = "plugin"
        if ("interfaceonly" in schema and schema["interfaceonly"]) or ("$schema" in schema and schema["$schema"] == "interface.schema.json"):
            document_type = "interface"

        # Emit title bar
        if "title" in info:
            MdHeader(info["title"])
        MdParagraph(bold("Version: " + version))
        MdParagraph(bold("Status: " + rating * ":black_circle:" + (3 - rating) * ":white_circle:"))
        MdParagraph("A %s %s for Thunder framework." % (plugin_class, document_type))

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

        # Emit TOC.
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
            MdParagraph("This document describes purpose and functionality of the %s %s.%s" % (plugin_class, document_type, extra))

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
                    MdBody("- " + wl)
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
                            descr = ""
                            if "summary" in contents:
                                descr = contents["summary"]
                                if "e.g" in descr:
                                    descr = descr[0:descr.index("e.g") - 1]
                                if "i.e" in descr:
                                    descr = descr[0:descr.index("i.e") - 1]
                                descr = descr.split(".", 1)[0] if "." in descr else descr
                            MdRow([link(header + "." + (method.rsplit(".", 1)[1] if "." in method else method)) + access, descr])
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

def CreateCode(schema, path, generateClasses, generateStubs, generateRpc):
    directory = os.path.dirname(path)
    filename = (schema["info"]["namespace"]) if "info" in schema and "namespace" in schema["info"] else ""
    filename += (schema["info"]["class"]) if "info" in schema and "class" in schema["info"] else ""
    if len(filename) == 0:
        filename = os.path.basename(path.replace("Plugin", "").replace(".json", "").replace(".h", ""))
    rpcObj = ParseJsonRpcSchema(schema)
    if rpcObj:
        header_file = os.path.join(directory, DATA_NAMESPACE + "_" + filename + ".h")
        enum_file = os.path.join(directory, "JsonEnum_" + filename + ".cpp")

        if generateClasses:
            data_emitted = 0
            with open(header_file, "w") as output_file:
                emitter = Emitter(output_file, INDENT_SIZE)
                emitter.Line()
                emitter.Line("// C++ classes for %s JSON-RPC API." % rpcObj.info["title"].replace("Plugin", "").strip())
                emitter.Line("// Generated automatically from '%s'. DO NOT EDIT." % os.path.basename(path))
                emitter.Line()
                emitter.Line(
                    "// Note: This code is inherently not thread safe. If required, proper synchronisation must be added."
                )
                emitter.Line()
                data_emitted = EmitObjects(rpcObj, emitter, os.path.basename(path), True)
                if data_emitted:
                    log.Success("JSON data classes generated in '%s'." % os.path.basename(output_file.name))
                else:
                    log.Info("No JSON data classes generated for '%s'." % os.path.basename(filename))
            if not data_emitted and not KEEP_EMPTY:
                try:
                    os.remove(header_file)
                except:
                    pass

            enum_emitted = 0
            with open(enum_file, "w") as output_file:
                emitter = Emitter(output_file, INDENT_SIZE)
                emitter.Line()
                emitter.Line("// Enumeration code for %s JSON-RPC API." %
                             rpcObj.info["title"].replace("Plugin", "").strip())
                emitter.Line("// Generated automatically from '%s'." % os.path.basename(path))
                emitter.Line()
                enum_emitted = EmitEnumRegs(rpcObj, emitter, filename, os.path.basename(path))
                if enum_emitted:
                    log.Success("JSON enumeration code generated in '%s'." % os.path.basename(output_file.name))
                else:
                    log.Info("No JSON enumeration code generated for '%s'." % os.path.basename(filename))
            if not enum_emitted and not KEEP_EMPTY:
                try:
                    os.remove(enum_file)
                except:
                    pass

        if generateStubs:
            with open(os.path.join(directory, filename + "JsonRpc.cpp"), "w") as output_file:
                emitter = Emitter(output_file, INDENT_SIZE)
                emitter.Line()
                EmitHelperCode(rpcObj, emitter, os.path.basename(header_file))
                log.Success("JSON-RPC stubs generated in '%s'." % os.path.basename(output_file.name))

        if generateRpc and "dorpc" in rpcObj.schema and rpcObj.schema["dorpc"] == True:
            with open(os.path.join(directory, "J" + filename + ".h"), "w") as output_file:
                emitter = Emitter(output_file, INDENT_SIZE)
                emitter.Line()
                EmitRpcCode(rpcObj, emitter, filename, os.path.basename(path), data_emitted)
                log.Success("JSON-RPC implementation generated in '%s'." % os.path.basename(output_file.name))

    else:
        log.Info("No code to generate.")


objTracker = ObjectTracker()
enumTracker = EnumTracker()

if __name__ == "__main__":
    argparser = argparse.ArgumentParser(
        description='Generate JSON C++ classes, stub code and API documentation from JSON definition files.',
        formatter_class=argparse.RawTextHelpFormatter)
    argparser.add_argument('path', nargs="*", help="JSON file(s), wildcards are allowed")
    argparser.add_argument("--version", dest="version", action="store_true", default=False, help="display version")
    argparser.add_argument("--verbose", dest="verbose", action="store_true", default=VERBOSE, help="enable verbose logging (default: no verbouse logging)")
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
                           help="generate JSON classes and JSON-RPC code if applicable")
    argparser.add_argument("-s",
                           "--stubs",
                           dest="stubs",
                           action="store_true",
                           default=False,
                           help="generate JSON-RPC stub code")
    argparser.add_argument(
        "-p",
        dest="if_path",
        metavar="PATH",
        action="store",
        type=str,
        default=IF_PATH,
        help="relative path for #include'ing JsonData header file (default: 'interfaces/json', '.' for no path)")
    argparser.add_argument(
        "-i",
        dest="if_dir",
        metavar="DIR",
        action="store",
        type=str,
        default=None,
        help=
        "a directory with JSON API interfaces that will substitute the {interfacedir} tag (default: same directory as source file)"
    )
    argparser.add_argument(
        "-j",
        dest="cppif_dir",
        metavar="DIR",
        action="store",
        type=str,
        default=None,
        help=
        "a directory with C++ API interfaces that will substitute the {cppinterfacedir} tag (default: same directory as source file)"
    )
    argparser.add_argument(
        "-o",
        "--output",
        dest="output_dir",
        metavar="DIR",
        action="store",
        default=None,
        help=
        "output directory, absolute path or directory relative to output file(default: output in the same directory as the source json)"
    )
    argparser.add_argument("--indent",
                           dest="indent_size",
                           metavar="SIZE",
                           type=int,
                           action="store",
                           default=INDENT_SIZE,
                           help="code indentation in spaces (default: %i)" % INDENT_SIZE)
    argparser.add_argument("--dump-json",
                           dest="dump_json",
                           action="store_true",
                           default=False,
                           help="dump intermediate JSON file when parsing C++ header")
    argparser.add_argument(
        "--copy-ctor",
        dest="copy_ctor",
        action="store_true",
        default=False,
        help="always emit a copy constructor and assignment operator for a class (default: emit only when needed)")
    argparser.add_argument("--keep-empty",
                           dest="keep_empty",
                           action="store_true",
                           default=False,
                           help="keep generated files that have no content (default: remove empty cpp/h files)")
    argparser.add_argument("--no-ref-names",
                           dest="no_ref_names",
                           action="store_true",
                           default=False,
                           help="do not derive class names from $refs (default: derive class names from $ref)")
    argparser.add_argument("--def-string",
                           dest="def_string",
                           metavar="STRING",
                           type=str,
                           action="store",
                           default=DEFAULT_EMPTY_STRING,
                           help="default string initialisation (default: \"%s\")" % DEFAULT_EMPTY_STRING)
    argparser.add_argument("--def-int-size",
                           dest="def_int_size",
                           metavar="SIZE",
                           type=int,
                           action="store",
                           default=DEFAULT_INT_SIZE,
                           help="default integer size in bits (default: %i)" % DEFAULT_INT_SIZE)
    argparser.add_argument("--no-style-warnings",
                           dest="no_style_warnings",
                           action="store_true",
                           default=not DOC_ISSUES,
                           help="suppress documentation issues (default: show all documentation issues)")
    argparser.add_argument("--no-duplicates-warnings",
                           dest="no_duplicates_warnings",
                           action="store_true",
                           default=not SHOW_WARNINGS,
                           help="suppress duplicate object warnings (default: show all duplicate object warnings)")
    argparser.add_argument("--no-interfaces-section",
                           dest="no_interfaces_section",
                           action="store_true",
                           default=False,
                           help="do not include Interfaces section in the documentation (default: include interface section)")
    argparser.add_argument("--include",
                           dest="extra_include",
                           metavar="FILE",
                           action="store",
                           default=DEFAULT_DEFINITIONS_FILE,
                           help="for C++ source: include a C++ header file (default: include '%s')" %
                           DEFAULT_DEFINITIONS_FILE)
    argparser.add_argument("--namespace",
                           dest="if_namespace",
                           metavar="NS",
                           type=str,
                           action="store",
                           default=INTERFACE_NAMESPACE,
                           help="for C++ source: set namespace to look for interfaces in (default: %s)" %
                           INTERFACE_NAMESPACE)
    argparser.add_argument('-I', dest="includePaths", metavar="INCLUDE_DIR", action='append', default=[], type=str,
                           help='for C++ source: add an include path (can be used multiple times)')

    args = argparser.parse_args(sys.argv[1:])

    VERBOSE = args.verbose
    DOC_ISSUES = not args.no_style_warnings
    log.doc_issues = DOC_ISSUES
    NO_DUP_WARNINGS = args.no_duplicates_warnings
    INDENT_SIZE = args.indent_size
    ALWAYS_COPYCTOR = args.copy_ctor
    KEEP_EMPTY = args.keep_empty
    CLASSNAME_FROM_REF = not args.no_ref_names
    DEFAULT_EMPTY_STRING = args.def_string
    DEFAULT_INT_SIZE = args.def_int_size
    DUMP_JSON = args.dump_json
    DEFAULT_DEFINITIONS_FILE = args.extra_include
    INTERFACE_NAMESPACE = "::" + args.if_namespace if args.if_namespace.find("::") != 0 else args.if_namespace
    INTERFACES_SECTION = not args.no_interfaces_section
    if args.if_path and args.if_path != ".":
        IF_PATH = args.if_path
    IF_PATH = posixpath.normpath(IF_PATH) + os.sep

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
            log.Header(path)
            try:
                log.Header(path)
                if path.endswith(".h"):
                    schemas = LoadInterface(path, args.includePaths)
                else:
                    schemas = [LoadSchema(path, args.if_dir, args.cppif_dir, args.includePaths)]
                for schema in schemas:
                    if schema:
                        warnings = GENERATED_JSON
                        GENERATED_JSON = "dorpc" in schema
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
                            CreateCode(schema, output_path, generateCode, generateStubs, generateRpc)
                        if generateDocs:
                            title = schema["info"]["title"] if "title" in schema["info"] \
                                    else schema["info"]["class"] if "class" in schema["info"] \
                                    else os.path.basename(output_path)
                            CreateDocument(schema, os.path.join(os.path.dirname(output_path), title.replace(" ", "")))
                        GENERATED_JSON = warnings
            except JsonParseError as err:
                log.Error(str(err))
            except RuntimeError as err:
                log.Error(str(err))
            except IOError as err:
                log.Error(str(err))
            except ValueError as err:
                log.Error(str(err))
        log.Info("JsonGenerator: All done, {} files parsed, {} error{}.".format(len(files), len(log.errors) if log.errors else 'no',
                                                              '' if len(log.errors) == 1 else 's'))
        if log.errors:
            sys.exit(1)
