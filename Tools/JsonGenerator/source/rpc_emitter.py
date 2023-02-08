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


def EmitEvent(emit, root, event, params_type, legacy = False):
    module_var = "_module_"
    filter_var = "_id_"
    params_var = "_params_"
    designator_var = "_designator_"
    sendiffn_var = "_sendIfFunction_"
    index_var = "_designatorId_"

    prefix = ("%s." % module_var) if not legacy else ""

    emit.Line("// Event: %s" % (event.Headline()))

    params = event.params

    # Build parameter list for the prototype
    parameters = [ ]

    if event.sendif_type:
        parameters.append("const %s& %s" % (event.sendif_type.cpp_native_type, filter_var))

    if not params.is_void:
        if params_type == "native":
            if params.properties and params.do_create:
                for p in params.properties:
                    parameters.append("const %s& %s" % (p.cpp_native_type, p.local_name))
            else:
                parameters.append("const %s& %s" % (params.cpp_native_type, params.local_name))

        elif params_type == "json":
            if params.properties and params.do_create:
                for p in params.properties:
                    parameters.append("const %s& %s" % (p.cpp_type, p.local_name))
            else:
                parameters.append("const %s& %s" % (params.cpp_type, params.local_name))

        elif params_type == "object":
            parameters.append("const %s& %s" % (params.cpp_type, params.local_name))

    if not legacy:
        parameters.insert(0, "const JSONRPC& %s" % module_var)

        if event.sendif_type or event.is_status_listener:
            parameters.append("const std::function<bool(const string&)>& %s = nullptr" % sendiffn_var)

    # Emit the prototype
    if legacy:
        line = "void %s::%s(%s)" % (root.cpp_name, event.endpoint_name, ", ".join(parameters))
    else:
        line = "static void %s(%s)" % (event.function_name, ", ".join(parameters))

    if event.included_from:
        line += " /* %s */" % event.included_from

    emit.Line(line)
    emit.Line("{")
    emit.Indent()

    # Convert the parameters to JSON types
    if params_type != "object" or legacy:
        if not params.is_void:
            emit.Line("%s %s;" % (params.cpp_type, params_var))

            if params.properties and params.do_create:
                for p in event.params.properties:
                    emit.Line("%s.%s = %s;" % (params_var, p.cpp_name, p.local_name))
                    if p.schema.get("opaque"):
                        emit.Line("%s.%s.SetQuoted(false);" % (params_var, p.cpp_name))
            else:
                emit.Line("%s = %s;" % (params_var, event.params.local_name))
                if params.schema.get("opaque"):
                    emit.Line("%s.SetQuoted(false);" % params_var)

            emit.Line()

        parameters = [ ]

        if not legacy:
            parameters.append(module_var)

        if not params.is_void:
            parameters.append(params_var)

        if event.sendif_type:
            parameters.insert(0 if legacy else 1, filter_var)

        # Emit the local call
        if not legacy:
            emit.Line("%s(%s);" % (event.function_name, ", ".join(parameters + ([sendiffn_var] if (event.sendif_type or event.is_status_listener) else []))))

    if params_type == "object" or legacy:
        # Build parameters for the notification call
        parameters = [ Tstring(event.json_name) ]

        if not params.is_void:
            parameters.append(params_var if legacy else params.local_name)

        if event.sendif_type:
            if not legacy:
                # If the event has an id specified (i.e. uses "send-if"), generate code for this too:
                # only call if extracted  designator id matches the index.
                emit.Line("if (%s == nullptr) {" % sendiffn_var)
                emit.Indent()

            # Use stock send-if function
            emit.Line('%sNotify(%s, [%s](const string& %s) -> bool {' % (prefix, ", ".join(parameters), filter_var, designator_var))
            emit.Indent()
            emit.Line("const string %s = %s.substr(0, %s.find('.'));" % (index_var, designator_var, designator_var))

            if isinstance(event.sendif_type, JsonInteger):
                conv_index_var = "_designatorIdAsInt"
                emit.Line("%s %s{};" % (sendif_type.cpp_native_type, conv_index_var))
                emit.Line("return ((Core::FromString(%s, %s) == true) && (%s == %s));" % (designator_var, index_var, filter_var, conv_index_var))

            elif isinstance(event.sendif_type, JsonEnum):
                conv_index_var = "_designatorIdAsEnum"
                emit.Line("Core::EnumerateType<%s> %s(%s.c_str());" % (event.sendif_type.cpp_native_type, conv_index_var, index_var))
                emit.Line("return (_value.IsSet() == true) && (%s == %s));" % (filter_var, conv_index_var))

            else:
                emit.Line("return (%s == %s);" % (filter_var, index_var))

            emit.Unindent()
            emit.Line("});")

            if not legacy:
                emit.Unindent()
                emit.Line("} else {")

                # Use supplied custom send-if function
                emit.Indent()
                emit.Line('%sNotify(%s);' % (prefix, ", ".join(parameters + [sendiffn_var])))
                emit.Unindent()
                emit.Line("}")
        else:
            # No send-if
            emit.Line('%sNotify(%s);' % (prefix, ", ".join(parameters + ([sendiffn_var] if event.is_status_listener else []))))

    emit.Unindent()
    emit.Line("}")
    emit.Line()


def _EmitRpcPrologue(root, emit, header_file, source_file, data_emitted, prototypes = []):
    json_source = source_file.endswith(".json")

    emit.Line()
    emit.Line("// Generated automatically from '%s'. DO NOT EDIT." % os.path.basename(source_file))
    emit.Line()
    emit.Line("#pragma once")

    if json_source and prototypes:
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

        if not json_source:
            emit.Line("#include <%s%s>" % (config.CPP_INTERFACE_PATH, source_file))

    emit.Line()
    emit.Line("namespace %s {" % config.FRAMEWORK_NAMESPACE)
    emit.Line()
    emit.Line("namespace %s {" % config.INTERFACE_NAMESPACE.split("::")[-1])
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

def _EmitRpcEpilogue(root, emit):
    emit.Unindent()
    emit.Line("} // namespace %s" % ("J" + root.json_name))
    emit.Line()

    if "info" in root.schema and "namespace" in root.schema["info"]:
        emit.Unindent()
        emit.Line("} // namespace %s" % root.schema["info"]["namespace"])
        emit.Line()

    emit.Unindent()
    emit.Line("} // namespace %s" % config.INTERFACE_NAMESPACE.split("::")[-1])
    emit.Line()
    emit.Line("}")

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

def _EmitRpcCode(root, emit, header_file, source_file, data_emitted):
    json_source = source_file.endswith(".json")

    emit.Indent()
    emit.Indent()

    _EmitVersionCode(emit, rpc_version.GetVersion(root.schema["info"] if "info" in root.schema else dict()))

    module_var = "_module_"
    impl_var = "_impl_"
    struct = "J" + root.json_name
    face = "I" + root.json_name

    has_listeners = any((isinstance(m, JsonNotification) and m.is_status_listener) for m in root.properties)

    emit.Line()
    emit.Line("using JSONRPC = PluginHost::JSONRPC%s;" % ("SupportsEventStatus" if has_listeners else ""))
    emit.Line()

    if json_source:
        emit.Line("template<typename IMPLEMENTATION>")

    emit.Line("static void Register(JSONRPC& %s, %s %s)" % (module_var,  ("IMPLEMENTATION&" if json_source else (face + "*")), impl_var))
    emit.Line("{")
    emit.Indent()

    if not json_source:
        emit.Line("ASSERT(%s != nullptr);" % impl_var)
        emit.Line()

    emit.Line("%s.RegisterVersion(%s, Version::Major, Version::Minor, Version::Patch);" % (module_var, Tstring(struct)))
    emit.Line()

    events = []
    method_count = 0

    emit.Line("// Register methods and properties...")
    emit.Line()

    prototypes = []

    for m in root.properties:
        if not isinstance(m, JsonNotification):

            # Prepare for handling indexed properties
            indexed = isinstance(m, JsonProperty) and m.index
            index_converted = False

            # Normalize property params/repsonse to match methods
            if isinstance(m, JsonProperty):
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

            if indexed:
                function_params = [ "const string&" ]

                if not params.is_void:
                    function_params.append("const %s&" % params.cpp_type)

                if not response.is_void:
                    function_params.append("%s&" % response.cpp_type)

                template_params.append("std::function<uint32_t(%s)>" % (", ".join(function_params)))

            emit.Line("%s.Register<%s>(%s, " % (module_var, (", ".join(template_params)), Tstring(m.json_name)))
            emit.Indent()

            lambda_params = []

            if indexed:
                lambda_params.append("const string& %s" % (m.index.TempName("_")))

            if not params.is_void:
                lambda_params.append("const %s& %s" % (params.cpp_type, params.local_name))

            if not response.is_void:
                lambda_params.append("%s& %s" % (response.cpp_type, response.local_name))

            emit.Line("[%s%s](%s) -> uint32_t {" % ("&" if json_source else "", impl_var, ", ".join(lambda_params)))

            def _Invoke(params, response, use_prefix = True, const_cast = False, parent = "", repsonse_parent = ""):
                vars = OrderedDict()

                # Build param/response dictionaries (dictionaries will ensure they do not repeat)
                if params and not params.is_void:
                    if isinstance(params, JsonObject) and params.do_create:
                        for param in params.properties:
                            vars[param.local_name] = [param, "r"]
                    else:
                        vars[params.local_name] = [params, "r"]

                if response and not response.is_void:
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

                for _, [arg, _] in vars.items():
                    arg.flags = dict()
                    arg.flags["prefix"] = ""

                # Tie buffer with length variables
                for _, [arg, _] in vars.items():
                    length_var_name = arg.schema.get("length")

                    if isinstance(arg, JsonString) and length_var_name:
                        for name, [var, type] in vars.items():
                            if name == length_var_name:
                                if type == "w":
                                    raise RuntimeError("'%s': parameter pointed to by @length is output only" % arg.name)
                                else:
                                    var.flags["isbufferlength"] = True
                                    arg.flags["length"] = var
                                    break

                # Emit temporary variables and deserializing of JSON data

                for _, [arg, arg_type] in sorted(vars.items(), key=lambda x: x[1][0].schema["position"]):
                    if arg.flags.get("isbufferlength"):
                        continue

                    is_readable = "r" in arg_type
                    is_writeable = "w" in arg_type
                    cv_qualifier = "const " if not is_writeable else ""

                    cpp_name = (parent + arg.cpp_name) if parent else arg.local_name

                    if arg.schema.get("ptr"):
                        arg.flags["prefix"] = "&"

                    # Special case for C-style buffers
                    if isinstance(arg, JsonString) and "length" in arg.flags:
                        length = arg.flags.get("length")

                        for name, [var, _] in vars.items():
                            if name == length.local_name:
                                initializer = (parent + var.cpp_name) if is_readable else ""
                                emit.Line("%s %s{%s};" % (var.cpp_native_type, var.TempName(), initializer))
                                break

                        encode = arg.schema.get("encode")

                        if not is_writeable and not encode:
                            initializer = "%s.Value().data()" % cpp_name
                            emit.Line("const %s* %s{%s};" % (arg.original_type, arg.TempName(), initializer))
                        else:
                            emit.Line("%s* %s = nullptr;" % (arg.original_type, arg.TempName()))
                            emit.Line()
                            emit.Line("if (%s != 0) {" % length.TempName())
                            emit.Indent()
                            emit.Line("%s = reinterpret_cast<%s*>(ALLOCA(%s));" % (arg.TempName(), arg.original_type, length.TempName()))
                            emit.Line("ASSERT(%s != nullptr);" % arg.TempName())

                        if is_readable:
                            if encode:
                                emit.Line("Core::FromString(%s, %s, %s, nullptr);" % (cpp_name, arg.TempName(), length_var.TempName()))
                            elif is_writeable:
                                emit.Line("::memcpy(%s, %s.Value().data(), %s);" % (arg.TempName(), cpp_name, length_var.TempName()))

                        if is_writeable or encode:
                            emit.Unindent()
                            emit.Line("}")

                    # Special case for iterators
                    elif isinstance(arg, JsonArray):
                        if arg.iterator:
                            if not is_writeable:
                                emit.Line("std::list<%s> _elements;" % (arg.items.cpp_native_type))
                                emit.Line("auto _Iterator = %s.Elements();" % cpp_name)
                                emit.Line("while (_Iterator.Next() == true) { _elements.push_back(_Iterator.Current()); }")
                                emit.Line()
                                impl = arg.iterator[:arg.iterator.index('<')].replace("IIterator", "Iterator") + "<%s>" % arg.iterator
                                initializer = "Core::Service<%s>::Create<%s>(_elements)" % (impl, arg.iterator)
                                emit.Line("%s* const %s{%s};" % (arg.iterator, arg.TempName(), initializer))
                                arg.flags["release"] = True

                                if arg.schema.get("ref"):
                                    arg.flags["cast"] = "static_cast<%s* const&>(%s)" % (arg.iterator, arg.TempName())

                            elif not is_readable:
                                emit.Line("%s%s* %s{};" % ("const " if arg.schema.get("ptrtoconst") else "", arg.iterator, arg.TempName()))
                            else:
                                raise RuntimeError("Read/write arrays are not supported: %s" % arg.json_name)
                        elif arg.items.schema.get("bitmask"):
                            initializer = cpp_name if is_readable else ""
                            emit.Line("%s%s %s{%s};" % (cv_qualifier, arg.items.cpp_native_type, arg.TempName(), initializer))
                        elif json_source:
                            response_cpp_name = (response_parent + arg.cpp_name) if response_parent else arg.local_name
                            initializer = ("(%s)" if isinstance(arg, JsonObject) else "{%s}") % (response_cpp_name if is_writeable else cpp_name)

                            if is_readable and is_writeable:
                                emit.Line("%s = %s;" % (response_cpp_name, cpp_name))

                            emit.Line("%s%s %s%s;" % (cv_qualifier, (arg.cpp_type + "&") if json_source else arg.cpp_native_type, arg.TempName(), initializer))
                        else:
                            raise RuntimeError("Arrays need to be iterators: %s" % arg.json_name)

                    # All Other
                    else:
                        if json_source:
                            response_cpp_name = (response_parent + arg.cpp_name) if response_parent else arg.local_name
                            initializer = ("(%s)" if isinstance(arg, JsonObject) else "{%s}") % (response_cpp_name if is_writeable else cpp_name)

                            if is_readable and is_writeable:
                                emit.Line("%s = %s;" % (response_cpp_name, cpp_name))
                        else:
                            initializer = (("(%s)" if isinstance(arg, JsonObject) else "{%s}") % cpp_name) if is_readable else "{}"

                        emit.Line("%s%s %s%s;" % (cv_qualifier, (arg.cpp_type + "&") if json_source else arg.cpp_native_type, arg.TempName(), initializer))

                emit.Line()

                # Emit call to the implementation
                if not json_source: # Full automatic mode
                    conditions = []

                    for _, [arg, _] in vars.items():
                        if arg.flags.get("release"):
                            conditions.append("(%s != nullptr)" % arg.TempName())
                            if len(conditions) == 1:
                                emit.Line()

                            emit.Line("ASSERT(%s != nullptr); " % arg.TempName())

                    if conditions:
                        emit.Line()
                        emit.Line("if (%s) {" % " && ".join(conditions))
                        emit.Indent()

                    implementation_object = "(static_cast<const %s*>(%s))" % (face, impl_var) if const_cast else impl_var
                    function_params = []

                    if indexed:
                        function_params.append(m.index.TempName("converted_") if index_converted else m.index.TempName("_"))

                    for _, [arg, _] in sorted(vars.items(), key=lambda x: x[1][0].schema["position"]):
                        function_params.append("%s%s" % (arg.flags.get("prefix"), arg.flags.get("cast") if arg.flags.get("cast") else arg.TempName()))

                    if not conditions:
                        emit.Line()

                    emit.Line("%s = %s->%s(%s);" % (error_code.TempName(), implementation_object, m.cpp_name, ", ".join(function_params)))

                    if conditions:
                        for _, _record in vars.items():
                            arg = _record[0]
                            if arg.flags.get("release"):
                                emit.Line("%s->Release();" % arg.TempName())

                        emit.Unindent()
                        emit.Line("} else {")
                        emit.Indent()
                        emit.Line("%s = Core::ERROR_GENERAL;" % error_code.TempName())
                        emit.Unindent()
                        emit.Line("}")

                # Semi-automatic mode
                else:
                    parameters = []

                    if indexed:
                        parameters.append(m.index.TempName("converted_") if index_converted else m.index.TempName("_"))

                    for _, [ arg, _ ] in sorted(vars.items(), key=lambda x: x[1][0].schema["position"]):
                        parameters.append("%s" % (arg.TempName()))

                    if const_cast:
                        emit.Line("%s = (static_cast<const IMPLEMENTATION&>(%s)).%s(%s);" % (error_code.TempName(), impl_var, m.function_name, ", ".join(parameters)))
                    else:
                        emit.Line("%s = %s.%s(%s);" % (error_code.TempName(), impl_var, m.function_name, ", ".join(parameters)))


                    parameters = []

                    if indexed:
                        parameters.append("const %s& %s" % (m.index.cpp_native_type, m.index.TempName("_")))

                    for _, [ arg, type ] in sorted(vars.items(), key=lambda x: x[1][0].schema["position"]):
                        parameters.append("%s%s& %s" % ("const " if type == "r" else "", arg.cpp_type, arg.local_name))

                    prototypes.append(["uint32_t %s(%s)%s" % (m.function_name, ", ".join(parameters), (" const" if (const_cast or (isinstance(m, JsonProperty) and m.readonly)) else "")), "Core::ERROR_NONE"])

                emit.Line()

                # Emit result handling and serialization to JSON data

                if response and not response.is_void and not json_source:
                    emit.Line("if (%s == Core::ERROR_NONE) {" % error_code.TempName())
                    emit.Indent()

                    for _, [arg, arg_type] in sorted(vars.items(), key=lambda x: x[1][0].schema["position"]):
                        if "w" not in arg_type:
                            continue

                        cpp_name = (repsonse_parent + arg.cpp_name) if repsonse_parent else arg.local_name

                        # Special case for C-style buffers disguised as base64-encoded strings
                        if isinstance(arg, JsonString) and "length" in arg.flags:
                            length_var = arg.flags.get("length")

                            emit.Line()
                            emit.Line("if (%s != 0) {" % length_var.TempName())
                            emit.Indent()

                            if arg.schema.get("encode"):
                                encoded_name = arg.TempName("encoded_")
                                emit.Line("%s %s;" % (arg.cpp_native_type, encoded_name))
                                emit.Line("Core::ToString(%s, %s, true, %s);" % (arg.TempName(), length_var.TempName(), encoded_name))
                                emit.Line("%s = %s;" % (cpp_name, encoded_name))
                            else:
                                emit.Line("%s = string(%s, %s);" % (cpp_name, arg.TempName(), length_var.TempName()))

                            emit.Unindent()
                            emit.Line("}")

                        # Special case for iterators disguised as arrays
                        elif isinstance(arg, JsonArray):
                            item_name = arg.items.TempName("item_")
                            if arg.iterator:
                                emit.Line("ASSERT(%s != nullptr);" % arg.TempName())
                                emit.Line()
                                emit.Line("if (%s != nullptr) {" % arg.TempName())
                                emit.Indent()
                                emit.Line("%s %s{};" % (arg.items.cpp_native_type, item_name))
                                emit.Line("while (%s->Next(%s) == true) { %s.Add() = %s; }" % (arg.TempName(), item_name, cpp_name, item_name))
                                emit.Line("%s->Release();" % arg.TempName())
                                emit.Unindent()
                                emit.Line("}")
                            elif arg.items.schema.get("bitmask"):
                                emit.Line("%s = %s;" % (cpp_name, arg.TempName()))
                            elif json_source:
                                emit.Line("for (const %s& %s : %s) {" % (arg.items.cpp_native_type, item_name, arg.TempName()))
                                emit.Indent()

                                if isinstance(arg.items, JsonObject):
                                    emit.Line("%s.Add(%s);" % (cpp_name, item_name))
                                else:
                                    emit.Line("%s.Add() = %s;" % (cpp_name, item_name))

                                emit.Unindent()
                                emit.Line("}")
                            else:
                                raise RuntimeError("unable to serialize a non-iterator array: %s" % arg.json_name)

                        # All others...
                        else:
                            emit.Line("%s = %s;" % (cpp_name, arg.TempName()))

                            if arg.schema.get("opaque"):
                                emit.Line("%s.SetQuoted(false);" % (cpp_name))

                    emit.Unindent()
                    emit.Line("}")
                    emit.Line()

            # Emit the function body

            params_parent = (params.local_name + '.') if (isinstance(params, JsonObject) and params.do_create) else ""
            response_parent = (response.local_name + '.') if (isinstance(response, JsonObject) and response.do_create) else ""

            emit.Indent()
            error_code = AuxJsonInteger("errorCode_", 32)
            emit.Line("%s %s%s;" % (error_code.cpp_native_type, error_code.TempName(), " = Core::ERROR_NONE" if json_source else ""))
            emit.Line()

            if isinstance(m, JsonProperty):
                # Emit property code

                if indexed:
                    # Automatically convert integral indexes in properties
                    if isinstance(m.index, JsonInteger):
                        index_converted = True
                        emit.Line("%s %s{};" % (m.index.cpp_native_type, m.index.TempName("converted_")))
                        emit.Line()
                        emit.Line("if (Core::FromString(%s, %s) == false) {" % (m.index.TempName("_"), m.index.TempName("converted_")))
                        emit.Indent()
                        emit.Line("%s = Core::ERROR_UNKNOWN_KEY;" % error_code.TempName())

                        if not m.writeonly and not m.readonly:
                            emit.Line("%s%s.Null(true);" % ("// " if isinstance(response, (JsonArray, JsonObject)) else "", response.local_name)) # FIXME

                        emit.Unindent()
                        emit.Line("} else {")
                        emit.Indent()
                    elif isinstance(m.index, JsonEnum):
                        index_converted = True
                        emit.Line("Core::EnumerateType<%s> %s(%s.c_str());" % (m.index.cpp_native_type, m.index.TempName("converted_"), m.index.TempName("_")))
                        emit.Line()
                        emit.Line("if (%s.IsSet() == false) {" % m.index.TempName("converted_"))
                        emit.Indent()
                        emit.Line("%s = Core::ERROR_UNKNOWN_KEY;" % error_code.TempName())

                        if not m.writeonly and not m.readonly:
                            emit.Line("%s%s.Null(true);" % ("// " if isinstance(response, (JsonArray, JsonObject)) else "", response.local_name)) # FIXME

                        emit.Unindent()
                        emit.Line("} else {")
                        emit.Indent()

                if not m.readonly and not m.writeonly:
                    emit.Line("if (%s.IsSet() == false) {" % (params.local_name))
                    emit.Indent()
                    emit.Line("// property get")
                elif m.readonly:
                    emit.Line("// read-only property get")

                if not m.writeonly:
                    _Invoke(None, response, True, not m.readonly, params_parent, response_parent)

                if not m.readonly:
                    if not m.writeonly:
                        emit.Unindent()
                        emit.Line("} else {")
                        emit.Indent()
                        emit.Line("// property set")

                    if m.writeonly:
                        emit.Line("// write-only property set")

                    _Invoke(params, None, False, False, params_parent, response_parent)

                    if not m.writeonly:
                        emit.Line()
                        emit.Line("%s%s.Null(true);" % ("// " if isinstance(response, (JsonArray, JsonObject)) else "", response.local_name)) # FIXME
                        emit.Unindent()
                        emit.Line("}")

                if index_converted:
                    emit.Unindent()
                    emit.Line("}")

            else:
                # Emit method code
                _Invoke(params, response, True, False, params_parent, response_parent)

            # Emit method epilogue

            emit.Line("return (%s);" % error_code.TempName())
            emit.Unindent()
            emit.Line("});")
            emit.Unindent()
            emit.Line()

            method_count += 1

        elif isinstance(m, JsonNotification):
            events.append(m)

    # Emit event status registrations

    if has_listeners and json_source:
        emit.Line("// Register event status listeners...")
        emit.Line()

        for event in events:
            if event.is_status_listener:
                emit.Line("%s.RegisterEventStatusListener(_T(\"%s\")," % (module_var, event.json_name))
                emit.Indent()
                emit.Line("[&%s](const string& client, const JSONRPC::Status status) {" % (impl_var))
                emit.Indent()
                emit.Line("const string id = client.substr(0, client.find('.'));")
                emit.Line("%s.On%sEventRegistration(id, status);" % (impl_var, event.function_name))
                emit.Unindent()
                emit.Line("});")
                emit.Unindent()
                emit.Line()

                prototypes.append(["void On%sEventRegistration(const string& client, const %s::JSONRPC::Status status)" % (event.function_name, struct), None])

    emit.Unindent()
    emit.Line("}")
    emit.Line()

    # Emit method deregistrations

    emit.Line("static void Unregister(JSONRPC& %s)" % module_var)
    emit.Line("{")
    emit.Indent()

    if method_count:
        emit.Line("// Unregister methods and properties...")

        for m in root.properties:
            if isinstance(m, JsonMethod) and not isinstance(m, JsonNotification):
                emit.Line("%s.Unregister(%s);" % (module_var, Tstring(m.json_name)))

    # Emit event status deregistrations

    if has_listeners and json_source:
        emit.Line()
        emit.Line("// Unregister event status listeners...")

        for event in events:
            if event.is_status_listener:
               emit.Line("%s.UnregisterEventStatusListener(%s);" % (module_var, Tstring(event.json_name)))

    emit.Unindent()
    emit.Line("}")
    emit.Line()

    # Finally emit event code

    if events:
        emit.Line("namespace Event {")
        emit.Indent()
        emit.Line()
        emit.Line("PUSH_WARNING(DISABLE_WARNING_UNUSED_FUNCTIONS)")
        emit.Line()

        for event in events:
            EmitEvent(emit, root, event, "object")

            if not event.params.is_void:
                if isinstance(event.params, JsonObject):
                    EmitEvent(emit, root, event, "json")

                EmitEvent(emit, root, event, "native")

        emit.Line("POP_WARNING()")
        emit.Line()
        emit.Unindent()
        emit.Line("} // namespace Event")
        emit.Line()

    # Return collected signatures, so the emited file can be prepended with
    return prototypes

def EmitRpcCode(root, emit, header_file, source_file, data_emitted):
    prototypes = _EmitRpcCode(root, emit, header_file, source_file, data_emitted)

    with emitter.Emitter(None, config.INDENT_SIZE) as prototypes_emitter:
        _EmitRpcPrologue(root, prototypes_emitter, header_file, source_file, data_emitted, prototypes)
        emit.Prepend(prototypes_emitter)

    _EmitRpcEpilogue(root, emit)

def EmitRpcVersionCode(root, emit, header_file, source_file, data_emitted):
    _EmitRpcPrologue(root, emit, header_file, source_file, data_emitted)
    _EmitVersionCode(emit, rpc_version.GetVersion(root.schema["info"] if "info" in root.schema else dict()))
    _EmitRpcEpilogue(root, emit)
