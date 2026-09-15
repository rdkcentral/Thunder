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

#
# Thunder proxy stub code generator
#

import re
import sys
import os
import argparse
import copy
import glob
from collections import OrderedDict
import Log
import CppParser

NAME = "ProxyStubGenerator"

# runtime changeable configuration
INDENT_SIZE = 4
SHOW_WARNINGS = True
BE_VERBOSE = False
EMIT_TRACES = False
FORCE = False

# static configuration
EMIT_COMMENT_WITH_PROTOTYPE = True
EMIT_COMMENT_WITH_STUB_ORDER = True
FRAMEWORK_NAMESPACE = "Thunder"
STUB_NAMESPACE = "::%s::ProxyStubs" % FRAMEWORK_NAMESPACE
INTERFACE_NAMESPACES = ["::%s" % FRAMEWORK_NAMESPACE]
CLASS_IUNKNOWN = "::%s::Core::IUnknown" % FRAMEWORK_NAMESPACE
PROXYSTUB_CPP_NAME = "ProxyStubs_%s.cpp"

ENABLE_CUSTOM_ALLOCATOR = False
ENABLE_SECURE = False
ENABLE_INSTANCE_VERIFICATION = ENABLE_SECURE
ENABLE_RANGE_VERIFICATION = ENABLE_SECURE
ENABLE_INTEGRITY_VERIFICATION = ENABLE_SECURE
ENABLE_ITERATOR_OPTIMIZATION = False

PARAMETER_SIZE_WARNING_THRESHOLD = 4*1024*1024-1

INSTANCE_ID = "Core::instance_id"
HRESULT = "Core::hresult"

DEFAULT_DEFINITIONS_FILE = "default.h"
IDS_DEFINITIONS_FILE = "Ids.h"

log = Log.Log(NAME, BE_VERBOSE, SHOW_WARNINGS)


# -------------------------------------------------------------------------
# Exception classes

class NotModifiedException(RuntimeError):
    pass


class SkipFileError(RuntimeError):
    pass


class NoInterfaceError(RuntimeError):
    pass


class TypenameError(RuntimeError):
    def __init__(self, type, msg):
        if type.parent:
            msg = "%s(%s): %s" % (os.path.basename(type.parent.parser_file), type.parent.parser_line, msg)
        super(TypenameError, self).__init__(msg)
        pass

def Unreachable():
    assert False, "reached theoretically unreachable code! :)"

# -------------------------------------------------------------------------

def CreateName(ns):
    return ns.replace("::I", "").replace("::", "")[1 if ns[0] == "I" else 0:]

def Normalize(n):
    return n.replace('.', '_').replace('[','_').replace(']','_').replace("()",'_').replace('(','_').replace(')','_')

class Emitter:
    def __init__(self, file, size=2):
        self.indent = ""
        self.size = size
        self.file = file

    def IndentInc(self):
        self.indent += ' ' * self.size

    def IndentDec(self):
        self.indent = self.indent[:-INDENT_SIZE]

    def String(self, string):
        self.file.write(string)

    def Line(self, string=""):
        self.String(((self.indent + string) if len((self.indent + string).strip()) else "") + "\n")


class Interface():
    def __init__(self, obj, iid, file):
        self.obj = obj
        self.id = iid
        self.file = file


# Looks for interface classes (ie. classes inheriting from Core::Unknown and specifying ID enum).
def FindInterfaceClasses(tree, namespace):
    interfaces = []
    omit_interface_used = False

    def __Traverse(tree, interface_namespace, faces):
        nonlocal omit_interface_used

        if isinstance(tree, CppParser.Namespace) or isinstance(tree, CppParser.Class):

            for c in tree.classes:
                if c.omit:
                    omit_interface_used = True

                if not isinstance(c, CppParser.TemplateClass):
                    if (c.full_name.startswith(interface_namespace + "::")):
                        if (c.is_iterator and ENABLE_ITERATOR_OPTIMIZATION) and not c.is_force_interface:
                            log.Info("Skipping iterator %s" % c.name, source_file)
                        else:
                            inherits_iunknown = False
                            for a in c.ancestors:
                                if CLASS_IUNKNOWN in str(a[0]):
                                    inherits_iunknown = True
                                    break

                            if inherits_iunknown:
                                has_id = False

                                for e in c.enums:
                                    if not e.scoped:
                                        for item in e.items:
                                            if item.name == "ID":
                                                if item.value != 0:
                                                    faces.append(Interface(c, item.value, source_file))
                                                has_id = True
                                                break

                                if not has_id and not c.omit:
                                    log.Warn("class %s does not have an ID enumerator" % c.full_name, source_file)

                __Traverse(c, interface_namespace, faces)

        if isinstance(tree, CppParser.Namespace):
            for n in tree.namespaces:
                __Traverse(n, namespace, faces)

    __Traverse(tree, namespace, interfaces)

    return interfaces, omit_interface_used


# Cut out scope resolution operators from all identifiers found in a string
def Flatten(identifier, scope):

    if not scope.startswith("::"):
        scope = "::" + scope

    split = scope.replace(" ", "").split("::")
    split[0] = ("::" + split[1])
    identifier = identifier.strip()

    for element in split:
        idx = 0

        while True:
            item = (element + "::")
            idx = identifier.find(item, idx)

            if idx == -1:
                break
            elif idx == 0 or (not identifier[idx - 1].isalnum() and identifier[idx - 1] != ':'):
                identifier = identifier[:idx] + identifier[idx + len(item):]
            else:
                idx += len(item)

    return identifier


# Generate interface information in lua
def GenerateLuaData(emit, interfaces_list, enums_list, source_file=None, tree=None, ns=None):

    if not source_file:
        assert tree==None
        assert ns==None

        emit.Line("-- enums")

        for k,v in enums_list.items():
            emit.Line("ENUMS[\"%s\"] = {" % k)
            emit.IndentInc()
            text = []
            size = len(v)
            idx = 1

            for val,name in v.items():
                emit.Line("[%s] = \"%s\"%s" % (val, name, "," if idx != size else ""))
                idx += 1

            emit.IndentDec()
            emit.Line("}")
            emit.Line()

        return

    interfaces, _ = FindInterfaceClasses(tree, ns)
    if not interfaces:
        return []

    if not interfaces_list:
        emit.Line("-- Interfaces definition data file")
        emit.Line("-- Generated automatically. DO NOT EDIT")
        emit.Line()
        emit.Line("INTERFACES, METHODS, ENUMS, Type = ...")
        emit.Line()

    emit.Line("--  %s" % os.path.basename(source_file))

    iface_namespace_l = ns.split("::")
    iface_namespace = iface_namespace_l[-1]

    for iface in interfaces:
        iface_name = Flatten(iface.obj.type, ns)

        interfaces_var_name = "INTERFACES"
        methods_var_name = "METHODS"

        assume_hresult = iface.obj.is_json

        if not iface.obj.full_name.startswith(ns):
            continue # interface in other namespace

        if iface.obj.omit:
            continue

        id_enumerator = None

        for e in iface.obj.enums:
            id_enumerator = e.Enumerator("ID")
            if id_enumerator:
                break

        id_value = id_enumerator.value

        if id_value in interfaces_list:
            log.Info("Skipping duplicate interface definition %s (%s)" % (iface_name, id_value))
            continue

        interfaces_list[id_value] = True

        emit.Line("%s[%s] = \"%s\"" % (interfaces_var_name, id_value, iface_name))
        emit.Line("%s[%s] = {" % (methods_var_name, id_value))
        emit.IndentInc()
        emit_methods = [m for m in iface.obj.methods if m.IsVirtual() and not m.IsDestructor() and not m.omit]

        for idx, m in enumerate(emit_methods):
            name = "name = \"%s\"" % m.name
            params = []
            retval = []
            items = [ name ]

            def ParseLength(param, length, retval, vars):
                def _Convert(size):
                    if size == "char":
                        return "8"
                    elif size == "long":
                        return "32"
                    else:
                        return "16"

                if isinstance(length, list) and len(length) == 1:
                    if length[0] == "void":
                        return [_Convert(param.Type().size), None]
                    elif length[0] == "return":
                        return [_Convert(retval.type.Type().size), "return"]

                    for v in vars:
                        if v.name == length[0]:
                            return [_Convert(v.type.Type().size), v.name]

                size = "8"

                value = 0
                expr = "".join(length)
                try:
                    value = eval(expr)
                    if value > 0xFF:
                        size = "16"
                    elif value > 0xFFFF:
                        size = "32"
                except:
                    pass

                return [size, value]


            def Convert(paramtype, retval, vars, hresult=False):
                if isinstance(paramtype.type, list):
                    return

                param = paramtype.type.Resolve()
                meta = paramtype.meta
                p = param.Type()

                if isinstance(p, CppParser.Optional):
                    optional_type = Convert(p.optional, retval, vars, hresult)
                    optional_type.append("optional = true")
                    return optional_type

                if isinstance(p, CppParser.Integer):
                    length_param = None
                    length_value = None

                    if param.IsPointer():
                        parsed = ParseLength(param, paramtype.array if paramtype.array else meta.length if meta.length else meta.maxlength, retval, vars)

                        if parsed[1]:
                            if (isinstance(parsed[1], str)):
                                length_param = parsed[1]
                            elif (isinstance(parsed[1], int)):
                                length_value = parsed[1]

                        value = "BUFFER" + parsed[0]
                    else:
                        if paramtype.type.TypeName().endswith(HRESULT):
                            value = "HRESULT"
                        elif p.size == "char" and "signed" not in p.type and "unsigned" not in p.type and "_t" not in p.type:
                            value = "CHAR"
                        else:
                            if p.size == "char":
                                value = "INT8"
                            elif p.size == "short":
                                value = "INT16"
                            elif p.size == "long":
                                value = "INT32"
                            elif p.size == "long long":
                                value = "INT64"

                            if not p.signed:
                                value = "U" + value

                    if value == "UINT32":
                        if hresult:
                           value = "HRESULT"
                        else:
                            if retval and retval.meta.interface and retval.meta.interface[0] == paramtype.name:
                                value = "INTERFACE"
                            else:
                                for v in vars:
                                    if v.meta.interface and v.meta.interface[0] == paramtype.name:
                                        value = "INTERFACE"
                                        break

                    rvalue = ["type = Type." + value]

                    if length_param:
                        rvalue.append("length_param = \"%s\"" % length_param)
                    elif length_value:
                        rvalue.append("length_value = %i" % length_value)

                    return rvalue

                elif isinstance(p, CppParser.String):
                    return ["type = Type.STRING"]

                elif isinstance(p, CppParser.Bool):
                    return ["type = Type.BOOL"]

                elif isinstance(p, CppParser.Float):
                    if p.type == "float":
                        return ["type = Type.FLOAT32"]
                    elif p.type == "double":
                        return ["type = Type.FLOAT64"]

                elif isinstance(p, CppParser.Class):
                    if param.IsPointer():
                        return ["type = Type.OBJECT", "class = \"%s\"" % Flatten(param.TypeName(), ns)]
                    else:
                        value = ["type = Type.POD", "class = \"%s\"" % Flatten(param.type.full_name, ns)]
                        pod_params = []

                        kind = p.Merge()

                        for v in kind.vars:
                            param_info = Convert(v, None, kind.vars)
                            text = []
                            text.append('name = "%s"' % v.name)

                            if param_info:
                                text.extend(param_info)

                            pod_params.append("{ %s }" % ", ".join(text))

                        if pod_params:
                            value.append("pod = { %s }" % ", ".join(pod_params))

                        return value

                elif isinstance(p, CppParser.Enum):
                    value = "32"
                    signed = "U"

                    if p.type.Type().size == "char":
                        value = "8"
                    elif p.type.Type().size == "short":
                        value = "16"

                    if p.type.Type().signed:
                        signed = ""

                    name = Flatten(param.type.full_name, ns)

                    if name not in enums_list:
                        data = dict()
                        for e in p.items:
                            if (isinstance(e.value, int)):
                                data[e.value] = e.name
                            else:
                                log.Warn("unable to evaluate enum value '%s'" % "".join([str(x) for x in e.value]))

                        enums_list[name] = data

                    value =  ["type = Type.ENUM" + signed + value, "enum = \"%s\"" % name]

                    if "bitmask" in meta.decorators or "bitmask" in paramtype.meta.decorators:
                        value.append("bitmask = true")

                    return value

                elif isinstance(p, CppParser.BuiltinInteger) and paramtype.type.TypeName().endswith(INSTANCE_ID):
                    return ["type = Type.OBJECT" ] # but without a class

                elif isinstance(p, CppParser.Void):
                    if param.IsPointer():
                        index = 0

                        if meta.interface:
                            counter = 0
                            for v in vars:
                                if v.meta.input or not v.meta.output:
                                    counter += 1

                                if meta.interface[0] == v.name:
                                    index = counter
                                    break

                        value = ["type = Type.OBJECT"]

                        if index:
                            value.append("interface_param = %s" % index)

                        return value
                    else:
                        return None

                return ["type = nil"]

            rv = Convert(m.retval, m.retval, m.vars, assume_hresult)
            if rv:
                text = []

                if m.retval.name and "__anonymous" not in m.retval.name:
                    text.append("name = \"%s\"" % m.retval.name)

                text.extend(rv)

                skip = False

                for v in m.vars:
                    if (v.meta.length and (v.meta.length[0] == "return")):
                        skip = True
                        break

                if not skip:
                    retval.append(" { %s }" % ", ".join(text))

            for p in m.vars:
                param = Convert(p, m.retval, m.vars)

                if param:
                    text = []

                    if p.name and "__anonymous" not in p.name:
                        text.append("name = \"%s\"" % p.name)

                    text.extend(param)

                    skip = False
                    skip2 = False

                    for v in m.vars:
                        if (v.meta.length and (v.meta.length[0] == p.name)):
                            # Will not be on the wire on inbound!
                            skip = True

                            if (not v.meta.output or v.meta.input):
                                # Will not be on the wire on inbound and outbound!
                                skip2 = True

                    if not skip2:
                        if p.meta.input or not p.meta.output:
                            params.append(" { %s }" % ", ".join(text))

                    if not skip:
                        if p.meta.output:
                            retval.append(" { %s }" % ", ".join(text))

            if retval:
                items.append("retvals = { %s }" % ", ".join(retval))

            if params:
                items.append("params = { %s }" % ", ".join(params))

            # emit.Line("-- %s" % m.Proto())
            emit.Line("[%s] = { %s }%s " % (idx + 3, ", ".join(items), "," if idx != len(emit_methods) - 1 else ""))

        emit.IndentDec()
        emit.Line("}")
        emit.Line()

def RangeStr(range):
    assert len(range) == 2
    return "%s..%s" % (range[0] if range[0] else "min", range[1] if range[1] else "max")

def Parse(source_file, framework_namespace, includePaths = [], defaults = "", extra_includes = []):

    log.Info("Parsing %s..." % source_file)

    files = []

    if defaults:
        files.append(defaults)

    files.extend(extra_includes)
    files.append(source_file)

    tree = CppParser.ParseFiles(files, framework_namespace, includePaths, log)
    if not isinstance(tree, CppParser.Namespace):
        raise SkipFileError(source_file)

    return tree

def GenerateStubs2(output_file, source_file, tree, ns, scan_only=False):
    log.Info("Scanning '%s' (in %s)..." % (source_file, ns))

    if not FORCE and (os.path.exists(output_file) and (os.path.getmtime(source_file) < os.path.getmtime(output_file))):
        raise NotModifiedException(output_file)

    interfaces, omit_interface_used = FindInterfaceClasses(tree, ns)
    if not interfaces:
        return [], omit_interface_used

    if scan_only:
        return interfaces, omit_interface_used

    interface_header_name = os.path.basename(source_file)

    interface_namespace = ns.split("::")[-1]

    with open(output_file, "w") as file:
        emit = Emitter(file, INDENT_SIZE)

        announce_list = OrderedDict()

        vars = { "reader": "reader",
                 "writer": "writer",
                 "channel": "channel",
                 "message": "message",
                 "input": "input",
                 "implementation": "implementation",
                 "result": "result",
                 "interface": "interface",
                 "hresult": "hresult",
                 "tempbuffer": "tempBuffer"
                }

        class AuxIdentifier():
            def __init__(self, type, qualifiers, name, value=None):
                name = Normalize(name)
                self.type = CppParser.Type(type)
                self.kind = self.type.Type()
                self.type.ref = qualifiers
                self.name = name
                self.type_name = self.type.TypeName()
                self.proto_no_cv = Flatten(self.type.Proto("nocv|noref"), FRAMEWORK_NAMESPACE)
                self.proto = Flatten(self.type.Proto("noref"), FRAMEWORK_NAMESPACE)
                self.proto_weak = self.proto
                self.proto_weak_no_cv = self.proto_no_cv
                self.value = value
                self.proxy = None
                self.proxy_instance = None
                self.return_proxy = False

            @property
            def as_lvalue(self):
                return self.name

            @property
            def as_rvalue(self):
                return (self.value if self.value else self.name)

            @property
            def as_pure_rvalue(self):
                return self.as_rvalue

            @property
            def temporary(self):
                return (self.type.Proto() + " " + self.name)

            @property
            def temporary_no_cv(self):
                return (self.type.Proto("nocv") + " " + self.name)

            @property
            def storage_size(self):
                if isinstance(self.kind, (CppParser.Integer, CppParser.Float, CppParser.Enum, CppParser.BuiltinInteger)):
                    return "sizeof(%s)" % self.type_name
                else:
                    Unreachable()

        class EmitIdentifier():
            def __init__(self, interface, identifier, override_name=None, suppress_type=False, prefix="_", suffix="", parent=None):

                def _FindLength(length_name, variable_name):
                    variable_name = Normalize(variable_name)

                    if length_name:
                        if length_name[0] == "void":
                            return EmitLength(interface, CppParser.Temporary(self.identifier.parent, ["uint8_t", variable_name], ["sizeof(%s)" % self.kind], []), variable_name)

                        elif length_name[0] == "return":
                            result = "result"

                            if isinstance(self.identifier.parent.retval.type.Type(), CppParser.Integer):
                                if self.identifier.parent.retval.type.Type().signed:
                                    result = "(result > 0? result : 0)"

                            return EmitRetVal(interface, CppParser.Temporary(self.identifier.parent, [self.identifier.parent.retval.type, "result"], [result], []), "result")

                        elif length_name[0] == "sizeof" and len(length_name) == 4:
                            matches = [v for v in self.identifier.parent.vars if v.name == length_name[2]]

                            if matches:
                                return EmitIdentifier(interface, CppParser.Temporary(self.identifier.parent, ("%s %s" % (length_type, variable_name)).split(), ["sizeof(_%s)" % matches[0].name]), variable_name)
                            else:
                                return EmitIdentifier(interface, CppParser.Temporary(self.identifier.parent, ("%s %s" % (length_type, variable_name)).split(), ["".join(length_name)]), variable_name)

                        matches = [v for v in self.identifier.parent.vars if v.name == "".join(length_name)]

                        if not matches:
                            try:
                                value = eval("".join(length_name))

                                if value > 0xFFFFFFFF:
                                    length_type = "uint64_t"
                                elif value > 0xFFFF:
                                    length_type = "uint32_t"
                                elif value > 0xFF:
                                    length_type = "uint16_t"
                                else:
                                    length_type = "uint8_t"

                                return EmitIdentifier(interface, CppParser.Temporary(self.identifier.parent, ("%s %s" % (length_type, variable_name)).split(), [str(value)]), variable_name)

                            except (SyntaxError, NameError):
                                raise TypenameError(self.identifier, "'%s': unable to parse this length expression ('%s')" % (self.trace_proto, "".join(length_name)))
                        else:
                            return EmitIdentifier(interface, matches[0])

                    return None

                if not isinstance(identifier.type, CppParser.Type):
                    undefined = CppParser.Type(CppParser.Undefined(identifier.type))
                    if identifier.parent and (not identifier.parent.stub and not identifier.parent.omit):
                        raise TypenameError(identifier, "'%s': undefined type" % Flatten(str(" ".join(identifier.type)), ns))
                    else:
                        type_ = undefined
                else:
                    type_ = copy.deepcopy(identifier.type)

                no_length_warnings = isinstance(self, EmitLength)
                is_return_value = isinstance(self, EmitRetVal) or (parent and parent.is_return_value)
                in_pod = '.' in override_name if override_name else False

                self.is_on_wire = True
                self.suffix = suffix
                self.identifier = identifier
                self.identifier_type = type_
                self.identifier_kind = self.identifier_type.Type()
                self.is_output = (identifier.meta.output or is_return_value)
                self.is_input = ((identifier.meta.input or not self.is_output) and not is_return_value)
                self.is_input_only = not identifier.meta.output
                self.is_output_only = ((identifier.meta.output and not identifier.meta.input) or is_return_value)
                self.is_return_value = is_return_value
                self.interface = interface

                # Declare input parameters 'const' even if 'const' is not on the original signature
                self.is_const = not identifier.meta.output

                # Yields identifier name as in the soruce code, meaningful for a human
                self.trace_proto = Flatten(self.identifier.Signature(), ns)

                if not self.identifier.name.startswith("__anonymous") and \
                        (self.identifier.name.startswith("_") or self.identifier.name.endswith("_")):
                    raise TypenameError(self.identifier, "%s: identifier names starting or ending with an underscore are reserved by the generator" % \
                                            self.trace_proto)

                # Always resovlve the type in typedefs...
                additional_ref = (CppParser.Ref.CONST if self.is_const else 0)
                self.type = self.identifier_type.Resolve(additional_ref)
                self.kind = self.type.Type()

                is_class = isinstance(self.kind, CppParser.Class)
                is_struct = is_class and self.kind.vars
                is_interface = is_class and (self.kind.Inherits(CLASS_IUNKNOWN) or not is_struct)

                self.is_iterator = is_class and self.kind.is_iterator

                # Length variables are actually not put on wire
                self.length_of = None
                self.max_length_of = None
                self.length = None
                self.max_length = None
                self.is_buffer = False
                self.is_array = False

                self.is_dynamic_array = False
                self.item = None
                self.element = None

                # Is this a length or max-length of a buffer?
                if self.identifier.parent:
                    for v in self.identifier.parent.vars:
                        if v.meta.length and ("".join(v.meta.length) == self.identifier.name):
                            self.length_of = v
                            if not self.max_length_of and not v.meta.maxlength and self.is_input and v.meta.output:
                                self.max_length_of = v
                                if not v.meta.input:
                                    self.length_of = None

                        if v.meta.maxlength and ("".join(v.meta.maxlength) == self.identifier.name):
                            self.max_length_of = v

                name = (override_name or (prefix + self.identifier.name).replace("__anonymous_", ""))
                self.value = self.identifier.value

                self.is_integer = isinstance(self.kind, CppParser.Integer)
                self.is_string = isinstance(self.kind, CppParser.String)
                self.is_ccstring = (isinstance(self.kind, CppParser.String) and self.kind.is_cc)

                self.interface_id = _FindLength(self.identifier.meta.interface, (name[1:] + "IntefaceId"))

                # Is it a buffer?

                is_array_pointer = (self.type.IsPointer() and not is_interface and not self.interface_id)
                is_buffer = is_array_pointer and isinstance(self.identifier_kind, CppParser.Integer) and (self.identifier_kind.size == "char")
                self.is_array = (self.identifier.array != None)
                self.length = _FindLength(self.identifier.meta.length, (name[1:] + "_Len"))
                self.max_length = _FindLength(self.identifier.meta.maxlength, (name[1:] + "_MaxLen"))
                self.is_buffer = ((self.length or self.max_length) and is_buffer and not self.is_array)

                if ((self.length or self.max_length) and is_array_pointer and not is_buffer):
                    raise TypenameError(self.identifier, "'%s': variable-length arrays are not supported (use std::vector instead)" % self.trace_proto)

                if not self.is_array:
                    if (is_array_pointer and not self.length and self.is_input):
                        raise TypenameError(self.identifier, "'%s': an outbound buffer requires a @length tag" % self.trace_proto)

                    if (is_array_pointer and (not self.length and not self.max_length) and self.is_output):
                        raise TypenameError(self.identifier, "'%s': an inbound-only buffer requires a @maxlength tag" % self.trace_proto)

                if self.length and not self.is_buffer:
                    raise TypenameError(self.identifier, "'%s': @length tag only allowed for raw buffers" % self.trace_proto)

                if self.max_length and not self.is_buffer:
                    raise TypenameError(self.identifier, "'%s': @maxlength tag only allowed for raw buffers" % self.trace_proto)

                if self.is_array and not self.length:
                    self.length = _FindLength([str(self.identifier.array)], (name[1:] + "FixedLen"))

                    if is_buffer:
                        # array of bytes, let's make it a buffer then
                        self.is_array = False
                        self.is_buffer = True

                    self.max_length = self.length

                # If have a input length, assume it's a max-length parameter even if not stated explicitly
                if ((self.is_buffer or self.is_array) and not self.max_length and self.length.is_input):
                    self.max_length = self.length

                    if self.is_input and self.is_output:
                        log.WarnLine(self.identifier, "'%s': maximum length of this inbound/outbound buffer is assumed to be same as @length, use @maxlength to disambiguate" % self.trace_proto)
                    elif self.is_output_only:
                        log.InfoLine(self.identifier, "'%s': @maxlength not specified for this inbound buffer, assuming same as @length" % self.trace_proto)

                if ((self.is_buffer or self.is_array) and self.is_output and not self.max_length):
                    raise TypenameError(self.identifier, "'%s': can't deduce maximum length of this inbound buffer, use @maxlength" % self.trace_proto)

                if ((self.is_buffer or self.is_array) and self.is_output and self.max_length and not self.length):
                    if self.is_input:
                        log.WarnLine(self.identifier, "'%s': length of this inbound buffer is not specified; using @maxlength, but this may be inefficient" % self.trace_proto)
                    self.length = self.max_length

                # Is it a hresult?
                self.is_hresult = self.identifier_type.TypeName().endswith("Core::hresult") \
                                    or (isinstance(self.kind, CppParser.Integer) and self.kind.size == "long" and interface.obj.is_json)

                # Is it a POD?
                # If forward class definition is encountered it will be assumed it's an interface not a POD
                self.is_compound = (is_class and not self.kind.IsVirtual() and (len(self.kind.vars) > 0))

                # Is it an interface instance?
                is_unspecified_interface_instance = (isinstance(self.kind, CppParser.Void) and self.identifier.meta.interface)
                has_proxy = not self.is_compound and (is_interface and self.type.IsPointer()) and self.is_input
                has_output_proxy = not self.is_compound and \
                    (self.is_output and self.type.IsPointer() and (is_interface or is_unspecified_interface_instance))

                self.proxy = None
                self.return_proxy = False
                self.proxy_instance = None

                self.type_name = Flatten(self.identifier_type.TypeName(), ns)
                self.target_type_name = Flatten(self.type.TypeName(), ns)
                self.proto = Flatten(self.identifier_type.Proto("noref"), ns)
                self.proto_no_cv = Flatten(self.identifier_type.Proto("nocv|noref"), ns)
                self.peek_length = None

                self.name = name

                self.iterator_optimization = self.is_iterator and ENABLE_ITERATOR_OPTIMIZATION and not "sequential-iterator" in self.identifier.meta.decorators

                if has_proxy and not self.iterator_optimization:
                    # Have to use instance_id instead of the class name
                    self.proto_weak = (("const " if self.is_const else "") + INSTANCE_ID).strip()
                    self.proto_weak_no_cv = INSTANCE_ID
                    self.proxy = AuxIdentifier(CppParser.Class(CppParser.Locate("ProxyStub"), "UnknownProxy"), CppParser.Ref.POINTER, ("%sProxy__" % name))
                    self.proxy_instance = AuxIdentifier(CppParser.InstanceId(), CppParser.Ref.VALUE, ("%sInstanceId__" % name))
                else:
                    self.proto_weak = self.proto
                    self.proto_weak_no_cv = self.proto_no_cv

                if has_output_proxy and not self.iterator_optimization:
                    self.return_proxy = True

                if has_output_proxy or has_proxy:
                    if not self.interface_id:
                        # Pick up interface ID from the type itself
                        _ref = (CppParser.Ref.VALUE | CppParser.Ref.CONST)
                        self.interface_id = AuxIdentifier(CppParser.Integer("uint32_t"), _ref, "id", (self.type_name + "::ID"))

                # Used with PODs
                self.suppress_type = suppress_type

                # Now do some more sanity checking on what we've got!...
                if not is_return_value and (self.is_output and self.return_proxy and not self.type.IsReference() and not parent):
                    raise TypenameError(self.identifier, "'%s': output interface must be a reference to a pointer" % self.trace_proto)

                if self.length_of and not no_length_warnings:
                    if not isinstance(self.kind, CppParser.Integer) or (self.is_buffer or self.is_array):
                        raise TypenameError(self.identifier, "'%s': this type cannot be a buffer length carrying parameter" % self.trace_proto)
                    if self.kind.size == "long long":
                        raise TypenameError(self.identifier, "'%s': 64-bit buffer length carrying parameter is not supported" % self.trace_proto)
                    elif self.kind.size == "long" and not self.identifier.meta.range.is_set:
                        log.WarnLine(self.identifier, "'%s': long int buffer length is supported, but buffers up to %s bytes are recommended" \
                                        % (self.trace_proto, PARAMETER_SIZE_WARNING_THRESHOLD))
                    if not self.kind.fixed:
                        raise TypenameError(self.identifier, "'%s': length parameter is not fixed-width integer, use a stdint type" % self.trace_proto)
                    if self.kind.signed:
                        log.WarnLine(self.identifier, "'%s': buffer length is a signed integer" % self.trace_proto)

                if self.max_length_of and (not self.length or (self.max_length.as_rvalue != self.length.as_rvalue)) and not no_length_warnings:
                    if not self.is_input:
                        raise TypenameError(self.identifier, "'%s': a max-length parameter must not be write-only" % self.trace_proto)
                    elif not isinstance(self.kind, CppParser.Integer) or (self.is_buffer or self.is_array):
                        raise TypenameError(self.identifier, "'%s': this type cannot be a buffer max-length carrying parameter" % self.trace_proto)
                    if self.kind.size == "long long":
                        raise TypenameError(self.identifier, "'%s': 64-bit buffer max-length carrying parameter is not supported" % self.trace_proto)
                    elif self.kind.size == "long" and not self.identifier.meta.range.is_set:
                        log.WarnLine(self.identifier, "'%s': long int buffer max-length is supported, but buffers up to %s bytes are recommended" \
                                            % (self.trace_proto, PARAMETER_SIZE_WARNING_THRESHOLD))
                    if not self.kind.fixed:
                        raise TypenameError(self.identifier, "'%s': max-length parameter is not fixed-width integer, use a stdint type" % self.trace_proto)
                    if self.kind.signed:
                        log.WarnLine(self.identifier, "'%s': buffer max-length is a signed integer" % self.trace_proto)

                if self.type.IsReference():
                    if not self.identifier_type.IsConst() and not self.is_output:
                        raise TypenameError(self.identifier, "'%s': non-const reference requires an @out tag" % self.trace_proto)
                    elif self.identifier_type.IsConst() and self.is_output:
                        raise TypenameError(identifier, "'%s': output parameter must not be const" % self.trace_proto)
                    if not self.identifier_type.IsConst() and self.is_input_only:
                        log.WarnLine(identifier, "'%s': read-only parameter is missing a const qualifier" % self.trace_proto)

                if ((self.is_buffer or self.is_array) and not in_pod):
                    if not self.identifier_type.IsPointerToConst() and not self.identifier.meta.input and not self.identifier.meta.output:
                        raise TypenameError(self.identifier, "'%s': pointer to non-const data requires an @in or @out tag" % self.trace_proto)
                    elif not self.identifier_type.IsPointerToConst() and self.is_input_only:
                        log.WarnLine(identifier, "'%s': read-only parameter is missing a const qualifier" % self.trace_proto)
                    elif self.identifier_type.IsPointerToConst() and self.is_output:
                        raise TypenameError(identifier, "'%s': output parameter must not be const" % self.trace_proto)

                if (not self.is_buffer and not self.is_array) and isinstance(self.kind, (CppParser.Integer, CppParser.BuiltinInteger)):
                    if not self.kind.IsFixed():
                        log.WarnLine(self.identifier, "'%s': integer is not fixed-width, use a stdint type" % self.trace_proto)

                if isinstance(self.kind, CppParser.Enum):
                    if not self.kind.type.Type().IsFixed():
                        log.WarnLine(self.identifier, "'%s': underlying type of enumeration is not fixed-width integer, use a stdint type" % self.trace_proto)

                # Lastly handle restrict
                self.restrict_range = identifier.meta.range

                if not self.restrict_range.is_set:
                    if self.length and self.length.identifier.meta.range:
                        self.restrict_range = self.length.identifier.meta.range
                    elif self.max_length and self.max_length.identifier.meta.range:
                        self.restrict_range = self.max_length.identifier.meta.range
                    elif self.length_of and self.length_of.meta.range:
                        self.restrict_range = self.length_of.meta.range
                    elif self.max_length_of and self.max_length_of.meta.range:
                        self.restrict_range = self.max_length_of.meta.range

                if self.restrict_range.is_set:
                    if self.is_integer and not self.is_buffer:
                        if not no_length_warnings and self.restrict_range.has_max:
                            if (((self.restrict_range.max < 256) and (self.kind.max > 256)) \
                                    or ((self.restrict_range.max < (64*1024)) and (self.kind.max > (64*1024))) \
                                    or ((self.restrict_range.max < (4*1024*1024*1024)) and (self.kind.max > (4*1024*1024*1024)))):
                                log.WarnLine(identifier, "'%s': inefficient use of type (%s) as value restricted to range (%s)" % (self.trace_proto, self.proto_no_cv, self.restrict_range))

                    elif (self.is_buffer or self.is_string or (self.is_integer and self.kind.min == 0)):
                        if self.restrict_range.has_max and self.restrict_range.max > (PARAMETER_SIZE_WARNING_THRESHOLD):
                            log.WarnLine(identifier, "'%s': parameters up to %s bytes are recommended for COM-RPC, see range (%s)" \
                                            % (self.trace_proto, PARAMETER_SIZE_WARNING_THRESHOLD, self.restrict_range))

                if isinstance(self.kind, CppParser.DynamicArray):
                    aux_size = "uint16_t"
                    if self.restrict_range.has_max:
                        if self.restrict_range.max < 256:
                            aux_size = "uint8_t"
                        elif self.restrict_range.max > 65535:
                            aux_size = "uint32_t"

                    self.length = CppParser.Temporary(self.kind.element.parent, [aux_size])
                    self.length.type_name = aux_size
                    self.element = self.kind.element
                    self.is_dynamic_array = True

                if self.is_iterator:
                    aux_size = "uint16_t"
                    if self.restrict_range.has_max:
                        if self.restrict_range.max < 256:
                            aux_size = "uint8_t"
                        elif self.restrict_range.max > 65535:
                            aux_size = "uint32_t"

                    self.length = CppParser.Temporary(self.kind, [aux_size])
                    self.length.type_name = aux_size
                    self.element = self.kind.resolvedArgs[0]

                if self.is_string or self.is_buffer or self.is_array or self.is_dynamic_array:
                    aux_name = Normalize(self.name + "PeekedLen__")
                    aux_size = "uint16_t"

                    if self.restrict_range.has_max:
                        if self.restrict_range.max >= (16*1024*1024):
                            aux_size = "uint32_t"
                        elif self.restrict_range.max >= (64*1024):
                            aux_size = "Core::UInt24"
                        elif self.restrict_range.max < 256:
                            aux_size = "uint8_t"

                    if self.is_string:
                        if self.is_ccstring and not self.restrict_range.is_set:
                            log.WarnLine(identifier, "'%s': parameters up to %s bytes are recommended for COM-RPC" \
                                            % (self.trace_proto, PARAMETER_SIZE_WARNING_THRESHOLD))
                            aux_size = "uint32_t"

                        self.peek_length = AuxIdentifier(CppParser.Integer(aux_size), (CppParser.Ref.VALUE | CppParser.Ref.CONST), aux_name)

                    elif self.is_buffer or self.is_array or self.is_dynamic_array:
                        if not self.restrict_range.is_set:
                            aux_size = (self.length.type_name if self.length else self.max_length.type_name)

                        self.peek_length = AuxIdentifier(CppParser.Integer(aux_size), (CppParser.Ref.VALUE | CppParser.Ref.CONST), aux_name)

                elif self.is_integer:
                    # Opportunity to shrink DWORD to 24-bit if restrict range fits
                    if self.kind.size == "long":
                        if self.kind.signed:
                            if self.restrict_range.has_min and self.restrict_range.has_max:
                                if ((self.restrict_range.min < (-32*1024)) and (self.restrict_range.min >= (-8*1024*1024))) or \
                                    ((self.restrict_range.max >= (32*1024)) and (self.restrict_range.max < (8*1024*1024))):
                                    self.type_name = "Core::SInt24"
                        else:
                            if (self.restrict_range.has_max and ((self.restrict_range.max >= (64*1024)) and (self.restrict_range.max < (16*1024*1024)))):
                                self.type_name = "Core::UInt24"

                if isinstance(self.kind, CppParser.Optional):
                    #if self.kind.optional.type.IsPointer() and not isinstance(self.kind.optional.type.Resolve().type, CppParser.Class) and not:
                    #    raise TypenameError(identifier, "'%s': raw buffer must not be OptionalType" % self.trace_proto)

                    self.kind.optional.meta = identifier.meta

                    #if not self.is_array:
                    #    self.kind.optional.type.ref |= identifier.type.ref

                    self.optional = EmitOptional(self.interface, self.kind.optional, self.name, parent=self)
                    self.is_compound = self.optional.is_compound
                    self.is_dynamic_array = self.optional.is_dynamic_array
                    self.is_buffer = self.optional.is_buffer
                    self.is_string = self.optional.is_string
                    self.is_iterator = self.optional.is_iterator
                    self.optional.suppress_type = self.suppress_type

                    if not self.length:
                        self.length = self.optional.length
                    if not self.max_length:
                        self.max_length = self.optional.max_length
                    if not self.peek_length:
                        self.peek_length = self.optional.peek_length
                    if not self.item:
                        self.item = self.optional.item
                    if not self.element:
                        self.element = self.optional.element

                    if self.is_compound:
                        if isinstance(self.optional.kind, CppParser.Optional):
                             raise TypenameError(identifier, "'%s': OptionalType of OptionalType is not supported" % self.trace_proto)

                        self.compound_merged = self.optional.kind.Merge()

                    if self.optional.proxy or self.optional.return_proxy:
                        self.name = self.optional.name
                        self.proxy = self.optional.proxy
                        self.proxy_instance = self.optional.proxy_instance
                        self.interface_id = self.optional.interface_id
                        self.return_proxy = self.optional.return_proxy
                        self.proto_weak = self.optional.proto_weak
                        self.proto_weak_no_cv = self.optional.proto_weak_no_cv

                else:
                    self.optional = None

                    if self.is_compound:
                        self.compound_merged = self.kind.Merge()

                if isinstance(self.kind, CppParser.Fundamental) and self.type.IsReference() and self.is_input_only and not isinstance(self, EmitOptional):
                    log.WarnLine(self.identifier, "%s: input-only fundamental type passed by reference" % self.trace_proto)

                if not is_return_value and ((self.is_dynamic_array or self.is_compound or self.optional) and not self.is_array) \
                        and not self.type.IsReference() and (not parent or not isinstance(parent.kind, (CppParser.DynamicArray, CppParser.Optional, CppParser.Class))):
                    log.WarnLine(self.identifier, "%s: object passed by value" % self.trace_proto)

            def __maybe_const_cast(self, expr):
                if not self.value and self.type.IsReference():
                    if (self.proxy or self.is_iterator) and self.type.IsConstPointer():
                        return "static_cast<%s%s* const&>(%s)" % (("const " if self.type.IsPointerToConst() else ""), self.type_name, expr)
                    elif self.identifier_type.IsConst():
                        return "static_cast<const %s&>(%s)" % (self.type_name, expr)
                return expr

            @property
            def as_rvalue(self):
                if self.optional:
                    return self.optional.as_rvalue
                else:
                    return (self.value if self.value else (self.name + self.suffix))

            @property
            def as_pure_rvalue(self):
                return (self.value if self.value else self.__maybe_const_cast(self.name))

            @property
            def as_lvalue(self):
                return (self.proxy_instance.as_lvalue if self.proxy_instance else self.name)

            @property
            def pure_proto(self):
                return (self.proto_no_cv if not self.optional else self.optional.proto_no_cv)

            @property
            def temporary(self):
                if self.is_array:
                    assert self.length
                    return ("%s%s%s" % (((self.type_name + " ") if not self.suppress_type else ""), self.name, ("[" + str(self.length.as_rvalue) + "]") if not self.suppress_type else ""))
                else:
                    return ("%s%s" % (((self.proxy_instance.proto if self.proxy_instance else self.proto) + " ") if not self.suppress_type else "", self.as_lvalue))

            @property
            def temporary_no_cv(self):
                if self.is_array:
                    assert self.length
                    return ("%s%s%s" % (((self.type_name + " ") if not self.suppress_type else ""), self.name, ("[" + str(self.length.as_rvalue) + "]") if not self.suppress_type else ""))
                else:
                    return ("%s%s" % (((self.proxy_instance.proto_no_cv if self.proxy_instance else self.proto_no_cv) + " ") if not self.suppress_type else "", self.as_lvalue))

            @property
            def read_rpc_type(self):
                if isinstance(self.kind, CppParser.Optional):
                    return self.optional.read_rpc_type

                # Interfaces
                elif self.proxy_instance or self.return_proxy:
                    return "Number<%s>()" % INSTANCE_ID

                # Raw buffers
                elif self.is_buffer:
                    assert self.length or self.max_length, "Invalid type for buffer"
                    if self.max_length:
                        return "Buffer<%s>(%s, %s)" % (self.max_length.type_name, self.max_length.as_rvalue, self.as_lvalue)
                    elif self.length:
                        return "Buffer<%s>(%s, %s)" % (self.length.type_name, self.length.as_rvalue, self.as_lvalue)
                    else:
                        Unreachable()

                # Strings
                elif self.is_string:
                    return ("Text<%s>()" % self.peek_length.proto_no_cv) if self.peek_length.proto_no_cv != "uint16_t" else "Text()"

                # MacAddress
                elif isinstance(self.kind, CppParser.MacAddress):
                    return "Buffer<uint8_t>(6, %s)" % self.as_lvalue

                # The integral types
                elif isinstance(self.kind, (CppParser.Integer, CppParser.BuiltinInteger, CppParser.Enum)):
                    return "Number<%s>()" % self.type_name
                elif isinstance(self.kind, CppParser.Bool):
                    return "Boolean()"
                elif isinstance(self.kind, CppParser.Time):
                    return "Number<%s>()" % self.target_type_name

                # Floating point types
                elif isinstance(self.kind, CppParser.Float):
                    if "long" not in self.type_name:
                        return "Number<%s>()" % self.type_name
                    else:
                        raise TypenameError(self.identifier, "long double type is not supported (see '%s')" % (self.trace_proto))

                elif isinstance(self.kind, CppParser.DynamicArray):
                    return self.item.read_rpc_type

                else:
                    raise TypenameError(self.identifier, "%s: unable to deserialise this type (see '%s')" % (self.type_name, self.trace_proto))

            @property
            def write_rpc_type(self):
                if self.optional:
                    return self.optional.write_rpc_type

                # Interfaces
                elif self.proxy_instance or self.return_proxy:
                    return "Number<%s>(RPC::instance_cast(%s))" % (INSTANCE_ID, self.as_rvalue)

                # Raw buffers
                elif self.is_buffer:
                    assert self.max_length, "Invalid type for buffer " + self.name
                    return "Buffer<%s>(%s, %s)" % (self.length.type_name, self.length.as_rvalue, self.as_rvalue)

                # Strings
                elif self.is_string:
                    return ("Text<%s>(%s)" % (self.peek_length.proto_no_cv, self.as_rvalue)) if self.peek_length.proto_no_cv != "uint16_t" else ("Text(%s)" % (self.as_rvalue))

                # MacAddress
                elif isinstance(self.kind, CppParser.MacAddress):
                    return "Buffer<uint8_t>(6, %s)" % self.as_lvalue

                # The integral types
                elif isinstance(self.kind, (CppParser.Integer, CppParser.Enum, CppParser.BuiltinInteger)):
                    return "Number<%s>(%s)" % (self.type_name, self.as_rvalue)
                elif isinstance(self.kind, CppParser.Bool):
                    return "Boolean(%s)" % self.as_rvalue
                elif isinstance(self.kind, CppParser.Time):
                    return "Number<%s>(%s.Ticks())" % (self.target_type_name, self.as_rvalue)

                # Floating point types
                elif isinstance(self.kind, CppParser.Float):
                    if "long" not in self.type_name:
                        return "Number<%s>(%s)" % (self.type_name, self.as_rvalue)
                    else:
                        raise TypenameError(self.identifier, "long double is not supported (see '%s')" % (self.trace_proto))

                elif isinstance(self.kind, CppParser.DynamicArray):
                    return self.item.write_rpc_type

                else:
                    raise TypenameError(self.identifier, "%s: sorry, unable to serialise this type (see '%s')" % (self.type_name, self.trace_proto))

            @property
            def storage_size(self):
                if isinstance(self.kind, CppParser.Optional):
                    assert self.optional
                    return "1"
                elif self.proxy_instance or self.return_proxy:
                    return "sizeof(%s)" % INSTANCE_ID
                elif self.is_buffer:
                    return "Core::RealSize<%s>()" % self.peek_length.type_name
                elif self.is_string:
                    if self.is_ccstring:
                        return "(sizeof(uint32_t))"
                    else:
                        return "Core::RealSize<%s>()" % self.peek_length.type_name
                elif isinstance(self.kind, (CppParser.Integer, CppParser.Float, CppParser.Enum, CppParser.BuiltinInteger)):
                    return "Core::RealSize<%s>()" % self.type_name
                elif isinstance(self.kind, CppParser.Bool):
                    return "1" # always one byte
                elif isinstance(self.kind, CppParser.MacAddress):
                    return "6" # always six bytes
                elif isinstance(self.kind, CppParser.DynamicArray):
                    return "Core::RealSize<%s>()" % self.peek_length.type_name
                elif isinstance(self.kind, CppParser.Time):
                    return "sizeof(%s)" % self.target_type_name
                else:
                    Unreachable()

            def __str__(self):
                return str(self.type)

            def __repr__(self):
                return str(self.type)

        class EmitParam(EmitIdentifier):
            def __init__(self, interface, identifier, name=None, suppress_type=False, suffix="", parent=None):
                EmitIdentifier.__init__(self, interface, identifier, name, suppress_type, "_", suffix, parent)

        class EmitRetVal(EmitIdentifier):
            def __init__(self, interface, identifier, name="_result", suppress_type=False):
                EmitParam.__init__(self, interface, identifier, name, suppress_type)

        class EmitLength(EmitIdentifier):
            def __init__(self, interface, identifier, name=None, suppress_type=False, suffix="", restrict_range=None):
                EmitIdentifier.__init__(self, interface, identifier, name, suppress_type, "", suffix)
                if restrict_range:
                    self.restrict_range = restrict_range

        class EmitOptional(EmitIdentifier):
            def __init__(self, interface, identifier, name=None, suppress_type=False, parent=None):
                EmitIdentifier.__init__(self, interface, identifier, name, suppress_type, "_", ".Value()", parent)


        def EmitFunctionOrder(methods):
            if methods:
                emit.Line("// Methods:")

                for index, method in enumerate(methods):
                    emit.Line("//  (%i) %s" % (index, Flatten(method.Proto(), ns)))

                emit.Line("//")

        def PrepareParams(method, interface):
            input_params = []
            proxy_params = []
            return_proxy_params = []
            output_params = []
            retval = None
            params = []

            if method.retval:
                if isinstance(method.retval, list) or isinstance(method.retval.type, list) or not method.retval.type.IsVoid():
                    retval = EmitRetVal(interface, method.retval, vars["result"])

            for index, var in enumerate(method.vars):
                params.append(EmitParam(interface, var))
                if var.meta.length and var.meta.length[0] == "return":
                    retval.is_on_wire = False

            if retval:
                output_params.append(retval)
                if retval.return_proxy:
                    return_proxy_params.append(retval)
            else:
                output_params.append(None)

            for p in params:
                # Length parameters are never sent in, max-length only if a buffer is output only
                if ((p.is_input or not p.is_output) and (not p.length_of or (p.max_length_of and not p.max_length_of.meta.input))):
                    input_params.append(p)
                    if p.proxy or (p.optional and p.optional.proxy):
                        proxy_params.append(p)

                # Length and max-length variables are never sent back
                if (p.is_output and (not p.length_of and not p.max_length_of)):
                    output_params.append(p)
                    if p.return_proxy or (p.optional and p.optional.return_proxy):
                        return_proxy_params.append(p)

            def _PrepareDebug(params, output):
                for p in params:
                    if p:
                        ch = []
                        if p.optional:
                            ch.append("opt")
                        if p.is_iterator:
                            ch.append("iter")
                        if p.proxy:
                            ch.append("proxy")
                        elif p.return_proxy:
                            ch.append("retproxy")
                        elif p.is_compound:
                            ch.append("pod")

                        if p.restrict_range.is_set:
                            ch.append("(%s)" % str(p.restrict_range))

                        chs = " [" + ", ".join(ch) + "]" if ch else ""

                        output.append(p.name + chs)
                    else:
                        output.append("void")

            _debug_input_params = []
            _debug_output_params = []

            _PrepareDebug(input_params, _debug_input_params)
            _PrepareDebug(output_params, _debug_output_params)

            log.Info("  %s, input = (%s), output = (%s)" % (method.name, ", ".join(_debug_input_params), ", ".join(_debug_output_params)))

            return retval, params, input_params, output_params, proxy_params, return_proxy_params

        def CheckFrame(p, by_parameter=False):
            if ENABLE_INTEGRITY_VERIFICATION:
                emit.Line("if (%s.Length() < (%s)) { return (COM_ERROR | Core::ERROR_READ_ERROR); }" % \
                            (vars["reader"], (p.as_rvalue if by_parameter else p.storage_size)))

        def CheckRange(p, val):
            if p.restrict_range.is_set:
                cmp = val if isinstance(val, str) else val.as_pure_rvalue

                restrictions = []

                if p.restrict_range.has_min:
                    restrictions.append("(%s >= %s)" % (cmp, p.restrict_range.min))
                if p.restrict_range.has_max:
                    restrictions.append("(%s <= %s)" % (cmp, p.restrict_range.max))

                emit.Line("ASSERT(%s);" % " && ".join(restrictions))

                if ENABLE_RANGE_VERIFICATION:
                    emit.Line("if (!(%s)) { return (COM_ERROR | Core::ERROR_INVALID_RANGE); }" % " && ".join(restrictions))
                    print("e")

        def CheckSize(p):
            if ENABLE_INTEGRITY_VERIFICATION:
                emit.Line("%s = %s.PeekNumber<%s>();" % (p.peek_length.temporary, vars["reader"], p.peek_length.type_name))
                CheckRange(p, p.peek_length)
                emit.Line("if (%s.Length() < (static_cast<uint32_t>(%s) + %s)) { return (COM_ERROR | Core::ERROR_READ_ERROR); }" % \
                            (vars["reader"], p.storage_size, p.peek_length.as_rvalue))
            else:
                CheckRange(p, "%s.PeekNumber<%s>()" % (vars["reader"], p.peek_length.type_name))

        # Build the announce list upfront
        for interface in interfaces:
            obj = interface.obj
            interface_name = Flatten(obj.full_name, ns)

            log.Info("Processing '%s' interface data..." % Flatten(obj.type, ns))

            if not obj.full_name.startswith(ns + "::"):
                log.Info("class %s not in requested namespace (%s)" % (obj.full_name, ns))
            elif obj.omit:
                omit_interface_used = True
                log.Info("omitting class %s" % interface_name)
            else:
                # Of course consider only virtual methods
                emit_methods = [method for method in obj.methods if method.IsVirtual() and not method.IsDestructor() and not method.omit]
                omitted_methods = [method for method in obj.methods if method.IsVirtual() and not method.IsDestructor() and method.omit]

                for method in omitted_methods:
                    log.Info("omitting method %s::%s()" % (interface_name, method.name))

                if not emit_methods:
                    log.Info("no virtual methods in class %s" % interface_name)
                else:
                    stub_methods_name = CreateName(interface_name) + "StubMethods"
                    proxy_name = CreateName(interface_name) + "Proxy"
                    stub_name = CreateName(interface_name) + "Stub"
                    prepared_params = [PrepareParams(method, interface) for method in emit_methods]
                    announce_list[interface_name] = [emit_methods, stub_methods_name, stub_name, proxy_name, interface, prepared_params]

        #
        # EMIT STUB CODE
        #
        def EmitStubMethodImplementation(index, method, interface_name, interface, retval, params, \
                                        input_params, output_params, proxy_params, return_proxy_params):

            hresult = AuxIdentifier(CppParser.Integer(HRESULT), CppParser.Ref.VALUE, vars["hresult"])

            has_restricted_parameters = any([v.restrict_range.is_set for v in input_params])

            output_buffers = []
            custom_buffers = []
            has_hresult = retval and retval.is_hresult

            # For stubs
            def ReadParameter(p, no_array=False):
                assert p.is_on_wire

                def _EmitAssignment(p):
                    param_ = "%s = %s.%s;" % (p.as_lvalue if p.optional else p.temporary, vars["reader"], p.read_rpc_type)

                    if p.is_string:
                        CheckFrame(p)
                        CheckSize(p)
                        emit.Line(param_)
                    elif p.is_iterator:
                        CheckFrame(p)
                        emit.Line(param_)
                    else:
                        CheckFrame(p)
                        emit.Line(param_)
                        CheckRange(p, p)

                if p.optional and (not p.is_array or no_array):
                    if not p.suppress_type:
                        emit.Line("%s{};" % p.temporary_no_cv)

                    CheckFrame(p)
                    emit.Line("if (%s.Boolean() == true) {" % vars["reader"])
                    emit.IndentInc()
                else:
                    if not p.suppress_type and (p.is_compound or p.is_array) and not no_array:
                        emit.Line("%s{};" % p.temporary_no_cv)

                if p.is_iterator:
                    iterator = p.optional if p.optional else p
                    if iterator.iterator_optimization:
                        name = Normalize(iterator.name)
                        count = name + "Count"
                        type = iterator.kind.resolvedArgs[0]
                        internal = name + "List"

                        if not p.optional:
                            emit.Line("%s{};" % p.temporary_no_cv)

                        length = EmitLength(interface, iterator.length, count, restrict_range=p.restrict_range)
                        element = EmitParam(interface, iterator.element, Normalize(name + "Item"), parent=iterator)
                        ReadParameter(length)

                        emit.Line("if ((%s != 0) || (%s.Boolean() == true)) {" % (count, vars["reader"]))
                        emit.IndentInc()

                        emit.Line("std::list<%s> %s;" % (type, internal))
                        # emit.Line("%s.reserve(%s);" % (internal, length.as_rvalue))

                        emit.Line("while (%s-- != 0) {" % count)
                        emit.IndentInc()
                        ReadParameter(element)
                        emit.Line("%s.push_back(std::move(%s));" % (internal, element.as_rvalue))
                        emit.IndentDec()
                        emit.Line("}")

                        impl = "::Thunder::RPC::IteratorType<%s>" % iterator.kind.type
                        emit.Line("%s = Core::ServiceType<%s>::Create<%s>(std::move(%s));" % (p.as_lvalue, impl, iterator.kind.type, internal))
                        emit.Line("ASSERT(%s != nullptr);" % (p.as_rvalue))

                        emit.IndentDec()
                        emit.Line("}")
                    else:
                        _EmitAssignment(p)

                # Fixed array
                elif p.is_array and not no_array:
                    index = chr(ord('i') + p.name.count('['))
                    element = EmitParam(interface, p.identifier, "%s[%s]" % (p.name, index), suppress_type=True)
                    emit.Line("for (uint16_t %s = 0; %s < %s; %s++) {" % (index, index, p.length.as_rvalue, index))
                    emit.IndentInc()
                    ReadParameter(element, no_array=True)
                    emit.IndentDec()
                    emit.Line("}")

                # Vector
                elif p.is_dynamic_array:
                    obj_name = p.name

                    if not p.suppress_type:
                        if p.optional:
                            obj_name = Normalize(obj_name + "Object__")
                            emit.Line("%s %s{};" % (p.optional.type_name, obj_name))
                        else:
                            emit.Line("%s %s{};" % (p.type_name, obj_name))

                    length = EmitParam(interface, p.length, Normalize("%sSize" % obj_name))
                    ReadParameter(length)
                    CheckRange(p, ("%s" % length.as_rvalue))
                    emit.Line("%s.reserve(%s);" % (p.as_rvalue, length.as_rvalue))

                    index = chr(ord('i') + p.name.count('Item'))
                    element = EmitParam(interface, p.element, Normalize(obj_name + "Item"), parent=p)
                    emit.Line("for (%s %s = 0; %s < %s; %s++) {" % (p.length.type_name, index, index, length.as_rvalue, index))
                    emit.IndentInc()
                    ReadParameter(element)
                    emit.Line("%s.push_back(std::move(%s));" % (p.as_rvalue, element.as_rvalue))
                    emit.IndentDec()
                    emit.Line("}")

                    if p.optional:
                        emit.Line("%s = std::move(%s);" % (p.name, obj_name))

                # POD
                elif p.is_compound:
                    obj_name = p.name

                    if p.optional:
                        obj_name = Normalize(p.name + "Object__")
                        emit.Line("%s %s{};" % (p.optional.type_name, obj_name))

                    params = [EmitParam(interface, v, (obj_name + "." + v.name), suppress_type=True, parent=p) for v in p.compound_merged.vars]

                    for pp in params:
                        ReadParameter(pp)
                        if pp.proxy:
                            proxy_params.append(pp)

                    if p.optional:
                        emit.Line("%s = std::move(%s);" % (p.name, obj_name))

                elif p.is_buffer:
                    CheckFrame(p)
                    CheckSize(p)

                    if not p.suppress_type:
                        emit.Line("%s{};" % p.temporary_no_cv)

                    _name = p.as_rvalue

                    buffer_param = "const_cast<const %s*&>(%s)" % (p.type_name, _name) if not p.identifier_type.IsPointerToConst() else _name
                    emit.Line("%s = %s.LockBuffer<%s>(%s);" % (p.length.temporary, vars["reader"], p.length.type_name, buffer_param))
                    emit.Line("%s.UnlockBuffer(%s);" % (vars["reader"], p.length.name))

                    if p.is_array:
                        emit.Line("ASSERT(%s == (%s * sizeof(%s)));" % (p.length.name, p.identifier.array, p.type_name))

                elif isinstance(p.kind, CppParser.MacAddress):

                    if not p.suppress_type:
                        length = AuxIdentifier(CppParser.Integer("uint8_t"), CppParser.Ref.VALUE | CppParser.Ref.CONST, (p.name[1:] + "Length"))
                        buffer = AuxIdentifier(CppParser.Integer("uint8_t"), CppParser.Ref.POINTER | CppParser.Ref.POINTER_TO_CONST, (p.name[1:] + "Buffer"))
                        emit.Line("%s{};" % buffer.temporary)

                        emit.Line("%s = %s.LockBuffer<uint8_t>(%s);" % (length.temporary, vars["reader"], buffer.name))
                        emit.Line("ASSERT(%s == 6);" % (length.name))
                        emit.Line("%s.UnlockBuffer(%s);" % (vars["reader"], length.name))

                        emit.Line("%s{%s};" % (p.temporary_no_cv, buffer.as_rvalue))

                else:
                    _EmitAssignment(p)

                if p.optional and (not p.is_array or no_array):
                    emit.IndentDec()
                    emit.Line("}")

            def TemporaryParameter(p):
                assert p

                if p.is_buffer:
                    assert p.length
                    output_buffers.append(p)
                    if p.length.is_output_only and not p.length.is_return_value:
                        emit.Line("%s{};" % p.length.temporary_no_cv)

                if (p.is_output and not p.is_input):
                    emit.Line("%s{};" % p.temporary_no_cv)
                    return 1

                return 0

            def AllocateBuffer(p, no_check=False):
                assert p
                assert p.is_buffer
                assert p.max_length

                has_same_buffer = (p.is_input and p.max_length and p.length and (p.max_length.as_rvalue == p.length.as_rvalue)) and not no_check

                if has_same_buffer:
                    return

                # May need to allocate custom memory to pass large buffer
                has_large_buffer_size = (p.max_length and (p.max_length.type.Type().size == "long"))
                has_large_buffer = (has_large_buffer_size and ENABLE_CUSTOM_ALLOCATOR)
                has_buffer_reuse = (p.is_input and p.max_length and p.length and (p.max_length.as_rvalue != p.length.as_rvalue))

                if has_large_buffer:
                    large_buffer = AuxIdentifier(CppParser.Void(), CppParser.Ref.POINTER, (p.name[1:] + "Custom"))
                    emit.Line("%s{};" % large_buffer.temporary_no_cv)

                if not p.max_length.value:
                    emit.Line("if (%s != 0) {" % p.max_length.as_rvalue)
                    emit.IndentInc()

                if has_buffer_reuse:
                    temp_buffer = AuxIdentifier(CppParser.Void(), CppParser.Ref.POINTER, vars["tempbuffer"])

                    if (p.max_length.as_rvalue != p.length.as_rvalue):
                        emit.Line("if (%s > %s) {" % (p.max_length.as_rvalue, p.length.as_rvalue))
                        emit.IndentInc()

                    emit.Line("%s{};" % temp_buffer.temporary_no_cv)
                    emit.Line()

                if has_large_buffer:
                    custom_buffers.append([p, large_buffer])
                    emit.Line("%s = RPC::Administrator::Instance().Allocate(%s);" % (large_buffer.as_rvalue, p.max_length.as_rvalue))
                    emit.Line()
                    emit.Line("if (%s == nullptr) {" % large_buffer.as_rvalue)
                    emit.IndentInc()

                if has_large_buffer or (not ENABLE_CUSTOM_ALLOCATOR and has_large_buffer_size):
                    emit.Line("ASSERT(%s <= 0x1000000);" % p.max_length.as_rvalue)

                    if ENABLE_RANGE_VERIFICATION:
                        emit.Line("if (%s > 0x1000000) { return (Core::ERROR_BAD_REQUEST); }" % p.max_length.as_rvalue)
                        emit.Line()

                if EMIT_TRACES:
                    emit.Line('fprintf(stderr, "*** Allocating %%u bytes on stack for a temporary buffer\\n", %s);' % p.max_length.as_rvalue)

                emit.Line("%s = static_cast<%s>(ALLOCA(%s * sizeof(%s)));" % \
                    ((p.as_rvalue if not has_buffer_reuse else temp_buffer.as_rvalue), p.proto, p.max_length.as_rvalue, p.type_name))
                emit.Line("ASSERT(%s != nullptr);" % (p.as_rvalue if not has_buffer_reuse else temp_buffer.as_rvalue))

                if has_large_buffer:
                    emit.IndentDec()
                    emit.Line("} else if (static_cast<uintptr_t>(%s) == ~0) {" % p.as_rvalue)
                    emit.IndentInc()
                    emit.Line("%s = nullptr;" % large_buffer.as_rvalue)
                    emit.IndentDec()
                    emit.Line("} else {")
                    emit.IndentInc()
                    emit.Line("%s = %s;" % (p.as_rvalue if not has_buffer_reuse else temp_buffer.as_rvalue, large_buffer.as_rvalue))
                    emit.IndentDec()
                    emit.Line("}")

                if has_buffer_reuse:
                    emit.Line()
                    emit.Line("if (%s != nullptr) {" % temp_buffer.as_rvalue)
                    emit.IndentInc()
                    emit.Line("::memcpy(%s, %s, %s);" % (temp_buffer.as_rvalue, p.as_rvalue, p.length.as_rvalue))
                    emit.IndentDec()
                    emit.Line("}")
                    emit.Line()
                    emit.Line("%s = static_cast<%s>(%s);" % (p.as_rvalue, p.proto, temp_buffer.as_rvalue))

                    if (p.max_length.as_rvalue != p.length.as_rvalue):
                        emit.IndentDec()
                        emit.Line("}")

                if ENABLE_INSTANCE_VERIFICATION:
                    emit.Line("if (%s == nullptr) { return (Core::ERROR_GENERAL); }" % p.as_rvalue)

                if not p.max_length.value:
                    emit.IndentDec()
                    emit.Line("}")

            def ReleaseBuffer(p, large_buffer):
                assert p.is_buffer
                if (p.max_length and (p.length.type.Type().size == "long")):
                    emit.Line("RPC::Administrator::Instance().Free(%s);" % large_buffer.as_rvalue)

            def WriteParameter(p, no_array=False):
                assert p

                if p.is_on_wire:
                    if p.optional and (not p.is_array or no_array):
                        emit.Line("%s.Boolean(%s.IsSet());" % ( vars["writer"], p.name))
                        emit.Line("if (%s.IsSet() == true) {" % p.name)
                        emit.IndentInc()

                    if p.is_iterator:
                        iterator = p.optional if p.optional else p
                        if iterator.iterator_optimization:
                            length = EmitLength(interface, iterator.length, p.as_rvalue + "->Count()", restrict_range=p.restrict_range)
                            element = EmitParam(interface, iterator.element, Normalize(iterator.name + "Item"), parent=iterator)
                            emit.Line("if (%s != nullptr) {" % p.as_rvalue)
                            emit.IndentInc()
                            WriteParameter(length)
                            emit.Line("if (%s == 0) {" % length.name)
                            emit.IndentInc()
                            emit.Line("%s.Boolean(true);" % vars["writer"])
                            emit.IndentDec()
                            emit.Line("}")
                            emit.Line("else {")
                            emit.IndentInc()
                            emit.Line("%s{};" % element.temporary_no_cv)
                            emit.Line("%s->Reset(0);" % p.as_rvalue)
                            emit.Line("while(%s->Next(%s) == true) {" % (p.as_rvalue, element.name))
                            emit.IndentInc()
                            WriteParameter(element)
                            emit.IndentDec()
                            emit.Line("}")
                            emit.IndentDec()
                            emit.Line("}")
                            emit.Line("%s->Release();" % p.as_rvalue)
                            emit.IndentDec()
                            emit.Line("}")
                            emit.Line("else {")
                            emit.IndentInc()
                            WriteParameter(EmitLength(interface, iterator.length, "0"))
                            emit.Line("%s.Boolean(false);" % vars["writer"])
                            emit.IndentDec()
                            emit.Line("}")
                        else:
                            emit.Line("%s.%s;" % (vars["writer"], p.write_rpc_type))

                    elif p.is_array and not no_array:
                        index = chr(ord('i') + p.name.count('['))
                        element = EmitParam(interface, p.identifier, "%s[%s]" % (p.name, index), suppress_type=True)
                        emit.Line("for (uint16_t %s = 0; %s < %s; %s++) {" % (index, index, p.length.as_rvalue, index))
                        emit.IndentInc()
                        WriteParameter(element, no_array=True)
                        emit.IndentDec()
                        emit.Line("}")

                    elif p.is_dynamic_array:
                        index = chr(ord('i') + p.name.count('['))
                        length = EmitLength(interface, p.length, (p.as_rvalue + ".size()"))
                        element = EmitParam(interface, p.element, "%s[%s]" % (p.as_rvalue, index), suppress_type=True, parent=p)
                        WriteParameter(length)
                        emit.Line("for (%s %s = 0; %s < %s; %s++) {" % (p.length.type_name, index, index, length.as_rvalue, index))
                        emit.IndentInc()
                        WriteParameter(element)
                        emit.IndentDec()
                        emit.Line("}")

                    elif p.is_compound:
                        params = [EmitParam(interface, v, (p.as_rvalue + "." + v.name), suppress_type=True, parent=p) for v in p.compound_merged.vars]

                        for pp in params:
                            WriteParameter(pp)

                            if pp.return_proxy or pp.proxy:
                                return_proxy_params.append(pp)

                    elif p.is_buffer:
                        if p.length.name == p.max_length.name and p.max_length and p.max_length.is_output:
                            emit.Line("%s.Buffer<%s>((%s == nullptr? 0 : %s), %s);" % (vars["writer"], p.max_length.type_name, p.as_rvalue, p.max_length.as_rvalue, p.as_rvalue))
                            WriteParameter(p.max_length)
                        else:
                            emit.Line("%s.%s;" % (vars["writer"], p.write_rpc_type))

                    else:
                        emit.Line("%s.%s;" % (vars["writer"], p.write_rpc_type))

                    if p.optional and (not p.is_array or no_array):
                        emit.IndentDec()
                        emit.Line("}")

            def AcquireInterface(p):
                assert p.proxy
                assert p.proxy_instance

                _name = Normalize(p.name)
                _name_maybe_opt = ((_name + "Object__") if p.optional else p.name)

                emit.Line("%s %s{};" % (p.proto_no_cv, _name))
                emit.Line("%s %s = nullptr;" % (p.proxy.proto_no_cv, p.proxy.name))
                emit.Line("if (%s != 0) {" % p.proxy_instance.as_rvalue)
                emit.IndentInc()

                if p.optional:
                    emit.Line("%s %s = nullptr;" % (p.optional.proto_no_cv, _name_maybe_opt))

                emit.Line("%s = RPC::Administrator::Instance().ProxyInstance(%s, %s, false, %s);" % (p.proxy.as_lvalue, vars["channel"], p.proxy_instance.as_lvalue, _name_maybe_opt))
                emit.Line("ASSERT((%s != nullptr) && (%s != nullptr));" % (_name_maybe_opt, p.proxy.as_rvalue))

                if ENABLE_INSTANCE_VERIFICATION:
                    emit.Line("if ((%s == nullptr) || (%s == nullptr)) { return (COM_ERROR | Core::ERROR_NOT_EXIST); }" % (_name_maybe_opt, p.proxy.as_rvalue))

                if p.optional:
                    emit.Line("%s = %s;" % (p.name, _name_maybe_opt))

                emit.IndentDec()
                emit.Line("}")

            def ReleaseProxy(p):
                assert p.proxy

                emit.Line("if (%s != nullptr) {" % p.proxy.as_rvalue)
                emit.IndentInc()
                emit.Line("RPC::Administrator::Instance().Release(%s, %s->Response());" % (p.proxy.as_rvalue, vars["message"]))
                emit.IndentDec()
                emit.Line("}")

            def RegisterInterface(p):
                assert p.interface_id

                if p.optional:
                    emit.Line("if (%s.IsSet() == true) {" % p.as_lvalue)
                    emit.IndentInc()

                if not isinstance(p.interface_id, AuxIdentifier):
                    # Interface ID comes from a parameter
                    emit.Line("RPC::Administrator::Instance().RegisterInterface(%s, %s, %s);" % (vars["channel"], p.name, p.interface_id.as_rvalue))
                else:
                    emit.Line("RPC::Administrator::Instance().RegisterInterface(%s, %s);" % (vars["channel"], p.as_rvalue))

                if p.optional:
                    emit.IndentDec()
                    emit.Line("}")

            def CallImplementation(retval, params):
                parameters = ", ".join([p.as_pure_rvalue for p in params])
                lhs = (((retval.temporary if not retval.return_proxy else retval.temporary_no_cv) + " = ") if retval else "")
                emit.Line("%s%s->%s(%s);" % (lhs, vars["implementation"], method.name, parameters))

            if ENABLE_SECURE:
                emit.Line("%s = Core::ERROR_NONE;" % hresult.temporary)
                emit.Line()

                emit.Line("%s = [&]() -> %s {" % (hresult.as_lvalue, hresult.proto_no_cv))
                emit.IndentInc()

            if interface.obj.is_iterator:
                face_name = vars["interface"]
                emit.Line("using %s = %s;" % (vars["interface"], Flatten(interface.obj.type, ns)))
                emit.Line()
            else:
                face_name = Flatten(interface.obj.type, ns)

            if ENABLE_INTEGRITY_VERIFICATION:
                emit.Line("if (%s->Parameters().Length() < (sizeof(%s) + sizeof(uint32_t) + 1)) { return (COM_ERROR | Core::ERROR_READ_ERROR); }" % (vars["message"], INSTANCE_ID))
                emit.Line()

            implementation_type = (method.CVString() + " " + face_name).strip()
            emit.Line("%s* %s = reinterpret_cast<%s*>(%s->Parameters().Implementation());" % (implementation_type, vars["implementation"], implementation_type, vars["message"]))

            emit.Line("ASSERT(%s != nullptr);" % (vars["implementation"]))

            if ENABLE_INSTANCE_VERIFICATION:
                emit.Line("if (RPC::Administrator::Instance().IsValid(%s, RPC::instance_cast(%s), %s) == false) { return (COM_ERROR | Core::ERROR_NOT_EXIST); }" \
                                % (vars["channel"], vars["implementation"], (face_name + "::ID")))
            elif ENABLE_SECURE:
                emit.Line("if (%s == nullptr) { return (COM_ERROR | Core::ERROR_NOT_EXIST); }" % vars["implementation"])

            emit.Line()

            if input_params:
                emit.Line("RPC::Data::Frame::Reader %s(%s->Parameters().Reader());" % (vars["reader"], vars["message"]))

                for p in input_params:
                    ReadParameter(p)

                emit.Line()

            if len(output_params) > 1:
                cnt = 0

                for p in output_params[1:]:
                    cnt += TemporaryParameter(p)

                if cnt:
                    emit.Line()

            if output_buffers:
                for p in output_buffers:
                    AllocateBuffer(p)

                emit.Line()

            if proxy_params:
                for p in proxy_params:
                    AcquireInterface(p)

                emit.Line()

            CallImplementation(retval, params)

            if len(output_params) > 1 or output_params[0]:
                emit.Line()
                emit.Line("RPC::Data::Frame::Writer %s(%s->Response().Writer());" % (vars["writer"], vars["message"]))
                for p in output_params:
                    if p:
                        WriteParameter(p)

            if custom_buffers:
                emit.Line()
                for p in custom_buffers:
                    ReleaseBuffer(p[0], p[1])

            if proxy_params:
                emit.Line()
                for p in proxy_params:
                    ReleaseProxy(p)

            if return_proxy_params:
                emit.Line()
                for p in return_proxy_params:
                    RegisterInterface(p)

            if ENABLE_SECURE:
                emit.Line()
                emit.Line("return (Core::ERROR_NONE);")

                emit.IndentDec()
                emit.Line("} ();")

                emit.Line()
                emit.Line("if (%s != Core::ERROR_NONE) {" % hresult.as_rvalue)
                emit.IndentInc()

                if has_hresult:
                    emit.Line("RPC::Data::Frame::Writer %s(%s->Response().Writer());" % (vars["writer"], vars["message"]))
                    emit.Line("%s.Number<uint32_t>(%s);" % (vars["writer"], hresult.as_rvalue))

                emit.Line("fprintf(stderr, \"COM-RPC stub 0x%%08x(%%u) failed: 0x%%08x\\n\", %s, %s, %s);" \
                                % (Flatten(interface.obj.type, ns) + "::ID", index, hresult.as_rvalue))

                if not has_hresult:
                    emit.Line("TRACE_L1(\"Warning: This COM-RPC failure will not propagate!\");")

                emit.IndentDec()
                emit.Line("}")

            if has_restricted_parameters and (not retval or not retval.is_hresult):
                log.InfoLine(method, "'%s': method is using restricted parameters, but its return value type is not 'Core::hresult'" % method.name)

        def EmitStubMethod(index, last, method, interface_name, interface, prepared_params):
            retval, params, input_params, output_params, proxy_params, return_proxy_params = prepared_params

            channel = "/* channel */" if method.stub or (not proxy_params and not return_proxy_params and not ENABLE_INSTANCE_VERIFICATION) else vars["channel"]
            message = "/* message */" if method.stub else vars["message"]
            emit.Line("// (%i) %s" % (index, Flatten(method.Proto(), ns)))
            emit.Line("//")
            emit.Line("[](Core::ProxyType<Core::IPCChannel>& %s, Core::ProxyType<RPC::InvokeMessage>& %s) {" % (channel, message))
            emit.IndentInc()

            if EMIT_TRACES:
                emit.Line('fprintf(stderr, "*** [%s stub] ENTER: %s()\\n");' % (interface_name, method.name))
                emit.Line()

            if method.stub:
                emit.Line("// stubbed method, no implementation")
            else:
                EmitStubMethodImplementation(index, method, interface_name, interface, retval, params, \
                                             input_params, output_params, proxy_params, return_proxy_params)

            if EMIT_TRACES:
                emit.Line()
                emit.Line('fprintf(stderr, "*** [%s stub] EXIT: %s()\\n");' % (interface_name, method.name))

            emit.IndentDec()
            emit.Line("}%s" % ("," if not last else ""))

        def EmitStub(interface_name, methods, stub_methods_name, interface, prepared_params):
            if BE_VERBOSE:
                log.Print("Emitting stub code for interface %s..." % Flatten(interface.obj.type, ns))

            # Emit class summary
            emit.Line("//")
            emit.Line("// %s interface stub definitions" % (interface_name))
            emit.Line("//")
            EmitFunctionOrder(methods)
            emit.Line()

            emit.Line("static ProxyStub::MethodHandler %s[] = {" % stub_methods_name)
            emit.IndentInc()

            for index, method in enumerate(methods):
                last = (index == len(methods) - 1)
                EmitStubMethod(index, last, method, interface_name, interface, prepared_params[index])

                if not last:
                    emit.Line()

            emit.Line(", nullptr")
            emit.IndentDec()
            emit.Line("}; // %s" % stub_methods_name)
            emit.Line()

        #
        # EMIT PROXY CODE
        #

        def EmitCompleteMethod():
            emit.Line("uint32_t _Complete(RPC::Data::Frame::Reader& %s) const" %  vars["reader"])
            emit.Line("{")
            emit.IndentInc()
            emit.Line("uint32_t result = Core::ERROR_NONE;")
            emit.Line("")
            emit.Line("while (%s.HasData() == true) {" % vars["reader"])
            emit.IndentInc()

            if ENABLE_INTEGRITY_VERIFICATION:
                emit.Line("const size_t entrySize = (sizeof(Core::instance_id) + sizeof(uint32_t) + sizeof(RPC::Data::Output::mode));")
                emit.Line("if (reader.Length() < entrySize) { result = (COM_ERROR | Core::ERROR_READ_ERROR); break; }")
                emit.Line()

            emit.Line("const Core::instance_id %s = reader.Number<Core::instance_id>();" % vars["implementation"])
            emit.Line("ASSERT(%s != 0);" % vars["implementation"])
            emit.Line()
            emit.Line("const uint32_t id = reader.Number<uint32_t>();")
            emit.Line("const RPC::Data::Output::mode how = reader.Number<RPC::Data::Output::mode>();")
            emit.Line()

            if ENABLE_INSTANCE_VERIFICATION:
                emit.Line("if (RPC::Administrator::Instance().IsValid(static_cast<const ProxyStub::UnknownProxy&>(*this).Channel(), %s, id) == false) { return (COM_ERROR | Core::ERROR_NOT_EXIST); }" \
                            % (vars["implementation"]))
                emit.Line()

            emit.Line("result = static_cast<const ProxyStub::UnknownProxy&>(*this).Complete(%s, id, how);" % vars["implementation"])
            emit.Line("if (result != Core::ERROR_NONE) { return (COM_ERROR | result); }")
            emit.IndentDec()
            emit.Line("}")
            emit.Line("")
            emit.Line("return (result);")
            emit.IndentDec()
            emit.Line("}")
            emit.Line()

        def EmitProxyMethodImplementation(index, method, interface_name, interface, retval, params,
                input_params, output_params, proxy_params, return_proxy_params):

            # For proxy
            def WriteParameter(p, no_array=False):
                assert p.is_on_wire

                if p.optional and (not p.is_array or no_array):
                    emit.Line("%s.Boolean(%s.IsSet());" % (vars["writer"], p.name))
                    emit.Line("if (%s.IsSet() == true) {" % p.name)
                    emit.IndentInc()

                if p.is_iterator:
                    iterator = p.optional if p.optional else p
                    if iterator.iterator_optimization:
                        emit.Line("if (%s != nullptr) {" % p.as_rvalue)
                        emit.IndentInc()
                        length = EmitLength(interface, iterator.length, p.as_rvalue + "->Count()")
                        element = EmitParam(interface, iterator.element, "element", parent=iterator)
                        WriteParameter(length)
                        emit.Line("if (%s == 0) {" % length.name)
                        emit.IndentInc()
                        emit.Line("%s.Boolean(true);" % vars["writer"])
                        emit.IndentDec()
                        emit.Line("}")
                        emit.Line("else {")
                        emit.IndentInc()
                        emit.Line("%s{};" % element.temporary_no_cv)
                        emit.Line("%s->Reset(0);" % p.as_rvalue)
                        emit.Line("while(%s->Next(%s) == true) {" % (p.as_rvalue, element.name))
                        emit.IndentInc()
                        WriteParameter(element)
                        emit.IndentDec()
                        emit.Line("}")
                        emit.IndentDec()
                        emit.Line("}")
                        emit.IndentDec()
                        emit.Line("} else {")
                        emit.IndentInc()
                        WriteParameter(EmitLength(interface, iterator.length, "0"))
                        emit.Line("%s.Boolean(false);" % vars["writer"])
                        emit.IndentDec()
                        emit.Line("}")
                    else:
                        emit.Line("%s.%s;" % (vars["writer"], p.write_rpc_type))

                elif p.is_array and not no_array:
                    index = chr(ord('i') + p.name.count('['))
                    element = EmitParam(interface, p.identifier, "%s[%s]" % (p.name, index), suppress_type=True)
                    emit.Line("for (uint16_t %s = 0; %s < %s; %s++) {" % (index, index, p.length.as_rvalue, index))
                    emit.IndentInc()
                    WriteParameter(element, no_array=True)
                    emit.IndentDec()
                    emit.Line("}")

                elif p.is_dynamic_array:
                    index = chr(ord('i') + p.name.count('['))
                    length = EmitLength(interface, p.length, (p.as_rvalue + ".size()"))
                    element = EmitParam(interface, p.element, "%s[%s]" % (p.as_rvalue, index), suppress_type=True, parent=p)
                    WriteParameter(length)
                    emit.Line("for (%s %s = 0; %s < %s; %s++) {" % (p.length.type_name, index, index, length.as_rvalue, index))
                    emit.IndentInc()
                    WriteParameter(element)
                    emit.IndentDec()
                    emit.Line("}")

                elif p.is_compound:
                    params = [EmitParam(interface, v, (p.as_rvalue + "." + v.name), suppress_type=True, parent=p) for v in p.compound_merged.vars]

                    for pp in params:
                        WriteParameter(pp)
                else:
                    emit.Line("%s.%s;" % (vars["writer"], p.write_rpc_type))

                if p.optional and (not p.is_array or no_array):
                    emit.IndentDec()
                    emit.Line("}")

            # For proxy
            def ReadParameter(p, no_array=False, suppress_type=True):
                if p.is_on_wire:
                    if p.optional and (not p.is_array or no_array):
                        CheckFrame(p)
                        emit.Line("if (%s.Boolean() == true) {" % vars["reader"])
                        emit.IndentInc()

                    if p.is_iterator:
                        iterator = p.optional if p.optional else p
                        if iterator.iterator_optimization:
                            name = Normalize(iterator.name)
                            count = name + "Count"
                            type = iterator.kind.resolvedArgs[0]
                            internal = name + "List"

                            length = EmitLength(interface, iterator.length, count, restrict_range=p.restrict_range)
                            element = EmitParam(interface, iterator.element, Normalize(name + "Item"), parent=iterator)
                            emit.Line("%s;" % length.temporary_no_cv)
                            ReadParameter(length)

                            emit.Line("if ((%s != 0) || (%s.Boolean() == true)) {" % (count, vars["reader"]))
                            emit.IndentInc()

                            emit.Line("std::list<%s> %s;" % (type, internal))
                            # emit.Line("%s.reserve(%s);" % (internal, length.as_rvalue))

                            emit.Line("while (%s-- != 0) {" % count)
                            emit.IndentInc()
                            emit.Line("%s{};" % element.temporary_no_cv)
                            ReadParameter(element)
                            emit.Line("%s.push_back(std::move(%s));" % (internal, element.as_rvalue))
                            emit.IndentDec()
                            emit.Line("}")

                            impl = "::Thunder::RPC::IteratorType<%s>" % iterator.kind.type
                            emit.Line("%s = Core::ServiceType<%s>::Create<%s>(std::move(%s));" % (p.as_lvalue, impl, iterator.kind.type, internal))
                            emit.Line("ASSERT(%s != nullptr);" % (p.as_rvalue))

                            emit.IndentDec()
                            emit.Line("}")
                            emit.Line("else {")
                            emit.IndentInc()
                            emit.Line("%s = nullptr;" % p.as_lvalue)
                            emit.IndentDec()
                            emit.Line("}")
                        else:
                            emit.Line("%s = reinterpret_cast<%s>(static_cast<const ProxyStub::UnknownProxy&>(*this).Interface(%s.%s, %s));" % \
                                (p.name, p.pure_proto, vars["reader"], p.read_rpc_type, p.interface_id.as_rvalue))

                    elif p.is_array and not no_array:
                        index = chr(ord('i') + p.name.count('['))
                        element = EmitParam(interface, p.identifier, "%s[%s]" % (p.name, index), suppress_type=True)
                        emit.Line("for (uint16_t %s = 0; %s < %s; %s++) {" % (index, index, p.length.as_rvalue, index))
                        emit.IndentInc()
                        ReadParameter(element, no_array=True)
                        emit.IndentDec()
                        emit.Line("}")

                    elif p.is_dynamic_array:
                        length = EmitLength(interface, p.length, Normalize(p.name + "Size"))
                        element = EmitParam(interface, p.element, Normalize(p.name + "Item"), parent=p)
                        emit.Line("%s{};" % (length.temporary_no_cv))
                        ReadParameter(length)
                        emit.Line("%s.reserve(%s);" % (p.as_rvalue, length.as_rvalue))
                        index = chr(ord('i') + p.name.count('Item'))
                        emit.Line("for (%s %s = 0; %s < %s; %s++) {" % (p.length.type_name, index, index, length.as_rvalue, index))
                        emit.IndentInc()
                        emit.Line("%s{};" % (element.temporary_no_cv))
                        ReadParameter(element)
                        emit.Line("%s.push_back(std::move(%s));" % (p.as_rvalue, element.as_rvalue))
                        emit.IndentDec()
                        emit.Line("}")

                    elif p.is_buffer:
                        CheckFrame(p)
                        CheckSize(p)

                        if p.length and p.length.is_output and (p.length.name != p.max_length.name):
                            emit.Line("%s = %s.%s;" % (p.length.as_lvalue, vars["reader"], p.read_rpc_type))
                        else:
                            emit.Line("%s.%s;" % (vars["reader"], p.read_rpc_type))

                        if p.max_length and p.max_length.is_output:
                            ReadParameter(p.max_length)

                    elif isinstance(p.kind, CppParser.MacAddress):
                        emit.Line("%s.%s;" % (vars["reader"], p.read_rpc_type))

                    elif p.is_compound:
                        if p.optional:
                            obj_name = Normalize(p.name + "Object__");
                            emit.Line("%s %s{};" % (p.optional.type_name, obj_name))
                        else:
                            obj_name = p.name

                        params = [EmitParam(interface, v, (obj_name + "." + v.name), suppress_type=True, parent=p) for v in p.compound_merged.vars]

                        for pp in params:
                            ReadParameter(pp)

                        if p.optional:
                            emit.Line("%s = std::move(%s);" % (p.name, obj_name))

                    elif p.return_proxy:
                        emit.Line("%s = reinterpret_cast<%s>(static_cast<const ProxyStub::UnknownProxy&>(*this).Interface(%s.%s, %s));" % \
                                (p.name, p.pure_proto, vars["reader"], p.read_rpc_type, p.interface_id.as_rvalue))

                    else:
                        param_ = "%s = %s.%s;" % (p.as_lvalue, vars["reader"], p.read_rpc_type)

                        def _EmitAssignment(p):
                            if p.is_string:
                                CheckFrame(p)
                                CheckSize(p)
                                emit.Line(param_)

                            else:
                                CheckFrame(p)
                                emit.Line(param_)
                                CheckRange(p, p)

                        _EmitAssignment(p.optional if p.optional else p)

                    if p.optional and (not p.is_array or no_array):
                        emit.IndentDec()
                        emit.Line("}")

            if EMIT_TRACES:
                emit.Line('fprintf(stderr, "*** [%s proxy] ENTER: %s()\\n");' % (interface_name, method.name))
                emit.Line()

            hresult = AuxIdentifier(CppParser.Integer(HRESULT), (CppParser.Ref.VALUE), vars["hresult"])

            emit.Line("IPCMessage %s(static_cast<const ProxyStub::UnknownProxy&>(*this).Message(%s));" % (vars["message"], index))
            emit.Line()

            if input_params:
                emit.Line("RPC::Data::Frame::Writer %s(%s->Parameters().Writer());" % (vars["writer"], vars["message"]))
                for p in input_params:
                    WriteParameter(p)

                emit.Line()

            if ENABLE_INSTANCE_VERIFICATION and proxy_params:
                instances = []

                for proxy in proxy_params:
                    if proxy.optional:
                        instances.append("{ (%s.IsSet() == true? RPC::instance_cast(%s) : 0), %s::ID }" % (proxy.name, proxy.as_rvalue, proxy.optional.type_name))
                    else:
                        instances.append("{ RPC::instance_cast(%s), %s::ID }" % (proxy.as_rvalue, proxy.type_name))

                instances.append("{ 0, 0 }")

                emit.Line("const RPC::InstanceRecord passedInstances[] = { %s };" % ", ".join(instances))
                emit.Line("static_cast<const ProxyStub::UnknownProxy&>(*this).Channel()->CustomData(passedInstances);")
                emit.Line()

            reuse_hresult = (retval and retval.is_hresult)
            needs_reader = (((len(output_params) > 1) or output_params[0]) or proxy_params)
            check_invoke = ((needs_reader) or ENABLE_SECURE)
            _first_param = (1 if reuse_hresult else 0)

            if not reuse_hresult and retval:
                emit.Line("%s{};" % retval.temporary_no_cv)
                emit.Line()

            _const_hresult = (not reuse_hresult and not ENABLE_SECURE)
            lhs = ("%s%s = " % (("const " if _const_hresult else ""), hresult.temporary_no_cv)) if check_invoke else ""

            emit.Line("%sstatic_cast<const ProxyStub::UnknownProxy&>(*this).Invoke(%s);" % (lhs, vars["message"]))

            if check_invoke:
                emit.Line("if (%s == Core::ERROR_NONE) {" % hresult.as_rvalue)
                emit.IndentInc()

                if ENABLE_SECURE:
                    emit.Line("%s = [&]() -> %s {" % (hresult.as_lvalue, hresult.proto_no_cv))
                    emit.IndentInc()

            if needs_reader:
                emit.Line("RPC::Data::Frame::Reader %s(%s->Response().Reader());" % (vars["reader"], vars["message"]))

                if retval:
                    if reuse_hresult:
                        CheckFrame(retval)
                        emit.Line("%s = %s.%s;" % (hresult.as_lvalue, vars["reader"], retval.read_rpc_type))
                        CheckRange(retval, retval)

                if ENABLE_SECURE and (reuse_hresult and (len(output_params) > _first_param)):
                    emit.Line("if ((%s & COM_ERROR) == 0) {" % hresult.as_rvalue)
                    emit.IndentInc()

                if len(output_params) > _first_param:
                    for p in output_params[_first_param:]:
                        if p:
                            ReadParameter(p)

                if ENABLE_SECURE and (reuse_hresult and (len(output_params) > _first_param)):
                    emit.IndentDec()
                    emit.Line("}")

            if proxy_params:
                emit.Line()

                if ENABLE_SECURE:
                    emit.Line("const uint32_t completeResult__ = _Complete(%s);" % vars["reader"])
                    emit.Line("if (completeResult__ != Core::ERROR_NONE) { return (completeResult__); }")
                else:
                    emit.Line("_Complete(%s);" % vars["reader"])

            if check_invoke:
                if ENABLE_SECURE:
                    emit.Line()

                    if reuse_hresult:
                        emit.Line("return (%s);" % hresult.as_rvalue)
                    else:
                        emit.Line("return (Core::ERROR_NONE);")

                    emit.IndentDec()
                    emit.Line("} ();")

                emit.IndentDec()
                emit.Line("} else {")
                emit.IndentInc()
                emit.Line("ASSERT((%s & COM_ERROR) != 0);" % hresult.as_rvalue)
                emit.IndentDec()
                emit.Line("}")

            if ENABLE_SECURE:
                emit.Line()
                emit.Line("if ((%s & COM_ERROR) != 0) {" % hresult.as_rvalue)
                emit.IndentInc()
                emit.Line("fprintf(stderr, \"COM-RPC call 0x%%08x(%%u) failed: 0x%%08x\\n\", %s, %s, %s);" \
                                % (Flatten(interface.obj.type, ns) + "::ID", index, hresult.as_rvalue))

                if not reuse_hresult:
                    emit.Line("TRACE_L1(\"Warning: This COM-RPC failure will not propagate!\");")

                emit.IndentDec()
                emit.Line("}")

            if ENABLE_INSTANCE_VERIFICATION and proxy_params:
                emit.Line()
                emit.Line("static_cast<const ProxyStub::UnknownProxy&>(*this).Channel()->CustomData(nullptr);")

            if EMIT_TRACES:
                emit.Line()
                emit.Line('fprintf(stderr, "*** [%s proxy] EXIT: %s()\\n");' % (interface_name, method.name))

            if retval:
                emit.Line()
                emit.Line("return (%s);" % (hresult.name if reuse_hresult else retval.name))

        def EmitProxyMethodStubbed(index, method, interface_name, retval):
            if EMIT_TRACES:
                emit.Line('fprintf(stderr, "*** [%s proxy] STUBBED METHOD: %s() [index %i]\\n");' % (interface_name, method.name, index))
                emit.Line()

            emit.Line("// stubbed method, no implementation")

            if retval:
                emit.Line()

                if retval.is_hresult:
                    emit.Line("return (%s{COM_ERROR | Core::ERROR_UNAVAILABLE});" % retval.type_name)
                else:
                    emit.Line("return {};")


        def EmitProxyMethod(index, method, interface_name, interface, prepared_params):
            retval, params, input_params, output_params, proxy_params, return_proxy_params = prepared_params

            method_type = ((Flatten(retval.identifier.Proto(), ns) + " ") if retval else "void ")
            method_qualifiers = ((" const" if "const" in method.qualifiers else "") + " override")
            params_list = [(Flatten(p.identifier.Signature((p.name if not method.stub else "")), ns)) for p in params]

            emit.Line("%s%s(%s)%s" % (method_type, method.name, ", ".join(params_list), method_qualifiers))
            emit.Line("{")
            emit.IndentInc()

            if not method.stub:
                EmitProxyMethodImplementation(index, method, interface_name, interface, retval, params, \
                                                input_params, output_params, proxy_params, return_proxy_params)
            else:
                EmitProxyMethodStubbed(index, methods, interface_name, retval)

            emit.IndentDec()
            emit.Line("}")
            emit.Line()

            return ((proxy_params != []) and not method.stub)

        def EmitProxy(interface_name, methods, proxy_name, interface, prepared_params):
            if BE_VERBOSE:
                log.Print("Emitting proxy code for interface %s..." % Flatten(interface.obj.type, ns))

            # Emit class summary
            emit.Line("//")
            emit.Line("// %s interface proxy definitions" % (interface_name))
            emit.Line("//")
            EmitFunctionOrder(methods)
            emit.Line()

            # Emit proxy class definition
            emit.Line("class %s final : public ProxyStub::UnknownProxyType<%s> {" % (proxy_name, Flatten(interface.obj.type, ns)))
            emit.Line("public:")
            emit.IndentInc()

            # Emit constructor
            emit.Line("%s(const Core::ProxyType<Core::IPCChannel>& channel, const %s implementation, const bool otherSideInformed)" % (proxy_name, INSTANCE_ID))
            emit.Line("    : BaseClass(channel, implementation, otherSideInformed)")
            emit.Line("{")
            emit.Line("}")
            emit.Line()

            have_proxy = False

            for index, method in enumerate(methods):
                has_proxy = EmitProxyMethod(index, method, interface_name, interface, prepared_params[index])
                if has_proxy:
                    have_proxy = True

            emit.IndentDec()

            if have_proxy:
                emit.Line("private:")
                emit.IndentInc()
                EmitCompleteMethod()
                emit.IndentDec()

            emit.Line("}; // class %s" % proxy_name)
            emit.Line()


        #
        # EMIT REGISTRATION CODE
        #
        def EmitRegistration(announce_list):
            if BE_VERBOSE:
                log.Print("Emitting stub registration code...")

            emit.Line("namespace {")
            emit.IndentInc()
            emit.Line()

            for _, [_, methods, stub, _, interface, _ ] in announce_list.items():
                emit.Line("typedef ProxyStub::UnknownStubType<%s, %s> %s;" % (Flatten(interface.obj.type, ns), methods, stub))

            emit.Line()
            emit.Line("static class Instantiation {")
            emit.Line("public:")
            emit.IndentInc()
            emit.Line("Instantiation()")
            emit.Line("{")
            emit.IndentInc()

            if EMIT_TRACES:
                emit.Line("fprintf(stderr, \"*** Announcing %s interface methods...\\n\");" % Flatten(interface.obj.type, ns))

            for _, [_, _, stub, proxy, interface, _ ]  in announce_list.items():
                emit.Line("RPC::Administrator::Instance().Announce<%s, %s, %s>();" % (Flatten(interface.obj.type, ns), proxy, stub))

            if EMIT_TRACES:
                emit.Line("fprintf(stderr, \"*** Announcing %s interface methods... done\\n\");" % Flatten(interface.obj.type, ns))

            emit.IndentDec()
            emit.Line("}")
            emit.Line("~Instantiation()")
            emit.Line("{")
            emit.IndentInc()

            if EMIT_TRACES:
                emit.Line("fprintf(stderr, \"*** Recalling %s interface methods...\\n\");" % Flatten(interface.obj.type, ns))

            for _, [_, _, _, _, interface, _ ]  in announce_list.items():
                emit.Line("RPC::Administrator::Instance().Recall<%s>();" % (Flatten(interface.obj.type, ns)))

            if EMIT_TRACES:
                emit.Line("fprintf(stderr, \"*** Announcing %s interface methods... done\\n\");" % Flatten(interface.obj.type, ns))

            emit.IndentDec()
            emit.Line("}")
            emit.IndentDec()
            emit.Line("} ProxyStubRegistration;")
            emit.Line()
            emit.IndentDec()
            emit.Line("} // namespace")
            emit.Line()


        emit.Line("//")
        emit.Line("// generated automatically from \"%s\"" % interface_header_name)
        emit.Line("//")
        emit.Line("// implements COM-RPC proxy stubs for:")

        for face in interfaces:
            if not face.obj.omit:
                emit.Line("//   - %s" % Flatten(str(face.obj), ns))

        emit.Line("//")

        if ENABLE_SECURE:
            emit.Line("// secure code enabled:")
            if ENABLE_INSTANCE_VERIFICATION:
                emit.Line("//   - instance verification enabled")
            if ENABLE_RANGE_VERIFICATION:
                emit.Line("//   - range verification enabled")
            if ENABLE_INTEGRITY_VERIFICATION:
                emit.Line("//   - frame coherency verification enabled")
            emit.Line("//")

        emit.Line()

        if os.path.isfile(os.path.join(os.path.dirname(source_file), "Module.h")):
            emit.Line('#include "Module.h"')

        if os.path.isfile(os.path.join(os.path.dirname(source_file), interface_header_name)):
            emit.Line('#include "%s"' % interface_header_name)

        emit.Line()

        emit.Line('#include <com/com.h>')
        emit.Line()

        emit.Line("namespace %s {" % STUB_NAMESPACE.split("::")[-2])
        emit.Line()
        emit.Line("namespace %s {" % STUB_NAMESPACE.split("::")[-1])
        emit.Line()
        emit.IndentInc()

        if (interface_namespace != STUB_NAMESPACE.split("::")[-2]):
            emit.Line("using namespace %s;" % interface_namespace)
            emit.Line()

        emit.Line("PUSH_WARNING(DISABLE_WARNING_DEPRECATED_USE)")
        emit.Line("PUSH_WARNING(DISABLE_WARNING_TYPE_LIMITS)")
        emit.Line()

        emit.Line("// -----------------------------------------------------------------")
        emit.Line("// STUBS")
        emit.Line("// -----------------------------------------------------------------\n")

        for name, element in announce_list.items():
            EmitStub(name, element[0], element[1], element[4], element[5])

        emit.Line("// -----------------------------------------------------------------")
        emit.Line("// PROXIES")
        emit.Line("// -----------------------------------------------------------------\n")

        for name, [methods, _, _, proxy_name, interface, prepared_params] in announce_list.items():
            EmitProxy(name, methods, proxy_name, interface, prepared_params)

        emit.Line("POP_WARNING()")
        emit.Line("POP_WARNING()")
        emit.Line()

        emit.Line("// -----------------------------------------------------------------")
        emit.Line("// REGISTRATION")
        emit.Line("// -----------------------------------------------------------------")
        EmitRegistration(announce_list)

        emit.IndentDec()
        emit.Line("} // namespace %s" % STUB_NAMESPACE.split("::")[-1])
        emit.Line()
        emit.Line("}")

    return interfaces, omit_interface_used


def GenerateIdentification(name):
    if not os.path.exists(name):
        with open(name, "w") as file:
            emit = Emitter(file, INDENT_SIZE)
            emit.Line("//")
            emit.Line("// generated automatically")
            emit.Line()
            emit.Line("#include \"Module.h\"")
            emit.Line("#include <com/ProxyStubs.h>")
            emit.Line()
            emit.Line("extern \"C\" {")
            emit.Line()

            options = []

            # These should always go together...
            assert ENABLE_INSTANCE_VERIFICATION == ENABLE_RANGE_VERIFICATION

            if ENABLE_INSTANCE_VERIFICATION:
                options.append("PROXYSTUBS_OPTIONS_SECURE")
            if ENABLE_INTEGRITY_VERIFICATION:
                options.append("PROXYSTUBS_OPTIONS_COHERENT")
            if not options:
                options.append("0")

            emit.Line("EXTERNAL_EXPORT proxystubs_options_t proxystubs_options()")
            emit.Line("{")
            emit.IndentInc()
            emit.Line("return (static_cast<proxystubs_options_t>(%s));" % " | ".join(options))
            emit.IndentDec()
            emit.Line("}")
            emit.IndentDec()
            emit.Line()
            emit.Line("}")
            emit.Line()

            log.Info("created file %s" % os.path.basename(name))


# -------------------------------------------------------------------------
# entry point

if __name__ == "__main__":
    argparser = argparse.ArgumentParser(description='Generate proxy stub code out of interface header files.',
                                        epilog='Note that using --secure, --coherent and --traces options will produce less performant code.',
                                        formatter_class=lambda prog: argparse.RawTextHelpFormatter(prog, max_help_position=30))
    argparser.add_argument('path', nargs="*", help="C++ interface file(s) or a directory(ies) with interface files")
    argparser.add_argument("--help-tags",
                           dest="help_tags",
                           action="store_true",
                           default=False,
                           help="show help on supported source code tags and exit")
    argparser.add_argument("--code",
                           dest="code",
                           action="store_true",
                           default=False,
                           help="generate stub and proxy C++ code (default operation)")
    argparser.add_argument("--lua-code",
                           dest="lua_code",
                           action="store_true",
                           default=False,
                           help="generate lua code with interface information")
    argparser.add_argument("--secure",
                           dest="secure",
                           action="store_true",
                           default=False,
                           help="emit additional parameter verification (default: no extra verification)")
    argparser.add_argument("--coherent",
                           dest="integrity",
                           action="store_true",
                           default=False,
                           help="emit additional frame coherency verification (default: no extra verification)")
    argparser.add_argument("--no-identify",
                           dest="noidentify",
                           action="store_true",
                           default=False,
                           help="do not emit additional file with identification code (default: emit identification code)")
    argparser.add_argument("--traces",
                           dest="traces",
                           action="store_true",
                           default=False,
                           help="emit additional debug traces (default: no extra traces)")
    argparser.add_argument("--namespace",
                           dest="if_namespaces",
                           metavar="NS",
                           action="append",
                           default=[],
                           help="set a namespace to look for interfaces in, can be used multiple times (default: search in %s)" % INTERFACE_NAMESPACES[0])
    argparser.add_argument("--framework-namespace",
                           dest="framework_namespace",
                           metavar="NS",
                           action="store",
                           default=FRAMEWORK_NAMESPACE,
                           help="set framework namespace (default: %s)" % FRAMEWORK_NAMESPACE)
    argparser.add_argument("--outdir",
                           dest="outdir",
                           metavar="DIR",
                           action="store",
                           default="",
                           help="specify output directory (default: generate files in the same directory as source)")
    argparser.add_argument("--no-warnings",
                           dest="no_warnings",
                           action="store_true",
                           default=not SHOW_WARNINGS,
                           help="suppress all warnings (default: show warnings)")
    argparser.add_argument("--verbose",
                           dest="verbose",
                           action="store_true",
                           default=BE_VERBOSE,
                           help="enable verbose logging (default: verbose logging disabled)")
    argparser.add_argument("--keep",
                           dest="keep_incomplete",
                           action="store_true",
                           default=False,
                           help="keep incomplete files (default: remove partially generated files)")
    argparser.add_argument("--force",
                           dest="force",
                           action="store_true",
                           default=FORCE,
                           help="force stub generation even if destination file is up-to-date (default: force disabled)")
    argparser.add_argument("--collated-iterators",
                           dest="collated_iterators",
                           action="store_true",
                           default=ENABLE_ITERATOR_OPTIMIZATION,
                           help="pass collated iterator data in one go (default: pass interface)")
    argparser.add_argument("-i",
                           dest="extra_includes",
                           metavar="FILE",
                           action='append',
                           default=[],
                           nargs="*",
                           help="include an additional C++ header file, can be used multiple times (default: include 'Ids.h')")
    argparser.add_argument('-I', dest="includePaths", metavar="INCLUDE_DIR", action='append', default=[], type=str,
                           help='add an include search path, can be used multiple times')

    args = argparser.parse_args(sys.argv[1:])
    SHOW_WARNINGS = not args.no_warnings
    BE_VERBOSE = args.verbose
    FORCE = args.force
    ENABLE_INSTANCE_VERIFICATION = args.secure
    ENABLE_RANGE_VERIFICATION = args.secure
    ENABLE_INTEGRITY_VERIFICATION = args.integrity
    ENABLE_SECURE = ENABLE_INSTANCE_VERIFICATION or ENABLE_RANGE_VERIFICATION or ENABLE_INTEGRITY_VERIFICATION
    ENABLE_ITERATOR_OPTIMIZATION = args.collated_iterators
    log.show_infos = BE_VERBOSE
    log.show_warnings = SHOW_WARNINGS
    OUTDIR = args.outdir
    EMIT_TRACES = args.traces
    scan_only = False
    keep_incomplete = args.keep_incomplete

    if args.framework_namespace:
        FRAMEWORK_NAMESPACE = args.framework_namespace
        STUB_NAMESPACE = "::%s::ProxyStubs" % FRAMEWORK_NAMESPACE
        INTERFACE_NAMESPACES = ["::%s" % FRAMEWORK_NAMESPACE]
        CLASS_IUNKNOWN = "::%s::Core::IUnknown" % FRAMEWORK_NAMESPACE

    if args.if_namespaces:
        INTERFACE_NAMESPACES = args.if_namespaces

    for i, ns in enumerate(INTERFACE_NAMESPACES):
        if not ns.startswith("::"):
            INTERFACE_NAMESPACES[i] = "::" + ns

    if args.help_tags:
        print("The following special tags are supported:")
        print("   @stop                  - a hard stop for parsing the file")
        print("   @omit                  - omit generating code for the next item (class or method)")
        print("   @stub                  - generate an empty stub for the next item (class or method)")
        print("   @insert \"file\"         - include a C++ header file, relative to the directory of the current file")
        print("   @insert <file>         - include a C++ header file, relative to the defined include directories")
        print("                            note: this is intended for resolving unknown types; classes defined in included")
        print("                            headers are not considered for stub generation (except for template classes)")
        print("   @define {identifier}   - defines a literal as a known identifier (equivalent of #define {identifier} in C++ code)")
        print("")
        print("Parameter-related tags:")
        print("   @in                    - indicates an input parameter")
        print("   @out                   - indicates an output parameter")
        print("   @inout                 - indicates an input/output parameter (equivalent of @in @out)")
        print("   @restrict:[{min}..]{max} - specifies valid range for a parameter (for buffers, vectors, iterators, strings: valid size)")
        print("                            e.g.: @restrict:1..32, @restrict:256..1K, @restrict:1M-1, @restrict:nonempty")
        print("   @interface:{expr}      - specifies a parameter or value indicating interface ID value for void* interface passing")
        print("   @length:{expr}         - specifies a buffer length value (a constant, a parameter name or a math expression)")
        print("   @maxlength:{expr}      - specifies a maximum buffer length value (a constant, a parameter name or a math expression),")
        print("                            if @maxlength is not specified, expresion from @length is used")
        print("")
        print("                            Examples:")
        print("                            @length:bufferSize, @length:32, @length:(width*height*4), @length:(sizeof(uint64_t))")
        print("                            @length:return - length is carried in the return value")
        print("                            @length:void - length is the size of one element")
        print("")
        print("JSON-RPC-related parameters:")
        print("   @json [version]        - takes a C++ class in for JSON-RPC generation (with optional version specification)")
        print("   @json:omit             - leaves out a method, property or notification from a class intended for JSON-RPC generation")
        print("   @compliant             - tags a class to be generated in JSON-RPC compliant format (default)")
        print("   @uncompliant:extended  - tags a class to be generated in the obsolete 'extended' format")
        print("   @uncompliant:collapsed - tags a class to be generated in the obsolete 'collapsed' format")
        print("   @text:keep             - keeps original C++ names for all JSON names in an interface")
        print("   @text:{caseconvention} - sets case convention for an interface")
        print("   @event                 - tags a class to be generated as an JSON-RPC notification")
        print("   @iterator              - tags a C++ class to be generated as an JSON-RPC interator")
        print("   @property              - tags C++ method to be generated as a JSON-RPC property")
        print("   @async                 - indicates that a method is asynchronous")
        print("   @wrapped               - enforces object result even if a single item is returned (method or interface level)")
        print("   @statuslistener        - emits registration/unregistration status listeners for an event")
        print("   @bitmask               - indicates that enumerator lists should be packed into into a bit mask")
        print("   @index                 - indicates that a parameter in a JSON-RPC property or notification is an index")
        print("   @opaque                - indicates that a string parameter is an opaque JSON object")
        print("   @extract               - indicates that that if only one element is present in the array it shall be taken out of it")
        print("   @optional              - indicates that a parameter, struct member or property index is optional (deprecated, use Core::OptionalType instead)")
        print("   @default:{value}       - set a default value for the deprecated optional parameter (deprecated)")
        print("   @encode:{method}       - encodes a buffer, array or vector to a string using the specified method (base64, hex, mac)")
        print("   @encode:{lookup}       - encodes interface instances to JSON using lookup methods (lookup, autolookup) ")
        print("   @encode:bitmask        - encodes a bitmask enum to a JSON array")
        print("   @encode:text           - enocdes enum to text (generate enum conversion tables), implicit for enums inside @json-tagged classes")
        print("   @prefix:{name}         - prefixes all JSON-RPC methods, properties and notifications names in an interface with a string")
        print("   @text:{name}           - sets a different name for a parameter, enumerator, struct or JSON-RPC method, struct members, property or notification")
        print("   @alt:{name}            - provides an alternative name a JSON-RPC method or property can by called by")
        print("                            (for a notification it provides an additional name to send out)")
        print("   @alt-deprecated:{name} - provides an alternative deprecated name a JSON-RPC method can by called by")
        print("   @alt-obsolete:{name}   - provides an alternative obsolete name a JSON-RPC method can by called by")
        print("")
        print("JSON-RPC documentation-related parameters:")
        print("   @sourcelocation {lnk}  - sets source location link to be used in the documentation")
        print("   @deprecated            - outlines a JSON-RPC method, property or notification as deprecated")
        print("   @obsolete              - outlines a JSON-RPC method, property or notification as osbsolete")
        print("   @brief {desc}          - sets a brief description for a JSON-RPC method, property or notification")
        print("   @details {desc}        - sets a detailed description for a JSON-RPC method, property or notification")
        print("   @param {name} {desc}   - sets a description for a parameter of a JSON-RPC method or notification")
        print("   @retval {value} {desc} - sets a description for a return value of a JSON-RPC method or property")
        print("   @end                   - indicates end of the enumerator list (meaning don't document further)")
        print("")
        print("Tags shall be placed inside C++ comments.")
        sys.exit()

    if not args.code and not args.lua_code:
        # Backwards compatibility if user selects nothing let's build proxystubs
        args.code = True

    if not args.path:
        argparser.print_help()
    else:
        files = []
        interface_files = []

        # Resolve wildcards
        for p in args.path:
            if "*" in p or "?" in p:
                files.extend(glob.glob(p))
            else:
                files.append(p)

        for elem in files:
            if os.path.isdir(elem):
                interface_files += [ os.path.join(elem, f) for f in os.listdir(elem)
                    if (os.path.isfile(os.path.join(elem, f)) and re.match("I.*\\.h", f) and f != IDS_DEFINITIONS_FILE) ]
            elif os.path.isfile(elem):
                if re.match(".*I.*\\.h", elem) and not elem.endswith(IDS_DEFINITIONS_FILE):
                    interface_files += [elem]
            else:
                log.Error("invalid file or directory", elem)

        faces = []
        skipped = []

        if interface_files:
            if args.lua_code:
                name = "protocol-thunder-comrpc.data"
                lua_file = open(("." if not OUTDIR else OUTDIR) + os.sep + name, "w")
                emit = Emitter(lua_file, INDENT_SIZE)
                lua_interfaces = dict()
                lua_enums = dict()

            if args.code and not args.noidentify:
                output_file = os.path.join(os.path.dirname(interface_files[0]) if not OUTDIR else OUTDIR, "ProxyStubsMetadata.cpp")

                out_dir = os.path.dirname(output_file)
                if not os.path.exists(out_dir):
                    os.makedirs(out_dir)

                GenerateIdentification(output_file)

            for source_file in interface_files:
                try:
                    _extra_includes = [ os.path.join("@" + os.path.dirname(source_file), IDS_DEFINITIONS_FILE) ]
                    _extra_includes.extend(args.extra_includes)

                    tree = Parse(source_file, FRAMEWORK_NAMESPACE, args.includePaths,
                                    os.path.join("@" + os.path.dirname(os.path.realpath(__file__)), DEFAULT_DEFINITIONS_FILE),
                                    _extra_includes)

                    if args.code:
                        log.Header(source_file)

                        output_file = os.path.join(os.path.dirname(source_file) if not OUTDIR else OUTDIR,
                            PROXYSTUB_CPP_NAME % CreateName(os.path.basename(source_file)).split(".", 1)[0])

                        out_dir = os.path.dirname(output_file)
                        if not os.path.exists(out_dir):
                            os.makedirs(out_dir)

                        new_faces = []
                        some_omitted = False

                        for ns in INTERFACE_NAMESPACES:
                            output, some_omitted = GenerateStubs2(output_file, source_file, tree, ns, scan_only)

                            new_faces += output

                        if not new_faces:
                            if not some_omitted:
                                raise NoInterfaceError
                            else:
                                log.Info("no interface classes found")

                        else:
                            faces += new_faces
                            log.Info("created file %s" % os.path.basename(output_file))

                        # dump interfaces if only scanning
                        if scan_only:
                            for f in sorted(output, key=lambda x: str(x.id)):
                                print(f.id, f.obj.full_name)

                    if args.lua_code:
                        log.Info("(lua generator) Scanning %s..." % os.path.basename(source_file))

                        for ns in INTERFACE_NAMESPACES:
                            GenerateLuaData(Emitter(lua_file, INDENT_SIZE), lua_interfaces, lua_enums, source_file, tree, ns)

                except NotModifiedException as err:
                    log.Info("skipped file %s, up-to-date" % os.path.basename(output_file))
                    skipped.append(source_file)
                except SkipFileError as err:
                    log.Print("skipped file %s" % os.path.basename(output_file))
                    skipped.append(source_file)
                except NoInterfaceError as err:
                    log.Warn("no interface classes found")
                except TypenameError as err:
                    log.Error(err)
                    if not keep_incomplete and os.path.isfile(output_file):
                        os.remove(output_file)
                except (CppParser.ParserError, CppParser.LoaderError) as err:
                    log.Error(err)

            if args.code:
                if scan_only:
                    print("\nInterface dump:")

                sorted_faces = sorted(faces, key=lambda x: str(x.id))
                for i, f in enumerate(sorted_faces):
                    if isinstance(f.id, int):
                        if scan_only:
                            if i and sorted_faces[i - 1].id < f.id - 1:
                                print("...")

                            print("%s (%s) - '%s'" % (hex(f.id) if isinstance(f.id, int) else "?", str(f.id), f.obj.full_name))

                        if i and sorted_faces[i - 1].id == f.id:
                            log.Warn("duplicate interface ID %s (%s) of %s" % \
                                (hex(f.id) if isinstance(f.id, int) else "?", f.id, f.obj.full_name))
                    else:
                        log.Info("can't evaluate interface ID '%s' of %s" % (f.id, f.obj.full_name))

                if len(interface_files) > 1 and BE_VERBOSE:
                    print("")

                log.Info(("all done; %i file%s processed" %
                        (len(interface_files) - len(skipped), "s" if len(interface_files) - len(skipped) > 1 else "")) +
                        ((" (%i file%s skipped)" % (len(skipped), "s" if len(skipped) > 1 else "")) if skipped else "") +
                        ("; %i interface%s parsed:" % (len(faces), "s" if len(faces) > 1 else "")) +
                        ((" %i error%s" %
                            (len(log.errors), "s" if len(log.errors) > 1 else "")) if log.errors else " no errors") +
                        ((" (%i warning%s)" %
                            (len(log.warnings), "s" if len(log.warnings) > 1 else "")) if log.warnings else ""))

            if args.lua_code:
                # Epilogue
                for ns in INTERFACE_NAMESPACES:
                    GenerateLuaData(Emitter(lua_file, INDENT_SIZE), lua_interfaces, lua_enums)
                    log.Info("Created %s (%s interfaces, %s enums)" % (lua_file.name, len(lua_interfaces), len(lua_enums)))

        else:
            log.Info("Nothing to do")

        sys.exit(1 if len(log.errors) else 0)
