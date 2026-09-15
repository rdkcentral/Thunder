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

import copy
from collections import OrderedDict

import config
import emitter
import rpc_version
from json_loader import *
from class_emitter import Restrictions
from class_emitter import IsObjectOptional

class RPCEmitterError(RuntimeError):
    pass

class DottedDict(dict):
    __getattr__ = dict.get
    __setattr__ = dict.__setitem__
    __delattr__ = dict.__delitem__

def EmitEvent(emit, root, event, params_type, legacy = False):
    names = DottedDict()
    names['module'] = "_module"
    names['filter'] = "_id"
    names['params'] = "_params"
    names['designator'] = "_designator"
    names['sendif'] = "_sendIfMethod"
    names['index'] = "_designatorId"
    names['jsonrpc_alias'] = "PluginHost::JSONRPC"

    prefix = ("%s." % names.module) if not legacy else ""

    params = event.params

    # Build parameter list for the prototype
    parameters = [ ]

    if event.sendif_type:
        parameters.append("const %s& %s" % (event.sendif_type.cpp_native_type, names.filter))

    if not params.is_void:
        if params_type == "native":
            if params.properties and params.do_create:
                for p in params.properties:
                    parameters.append("const %s& %s" % (p.cpp_native_type_opt, p.local_name))
            else:
                parameters.append("const %s& %s" % (params.cpp_native_type_opt, params.local_name))

        elif params_type == "json":
            if params.properties and params.do_create:
                for p in params.properties:
                    if p.properties:
                        return

                    parameters.append("const %s& %s" % (p.cpp_type, p.local_name))
            else:
                parameters.append("const %s& %s" % (params.cpp_type, params.local_name))

        elif params_type == "object":
            parameters.append("const %s& %s" % (params.cpp_type, params.local_name))

    if not legacy:
        parameters.insert(0, "const %s& %s" % (names.jsonrpc_alias, names.module))

        if event.sendif_type or event.is_status_listener:
            parameters.append("const std::function<bool(const string&)>& %s = nullptr" % names.sendif)

    # Emit the prototype
    if legacy:
        line = "void %s::%s(%s)" % (root.cpp_name, event.endpoint_name, ", ".join(parameters))
    else:
        line = "static void %s(%s)" % (event.function_name, ", ".join(parameters))

    if event.included_from:
        line += " /* %s */" % event.included_from

    emit.Line("// Event: %s" % (event.Headline()))
    emit.Line(line)
    emit.Line("{")
    emit.Indent()

    # Convert the parameters to JSON types
    if params_type != "object" or legacy:
        if not params.is_void:
            emit.Line("%s %s;" % (params.cpp_type, names.params))

            if params.properties and params.do_create:
                for p in params.properties:
                    if p.optional and (params_type == "native") and not p.default_value:
                        emit.Line("if (%s.IsSet() == true) {" % (p.local_name))
                        emit.Indent()

                    emit.Line("%s.%s = %s;" % (names.params, p.cpp_name, p.local_name))
                    if p.schema.get("opaque"):
                        emit.Line("%s.%s.SetQuoted(false);" % (names.params, p.cpp_name))

                    if p.optional and (params_type == "native") and not p.default_value:
                        emit.Unindent()
                        emit.Line("}")
            else:
                if params.optional and (params_type == "native") and not params.default_value:
                    emit.Line("if (%s.IsSet() == true) {" % (params.local_name))
                    emit.Indent()

                emit.Line("%s = %s;" % (names.params, params.local_name))
                if params.schema.get("opaque"):
                    emit.Line("%s.SetQuoted(false);" % names.params)

                if params.optional and (params_type == "native") and not params.default_value:
                    emit.Unindent()
                    emit.Line("}")

            emit.Line()

        parameters = [ ]

        if not legacy:
            parameters.append(names.module)

        if not params.is_void:
            parameters.append(names.params)

        if event.sendif_type:
            parameters.insert(0 if legacy else 1, names.filter)

        # Emit the local call
        if not legacy:
            emit.Line("%s(%s);" % (event.function_name, ", ".join(parameters + ([names.sendif] if (event.sendif_type or event.is_status_listener) else []))))

    if params_type == "object" or legacy:
        # Build parameters for the notification call
        parameters = [ Tstring(event.json_name) ]

        if not params.is_void:
            parameters.append(names.params if legacy else params.local_name)

        if event.sendif_type:
            if not legacy:
                # If the event has an id specified (i.e. uses "send-if"), generate code for this too:
                # only call if extracted  designator id matches the index.
                emit.Line("if (%s == nullptr) {" % names.sendif)
                emit.Indent()

            def Emit(parameters):
                # Use stock send-if function
                emit.Line('%sNotify(%s, [%s](const string& %s) -> bool {' % (prefix, ", ".join(parameters), names.filter, names.designator))
                emit.Indent()
                emit.Line("const string %s = %s.substr(0, %s.find('.'));" % (names.index, names.designator, names.designator))

                if isinstance(event.sendif_type, JsonInteger):
                    conv_index_name = (names.index + "Converted_")
                    emit.Line("%s %s{};" % (event.sendif_type.cpp_native_type, conv_index_name))
                    emit.Line("return ((Core::FromString(%s, %s) == true) && (%s == %s));" % (names.designator, names.index, names.filter, conv_index_name))

                elif isinstance(event.sendif_type, JsonEnum):
                    conv_index_name = (names.index + "Converted_")
                    emit.Line("Core::EnumerateType<%s> %s(%s.c_str());" % (event.sendif_type.cpp_native_type, conv_index_name, names.index))
                    emit.Line("return (_value.IsSet() == true) && (%s == %s));" % (names.filter, conv_index_name))

                else:
                    emit.Line("return (%s == %s);" % (names.filter, names.index))

                emit.Unindent()
                emit.Line("});")

            Emit(parameters)

            if event.alternative and config.LEGACY_ALT:
                Emit([Tstring(event.alternative)] + parameters[1:])

            if not legacy:
                emit.Unindent()
                emit.Line("} else {")

                def Emit(parameters):
                    emit.Line('%sNotify(%s);' % (prefix, ", ".join(parameters + [names.sendif])))

                # Use supplied custom send-if function
                emit.Indent()
                Emit(parameters)

                if event.alternative and config.LEGACY_ALT:
                    Emit([Tstring(event.alternative)] + parameters[1:])

                emit.Unindent()
                emit.Line("}")
        else:
            # No send-if
            def Emit(parameters):
                emit.Line('%sNotify(%s);' % (prefix, ", ".join(parameters + ([names.sendif] if event.is_status_listener else []))))

            Emit(parameters)

            if config.LEGACY_ALT and event.alternative:
                Emit([Tstring(event.alternative)] + parameters[1:])

    emit.Unindent()
    emit.Line("}")
    emit.Line()

def _EmitRpcPrologue(root, emit, header_file, source_file, ns, data_emitted, prototypes = []):
    is_json_source = source_file.endswith(".json")

    emit.Line()
    emit.Line("// Generated automatically from '%s'. DO NOT EDIT." % os.path.basename(source_file))
    emit.Line()
    emit.Line("#pragma once")

    if is_json_source and prototypes:
        emit.Line()
        emit.Line("#if _IMPLEMENTATION_STUB")
        emit.Line("// sample implementation class")
        emit.Line("class JSONRPCImplementation {")
        emit.Line("public:")
        emit.Indent()

        for p in prototypes:
            emit.Line("%s { %s}" % (p[0], "return (%s); " % p[1] if p[1] else ""))

        emit.Unindent()
        emit.Line("}; // class JSONRPCImplementation")
        emit.Line("#endif // _IMPLEMENTATION_STUB")

    emit.Line()

    if not config.NO_INCLUDES:
        emit.Line("#include \"Module.h\"")

        if data_emitted:
            emit.Line("#include \"%s_%s.h\"" % (config.DATA_NAMESPACE, header_file))

        if not is_json_source:
            emit.Line("#include <%s%s>" % (config.CPP_INTERFACE_PATH, source_file))

    emit.Line()

    for i, ns_ in enumerate(ns.split("::")):
        if ns_:
            emit.Line("namespace %s {" % ns_)
            if i >= 2:
                emit.Indent()
            emit.Line()

    namespace = root.json_name

    if "info" in root.schema and "namespace" in root.schema["info"]:
        namespace = root.schema["info"]["namespace"] + "::" + namespace
        emit.Line("namespace %s {" % root.schema["info"]["namespace"])
        emit.Indent()
        emit.Line()

    namespace = config.DATA_NAMESPACE + "::" + namespace
    emit.Line("namespace %s {" % ("J" + root.json_name))
    emit.Indent()
    emit.Line()

def _EmitRpcEpilogue(root, emit, ns):
    emit.Unindent()
    emit.Line("} // namespace %s" % ("J" + root.json_name))
    emit.Line()

    if "info" in root.schema and "namespace" in root.schema["info"]:
        emit.Unindent()
        emit.Line("} // namespace %s" % root.schema["info"]["namespace"])
        emit.Line()

    for i, ns_ in reversed(list(enumerate(ns.split("::")))):
        if ns_:
            if i >= 2:
                emit.Unindent()
            emit.Line("} // namespace %s" % ns_)
            emit.Line()

    emit.Line()

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
    emit.Line()

def _EmitRpcCode(root, emit, ns, header_file, source_file, data_emitted):

    def _EmitHandlerInterface(listener_events):
        assert listener_events

        emit.Line("struct IHandler {")
        emit.Indent()
        emit.Line("virtual ~IHandler() = default;")

        for m in listener_events:
            emit.Line("virtual void On%sEventRegistration(const string& client, const %s::Status status) = 0;" % (m.cpp_name, names.jsonrpc_alias))

        emit.Unindent()
        emit.Line("};")
        emit.Line()

    def _EmitNoPushWarnings(prologue = True):
        if prologue:
            if not config.NO_PUSH_WARNING:
                emit.Line("PUSH_WARNING(DISABLE_WARNING_UNUSED_FUNCTIONS)")
                emit.Line()
            else:
                emit.Line("#if defined(__GNUC__) || defined(__clang__)")
                emit.Line('#pragma GCC diagnostic ignored "-Wunused-function"')
                emit.Line("#endif")
                emit.Line()
        else:
            if not config.NO_PUSH_WARNING:
                emit.Line("POP_WARNING()")
                emit.Line()

    def _EmitEvents(events):
        assert events

        emit.Line("namespace Event {")
        emit.Indent()
        emit.Line()

        for event in events:
            EmitEvent(emit, root, event, "object")

            if not event.params.is_void:
                if isinstance(event.params, JsonObject):
                    EmitEvent(emit, root, event, "json")

                if not isinstance(event.params, JsonArray):
                    EmitEvent(emit, root, event, "native")

        emit.Unindent()
        emit.Line("} // namespace Event")
        emit.Line()

    def _EmitAlternativeEventsRegistration(alt_events, prologue=True):
        assert alt_events

        if prologue:
            emit.Line("// Register alternative notification names...")

            for event in alt_events:
                emit.Line("%s.RegisterEventAlias(%s, %s);" % (names.module, Tstring(event.alternative), Tstring(event.name)))

            emit.Line()
        else:
            emit.Line()
            emit.Line("// Unegister alternative notification names...")

            for event in alt_events:
                emit.Line("%s.UnregisterEventAlias(%s, %s);" % (names.module, Tstring(event.alternative), Tstring(event.name)))

    def _EmitEventStatusListenerRegistration(listener_events, legacy, prologue=True):
        if prologue:
            emit.Line("// Register event status listeners...")
            emit.Line()

            for event in listener_events:
                emit.Line("%s.RegisterEventStatusListener(_T(\"%s\")," % (names.module, event.json_name))
                emit.Indent()
                emit.Line("[%s%s](const string& client, const %s::Status status) {" % ("&" if legacy else "", names.handler, names.jsonrpc_alias))
                emit.Indent()
                emit.Line("%s%sOn%sEventRegistration(client, status);" % (names.handler, '.' if legacy else '->', event.function_name))
                emit.Unindent()
                emit.Line("});")
                emit.Unindent()
                emit.Line()

                prototypes.append(["void On%sEventRegistration(const string& client, const %s::Status status)" % (event.function_name, names.jsonrpc_alias), None])
        else:
            emit.Line()
            emit.Line("// Unregister event status listeners...")

            for event in listener_events:
                emit.Line("%s.UnregisterEventStatusListener(%s);" % (names.module, Tstring(event.json_name)))

    is_json_source = source_file.endswith(".json")

    for i, ns_ in enumerate(ns.split("::")):
        if ns_ and i >= 2:
            emit.Indent()

    if "info" in root.schema and "namespace" in root.schema["info"]:
        emit.Indent()

    emit.Indent()

    _EmitVersionCode(emit, rpc_version.GetVersion(root.schema["info"] if "info" in root.schema else dict()))

    methods_and_properties = [x for x in root.properties if not isinstance(x, (JsonNotification))]
    methods = [x for x in methods_and_properties if not isinstance(x, (JsonNotification, JsonProperty))]
    events = [x for x in root.properties if isinstance(x, JsonNotification)]
    listener_events = [x for x in events if x.is_status_listener]
    alt_events = [x for x in events if x.alternative]

    names = DottedDict()
    names['module'] = "_module"
    names['impl'] = "_implementation__"
    names['handler'] = ("_handler_" if not is_json_source else names.impl)
    names['handler_interface'] = "IHandler"
    names['context'] = "_context_"
    names['namespace'] = ("J" + root.json_name)
    names['interface'] = (root.info["interface"] if "interface" in root.info else ("I" + root.json_name))
    names['jsonrpc_alias'] = ("PluginHost::%s" % ("JSONRPCSupportsEventStatus" if listener_events else "JSONRPC"))
    names['context_alias'] = "Core::JSONRPC::Context"

    impl_required = methods_and_properties or listener_events
    module_required = (impl_required or (alt_events and not config.LEGACY_ALT))

    if listener_events and not is_json_source:
        _EmitHandlerInterface(listener_events)

    _EmitNoPushWarnings(prologue=True)

    if is_json_source:
        emit.Line("using JSONRPC = %s;" % names.jsonrpc_alias)
        emit.Line()

    impl_name = ((" " + names.impl) if impl_required else "")

    register_params = [ "%s& %s" % (names.jsonrpc_alias, names.module) ]

    if is_json_source:
        register_params.append("IMPLEMENTATION&%s" % impl_name)
        emit.Line("template<typename IMPLEMENTATION>")

    else:
        register_params.append("%s*%s" % (names.interface, impl_name))

        if listener_events:
            register_params.append("%s* %s" % (names.handler_interface, names.handler))

    emit.Line("static void Register(%s)" % (", ".join(register_params)))

    emit.Line("{")
    emit.Indent()

    if not is_json_source:
        if methods_and_properties:
            emit.Line("ASSERT(%s != nullptr);" % names.impl)

        if listener_events:
            emit.Line("ASSERT(%s != nullptr);" % names.handler)

        if methods_and_properties or listener_events:
            emit.Line()


    emit.Line("%s.RegisterVersion(%s, Version::Major, Version::Minor, Version::Patch);" % (names.module, Tstring(names.namespace)))

    if module_required:
        emit.Line()

    if alt_events and not config.LEGACY_ALT:
        _EmitAlternativeEventsRegistration(alt_events, prologue=True)

    if methods_and_properties:
        emit.Line("// Register methods and properties...")
        emit.Line()

    prototypes = []

    for m in methods_and_properties:

        def _Invoke(params, response, parent="", repsonse_parent="", const_cast=False):
            vars = OrderedDict()

            # Build param/response dictionaries (dictionaries will ensure they do not repeat)
            if params:
                if isinstance(params, JsonObject) and params.do_create:
                    for param in params.properties:
                        vars[param.local_name] = [param, "r"]
                else:
                    vars[params.local_name] = [params, "r"]

            if response:
                if isinstance(response, JsonObject) and response.do_create:
                    for resp in response.properties:
                        if resp.local_name not in vars:
                            vars[resp.local_name] = [resp, "w"]
                        else:
                            vars[resp.local_name][1] += "w"
                else:
                    if response.local_name not in vars:
                        vars[response.local_name] = [response, "w"]
                    else:
                        vars[response.local_name][1] += "w"

            sorted_vars = sorted(vars.items(), key=lambda x: x[1][0].schema["@position"])

            for _, [arg, _] in sorted_vars:
                arg.flags = DottedDict()
                arg.flags.prefix = ""

            # Tie buffer with length variables
            for _, [arg, _] in sorted_vars:
                length_var_name = arg.schema.get("length")

                if isinstance(arg, JsonString) and length_var_name:
                    for name, [var, type] in sorted_vars:
                        if name == length_var_name:
                            if type == "w":
                                raise RPCEmitterError("'%s': parameter pointed to by @length is output only" % arg.name)
                            else:
                                var.flags.is_buffer_length = True
                                arg.flags.length = var
                                break

            restrictions = Restrictions(test_set=True)
            buffer_restrictions_used = False

            if params:
                restrictions.append(params, override=params.local_name)

                if restrictions.present():
                    emit.Line()
                    emit.Line("if (%s) {" % restrictions.join())
                    emit.Indent()
                    emit.Line("%s = %s;" % (error_code.temp_name, CoreError("bad_request")))
                    emit.Unindent()
                    emit.Line("} else {")
                    emit.Indent()

            # Emit temporary variables and deserializing of JSON data

            for _, [arg, arg_type] in sorted_vars:
                if arg.flags.is_buffer_length:
                    continue

                is_readable = ("r" in arg_type)
                is_writeable = ("w" in arg_type)
                cv_qualifier = ("const " if not is_writeable else "") # don't consider volatile

                # Take care of POD aggregation
                cpp_name = ((parent + arg.cpp_name) if parent else arg.local_name)

                if arg.schema.get("@bypointer"):
                    arg.flags.prefix = "&"

                # Special case for C-style buffers
                if isinstance(arg, JsonString) and arg.flags.length:
                    assert not arg.optional

                    length = arg.flags.length

                    buffer_restrictions = Restrictions(test_set=True, reverse=True, json=False)

                    for name, [var, var_type] in sorted_vars:
                        if name == length.local_name:
                            initializer = (parent + var.cpp_name) if "r" in var_type else ""
                            emit.Line("%s %s{%s};" % (var.cpp_native_type, var.temp_name, initializer))
                            break

                    encode = arg.schema.get("encode")

                    if not is_writeable and not encode:
                        initializer = "%s.Value().data()" % cpp_name
                        emit.Line("const %s* %s{%s};" % (arg.original_type, arg.temp_name, initializer))
                    else:
                        emit.Line("%s* %s{nullptr};" % (arg.original_type, arg.temp_name))
                        emit.Line()

                        emit.Line("if (%s != 0) {" % length.temp_name)
                        emit.Indent()

                        if not buffer_restrictions.present():
                            buffer_restrictions.append(arg, relay=length, test_set=False)

                        if not buffer_restrictions.present() and (length.size > 16):
                            buffer_restrictions.extend("(%s <= 0x400000) /* sanity! */" % length.temp_name)

                        if buffer_restrictions.present():
                            buffer_restrictions_used = True
                            emit.Line("if (%s) {" % buffer_restrictions.join())
                            emit.Indent()

                        emit.Line("%s = reinterpret_cast<%s*>(ALLOCA(%s));" % (arg.temp_name, arg.original_type, length.temp_name))
                        emit.Line("ASSERT(%s != nullptr);" % arg.temp_name)

                    if is_readable:
                        if encode:
                            emit.Line("Core::FromString(%s, %s, %s, nullptr);" % (cpp_name, arg.temp_name, length.temp_name))
                        elif is_writeable:
                            emit.Line("::memcpy(%s, %s.Value().data(), %s);" % (arg.temp_name, cpp_name, length.temp_name))

                    if buffer_restrictions.present():
                        emit.Unindent()
                        emit.Line("}")
                        emit.Line("else {")
                        emit.Indent()
                        emit.Line("%s = %s;" % (error_code.temp_name, CoreError("bad_request")))
                        emit.Unindent()
                        emit.Line("}")

                    if is_writeable or encode:
                        emit.Unindent()
                        emit.Line("}")

                    emit.Line()

                # Special case for iterators
                elif isinstance(arg, JsonArray):

                    if arg.iterator:
                        if arg.optional:
                            raise RPCEmitterError("OptionalType iterators are not supported, use @optional tag (see %s)" % arg.cpp_native_type_opt)

                        face_name = "_" + arg.items.local_name.capitalize() + "IteratorType"
                        emit.Line("using %s = %s;" % (face_name, arg.iterator))

                        if not is_writeable:
                            arg.flags.release = True

                            elements_name = arg.items.TempName("elements")
                            iterator_name = arg.items.TempName("iterator")
                            impl_name = "_" + arg.items.local_name.capitalize() + "IteratorImplType"

                            emit.Line("std::list<%s> %s;" % (arg.items.cpp_native_type, elements_name))
                            emit.Line("auto %s = %s.Elements();" % (iterator_name, cpp_name))
                            emit.Line("while (%s.Next() == true) { %s.push_back(%s.Current()); }" % (iterator_name, elements_name, iterator_name))
                            impl = (arg.iterator[:arg.iterator.index('<')].replace("IIterator", "Iterator") + ("<%s>" % face_name))
                            emit.Line("using %s = %s;" % (impl_name, impl))
                            initializer = "Core::ServiceType<%s>::Create<%s>(std::move(%s))" % (impl_name, face_name, elements_name)
                            emit.Line("%s* const %s{%s};" % (face_name, arg.temp_name, initializer))
                            emit.Line("ASSERT(%s != nullptr); " % arg.temp_name)

                            if arg.schema.get("@byreference"):
                                arg.flags.cast = "static_cast<%s* const&>(%s)" % (face_name, arg.temp_name)

                        elif not is_readable:
                            emit.Line("%s%s* %s{};" % ("const " if arg.schema.get("@ptrtoconst") else "", face_name, arg.temp_name))
                        else:
                            raise RPCEmitterError("Read/write arrays (iterators) are not supported (see %s)" % arg.arg.cpp_native_type_opt)

                    elif arg.items.schema.get("@bitmask"):
                        initializer = cpp_name if is_readable else ""
                        emit.Line("%s%s %s{%s};" % (cv_qualifier, arg.items.cpp_native_type, arg.temp_name, initializer))

                    elif is_json_source:
                        response_cpp_name = (response_parent + arg.cpp_name) if response_parent else arg.local_name
                        initializer = ("(%s)" if isinstance(arg, JsonObject) else "{%s}") % (response_cpp_name if is_writeable else cpp_name)

                        if is_readable and is_writeable:
                            emit.Line("%s = %s;" % (response_cpp_name, cpp_name))

                        emit.Line("%s%s %s%s;" % (cv_qualifier, (arg.cpp_type + "&") if is_json_source else arg.cpp_native_type, arg.temp_name, initializer))

                    else:
                        raise RPCEmitterError("Arrays must be iterators: %s" % arg.json_name)

                # All Other
                else:
                    if is_json_source:
                        response_cpp_name = (response_parent + arg.cpp_name) if response_parent else arg.local_name
                        initializer = ("(%s)" if isinstance(arg, JsonObject) else "{%s}") % (response_cpp_name if is_writeable else cpp_name)

                        if is_readable and is_writeable:
                            emit.Line("%s = %s;" % (response_cpp_name, cpp_name))

                    else:
                        initializer = (("(%s)" if isinstance(arg, JsonObject) else "{%s}") % cpp_name) if is_readable and not arg.convert else "{}"

                    if arg.optional and is_readable and (arg.default_value == None or not parent):
                        emit.Line("%s %s{};" % (arg.cpp_native_type_opt, arg.temp_name))
                        emit.Line("if (%s.IsSet() == true) {" % (cpp_name))
                        emit.Indent()
                        emit.Line("%s = %s;" % (arg.temp_name, cpp_name))
                        emit.Unindent()
                        if arg.default_value:
                            emit.Line("} else {")
                            emit.Indent()
                            emit.Line("%s = %s;" % (arg.temp_name, arg.default_value))
                            emit.Unindent()
                        emit.Line("}")
                        emit.Line()
                    else:
                        emit.Line("%s%s %s%s;" % (cv_qualifier, (arg.cpp_type + "&") if is_json_source else arg.cpp_native_type_opt, arg.temp_name, initializer))

                    if arg.convert and is_readable:
                        emit.Line((arg.convert + ";") % (arg.temp_name, cpp_name))

            if buffer_restrictions_used:
                emit.Line()
                emit.Line("if (%s == %s) {" % (error_code.temp_name, CoreError("none")))
                emit.Indent()

            # Emit call to the implementation
            if not is_json_source: # Full automatic mode

                impl = names.impl
                interface = names.interface

                if lookup:
                    impl = "_lookup" + impl
                    interface = lookup[0]
                    emit.Line("%s%s* const %s = %s->%s(%s);" % ("const " if const_cast else "", lookup[0], impl, names.impl,lookup[1], lookup[2]))
                    emit.Line()
                    emit.Line("if (%s != nullptr) {" % impl)
                    emit.Indent()

                iterator_conditions = []

                for _, [arg, _] in sorted_vars:
                    if arg.flags.release:
                        iterator_conditions.append("(%s != nullptr)" % arg.temp_name)

                        if len(iterator_conditions) == 1:
                            emit.Line()

                if iterator_conditions:
                    emit.Line()
                    emit.Line("if (%s) {" % " && ".join(iterator_conditions))
                    emit.Indent()

                implementation_object = "(static_cast<const %s*>(%s))" % (interface, impl) if const_cast and not lookup else impl
                function_params = []

                if contexted:
                    function_params.append(names.context)

                if indexed:
                    function_params.append(index_name)

                for _, [arg, _] in sorted_vars:
                    function_params.append("%s%s" % (arg.flags.prefix, (arg.flags.cast if arg.flags.cast else arg.temp_name)))

                emit.Line("%s = %s->%s(%s);" % (error_code.temp_name, implementation_object, m.cpp_name, ", ".join(function_params)))

                if iterator_conditions:
                    for _, _record in sorted_vars:
                        arg = _record[0]
                        if arg.flags.release:
                            emit.Line("%s->Release();" % arg.temp_name)

                    emit.Unindent()
                    emit.Line("} else {")
                    emit.Indent()
                    emit.Line("%s = %s;" % (error_code.temp_name, CoreError("general")))
                    emit.Unindent()
                    emit.Line("}")

                if lookup:
                    emit.Line("%s->Release();" % impl)
                    emit.Unindent()
                    emit.Line("}")
                    emit.Line("else {")
                    emit.Indent()
                    emit.Line("%s = %s;" % (error_code.temp_name, CoreError("bad_request")))
                    emit.Unindent()
                    emit.Line("}")

            # Semi-automatic mode
            else:
                parameters = []

                if indexed:
                    parameters.append(index_name)

                for _, [ arg, _ ] in sorted_vars:
                    parameters.append("%s" % (arg.temp_name))

                if const_cast:
                    emit.Line("%s = (static_cast<const IMPLEMENTATION&>(%s)).%s(%s);" % (error_code.temp_name, names.impl, m.function_name, ", ".join(parameters)))
                else:
                    emit.Line("%s = %s.%s(%s);" % (error_code.temp_name, names.impl, m.function_name, ", ".join(parameters)))

                parameters = []

                if indexed:
                    parameters.append("const %s& %s" % (m.index.cpp_native_type, index_name))

                for _, [ arg, type ] in sorted_vars:
                    parameters.append("%s%s& %s" % ("const " if type == "r" else "", arg.cpp_type, arg.local_name))

                prototypes.append(["uint32_t %s(%s)%s" % (m.function_name, ", ".join(parameters), (" const" if (const_cast or (isinstance(m, JsonProperty) and m.readonly)) else "")), CoreError("none")])

            if buffer_restrictions_used:
                emit.Unindent()
                emit.Line("}")

            emit.Line()

            # Emit result handling and serialization to JSON data

            if response and not is_json_source:
                emit.Line("if (%s == %s) {" % (error_code.temp_name, CoreError("none")))
                emit.Indent()

                for _, [arg, arg_type] in sorted_vars:
                    if "w" not in arg_type:
                        continue

                    cpp_name = (repsonse_parent + arg.cpp_name) if repsonse_parent else arg.local_name

                    # Special case for C-style buffers disguised as base64-encoded strings
                    if isinstance(arg, JsonString) and "length" in arg.flags:
                        length_var = arg.flags.get("length")

                        emit.Line()
                        emit.Line("if (%s != 0) {" % length_var.temp_name)
                        emit.Indent()

                        if arg.schema.get("encode"):
                            encoded_name = arg.TempName("encoded_")
                            emit.Line("%s %s;" % (arg.cpp_native_type, encoded_name))
                            emit.Line("Core::ToString(%s, %s, true, %s);" % (arg.temp_name, length_var.temp_name, encoded_name))
                            emit.Line("%s = %s;" % (cpp_name, encoded_name))
                        else:
                            emit.Line("%s = string(%s, %s);" % (cpp_name, arg.temp_name, length_var.temp_name))

                        emit.Unindent()
                        emit.Line("}")

                    # Special case for iterators disguised as arrays
                    elif isinstance(arg, JsonArray):
                        if arg.iterator:
                            item_name = arg.items.TempName("item_")

                            emit.Line("if (%s != nullptr) {" % arg.temp_name)
                            emit.Indent()

                            emit.Line("%s %s{};" % (arg.items.cpp_native_type, item_name))
                            emit.Line("while (%s->Next(%s) == true) { %s.Add() = %s; }" % (arg.temp_name, item_name, cpp_name, item_name))

                            if arg.schema.get("@extract"):
                                emit.Line("%s.SetExtractOnSingle(true);" % (cpp_name))

                            emit.Line("%s->Release();" % arg.temp_name)
                            emit.Unindent()
                            emit.Line("}")

                        elif arg.items.schema.get("@bitmask"):
                            emit.Line("%s = %s;" % (cpp_name, arg.temp_name))

                        else:
                            raise RPCEmitterError("unable to serialize a non-iterator array: %s" % arg.json_name)

                    # All others...
                    else:
                        emit.Line("%s = %s;" % (cpp_name, arg.temp_name + arg.convert_rhs))

                        if arg.schema.get("opaque"):
                            emit.Line("%s.SetQuoted(false);" % (cpp_name))

                emit.Unindent()
                emit.Line("}")

            if restrictions.present():
                emit.Unindent()
                emit.Line("}")

        is_property = isinstance(m, JsonProperty)

        # Prepare for handling indexed properties
        indexed = is_property and m.index
        contexted = not is_property and m.context
        optional_checked = False
        index_name = m.index.local_name if indexed else None
        index_name_converted = None
        lookup = m.schema.get("@lookup")

        if is_property:
            # Normalize property params/repsonse to match methods
            if m.properties[1].is_void and not m.writeonly:
                # Try to detect the uncompliant format
                params = copy.deepcopy(m.properties[0] if not m.readonly else m.properties[1])
                response = copy.deepcopy(m.properties[0] if not m.writeonly else m.properties[1])
            else:
                params = copy.deepcopy(m.properties[0])
                response = copy.deepcopy(m.properties[1])

            params.Rename("Params")
            response.Rename("Result")
            emit.Line("// %sProperty: %s%s" % ("Indexed " if indexed else "", m.Headline(), " (r/o)" if m.readonly else (" (w/o)" if m.writeonly else "")))
        else:
            params = copy.deepcopy(m.properties[0])
            response = copy.deepcopy(m.properties[1])
            emit.Line("// Method: %s" % m.Headline())

        # Emit method prologue
        template_params = [ params.cpp_type, response.cpp_type ]

        if indexed or contexted or lookup:
            function_params = []

            if contexted:
                function_params.append("const %s&" % names.context_alias)

            if lookup:
                function_params.append("const uint32_t")

            if indexed:
                function_params.append("const string&")

            if not params.is_void:
                function_params.append("const %s&" % params.cpp_type)

            if not response.is_void:
                function_params.append("%s&" % response.cpp_type)

            template_params.append("std::function<uint32_t(%s)>" % (", ".join(function_params)))

        emit.Line("%s.Register<%s>(%s, " % (names.module, (", ".join(template_params)), Tstring(m.json_name)))
        emit.Indent()

        lambda_params = []

        if contexted:
            lambda_params.append("const %s& %s" % (names.context_alias, names.context))

        if lookup:
            lambda_params.append("const uint32_t %s" % (lookup[2]))

        if indexed:
            lambda_params.append("const string& %s" % (index_name))

        if not params.is_void:
            lambda_params.append("const %s& %s" % (params.cpp_type, params.local_name))

        if not response.is_void:
            lambda_params.append("%s& %s" % (response.cpp_type, response.local_name))

        emit.Line("[%s%s](%s) -> uint32_t {" % ("&" if is_json_source else "", names.impl, ", ".join(lambda_params)))
        emit.Indent()

        # Emit the function body

        params_parent = ((params.local_name + '.') if (isinstance(params, JsonObject) and params.do_create) else "")
        response_parent = ((response.local_name + '.') if (isinstance(response, JsonObject) and response.do_create) else "")

        error_code = AuxJsonInteger("errorCode_", 32)
        emit.Line("%s %s = %s;" % (error_code.cpp_native_type, error_code.temp_name, CoreError("none")))
        emit.Line()

        if not is_property:
            _Invoke((params if not params.is_void else None), (response if not response.is_void else None), params_parent, response_parent)
        else:
            is_read_only = m.readonly
            is_write_only = m.writeonly
            is_read_write = not m.readonly and not m.writeonly

            if indexed:
                if isinstance(m.index, JsonInteger):
                    # Automatically convert integer indexes in properties
                    index_name_converted = m.index.TempName("converted_")

                    emit.Line("%s %s{};" % (m.index.cpp_native_type, index_name_converted))
                    emit.Line()

                    emit.Line("if ((%s.empty() == true) || (Core::FromString(%s, %s) == false)) {" % (index_name, index_name, index_name_converted))
                    emit.Indent()

                    emit.Line("%s = %s;" % (error_code.temp_name, CoreError("bad_request")))

                    if is_read_write:
                        emit.Line("%s%s.Null(true);" % ("// " if isinstance(response, (JsonArray, JsonObject)) else "", response.local_name)) # FIXME

                    emit.Unindent()
                    emit.Line("} else {")
                    emit.Indent()

                    index_name = index_name_converted

                elif isinstance(m.index, JsonEnum):
                    # Automatically convert enum indexes in properties
                    index_name_converted = m.index.TempName("converted_")

                    emit.Line("Core::EnumerateType<%s> %s(%s.c_str());" % (m.index.cpp_native_type, index_name_converted, index_name))
                    emit.Line()

                    emit.Line("if (%s.IsSet() == false) {" % index_name_converted)
                    emit.Indent()

                    emit.Line("%s = %s;" % (error_code.temp_name, CoreError("bad_request")))

                    if is_read_write:
                        emit.Line("%s%s.Null(true);" % ("// " if isinstance(response, (JsonArray, JsonObject)) else "", response.local_name)) # FIXME

                    emit.Unindent()
                    emit.Line("} else {")
                    emit.Indent()

                    index_name = index_name_converted

                elif isinstance(m.index, JsonString):
                    if not IsObjectOptional(m.index):
                        # Ensure the not-optional index is not empty
                        assert isinstance(m.index, JsonString)
                        optional_checked = True

                        emit.Line("if (%s.empty() == true) {" % index_name)
                        emit.Indent()

                        emit.Line("%s = %s;" % (error_code.temp_name, CoreError("bad_request")))

                        if is_read_write:
                            emit.Line("%s%s.Null(true);" % ("// " if isinstance(response, (JsonArray, JsonObject)) else "", response.local_name)) # FIXME

                        emit.Unindent()
                        emit.Line("} else {")
                        emit.Indent()
                else:
                    assert False, "Invalid type for index"

            if is_read_write:
                emit.Line("if (%s.IsSet() == false) {" % (params.local_name))
                emit.Indent()
                emit.Line("// property get")

            elif is_read_only:
                emit.Line("// read-only property get")

            if not is_write_only:
                assert not response.is_void
                _Invoke(None, response, params_parent, response_parent, const_cast=is_read_write)

            if not is_read_only:
                if is_read_write:
                    emit.Unindent()
                    emit.Line("} else {")
                    emit.Indent()
                    emit.Line("// property set")
                else:
                    emit.Line("// write-only property set")

                assert not params.is_void
                _Invoke(params, None, params_parent, response_parent)

                if is_read_write:
                    emit.Line()
                    emit.Line("%s%s.Null(true);" % ("// " if isinstance(response, (JsonArray, JsonObject)) else "", response.local_name)) # FIXME
                    emit.Unindent()
                    emit.Line("}")

            if index_name_converted or optional_checked:
                emit.Unindent()
                emit.Line("}")

        emit.Line()

        # Emit method epilogue

        emit.Line("return (%s);" % error_code.temp_name)
        emit.Unindent()
        emit.Line("});")
        emit.Unindent()
        emit.Line()

        if not config.LEGACY_ALT and m.alternative:
            emit.Line()
            emit.Line("%s.Register(%s, %s);" % (names.module, Tstring(m.alternative), Tstring(m.name)))
            emit.Line()

    # Emit event status registrations
    if listener_events:
        _EmitEventStatusListenerRegistration(listener_events, is_json_source, prologue=True)

    emit.Unindent()
    emit.Line("}")
    emit.Line()

    # Emit method deregistrations
    module_name = ((" " + names.module) if module_required else "")
    emit.Line("static void Unregister(%s&%s)" % (names.jsonrpc_alias, module_name))
    emit.Line("{")
    emit.Indent()

    if methods_and_properties:
        emit.Line("// Unregister methods and properties...")

        for m in methods_and_properties:
            if isinstance(m, JsonMethod) and not isinstance(m, JsonNotification):
                emit.Line("%s.Unregister(%s);" % (names.module, Tstring(m.json_name)))

                if not config.LEGACY_ALT and m.alternative:
                    emit.Line("%s.Unregister(%s);" % (names.module, Tstring(m.alternative)))

    # Emit alternative events deregistrations
    if alt_events and not config.LEGACY_ALT:
        _EmitAlternativeEventsRegistration(alt_events, prologue=False)

    # Emit event status listeners deregistrations
    if listener_events:
        _EmitEventStatusListenerRegistration(listener_events, is_json_source, prologue=False)

    emit.Unindent()
    emit.Line("}")
    emit.Line()

    # Finally emit utility event code
    if events:
        _EmitEvents(events)

    # Restore warnings level
    _EmitNoPushWarnings(prologue=False)

    # Return collected signatures, so the emited file can be prepended with
    return prototypes

def __Namespace(root):
    return (root.schema["namespace"] if "namespace" in root.schema else "::%s::Exchange" % config.FRAMEWORK_NAMESPACE)

def EmitRpcCode(root, emit, header_file, source_file, data_emitted):
    _namespace = __Namespace(root)
    prototypes = _EmitRpcCode(root, emit, _namespace, header_file, source_file, data_emitted)

    with emitter.Emitter(None, config.INDENT_SIZE) as prototypes_emitter:
        _EmitRpcPrologue(root, prototypes_emitter, header_file, source_file, _namespace, data_emitted, prototypes)
        emit.Prepend(prototypes_emitter)

    _EmitRpcEpilogue(root, emit, _namespace)

def EmitRpcVersionCode(root, emit, header_file, source_file, data_emitted):
    _namespace = __Namespace(root)
    _EmitRpcPrologue(root, emit, header_file, source_file, _namespace, data_emitted)
    _EmitVersionCode(emit, rpc_version.GetVersion(root.schema["info"] if "info" in root.schema else dict()))
    _EmitRpcEpilogue(root, emit, _namespace)
