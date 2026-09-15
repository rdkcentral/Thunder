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

import config
import trackers
import stub_emitter
import rpc_emitter
import class_emitter

from emitter import Emitter
from json_loader import *

def CreateApiHeader(log, source_name, path, headers):
    assert(headers)

    header_file = os.path.join(path, "json_" + source_name + ".h")
    with Emitter(header_file, config.INDENT_SIZE) as emitter:
        emitter.Line("// %s JSON-RPC API" % os.path.basename(source_name))
        emitter.Line("// Generated automatically. DO NOT EDIT.")
        emitter.Line()
        emitter.Line("#pragma once")
        emitter.Line()

        sorted = [_h for _h in headers if config.DATA_NAMESPACE in _h]
        sorted.extend([_h for _h in headers if config.DATA_NAMESPACE not in _h])

        for h in sorted:
            emitter.Line("#include \"%s\"" % os.path.basename(h))

        log.Success("JSON-RPC API header generated in %s" % (os.path.basename(emitter.FileName())))

def Create(log, schema, source_file, path, additional_includes, generate_classes, generate_stubs, generate_rpc):
    def _ParseJsonRpcSchema(schema):
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

    headers = []

    directory = path[0]
    cpp_directory = path[1]

    filename = (schema["info"]["namespace"]) if "info" in schema and "namespace" in schema["info"] else ""
    filename += (schema["info"]["class"]) if "info" in schema and "class" in schema["info"] else ""

    if len(filename) == 0:
        filename = os.path.basename(source_file.replace("Plugin", "").replace(".json", "").replace(".h", ""))

    rpcObj = _ParseJsonRpcSchema(schema)
    if rpcObj:
        header_file = os.path.join(directory, config.DATA_NAMESPACE + "_" + filename + ".h")
        enum_file = os.path.join(cpp_directory, "JsonEnum_" + filename + ".cpp")

        data_emitted = 0

        if generate_classes:
            # Generate classes...
            if not config.FORCE and (os.path.exists(header_file) and (os.path.getmtime(source_file) < os.path.getmtime(header_file))):
                log.Success("skipping file %s, up-to-date" % os.path.basename(header_file))
                data_emitted = 1
            else:
                with Emitter(header_file, config.INDENT_SIZE) as emitter:
                    data_emitted = class_emitter.EmitObjects(log, rpcObj, emitter, os.path.basename(source_file), additional_includes, True)

                    if data_emitted:
                        log.Success("JSON data classes generated in %s" % os.path.basename(emitter.FileName()))
                        headers.append(header_file)
                    else:
                        log.Info("No JSON data classes generated for %s" % os.path.basename(filename))

                if not data_emitted and not config.KEEP_EMPTY:
                    try:
                        os.remove(header_file)
                    except:
                        pass

            # Generate enum registrations...
            if not config.FORCE and (os.path.exists(enum_file) and (os.path.getmtime(source_file) < os.path.getmtime(enum_file))):
                log.Success("skipping file %s, up-to-date" % os.path.basename(enum_file))
                class_emitter.ProcessEnums(log)
            else:
                enum_emitted = 0

                with Emitter(enum_file, config.INDENT_SIZE) as emitter:
                    enum_emitted = class_emitter.EmitEnumRegs(log, rpcObj, emitter, filename, os.path.basename(source_file))

                    if enum_emitted:
                        log.Success("JSON enumeration code generated in %s" % os.path.basename(emitter.FileName()))
                    else:
                        log.Info("No JSON enumeration code generated for %s" % os.path.basename(filename))

                if not enum_emitted and not config.KEEP_EMPTY:
                    try:
                        os.remove(enum_file)
                    except:
                        pass

            # Also emit version if source was json meta file in manual mode
            if (rpcObj.schema.get("mode") != "auto") and not config.NO_VERSIONING:
                output_filename = os.path.join(directory, "J" + filename + ".h")

                if not config.FORCE and (os.path.exists(output_filename) and (os.path.getmtime(source_file) < os.path.getmtime(output_filename))):
                    log.Success("skipping file %s, up-to-date" % os.path.basename(output_filename))
                else:
                    with Emitter(output_filename, config.INDENT_SIZE) as emitter:
                        rpc_emitter.EmitRpcVersionCode(rpcObj, emitter, filename, os.path.basename(source_file), data_emitted)
                        log.Success("JSON-RPC version information generated in %s" % os.path.basename(emitter.FileName()))
                        headers.append(output_filename)


        # Generate manual stub code...
        if generate_stubs:
            with Emitter(os.path.join(cpp_directory, filename + "JsonRpc.cpp"), config.INDENT_SIZE) as emitter:
                stub_emitter.EmitHelperCode(log, rpcObj, emitter, os.path.basename(header_file))
                log.Success("JSON-RPC stubs generated in %s" % os.path.basename(emitter.FileName()))

        # Generate full or semi automatic RPC code
        if generate_rpc and (rpcObj.schema.get("mode") == "auto"):
            output_filename = os.path.join(directory, "J" + filename + ".h")

            if not config.FORCE and (os.path.exists(output_filename) and (os.path.getmtime(source_file) < os.path.getmtime(output_filename))):
               log.Success("skipping file %s, up-to-date" % os.path.basename(output_filename))
            else:
                with Emitter(output_filename, config.INDENT_SIZE) as emitter:
                    rpc_emitter.EmitRpcCode(rpcObj, emitter, filename, os.path.basename(source_file), data_emitted)
                    log.Success("JSON-RPC implementation generated in %s" % os.path.basename(emitter.FileName()))
                    headers.append(output_filename)

    else:
        log.Info("No code to generate.")

    return headers

