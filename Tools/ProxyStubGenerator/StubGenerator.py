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
# WPE Framework proxy stub code generator
#

import re
import sys
import os
import argparse
import copy
import glob
import CppParser
from collections import OrderedDict
import Log
import traceback

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
STUB_NAMESPACE = "::WPEFramework::ProxyStubs"
INTERFACE_NAMESPACE = "::WPEFramework"
CLASS_IUNKNOWN = "::WPEFramework::Core::IUnknown"
PROXYSTUB_CPP_NAME = "ProxyStubs_%s.cpp"

ENABLE_CUSTOM_ALLOCATOR = False
ENABLE_INSTANCE_VERIFICATION = False
ENABLE_RESTRICT = False

MIN_INTERFACE_ID = 64
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
def FindInterfaceClasses(tree):
    interfaces = []

    def __Traverse(tree, faces):
        if isinstance(tree, CppParser.Namespace) or isinstance(tree, CppParser.Class):
            for c in tree.classes:
                if not isinstance(c, CppParser.TemplateClass):
                    if (c.full_name.startswith(INTERFACE_NAMESPACE + "::")):
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
                                            faces.append(Interface(c, item.value, source_file))
                                            has_id = True
                                            break

                            if not has_id and not c.omit:
                                log.Warn("class %s does not have an ID enumerator" % c.full_name, source_file)

                    elif not c.omit:
                        log.Info("class %s not in %s namespace" % (c.full_name, INTERFACE_NAMESPACE), source_file)

                __Traverse(c, faces)

        if isinstance(tree, CppParser.Namespace):
            for n in tree.namespaces:
                __Traverse(n, faces)

    __Traverse(tree, interfaces)
    return interfaces


# Cut out scope resolution operators from all identifiers found in a string
def Flatten(identifier, scope=INTERFACE_NAMESPACE):
    assert scope.startswith("::")
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
def GenerateLuaData(emit, interfaces_list, enums_list, source_file, includePaths = [], defaults = "", extra_includes = []):

    if not source_file:
        emit.Line("-- enums")

        for k,v in lua_enums.items():
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

    files = []

    if defaults:
        files.append(defaults)

    files.extend(extra_includes)
    files.append(source_file)

    tree = CppParser.ParseFiles(files, includePaths, log)

    if not isinstance(tree, CppParser.Namespace):
        raise SkipFileError(source_file)

    interfaces = FindInterfaceClasses(tree)
    if not interfaces:
        raise NoInterfaceError(source_file)

    if not interfaces_list:
        emit.Line("-- Interfaces definition data file")
        emit.Line("-- Generated automatically. DO NOT EDIT")
        emit.Line()
        emit.Line("INTERFACES, METHODS, ENUMS, Type = ...")
        emit.Line()

    emit.Line("--  %s" % os.path.basename(source_file))

    iface_namespace_l = INTERFACE_NAMESPACE.split("::")
    iface_namespace = iface_namespace_l[-1]

    for iface in interfaces:
        iface_name = Flatten(iface.obj.type)

        interfaces_var_name = "INTERFACES"
        methods_var_name = "METHODS"

        assume_hresult = iface.obj.is_json

        if not iface.obj.full_name.startswith(INTERFACE_NAMESPACE):
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
        emit_methods = [m for m in iface.obj.methods if m.IsPureVirtual()]

        for idx, m in enumerate(emit_methods):
            name = "name = \"%s\"" % m.name
            params = []
            retval = []
            items = [ name ]

            def ParseLength(param, length, vars):
                def _Convert(size):
                    if size == "char":
                        return "8"
                    elif size == "long":
                        return "32"
                    else:
                        return "16"

                if len(length) == 1:
                    if length[0] == "void":
                        return [_Convert(param.Type().size), None]

                    for v in vars:
                        if v.name == length[0]:
                            return [_Convert(v.type.Type().size), v.name]

                size = "8"

                expr = "".join(length)
                try:
                    value = eval(expr)
                    if value > 0xFF:
                        size = "16"
                    elif value > 0xFFFF:
                        size = "32"
                except:
                    pass

                return [size, None]


            def Convert(paramtype, retval, vars, hresult=False):
                if isinstance(paramtype.type, list):
                    return

                param = paramtype.type.Resolve()
                meta = paramtype.meta
                p = param.Type()

                if isinstance(p, CppParser.Integer):
                    length_param = None

                    if param.IsPointer():
                        parsed = ParseLength(param, meta.length if meta.length else meta.maxlength, vars)
                        if parsed[1]:
                            length_param = parsed[1]
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

                    return rvalue

                elif isinstance(p, CppParser.String):
                    return ["type = Type.STRING"]

                elif isinstance(p, CppParser.Bool):
                    return ["type = Type.BOOL"]

                elif isinstance(p, CppParser.Class):
                    if param.IsPointer():
                        return ["type = Type.OBJECT", "class = \"%s\"" % Flatten(param.TypeName())]
                    else:
                        value = ["type = Type.POD", "class = \"%s\"" % Flatten(param.type.full_name)]
                        pod_params = []
                        for v in p.vars:
                            param_info = Convert(v, None, p.vars)
                            text = []
                            text.append("name = " + v.name)
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

                    name = Flatten(param.type.full_name)
                    if name not in enums_list:
                        data = dict()
                        for e in p.items:
                            data[e.value] = e.name
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

                if m.retval.name and "__unnamed" not in m.retval.name:
                    text.append("name = \"%s\"" % m.retval.name)

                text.extend(rv)

                retval.append(" { %s }" % ", ".join(text))

            for p in m.vars:
                param = Convert(p, m.retval, m.vars)

                if param:
                    text = []

                    if p.name and "__unnamed" not in p.name:
                        text.append("name = \"%s\"" % p.name)

                    text.extend(param)

                    skip = False
                    for v in m.vars:
                        if (v.meta.length and (v.meta.length[0] == p.name)) and not v.meta.maxlength:
                            # Will not be on the wire!
                            skip = True

                    if not skip:
                        if p.meta.input or not p.meta.output:
                            if not skip:
                                params.append(" { %s }" % ", ".join(text))

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


def GenerateStubs2(output_file, source_file, includePaths = [], defaults = "", extra_includes = [], scan_only=False):
    log.Info("Parsing '%s'..." % source_file)

    if not FORCE and (os.path.exists(output_file) and (os.path.getmtime(source_file) < os.path.getmtime(output_file))):
        raise NotModifiedException(output_file)

    files = []

    if defaults:
        files.append(defaults)

    files.extend(extra_includes)
    files.append(source_file)

    tree = CppParser.ParseFiles(files, includePaths, log)
    if not isinstance(tree, CppParser.Namespace):
        raise SkipFileError(source_file)

    interfaces = FindInterfaceClasses(tree)
    if not interfaces:
        raise NoInterfaceError(source_file)

    if scan_only:
        return interfaces

    interface_header_name = os.path.basename(source_file)

    interface_namespace = INTERFACE_NAMESPACE.split("::")[-1]

    with open(output_file, "w") as file:
        emit = Emitter(file, INDENT_SIZE)

        emit.Line("//")
        emit.Line("// generated automatically from \"%s\"" % interface_header_name)
        emit.Line("//")
        emit.Line("// implements COM-RPC proxy stubs for:")

        for face in interfaces:
            if not face.obj.omit:
                emit.Line("//   - %s" % Flatten(str(face.obj)))

        emit.Line("//")

        if ENABLE_RESTRICT:
            emit.Line("// secure code enabled")
            emit.Line("//")

        emit.Line
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

        announce_list = OrderedDict()

        class AuxIdentifier():
            def __init__(self, type, qualifiers, name, value=None):
                self.type = CppParser.Type(type)
                self.type.ref = qualifiers
                self.name = name
                self.proto_no_cv = self.type.Proto("nocv|noref")
                self.proto = self.type.Proto("noref")
                self.value = value

            @property
            def as_temporary(self):
                return (self.type.Proto() + " " + self.name)

            @property
            def as_temporary_no_cv(self):
                return (self.type.Proto("nocv") + " " + self.name)

            @property
            def as_lvalue(self):
                return self.name

            @property
            def as_rvalue(self):
                return self.value if self.value else self.name

        class EmitIdentifier():
            def __init__(self, index, interface, identifier, override_name=None, suppress_type=False):

                def _FindLength(length_name, variable_name):
                    if length_name:
                        if length_name[0] == "void":
                            return EmitIdentifier(-2, interface, \
                                CppParser.Temporary(self.identifier.parent, ["uint8_t", variable_name], ["sizeof(%s)" % self.kind], []), variable_name)

                        elif length_name[0] == "sizeof" and len(length_name) == 4:
                            matches = [v for v in self.identifier.parent.vars if v.name == length_name[2]]
                            if matches:
                                return EmitIdentifier(-2, interface, \
                                    CppParser.Temporary(self.identifier.parent, ["uint8_t", variable_name], ["sizeof(_%s)" % matches[0].name], []), variable_name)
                            else:
                                return EmitIdentifier(-2, interface, \
                                    CppParser.Temporary(self.identifier.parent, ["uint8_t", variable_name], ["".join(length_name)], []), variable_name,)

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

                                return EmitIdentifier(-2, interface, \
                                    CppParser.Temporary(self.identifier.parent, [length_type, variable_name], [str(value)], []), variable_name)

                            except (SyntaxError, NameError):
                                raise TypenameError(self.identifier, "'%s': unable to parse this length expression ('%s')" % (self.trace_proto, "".join(length_name)))
                        else:
                            return EmitIdentifier(-1, interface, matches[0])

                    return None

                if not isinstance(identifier.type, CppParser.Type):
                    undefined = CppParser.Type(CppParser.Undefined(identifier.type))
                    if not identifier.parent.stub and not identifier.parent.omit:
                        raise TypenameError(identifier, "'%s': undefined type" % Flatten(str(" ".join(identifier.type))))
                    else:
                        type_ = undefined
                else:
                    type_ = copy.deepcopy(identifier.type)

                is_const_length_of = (index == -2)
                is_variable_length_of = (index == -1)

                self.identifier = identifier
                self.identifier_type = type_
                self.identifier_kind = self.identifier_type.Type()
                self.is_output = (identifier.meta.output or (override_name == "result"))
                self.is_input = (identifier.meta.input or not self.is_output)

                # Declare input parameters 'const' even if 'const' is not on the original signature
                self.is_const = not identifier.meta.output

                # Yields identifier name as in the soruce code, meaningful for a human
                self.trace_proto = Flatten(self.identifier.Signature())

                if not self.identifier.name.startswith("__unnamed") and \
                        (self.identifier.name.startswith("_") or self.identifier.name.endswith("_")):
                    raise TypenameError(self.identifier, "%s: identifier names starting or ending with an underscore are reserved by the generator" % \
                                            self.trace_proto)

                # Always resovlve the type in typedefs...
                additional_ref = (CppParser.Ref.CONST if self.is_const else 0)
                self.type = self.identifier_type.Resolve(additional_ref)
                self.kind = self.type.Type()

                is_class = isinstance(self.kind, CppParser.Class)

                # Length variables are actually not put on wire
                self.length_of = None
                self.max_length_of = None

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

                if not override_name and self.length_of and not self.max_length_of:
                    override_name = (self.length_of.name + "Len")

                name = (override_name or ("_" + self.identifier.name).replace("__unnamed_", ""))
                self.value = self.identifier.value

                self.interface_id = _FindLength(self.identifier.meta.interface, (name[1:] + "IntefaceId"))

                # Is it  a buffer?
                is_buffer = (self.type.IsPointer() and not is_class and not self.interface_id)
                self.length = _FindLength(self.identifier.meta.length, (name[1:] + "Len"))
                self.max_length = _FindLength(self.identifier.meta.maxlength, (name[1:] + "Len"))

                if (is_buffer and not self.length and self.is_input):
                    raise TypenameError(self.identifier, "'%s': an input raw buffer requires a @length tag" % self.trace_proto)

                if (is_buffer and (not self.length and not self.max_length) and self.is_output):
                    raise TypenameError(self.identifier, "'%s': an output-only raw buffer requires a @maxlength tag" % self.trace_proto)

                self.is_buffer = ((self.length or self.max_length) and is_buffer)
                if self.is_buffer:
                    pass

                # If have a input length, assume it's a max-length parameter even if not stated explicitly
                if (self.is_buffer and not self.max_length and self.length.is_input):
                    self.max_length = self.length
                    if self.is_output and not self.is_input and not self.length.is_output:
                        # It appears it should've been @maxlength instead of @length... Fix this
                        self.length = None
                        log.WarnLine(self.identifier, "'%s': this buffer is output-only, hence expected @maxlength here, not @length" % self.trace_proto)
                    elif self.is_input and self.is_output:
                        log.WarnLine(self.identifier, \
                            "'%s': maximum length of this input/output buffer is assumed to be same as @length, use @maxlength to disambiguate" % \
                                (self.trace_proto))

                if (self.is_buffer and self.is_output and not self.max_length):
                    raise TypenameError(self.identifier, "'%s': can't deduce maximum length of the buffer, use @maxlength" % self.trace_proto)

                if (self.is_buffer and self.max_length and not self.length):
                    log.WarnLine(self.identifier, "'%s': actual length of returned buffer is implicit" % self.trace_proto)

                # Is it a hresult?
                self.is_hresult = self.identifier_type.TypeName().endswith("Core::hresult") \
                                    or (isinstance(self.kind, CppParser.Integer) and self.kind.size == "long" and interface.obj.is_json)

                # Is it a POD?
                # If forward class definition is encountered it will be assumed it's an interface not a POD
                self.is_compound = (is_class and not self.kind.IsVirtual() and (len(self.kind.vars) > 0))

                # Is it an interface instance?
                is_iunknown_descendant = is_class #and self.kind.Inherits(CLASS_IUNKNOWN)
                is_unspecified_interface_instance = (isinstance(self.kind, CppParser.Void) and self.identifier.meta.interface)
                has_proxy = not self.is_compound and (is_class and is_iunknown_descendant and self.type.IsPointer()) and self.is_input
                has_output_proxy = not self.is_compound and \
                    (self.is_output and self.type.IsPointer() and (is_iunknown_descendant or is_unspecified_interface_instance))

                self.proxy = None
                self.return_proxy = False
                self.proxy_instance = None

                if has_proxy:
                    # Have to use instance_id instead of the class name
                    self.type_name = Flatten(self.identifier_type.TypeName())
                    self.proto = (("const " if self.is_const else "") + INSTANCE_ID).strip()
                    self.proto_no_cv = INSTANCE_ID
                else:
                    self.type_name = Flatten(self.identifier_type.TypeName())
                    self.proto = Flatten(self.identifier_type.Proto("noref"))
                    self.proto_no_cv = Flatten(self.identifier_type.Proto("nocv|noref"))

                if has_proxy:
                    self.name = ("%sImplementation" % name[1:])
                    self.proxy = ("%sProxy" % name[1:])
                    self.proxy_instance = name
                else:
                    self.name = name

                if has_output_proxy:
                    self.return_proxy = True
                    if not self.interface_id:
                        # Pick up interface ID from the type itself
                        self.interface_id = AuxIdentifier(CppParser.Integer("uint32_t"), \
                                (CppParser.Ref.VALUE | CppParser.Ref.CONST), "id", (self.type_name + "::ID"))

                self.restrict_range = None
                if ENABLE_RESTRICT and self.max_length_of:
                    self.restrict_range = identifier.meta.range
                    if not self.restrict_range:
                        if self.length and self.length.identifier.meta.range:
                            self.restrict_range = self.length.identifier.meta.range
                        elif self.max_length and self.max_length.identifier.meta.range:
                            self.restrict_range = self.max_length.identifier.meta.range
                        elif self.length_of and self.length_of.meta.range:
                            self.restrict_range = self.length_of.meta.range
                        elif self.max_length_of and self.max_length_of.meta.range:
                            self.restrict_range = self.max_length_of.meta.range

                # Used with PODs
                self.suppress_type = suppress_type

                # Now do some more sanity checking on what we've got!...
                if not override_name == "result" and (self.is_output and self.return_proxy and not self.type.IsReference()):
                    raise TypenameError(self.identifier, "'%s': output interface must be a reference to a pointer" % self.trace_proto)

                if self.is_compound and self.type.IsPointer():
                    raise TypenameError(self.identifier, "'%s': C-style POD array is not supported, use an iterator" % self.trace_proto)

                if self.length:
                    if not self.is_buffer:
                        raise TypenameError(self.identifier, "'%s': @length tag is not allowed for this type" % self.trace_proto)

                if self.length_of:
                    if not self.is_input:
                        raise TypenameError(self.identifier, "'%s': a length parameter must not be write-only" % self.trace_proto)
                    elif self.is_buffer:
                        raise TypenameError(self.identifier, "'%s': this type cannot be a buffer length carrying parameter" % self.trace_proto)

                if self.max_length:
                    if not self.is_buffer:
                        raise TypenameError(self.identifier, "'%s': @maxlength tag is not allowed for this type" % self.trace_proto)
                    elif not self.is_buffer:
                        raise TypenameError(self.identifier, "'%s': this type cannot be a buffery length carrying parameter" % self.trace_proto)

                if self.max_length_of:
                    if not self.is_input:
                        raise TypenameError(self.identifier, "'%s': a max-length parameter must not be write-only" % self.trace_proto)

                if self.type.IsReference():
                    if not self.identifier_type.IsConst() and not self.identifier.meta.output:
                        raise TypenameError(self.identifier, "'%s': non-const reference requires an @out tag" % self.trace_proto)
                    elif not self.identifier_type.IsConst() and self.identifier.meta.input and not self.identifier.meta.output:
                        log.WarnLine(identifier, "'%s': read-only parameter is missing a const qualifier" % self.trace_proto)
                    elif self.identifier_type.IsConst() and self.identifier.meta.output:
                        raise TypenameError(identifier, "'%s': output parameter must not be const" % self.trace_proto)

                if self.is_buffer:
                    if not self.identifier_type.IsPointerToConst() and not self.identifier.meta.input and not self.identifier.meta.output:
                        raise TypenameError(self.identifier, "'%s': pointer to non-const data requires an @in or @out tag" % self.trace_proto)
                    elif not self.identifier_type.IsPointerToConst() and self.identifier.meta.input and not self.identifier.meta.output:
                        log.WarnLine(identifier, "'%s': read-only parameter is missing a const qualifier" % self.trace_proto)
                    elif self.identifier_type.IsPointerToConst() and self.identifier.meta.output:
                        raise TypenameError(identifier, "'%s': output parameter must not be const" % self.trace_proto)

                if self.length_of or is_const_length_of:
                    if not isinstance(self.kind, CppParser.Integer):
                        raise TypenameError(self.identifier, "'%s': a buffer length carrying parameter must be an integer" % self.trace_proto)
                    elif self.kind.size == "long long":
                        raise TypenameError(self.identifier, "'%s': 64-bit buffer length carrying parameter is not supported" % self.trace_proto)
                    elif (index != -1) and self.kind.size == "long":
                        log.WarnLine(self.identifier, "'%s': long int buffer length is supported, but buffers up to 64 KB are recommended" % self.trace_proto)

                if is_variable_length_of and not self.is_buffer and isinstance(self.kind, (CppParser.Integer, CppParser.BuiltinInteger)):
                    if not self.kind.IsFixed():
                        log.WarnLine(self.identifier, "'%s': integer is not fixed-width, use a stdint type" % self.trace_proto)

                if isinstance(self.kind, CppParser.Enum):
                    if not self.kind.type.Type().IsFixed():
                        log.WarnLine(self.identifier, "'%s': underlying type of enumeration is not fixed-width integer, use a stdint type" % self.trace_proto)

            @property
            def as_rvalue(self):
                def maybe_cast(expr):
                    if not self.value and self.type.IsReference():
                        if self.proxy and self.type.IsConstPointer():
                            return "static_cast<%s%s* const&>(%s)" % (("const " if self.type.IsPointerToConst() else ""), self.type_name, expr)
                        elif self.identifier_type.IsConst():
                            return "static_cast<const %s&>(%s)" % (self.type_name, expr)
                    return expr

                return maybe_cast(self.value if self.value else self.as_in_prototype)

            @property
            def as_in_prototype(self):
                return self.proxy_instance if self.proxy_instance else self.name

            @property
            def as_lvalue(self):
                return self.name

            @property
            def as_temporary(self):
                return self.name if self.suppress_type else (self.proto + " " + self.name)

            @property
            def as_temporary_no_cv(self):
                return self.name if self.suppress_type else (self.proto_no_cv + " " + self.name)

            @property
            def read_rpc_type(self):
                if self.proxy_instance or self.return_proxy:
                    return "Number<%s>()" % INSTANCE_ID
                elif self.is_buffer:
                    assert self.length or self.max_length, "Invalid type for buffer"
                    if self.length:
                        return "Buffer<%s>(%s, %s)" % (self.length.type_name, self.length.as_rvalue, self.as_lvalue)
                    elif self.max_length:
                        return "Buffer<%s>(%s, %s)" % (self.max_length.type_name, self.max_length.as_rvalue, self.as_lvalue)
                    else:
                        Unreachable()
                elif isinstance(self.kind, CppParser.Integer):
                    return "Number<%s>()" % self.type_name
                elif isinstance(self.kind, CppParser.String):
                    return "Text<uint32_t>()" if self.kind.is_cc else "Text()"
                elif isinstance(self.kind, CppParser.Bool):
                    return "Boolean()"
                elif isinstance(self.kind, CppParser.Enum):
                    return "Number<%s>()" % self.type_name
                elif isinstance(self.kind, CppParser.BuiltinInteger):
                    return "Number<%s>()" % self.type_name
                else:
                    raise TypenameError(self.identifier, "%s: sorry, unable to deserialise this type" % self.trace_proto)

            @property
            def write_rpc_type(self):
                if self.proxy_instance or self.return_proxy:
                    return "Number<%s>(RPC::instance_cast(%s))" % (INSTANCE_ID, self.as_rvalue)
                elif self.is_buffer:
                    assert self.max_length, "Invalid type for buffer " + self.name
                    return "Buffer<%s>(%s, %s)" % (self.max_length.type_name, self.max_length.as_rvalue, self.as_rvalue)
                elif isinstance(self.kind, CppParser.Integer):
                    return "Number<%s>(%s)" % (self.type_name, self.as_rvalue)
                elif isinstance(self.kind, CppParser.String):
                    return "Text(%s)" % self.as_rvalue
                elif isinstance(self.kind, CppParser.Bool):
                    return "Boolean(%s)" % self.as_rvalue
                elif isinstance(self.kind, CppParser.Enum):
                    return "Number<%s>(%s)" % (self.type_name, self.as_rvalue)
                elif isinstance(self.kind, CppParser.BuiltinInteger):
                    return "Number<%s>(%s)" % (self.type_name, self.as_rvalue)
                else:
                    raise TypenameError(self.identifier, "%s: sorry, unable to serialise this type" % self.trace_proto)

            @property
            def storage_size(self):
                if self.proxy_instance or self.return_proxy:
                    return "sizeof(%s)" % INSTANCE_ID
                elif self.is_buffer:
                    return "sizeof(%s)" % self.length.type_name
                elif isinstance(self.kind, CppParser.String):
                    return "sizeof(%s)" % ("uin32_t" if self.kind.is_cc else "uint16_t")
                elif isinstance(self.kind, (CppParser.Integer, CppParser.Enum, CppParser.BuiltinInteger)):
                    return "sizeof(%s)" % self.type_name
                elif isinstance(self.kind, CppParser.Bool):
                    return 1
                else:
                    Unreachable()

            def __str__(self):
                return str(self.type)

            def __repr__(self):
                return str(self.type)

        class EmitParam(EmitIdentifier):
            def __init__(self, interface, identifier, name=None, suppress_type=False, index=-1):
                EmitIdentifier.__init__(self, index, interface, identifier, name, suppress_type)

        class EmitRetVal(EmitIdentifier):
            def __init__(self, interface, identifier, name="_result", suppress_type=False):
                EmitParam.__init__(self, interface, identifier, name, suppress_type)

        def EmitFunctionOrder(methods):
            if methods:
                emit.Line("// Methods:")

                for index, method in enumerate(methods):
                    emit.Line("//  (%i) %s" % (index, Flatten(method.Proto())))

                emit.Line("//")

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

        def PrepareParams(method, interface):
            input_params = []
            proxy_params = []
            return_proxy_params = []
            output_params = []
            retval = None
            params = []

            if method.retval:
                if isinstance(method.retval, list) or not method.retval.type.IsVoid():
                    retval = EmitRetVal(interface, method.retval, vars["result"])

            for index, var in enumerate(method.vars):
                params.append(EmitParam(interface, var, index = index))

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
                    if p.proxy:
                        proxy_params.append(p)

                # Length and max-length variables are never sent back
                if (p.is_output and (not p.length_of and not p.max_length_of)):
                    output_params.append(p)
                    if p.return_proxy:
                        return_proxy_params.append(p)

            log.Info("  %s, input = (%s), output = (%s)" % (method.name, ", ".join([p.as_in_prototype for p in input_params]), \
                        ", ".join([p.as_in_prototype if p else "void" for p in output_params])))

            return retval, params, input_params, output_params, proxy_params, return_proxy_params

        # Build the announce list upfront
        for interface in interfaces:
            obj = interface.obj
            interface_name = Flatten(obj.full_name)

            log.Info("Processing '%s' interface data..." % Flatten(obj.type))

            if not obj.full_name.startswith(INTERFACE_NAMESPACE + "::"):
                log.Info("class %s not in requested namespace (%s)" % (obj.full_name, INTERFACE_NAMESPACE))
            elif obj.omit:
                log.Info("omitting class %s" % interface_name)
            else:
                # Of course consider only virtual methods
                emit_methods = [method for method in obj.methods if method.IsVirtual() and not method.IsDestructor() and not method.omit]
                omitted_methods = [method for method in obj.methods if method.IsVirtual() and not method.IsDestructor() and method.omit]

                for method in omitted_methods:
                    log.Info("omitting method %s::%s()" % (interface_name, method.name))

                if not emit_methods:
                    log.Warn("no virtual methods in class %s" % interface_name)
                else:
                    stub_methods_name = CreateName(interface_name) + "StubMethods"
                    proxy_name = CreateName(interface_name) + "Proxy"
                    stub_name = CreateName(interface_name) + "Stub"
                    prepared_params = [PrepareParams(method, interface) for method in emit_methods]
                    announce_list[interface_name] = [emit_methods, stub_methods_name, stub_name, proxy_name, interface, prepared_params]

        #
        # EMIT STUB CODE
        #
        emit.Line("// -----------------------------------------------------------------")
        emit.Line("// STUBS")
        emit.Line("// -----------------------------------------------------------------\n")

        def EmitStubMethodImplementation(index, method, interface_name, interface, retval, params, \
                                        input_params, output_params, proxy_params, return_proxy_params):

            hresult = AuxIdentifier(CppParser.Integer(HRESULT), CppParser.Ref.VALUE, vars["hresult"])
            has_restricted_parameters = False

            output_buffers = []
            custom_buffers = []
            has_hresult = retval and retval.is_hresult

            def ReadParameter(p):
                def CheckFrame(p, int_size=None):
                    if ENABLE_RESTRICT:
                        if not int_size:
                            emit.Line()

                        emit.Line("if (%s.Length() < (%s)) { return (Core::ERROR_INVALID_INPUT_LENGTH); }" % \
                                    (vars["reader"], (int_size if int_size else p.storage_size)))

                def CheckSize(p, peek=False, int_size="uint16_t"):
                    nonlocal has_restricted_parameters
                    name = p.as_lvalue

                    if ENABLE_RESTRICT:
                        if peek:
                            emit.Line()
                            CheckFrame(p, "sizeof(%s)" % int_size)
                            name = (name.replace(".", "_") + "Size_")
                            emit.Line("const %s = %s.Peek<%s>();" % (name, vars["reader"], int_size))

                        if p.restrict_range:
                            has_restricted_parameters = True
                            emit.Line("if (!((%s >= %s) && (%s <= %s))) { return (Core::ERROR_INVALID_RANGE) }" % \
                                            (name, p.restrict_range[0], name, p.restrict_range[1]))

                        if peek:
                            CheckFrame(p, "%s + sizeof(%s)" % (name, int_size))

                if p.is_compound:
                    if not p.suppress_type:
                        emit.Line("%s{};" % p.as_temporary_no_cv)

                    params = [EmitParam(interface, v, (p.name + "." + v.name), True) for v in p.kind.vars]

                    for pp in params:
                        ReadParameter(pp)

                elif p.is_buffer:
                    CheckSize(p, True, p.length.type_name)
                    emit.Line("%s{};" % p.as_temporary_no_cv)
                    buffer_param = "const_cast<const %s*&>(%s)" % (p.type_name, p.as_rvalue) if not p.identifier_type.IsPointerToConst() else p.as_rvalue
                    emit.Line("%s = %s.LockBuffer<%s>(%s);" % (p.length.as_temporary, vars["reader"], p.length.type_name, buffer_param))
                    emit.Line("%s.UnlockBuffer(%s);" % (vars["reader"], p.length.name))

                elif isinstance(p.kind, CppParser.String):
                    CheckSize(p, True, "uint32_t" if p.kind.is_cc else "uint16_t")
                    emit.Line("%s = %s.%s;" % (p.as_temporary, vars["reader"], p.read_rpc_type))

                else:
                    CheckFrame(p)
                    emit.Line("%s = %s.%s;" % (p.as_temporary, vars["reader"], p.read_rpc_type))
                    CheckSize(p, False)

            def TemporaryParameter(p):
                if p.is_buffer:
                    output_buffers.append(p)

                if (p.is_output and not p.is_input):
                    emit.Line("%s{};" % p.as_temporary_no_cv)
                    return 1

                return 0

            def AllocateBuffer(p):
                assert p.is_buffer
                assert p.max_length

                has_same_buffer = (p.is_input and p.max_length and p.length and (p.max_length == p.length))

                if has_same_buffer:
                    return

                # May need to allocate custom memory to pass large buffer
                has_large_buffer_size = (p.max_length and (p.max_length.type.Type().size == "long"))
                has_large_buffer = (has_large_buffer_size and ENABLE_CUSTOM_ALLOCATOR)
                has_buffer_reuse = (p.is_input and p.max_length and p.length and (p.max_length != p.length))

                if has_large_buffer:
                    large_buffer = AuxIdentifier(CppParser.Void(), CppParser.Ref.POINTER, (p.name[1:] + "Custom"))
                    emit.Line("%s{};" % large_buffer.as_temporary_no_cv)

                emit.Line("if (%s != 0) {" % p.max_length.as_rvalue)
                emit.IndentInc()

                if has_buffer_reuse:
                    temp_buffer = AuxIdentifier(CppParser.Void(), CppParser.Ref.POINTER, vars["tempbuffer"])
                    emit.Line("if (%s > %s) {" % (p.max_length.as_rvalue, p.length.as_rvalue))
                    emit.IndentInc()
                    emit.Line("%s{};" % temp_buffer.as_temporary_no_cv)
                    emit.Line()

                if has_large_buffer:
                    custom_buffers.append([p, large_buffer])
                    emit.Line("%s = RPC::Administrator::Instance().Allocate(%s);" % (large_buffer.as_rvalue, p.max_length.as_rvalue))
                    emit.Line()
                    emit.Line("if (%s == nullptr) {" % large_buffer.as_rvalue)
                    emit.IndentInc()

                if has_large_buffer or (not ENABLE_CUSTOM_ALLOCATOR and has_large_buffer_size):
                    emit.Line("ASSERT(%s <= 0x1000000);" % p.max_length.as_rvalue)

                    if ENABLE_RESTRICT:
                        emit.Line("if (%s > 0x100000) { return (Core::ERROR_BAD_REQUEST); }" % p.max_length.as_rvalue)
                        emit.Line()

                if EMIT_TRACES:
                    emit.Line('fprintf(stderr, "*** Allocating %%u bytes on stack for a temporary buffer\\n", %s);' % p.max_length.as_rvalue)

                emit.Line("%s = static_cast<%s>(ALLOCA(%s));" % \
                    ((p.as_rvalue if not has_buffer_reuse else temp_buffer.as_rvalue), p.proto, p.max_length.as_rvalue))

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
                    emit.Line()

                if has_buffer_reuse:
                    if not has_large_buffer:
                        emit.Line()

                    emit.Line("if (%s != nullptr) {" % temp_buffer.as_rvalue)
                    emit.IndentInc()

                    emit.Line("::memcpy(%s, %s, %s);" % (temp_buffer.as_rvalue, p.as_rvalue, p.length.as_rvalue))
                    emit.IndentDec()
                    emit.Line("}")
                    emit.Line()
                    emit.Line("%s = static_cast<%s>(%s);" % (p.as_rvalue, p.proto, temp_buffer.as_rvalue))
                    emit.IndentDec()
                    emit.Line("}")
                    emit.Line()

                if ENABLE_RESTRICT:
                    emit.Line()
                    emit.Line("if (%s == nullptr) { return (Core::ERROR_GENERAL); }" % p.as_rvalue)
                else:
                    emit.Line("ASSERT(%s != nullptr);" % p.as_rvalue)

                emit.IndentDec()
                emit.Line("}")

            def ReleaseBuffer(p, large_buffer):
                assert p.is_buffer
                if (p.max_length and (p.length.type.Type().size == "long")):
                    emit.Line("RPC::Administrator::Instance().Free(%s);" % large_buffer.as_rvalue)

            def WriteParameter(p):
                if p.is_compound:
                    params = [EmitParam(interface, v, (p.name + "." + v.name)) for v in p.kind.vars]
                    for pp in params:
                        WriteParameter(pp)
                else:
                    emit.Line("%s.%s;" % (vars["writer"], p.write_rpc_type))

            def AcquireInterface(p):
                assert p.proxy
                assert p.proxy_instance
                emit.Line("%s* %s = nullptr;" %(p.type_name, p.proxy_instance))
                emit.Line("ProxyStub::UnknownProxy* %s = nullptr;" % p.proxy)
                emit.Line("if (%s != 0) {" % p.name)
                emit.IndentInc()
                emit.Line("%s = RPC::Administrator::Instance().ProxyInstance(%s, %s, false, %s);" % \
                            (p.proxy, vars["channel"], p.name, p.proxy_instance))

                if ENABLE_RESTRICT:
                    emit.Line()
                    emit.Line("if ((%s == nullptr) || (%s == nullptr)) { return (Core::ERROR_NOT_EXIST); }" % (p.proxy_instance, p.proxy))
                else:
                    emit.Line("ASSERT((%s != nullptr) && (%s != nullptr));" % (p.proxy_instance, p.proxy))

                emit.IndentDec()
                emit.Line("}")

            def ReleaseProxy(p):
                assert p.proxy
                emit.Line("if (%s != nullptr) {" % p.proxy)
                emit.IndentInc()
                emit.Line("RPC::Administrator::Instance().Release(%s, %s->Response());" % (p.proxy, vars["message"]))
                emit.IndentDec()
                emit.Line("}")

            def RegisterInterface(p):
                assert p.interface_id
                if not isinstance(p.interface_id, AuxIdentifier):
                    # Interface ID comes from a parameter
                    emit.Line("RPC::Administrator::Instance().RegisterInterface(%s, %s, %s);" % (vars["channel"], p.name, p.interface_id.as_rvalue))
                else:
                    emit.Line("RPC::Administrator::Instance().RegisterInterface(%s, %s);" % (vars["channel"], p.as_rvalue))

            def CallImplementation(retval, params):
                parameters = ", ".join([p.as_rvalue for p in params])
                lhs = (((retval.as_temporary if not retval.return_proxy else retval.as_temporary_no_cv) + " = ") if retval else "")
                emit.Line("%s%s->%s(%s);" % (lhs, vars["implementation"], method.name, parameters))

            if ENABLE_RESTRICT:
                emit.Line("%s = Core::ERROR_NONE;" % hresult.as_temporary)
                emit.Line()

                lambda_params = []
                if proxy_params or return_proxy_params:
                    lambda_params.append("&" + vars["channel"])

                lambda_params.append("&" + vars["message"])

                emit.Line("%s = [%s]() -> %s {" % (hresult.as_lvalue, ", ".join(lambda_params), hresult.proto_no_cv))
                emit.IndentInc()

            if interface.obj.is_iterator:
                face_name = vars["interface"]
                emit.Line("using %s = %s;" % (vars["interface"], Flatten(interface.obj.type)))
                emit.Line()
            else:
                face_name = interface_name

            if ENABLE_RESTRICT:
                emit.Line("if (%s->Parameters().Length() < ((sizeof(%s) + sizeof(uint32_t) + 1)) { return (Core::ERROR_INVALID_INPUT_LENGTH); }" % (vars["message"], INSTANCE_ID))
                emit.Line()

            implementation_type = (method.CVString() + " " + face_name).strip()
            emit.Line("%s* %s = reinterpret_cast<%s*>(%s->Parameters().Implementation());" % (implementation_type, vars["implementation"], implementation_type, vars["message"]))

            if ENABLE_RESTRICT:
                if ENABLE_INSTANCE_VERIFICATION:
                    emit.Line("if (RPC::Administrator::Instance().IsValid(%s) == false) { return (Core::ERROR_NOT_EXIST); }" % vars["implementation"])
                else:
                    emit.Line("if (%s == nullptr) { return (Core::ERROR_NOT_EXIST); }" % vars["implementation"])
            else:
                emit.Line("ASSERT(%s != nullptr);" % (vars["implementation"]))

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

            if ENABLE_RESTRICT:
                emit.Line()
                emit.Line("return (Core::ERROR_NONE);")

                emit.IndentDec()
                emit.Line("} ();")

                emit.Line()
                emit.Line("if (%s != Core::ERROR_NONE) {" % hresult.as_rvalue)
                emit.IndentInc()

                if has_hresult:
                    emit.Line("RPC::Data::Frame::Writer %s(%s->Response().Writer());" % (vars["writer"], vars["message"]))
                    emit.Line("%s.Number<uint32_t>(0x80000000 | %s);" % (vars["writer"], hresult.as_rvalue))

                emit.Line("fprintf(stderr, \"COM-RPC outbound call failed: %%u!\\n\", %s);" % hresult.as_rvalue)

                if not has_hresult:
                    emit.Line("fprintf(stderr, \"Warning: This COM-RPC failure cannot be detected by the caller!\\n\");")

                emit.Line("ASSERT(!\"COM-RPC failure\");")

                emit.IndentDec()
                emit.Line("}")

            if has_restricted_parameters and (not retval or not retval.is_hresult):
                log.WarnLine(retval.identifier, "'%s': method is using restricted parameters but its return value type is not 'Core::hresult'" % method.name)

        def EmitStubMethod(index, last, method, interface_name, interface, prepared_params):
            retval, params, input_params, output_params, proxy_params, return_proxy_params = prepared_params

            channel = "/* channel */" if method.stub or (not proxy_params and not return_proxy_params) else "channel"
            message = "/* message */" if method.stub else "message"
            emit.Line("// (%i) %s" % (index, Flatten(method.Proto())))
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
                log.Print("Emitting stub code for interface %s..." % Flatten(interface.obj.type))

            # Emit class summary
            emit.Line("//")
            emit.Line("// %s interface stub definitions" % (interface_name))
            emit.Line("//")
            EmitFunctionOrder(methods)
            emit.Line()

            emit.Line("ProxyStub::MethodHandler %s[] = {" % stub_methods_name)
            emit.IndentInc()

            for index, method in enumerate(methods):
                last = (index == len(methods) - 1)
                EmitStubMethod(index, last, method, interface_name, interface, prepared_params[index])

                if not last:
                    emit.Line()

            emit.IndentDec()
            emit.Line("}; // %s" % stub_methods_name)
            emit.Line()

        for name, element in announce_list.items():
            EmitStub(name, element[0], element[1], element[4], element[5])


        #
        # EMIT PROXY CODE
        #

        emit.Line("// -----------------------------------------------------------------")
        emit.Line("// PROXIES")
        emit.Line("// -----------------------------------------------------------------\n")

        def EmitProxyMethodImplementation(index, method, interface_name, interface, retval, params,
                input_params, output_params, proxy_params, return_proxy_paramse):

            if EMIT_TRACES:
                emit.Line('fprintf(stderr, "*** [%s proxy] ENTER: %s()\\n");' % (interface_name, method.name))
                emit.Line()

            emit.Line("IPCMessage %s(BaseClass::Message(%s));" % (vars["message"], index))
            emit.Line()

            hresult = AuxIdentifier(CppParser.Integer(HRESULT), (CppParser.Ref.VALUE), vars["hresult"])

            def WriteParameter(p):
                if p.is_compound:
                    params = [EmitParam(interface, v, (p.name + "." + v.name), True) for v in p.kind.vars]

                    for pp in params:
                        WriteParameter(pp)
                else:
                    emit.Line("writer.%s;" % p.write_rpc_type)

            def ReadParameter(p):
                if p.is_compound:
                    params = [EmitParam(interface, v, (p.name + "." + v.name), True) for v in p.kind.vars]

                    for pp in params:
                        ReadParameter(pp)
                elif p.return_proxy:
                    emit.Line("%s = reinterpret_cast<%s>(Interface(%s.%s, %s));" % \
                              (p.as_lvalue, p.proto_no_cv, vars["reader"], p.read_rpc_type, p.interface_id.as_rvalue))
                elif p.is_buffer:
                    if p.length:
                        emit.Line("%s = %s.%s;" % (p.length.as_lvalue, vars["reader"], p.read_rpc_type))
                    else:
                        # No one's interested in the return length, perhaps it's sent via method's return value
                        emit.Line("%s.%s;" % (vars["reader"], p.read_rpc_type))
                else:
                    emit.Line("%s = %s.%s;" % (p.as_lvalue, vars["reader"], p.read_rpc_type))

            if input_params:
                emit.Line("RPC::Data::Frame::Writer %s(%s->Parameters().Writer());" % (vars["writer"], vars["message"]))
                for p in input_params:
                    WriteParameter(p)

                emit.Line()

            reuse_hresult = (retval and retval.is_hresult)
            needs_reader = ((len(output_params) > 1) or output_params[0]) or proxy_params

            if not reuse_hresult and retval:
                emit.Line("%s{};" % retval.as_temporary_no_cv)
                emit.Line()

            lhs = (("const " if not reuse_hresult else "") + hresult.as_temporary_no_cv + " = ") if needs_reader else ""
            emit.Line("%sProxyStub::UnknownProxyType<%s>::Invoke(%s);" % (lhs, Flatten(interface.obj.type), vars["message"]))

            if needs_reader:
                emit.Line("if (%s == Core::ERROR_NONE) {" % hresult.as_rvalue)
                emit.IndentInc()
                emit.Line("RPC::Data::Frame::Reader %s(%s->Response().Reader());" % (vars["reader"], vars["message"]))

            if retval:
                if reuse_hresult:
                    emit.Line("%s = %s.%s;" % (hresult.as_lvalue, vars["reader"], retval.read_rpc_type))

                    if len(output_params) > 1:
                        emit.Line("if ((%s & 0x80000000) == 0) {" % hresult.as_rvalue)
                        emit.IndentInc()
                else:
                    ReadParameter(retval)

            if len(output_params) > 1:
                for p in output_params[1:]:
                    ReadParameter(p)

            if reuse_hresult and (len(output_params) > 1):
                emit.IndentDec()
                emit.Line("}")

            if proxy_params:
                emit.Line("Complete(%s);" % vars["reader"])

            if needs_reader:
                emit.IndentDec()

            if reuse_hresult:
                emit.Line("} else {")
                emit.IndentInc()
                emit.Line("%s |= 0x80000000;" % hresult.as_rvalue)
                emit.IndentDec()

            if needs_reader:
                emit.Line("}")

            if EMIT_TRACES:
                emit.Line()
                emit.Line('fprintf(stderr, "*** [%s proxy] EXIT: %s()\\n");' % (interface_name, method.name))

            if retval:
                emit.Line()
                emit.Line("return (%s);" % (hresult.name if reuse_hresult else retval.name))


        def EmitProxyMethod(index, method, interface_name, interface, prepared_params):
            retval, params, input_params, output_params, proxy_params, return_proxy_params = prepared_params

            method_type = ((Flatten(retval.identifier.Proto()) + " ") if retval else "void ")
            method_qualifiers = ((" const" if "const" in method.qualifiers else "") + " override")
            params_list = [(Flatten(p.identifier.Proto()) + ((" " + p.as_in_prototype) if not method.stub else "")) for p in params]
            emit.Line("%s%s(%s)%s" % (method_type, method.name, ", ".join(params_list), method_qualifiers))
            emit.Line("{")
            emit.IndentInc()

            if not method.stub:
                EmitProxyMethodImplementation(index, method, interface_name, interface, retval, params, \
                                                input_params, output_params, proxy_params, return_proxy_params)

            if method.stub:
                if EMIT_TRACES:
                    emit.Line('fprintf(stderr, "*** [%s proxy] STUBBED METHOD: %s()\\n");' % (interface_name, method.name))
                    emit.Line()

                emit.Line("// stubbed method, no implementation")
                if retval:
                    emit.Line()
                    if retval.is_hresult:
                        emit.Line("return (%s{0x80000000 | Core::ERROR_UNAVAILABLE});" % retval.type_name)
                    else:
                        emit.Line("return {};")

            emit.IndentDec()
            emit.Line("}")
            emit.Line()

        def EmitProxy(interface_name, methods, proxy_name, interface, prepared_params):
            if BE_VERBOSE:
                log.Print("Emitting proxy code for interface %s..." % Flatten(interface.obj.type))

            # Emit class summary
            emit.Line("//")
            emit.Line("// %s interface proxy definitions" % (interface_name))
            emit.Line("//")
            EmitFunctionOrder(methods)
            emit.Line()

            # Emit proxy class definition
            emit.Line("class %s final : public ProxyStub::UnknownProxyType<%s> {" % (proxy_name, Flatten(interface.obj.type)))
            emit.Line("public:")
            emit.IndentInc()

            # Emit constructor
            emit.Line("%s(const Core::ProxyType<Core::IPCChannel>& channel, const %s implementation, const bool otherSideInformed)" % (proxy_name, INSTANCE_ID))
            emit.Line("    : BaseClass(channel, implementation, otherSideInformed)")
            emit.Line("{")
            emit.Line("}")
            emit.Line()

            for index, method in enumerate(methods):
                EmitProxyMethod(index, method, interface_name, interface, prepared_params[index])

            emit.IndentDec()
            emit.Line("}; // class %s" % proxy_name)
            emit.Line()

        for name, element in announce_list.items():
            EmitProxy(name, element[0], element[3], element[4], element[5])

        if BE_VERBOSE:
            log.Print("Emitting stub registration code...")


        #
        # EMIT REGISTRATION CODE
        #
        emit.Line("// -----------------------------------------------------------------")
        emit.Line("// REGISTRATION")
        emit.Line("// -----------------------------------------------------------------")
        emit.Line()
        emit.Line("namespace {")
        emit.IndentInc()

        emit.Line()

        for _, [_, methods, stub, _, interface, _ ] in announce_list.items():
            emit.Line("typedef ProxyStub::UnknownStubType<%s, %s> %s;" % (Flatten(interface.obj.type), methods, stub))

        emit.Line()

        emit.Line("static class Instantiation {")
        emit.Line("public:")
        emit.IndentInc()
        emit.Line("Instantiation()")
        emit.Line("{")
        emit.IndentInc()

        if EMIT_TRACES:
            emit.Line("fprintf(stderr, \"Announcing %s interface methods...\\n\");" % Flatten(interface.obj.type))

        for _, [_, _, stub, proxy, interface, _ ]  in announce_list.items():
            emit.Line("RPC::Administrator::Instance().Announce<%s, %s, %s>();" % (Flatten(interface.obj.type), proxy, stub))

        if EMIT_TRACES:
            emit.Line("fprintf(stderr, \"Announcing %s interface methods... done\\n\");" % Flatten(interface.obj.type))

        emit.IndentDec()
        emit.Line("}")

        emit.Line("~Instantiation()")
        emit.Line("{")
        emit.IndentInc()

        if EMIT_TRACES:
            emit.Line("fprintf(stderr, \"*** Recalling %s interface methods...\\n\");" % Flatten(interface.obj.type))

        for _, [_, _, _, _, interface, _ ]  in announce_list.items():
            emit.Line("RPC::Administrator::Instance().Recall<%s>();" % (Flatten(interface.obj.type)))

        if EMIT_TRACES:
            emit.Line("fprintf(stderr, \"*** Announcing %s interface methods... done\\n\");" % Flatten(interface.obj.type))

        emit.IndentDec()
        emit.Line("}")

        emit.IndentDec()
        emit.Line("} ProxyStubRegistration;")
        emit.Line()
        emit.IndentDec()
        emit.Line("} // namespace")
        emit.Line()

        emit.IndentDec()
        emit.Line("} // namespace %s" % STUB_NAMESPACE.split("::")[-1])
        emit.Line()
        emit.Line("}") # // namespace %s" % STUB_NAMESPACE.split("::")[-1])

    return interfaces


# -------------------------------------------------------------------------
# entry point

if __name__ == "__main__":
    argparser = argparse.ArgumentParser(description='Generate proxy stub code out of interface header files.',
                                        formatter_class=argparse.RawTextHelpFormatter)
    argparser.add_argument('path', nargs="*", help="Interface file(s) or a directory(ies) with interface files")
    argparser.add_argument("--help-tags",
                           dest="help_tags",
                           action="store_true",
                           default=False,
                           help="show help on supported source code tags and exit")
    argparser.add_argument("--code",
                           dest="code",
                           action="store_true",
                           default=False,
                           help="generate stub and proxy C++ code (default)")
    argparser.add_argument("--lua-code",
                           dest="lua_code",
                           action="store_true",
                           default=False,
                           help="generate lua code with interface information")
    argparser.add_argument("--secure",
                           dest="secure",
                           action="store_true",
                           default=False,
                           help="emit additinal parameter integrity and range checking, enables the @restrict tag")
    argparser.add_argument("--traces",
                           dest="traces",
                           action="store_true",
                           default=False,
                           help="emit additional debug traces (default: no extra traces)")
    argparser.add_argument("-i",
                           dest="extra_includes",
                           metavar="FILE",
                           action='append',
                           default=[],
                           help="include an additional C++ header file, may be used multiple times (default: include 'Ids.h')")
    argparser.add_argument("--namespace",
                           dest="if_namespace",
                           metavar="NS",
                           type=str,
                           action="store",
                           default=INTERFACE_NAMESPACE,
                           help="set namespace to look for interfaces in (default: %s)" % INTERFACE_NAMESPACE)
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
                           help="Suppress all warnings (default: show warnings)")
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
                           help="force stub generation even if destination appears up-to-date (default: force disabled)")
    argparser.add_argument('-I', dest="includePaths", metavar="INCLUDE_DIR", action='append', default=[], type=str,
                           help='add an include search path, can be used multiple times')

    args = argparser.parse_args(sys.argv[1:])
    SHOW_WARNINGS = not args.no_warnings
    BE_VERBOSE = args.verbose
    FORCE = args.force
    ENABLE_RESTRICT = args.secure
    log.show_infos = BE_VERBOSE
    log.show_warnings = SHOW_WARNINGS
    INTERFACE_NAMESPACE = args.if_namespace
    OUTDIR = args.outdir
    EMIT_TRACES = args.traces
    scan_only = False
    keep_incomplete = args.keep_incomplete

    if INTERFACE_NAMESPACE[0:2] != "::":
        INTERFACE_NAMESPACE = "::" + INTERFACE_NAMESPACE

    if args.help_tags:
        print("The following special tags are supported:")
        print("   @stop                  - a hard stop for parsing the file")
        print("   @omit                  - omit generating code for the next item (class or method)")
        print("   @stub                  - generate an empty stub for the next item (class or method)")
        print("   @insert \"file\"         - include a C++ header file, relative to the directory of the current file")
        print("   @insert <file>         - include a C++ header file, relative to the defined include directories")
        print("                            note: this is intended for resolving unknown types, classes defined in included")
        print("                            headers are not considered for stub generation (except for template classes)")
        print("   @define {identifier}   - defines a literal as a known identifier (equivalent of #define {identifier})")
        print("")
        print("Parameter-related tags:")
        print("   @in                    - indicates an input parameter")
        print("   @out                   - indicates an output parameter")
        print("   @inout                 - indicates an input/output parameter (equivalent of @in @out)")
        print("   @restrict              - specifies valid range for a parameter (for buffers and strings: valid size)")
        print("                            e.g.: @restrict:1..32, @restrict:256..1K, @restrict:1M")
        print("   @interface:{expr}      - specifies a parameter or value indicating interface ID value for void* interface passing")
        print("   @length:{expr}         - specifies a buffer length value (a constant, a parameter name or a math expression)")
        print("   @maxlength:{expr}      - specifies a maximum buffer length value (a constant, a parameter name or a math expression),")
        print("                            if not specified, @length is considered as maximum length; use round parenthesis for expressions",)
        print("                            e.g.: @length:bufferSize @length:(width*height*4), @length:(sizeof(uint64_t)), @length:void")
        print("")
        print("JSON-RPC-related parameters:")
        print("   @json                  - takes a class in for JSON-RPC generation")
        print("   @json:omit             - leaves a method out from a class intended for JSON-RPC generation")
        print("   @compliant             - tags a class to be generated in JSON-RPC compliant format (default)")
        print("   @uncompliant:extended  - tags a class to be generated in the obsolete 'extended' format")
        print("   @uncompliant:collapsed - tags a class to be generated in the obsolete 'collapsed' format")
        print("   @event                 - takes a class in to be generated as an JSON-RPC event")
        print("   @property              - tags method to be generated as a JSON-RPC property")
        print("   @iterator              - tags a class to be generated as an JSON-RPC interator")
        print("   @bitmask               - indicates that enumerator lists can be packed into into a bit mask")
        print("   @index                 - indicates that a parameter in a JSON-RPC property or event is an index")
        print("   @opaque                - indicates that a string parameter is an opaque JSON object")
        print("   @alt {name}            - provides an alternative name a method can by called by")
        print("   @text {name}           - sets a different name for a parameter, enumerator, struct or method")
        print("")
        print("JSON-RPC documentation-related parameters:")
        print("   @sourcelocation {lnk}  - sets source location link to be used in the documentation")
        print("   @deprecated            - outlines a JSON-RPC method, property or event as deprecated")
        print("   @obsolete              - outlines a JSON-RPC method, property or event as osbsolete")
        print("   @brief {desc}          - sets a brief description for a JSON-RPC method, property or event")
        print("   @details {desc}        - sets a detailed description for a JSON-RPC method, property or event")
        print("   @param {name} {desc}   - sets a description for a parameter of a JSON-RPC method or event")
        print("   @retval {desc}         - sets a description for a return value of a JSON-RPC method")
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

            for source_file in interface_files:
                try:
                    _extra_includes = [ os.path.join("@" + os.path.dirname(source_file), IDS_DEFINITIONS_FILE) ]
                    _extra_includes.extend(args.extra_includes)

                    if args.code:
                        log.Header(source_file)

                        output_file = os.path.join(os.path.dirname(source_file) if not OUTDIR else OUTDIR,
                            PROXYSTUB_CPP_NAME % CreateName(os.path.basename(source_file)).split(".", 1)[0])

                        out_dir = os.path.dirname(output_file)
                        if not os.path.exists(out_dir):
                            os.makedirs(out_dir)

                        output = GenerateStubs2(
                            output_file, source_file,
                            args.includePaths,
                            os.path.join("@" + os.path.dirname(os.path.realpath(__file__)), DEFAULT_DEFINITIONS_FILE),
                            _extra_includes,
                            scan_only)

                        faces += output

                        log.Print("created file %s" % os.path.basename(output_file))

                        # dump interfaces if only scanning
                        if scan_only:
                            for f in sorted(output, key=lambda x: str(x.id)):
                                print(f.id, f.obj.full_name)

                    if args.lua_code:
                        log.Print("(lua generator) Scanning %s..." % os.path.basename(source_file))
                        GenerateLuaData(Emitter(lua_file, INDENT_SIZE), lua_interfaces, lua_enums, source_file, args.includePaths,
                            os.path.join("@" + os.path.dirname(os.path.realpath(__file__)), DEFAULT_DEFINITIONS_FILE),
                            _extra_includes)

                except NotModifiedException as err:
                    log.Print("skipped file %s, up-to-date" % os.path.basename(output_file))
                    skipped.append(source_file)
                except SkipFileError as err:
                    log.Print("skipped file %s" % os.path.basename(output_file))
                    skipped.append(source_file)
                except NoInterfaceError as err:
                    log.Warn("no interface classes found in %s" % (INTERFACE_NAMESPACE), os.path.basename(source_file))
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
                GenerateLuaData(Emitter(lua_file, INDENT_SIZE), lua_interfaces, lua_enums, None)
                log.Print("Created %s (%s interfaces, %s enums)" % (lua_file.name, len(lua_interfaces), len(lua_enums)))

        else:
            log.Print("Nothing to do")

        sys.exit(1 if len(log.errors) else 0)
