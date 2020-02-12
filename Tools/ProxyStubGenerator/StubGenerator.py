#!/usr/bin/env python

#
# WPE Framework proxy stub code generator
#

import re
import uuid
import sys
import os
import argparse
import CppParser

VERSION = "1.5.4"
NAME = "ProxyStubGenerator"

# runtime changeable configuration
INDENT_SIZE = 4
SHOW_WARNINGS = True
USE_OLD_CPP = False
BE_VERBOSE = False
EMIT_TRACES = False

# static configuration
EMIT_DESTRUCTOR_FOR_PROXY_CLASS = False
EMIT_MODULE_NAME_DECLARATION = False
EMIT_COMMENT_WITH_PROTOTYPE = True
EMIT_COMMENT_WITH_STUB_ORDER = True
STUB_NAMESPACE = "::WPEFramework::ProxyStubs"
INTERFACE_NAMESPACE = "::WPEFramework::Exchange"
CLASS_IUNKNOWN = "::WPEFramework::Core::IUnknown"
PROXYSTUB_CPP_NAME = "ProxyStubs_%s.cpp"

MIN_INTERFACE_ID = 64

DEFAULT_DEFINITIONS_FILE = "default.h"
IDS_DEFINITIONS_FILE = "Ids.h"

# -------------------------------------------------------------------------
# Logger


class Log:

    def __init__(self):
        self.warnings = []
        self.errors = []
        self.infos = []

    def Info(self, text, file=""):
        if BE_VERBOSE:
            self.infos.append("%s: INFO: %s%s%s" %
                              (NAME, file, ": " if file else "", text))
            print(self.infos[-1])

    def Warn(self, text, file=""):
        if SHOW_WARNINGS:
            self.warnings.append("%s: WARNING: %s%s%s" %
                                 (NAME, file, ": " if file else "", text))
            print(self.warnings[-1])

    def Error(self, text, file=""):
        self.errors.append("%s: ERROR: %s%s%s" %
                           (NAME, file, ": " if file else "", text))
        print >> sys.stderr, self.errors[-1]

    def Print(self, text, file=""):
        print("%s: %s%s%s" % (NAME, file, ": " if file else "", text))

    def Dump(self):
        if self.errors or self.warnings or self.infos:
            print("")
            for item in self.errors + self.warnings + self.infos:
                print(item)


log = Log()

# -------------------------------------------------------------------------
# Exception classes


class SkipFileError(RuntimeError):
    pass


class NoInterfaceError(RuntimeError):
    pass


class TypenameError(RuntimeError):

    def __init__(self, type, msg):
        msg = "%s(%s): %s" % (type.parser_file, type.parser_line, msg)
        super(TypenameError, self).__init__(msg)
        pass


# -------------------------------------------------------------------------


def CreateName(ns):
    return ns.replace("::I", "").replace("::", "")[1 if ns[0] == "I" else 0:]


def GenerateStubs(output_file, source_file, defaults="", scan_only=False):

    class Interface():

        def __init__(self, obj, iid, file):
            self.obj = obj
            self.id = iid
            self.file = file

    # Looks for interface clasess (ie. classes inheriting from Core::Unknown and specifying ID enum).
    def FindInterfaceClasses(tree):
        interfaces = []

        def __Traverse(tree, faces):
            if isinstance(tree, CppParser.Namespace) or isinstance(
                    tree, CppParser.Class):
                for c in tree.classes:
                    if c.methods:
                        if (INTERFACE_NAMESPACE + "::") in c.full_name:
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
                                                faces.append(
                                                    Interface(
                                                        c, item.value,
                                                        source_file))
                                                has_id = True
                                                break
                                if not has_id:
                                    log.Warn(
                                        "class %s does not have ID enumerator" %
                                        c.full_name, source_file)
                            else:
                                log.Info(
                                    "class %s not does not inherit from %s" %
                                    (c.full_name, CLASS_IUNKNOWN), source_file)
                        else:
                            log.Info(
                                "class %s not in %s namespace" %
                                (c.full_name, INTERFACE_NAMESPACE), source_file)

                    __Traverse(c, faces)

            if isinstance(tree, CppParser.Namespace):
                for n in tree.namespaces:
                    __Traverse(n, faces)

        __Traverse(tree, interfaces)
        return interfaces

    if BE_VERBOSE:
        log.Print("Parsing '%s'..." % source_file)

    ids = os.path.join(os.path.dirname(source_file), IDS_DEFINITIONS_FILE)

    tree = CppParser.ParseFiles([defaults, ids, source_file])
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
                file.write(string)

            def Line(self, string=""):
                self.String(((self.indent + string) if len(
                    (self.indent + string).strip()) else "") + "\n")

        def EmitFunctionOrder(methods):
            if methods:
                emit.Line("// Methods:")
                c = 0
                for m in methods:
                    if "pure-virtual" in m.specifiers:
                        emit.Line("//  (%i) %s" % (c, SignatureStr(m, None)))
                        c += 1
                emit.Line("//")

        NULLPTR = "0" if USE_OLD_CPP else "nullptr"

        emit = Emitter(file, INDENT_SIZE)

        emit.Line("//")
        emit.Line("// generated automatically from \"%s\"" %
                  interface_header_name)
        emit.Line("//")
        emit.Line("// implements RPC proxy stubs for:")
        for face in interfaces:
            if not face.obj.omit:
                emit.Line(
                    "//   - class %s" %
                    face.obj.full_name.replace(INTERFACE_NAMESPACE + "::", ""))
        emit.Line("//")
        emit.Line()

        emit.Line('#include "%s"' % interface_header_name)
        emit.Line()

        if EMIT_MODULE_NAME_DECLARATION:
            emit.Line("MODULE_NAME_DECLARATION(BUILDREF_WEBBRIDGE)")
            emit.Line()

        emit.Line("namespace %s {" % STUB_NAMESPACE.split("::")[-2])
        emit.Line()
        emit.Line("namespace %s {" % STUB_NAMESPACE.split("::")[-1])
        emit.Line()
        emit.IndentInc()
        emit.Line("using namespace %s;" % iface_namespace)
        emit.Line()

        announce_list = {}

        #
        # EMIT STUB CODE
        #
        emit.Line(
            "// -----------------------------------------------------------------"
        )
        emit.Line("// STUB")
        emit.Line(
            "// -----------------------------------------------------------------\n"
        )

        for iface in interfaces:
            if (iface_namespace + "::") not in iface.obj.full_name:
                continue  # inteface in other namespace

            iface_name = iface.obj.full_name.split(iface_namespace + "::", 1)[1]
            array_name = CreateName(iface_name) + "StubMethods"
            class_name = CreateName(iface_name) + "Proxy"
            stub_name = CreateName(iface_name) + "Stub"

            # Stringifies a type, omitting outer namespace if necessary
            def TypeStr(type):
                ostr = ""
                for t in type:
                    strt = t.Name() if isinstance(
                        t, CppParser.Identifier) else str(t)
                    # cut the scope to current namespace
                    for n in iface_namespace_l:
                        if (n + "::") in strt:
                            strt = strt[strt.find(n + "::") + len(n) + 2:]
                    ostr += strt + " "
                return ostr.strip().replace(" *", "*").replace(" &", "&")

            class EmitType:

                def __init__(self, type_, cv=[]):

                    def _Typename(type, cv):
                        is_ref = False
                        is_ptr = False
                        typename_idx = len(type) - 1
                        while type[typename_idx] in ["const", "volatile"]:
                            typename_idx -= 1
                        typename_idx2 = typename_idx
                        if type[typename_idx] == "&":
                            is_ref = True
                            typename_idx -= 1
                        if type[typename_idx] == "*":
                            is_ptr = True
                        while self.type[typename_idx] in ["*", "&"]:
                            typename_idx -= 1
                        if cv and not is_ptr and not is_ref and (
                                type[-1] != "void") and (type[0] not in [
                                    "const", "volatile"
                                ]):
                            type = cv + type
                            typename_idx += len(cv)
                        return is_ref, is_ptr, typename_idx, typename_idx2, type

                    type = type_.type
                    input = type_.input
                    self.interface = type_.interface
                    output = type_.output
                    length = type_.length
                    maxlength = type_.maxlength
                    interface = type_.interface
                    origname = type_.name
                    self.ocv = cv
                    self.oclass = type_
                    self.unexpanded = type
                    self.type = self._ExpandTypedefs(type)
                    self.is_ref, self.is_ptr, idx, _, self.type = _Typename(
                        self.type, cv)
                    _, _, typename_idx, typename_idx2, self.unexpanded = _Typename(
                        self.unexpanded, cv)
                    self.is_nonconstref = self.is_ref and self.type[0] != "const"
                    self.is_nonconstptr = self.is_ptr and self.type[0] != "const"

                    self.typename = self.unexpanded[typename_idx]
                    self.expanded_typename = self.type[idx]
                    self.obj = self.expanded_typename if isinstance(
                        self.expanded_typename, CppParser.Class) else None
                    self.str = TypeStr(self.unexpanded)
                    self.str_typename = TypeStr([self.typename])
                    self.str_noptrref = TypeStr(self.unexpanded[:typename_idx +
                                                                1])
                    self.str_nocvref = self.str_typename + \
                        ("*" if self.is_ptr else "")
                    i = len(self.unexpanded) - 1
                    while self.unexpanded[i] in ["const", "volatile", "&"]:
                        i -= 1
                    self.str_noref = TypeStr(self.unexpanded[:i + 1])
                    self.is_interface = interface or (self.is_ptr and self.obj)

                    self.str_rpctype = None
                    self.str_rpctype_nocv = None
                    self.is_inputptr = self.is_ptr and (input or
                                                        not self.is_nonconstptr)
                    self.is_outputptr = self.is_nonconstptr and output
                    self.is_inputref = self.is_ref and (input or
                                                        not self.is_nonconstref)
                    self.is_outputref = self.is_nonconstref and output
                    self.is_output = self.is_outputptr or self.is_outputref
                    self.is_input = self.is_inputptr or self.is_inputref
                    self.ptr_length = length
                    self.ptr_maxlength = maxlength
                    self.ptr_interface = self.interface
                    self.proxy = self.is_interface and (not self.is_ref
                                                        or self.is_input)
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
                    self.length_type = "uint16_t"
                    self.str_nocv = TypeStr(
                        self.type[typename_idx:typename_idx2 + 1])

                    if not self._IsValid(self.typename):
                        raise TypenameError(
                            type_,
                            "unable to serialise '%s %s': undefined type '%s'" %
                            (self.CppType(), self.origname, self.str_typename))
                    if not self.obj and self.is_nonconstptr and not self.is_inputptr and not self.is_outputptr and not interface:
                        raise TypenameError(
                            type_,
                            "unable to serialise '%s %s': a non-const pointer requires an in/out tag"
                            % (self.CppType(), self.origname))
                    if not self.obj and self.is_nonconstref and not self.is_inputref and not self.is_outputref and not interface:
                        raise TypenameError(
                            type_,
                            "unable to serialise '%s %s': a non-const reference requires an in/out tag"
                            % (self.CppType(), self.origname))
                    if self.obj and self.is_nonconstref and self.is_nonconstptr and not self.is_inputref and not self.is_outputref and not interface:
                        raise TypenameError(
                            type_,
                            "unable to serialise '%s %s': a non-const reference to pointer requires an in/out tag"
                            % (self.CppType(), self.origname))
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
                    return self.str

                def RpcType(self):
                    if self.str_rpctype == None:
                        self.str_rpctype = self._RpcType(self.str_noref)
                    return self.str_rpctype

                def RpcTypeNoCV(self):
                    if self.str_rpctype_nocv == None:
                        self.str_rpctype_nocv = self._RpcType(self.str_nocvref)
                    return self.str_rpctype_nocv

                def _IsNumeric(self, type):
                    if type in ["size_t", "time_t", "float", "uint64_t", "int64_t", "uint32_t", "int32_t", "uint16_t", "int16_t", "uint8_t", "int8_t"] \
                            or str(type).split(" ")[-1] in ["char", "short", "long", "int", "signed", "unsigned", "double", "wchar_t"]:
                        return True
                    return False

                def _IsBoolean(self, type):
                    return type in ["bool", "Bool", "Boolean"]

                def _IsString(self, type):
                    return type in ["std::string", "string"]

                def _IsEnum(self, type):
                    return isinstance(type, CppParser.Enum)

                def _IsTypedef(self, type):
                    return isinstance(type, CppParser.Typedef)

                def _IsObject(self, type):
                    return isinstance(type, CppParser.Class)

                def _IsValid(self, type):
                    return self._IsNumeric(type) or self._IsBoolean(
                        type) or self._IsString(type) or self._IsEnum(
                            type) or self._IsTypedef(type) or self._IsObject(
                                type) or type == "void"

                # Converts a C++ type to RPC types
                def _RpcType(self, noref):
                    if self.is_ptr:
                        if self.is_interface:
                            return "Number<%s>" % noref
                        else:
                            return "Buffer<%s>" % self.length_type
                    elif self._IsEnum(self.expanded_typename):
                        return "Number<%s>" % noref
                    elif self._IsNumeric(self.expanded_typename):
                        return "Number<%s>" % noref
                    elif self._IsString(self.expanded_typename):
                        return "Text"
                    elif self._IsBoolean(self.expanded_typename):
                        return "Boolean"
                    elif self._IsTypedef(self.expanded_typename):
                        et = EmitType(self.oclass, self.ocv)
                        return et._RpcType(noref)
                    else:
                        raise TypenameError(
                            self.oclass,
                            "unable to serialise type '%s'" % self.CppType())

                def _ExpandTypedefs(self, typ):
                    expanded = []
                    for t in typ:
                        if isinstance(t, CppParser.Typedef):
                            expanded += self._ExpandTypedefs(t.type)
                        else:
                            expanded.append(t)
                    return expanded

            class EmitParam(EmitType):

                def __init__(self, type_, name="param", cv=[]):
                    EmitType.__init__(self, type_, cv)
                    self.name = name

            class EmitRetVal(EmitParam):

                def __init__(self, type_, name="output", cv=[]):
                    EmitParam.__init__(self, type_, name, cv)
                    self.has_output = self.type and (self.type[-1] != "void")
                    self.is_output = True

            # Stringify a method object to full signature
            def SignatureStr(method, parameters=None):
                params = parameters if parameters else method.vars
                sig = ""
                if "pure-virtual" in method.specifiers or "virtual" in method.specifiers:
                    sig += "virtual "
                sig += "%s %s(" % (TypeStr(method.type), method.name)
                for c, p in enumerate(params):
                    acc = ""
                    if parameters:
                        if p.is_nonconstref or p.is_nonconstptr:
                            if p.is_input and p.is_output:
                                acc += " [inout]"
                            elif p.is_input:
                                acc += " [in]"
                            elif p.is_output:
                                acc += " [out]"
                    sig += TypeStr(p.unexpanded if isinstance(p, EmitType) else p.type) + \
                        acc + (", " if c != len(method.vars) - 1 else "")
                sig += ")"
                for q in method.qualifiers:
                    sig += " " + q
                if "pure-virtual" in method.specifiers:
                    sig += " = 0"
                return sig

            # Stringify a method object to a prototype
            def PrototypeStr(method, parameters=None):
                params = parameters if parameters else method.vars
                proto = "%s %s(" % (TypeStr(method.type), method.name)
                for c, p in enumerate(params):
                    acc = ""
                    if parameters:
                        if p.is_nonconstref or p.is_nonconstptr:
                            if p.is_input and p.is_output:
                                acc += " /* inout */"
                            elif p.is_input:
                                acc += " /* in */"
                            elif p.is_output:
                                acc += " /* out */"
                    proto += TypeStr(p.unexpanded) + acc + " param%i%s" % (c, (
                        ", " if c != len(method.vars) - 1 else ""))
                proto += ")"
                for q in method.qualifiers:
                    proto += " " + q
                return proto

            def _ConstCast(typ, identifier):
                return "const_cast<%s>(%s)" % (typ, identifier)

            if iface.obj.omit:
                log.Print("omitted class %s" % iface.obj.full_name, source_file)
                continue

            emit_methods = [
                m for m in iface.obj.methods if "pure-virtual" in m.specifiers
            ]
            if not emit_methods:
                log.Warn(
                    "nothing emit for interface class %s" % iface.obj.full_name,
                    source_file)
                continue

            if BE_VERBOSE:
                log.Print("Emitting stub code for interface '%s'..." %
                          iface_name)

            # build the announce list upfront
            announce_list[iface_name] = [
                class_name, array_name, stub_name, iface
            ]

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
                        if p.is_ptr and not p.is_ref and (not p.is_interface
                                                          or p.interface):

                            def __ParseLength(length, maxlength, target,
                                              length_name):
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
                                                            %
                                                            (p.origname,
                                                             "max" if maxlength
                                                             else "", token))
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
                                                        % (p.origname, "max" if
                                                           maxlength else ""))
                                                parsed.append(token)
                                                sizeof_parsing = -True
                                            elif sizeof_parsing:
                                                parsed.append(token)
                                            else:
                                                raise TypenameError(
                                                    p.oclass,
                                                    "unable to serialise '%s': %slength variable '%s' not found"
                                                    %
                                                    (p.origname, "max" if
                                                     maxlength else "", token))
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
                                    p.ptr_interface, True, c - 1,
                                    p.interface_var)
                                if constant:
                                    raise TypenameError(
                                        p.oclass,
                                        "unable to serialise '%s': constant not allowed for interface id"
                                        % (p.origname))
                            else:
                                if p.is_output and p.ptr_maxlength and p.ptr_maxlength != p.ptr_length:
                                    if p.ptr_length:
                                        p.maxlength_var = p.name + "_maxlength"
                                        p.maxlength_expr, p.length_type, p.maxlength_constant, p.length_ref = __ParseLength(
                                            p.ptr_maxlength, True, c - 1,
                                            p.maxlength_var)
                                    else:
                                        p.ptr_length = p.ptr_maxlength
                                if p.ptr_length:
                                    p.length_var = p.name + "_length"
                                    p.length_expr, p.length_type, p.length_constant, p.length_ref = __ParseLength(
                                        p.ptr_length, False, c - 1,
                                        p.length_var)
                            if not p.ptr_length and not p.ptr_interface:
                                raise TypenameError(
                                    p.oclass,
                                    "unable to serialise '%s': length variable not defined"
                                    % (p.origname))

            for m in emit_methods:
                if m.omit:
                    log.Print("omitted method %s" % iface.obj.full_name,
                              source_file)
                    emit.Line("// method omitted")
                    emit.Line("//")
                    emit.Line("")
                    continue
                elif BE_VERBOSE:
                    log.Print("  generating code for %s()" % m.full_name)

                proxy_count = 0
                output_params = 0

                # enumerate and prepare parameters for emitting
                # force non-ptr, non-ref parameters to be const
                retval = EmitRetVal(m, cv=["const"])
                params = [EmitParam(v, cv=["const"]) for v in m.vars]
                orig_params = [EmitParam(v) for v in m.vars]
                for c, p in enumerate(params):
                    if p.proxy and p.obj:
                        proxy_count += 1
                    if p.is_output:
                        output_params += 1
                    p.name += str(c)

                LinkPointers(retval, params)
                # emit a comment with function signature (optional)
                if EMIT_COMMENT_WITH_PROTOTYPE:
                    emit.Line("// " + SignatureStr(m, orig_params))
                    emit.Line("//")

                # emit the lambda prototype
                emit.Line(
                    "[](Core::ProxyType<Core::IPCChannel>& channel%s, Core::ProxyType<RPC::InvokeMessage>& message) {"
                    % (" VARIABLE_IS_NOT_USED" if not proxy_count else ""))
                emit.IndentInc()

                if EMIT_TRACES:
                    emit.Line(
                        'fprintf(stderr, "*** [%s stub] ENTER: %s()\\n");' %
                        (iface.obj.full_name, m.name))
                    emit.Line()

                emit.Line("RPC::Data::Input& input(message->Parameters());")

                if not m.stub:
                    emit.Line()

                    # emit parameter readout
                    if params:
                        emit.Line("// read parameters")
                        emit.Line(
                            "RPC::Data::Frame::Reader reader(input.Reader());")
                        for c, p in enumerate(params):
                            if (p.is_length or p.is_maxlength):
                                if p.is_ptr:
                                    raise TypenameError(
                                        p.oclass,
                                        "unsupported: '%s' length variable is a pointer"
                                        % p.origname)
                                elif p.obj:
                                    raise TypenameError(
                                        p.oclass,
                                        "'%s' length variable is an object" %
                                        p.origname)

                            # if parameter is passed by value or by reference, then try to  decompose it
                            if not p.is_ptr and not p.CheckRpcType():
                                if p.obj:
                                    emit.Line("// (decompose %s)" %
                                              p.str_typename)
                                    emit.Line("%s %s;" %
                                              (p.str_typename, p.name))
                                    # TODO: make this recursive
                                    if p.obj.vars:
                                        for attr in p.obj.vars:
                                            emit.Line(
                                                "param%i.%s = reader.%s();" %
                                                (c, attr.name,
                                                 EmitParam(attr).RpcTypeNoCV()))
                                    else:
                                        raise TypenameError(
                                            m,
                                            "method '%s': unable to decompose parameter '%s': non-POD type"
                                            % (m.name, p.str_typename))
                                elif not p.RpcType():
                                    raise TypenameError(
                                        m,
                                        "method '%s': unable to decompose parameter '%s': unknown type"
                                        % (m.name, p.str_typename))
                            else:
                                if p.is_ptr and not p.obj and not p.is_ref and p.length_type == "void":
                                    emit.Line("%s %s = %s; // storage" %
                                              (p.str_typename, p.name, NULLPTR))
                                elif p.is_ptr and not p.obj and not p.is_ref:
                                    if p.is_input:
                                        emit.Line(
                                            "const %s %s = %s;" %
                                            (p.str_nocvref, p.name, NULLPTR))
                                        emit.Line(
                                            "%s %s_length = reader.Lock%s(%s);"
                                            % (p.length_type, p.name,
                                               p.RpcTypeNoCV(), p.name))
                                        emit.Line(
                                            "reader.UnlockBuffer(%s_length);" %
                                            p.name)
                                elif p.is_ref and not p.is_input:
                                    emit.Line("%s %s{}; // storage" %
                                              (p.str_nocvref, p.name))
                                    if p.is_length or p.is_maxlength:
                                        raise TypenameError(
                                            p.oclass,
                                            "'%s' is defined as a length variable but is write-only"
                                            % p.origname)
                                elif not p.is_length or p.is_maxlength or not params[
                                        p.length_target].is_input:
                                    emit.Line("%s %s = reader.%s();" %
                                              (p.str_nocvref if p.proxy else
                                               p.str_noref, p.length_name
                                               if p.is_length else p.name,
                                               p.RpcTypeNoCV()))
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
                                                emit.Line(
                                                    "// allocate receive buffer if necessary"
                                                )
                                            oldname = p.name
                                            p.name = p.name + "_buffer"
                                        elif not p.is_input:
                                            emit.Line(
                                                "// allocate receive buffer")
                                        emit.Line("%s %s{};" %
                                                  (p.str_nocvref, p.name))
                                        length_var = p.maxlength_var if p.maxlength_var else p.length_var
                                        if p.length_constant:
                                            emit.Line(
                                                "const %s %s = %s;" %
                                                (p.length_type, p.length_var,
                                                 p.length_expr))
                                        if p.is_input:
                                            if p.maxlength_var:
                                                # input/output and maxlength defined
                                                emit.Line(
                                                    "%s = const_cast<%s>(%s);" %
                                                    (p.name, p.str_nocvref,
                                                     oldname))
                                                emit.Line("if (%s > %s) {" %
                                                          (p.maxlength_var,
                                                           p.length_var))
                                                emit.IndentInc()
                                                if p.length_type not in [
                                                        "char", "short",
                                                        "int8_t", "uint8_t",
                                                        "int16_t", "uint16_t"
                                                ]:
                                                    emit.Line(
                                                        "ASSERT((%s < 0x10000) && \"Buffer too large\");"
                                                        % length_var)
                                                emit.Line(
                                                    "%s = static_cast<%s>(ALLOCA(%s));"
                                                    % (p.name, p.str_nocvref,
                                                       length_var))
                                                emit.Line(
                                                    "ASSERT(%s != nullptr);" %
                                                    p.name)
                                            else:
                                                # is input/output but maxlength not defined
                                                emit.Line(
                                                    "%s = const_cast<%s>(%s); // reuse the input buffer"
                                                    % (p.name, p.str_nocvref,
                                                       oldname))
                                        else:
                                            # is output-only
                                            if not p.length_constant:
                                                emit.Line("if (%s != 0) {" %
                                                          p.length_var)
                                                emit.IndentInc()
                                            if p.length_type not in [
                                                    "char", "short", "int8_t",
                                                    "uint8_t", "int16_t",
                                                    "uint16_t"
                                            ]:
                                                emit.Line(
                                                    "ASSERT((%s < 0x10000) && \"Buffer length too big\");"
                                                    % length_var)
                                            emit.Line(
                                                "%s = static_cast<%s>(ALLOCA(%s));"
                                                % (p.name, p.str_nocvref,
                                                   length_var))
                                            emit.Line("ASSERT(%s != nullptr);" %
                                                      p.name)
                                            if not p.length_constant:
                                                emit.IndentDec()
                                                emit.Line("}")
                                        if p.is_input and p.maxlength_var:
                                            emit.Line(
                                                "::memcpy(%s, %s, %s);" %
                                                (p.name, oldname, p.length_var))
                                            emit.IndentDec()
                                            emit.Line("}")

                        # emit proxy for the parameters if applicable
                        for p in params:
                            if p.proxy:
                                proxy_name = p.name + "_proxy"
                                emit.Line("%s %s = %s;" %
                                          (p.str_nocvref, proxy_name, NULLPTR))
                                emit.Line(
                                    "ProxyStub::UnknownProxy* %s_inst = %s;" %
                                    (proxy_name, NULLPTR))
                                emit.Line("if (%s != %s) {" % (p.name, NULLPTR))
                                emit.IndentInc()
                                # create proxy
                                emit.Line(
                                    "%s_inst = RPC::Administrator::Instance().ProxyInstance(channel, %s, %s::ID, false, %s::ID, true);"
                                    % (proxy_name, p.name, p.str_typename,
                                       p.str_typename))
                                emit.Line("if (%s_inst != %s) {" %
                                          (proxy_name, NULLPTR))
                                emit.IndentInc()
                                emit.Line(
                                    "%s = %s_inst->QueryInterface<%s>();" %
                                    (proxy_name, proxy_name, p.str_typename))
                                emit.IndentDec()
                                emit.Line("}")
                                emit.Line()
                                emit.Line(
                                    "ASSERT((%s != %s) && \"Failed to get instance of %s proxy\");"
                                    % (proxy_name, NULLPTR, p.str_typename))
                                emit.Line("if (%s == %s) {" %
                                          (proxy_name, NULLPTR))
                                emit.IndentInc()
                                emit.Line(
                                    "TRACE_L1(\"Failed to get instance of %s proxy\");"
                                    % p.str_typename)
                                emit.IndentDec()
                                emit.Line("}")
                                emit.IndentDec()
                                emit.Line("}")
                        emit.Line()

                    if (retval.has_output or output_params) and proxy_count:
                        emit.Line("// write return value%s" %
                                  ("s" if
                                   (int(retval.has_output) + output_params > 1)
                                   else ""))
                        emit.Line(
                            "RPC::Data::Frame::Writer writer(message->Response().Writer());"
                        )
                        emit.Line()

                    # emit code to validate the proxy(s)
                    if proxy_count:
                        if_proxy_line = "if " + \
                            ("(" if proxy_count > 1 else "")
                        c = 0
                        for p in params:
                            if p.proxy:
                                if_proxy_line += "((" + p.name + " == nullptr) || (" + p.name + "_proxy != %s" % NULLPTR + "))" + (
                                    " && " if (c != proxy_count - 1) else "")
                                c += 1
                        if_proxy_line += (")" if proxy_count > 1 else "") + " {"
                        emit.Line(if_proxy_line)
                        emit.IndentInc()

                    # emit function call
                    emit.Line("// call implementation")
                    emit.Line(
                        "%s* implementation = input.Implementation<%s>();" %
                        ((" ".join(m.qualifiers) + " " + iface_name).strip(),
                         iface_name))
                    emit.Line(
                        "ASSERT((implementation != %s) && \"Null %s implementation pointer\");"
                        % (NULLPTR, iface_name))
                    call = ""
                    if retval.has_output:
                        call += "%s %s = " % (retval.str_noref, retval.name)
                    call += "implementation->%s(" % m.name
                    for c, p in enumerate(params):
                        call += "%s%s%s%s" % (
                            "&" if p.length_type == "void" else "", p.name,
                            ("_proxy" if p.proxy else ""),
                            (", " if c < len(params) - 1 else ""))
                    call += ");"
                    emit.Line(call)

                    if (retval.has_output or output_params) and not proxy_count:
                        emit.Line()
                        emit.Line("// write return value%s" %
                                  ("s" if
                                   (int(retval.has_output) + output_params > 1)
                                   else ""))
                        emit.Line(
                            "RPC::Data::Frame::Writer writer(message->Response().Writer());"
                        )

                    # forward the output value
                    if retval.has_output:
                        if not retval.is_ptr and not retval.CheckRpcType():
                            if retval.obj:
                                emit.Line("// (decompose %s)" %
                                          retval.str_typename)
                                if retval.obj.vars:
                                    for attr in retval.obj.vars:
                                        emit.Line(
                                            "writer.%s(output.%s);" %
                                            (EmitParam(attr).RpcTypeNoCV(),
                                             attr.name))
                                else:
                                    raise TypenameError(
                                        m,
                                        "method '%s': unable to decompose parameter type '%s': non-POD type"
                                        % (m.name, retval.str_typename))
                            elif not retval.RpcType():
                                raise TypenameError(
                                    m,
                                    "method '%s': unable to decompose parameter '%s': unknown type"
                                    % (m.name, retval.str_typename))
                        else:
                            emit.Line("writer.%s(%s);" %
                                      (retval.RpcType(), retval.name))
                            if retval.is_interface:
                                if retval.typename == "void":
                                    emit.Line(
                                        "RPC::Administrator::Instance().RegisterInterface(channel, %s, %s);"
                                        % (retval.name,
                                           retval.interface_ref.length_name))
                                else:
                                    emit.Line(
                                        "RPC::Administrator::Instance().RegisterInterface(channel, %s);"
                                        % retval.name)

                    if output_params:
                        for p in params:
                            if not p.obj and p.is_outputptr and not p.is_ref:
                                if p.length_type == "void":
                                    # temporarily remove the pointer
                                    temp = p.oclass
                                    temp.type.remove("*")
                                    emit.Line(
                                        "writer.%s(%s);" %
                                        (EmitType(temp).RpcType(), p.name))
                                else:
                                    if p.length_var and p.length_ref and p.length_ref.is_output:
                                        emit.Line("writer.%s(%s);" %
                                                  (p.length_ref.RpcType(),
                                                   p.length_var))
                                    emit.Line("if ((%s != %s) && (%s != 0)) {" %
                                              (p.name, NULLPTR, p.length_var))
                                    emit.IndentInc()
                                    emit.Line("writer.%s(%s, %s);" %
                                              (p.RpcType(),
                                               p.length_var if p.length_var else
                                               p.maxlength_var, p.name))
                                    emit.IndentDec()
                                    emit.Line("}")
                            elif p.is_nonconstref:
                                if not p.is_length:
                                    emit.Line("writer.%s(%s);" %
                                              (p.RpcType(), p.name))
                                if p.is_interface:
                                    emit.Line(
                                        "RPC::Administrator::Instance().RegisterInterface(channel, %s);"
                                        % p.name)

                    if proxy_count:
                        emit.IndentDec()
                        emit.String(emit.indent + "}")
                        if any(x in ["uint32_t", "int", "long"]
                               for x in str(retval.typename).split()):
                            emit.String(" else {")
                            emit.Line()
                            emit.IndentInc()
                            emit.Line("// return error code")
                            emit.Line(
                                "writer.Number<const %s>(Core::ERROR_RPC_CALL_FAILED);"
                                % retval.typename)
                            emit.IndentDec()
                            emit.Line("}")
                        else:
                            emit.Line()

                        # emit release proxy call if applicable
                        emit.Line()
                        for p in params:
                            if p.proxy:
                                emit.Line("if (%s_proxy_inst != %s) {" %
                                          (p.name, NULLPTR))
                                emit.IndentInc()
                                emit.Line(
                                    "RPC::Administrator::Instance().Release(%s_proxy_inst, message->Response());"
                                    % p.name)
                                emit.IndentDec()
                                emit.Line("}")

                else:
                    log.Print("stubbed method %s" % m.full_name, source_file)
                    if params:
                        emit.Line(
                            "RPC::Data::Frame::Reader reader(input.Reader());")
                    if retval.has_output:
                        emit.Line(
                            "RPC::Data::Frame::Writer writer(message->Response().Writer());"
                        )
                    emit.Line("// TODO")

                if EMIT_TRACES:
                    emit.Line()
                    emit.Line(
                        'fprintf(stderr, "*** [%s stub] EXIT: %s()\\n");' %
                        (iface.obj.full_name, m.name))

                emit.IndentDec()
                emit.Line("},\n")

            emit.Line(NULLPTR)
            emit.IndentDec()
            emit.Line("}; // %s[]\n" % array_name)

        #
        # EMIT PROXY CODE
        #
        emit.Line(
            "// -----------------------------------------------------------------"
        )
        emit.Line("// PROXY")
        emit.Line(
            "// -----------------------------------------------------------------\n"
        )

        for iface in interfaces:
            if (iface_namespace + "::") not in iface.obj.full_name:
                continue
            iface_name = iface.obj.full_name.split(INTERFACE_NAMESPACE + "::",
                                                   1)[1]
            if not iface_name in announce_list:
                continue

            class_name = announce_list[iface_name][0]

            emit_methods = [
                m for m in iface.obj.methods if "pure-virtual" in m.specifiers
            ]
            if not emit_methods:
                continue

            if BE_VERBOSE:
                log.Print("Emitting proxy code for interface '%s'..." %
                          iface_name)

            emit.Line("//")
            emit.Line("// %s interface proxy definitions" % (iface_name))
            emit.Line("//")

            # emit stub order comment
            if EMIT_COMMENT_WITH_STUB_ORDER:
                EmitFunctionOrder(emit_methods)

            emit.Line()

            # emit proxy class definition
            emit.Line(
                "class %s%s : public ProxyStub::UnknownProxyType<%s> {" %
                (class_name, " final" if not USE_OLD_CPP else " /* final */",
                 iface_name))
            emit.Line("public:")
            emit.IndentInc()

            # emit constructor
            emit.Line(
                "%s(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)"
                % class_name)
            emit.Line(
                "    : BaseClass(channel, implementation, otherSideInformed)")
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

                for c, p in enumerate(params):
                    p.name += str(c)
                    if (not p.is_nonconstref and not p.is_nonconstptr) or (
                            p.is_input
                            and not p.is_length) or (p.is_ptr and p.obj) or (
                                p.is_length
                                and not params[p.length_target].is_input):
                        input_params += 1

                method_line = PrototypeStr(m, orig_params) + (
                    " override" if not USE_OLD_CPP else " /* override */")

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
                    emit.Line(
                        'fprintf(stderr, "*** [%s proxy] ENTER: %s()\\n");' %
                        (iface.obj.full_name, m.name))
                    emit.Line()

                emit.Line("IPCMessage newMessage(BaseClass::Message(%i));" %
                          count)
                emit.Line()
                count += 1
                proxy_params = 0
                output_params = 0

                if not m.stub:
                    if input_params:
                        emit.Line("// write parameters")
                        emit.Line(
                            "RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());"
                        )

                        for c, p in enumerate(params):
                            if not p.is_ptr and not p.CheckRpcType():
                                if p.obj:
                                    if p.obj.vars:
                                        emit.Line("// (decompose %s)" %
                                                  p.str_typename)
                                        for attr in p.obj.vars:
                                            emit.Line(
                                                "writer.%s(param%i.%s);" %
                                                (EmitParam(attr).RpcTypeNoCV(),
                                                 c, attr.name))
                                    else:
                                        raise TypenameError(
                                            m,
                                            "method '%s': unable to decompose parameter '%s': non-POD type"
                                            % (m.name, p.str_typename))
                                elif not p.RpcType():
                                    raise TypenameError(
                                        m,
                                        "method '%s': unable to decompose parameter '%s': unknown type"
                                        % (m.name, p.str_typename))
                            else:
                                if p.is_ptr and p.obj:
                                    proxy_params += 1
                                if not p.obj and p.is_ptr:
                                    if p.is_input:
                                        emit.Line(
                                            "writer.%s(%s, param%i);" %
                                            (p.RpcType(), p.length_expr, c))
                                elif not p.is_input and p.is_nonconstref and p.is_nonconstptr:
                                    pass
                                elif (not p.is_length
                                      or not params[p.length_target].is_input
                                      or p.is_maxlength) and (
                                          p.is_input or
                                          (not p.is_nonconstref
                                           and not p.is_nonconstptr) or p.obj):
                                    emit.Line("writer.%s(param%i);" %
                                              (p.RpcType(), c))
                        emit.Line()

                    for c, p in enumerate(params):
                        if not p.is_ptr and not p.CheckRpcType():
                            pass
                        else:
                            if (p.is_nonconstref and p.obj) or (
                                    not p.obj and p.is_outputptr) or (
                                        p.is_nonconstref and not p.is_length):
                                output_params += 1

                    retval_has_proxy = retval.has_output and retval.is_interface

                    emit.Line("// invoke the method handler")
                    if retval.has_output:
                        default = "{}"
                        if isinstance(retval.typename,
                                      (CppParser.Typedef, CppParser.Enum)):
                            default = " = static_cast<%s>(~0)" % retval.str_nocvref
                        emit.Line(
                            "%s %s%s%s;" %
                            (retval.str_nocvref, retval.name,
                             "_proxy" if retval_has_proxy else "", default))
                        # assume it's a status code
                        if any(x in ["uint32_t"]
                               for x in str(retval.typename).split()):
                            emit.Line(
                                "if ((%s = Invoke(newMessage)) == Core::ERROR_NONE) {"
                                % retval.name)
                        else:
                            emit.Line(
                                "if (Invoke(newMessage) == Core::ERROR_NONE) {")
                        emit.IndentInc()
                    elif proxy_params + output_params > 0:
                        emit.Line(
                            "if (Invoke(newMessage) == Core::ERROR_NONE) {")
                        emit.IndentInc()
                    else:
                        emit.Line("Invoke(newMessage);")

                    if retval.has_output or (output_params > 0) or (proxy_params
                                                                    > 0):
                        emit.Line("// read return value%s" %
                                  ("s" if
                                   (int(retval.has_output) + output_params > 1)
                                   else ""))
                        emit.Line(
                            "RPC::Data::Frame::Reader reader(newMessage->Response().Reader());"
                        )

                    if retval.has_output:
                        if retval.is_interface:
                            if retval.obj:
                                emit.Line(
                                    "%s_proxy = reinterpret_cast<%s>(Interface(reader.Number<void*>(), %s::ID));"
                                    % (retval.name, retval.str_nocvref,
                                       retval.str_typename))
                            else:
                                emit.Line(
                                    "%s_proxy = Interface(reader.Number<void*>(),%s);"
                                    % (retval.name, retval.interface_expr))
                        else:
                            if not retval.is_ptr and not retval.CheckRpcType():
                                if retval.obj:
                                    emit.Line("// (decompose %s)" %
                                              retval.str_typename)
                                    if retval.obj.vars:
                                        for attr in retval.obj.vars:
                                            emit.Line(
                                                "%s.%s = reader.%s();" %
                                                (retval.name, attr.name,
                                                 EmitParam(attr, cv=[
                                                     "const"
                                                 ]).RpcTypeNoCV()))
                                    else:
                                        raise TypenameError(
                                            m,
                                            "method '%s': unable to decompose return value '%s': non-POD type"
                                            % (m.name, retval.str_typename))
                                elif not retval.RpcType():
                                    raise TypenameError(
                                        m,
                                        "method '%s': unable to decompose '%s': unknown type"
                                        % (m.name, retval.str_typename))
                            else:
                                emit.Line("%s = reader.%s();" %
                                          (retval.name, retval.RpcTypeNoCV()))

                    for p in params:
                        if p.is_nonconstref and p.is_interface:
                            emit.Line(
                                "%s = reinterpret_cast<%s>(Interface(reader.Number<void*>(), %s::ID));"
                                % (p.name, p.str_nocvref, p.str_typename))
                        elif not p.obj and p.is_outputptr:
                            if p.length_var and p.length_ref and p.length_ref.is_output:
                                emit.Line(
                                    "%s = reader.%s();" %
                                    (p.length_ref.name, p.length_ref.RpcType()))
                            emit.Line("if ((%s != %s) && (%s != 0)) {" %
                                      (p.name, NULLPTR, p.length_expr))
                            emit.IndentInc()
                            emit.Line("reader.%s(%s, %s);" %
                                      (p.RpcType(), p.length_expr, p.name))
                            emit.IndentDec()
                            emit.Line("}")
                        elif p.is_nonconstref and not p.is_length:
                            emit.Line("%s = reader.%s();" %
                                      (p.name, p.RpcTypeNoCV()))

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
                        emit.Line(
                            'fprintf(stderr, "*** [%s proxy] EXIT: %s()\\n");' %
                            (iface.obj.full_name, m.name))

                    if retval.has_output:
                        emit.Line()
                        emit.Line(
                            "return %s%s;" %
                            (retval.name, "_proxy" if retval_has_proxy else ""))

                else:
                    if m.vars:
                        emit.Line(
                            "RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());"
                        )
                        emit.Line("// TODO")
                        emit.Line()

                    if retval.has_output:
                        emit.Line(
                            "RPC::Data::Frame::Reader reader(newMessage->Response().Reader());"
                        )
                        emit.Line("// TODO")
                        emit.Line()

                        emit.Line("//Complete(newMessage->Response());")
                        emit.Line()

                    if retval.has_output:
                        emit.Line("// return ...")

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
        emit.Line(
            "// -----------------------------------------------------------------"
        )
        emit.Line("// REGISTRATION")
        emit.Line(
            "// -----------------------------------------------------------------"
        )
        emit.Line()
        emit.Line("namespace {")
        emit.IndentInc()

        if announce_list:
            emit.Line()

        for key, val in announce_list.items():
            emit.Line("typedef ProxyStub::UnknownStubType<%s, %s> %s;" %
                      (key, val[1], val[2]))

        emit.Line()

        emit.Line("static class Instantiation {")
        emit.Line("public:")
        emit.IndentInc()
        emit.Line("Instantiation()")
        emit.Line("{")
        emit.IndentInc()

        for key, val in announce_list.items():
            emit.Line("RPC::Administrator::Instance().Announce<%s, %s, %s>();" %
                      (key, val[0], val[2]))

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
        emit.Line("}")  # // namespace %s" % STUB_NAMESPACE.split("::")[-1])

    return interfaces


# -------------------------------------------------------------------------
# entry point

if __name__ == "__main__":
    argparser = argparse.ArgumentParser(
        description='Generate proxy stub code out of interface header files.',
        formatter_class=argparse.RawTextHelpFormatter)
    argparser.add_argument(
        'path',
        nargs="*",
        help="Interface file(s) or a directory(ies) with interface files")
    argparser.add_argument(
        "--help-tags",
        dest="help_tags",
        action="store_true",
        default=False,
        help="show help on supported source code tags and exit")
    argparser.add_argument("--version",
                           dest="show_version",
                           action="store_true",
                           default=False,
                           help="display version")
    argparser.add_argument(
        "-i",
        dest="extra_include",
        metavar="FILE",
        action="store",
        default=DEFAULT_DEFINITIONS_FILE,
        help="include a C++ header file (default: include '%s')" %
        DEFAULT_DEFINITIONS_FILE)
    argparser.add_argument(
        "--namespace",
        dest="if_namespace",
        metavar="NS",
        type=str,
        action="store",
        default=INTERFACE_NAMESPACE,
        help="set namespace to look for interfaces in (default: %s)" %
        INTERFACE_NAMESPACE)
    argparser.add_argument("--indent",
                           dest="indent_size",
                           metavar="SIZE",
                           type=int,
                           action="store",
                           default=INDENT_SIZE,
                           help="set code indentation in spaces (default: %i)" %
                           INDENT_SIZE)
    argparser.add_argument(
        "--traces",
        dest="traces",
        action="store_true",
        default=False,
        help=
        "emit traces in generated proxy/stub code (default: no extra traces)")
    argparser.add_argument(
        "--old-cpp",
        dest="old_cpp",
        action="store_true",
        default=False,
        help="do not emit some C++11 keywords (default: emit modern C++ code)")
    argparser.add_argument(
        "--no-warnings",
        dest="no_warnings",
        action="store_true",
        default=False,
        help="suppress all warnings (default: show warnings)")
    argparser.add_argument(
        "--keep",
        dest="keep_incomplete",
        action="store_true",
        default=False,
        help="keep incomplete files (default: remove partially generated files)"
    )
    argparser.add_argument(
        "--verbose",
        dest="verbose",
        action="store_true",
        default=False,
        help="enable verbose output (default: verbose output disabled)")
    args = argparser.parse_args(sys.argv[1:])
    DEFAULT_DEFINITIONS_FILE = args.extra_include
    INDENT_SIZE = args.indent_size if (
        args.indent_size > 0 and args.indent_size < 32) else INDENT_SIZE
    USE_OLD_CPP = args.old_cpp
    SHOW_WARNINGS = not args.no_warnings
    BE_VERBOSE = args.verbose
    INTERFACE_NAMESPACE = args.if_namespace
    EMIT_TRACES = args.traces
    scan_only = False
    keep_incomplete = args.keep_incomplete

    if INTERFACE_NAMESPACE[0:2] != "::":
        INTERFACE_NAMESPACE = "::" + INTERFACE_NAMESPACE

    if args.help_tags:
        print("The following special tags are supported:")
        print("   @stubgen:skip     - skip parsing of the rest of the file")
        print(
            "   @stubgen:omit     - omit generating code for the next item (class or method)"
        )
        print(
            "   @stubgen:stub     - generate empty stub for the next item (class or method)"
        )
        print("For non-const pointer and reference method/function parameters:")
        print("   @in               - denotes an input parameter")
        print("   @out              - denotes an output parameter")
        print(
            "   @inout            - denotes an input/output parameter (equivalent of @in @out)"
        )
        print(
            "   @length:<expr>    - specifies a buffer length value (a constant, a parameter name or a math expression)"
        )
        print(
            "   @maxlength:<expr> - specifies a maximum buffer length value (a constant, a parameter name or a math expression),"
        )
        print(
            "                       if not specified @length is used as maximum length, use round parenthesis for expressions,"
        )
        print(
            "                       e.g.: @length:bufferSize @length:(width*height*4)"
        )
        print("")
        print("The tags shall be placed inside comments.")
        sys.exit()

    if args.show_version:
        print("Version: " + VERSION)
        sys.exit()

    if not args.path:
        argparser.print_help()
    else:
        interface_files = []
        for elem in args.path:
            if os.path.isdir(elem):
                interface_files += [
                    os.path.join(elem, f) for f in os.listdir(elem)
                    if (os.path.isfile(os.path.join(elem, f)) and re.match(
                        "I.*\\.h", f) and f != IDS_DEFINITIONS_FILE)
                ]
            elif os.path.isfile(elem):
                interface_files += [elem]
            else:
                log.Error("invalid file or directory", elem)

        faces = []
        skipped = []
        if interface_files:
            for source_file in interface_files:
                try:
                    output_file = os.path.join(
                        os.path.dirname(source_file),
                        PROXYSTUB_CPP_NAME % CreateName(
                            os.path.basename(source_file)).split(".", 1)[0])

                    output = GenerateStubs(
                        output_file, source_file,
                        os.path.join(
                            os.path.dirname(os.path.realpath(__file__)),
                            DEFAULT_DEFINITIONS_FILE), scan_only)
                    faces += output

                    log.Print("created file '%s'" % output_file)

                    # dump interfaces if only scanning
                    for f in sorted(output, key=lambda x: x.id):
                        if scan_only:
                            print(f.id, f.obj.full_name)

                except SkipFileError as err:
                    log.Print("skipped file '%s'" % err)
                    skipped.append(source_file)
                except NoInterfaceError as err:
                    log.Info(
                        "no interface classes found in %s" %
                        (INTERFACE_NAMESPACE), source_file)
                except TypenameError as err:
                    log.Error(err)
                    if not keep_incomplete and os.path.isfile(output_file):
                        os.remove(output_file)
                except CppParser.ParserError as err:
                    log.Error(err)

            if scan_only:
                print("\nInterface dump:")

            sorted_faces = sorted(faces, key=lambda x: x.id)
            for i, f in enumerate(sorted_faces):
                if isinstance(f.id, int):
                    if scan_only:
                        if i and sorted_faces[i - 1].id < f.id - 1:
                            print("...")
                        print("%s (%s) - '%s'" %
                              (hex(f.id) if isinstance(f.id, int) else "?",
                               str(f.id), f.obj.full_name))
                    if i and sorted_faces[i - 1].id == f.id:
                        log.Warn(
                            "duplicate interface ID %s (%s) of %s" %
                            (hex(f.id) if isinstance(f.id, int) else "?",
                             str(f.id), f.obj.full_name), f.file)
                else:
                    log.Info(
                        "can't evaluate interface ID \"%s\" of %s" %
                        (str(f.id), f.obj.full_name), f.file)

            if len(interface_files) > 1 and BE_VERBOSE:
                print("")

            log.Print(
                ("all done; %i file%s processed" %
                 (len(interface_files) - len(skipped),
                  "s" if len(interface_files) - len(skipped) > 1 else "")) +
                ((" (%i file%s skipped)" %
                  (len(skipped),
                   "s" if len(skipped) > 1 else "")) if skipped else "") +
                ("; %i interface%s parsed:" %
                 (len(faces), "s" if len(faces) > 1 else "")) +
                ((" %i error%s" %
                  (len(log.errors), "s" if len(log.errors) > 1 else "")
                  ) if log.errors else " no errors") +
                ((" (%i warning%s)" %
                  (len(log.warnings), "s" if len(log.warnings) > 1 else "")
                  ) if log.warnings else ""))
        else:
            log.Print("Nothing to do")

        sys.exit(1 if len(log.errors) else 0)
