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

import json

import config
import rpc_emitter
from json_loader import *


def EmitHelperCode(log, root, emit, header_file):
    if config.DUMP_JSON:
        print("\n// JSON interface -----------")
        print(json.dumps(root.schema, indent=2))
        print("// ----------------\n")

    if root.objects:

        def _ScopedName(obj):
            ns = config.DATA_NAMESPACE + "::" + (root.json_name if not obj.included_from else obj.included_from)
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
        emit.Line("#include <%s%s>" % (config.JSON_INTERFACE_PATH, header_file))

        for inc in root.includes:
            emit.Line("#include <%s%s_%s.h>" % (config.JSON_INTERFACE_PATH, config.DATA_NAMESPACE, inc))

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
                parameters = []

                if not method.params.is_void:
                    parameters.append("const %s& %s" % (method.params.cpp_type, method.params.local_name))

                if not method.result.is_void:
                    parameters.append("%s& %s" % (method.result.cpp_type, method.result.local_name))

                line = "uint32_t %s(%s);" % (method.endpoint_name, ", ".join(parameters))

                if method.included_from:
                    line += " // %s" % method.included_from

                emit.Line(line)

        for method in root.properties:
            if isinstance(method, JsonProperty):
                if not method.writeonly:
                    parameters = []

                    if method.index:
                        parameters.append("const string& index")

                    if not method.result.is_void:
                        parameters.append("%s& %s" % (method.result.cpp_type, method.result.local_name))

                    line = "uint32_t %s(%s) const;" % (method.endpoint_get_name, ", ".join(parameters))

                    if method.included_from:
                        line += " // %s" % method.included_from

                    emit.Line(line)

                if not method.readonly:
                    parameters = []

                    if method.index:
                        parameters.append("const string& index")

                    if not method.params.is_void:
                        parameters.append("const %s& %s" % (method.params.cpp_type, method.params.local_name))

                    line = "uint32_t %s(%s);" % (method.endpoint_set_name, ", ".join(parameters))

                    if method.included_from:
                        line += " // %s" % method.included_from

                    emit.Line(line)

        for method in root.properties:
            if isinstance(method, JsonNotification):
                parameters = []

                if method.sendif_type:
                    parameters.append("const string& id")

                if not method.params.is_void:
                    if method.params.properties and method.params.do_create:
                        for p in method.params.properties:
                            parameters.append("const %s& %s" % (p.cpp_native_type, p.local_name))
                    else:
                        parameters.append("const %s& %s" % (method.params.cpp_native_type, method.params.local_name))

                line = 'void %s(%s);' % (method.endpoint_name, ", ".join(parameters))

                if method.included_from:
                    line += " // %s" % method.included_from

                emit.Line(line)

        emit.Unindent()
        emit.Unindent()
        emit.Line("*/")
        emit.Line()

        # Registration code
        emit.Line("namespace %s {" % config.FRAMEWORK_NAMESPACE)
        emit.Line()
        emit.Line("namespace %s {" % config.PLUGIN_NAMESPACE)
        emit.Indent()
        emit.Line()

        emit.Line()
        emit.Line("// Registration")
        emit.Line("//")
        emit.Line()
        emit.Line("void %s::RegisterAll()" % root.json_name)
        emit.Line("{")
        emit.Indent()

        for method in root.properties:
            if isinstance(method, JsonNotification) and method.is_status_listener:
                emit.Line("RegisterEventStatusListener(%s), [this](const string& client, Status status) {" % Tstring(method.json_name))
                emit.Indent()
                emit.Line("const string id = client.substr(0, client.find('.'));")
                emit.Line("// TODO...")
                emit.Unindent()
                emit.Line("});")
                emit.Line()

        for method in root.properties:
            if not isinstance(method, JsonNotification) and not isinstance(method, JsonProperty):
                line = 'Register<%s,%s>(%s, &%s::%s, this);' % (method.params.cpp_type, method.result.cpp_type, Tstring(method.json_name), root.json_name, method.endpoint_name)

                if method.included_from:
                    line += " /* %s */" % method.included_from

                emit.Line(line)

        for method in root.properties:
            if isinstance(method, JsonProperty):
                line = ""

                if root.rpc_format != config.RpcFormat.COMPLIANT or method.readonly or method.writeonly:
                    line += 'Property<%s>(%s' % (method.params.cpp_type if not method.readonly else method.result.cpp_type, Tstring(method.json_name))
                    line += ", &%s::%s" % (root.json_name, method.endpoint_get_name) if not method.writeonly else ", nullptr"
                    line += ", &%s::%s" % (root.json_name, method.endpoint_set_name) if not method.readonly else ", nullptr"
                    line += ', this);'
                else:
                    line = 'Register<%s,%s>(%s, ([this](const %s& Params, %s& Response) { if (Params.IsSet() == true) return(%s(Params)); else return(%s(Response); }), this);' % (
                        method.params.cpp_type, method.result.cpp_type, Tstring(method.json_name),
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
                emit.Line('Unregister(%s);' % Tstring(method.json_name))

        for method in reversed(root.properties):
            if isinstance(method, JsonProperty):
                emit.Line('Unregister(%s);' % Tstring(method.json_name))

        for method in reversed(root.properties):
            if isinstance(method, JsonNotification) and method.is_status_listener:
                emit.Line()
                emit.Line("UnregisterEventStatusListener(%s);" % Tstring(method.json_name))

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

                emit.Line("// Method: %s" % method.Headline())
                emit.Line("// Return codes:")
                emit.Line("//  - ERROR_NONE: Success")

                for e in method.errors:
                    description = e["description"] if "description" in e else ""
                    emit.Line("//  - %s: %s" % (e["message"], description))

                parameters = []

                if not method.params.is_void:
                    parameters.append("const %s& %s" % (method.params.cpp_type, method.params.local_name))

                if not method.result.is_void:
                    parameters.append("%s& %s" % (method.result.cpp_type, method.result.local_name))

                line = "uint32_t %s(%s)" % (method.endpoint_name, ", ".join(parameters))

                if method.included_from:
                    line += " /* %s */" % method.included_from

                emit.Line(line)
                emit.Line("{")
                emit.Indent()
                emit.Line("uint32_t errorCode = Core::ERROR_NONE;")
                emit.Line()

                if not method.params.is_void:
                    for p in method.params.properties:
                        if not isinstance(p, (JsonObject, JsonArray)):
                            emit.Line("const %s& %s = params.%s.Value();" % (p.cpp_native_type, p.json_name, p.cpp_name))
                        elif isinstance(p, (JsonObject)):
                            for prop in p.properties:
                                emit.Line("// %s.%s.%s ..." % (method.params.local_name, p.cpp_name, prop.cpp_name))
                        else:
                            emit.Line("// %s.%s ..." % (method.params.local_name, p.cpp_name))

                emit.Line()
                emit.Line("// TODO...")
                emit.Line()

                if not method.result.is_void:
                    if not isinstance(method.result, JsonObject):
                        emit.Line("// %s = ... " % method.result.local_name)
                    else:
                        for p in method.result.properties:
                            emit.Line("// %s.%s = ..." % (method.result.local_name, p.cpp_name))

                    emit.Line()

                emit.Line("return (errorCode);")
                emit.Unindent()
                emit.Line("}")
                emit.Line()

        for method in root.properties:
            if isinstance(method, JsonProperty):

                def _EmitProperty(method, name, getter):
                    emit.Line("// Property: %s" % method.Headline())
                    emit.Line("// Return codes:")
                    emit.Line("//  - ERROR_NONE: Success")

                    for e in method.errors:
                        description = e["description"] if "description" in e else ""
                        emit.Line("//  - %s: %s" % (e["message"], description))

                    parameters = []

                    if method.index:
                        parameters.append("const string& index")

                    if getter:
                        if not method.result.is_void:
                            parameters.append("%s& %s" % (method.result.cpp_type, method.result.local_name))

                        line = "uint32_t %s(%s) const" % (method.endpoint_get_name, ", ".join(parameters))
                    else:
                        if not method.params.is_void:
                            parameters.append("const %s& %s" % (method.params.cpp_type, method.params.local_name))

                        line = "uint32_t %s(%s)" % (method.endpoint_set_name, ", ".join(parameters))

                    if method.included_from:
                        line += " /* %s */" % method.included_from

                    emit.Line(line)
                    emit.Line("{")
                    emit.Indent()

                    emit.Line("uint32_t errorCode = Core::ERROR_NONE;")

                    if not getter and not method.params.is_void:
                        for p in method.params.properties:
                            if not isinstance(p, (JsonObject, JsonArray)):
                                emit.Line("const %s& %s = %s.%s.Value();" % (p.cpp_native_type, p.local_name, method.params.local_name, p.cpp_name))
                            elif isinstance(p, (JsonObject)):
                                for prop in p.properties:
                                    emit.Line("// %s.%s.%s ..." % (method.params.local_name, p.cpp_name, prop.cpp_name))
                            else:
                                emit.Line("// %s.%s ..." % (method.params.local_name, p.cpp_name))

                    emit.Line()
                    emit.Line("// TODO...")
                    emit.Line()

                    if getter:
                        if not isinstance(method.result, JsonObject):
                            emit.Line("// %s = ... " % method.result.local_name)
                        else:
                            for p in method.result.properties:
                                emit.Line("// %s.%s = ..." % (method.result.local_name, p.cpp_name))
                        emit.Line()

                    emit.Line()
                    emit.Line("return (errorCode);")
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
                rpc_emitter.EmitEvent(emit, root, method, "native", legacy=True)

        emit.Unindent()
        emit.Line("} // namespace %s" % config.PLUGIN_NAMESPACE)
        emit.Line()
        emit.Line("}")
        emit.Line()
