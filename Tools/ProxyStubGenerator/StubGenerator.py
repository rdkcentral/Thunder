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
import uuid
import sys
import os
import argparse
import copy
import CppParser
from collections import OrderedDict
import Log


NAME = "ProxyStubGenerator"

# runtime changeable configuration
INDENT_SIZE = 4
SHOW_WARNINGS = True
USE_OLD_CPP = False
BE_VERBOSE = False
EMIT_TRACES = False
FORCE = False

# static configuration
EMIT_DESTRUCTOR_FOR_PROXY_CLASS = False
EMIT_MODULE_NAME_DECLARATION = False
EMIT_COMMENT_WITH_PROTOTYPE = True
EMIT_COMMENT_WITH_STUB_ORDER = True
STUB_NAMESPACE = "::WPEFramework::ProxyStubs"
INTERFACE_NAMESPACE = "::WPEFramework"
CLASS_IUNKNOWN = "::WPEFramework::Core::IUnknown"
PROXYSTUB_CPP_NAME = "ProxyStubs_%s.cpp"

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
        msg = "%s(%s): %s" % (os.path.basename(type.parent.parser_file), type.parent.parser_line, msg)
        super(TypenameError, self).__init__(msg)
        pass


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
                    if (c.full_name.find(INTERFACE_NAMESPACE + "::")) == 0:
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
                                log.Warn("class %s does not have ID enumerator" % c.full_name, source_file)

                    elif not c.omit:
                        log.Info("class %s not in %s namespace" % (c.full_name, INTERFACE_NAMESPACE), source_file)

                __Traverse(c, faces)

        if isinstance(tree, CppParser.Namespace):
            for n in tree.namespaces:
                __Traverse(n, faces)

    __Traverse(tree, interfaces)
    return interfaces


# Cut out interface namespace from all identifiers found in a string
def Strip(string, iface_namespace_l, index=0):
    pos = 0
    idx = index
    length = 0

    for p in iface_namespace_l:

        def _FindIdentifier(string, ps, pos, idx, length):
            while (True):
                i = string.find(ps, idx)
                if i != -1:
                    if (((length == 0) and (i == 0 or string[i - 1] in [" ", "(", ",", "<"])) or (i == pos + length)):
                        if length == 0:
                            pos = i
                        length += len(ps)
                        idx = i + 1
                        break
                    else:
                        idx = i + 1
                else:
                    if length != 0:
                        return [pos, idx, length, True]
                    break
            return [pos, idx, length, False]

        [pos, idx, length, finish] = _FindIdentifier(string, p + "::", pos, idx, length)
        if finish:
            break

    string = Strip(string, iface_namespace_l, pos + length + 1) if (length > 0) else string
    if length > 2:
        string = string[0:pos] + string[pos + length:]

    return string


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
        iface_name = Strip(iface.obj.type.split(iface_namespace + "::", 1)[1], iface_namespace_l)

        interfaces_var_name = "INTERFACES"
        methods_var_name = "METHODS"

        assume_hresult = iface.obj.is_json

        if (iface_namespace + "::") not in iface.obj.full_name:
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

            def Convert(paramtype, retval, vars, hresult=False):
                if isinstance(paramtype.type, list):
                    return

                param = paramtype.type.Resolve()
                meta = paramtype.meta
                p = param.Type()

                if isinstance(p, CppParser.Integer):
                    if param.IsPointer():
                        value = "16"
                        if meta.length:
                            for v in vars:
                                if v.name == meta.length[0]:
                                    if v.type.Type().size == "char":
                                        value = "8"
                                    elif v.type.Type().size == "long":
                                        value = "32"
                                    break
                        value = "BUFFER" + value
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


                    return ["type = Type." + value]

                elif isinstance(p, CppParser.String):
                    return ["type = Type.STRING"]

                elif isinstance(p, CppParser.Bool):
                    return ["type = Type.BOOL"]

                elif isinstance(p, CppParser.Class):
                    if param.IsPointer():
                        return ["type = Type.OBJECT", "class = \"%s\"" % Strip(param.TypeName(), iface_namespace_l)]
                    else:
                        value = ["type = Type.POD", "class = \"%s\"" % Strip(param.type.full_name, iface_namespace_l)]
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

                    name = Strip(param.type.full_name, iface_namespace_l)
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

                    if p.meta.input or not p.meta.output:
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


def GenerateStubs(output_file, source_file, includePaths = [], defaults = "", extra_includes = [], scan_only=False):
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

    with open(output_file, "w") as file:
        iface_namespace_l = INTERFACE_NAMESPACE.split("::")
        iface_namespace = iface_namespace_l[-1]

        def EmitFunctionOrder(methods):
            if methods:
                emit.Line("// Methods:")
                c = 0
                for m in methods:
                    if m.IsPureVirtual:
                        emit.Line("//  (%i) %s" % (c, SignatureStr(m, None)))
                        c += 1
                emit.Line("//")

        NULLPTR = "0" if USE_OLD_CPP else "nullptr"

        emit = Emitter(file, INDENT_SIZE)

        emit.Line("//")
        emit.Line("// generated automatically from \"%s\"" % interface_header_name)
        emit.Line("//")
        emit.Line("// implements RPC proxy stubs for:")
        for face in interfaces:
            if not face.obj.omit:
                emit.Line("//   - %s" % str(face.obj).replace(INTERFACE_NAMESPACE + "::", ""))
        emit.Line("//")
        emit.Line()

        if os.path.isfile(os.path.join(os.path.dirname(source_file), "Module.h")):
            emit.Line('#include "Module.h"')
        if os.path.isfile(os.path.join(os.path.dirname(source_file), interface_header_name)):
            emit.Line('#include "%s"' % interface_header_name)
        emit.Line()

        emit.Line('#include <com/com.h>')
        emit.Line()

        if EMIT_MODULE_NAME_DECLARATION:
            emit.Line("MODULE_NAME_DECLARATION(BUILDREF_WEBBRIDGE)")
            emit.Line()

        emit.Line("namespace %s {" % STUB_NAMESPACE.split("::")[-2])
        emit.Line()
        emit.Line("namespace %s {" % STUB_NAMESPACE.split("::")[-1])
        emit.Line()
        emit.IndentInc()
        if (iface_namespace != STUB_NAMESPACE.split("::")[-2]):
            emit.Line("using namespace %s;" % iface_namespace)
            emit.Line()

        announce_list = OrderedDict()

        #
        # EMIT STUB CODE
        #
        emit.Line("// -----------------------------------------------------------------")
        emit.Line("// STUB")
        emit.Line("// -----------------------------------------------------------------\n")

        for iface in interfaces:
            if (iface_namespace + "::") not in iface.obj.full_name:
                continue # interface in other namespace

            name = iface.obj.full_name.split(iface_namespace + "::", 1)[1]
            array_name = CreateName(name) + "StubMethods"
            class_name = CreateName(name) + "Proxy"
            stub_name = CreateName(name) + "Stub"
            iface_name = Strip(iface.obj.type.split(iface_namespace + "::", 1)[1], iface_namespace_l)

            # Stringifies a type, omitting outer namespace if necessary
            def TypeStr(type):
                if isinstance(type, list):
                    return TypeStr(CppParser.Type(CppParser.Undefined(type)))
                else:
                    return Strip(str(type), iface_namespace_l)

            class EmitType:
                def __init__(self, type_, cv=[]):
                    if not isinstance(type_.type, CppParser.Type):
                        undefined = CppParser.Type(CppParser.Undefined(type_.type))
                        if not type_.parent.stub and not type_.parent.omit:
                            raise TypenameError(type_, "'%s': undefined type" % TypeStr(undefined))
                        else:
                            type = undefined
                    else:
                        type = copy.copy(type_.type)

                    self.proto_long = type.Proto()
                    self.proto = Strip(self.proto_long, iface_namespace_l)

                    meta = type_.meta
                    if type.IsValue():
                        if "const" in cv:
                            type.ref |= CppParser.Ref.CONST
                        if "volatile" in cv:
                            type.ref |= CppParser.Ref.VOLATILE
                    input = meta.input
                    self.interface = meta.interface
                    output = meta.output
                    length = meta.length
                    maxlength = meta.maxlength
                    interface = meta.interface
                    origname = type_.name
                    self.ocv = cv
                    self.oclass = type_
                    self.unexpanded = type
                    self.type = self._ExpandTypedefs(type)
                    if not isinstance(self.type, CppParser.Type):
                        raise TypenameError(type_, "'%s': undefined type" % TypeStr(CppParser.Type(CppParser.Undefined(self.type))))

                    self.is_ref = type.IsReference()
                    self.is_ptr = type.IsPointer()
                    self.is_ptr_ptr = type.IsPointerToPointer() and not type.IsConst() and not type.IsPointerToConst()

                    self.is_nonconstref = type.IsReference() and not type.IsConst()# and not type.IsPointerToConst()
                    self.is_nonconstptr = type.IsPointer() and not type.IsConst()# and not type.IsPointerToConst()

                    self.typename = type.Type()
                    self.expanded_typename = self.type.Type()
                    self.obj = self.expanded_typename if isinstance(self.expanded_typename, CppParser.Class) else None
                    self.str = TypeStr(self.unexpanded)
                    self.str_typename = Strip(type.TypeName(), iface_namespace_l)
                    self.str_noptrref = TypeStr(self.unexpanded).replace("*", "").replace("&", "")
                    self.str_noptrptrref = TypeStr(self.unexpanded).replace("**", "*").replace("&", "")
                    self.str_nocvref = ("const " if type.IsPointerToConst() else "") + self.str_typename + ("*" if self.is_ptr else "")
                    self.str_noref = TypeStr(self.unexpanded).replace("&", "")
                    self.is_interface = (interface != None) or (self.is_ptr and self.obj != None)

                    self.str_rpctype = None
                    self.str_rpctype_nocv = None
                    self.str_rpctype_bare = None
                    self.is_inputptr = self.is_ptr and (input or not self.is_nonconstptr)
                    self.is_outputptr = self.is_nonconstptr and output
                    self.is_outputptrptr = self.is_nonconstptr and self.is_ptr_ptr and output
                    self.is_inputref = self.is_ref and (input or not self.is_nonconstref)
                    self.is_outputref = self.is_nonconstref and output
                    self.is_output = self.is_outputptr or self.is_outputref or self.is_outputptrptr
                    self.is_input = self.is_inputptr or self.is_inputref or ((self.obj != None) and self.is_ptr and not self.is_nonconstref and not self.is_ptr_ptr)
                    self.ptr_length = length
                    self.ptr_maxlength = maxlength
                    self.ptr_interface = self.interface
                    self.proxy = self.is_interface and ((not self.is_ref and not self.is_ptr_ptr) or self.is_input)
                    self.origname = origname
                    self.length_constant = False
                    self.maxlength_constant = False
                    self.length_expr = None
                    self.maxlength_expr = None
                    self.length_var = None
                    self.maxlength_var = None
                    self.is_length = False
                    self.is_maxlength = False
                    self.interface_expr = None
                    self.interface_type = None
                    self.is_property = self.oclass.parent.retval.meta.is_property if isinstance(self.oclass.parent, CppParser.Method) else False
                    self.length_type = "void" if (((self.is_interface and self.is_ptr_ptr) or (self.is_property and (self.is_ptr and not self.is_ref))) and self.is_output) else "uint16_t"
                    self.str_nocv = TypeStr(self.type).replace("const ", "").replace("volatile ", "")
                    self.str_cv = type.CVString()

                    if not self.obj and self.is_nonconstptr and not self.is_inputptr and not self.is_outputptr and not interface:
                        raise TypenameError(
                            type_, "unable to serialise '%s %s': a non-const pointer requires an in/out tag" %
                            (self.CppType(), self.origname))
                    if not self.obj and self.is_nonconstref and not self.is_inputref and not self.is_outputref and not interface:
                        raise TypenameError(
                            type_, "unable to serialise '%s %s': a non-const reference requires an in/out tag" %
                            (self.CppType(), self.origname))
                    if self.obj and self.is_nonconstref and self.is_nonconstptr and not self.is_inputref and not self.is_outputref and not interface:
                        raise TypenameError(
                            type_,
                            "unable to serialise '%s %s': a non-const reference to pointer requires an in/out tag" %
                            (self.CppType(), self.origname))
                    if self.obj and self.is_nonconstref and self.is_nonconstptr and self.is_inputref and not interface:
                        raise TypenameError(
                            type_,
                            "unable to serialise '%s %s': an input non-const reference to pointer parameter is not supported"
                            % (self.CppType(), self.origname))

                def CheckRpcType(self):
                    if self.str_rpctype == None:
                        try:
                            self.str_rpctype = self._RpcType(self.str_noref)
                        except:
                            pass
                    return self.str_rpctype

                def CppType(self):
                    return self.proto

                def RpcType(self):
                    if self.str_rpctype == None:
                        self.str_rpctype = self._RpcType(self.str_noref)
                    return self.str_rpctype

                def RpcTypeNoRefPtr(self):
                    if self.str_rpctype == None:
                        self.str_rpctype = self._RpcType(self.str_noptrref)
                    return self.str_rpctype

                def RpcTypeBare(self):
                    if self.str_rpctype == None:
                        self.str_rpctype_bare = self._RpcType(self.str_noptrref, True)
                    return self.str_rpctype_bare

                def RpcTypeNoCV(self):
                    if self.str_rpctype_nocv == None:
                        self.str_rpctype_nocv = self._RpcType(self.str_nocvref)
                    return self.str_rpctype_nocv

                # Converts a C++ type to RPC types
                def _RpcType(self, noref, noptr = False):
                    if self.is_ptr and not noptr:
                        if self.is_interface:
                            return "Number<%s>" % INSTANCE_ID
                        else:
                            return "Buffer<%s>" % self.length_type
                    elif isinstance(self.expanded_typename, CppParser.Integer):
                        if not self.expanded_typename.IsFixed():
                            log.WarnLine(self.oclass, "%s: integer size is not fixed, use a stdint type instead" % self.str_typename)
                        return "Number<%s>" % noref
                    elif isinstance(self.expanded_typename, CppParser.String):
                        return "Text"
                    elif isinstance(self.expanded_typename, CppParser.Bool):
                        return "Boolean"
                    elif isinstance(self.expanded_typename, CppParser.Enum):
                        if not self.expanded_typename.type.Type().IsFixed():
                            log.WarnLine(self.oclass, "%s: underlying type of enumeration is not fixed" % self.str_typename)
                        return "Number<%s>" % noref
                    elif isinstance(self.expanded_typename, CppParser.BuiltinInteger):
                        if not self.expanded_typename.IsFixed():
                            log.WarnLine(self.oclass, "%s: %s size is not fixed, use a stdint type instead" % (self.str_typename, noref))
                        return "Number<%s>" % noref
                    elif isinstance(self.expanded_typename, CppParser.Typedef):
                        et = EmitType(self.oclass, self.ocv)
                        return et._RpcType(noref)
                    else:
                        raise TypenameError(self.oclass, "unable to serialise type '%s'" % self.CppType())

                def _ExpandTypedefs(self, type):
                    return type.Resolve()

            class EmitParam(EmitType):
                def __init__(self, type_, name="param", cv=[]):
                    EmitType.__init__(self, type_, cv)
                    self.name = name

            class EmitRetVal(EmitParam):
                def __init__(self, type_, name="output", cv=[]):
                    EmitParam.__init__(self, type_.retval, name, cv)
                    self.has_output = self.type and (not isinstance(self.type.Type(), CppParser.Void)
                                                     or self.type.IsPointer())
                    self.is_output = True

            # Stringify a method object to full signature
            def SignatureStr(method, parameters=None):
                return Strip(str(method), iface_namespace_l)

            # Stringify a method object to a prototype
            def PrototypeStr(method, parameters=None, unused=False):
                params = parameters if parameters else method.vars
                proto = "%s %s(" % (TypeStr(method.retval.type), method.name)
                for c, p in enumerate(params):
                    acc = ""
                    if parameters:
                        if p.is_nonconstref or p.is_nonconstptr:
                            if p.is_input and p.is_output:
                                acc += " /* inout */"
                            elif p.is_input:
                                pass
                                # acc += " /* in */"
                            elif p.is_output:
                                acc += " /* out */"
                    proto += TypeStr(p.unexpanded) + acc + " param%i%s%s" % (c, (" VARIABLE_IS_NOT_USED" if unused else ""),
                                    (", " if c != len(method.vars) - 1 else ""))
                proto += ")"
                for q in method.qualifiers:
                    proto += " " + q
                return proto

            if iface.obj.omit:
                log.Info("omitted class %s" % iface.obj.full_name, source_file)
                continue

            emit_methods = [m for m in iface.obj.methods if m.IsPureVirtual()]
            if not emit_methods:
                log.Warn("nothing emit for interface class %s" % iface.obj.full_name, source_file)
                continue

            if BE_VERBOSE:
                log.Print("Emitting stub code for interface '%s'..." % iface_name)

            # build the announce list upfront
            announce_list[iface_name] = [class_name, array_name, stub_name, iface]

            emit.Line("//")
            emit.Line("// %s interface stub definitions" % (iface_name))
            emit.Line("//")
            if EMIT_COMMENT_WITH_STUB_ORDER:
                EmitFunctionOrder(emit_methods)

            emit.Line()

            emit.Line("%s %s[] = {" % ("ProxyStub::MethodHandler", array_name))
            emit.IndentInc()

            def LinkPointers(retval, parameters):
                params = [retval] + parameters
                for c, p in enumerate(params):
                    if not p.is_length:
                        if p.is_ptr and (not p.is_ref and not p.is_ptr_ptr and not p.is_property) and (not p.is_interface or p.interface):

                            def __ParseLength(length, maxlength, target, length_name):
                                parsed = []
                                has_param_ref = False
                                sizeof_parsing = False
                                param_ref = None
                                par_count = 0
                                param_type = None
                                for token in length:
                                    if token[0].isalpha():
                                        t = None
                                        for d, q in enumerate(parameters):
                                            if q.origname == token:
                                                t = "param" + str(d)
                                                if not sizeof_parsing:
                                                    q.is_length = True
                                                    q.is_maxlength = maxlength
                                                    q.length_name = length_name
                                                    q.length_target = target
                                                    if param_type and param_type != q.str_typename:
                                                        raise TypenameError(
                                                            p.oclass,
                                                            "unable to serialise '%s': %slength variable '%s' type mismatch"
                                                            % (p.origname, "max" if maxlength else "", token))
                                                    param_type = q.str_typename
                                                    has_param_ref = True
                                                    param_ref = q
                                                break
                                        if not t:
                                            if token == "void":
                                                parsed = []
                                                param_type = "void"
                                                break
                                            elif token == "sizeof":
                                                if sizeof_parsing:
                                                    raise TypenameError(
                                                        p.oclass,
                                                        "unable to serialise '%s': %slength variable has nested sizeof"
                                                        % (p.origname, "max" if maxlength else ""))
                                                parsed.append(token)
                                                sizeof_parsing = -True
                                            elif sizeof_parsing:
                                                parsed.append(token)
                                            else:
                                                raise TypenameError(
                                                    p.oclass,
                                                    "unable to serialise '%s': %slength variable '%s' not found" %
                                                    (p.origname, "max" if maxlength else "", token))
                                        else:
                                            parsed.append(t)
                                    else:
                                        if sizeof_parsing:
                                            if token == '(':
                                                par_count += 1
                                            elif token == ')':
                                                par_count -= 1
                                                if par_count == 0:
                                                    sizeof_parsing = False
                                        parsed.append(token)
                                joined = "".join(parsed)
                                if not param_type:
                                    param_type = "uint16_t"
                                    if not has_param_ref:
                                        try:
                                            value = eval(joined)
                                            joined = str(value)
                                            if value < 256:
                                                param_type = "uint8_t"
                                        except:
                                            pass
                                return joined, param_type, not has_param_ref, param_ref

                            if p.is_output and p.ptr_interface:
                                p.interface_var = p.name + "_interfaceid"
                                p.interface_expr, p.interface_type, constant, p.interface_ref = __ParseLength(
                                    p.ptr_interface, True, c - 1, p.interface_var)
                                if constant:
                                    raise TypenameError(
                                        p.oclass, "unable to serialise '%s': constant not allowed for interface id" %
                                        (p.origname))
                            else:
                                if p.is_output and p.ptr_maxlength and p.ptr_maxlength != p.ptr_length:
                                    if p.ptr_length:
                                        p.maxlength_var = p.name + "_maxlength"
                                        p.maxlength_expr, p.length_type, p.maxlength_constant, p.length_ref = __ParseLength(
                                            p.ptr_maxlength, True, c - 1, p.maxlength_var)
                                    else:
                                        p.ptr_length = p.ptr_maxlength
                                if p.ptr_length:
                                    p.length_var = p.name + "_length"
                                    p.length_expr, p.length_type, p.length_constant, p.length_ref = __ParseLength(
                                        p.ptr_length, False, c - 1, p.length_var)
                                    if p.length_ref:
                                        if isinstance(p.length_ref.expanded_typename, CppParser.Integer):
                                            if p.length_ref.expanded_typename.size == "long":
                                                log.WarnLine(p.oclass, "32-bits length variables are supported ('%s'), but buffer lengths up to 64 KB are recommended" % p.length_ref.origname)
                                            if p.length_ref.expanded_typename.size == "long long":
                                                raise TypenameError(p.oclass, "unable to serialise '%s': 64-bit length variables ('%s') are not allowed" % (p.origname, p.length_ref.origname))
                                            pass
                                        else:
                                            raise TypenameError(p.oclass, "unable to serialise '%s': length variable '%s' is not of integral type" % (p.origname, p.length_ref.origname))
                            if not p.ptr_length and not p.ptr_interface:
                                raise TypenameError(
                                    p.oclass, "unable to serialise '%s': length variable not defined" % (p.origname))

            for m in emit_methods:
                proxy_count = 0
                output_params = 0
                interface_params = 0

                # enumerate and prepare parameters for emitting
                # force non-ptr, non-ref parameters to be const
                retval = EmitRetVal(m, cv=["const"])
                params = [EmitParam(v, cv=["const"]) for v in m.vars]
                orig_params = [EmitParam(v) for v in m.vars]
                for i, p in enumerate(params):
                    if p.is_interface:
                        interface_params += 1
                    if p.proxy and p.obj:
                        proxy_count += 1
                    if p.is_output:
                        output_params += 1
                    p.name += str(i)

                interface_params += 1 if retval.is_interface else 0

                if m.omit:
                    log.Info("omitted method %s" % m.full_name, source_file)
                    emit.Line("// %s" % SignatureStr(m, orig_params))
                    emit.Line("//")
                    emit.Line("// method omitted")
                    emit.Line("//")
                    emit.Line("")
                    continue
                elif BE_VERBOSE:
                    log.Print("  generating code for %s()" % m.full_name)

                LinkPointers(retval, params)
                # emit a comment with function signature (optional)
                if EMIT_COMMENT_WITH_PROTOTYPE:
                    emit.Line("// " + SignatureStr(m, orig_params))
                    emit.Line("//")

                # emit the lambda prototype
                emit.Line(
                    "[](Core::ProxyType<Core::IPCChannel>& channel%s, Core::ProxyType<RPC::InvokeMessage>& message%s) {" %
                    (" VARIABLE_IS_NOT_USED" if ((not interface_params and not proxy_count) or m.stub) else "", " VARIABLE_IS_NOT_USED" if m.stub else ""))
                emit.IndentInc()

                if EMIT_TRACES:
                    emit.Line('fprintf(stderr, "*** [%s stub] ENTER: %s()\\n");' % (iface.obj.full_name, m.name))
                    emit.Line()

                if not m.stub:
                    emit.Line("RPC::Data::Input& input(message->Parameters());")
                    emit.Line()

                    # emit parameter readout
                    if params:
                        emit.Line("// read parameters")
                        emit.Line("RPC::Data::Frame::Reader reader(input.Reader());")
                        for c, p in enumerate(params):
                            if (p.is_length or p.is_maxlength):
                                if p.is_ptr:
                                    raise TypenameError(p.oclass,
                                                        "unsupported: '%s' length variable is a pointer" % p.origname)
                                elif p.obj:
                                    raise TypenameError(p.oclass, "'%s' length variable is an object" % p.origname)

                            # if parameter is passed by value or by reference, then try to  decompose it
                            if not p.is_ptr and not p.CheckRpcType():
                                if p.obj:
                                    if p.is_input or not p.is_output:
                                        emit.Line("// (decompose %s)" % p.str_typename)
                                        emit.Line("%s %s;" % (p.str_typename, p.name))
                                        # TODO: make this recursive
                                        if p.obj.vars:
                                            for attr in p.obj.vars:
                                                emit.Line("param%i.%s = reader.%s();" %
                                                        (c, attr.name, EmitParam(attr).RpcTypeNoCV()))
                                        else:
                                            raise TypenameError(
                                                m, "method '%s': unable to decompose parameter '%s': non-POD type" %
                                                (m.name, p.str_typename))
                                    else:
                                        emit.Line("%s %s{}; // storage" % (p.str_noref, p.name))
                                elif not p.RpcType():
                                    raise TypenameError(
                                        m, "method '%s': unable to decompose parameter '%s': unknown type" %
                                        (m.name, p.str_typename))
                            else:
                                if p.is_ptr and not p.obj and not p.is_ref and p.length_type == "void":
                                    emit.Line("%s %s{}; // storage" % (p.str_typename, p.name))
                                elif p.is_ptr and not p.obj and not p.is_ref:
                                    if p.is_input:
                                        emit.Line("%s%s %s = %s;" % ("const " if "const" not in p.str_nocvref else "", p.str_nocvref, p.name, NULLPTR))
                                        emit.Line("%s %s_length = reader.Lock%s(%s);" %
                                                  (p.length_type, p.name, p.RpcTypeNoCV(), p.name))
                                        emit.Line("reader.UnlockBuffer(%s_length);" % p.name)
                                elif p.is_ref and not p.is_input and not p.is_ptr_ptr:
                                    emit.Line("%s %s{}; // storage" % (p.str_noref, p.name))
                                    if p.is_length or p.is_maxlength:
                                        raise TypenameError(
                                            p.oclass,
                                            "'%s' is defined as a length variable but is write-only" % p.origname)
                                elif p.is_ptr_ptr:
                                    if p.is_ref:
                                        raise TypenameError(p.oclass,
                                            "'%s' reference to a pointer to pointer is not supported" % p.origname)
                                    elif p.is_output and p.is_interface:
                                        emit.Line("%s %s{}; // storage" % (p.str_noref, p.name))
                                    else:
                                        raise TypenameError(p.oclass,
                                            "'%s' pointer to pointer must be an interface output parameter" % p.origname)

                                elif not p.is_length or p.is_maxlength or not params[p.length_target].is_input:
                                    emit.Line("%s %s = reader.%s();" %
                                              (INSTANCE_ID if p.proxy else p.str_noref,
                                               p.length_name if p.is_length else p.name, p.RpcTypeNoCV()))
                                if p.is_length:
                                    p.name = p.length_name

                        for c, p in enumerate(params):
                            if p.is_ptr and p.is_ref:
                                pass
                            elif not p.is_ptr and not p.CheckRpcType():
                                pass
                            else:
                                if p.is_ptr and not p.obj and p.is_output and p.length_type != "void":
                                    if p.is_output:
                                        emit.Line()
                                        if p.is_input:
                                            if p.maxlength_var:
                                                emit.Line("// allocate receive buffer if necessary")
                                            oldname = p.name
                                            p.name = p.name + "_buffer"
                                        elif not p.is_input:
                                            emit.Line("// allocate receive buffer")
                                        emit.Line("%s %s{};" % (p.str_nocvref, p.name))
                                        length_var = p.maxlength_var if p.maxlength_var else p.length_var
                                        if p.length_constant:
                                            emit.Line("const %s %s = %s;" %
                                                      (p.length_type, p.length_var, p.length_expr))
                                        if p.is_input:
                                            if p.maxlength_var:
                                                # input/output and maxlength defined
                                                emit.Line("%s = const_cast<%s>(%s);" % (p.name, p.str_nocvref, oldname))
                                                emit.Line("if (%s > %s) {" % (p.maxlength_var, p.length_var))
                                                emit.IndentInc()
                                                if p.length_type not in [
                                                        "char", "short", "int8_t", "uint8_t", "int16_t", "uint16_t"
                                                ]:
                                                    emit.Line("ASSERT((%s < 0x100000) && \"Buffer length too big\");" %
                                                              length_var)
                                                emit.Line("%s = static_cast<%s>(ALLOCA(%s));" %
                                                          (p.name, p.str_nocvref, length_var))
                                                emit.Line("ASSERT(%s != %s);" % (p.name, NULLPTR))
                                            else:
                                                # is input/output but maxlength not defined
                                                emit.Line("%s = const_cast<%s>(%s); // reuse the input buffer" %
                                                          (p.name, p.str_nocvref, oldname))
                                        else:
                                            # is output-only
                                            if not p.length_constant:
                                                emit.Line("if (%s != 0) {" % p.length_var)
                                                emit.IndentInc()
                                            if p.length_type not in [
                                                    "char", "short", "int8_t", "uint8_t", "int16_t", "uint16_t"
                                            ]:
                                                emit.Line("ASSERT((%s < 0x100000) && \"Buffer length too big\");" %
                                                          length_var)
                                            emit.Line("%s = static_cast<%s>(ALLOCA(%s));" %
                                                      (p.name, p.str_nocvref, length_var))
                                            emit.Line("ASSERT(%s != %s);" % (p.name, NULLPTR))
                                            if not p.length_constant:
                                                emit.IndentDec()
                                                emit.Line("}")
                                        if p.is_input and p.maxlength_var:
                                            emit.Line("::memcpy(%s, %s, %s);" % (p.name, oldname, p.length_var))
                                            emit.IndentDec()
                                            emit.Line("}")

                        # emit proxy for the parameters if applicable
                        for p in params:
                            if p.proxy:
                                proxy_name = p.name + "_proxy"
                                emit.Line("%s %s = %s;" % (p.str_nocvref, proxy_name, NULLPTR))
                                emit.Line("ProxyStub::UnknownProxy* %s_inst = %s;" % (proxy_name, NULLPTR))
                                emit.Line("if (%s != 0) {" % (p.name))
                                emit.IndentInc()
                                # create proxy
                                emit.Line(
                                    "%s_inst = RPC::Administrator::Instance().ProxyInstance(channel, %s, false, %s);"
                                    % (proxy_name, p.name, proxy_name))
                                emit.Line("ASSERT((%s_inst != %s) && (%s != %s) && \"Failed to get instance of %s proxy\");" %
                                          (proxy_name, NULLPTR, proxy_name, NULLPTR, p.str_typename))
                                emit.Line()
                                emit.Line("if ((%s_inst == %s) || (%s == %s)) {" % (proxy_name, NULLPTR, proxy_name, NULLPTR))
                                emit.IndentInc()
                                emit.Line("TRACE_L1(\"Failed to get instance of %s proxy\");" % p.str_typename)
                                emit.IndentDec()
                                emit.Line("}")
                                emit.IndentDec()
                                emit.Line("}")
                        emit.Line()

                    if (retval.has_output or output_params) and proxy_count:
                        emit.Line("// write return value%s" % ("s" if
                                                               (int(retval.has_output) + output_params > 1) else ""))
                        emit.Line("RPC::Data::Frame::Writer writer(message->Response().Writer());")
                        emit.Line()

                    # emit function call
                    emit.Line("// call implementation")
                    emit.Line("%s* implementation = reinterpret_cast<%s*>(input.Implementation());" %
                              ((" ".join(m.qualifiers) + " " + iface_name).strip(),  (" ".join(m.qualifiers) + " " + iface_name).strip()))
                    emit.Line("ASSERT((implementation != %s) && \"Null %s implementation pointer\");" %
                              (NULLPTR, iface_name))
                    call = ""
                    if retval.has_output:
                        call += "%s %s = " % (retval.str_noref, retval.name)
                    call += "implementation->%s(" % m.name
                    for c, p in enumerate(params):
                        parameter = "%s%s%s" % ("&" if p.length_type == "void" else "", p.name,
                                              ("_proxy" if p.proxy else ""))
                        if (p.is_inputptr and p.is_inputref and p.proxy):
                            parameter = "static_cast<%s* const&>(%s)" % (p.str_typename, parameter)
                        elif (p.obj and not p.is_output and not p.is_interface and not p.proxy):
                            parameter = "static_cast<const %s>(%s)" % (p.str_typename, parameter)
                        elif (p.obj and not p.is_output and p.type.IsPointerToConst()) :
                            parameter = "static_cast<%s>(%s)" % (p.str_nocvref, parameter)
                        parameter = parameter + (", " if c < len(params) - 1 else "")
                        call += parameter
                    call += ");"
                    emit.Line(call)

                    if (retval.has_output or output_params) and not proxy_count:
                        emit.Line()
                        emit.Line("// write return value%s" % ("s" if
                                                               (int(retval.has_output) + output_params > 1) else ""))
                        emit.Line("RPC::Data::Frame::Writer writer(message->Response().Writer());")

                    # forward the output value
                    if retval.has_output:
                        if not retval.is_ptr and not retval.CheckRpcType():
                            if retval.obj:
                                emit.Line("// (decompose %s)" % retval.str_typename)
                                if retval.obj.vars:
                                    for attr in retval.obj.vars:
                                        emit.Line("writer.%s(output.%s);" % (EmitParam(attr).RpcTypeNoCV(), attr.name))
                                else:
                                    raise TypenameError(
                                        m, "method '%s': unable to decompose parameter type '%s': non-POD type" %
                                        (m.name, retval.str_typename))
                            elif not retval.RpcType():
                                raise TypenameError(
                                    m, "method '%s': unable to decompose parameter '%s': unknown type" %
                                    (m.name, retval.str_typename))
                        else:
                            if retval.proxy:
                                emit.Line("writer.%s(RPC::instance_cast<%s>(%s));" % (retval.RpcType(), retval.CppType(), retval.name))
                            else:
                                emit.Line("writer.%s(%s);" % (retval.RpcType(), retval.name))
                            if retval.is_interface:
                                if isinstance(retval.type.Type(), CppParser.Void):
                                    emit.Line("RPC::Administrator::Instance().RegisterInterface(channel, %s, %s);" %
                                              (retval.name, retval.interface_ref.length_name))
                                else:
                                    emit.Line("RPC::Administrator::Instance().RegisterInterface(channel, %s);" %
                                              retval.name)

                    if output_params:
                        for p in params:
                            if not p.obj and p.is_outputptr and not p.is_ref:
                                if p.length_type == "void":
                                    emit.Line("writer.%s(%s);" % (EmitType(p.oclass).RpcTypeBare(), p.name))
                                else:
                                    if p.length_var and p.length_ref and p.length_ref.is_output:
                                        emit.Line("writer.%s(%s);" % (p.length_ref.RpcType(), p.length_var))
                                    emit.Line("if ((%s != %s) && (%s != 0)) {" % (p.name, NULLPTR, p.length_var))
                                    emit.IndentInc()
                                    emit.Line("writer.%s(%s, %s);" %
                                              (p.RpcType(), p.length_var if p.length_var else p.maxlength_var, p.name))
                                    emit.IndentDec()
                                    emit.Line("}")
                            elif p.is_nonconstref or p.is_ptr_ptr:
                                if p.obj and not p.is_interface:
                                    emit.Line("// (decompose %s)" % p.str_typename)
                                    if p.obj.vars:
                                        for attr in p.obj.vars:
                                            emit.Line("writer.%s(%s.%s);" % (EmitParam(attr).RpcTypeNoCV(), p.name, attr.name))
                                    else:
                                        raise TypenameError(
                                            m, "method '%s': unable to decompose parameter type '%s': non-POD type" %
                                            (m.name, p.str_typename))
                                elif not p.is_length:
                                    if p.is_interface:
                                        emit.Line("writer.%s(RPC::instance_cast<%s>(%s));" % (p.RpcType(), p.str_noptrptrref, p.name))
                                    else:
                                        emit.Line("writer.%s(%s);" % (p.RpcType(), p.name))
                                if p.is_interface and not p.type.IsConst():
                                    emit.Line("RPC::Administrator::Instance().RegisterInterface(channel, %s);" % p.name)

                    if proxy_count:
                        # emit release proxy call if applicable
                        emit.Line()
                        for p in params:
                            if p.proxy:
                                emit.Line("if (%s_proxy_inst != %s) {" % (p.name, NULLPTR))
                                emit.IndentInc()
                                emit.Line(
                                    "RPC::Administrator::Instance().Release(%s_proxy_inst, message->Response());" %
                                    p.name)
                                emit.IndentDec()
                                emit.Line("}")

                else:
                    log.Info("stubbed method %s" % m.full_name, source_file)
                    emit.Line("// RPC::Data::Input& input(message->Parameters());")
                    emit.Line()

                    if params:
                        emit.Line("// RPC::Data::Frame::Reader reader(input.Reader());")
                    if retval.has_output:
                        emit.Line("// RPC::Data::Frame::Writer writer(message->Response().Writer());")
                    emit.Line("// TODO")

                if EMIT_TRACES:
                    emit.Line()
                    emit.Line('fprintf(stderr, "*** [%s stub] EXIT: %s()\\n");' % (iface.obj.full_name, m.name))

                emit.IndentDec()
                emit.Line("},\n")

            emit.Line(NULLPTR)
            emit.IndentDec()
            emit.Line("}; // %s[]\n" % array_name)

        #
        # EMIT PROXY CODE
        #
        emit.Line("// -----------------------------------------------------------------")
        emit.Line("// PROXY")
        emit.Line("// -----------------------------------------------------------------\n")

        for iface in interfaces:
            if (iface_namespace + "::") not in iface.obj.full_name:
                continue
            iface_name = Strip(iface.obj.type.split(INTERFACE_NAMESPACE + "::", 1)[1], iface_namespace_l)
            if not iface_name in announce_list:
                continue

            class_name = announce_list[iface_name][0]

            emit_methods = [m for m in iface.obj.methods if "pure-virtual" in m.specifiers]
            if not emit_methods:
                continue

            if BE_VERBOSE:
                log.Print("Emitting proxy code for interface '%s'..." % iface_name)

            emit.Line("//")
            emit.Line("// %s interface proxy definitions" % (iface_name))
            emit.Line("//")

            # emit stub order comment
            if EMIT_COMMENT_WITH_STUB_ORDER:
                EmitFunctionOrder(emit_methods)

            emit.Line()

            # emit proxy class definition
            emit.Line("class %s%s : public ProxyStub::UnknownProxyType<%s> {" %
                      (class_name, " final" if not USE_OLD_CPP else " /* final */", iface_name))
            emit.Line("public:")
            emit.IndentInc()

            # emit constructor
            emit.Line("%s(const Core::ProxyType<Core::IPCChannel>& channel, %s implementation, const bool otherSideInformed)" % (class_name, INSTANCE_ID))
            emit.Line("    : BaseClass(channel, implementation, otherSideInformed)")
            emit.Line("{")
            emit.Line("}")
            emit.Line()

            # emit destructor
            if EMIT_DESTRUCTOR_FOR_PROXY_CLASS:
                emit.Line("~%s()" % class_name)
                emit.Line("{")
                emit.Line("}")
                emit.Line()

            count = 0
            for m in emit_methods:
                input_params = 0
                # enumerate and prepare parameters for emitting
                retval = EmitRetVal(m, cv=["const"])
                params = [EmitParam(v, cv=["const"]) for v in m.vars]
                orig_params = [EmitParam(v) for v in m.vars]

                LinkPointers(retval, params)

                for i, p in enumerate(params):
                    p.name += str(i)
                    if (not p.is_nonconstref and not p.is_nonconstptr) or (p.is_input and not p.is_length) or (
                            p.is_ptr and p.obj) or (p.is_length and not params[p.length_target].is_input):
                        input_params += 1

                method_line = PrototypeStr(m, orig_params, m.stub) + (" override" if not USE_OLD_CPP else " /* override */")

                if m.omit:
                    emit.Line("// %s" % method_line)
                    emit.Line("//")
                    emit.Line("// method omitted")
                    emit.Line("//")
                    emit.Line("")
                    continue

                emit.Line(method_line)
                emit.Line("{")
                emit.IndentInc()
                if EMIT_TRACES:
                    emit.Line('fprintf(stderr, "*** [%s proxy] ENTER: %s()\\n");' % (iface.obj.full_name, m.name))
                    emit.Line()

                proxy_params = 0
                output_params = 0

                if not m.stub:
                    emit.Line("IPCMessage newMessage(BaseClass::Message(%i));" % count)
                    emit.Line()

                    if input_params:
                        emit.Line("// write parameters")
                        emit.Line("RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());")

                        for c, p in enumerate(params):
                            if not p.is_ptr and not p.CheckRpcType():
                                if p.obj:
                                    if p.obj.vars:
                                        emit.Line("// (decompose %s)" % p.str_typename)
                                        for attr in p.obj.vars:
                                            emit.Line("writer.%s(param%i.%s);" %
                                                      (EmitParam(attr).RpcTypeNoCV(), c, attr.name))
                                    else:
                                        raise TypenameError(
                                            m, "method '%s': unable to decompose parameter '%s': non-POD type" %
                                            (m.name, p.str_typename))
                                elif not p.RpcType():
                                    raise TypenameError(
                                        m, "method '%s': unable to decompose parameter '%s': unknown type" %
                                        (m.name, p.str_typename))
                            else:
                                if p.is_ptr and p.obj and p.is_input:
                                    proxy_params += 1
                                if not p.obj and p.is_ptr:
                                    if p.is_input:
                                        emit.Line("writer.%s(%s, param%i);" % (p.RpcType(), p.length_expr, c))
                                elif not p.is_input and ((p.is_nonconstref and p.is_nonconstptr) or p.is_ptr_ptr):
                                    pass
                                elif (not p.is_length or not params[p.length_target].is_input
                                      or p.is_maxlength) and (p.is_input or
                                                              (not p.is_nonconstref and not p.is_nonconstptr) or p.obj):
                                    if p.proxy:
                                        emit.Line("writer.%s(RPC::instance_cast<%s>(param%i));" % (p.RpcType(), p.CppType(), c))
                                    else:
                                        emit.Line("writer.%s(param%i);" % (p.RpcType(), c))
                        emit.Line()

                    for c, p in enumerate(params):
                        if not p.obj and not p.is_ptr and not p.CheckRpcType():
                            pass
                        else:
                            if ((p.is_nonconstref or p.is_ptr_ptr) and p.obj) or (not p.obj and p.is_outputptr) or (p.is_nonconstref
                                                                                                  and not p.is_length):
                                output_params += 1

                    retval_has_proxy = retval.has_output and retval.is_interface

                    invoke_method = "ProxyStub::UnknownProxyType<%s>::Invoke" % iface_name

                    emit.Line("// invoke the method handler")
                    if retval.has_output:
                        default = "{}"
                        if isinstance(retval.typename, (CppParser.Typedef, CppParser.Enum)):
                            default = " = static_cast<%s>(~0)" % retval.str_nocvref
                        emit.Line("%s %s%s%s;" %
                                  (retval.str_nocvref, retval.name, "_proxy" if retval_has_proxy else "", default))
                        # assume it's a status code
                        if isinstance(retval.typename, CppParser.Integer) and retval.typename.type == "uint32_t":
                            emit.Line("if ((%s = %s(newMessage)) == Core::ERROR_NONE) {" % (retval.name, invoke_method))
                        else:
                            emit.Line("if (%s(newMessage) == Core::ERROR_NONE) {" % invoke_method)
                        emit.IndentInc()
                    elif proxy_params + output_params > 0:
                        emit.Line("if (%s(newMessage) == Core::ERROR_NONE) {" % invoke_method)
                        emit.IndentInc()
                    else:
                        emit.Line("%s(newMessage);" % invoke_method)

                    if retval.has_output or (output_params > 0) or (proxy_params > 0):
                        emit.Line("// read return value%s" % ("s" if
                                                              (int(retval.has_output) + output_params > 1) else ""))
                        emit.Line("RPC::Data::Frame::Reader reader(newMessage->Response().Reader());")

                    if retval.has_output:
                        if retval.is_interface:
                            if retval.obj:
                                emit.Line(
                                    "%s_proxy = reinterpret_cast<%s>(Interface(reader.Number<%s>(), %s::ID));" %
                                    (retval.name, retval.str_noref, INSTANCE_ID, retval.str_typename))
                            else:
                                emit.Line("%s_proxy = Interface(reader.Number<%s>(), %s);" %
                                          (retval.name, INSTANCE_ID, retval.interface_expr))
                        else:
                            if not retval.is_ptr and not retval.CheckRpcType():
                                if retval.obj:
                                    emit.Line("// (decompose %s)" % retval.str_typename)
                                    if retval.obj.vars:
                                        for attr in retval.obj.vars:
                                            emit.Line(
                                                "%s.%s = reader.%s();" %
                                                (retval.name, attr.name, EmitParam(attr, cv=["const"]).RpcTypeNoCV()))
                                    else:
                                        raise TypenameError(
                                            m, "method '%s': unable to decompose return value '%s': non-POD type" %
                                            (m.name, retval.str_typename))
                                elif not retval.RpcType():
                                    raise TypenameError(
                                        m, "method '%s': unable to decompose '%s': unknown type" %
                                        (m.name, retval.str_typename))
                            else:
                                emit.Line("%s = reader.%s();" % (retval.name, retval.RpcTypeNoCV()))

                    for p in params:
                        if (p.is_nonconstref or p.is_ptr_ptr) and p.is_interface:
                            if p.is_ptr_ptr:
                                emit.Line("ASSERT(%s != %s);" % (p.name, NULLPTR));
                            emit.Line("%s%s = reinterpret_cast<%s>(Interface(reader.Number<%s>(), %s::ID));" %
                                      ("*" if p.is_ptr_ptr else "", p.name, p.str_noref, INSTANCE_ID, p.str_typename))
                        elif not p.obj and p.is_outputptr:
                            if p.length_type != "void":
                                if p.length_var and p.length_ref and p.length_ref.is_output:
                                    emit.Line("%s = reader.%s();" % (p.length_ref.name, p.length_ref.RpcType()))
                                emit.Line("if ((%s != 0) && (%s != 0)) {" % (p.name, p.length_expr))
                                emit.IndentInc()
                                emit.Line("reader.%s(%s, %s);" % (p.RpcType(), p.length_expr, p.name))
                                emit.IndentDec()
                                emit.Line("}")
                            else:
                                emit.Line("ASSERT(%s != %s);" % (p.name, NULLPTR));
                                emit.Line("*%s = reader.%s();" % (p.name, p.RpcTypeBare()))
                        elif not p.obj and p.is_nonconstref and not p.is_length:
                            emit.Line("%s = reader.%s();" % (p.name, p.RpcTypeNoCV()))
                        elif p.obj and not p.is_interface and p.is_output:
                             for attr in p.obj.vars:
                                emit.Line("%s.%s = reader.%s();" % (p.name, attr.name, EmitParam(attr, cv=["const"]).RpcTypeNoCV()))

                    # emit Complete() only if there were interfaces passed
                    if proxy_params > 0:
                        if retval.has_output or output_params:
                            emit.Line()
                        emit.Line("Complete(reader);")

                    if retval.has_output or (proxy_params + output_params > 0):
                        emit.IndentDec()
                        emit.Line("}")

                    if EMIT_TRACES:
                        emit.Line()
                        emit.Line('fprintf(stderr, "*** [%s proxy] EXIT: %s()\\n");' % (iface.obj.full_name, m.name))

                    if retval.has_output:
                        emit.Line()
                        emit.Line("return %s%s;" % (retval.name, "_proxy" if retval_has_proxy else ""))

                else:
                    emit.Line("// IPCMessage newMessage(BaseClass::Message(%i));" % count)
                    emit.Line()

                    if m.vars:
                        emit.Line("// RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());")
                        emit.Line("// TODO")
                        emit.Line()

                    if retval.has_output:
                        emit.Line("// RPC::Data::Frame::Reader reader(newMessage->Response().Reader());")
                        emit.Line("// TODO")
                        emit.Line()

                        emit.Line("// Complete(newMessage->Response());")
                        emit.Line("// TODO")
                        emit.Line()

                    if retval.has_output:
                        if isinstance(retval.typename, CppParser.Integer) and retval.typename.type == "uint32_t":
                            emit.Line("%s output{Core::ERROR_UNAVAILABLE};" % retval.str_nocvref)
                        else:
                            emit.Line("%s output{};" % retval.str_nocvref)
                        emit.Line("return (output);")

                count += 1

                emit.IndentDec()
                emit.Line("}")

                if (iface.obj.methods.index(m) != (len(iface.obj.methods) - 1)):
                    emit.Line()

            emit.IndentDec()
            emit.Line("}; // class %s" % class_name)
            emit.Line()

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

        if announce_list:
            emit.Line()

        for key, val in announce_list.items():
            emit.Line("typedef ProxyStub::UnknownStubType<%s, %s> %s;" % (key, val[1], val[2]))

        emit.Line()

        emit.Line("static class Instantiation {")
        emit.Line("public:")
        emit.IndentInc()
        emit.Line("Instantiation()")
        emit.Line("{")
        emit.IndentInc()

        for key, val in announce_list.items():
            emit.Line("RPC::Administrator::Instance().Announce<%s, %s, %s>();" % (key, val[0], val[2]))

        emit.IndentDec()
        emit.Line("}")

        emit.Line("~Instantiation()")
        emit.Line("{")
        emit.IndentInc()

        for key, val in announce_list.items():
            emit.Line("RPC::Administrator::Instance().Recall<%s>();" % (key))

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
                           help="Generate stub and proxy C++ code (default)")
    argparser.add_argument("--lua-code",
                           dest="lua_code",
                           action="store_true",
                           default=False,
                           help="Generate lua code with interface information")
    argparser.add_argument("-i",
                           dest="extra_includes",
                           metavar="FILE",
                           action='append',
                           default=[],
                           help="include a additional C++ header file(s), default: Ids.h")
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
    argparser.add_argument("--indent",
                           dest="indent_size",
                           metavar="SIZE",
                           type=int,
                           action="store",
                           default=INDENT_SIZE,
                           help="set code indentation in spaces (default: %i)" % INDENT_SIZE)
    argparser.add_argument("--traces",
                           dest="traces",
                           action="store_true",
                           default=False,
                           help="emit traces in generated proxy/stub code (default: no extra traces)")
    argparser.add_argument("--old-cpp",
                           dest="old_cpp",
                           action="store_true",
                           default=False,
                           help="do not emit some C++11 keywords (default: emit modern C++ code)")
    argparser.add_argument("--no-warnings",
                           dest="no_warnings",
                           action="store_true",
                           default=not SHOW_WARNINGS,
                           help="suppress all warnings (default: show warnings)")
    argparser.add_argument("--keep",
                           dest="keep_incomplete",
                           action="store_true",
                           default=False,
                           help="keep incomplete files (default: remove partially generated files)")
    argparser.add_argument("--verbose",
                           dest="verbose",
                           action="store_true",
                           default=BE_VERBOSE,
                           help="enable verbose logging (default: verbose logging disabled)")
    argparser.add_argument("--force",
                           dest="force",
                           action="store_true",
                           default=FORCE,
                           help="force stub generation even if destination appears up-to-date (default: force disabled)")
    argparser.add_argument('-I', dest="includePaths", metavar="INCLUDE_DIR", action='append', default=[], type=str,
                           help='add an include path (can be used multiple times)')

    args = argparser.parse_args(sys.argv[1:])
    INDENT_SIZE = args.indent_size if (args.indent_size > 0 and args.indent_size < 32) else INDENT_SIZE
    USE_OLD_CPP = args.old_cpp
    SHOW_WARNINGS = not args.no_warnings
    BE_VERBOSE = args.verbose
    FORCE = args.force
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
        print("   @stop                  - skip parsing of the rest of the file")
        print("   @omit                  - omit generating code for the next item (class or method)")
        print("   @stub                  - generate empty stub for the next item (class or method)")
        print("   @insert \"file\"         - include another file, relative to the directory of the current file")
        print("   @insert <file>         - include another file, relative to the defined include directories")
        print("")
        print("For non-const pointer and reference method parameters:")
        print("   @in                    - denotes an input parameter")
        print("   @out                   - denotes an output parameter")
        print("   @inout                 - denotes an input/output parameter (equivalent to @in @out)")
        print("   @interface:{expr}      - specifies a parameter holding interface ID value for void* interface passing")
        print("   @length:{expr}         - specifies a buffer length value (a constant, a parameter name or a math expression)")
        print("   @maxlength:{expr}      - specifies a maximum buffer length value (a constant, a parameter name or a math expression),")
        print("                            if not specified @length is used as maximum length, use round parenthesis for expressions",)
        print("                            e.g.: @length:bufferSize @length:(width*height*4)")
        print("")
        print("JSON-RPC-related parameters:")
        print("   @json                  - marks a class for JSON-RPC generation")
        print("   @compliant             - marks a class to be generated in JSON-RPC compliant format (default)")
        print("   @uncompliant:extended  - marks a class to be generated in the obsolete 'extended' format")
        print("   @uncompliant:collapsed - marks a class to be generated in the obsolete 'collapsed' format")
        print("   @json:omit             - unmarks a method from JSON-RPC generation")
        print("   @event                 - marks a class to be generated as an JSON-RPC event")
        print("   @property              - marks method to be generated as a JSON-RPC property")
        print("   @iterator              - marks a class to be generated as an JSON-RPC interator")
        print("   @text {name}           - sets an alternative name for an enum or a variable")
        print("   @brief {desc}          - sets a brief description for a JSON-RPC method, property or event")
        print("   @details {desc}        - sets a detailed description for a JSON-RPC method, property or event")
        print("   @param {name} {desc}   - sets a description for a parameter of a JSON-RPC method or event")
        print("   @retval {desc}         - sets a description for a return value of a JSON-RPC method")
        print("   @bitmask               - indicates that enumerator lists of an enum can be packed into into a bit mask")
        print("   @index                 - marks a parameter in a JSON-RPC property or event to be an index")
        print("   @deprecated            - marks a JSON-RPC method, property or event as deprecated in documentation")
        print("   @obsolete              - marks a JSON-RPC method, property or event as osbsolete in documentation")
        print("   @sourcelocation {lnk}  - sets source location link to be used in documentation")
        print("")
        print("Tags shall be placed inside C++ comments.")
        sys.exit()

    if not args.code and not args.lua_code:
        # Backwards compatibility if user selects nothing let's build proxystubs
        args.code = True

    if not args.path:
        argparser.print_help()
    else:
        interface_files = []
        for elem in args.path:
            if os.path.isdir(elem):
                interface_files += [
                    os.path.join(elem, f) for f in os.listdir(elem)
                    if (os.path.isfile(os.path.join(elem, f)) and re.match("I.*\\.h", f) and f != IDS_DEFINITIONS_FILE)
                ]
            elif os.path.isfile(elem):
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

                        output = GenerateStubs(
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
                            print("%s (%s) - '%s'" %
                                (hex(f.id) if isinstance(f.id, int) else "?", str(f.id), f.obj.full_name))
                        if i and sorted_faces[i - 1].id == f.id:
                            log.Warn(
                                "duplicate interface ID %s (%s) of %s" %
                                (hex(f.id) if isinstance(f.id, int) else "?", str(f.id), f.obj.full_name), f.file)
                    else:
                        log.Info("can't evaluate interface ID \"%s\" of %s" % (str(f.id), f.obj.full_name), f.file)

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
