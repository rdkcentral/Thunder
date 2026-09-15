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
# distributed under the Lifcense is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import sys
import os
import json
import copy
import posixpath
import re
from collections import OrderedDict
from enum import Enum

import config

sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), os.pardir + os.sep + os.pardir))

import ProxyStubGenerator.CppParser as CppParser
import ProxyStubGenerator.Interface as CppInterface


class CaseConverter:
    OBJECTS = 0
    METHODS = 1
    EVENTS = 2
    PARAMS = 3
    MEMBERS = 4
    ENUMS = 5
    _MAX = 6

    class Format(Enum):
        LOWER = "lower"             # lowercase
        UPPER = "upper"             # UPPERCASE
        LOWERSNAKE = "lowersnake"   # lower_snake_case
        UPPERSNAKE = "uppersnake"   # UPPER_SNAKE_CASE
        CAMEL = "camel"             # camelCase
        PASCAL = "pascal"           # PascalCase
        KEEP = "keep"               # ... keep as is
                # keep has side effect that property parameter does not
                # change the name to "value" but keeps original name

        @staticmethod
        def __to_pascal(string, uppercase=True):
            _pattern = re.compile(r"(?<=[a-z])(?=[A-Z])|(?<=[A-Z])(?=[A-Z][a-z])")
            _split = _pattern.sub('_', string.replace('__', '_').strip('_')).split('_')

            if not string.isupper() and '_' not in string:
                _pascal = [x[0].upper() + x[1:] for x in _split]
            else:
                _pascal = [x.capitalize() for x in _split]

            if uppercase:
                if _split[0].isupper() and not string.isupper() and '_' not in string:
                    _pascal[0] = _pascal[0].upper()
            else:
                _pascal[0] = _pascal[0].lower()

            _pascal = "".join(_pascal)

            return _pascal

        @staticmethod
        def __to_snake(string, uppercase=True):
            _pattern = re.compile(r"(?<=[a-z])(?=[A-Z])|(?<=[A-Z])(?=[A-Z][a-z])")
            _snake = _pattern.sub('_', string.replace('__', '_').strip('_'))
            return (_snake.upper() if uppercase else _snake.lower())

        @staticmethod
        def transform(string, format):
            if format == CaseConverter.Format.LOWER:
                return string.lower()
            elif format == CaseConverter.Format.UPPER:
                return string.upper()
            elif format == CaseConverter.Format.CAMEL:
                return CaseConverter.Format.__to_pascal(string, False)
            elif format == CaseConverter.Format.PASCAL:
                return CaseConverter.Format.__to_pascal(string, True)
            elif format == CaseConverter.Format.LOWERSNAKE:
                return CaseConverter.Format.__to_snake(string, False)
            elif format == CaseConverter.Format.UPPERSNAKE:
                return CaseConverter.Format.__to_snake(string, True)
            elif format == CaseConverter.Format.KEEP:
                return string
            else:
                assert False, "invalid case format"

    def __init__(self, convention=None):
        self._convention = None # error situation

        self._map = {
            # [ objects, methods, events, parameters, struct members, enums ]
            config.CaseConvention.STANDARD: [self.Format.CAMEL, self.Format.CAMEL, self.Format.CAMEL, self.Format.CAMEL, self.Format.CAMEL, self.Format.UPPERSNAKE],
            config.CaseConvention.LEGACY: [self.Format.LOWER, self.Format.LOWER, self.Format.LOWER, self.Format.LOWER, self.Format.LOWER, self.Format.PASCAL],
            config.CaseConvention.KEEP: [self.Format.KEEP, self.Format.KEEP, self.Format.KEEP, self.Format.KEEP, self.Format.KEEP, self.Format.KEEP],
        }

        self._map[config.CaseConvention.CUSTOM] = self._map[config.DEFAULT_CASE_CONVENTION]

        if convention and not config.IGNORE_SOURCE_CASE_CONVENTION:
            if isinstance(convention, str):
                if convention == "standard":
                    self._convention = config.CaseConvention.STANDARD
                elif (convention == "legacy_lowercase") or (convention == "legacy"):
                    self._convention = config.CaseConvention.LEGACY
                elif (convention == "keep"):
                    self._convention = config.CaseConvention.KEEP
                elif convention.startswith("custom="):
                    _custom = convention[7:].split(',')
                    if len(_custom) == self._MAX:
                        self._convention = config.CaseConvention.CUSTOM
                        try:
                            if _custom[self.OBJECTS]:
                                self._map[config.CaseConvention.CUSTOM][self.OBJECTS] = self.Format[_custom[self.OBJECTS].upper()]
                            if _custom[self.METHODS]:
                                self._map[config.CaseConvention.CUSTOM][self.METHODS] = self.Format[_custom[self.METHODS].upper()]
                            if _custom[self.EVENTS]:
                                self._map[config.CaseConvention.CUSTOM][self.EVENTS] = self.Format[_custom[self.EVENTS].upper()]
                            if _custom[self.PARAMS]:
                                self._map[config.CaseConvention.CUSTOM][self.PARAMS] = self.Format[_custom[self.PARAMS].upper()]
                            if _custom[self.MEMBERS]:
                                self._map[config.CaseConvention.CUSTOM][self.MEMBERS] = self.Format[_custom[self.MEMBERS].upper()]
                            if _custom[self.ENUMS]:
                                self._map[config.CaseConvention.CUSTOM][self.ENUMS] = self.Format[_custom[self.ENUMS].upper()]
                        except KeyError as e:
                            raise CppParseError(None, "invalid @text:custom case parameter: %s (expect one of: upper, lower, uppersnake, lowersnake, pascal, camel, keep) " % e)
                    else:
                        raise CppParseError(None, "expected %s case parameters for @text:custom (objects,methods,events,params,members,enums)" % self._MAX)
                else:
                    raise CppParseError(None, "unknown case convention '%s' (expected one of: standard, legacy, keep, custom)" % convention)
            else:
                self._convention = convention
        else:
            self._convention = config.DEFAULT_CASE_CONVENTION

    @property
    def convention(self):
        return self._convention

    @property
    def is_keep(self):
        return (self._convention == config.CaseConvention.KEEP)

    @property
    def is_legacy(self):
        return (self._convention == config.CaseConvention.LEGACY)

    def transform(self, input, attr):
        assert input
        return self.Format.transform(input, self.__format(attr))

    def __format(self, attr):
        assert self.convention
        return self._map[self.convention][attr]


class CppParseError(RuntimeError):
    def __init__(self, obj, msg):
        if obj:
            try:
                msg = "%s(%s): %s (see '%s')" % (obj.parser_file, obj.parser_line, msg, obj.Signature())
                super(CppParseError, self).__init__(msg)
            except:
                super(CppParseError, self).__init__("generic parsing failure: %s(%i): %s" % (obj.parser_file, obj.parser_line, msg))
        else:
            super(CppParseError, self).__init__(msg)

def StripFrameworkNamespace(identifier):
    return str(identifier).replace("::" + config.FRAMEWORK_NAMESPACE + "::", "")

def compute_name(log, converter, object, argument_type, relay=None, is_property=False):
    if isinstance(object, str):
        transformed = converter.transform(object, argument_type)
    else:
        obj = relay if relay else object

        # Keep convention has a side effect of keeping the original parameter name of property
        # instead of having it always as "value".
        original = ("value" if (is_property and not converter.is_keep) else obj.name)
        name = converter.transform(original, argument_type)
        overriden = object.meta.text
        transformed = overriden if overriden else name

        if overriden == name:
            log.WarnLine(object, "'%s': overriden name is same as default ('%s')" % (overriden, name))

    return transformed

def BuildEnum(log, _case_converter, cppType, meta, var=None, extra_decorators=[], quiet=False):
    props = { "enum": [compute_name(log, _case_converter, e, _case_converter.ENUMS) for e in cppType.items] }

    if cppType.scoped:
        props["scoped"] = True

    props["ids"] = [e.name for e in cppType.items]

    if not cppType.items:
        raise CppParseError(var, "%s: no enumerators in enum" % cppType.name)

    if not cppType.items[0].auto_value:
        try:
            props["values"] = [int(e.value) for e in cppType.items]
        except:
            log.InfoLine(var, "'%s': unparsable enum values" % var.name)

    for e in cppType.items:
        if "endmarker" in e.meta.decorators:
            props["@endmarker"] = e.name
            break

    if var:
        if isinstance(var.type.Type(), CppParser.Typedef):
            type = var.type.Type().type.type
            if not isinstance(type.parent, CppParser.Class) or type.parent.is_json:
                props["@register"] = False
    else:
        props["@originaltype"] = StripFrameworkNamespace(cppType.full_name)

    if "bitmask" in meta.decorators or "bitmask" in extra_decorators:
        if var:
            props["type"] = "string"
            props["@bitmask"] = True
            props["@originaltype"] = StripFrameworkNamespace(cppType.full_name)
            result = ["array", { "items": props } ]
        else:
            raise CppParseError(var, "%s: @bitmask is not valid in non-json enum conversions" % cppType.name)
    else:
        result = ["string", props]

    p = cppType.parent
    while p:
        if isinstance(p, CppParser.Class):
            if p.is_json:
                while p:
                    if p.omit_mode:
                        log.Info("Enum will be omitted due to json-import", cppType)
                        props["omit"] = True
                        break
                    p = p.parent
                break

        p = p.parent

    return result

def LoadEnumDefinitionsInternal(file, tree, ns, log, scanned, all = False, include_paths = []):
    def FindEnums(tree, namespace):
        enums = []

        def __Traverse(tree, interface_namespace, enums):

            def Find(tree):
                for e in tree:
                    if (e.full_name.startswith(interface_namespace + "::")):
                        if "encode:text" in e.meta.decorators:
                            enums.append(e)

            if isinstance(tree, CppParser.Namespace) or isinstance(tree, CppParser.Class):

                for c in tree.classes:
                    if not c.omit:
                        Find(c.enums)

                    __Traverse(c, interface_namespace, enums)

            if isinstance(tree, CppParser.Namespace):
                Find(tree.enums)

                for n in tree.namespaces:
                    __Traverse(n, namespace, enums)

        __Traverse(tree, namespace, enums)

        return enums

    enums = FindEnums(tree, ns)

    if enums:
        schema = OrderedDict()
        schema["$schema"] = "interface.json.schema"
        schema["jsonrpc"] = "2.0"
        schema["@generated"] = True
        schema["@fullname"] = ns + "::__single_enumerations"
        schema["namespace"] = ns
        schema["@interfaceonly"] = True
        schema["@enumsonly"] = True
        schema["info"] = OrderedDict()
        schema["info"]["version"] = [1, 0, 0]
        schema["info"]["sourcefile"] = os.path.basename(file)
        schema["info"]["class"] = os.path.basename(file).split('.')[0]
        schema["info"]["title"] = os.path.basename(file) + " enum conversions"
        schema["methods"] = OrderedDict()
        params = OrderedDict()

        for enum in enums:
            props = BuildEnum(log, CaseConverter(enum.meta.text), enum, enum.meta)
            params[enum.name] = props[1]
            params[enum.name]["type"] = props[0]
            params[enum.name]["description"] = "Enum " + enum.name

        method = OrderedDict()
        method["params"] = {"description": "Dummy object", "type": "object", "properties": params}
        method["omit"] = True
        method["summary"] = "Dummy method"
        schema["methods"]["dummy"] = method

        return [schema], []
    else:
        return [], []


def LoadInterfaceInternal(file, tree, ns, log, scanned, all = False, include_paths = []):

    def StripInterfaceNamespace(identifier):
        return str(identifier).replace(ns + "::", "")

    interfaces = [i for i in CppInterface.FindInterfaceClasses(tree, ns, file, []) if ((i.obj.is_json) or (all and not i.obj.is_event))]

    def Build(face):
        def _EvaluateRpcFormat(obj):
            rpc_format = config.RPC_FORMAT

            if not config.RPC_FORMAT_FORCED:
                if obj.is_collapsed:
                    rpc_format = config.RpcFormat.COLLAPSED
                elif obj.is_extended:
                    rpc_format = config.RpcFormat.EXTENDED
                elif obj.is_compliant:
                    rpc_format = config.RpcFormat.COMPLIANT

            return rpc_format

        def handle_optional(var, var_type, properties, quiet=False):
            if "optional" not in var.meta.decorators:
                return "@optionaltype" in properties
            else:
                if "@optionaltype" not in properties:
                    if not quiet:
                        log.WarnLine(var, "%s: @optional tag is deprecated, use Core::OptionalType<...> instead" % var.name)

                    is_iterator = isinstance(var_type.type, CppParser.Class) and var_type.type.is_iterator

                    if not var.meta.output:
                        if isinstance(var_type.type, (CppParser.Integer, CppParser.Enum)):
                            if not var.meta.default and not quiet:
                                log.WarnLine(var, "%s: default value is assumed to be (%s)0 (use @default with @optional)" % (var.name, var.type.TypeName()))
                        elif not isinstance(var_type.type, (CppParser.String, CppParser.Vector, CppParser.Class)) or (var_type.IsPointer() and not is_iterator):
                            raise CppParseError(var, "%s: @optional is not supported for this type" % var.name)
                    else:
                        if not isinstance(var_type.type, (CppParser.String, CppParser.Vector, CppParser.Class)) or (var_type.IsPointer() and not is_iterator):
                            raise CppParseError(var, "%s: @optional is not supported for this type on output" % var.name)
                else:
                    log.WarnLine(var, "%s: @optional is redundant for OptionalType<...>" % var.name)

                properties["optional"] = True
                ConvertDefault(var, properties["type"], properties)

                return True


        log.Info("Parsing interface %s in file %s" % (face.obj.full_name, file))

        if face.obj.json_prefix:
            log.Info("Additional method prefix: %s" % face.obj.json_prefix)

        schema = OrderedDict()
        methods = OrderedDict()
        properties = OrderedDict()
        events = OrderedDict()

        schema["$schema"] = "interface.json.schema"
        schema["jsonrpc"] = "2.0"
        schema["@generated"] = True
        schema["@fullname"] = face.obj.full_name
        schema["namespace"] = ns

        if face.obj.is_json:
            schema["mode"] = "auto"

            if face.obj.is_custom_lookup and face.obj.is_auto_lookup:
                raise CppParseError(face.obj, "interface cannot be auto lookup and custom lookup at the same time")

            if face.obj.is_custom_lookup:
                schema["custom_lookup"] = True
                log.Info("Interface is intended for custom lookup")
            elif face.obj.is_custom_lookup:
                schema["auto_lookup"] = True
                log.Info("Interface is intended for auto lookup")

            rpc_format = _EvaluateRpcFormat(face.obj)
            assert not face.obj.is_event
        else:
            rpc_format = config.RpcFormat.COLLAPSED

        verify = face.obj.is_json or face.obj.is_event

        _case_converter = CaseConverter(face.obj.meta.text)

        if not _case_converter.convention:
            raise CppParseError(face.obj, "unknown interface-level @text parameter:%s" % face.obj.meta.text)
        else:
            log.Info("Case convention is %s" % _case_converter.convention.value)


        schema["@interfaceonly"] = True
        schema["configuration"] = { "nodefault" : True }

        info = dict()
        info["format"] = rpc_format.value

        log.Info("JSON-RPC format is %s" % rpc_format.value)

        if not face.obj.parent.full_name.endswith(ns):
            info["namespace"] = face.obj.parent.name[1:] if (face.obj.parent.name[0] == "I" and face.obj.parent.name[1].isupper()) else face.obj.parent.name

        info["class"] = face.obj.name[1:] if face.obj.name[0] == "I" else face.obj.name
        scoped_face = face.obj.full_name.split("::")[1:]

        info["interface"] = StripInterfaceNamespace("::" + "::".join(scoped_face))
        info["sourcefile"] = os.path.basename(file)

        if face.obj.omit_mode:
             schema["@omit"] = True

        if face.obj.sourcelocation:
            info["sourcelocation"] = face.obj.sourcelocation

        # global switch for interface
        wrapped_result = "wrapped" in face.obj.meta.decorators

        if face.obj.json_version:
            try:
                info["version"] = [int(x) for x in face.obj.json_version.split(".")]
            except:
                raise CppParseError(face.obj, "Interface version must be provided in major[.minor[.patch]] format")

        info["title"] = info["class"] + " API"
        info["description"] = info["class"] + " JSON-RPC interface"

        if _case_converter.is_legacy:
            info["@legacylowercase"] = True

        schema["info"] = info

        clash_msg = "JSON-RPC name clash detected"

        passed_interfaces = []

        event_interfaces = set()

        for interface in face.obj.classes:
            if interface.is_event:
                event_interfaces.add(CppInterface.Interface(interface, 0, file))

        def ResolveTypedef(type, parent=type):
            if isinstance(type, str):
                raise CppParseError(parent, "%s: undefined type" % type)
            elif isinstance(type, list):
                raise CppParseError(parent, "%s: undefined type" % " ".join(type))

            return type.Resolve()

        def ConvertType(var, quiet=False, meta=None, no_array=False):
            if not meta:
                meta = var.meta

            if isinstance(var.type, str):
                raise CppParseError(var, "%s: undefined type" % var.type)
            elif isinstance(var.type, list):
                raise CppParseError(var, "%s: undefined type" % " ".join(var.type))

            var_type = ResolveTypedef(var.type)

            if isinstance(var_type, str):
                raise CppParseError(var, "%s: undefined type" % var_type)
            elif isinstance(var_type, list):
                raise CppParseError(var, "%s: undefined type" % " ".join(var_type))

            cppType = var_type.Type()

            is_iterator = (isinstance(cppType, CppParser.Class) and cppType.is_iterator)
            is_bitmask = "bitmask" in meta.decorators

            result = None

            if (meta.maxlength and meta.maxlength != ["void"]) and (not meta.length or meta.length == ["void"]) and meta.input:
                meta.length = meta.maxlength
                meta.maxlength = None
                log.WarnLine(var, "'%s': no @length tag, using @maxlegth for actual return buffer size" % var.name)
            elif (not meta.maxlength or meta.maxlength == ["void"]) and (meta.length and meta.length != ["void"]) and meta.output:
                log.WarnLine(var, "'%s': no @maxlength tag, using @length for maximum return buffer size" % var.name)

            encoding = None
            for x in meta.decorators:
                if x.startswith("encode:"):
                    encoding = x[7:]
                    break

            if isinstance(cppType, CppParser.Optional) and (not var.array or no_array):
                result = ConvertType(cppType.optional, quiet=quiet, meta=var.meta)
                result[1]["@optionaltype"] = True
                result[1]["@originaltype"] = StripFrameworkNamespace(cppType.optional)
                result[1]["@proto"] = var.ProtoFmt()
                ConvertDefault(var, result[0], result[1])

            # Pointers
            elif var_type.IsPointer() and (is_iterator or (meta.length and meta.length != ["void"]) or var.array) and not no_array and not is_bitmask:

                # C-style buffers that will be converted to base64 encoded JSON strings
                if isinstance(cppType, CppParser.Integer) and (cppType.size == "char") and encoding:
                    props = {}

                    props["@originaltype"] = cppType.type
                    props["encode"] = encoding
                    props["@proto"] = var.ProtoFmt()

                    if meta.output and meta.maxlength:
                        props["@maxlength"] = " ".join(meta.maxlength)

                    if meta.length:
                        props["@length"] = " ".join(meta.length)
                    elif var.array:
                        props["@arraysize"] = var.array
                        props["range"] = [var.array, var.array]

                        #if var.array > 0xFFFF:
                        #    log.WarnLine(var, "%s: large array size (%s elements)" % (var.name, var.array))
                    else:
                        assert False

                    result = ["string", props]

                # C-style buffers converted to JSON arrays (no encode tag)
                elif isinstance(cppType, CppParser.Integer) and (cppType.size == "char") and not var.array:
                    props = {}
                    props["items"] = ConvertParameter(var, quiet=quiet, no_array=True)
                    props["@length"] = " ".join(meta.length)
                    props["@proto"] = var.ProtoFmt()

                    if meta.output and meta.maxlength:
                        props["@maxlength"] = " ".join(meta.maxlength)

                    result = ["array", props]

                # C-style buffers converted to JSON array (not char type or fixed array)
                elif isinstance(cppType, (CppParser.Class, CppParser.String, CppParser.Bool, CppParser.Integer, CppParser.Float, CppParser.Enum, CppParser.Optional)) and var.array:
                    if encoding:
                        raise CppParseError(var, "'%s': @encode only available for char buffers" % var.name)

                    if var.array > 0xFFFF:
                        log.WarnLine(var, "%s: large array size (%s elements)" % (var.name, var.array))

                    props = {}
                    props["items"] = ConvertParameter(var, quiet=quiet, no_array=True)
                    props["@arraysize"] = var.array
                    props["@proto"] = var.ProtoFmt()
                    props["range"] = [var.array, var.array]

                    result = ["array", props]

                # Iterators that will be converted to JSON arrays
                elif is_iterator and len(cppType.args) == 2:
                    # Take element type from return value of the Current() method
                    currentMethod = next((x for x in cppType.methods if x.name == "Current"), None)

                    if currentMethod == None:
                        raise CppParseError(var, "%s is not an @iterator type" % cppType.type)

                    props = {}

                    props["items"] = ConvertParameter(currentMethod.retval, quiet=quiet)
                    props["@iterator"] = StripInterfaceNamespace(cppType.type)
                    props["@proto"]= var.ProtoFmt()

                    if var_type.IsPointerToConst():
                        raise CppParseError(var, "iterators must be passed as const pointers (not pointers to const)")

                    if not quiet:
                        if meta.length:
                            log.WarnLine(var, "'%s': @length has no effect on iterators" % var.name)
                        if meta.default:
                            log.WarnLine(var, "'%s': @default has no effect on iterators" % var.name)
                        if encoding:
                            log.WarnLine(var, "'%s': @encode has no effect on iterators" % var.name)

                    result = ["array", props]

                # All other pointer types are not supported
                else:
                    raise CppParseError(var, "unable to convert C++ type to JSON type: %s (input pointer allowed only on interator or C-style buffer)" % cppType.type)

                if "extract" in meta.decorators:
                    result[1]["@extract"] = True

            elif var_type.IsPointer() and not isinstance(cppType, CppParser.Class) and not no_array and not is_bitmask:
                raise CppParseError(var, "unable to convert C++ type to JSON type, missing @length?")

            # Primitives
            else:
                # String
                if isinstance(cppType, CppParser.String):
                    if "opaque" in meta.decorators :
                        result = [ "string", { "opaque": True } ]
                    else:
                        result = [ "string", {} ]

                # Boolean
                elif isinstance(cppType, CppParser.Bool):
                    result = ["boolean", {} ]

                # Integer
                elif isinstance(cppType, CppParser.Integer):
                    size = 8 if cppType.size == "char" else 16 if cppType.size == "short" else \
                        32 if cppType.size == "int" or cppType.size == "long" else 64 if cppType.size == "long long" else 32

                    result = [ "integer", { "size": size, "signed": cppType.signed } ]

                # Instance ID
                elif isinstance(cppType, CppParser.InstanceId):
                    result = [ "instanceid", {} ]

                # Time
                elif isinstance(cppType, CppParser.Time):
                    result = [ "string", { "time": "iso8601" } ]

                # MACAddress
                elif isinstance(cppType, CppParser.MacAddress):
                    result = [ "string", { "@macaddress": True, "@construct_from_cstr": True, "range": [6,6] } ]

                # Float
                elif isinstance(cppType, CppParser.Float):
                    result = [ "number", { "float": True, "size": 32 if cppType.type == "float" else 64 if cppType.type == "double" else 128 } ]

                # Null
                elif isinstance(cppType, CppParser.Void):
                    result = [ "null", {} ]

                # Enums
                elif isinstance(cppType, CppParser.Enum):
                    result = BuildEnum(log, _case_converter, cppType, meta, var, var.type.Type().meta.decorators, quiet)

                # std::vector
                elif isinstance(cppType, CppParser.DynamicArray):
                    if isinstance(cppType.element.Type().type, (CppParser.Optional, CppParser.DynamicArray)):
                        raise CppParseError(var, "usupported type for std::vector element")

                    props = { "items": ConvertParameter(cppType.element, quiet=quiet), "@container": "vector" }

                    if meta.range:
                        props["range"] = meta.range.as_list

                    if encoding:
                        if not isinstance(cppType.element.Type().type, CppParser.Integer) or cppType.element.Type().type.size != "char":
                            raise CppParseError(var, "invalid type for encoded std::vector")

                        props["encode"] = encoding
                        props["@originaltype"] = cppType.type
                        result = ["string", props]
                    else:
                        result = ["array", props]

                # POD objects
                elif isinstance(cppType, CppParser.Class):
                    def GenerateObject(ctype, from_typedef):
                        properties = dict()

                        kind = ctype.Merge()

                        required = []

                        for p in kind.vars:
                            p_name = compute_name(log, _case_converter, p, _case_converter.MEMBERS)
                            p_type = ResolveTypedef(p.type)

                            if isinstance(p.type, list):
                                raise CppParseError(p, "%s: undefined type" % " ".join(p.type))
                            if isinstance(p.type, str):
                                raise CppParseError(p, "%s: undefined type" % p.type)
                            elif isinstance(p_type.Type(), CppParser.Class) and not p.array:
                                _, props = GenerateObject(p_type.Type(), isinstance(p.type.Type(), CppParser.Typedef))
                                properties[p_name] = props
                                properties[p_name]["type"] = "object"
                                properties[p_name]["@originaltype"] = StripFrameworkNamespace(p.type.Type().full_name)

                                if p.meta.brief:
                                    properties[p_name]["description"] = p.meta.brief
                            else:
                                properties[p_name] = ConvertParameter(p, quiet=quiet)

                            properties[p_name]["@originalname"] = p.name

                            if from_typedef:
                                properties[p_name]["@register"] = False

                            if not handle_optional(p, p_type, properties[p_name], quiet):
                                required.append(p_name)

                        return "object", { "properties": properties, "required": required }

                    if "Core::JSONRPC::Context" in cppType.full_name:
                        result = [ "@context", {} ]
                    elif (cppType.vars and not cppType.methods) or not verify:
                        result = GenerateObject(cppType, isinstance(var.type.Type(), CppParser.Typedef))
                    elif cppType.is_json and cppType.is_custom_lookup:
                        obj_prefix = (cppType.name[1:] if cppType.name[0] == 'I' else cppType.name)
                        result =  [ "string", { "@lookup": "custom" } ]
                        if not [x for x in passed_interfaces if x["fullname"] == cppType.full_name]:
                            passed_interfaces.append({ "fullname": cppType.full_name, "id": "custom", "type": "string", "object": obj_prefix, "prefix": compute_prefix(log, _case_converter, None, obj_prefix), "fullprefix": compute_prefix(log, _case_converter, cppType, obj_prefix)})
                    elif cppType.is_json and cppType.is_auto_lookup:
                        obj_prefix = (cppType.name[1:] if cppType.name[0] == 'I' else cppType.name)
                        result =  [ "integer", { "size": 32, "signed": False, "@lookup": "auto" } ]
                        if not [x for x in passed_interfaces if x["fullname"] == cppType.full_name]:
                            passed_interfaces.append({ "fullname": cppType.full_name, "id": "generate", "type": "uint32_t", "object": obj_prefix, "prefix": compute_prefix(log, _case_converter, None, obj_prefix), "fullprefix": compute_prefix(log, _case_converter, cppType, obj_prefix)})
                    else:
                        if cppType.is_iterator:
                            raise CppParseError(var, "iterators must be passed by pointer: %s" % cppType.type)
                        elif "async" in method.retval.meta.decorators:
                            async_method = dict()
                            for im in cppType.methods:
                                if im.IsVirtual() and not im.IsDestructor():
                                    if not async_method:
                                        async_method["name"] = prefix + compute_name(log, _case_converter, method.retval, _case_converter.EVENTS, relay=method)
                                        async_method["@originalname"] = im.name
                                        async_method["@originaltype"] = StripFrameworkNamespace(cppType.type)
                                        async_method["params"] = BuildParameters(None, im.vars, rpc_format)

                                        if BuildResult(im.vars)["type"] != "null":
                                            raise CppParseError(im, "async callback method must not return anything")

                                        if not isinstance(im.retval.Type().type, CppParser.Void):
                                            raise CppParseError(im, "async callback method must have void return value")
                                    else:
                                        log.WarnLine(var, "'%s': callback interface has mulitple methods; the first one ('%s') will be used for JSON-RPC notification" % (cppType.name, async_method["@originalname"]))
                                        break

                            if async_method:
                                result = [ "string", { "@async": async_method } ]
                            else:
                                raise CppParseError(var, "callback interface has no methods defined")
                        else:
                            raise CppParseError(var, "unable to convert this C++ class to JSON type: %s (passing a non-lookup interface is not possible)" % cppType.type)

                # All other types are not supported
                else:
                    raise CppParseError(var, "unable to convert this C++ type to JSON type: %s (unsupported type)" % cppType.type)

                if var_type.IsPointer():
                    result[1]["@bypointer"] = True

                if var_type.IsReference():
                    result[1]["@byreference"] = True

                if var_type.CVString():
                    result[1]["@cv"] = var_type.CVString()

                if var.Proto():
                    result[1]["@proto"] = var.ProtoFmt()

            if result:
                if meta.range.is_set:
                    result[1]["range"] = meta.range.as_list

                    if meta.default:
                        if result[0] == "integer" or result[0] == "number":
                            if (meta.range.has_min and float(meta.default[0]) < meta.range.min) or (meta.range.has_max and (float(meta.default[0]) > meta.range.max)):
                                raise CppParseError(var, "default value is outside of restrict range")
                        elif result[0] == "string" and not result[1].get("enum"):
                            if (meta.range.has_min and (len(meta.default[0]) - 2 < meta.range.min)) or (meta.range.has_max and (len(meta.default[0]) - 2  > meta.range.max)):
                                raise CppParseError(var, "default value string length is outside of restrict range")
            else:
                assert False

            return result

        def ExtractExample(var):
            egidx = var.meta.brief.index("(e.g.") if "(e.g." in var.meta.brief else None

            if egidx != None and ")" in var.meta.brief[egidx + 1:]:
                example = var.meta.brief[egidx + 5:var.meta.brief.rfind(")")].strip(' "')
                description = var.meta.brief[0:egidx].strip()
                return [example, description]
            else:
                return None

        def ConvertParameter(var, quiet=False, no_array=False):
            jsonType, args = ConvertType(var, quiet=quiet, no_array=no_array)
            properties = {"type": jsonType}

            if args != None:
                properties.update(args)
                try:
                    properties["@originaltype"] = StripFrameworkNamespace(var.type.Type().full_name)
                except:
                    pass

            if var.meta.brief:
                # Also attempt to craft some description
                pair = ExtractExample(var)
                if pair:
                    properties["example"] = pair[0]
                    properties["description"] = pair[1]
                else:
                    properties["description"] = var.meta.brief.strip()

            if var.meta.is_deprecated:
                properties["deprecated"] = True

            return properties

        def EventParameters(vars):
            events = []

            for var in vars:
                def ResolveTypedef(resolved, events, type):
                    if not isinstance(type, list):
                        if isinstance(type.Type(), CppParser.Typedef):
                            if type.Type().is_event:
                                events.append(var)
                            ResolveTypedef(resolved, events, type.Type())
                        else:
                            if isinstance(type.Type(), CppParser.Class) and type.Type().is_event:
                                events.append(var)
                    else:
                        raise CppParseError(var, "undefined type: '%s'" % " ".join(type))

                    resolved.append(type)
                    return events

                resolved = []
                events = ResolveTypedef(resolved, events, var.type)

            return events

        def ConvertDefault(var, type, converted):
            if var.meta.default:
                try:
                    if type == "integer":
                        converted["@default"] = int(var.meta.default[0])
                        converted["default"] = converted["@default"]
                    elif type == "number":
                        converted["@default"] = float(var.meta.default[0])
                        converted["default"] = converted["@default"]
                    elif type == "boolean":
                        converted["@default"] = ("true" if var.meta.default[0] == "true" else "false")
                        converted["default"] = (var.meta.default[0] == "true")
                    elif type == "string" and "enum" in converted:
                        if var.meta.default[0] not in converted.get("ids"):
                            raise CppParseError(var, "default value for enumerator type is not an enum value")
                        converted["@default"] = converted["@originaltype"] + "::" + str(var.meta.default[0])
                        converted["default"] = str(var.meta.default[0])
                    elif type == "string":
                        if not var.meta.default[0].startswith('"') or not var.meta.default[0].endswith('"'):
                            raise CppParseError(var, "default value for string type must be a quoted string literal %s" % converted.get("type"))
                        converted["@default"] = '_T(' + str(var.meta.default[0]) + ')'
                        converted["default"] = str(var.meta.default[0][1:-1])
                except CppParseError as err:
                    raise err
                except:
                    raise CppParseError(var, "type and default value type mismatch (expected %s, got '%s')" % (converted.get("@originaltype"), var.meta.default[0]))


        def BuildParameters(obj, vars, rpc_format, is_property=False, test=False):
            params = {"type": "object"}
            properties = OrderedDict()
            required = []

            method_asynchronous = False

            for idx,var in enumerate(vars):
                if isinstance(var.type, list):
                    raise CppParseError(var, "unknown type")

                if var.meta.input or not var.meta.output:
                    if verify:
                        if not var.type.IsConst() and (not var.type.IsPointerToConst()):
                            if not var.meta.input:
                                if var.type.IsPointer():
                                    raise CppParseError(var, "ambiguous non-const pointer parameter without @in/@out tag (forgot 'const'?)")
                                else:
                                    log.WarnLine(var, "'%s': non-const parameter assumed to be input (forgot 'const'?)" % var.name)
                            elif not var.meta.output:
                                log.WarnLine(var, "'%s': non-const parameter marked with @in tag (forgot 'const'?)" % var.name)

                    var_name = compute_name(log, _case_converter, var, _case_converter.PARAMS, is_property=is_property)

                    if var_name.startswith("__anonymous_") and not test:
                        raise CppParseError(var.parent, "unnamed parameter %s (input)" % (idx + 1))

                    converted = ConvertParameter(var, quiet=test)

                    if converted["type"] == "string" and "@async" in converted:
                        if method_asynchronous:
                            raise CppParseError(method, "multiple callbacks defined")

                        var_name = compute_name(log, _case_converter, "Id", _case_converter.PARAMS)
                        method_asynchronous = True

                    if var_name in list(properties.keys()):
                        if "@async" in converted:
                            raise CppParseError(var, "only one interface parameter is allowed for async method")
                        else:
                            raise CppParseError(var, "parameter name already exists")

                    if converted["type"] == "@context":
                        if obj:
                            obj["context"] = True
                        else:
                            raise CppParseError(var, "context parameter is only valid for methods")
                    else:
                        if not is_property and not var.name.startswith("__anonymous_"):
                            converted["@originalname"] = var.name

                        converted["@position"] = vars.index(var)

                        if not handle_optional(var, ResolveTypedef(var.type), converted, test):
                            required.append(var_name)

                        if converted["type"] == "string" and not var.type.IsReference() and not var.type.IsPointer() \
                                and not "enum" in converted and not "time" in converted:
                            log.WarnLine(var, "'%s': passing input string by value (forgot &?)" % var.name)

                        if converted.get("@optionaltype") and not var.type.IsReference():
                            log.WarnLine(var, "'%s': passing input optional type by value (forgot &?)" % var.name)

                        properties[var_name] = converted


            params["properties"] = properties
            params["required"] = required

            if is_property and ((rpc_format == config.RpcFormat.EXTENDED) or (rpc_format == config.RpcFormat.COLLAPSED)):
                if len(properties) == 1:
                    return list(properties.values())[0]
                elif len(properties) > 1:
                    params["required"] = required
                    return params
                else:
                    return None
            else:
                if (len(properties) == 0):
                    return None
                elif (len(properties) == 1) and (rpc_format == config.RpcFormat.COLLAPSED):
                    # collapsed format: if only one parameter present then omit the outer object
                    return list(properties.values())[0]
                else:
                    return params

        def BuildIndex(var, test=False):
            return BuildParameters(None, [var], rpc_format.COLLAPSED, True, test)

        def BuildResult(vars, is_property=False, test=False, method=None):
            params = {"type": "object"}
            properties = OrderedDict()
            required = []

            method_wrapped = method and "wrapped" in method.retval.meta.decorators

            for idx,var in enumerate(vars):
                var_type = ResolveTypedef(var.type)

                if var.meta.output:
                    var_name = compute_name(log, _case_converter, var, _case_converter.PARAMS, is_property=is_property)

                    if var_name.startswith("__anonymous_"):
                        raise CppParseError(var, "unnamed parameter %s (result)" % (idx + 1))

                    properties[var_name] = ConvertParameter(var, quiet=test)

                    if var_type.IsValue():
                        raise CppParseError(var, "parameter marked with @out tag must be either a reference or a pointer")

                    if var_type.IsConst():
                        raise CppParseError(var, "parameter marked with @out tag must not be const")

                    if not is_property and not var.name.startswith("__anonymous_"):
                        properties[var_name]["@originalname"] = var.name

                    properties[var_name]["@position"] = vars.index(var)

                    if not handle_optional(var, var_type, properties[var_name], test):
                        required.append(var_name)

            params["properties"] = properties

            if is_property and len(properties) != 1:
                raise CppParseError(var, "property getter must have exactly one output parameter")

            if len(properties) != 1 and method_wrapped:
                log.WarnLine(method, "%s(): @wrapped has no effect on this method" % method.name)

            if len(properties) == 1:
                if not is_property and (method_wrapped or (wrapped_result and properties[list(properties.keys())[0]]["type"] != "object")):
                    params["required"] = required
                    return params
                else:
                    return list(properties.values())[0]
            elif len(properties) > 1:
                params["required"] = required
                return params
            else:
                void = {"type": "null"}
                void["description"] = "Always null"
                return void

        def compute_prefix(log, converter, obj, suffix=None):
            prefix = ""

            if obj:
                if obj.json_prefix:
                    prefix += obj.json_prefix + ("::" if not obj.json_prefix.endswith("_") else "")
                elif config.AUTO_PREFIX and (obj.parent.full_name != ns):
                    prefix += obj.parent.name.lower() + "_"

            if suffix:
                prefix += compute_name(log, converter, suffix, converter.OBJECTS)

            return prefix


        faces = []
        faces.append((compute_prefix(log, _case_converter, face.obj), face.obj.methods))

        for prefix, _methods in faces:
          for method in _methods:

            if not method.IsVirtual() or method.IsDestructor():
                continue

            event_params = EventParameters(method.vars)

            if method.is_excluded:
                if event_params:
                    log.WarnLine(method, "'%s': @json:omit is redundant for notification registration methods" % method.name)

                continue

            # Copy over @text tag to the other method of a property
            if method.retval.meta.text and method.retval.meta.is_property:
                for mm in face.obj.methods:
                    if mm != method and mm.name == method.name:
                        mm.retval.meta.text = method.retval.meta.text
                        break

            # Copy over @alt tag to the other method of a property
            if method.retval.meta.alt and method.retval.meta.is_property:
                for mm in face.obj.methods:
                    if mm != method and mm.name == method.name:
                        mm.retval.meta.alt = method.retval.meta.alt
                        break

            method_name = compute_name(log, _case_converter, method.retval, _case_converter.METHODS, relay=method)

            if method.retval.meta.alt == method_name:
                log.WarnLine(method, "'%s': alternative name is same as default ('%s')" % (method.name, method.retval.meta.alt))

            if method.parent.is_json: # excludes .json inlcusion of C++ headers
                for mm in methods:
                    if mm == prefix + method_name:
                        raise CppParseError(method, "%s ('%s')" % (clash_msg, prefix + method_name))
                    if method.retval.meta.alt and (mm == prefix + method.retval.meta.alt):
                        raise CppParseError(method, "%s ('%s' alternative)" % (clash_msg, prefix + method_name))
                    if methods[mm].get("alt") == method_name:
                        raise CppParseError(method, "%s (override clashes with '%s' alternative)" % (clash_msg, methods[mm].get("alt")))
                    if method.retval.meta.alt and (methods[mm].get("alt") == method.retval.meta.alt):
                        raise CppParseError(method, "%s ('%s' alternatives clash)" % (clash_msg, method.retval.meta.alt))

                for mm in properties:
                    if properties[mm]["@originalname"] != method.name:
                        if mm == prefix + method_name:
                            raise CppParseError(method, "%s ('%s')" % (clash_msg, prefix + method_name))
                        if method.retval.meta.alt and (mm == prefix + method.retval.meta.alt):
                            raise CppParseError(method, "%s ('%s' alternative)" % (clash_msg, prefix + method_name))
                        if properties[mm].get("alt") == method_name:
                            raise CppParseError(method, "%s (override clashes with '%s' alternative)" % (clash_msg, properties[mm].get("alt")))
                        if method.retval.meta.alt and (properties[mm].get("alt") == method.retval.meta.alt):
                            raise CppParseError(method, "%s ('%s' alternatives clash)" % (clash_msg, method.retval.meta.alt))

            for e in event_params:
                exists = any(x.obj.type == e.type.type.type for x in event_interfaces)

                if not exists:
                    event_interfaces.add(CppInterface.Interface(ResolveTypedef(e.type).type, 0, file))

            obj = None

            if method.retval.meta.is_property or (prefix + method_name) in properties:
                if "async" in method.retval.meta.decorators:
                    raise CppParseError(method, "a property cannot be asynchronous")

                if "wrapped" in method.retval.meta.decorators:
                    log.WarnLine(method, "%s(): @wrapped is not supported for properties" % method.name)

                try:
                    obj = properties[prefix + method_name]
                except:
                    obj = OrderedDict()
                    properties[prefix + method_name] = obj

                obj["@originalname"] = method.name
                method.retval.meta.is_property = True

                indexed_property = (len(method.vars) == 2 and method.vars[0].meta.is_index)

                if len(method.vars) == 1 or (len(method.vars) == 2 and indexed_property):
                    if indexed_property:
                        #if method.vars[0].type.IsPointer():
                        #    raise CppParseError(method.vars[0], "index to a property must not be a pointer")

                        if not method.vars[0].type.IsConst() and method.vars[0].type.IsReference():
                            raise CppParseError(method.vars[0], "index to a property must be an const input parameter")

                        if "index" not in obj:
                            obj["index"] = [None, None]

                        _index_idx = (0 if "const" in method.qualifiers else 1)

                        obj["index"][_index_idx] = BuildIndex(method.vars[0])
                        _index = obj["index"][_index_idx]

                        if _index:
                            _index["name"] = _case_converter.transform(method.vars[0].name, _case_converter.PARAMS)
                            _index["@originalname"] = method.vars[0].name

                            if "example" not in _index:
                                # example not specified, let's invent something...

                                if method.vars[0].meta.default:
                                    _index["example"] = method.vars[0].meta.default[0]
                                elif "enum" in _index:
                                    _index["example"] = _index["enum"][0]
                                else:
                                    _index["example"] = ("0" if _index["type"] == "integer" else "xyz")

                                _index["fake_example"] = True

                            if _index["type"] not in ["integer", "string", "boolean"]:
                                raise CppParseError(method.vars[0], "index to a property must be integer, enum, boolean, string, encoded array of bytes or encoded std::vector of bytes")
                        else:
                            raise CppParseError(method.vars[0], "failed to determine type of index")

                        if obj["index"][0] and obj["index"][1]:
                            if obj["index"][0]["type"] != obj["index"][1]["type"]:
                                raise CppParseError(method.vars[0], "setter and getter of the same property must have same index type")

                            if "description" in obj["index"][0] and "description" not in obj["index"][1]:
                                obj["index"][1]["description"] = obj["index"][0]["description"]
                            elif "description" in obj["index"][1] and "description" not in obj["index"][0]:
                                obj["index"][0]["description"] = obj["index"][1]["description"]

                            if "fake_example" in obj["index"][0] and "fake_example" not in obj["index"][1]:
                                obj["index"][0]["example"] = obj["index"][1]["example"]
                                del obj["index"][0]["fake_example"]
                            elif "fake_example" in obj["index"][1] and "fake_example" not in obj["index"][0]:
                                obj["index"][1]["example"] = obj["index"][0]["example"]
                                del obj["index"][1]["fake_example"]

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
                            # These are predefined by JSON-RPC standerd, so do not follow case convention rules
                            if rpc_format == config.RpcFormat.COLLAPSED:
                                result_name = "params"
                            else:
                                result_name = "result"

                            if "writeonly" in obj:
                                del obj["writeonly"]
                            else:
                                obj["readonly"] = True

                            if result_name not in obj:
                                obj[result_name] = BuildResult([method.vars[value]], True, False, method)
                            else:
                                test = BuildResult([method.vars[value]], True, True, method)

                                if not test:
                                    raise CppParseError(method.vars[value], "property getter method must have one output parameter")

                                if obj[result_name]["type"] != test["type"]:
                                    raise CppParseError(method.vars[value], "setter and getter of the same property must have same type (*1)")

                                if "@bypointer" in test and test["@bypointer"]:
                                    obj[result_name]["@bypointer"] = True

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
                                obj["params"] = BuildParameters(None, [method.vars[value]], rpc_format, True, False)
                            else:
                                test = BuildParameters(None, [method.vars[value]], rpc_format, True, True)

                                if not test:
                                    raise CppParseError(method.vars[value], "property setter method must have one input parameter")

                                if obj["params"]["type"] != test["type"]:
                                    raise CppParseError(method.vars[value], "setter and getter of the same property must have same type (*2)")

                                if "@bypointer" in test and test["@bypointer"]:
                                    obj[result_name]["@bypointer"] = True

                            if obj["params"] == None or obj["params"]["type"] == "null":
                                raise CppParseError(method.vars[value], "property setter method must have one input parameter")
                else:
                    raise CppParseError(method, "property method must have one parameter")

            elif not event_params:
                var_type = ResolveTypedef(method.retval.type)

                if var_type and ((isinstance(var_type.Type(), CppParser.Integer) and (var_type.Type().size == "long")) or not verify):
                    obj = OrderedDict()
                    obj["@originalname"] = method.name

                    params = BuildParameters(obj, method.vars, rpc_format)

                    if params:
                        obj["params"] = params
                        if "async" in method.retval.meta.decorators:
                            obj["@async"] = True

                        #if "properties" in params and params["properties"]:
                        #    if method.name.lower() in [x.lower() for x in params["required"]]:
                        #        raise CppParseError(method, "parameters must not use the same name as the method")


                    obj["result"] = BuildResult(method.vars, method=method)
                    methods[prefix + method_name] = obj

                else:
                    raise CppParseError(method, "method return type must be uint32_t (error code), i.e. pass other return values by reference parameter")
            else:
                for p in method.vars:
                    if p.meta.is_index:
                        index_found = False
                        for e in event_params:
                            for ev in event_interfaces:
                                if ev.obj.type == e.type.type.type:
                                    for evm in e.type.type.methods:
                                        for evmp in evm.vars:
                                            if evmp.name == p.name:
                                                if "index-by-register" not in evmp.meta.decorators:
                                                    if "index-deprecated" in p.meta.decorators:
                                                        if "index-deprecated" not in evmp.meta.decorators and evmp.meta.is_index:
                                                            raise CppParseError(evm, "%s: event index depreciation mismatch (expected @index:deprecated)" % evmp.name)
                                                        evmp.meta.decorators.append("index-deprecated")
                                                    elif "index-deprecated" in evmp.meta.decorators and evmp.meta.is_index:
                                                        raise CppParseError(evm, "%s: event index depreciation mismatch (did not expect @index:deprecated)" % evmp.name)

                                                    if evmp.meta.is_index:
                                                        log.WarnLine(evm, "%s: @index is deprecated here, already specified by registration method '%s'" % (evmp.name, evm.full_name))

                                                    evmp.meta.decorators.append("index-by-register")
                                                    evmp.meta.is_index = True

                                                    if isinstance(p.type.type, CppParser.Optional):
                                                        evmp.type.type = CppParser.Optional(CppParser.OptionalElement(evmp, [evmp.type.type.type]))
                                                        evmp.type.ref |= CppParser.Ref.REFERENCE

                                                    if str(evmp.type.type) != str(p.type.type):
                                                        raise CppParseError(evm, "%s: event index type mismatch (expected '%s')" % (evmp.name, p.type))

                                                index_found = True

                        if not index_found:
                            raise CppParseError(p, "event index '%s' not found in any event methods" % p.name)

            if obj:
                if method.retval.meta.is_deprecated:
                    obj["deprecated"] = True
                elif method.retval.meta.is_obsolete:
                    obj["obsolete"] = True

                if method.retval.meta.brief:
                    obj["summary"] = method.retval.meta.brief.strip()

                if method.retval.meta.details:
                    obj["description"] = method.retval.meta.details.strip()

                if method.retval.meta.pre:
                    obj["preconditions"] = method.retval.meta.pre.strip()

                if method.retval.meta.post:
                    obj["postconditions"] = method.retval.meta.post.strip()

                if method.retval.meta.retval:
                    errors = []

                    for err in method.retval.meta.retval:
                        errEntry = OrderedDict()
                        errEntry["description"] = method.retval.meta.retval[err]
                        errEntry["message"] = err
                        errors.append(errEntry)

                    if errors:
                        obj["errors"] = errors

                if config.LEGACY_ALT:
                    upd = properties if method.retval.meta.is_property else methods

                if method.retval.meta.alt:
                    idx = prefix + method.retval.meta.alt
                    obj["alt"] = idx

                    if method.retval.meta.alt_is_deprecated:
                        obj["altisdeprecated"] = method.retval.meta.alt_is_deprecated

                    if method.retval.meta.alt_is_obsolete:
                        obj["altisobsolete"] = method.retval.meta.alt_is_obsolete

                    if config.LEGACY_ALT:
                        idx = prefix + method.retval.meta.alt
                        upd[idx] = copy.deepcopy(obj)
                        upd[idx]["alt"] = prefix + method_name
                        obj["alt"] = idx

                        if "deprecated" in upd[idx]:
                            del upd[idx]["deprecated"]
                        if "obsolete" in upd[idx]:
                            del upd[idx]["obsolete"]

                        if method.retval.meta.alt_is_deprecated:
                            upd[idx]["deprecated"] = True
                        elif method.retval.meta.alt_is_obsolete:
                            upd[idx]["obsolete"] = True

                elif "alt" in obj:
                    if config.LEGACY_ALT:
                        o = upd[obj["alt"]]
                        if "readonly" in o and "readonly" not in obj:
                            del o["readonly"]
                        if "writeonly" in o and "writeonly" not in obj:
                            del o["writeonly"]

        for f in event_interfaces:
            rpc_format = _EvaluateRpcFormat(f.obj)

            for method in f.obj.methods:
                EventParameters(method.vars) # just to check for undefined types...

                if method.IsVirtual() and not method.IsDestructor() and not method.is_excluded:
                    obj = OrderedDict()
                    obj["@originalname"] = method.name
                    varsidx = 0

                    if len(method.vars) > 0:
                        for v in method.vars[1:]:
                            if v.meta.is_index:
                                if "index-by-register" not in v.meta.decorators:
                                    log.WarnLine(method, "%s: @index ignored, event index must be at the first parameter" % v.name)
                                else:
                                    raise CppParseError(method, "%s: event index must be at the first parameter" % v.name)

                        if method.vars[0].meta.is_index:
                            if method.vars[0].type.IsPointer():
                                raise CppParseError(method, "%s: index to a notification must not be pointer" % method.vars[0].name)

                            if not method.vars[0].type.IsConst() and method.vars[0].type.IsReference():
                                raise CppParseError(method, "%s: index to a notification must be an input parameter" % method.vars[0].name)

                            if not "index-by-register" in method.vars[0].meta.decorators:
                                log.WarnLine(method, "%s: @index is deprecated here, declare in event registration method instead" % method.vars[0].name)

                            obj["id"] = BuildParameters(None, [method.vars[0]], config.RpcFormat.COLLAPSED, True, False)

                            deprecated = "index-deprecated" in method.vars[0].meta.decorators

                            if obj["id"]:
                                obj["id"]["name"] = method.vars[0].name
                                obj["id"]["@originalname"] = "designatorId_" if deprecated else "index_"
                                obj["id"]["@generated"] = True

                                if deprecated:
                                    obj["id"]["deprecated"] = True
                                    varsidx = 1
                                else:
                                    varsidx = 0 if isinstance(method.vars[0].type.type, CppParser.Optional) else 1

                                if "example" not in obj["id"]:
                                    obj["id"]["example"] = "0" if obj["id"]["type"] == "integer" else "abc"

                                if obj["id"]["type"] not in ["integer", "string"]:
                                    raise CppParseError(method.vars[0], "id of a notification must be integer, enum or string type")

                            else:
                                raise CppParseError(method.vars[0], "failed to determine type of notification id parameter")

                    if "statuslistener" in method.retval.meta.decorators:
                        obj["statuslistener"] = True

                    params = BuildParameters(None, method.vars[varsidx:], rpc_format, False)
                    retvals = BuildResult(method.vars[varsidx:], method=method)

                    if retvals and retvals["type"] != "null":
                        raise CppParseError(method, "output parameters are invalid for JSON-RPC events")

                    if method.retval.meta.is_deprecated:
                        obj["deprecated"] = True
                    elif method.retval.meta.is_obsolete:
                        obj["obsolete"] = True

                    if method.retval.meta.brief:
                        obj["summary"] = method.retval.meta.brief.strip()

                    if method.retval.meta.details:
                        obj["description"] = method.retval.meta.details.strip()

                    if method.retval.meta.pre:
                        obj["preconditions"] = method.retval.meta.pre.strip()

                    if method.retval.meta.post:
                        obj["postconditions"] = method.retval.meta.post.strip()

                    if params:
                        obj["params"] = params

                    method_name = compute_name(log, _case_converter, method.retval, _case_converter.EVENTS, relay=method)

                    if method.parent.is_event: # excludes .json inlcusion of C++ headers
                        for mm in events:
                            if mm == prefix + method_name:
                                # raise CppParseError(method, "%s ('%s')" % (clash_msg, prefix + method_name))
                                method_name += "#"
                            if method.retval.meta.alt and (mm == prefix + method.retval.meta.alt):
                                raise CppParseError(method, "%s ('%s' alternative)" % (clash_msg, prefix + method_name))
                            if events[mm].get("alt") == method_name:
                                raise CppParseError(method, "%s (override clashes with '%s' alternative)" % (clash_msg, events[mm].get("alt")))
                            if method.retval.meta.alt and (events[mm].get("alt") == method.retval.meta.alt):
                                raise CppParseError(method, "%s ('%s' alternatives clash)" % (clash_msg, method.retval.meta.alt))

                    if method.retval.meta.alt:
                        obj["alt"] = method.retval.meta.alt

                        if method.retval.meta.alt_is_deprecated:
                            obj["altisdeprecated"] = method.retval.meta.alt_is_deprecated

                        if method.retval.meta.alt_is_obsolete:
                            obj["altisobsolete"] = method.retval.meta.alt_is_obsolete

                    events[prefix + method_name] = obj
                else:
                    log.Info("Omitting method %s()" % method.name)

        if passed_interfaces:
            schema["@interfaces"] = passed_interfaces

        if methods:
            schema["methods"] = methods

        if properties:
            schema["properties"] = properties

        if events:
            schema["events"] = events

        return schema

    schemas = []

    if interfaces:
        for face in interfaces:
            if face.obj.full_name not in scanned:
                schema = Build(face)
                if schema:
                    schemas.append(schema)
                    scanned.append(face.obj.full_name)

        # prepare a declaration of all passed objects
        for s in schemas:
            for face in (s["@interfaces"] if "@interfaces" in s else []):
                for ss in schemas:
                    if ss["@fullname"] == face["fullname"]:
                        if "methods" in ss:
                            methods = ss["methods"]
                            for k,_ in methods.items():
                                split_k = k.split("::")
                                split_k.insert(-1, face["prefix"])
                                new_k = "::".join(split_k)
                                if "methods" not in s:
                                    s["methods"] = {}
                                s["methods"][new_k] = methods.get(k)
                                s["methods"][new_k]["@lookup"] = face
                            ss.pop("methods")
                            ss["@generated"] = False

                        if "properties" in ss:
                            properties = ss["properties"]
                            for k,_ in properties.items():
                                split_k = k.split("::")
                                split_k.insert(-1, face["prefix"])
                                new_k = "::".join(split_k)
                                if "properties" not in s:
                                    s["properties"] = {}
                                s["properties"][new_k] = properties.get(k)
                                s["properties"][new_k]["@lookup"] = face
                            ss.pop("properties")
                            ss["@generated"] = False

                        if "events" in ss:
                            events = ss["events"]
                            for k,_ in events.items():
                                split_k = k.split("::")
                                split_k.insert(-1, face["prefix"] + "#ID")
                                new_k = "::".join(split_k)
                                if "events" not in s:
                                    s["events"] = {}
                                s["events"][new_k] = events.get(k)
                                s["events"][new_k]["@lookup"] = face
                            ss.pop("events")
                            ss["@generated"] = False


        schemas = [s for s in schemas if s.get("@generated")]

    return schemas, []

def LoadInterface(file, log, all = False, include_paths = []):
    try:
        schemas = []
        includes = []
        scanned = []

        tree = CppParser.ParseFiles([os.path.join(os.path.dirname(os.path.realpath(__file__)),
                   posixpath.normpath(config.DEFAULT_DEFINITIONS_FILE)), file], config.FRAMEWORK_NAMESPACE, include_paths, log)

        for ns in config.INTERFACE_NAMESPACES:
            their_schemas, their_includes = LoadInterfaceInternal(file, tree, ns, log, scanned, all, include_paths)

            for s in their_schemas:
                f = list(filter(lambda x: x["@fullname"] == s["@fullname"], schemas))

                if not f:
                    schemas.append(s)

            includes.extend(their_includes)

        for ns in config.INTERFACE_NAMESPACES:
            their_schemas, their_includes = LoadEnumDefinitionsInternal(file, tree, ns, log, scanned, all, include_paths)

            for s in their_schemas:
                f = list(filter(lambda x: x["@fullname"] == s["@fullname"], schemas))

                if not f:
                    schemas.append(s)

            includes.extend(their_includes)

        if not schemas:
            log.Info("No interfaces found")

        else:
            if config.DUMP_JSON:
                for s in schemas:
                    print("\n// JSON interface for %s -----------" % s["@fullname"])
                    print(json.dumps(s, indent=2))
                    print("// ----------------\n")

        return schemas, includes

    except CppParser.ParserError as ex:
        raise CppParseError(None, str(ex))
    except CppParser.LoaderError as ex:
        raise CppParseError(None, str(ex))
