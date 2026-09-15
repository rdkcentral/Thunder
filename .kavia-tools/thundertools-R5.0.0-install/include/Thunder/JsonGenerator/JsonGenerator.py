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

import sys
import os
import glob

try:
    import jsonref
except:
    print("Install jsonref first")

sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), "source"))

import logger
import config
import code_generator
import documentation_generator
import json_loader
import header_loader
import trackers
import rpc_emitter

NAME = "JsonGenerator"

if __name__ == "__main__":
    argparser, args = config.Parse(sys.argv)

    log = logger.Create(NAME, args.verbose, not args.no_warnings, not args.no_style_warnings)
    trackers.SetLogger(log)
    json_loader.SetLogger(log)

    temp_files = [] # Track temporary files the procedure may create

    if not args.path or (not args.code and not args.stubs and not args.docs):
        argparser.print_help()
    else:
        files = []

        # Resolve wildcards
        for p in args.path:
            if "*" in p or "?" in p:
                files.extend(glob.glob(p))
            else:
                files.append(p)

        for path in files:

            trackers.enum_tracker.Reset()

            try:
                log.Header(path)

                schemas, additional_includes, temp_files = json_loader.Load(log, path, args.if_dirs, args.cpp_if_dirs, args.include_paths)

                joint_headers = {}

                for schema in schemas:
                    trackers.object_tracker.Reset()

                    if schema:
                        warnings = config.GENERATED_JSON
                        config.GENERATED_JSON = schema.get("@generated")

                        if args.output_dir:
                            if (args.output_dir[0]) == os.sep:
                                output_path = os.path.normpath(args.output_dir)
                            else:
                                output_path = os.path.join(os.path.dirname(path), os.path.normpath(args.output_dir))

                            if not os.path.exists(output_path):
                                os.makedirs(output_path)
                        else:
                            output_path = os.path.dirname(path)

                        if args.cpp_output_dir:
                            if (args.cpp_output_dir[0]) == os.sep:
                                cpp_output_path = os.path.normpath(args.cpp_output_dir)
                            else:
                                cpp_output_path = os.path.join(os.path.dirname(path), os.path.normpath(args.cpp_output_dir))

                            if not os.path.exists(cpp_output_path):
                                os.makedirs(cpp_output_path)
                        else:
                            cpp_output_path = output_path

                        if args.code or args.stubs:
                            headers = code_generator.Create(log, schema, path, [output_path, cpp_output_path], additional_includes, args.code, args.stubs, args.code)

                            name = os.path.basename(path).replace(".h", "").replace(".json", "")

                            if headers:
                                if name not in joint_headers:
                                    joint_headers[name] = []

                                joint_headers[name].extend(headers)

                        if args.docs:
                            if "$schema" in schema:
                                if "info" in schema:
                                    title = schema["info"]["title"] if "title" in schema["info"] \
                                            else schema["info"]["class"] if "class" in schema["info"] \
                                            else os.path.basename(output_path)
                                else:
                                    title = os.path.basename(output_path)

                                documentation_generator.Create(log, schema, os.path.join(output_path, title.replace(" ", "")))
                            else:
                                log.Warn("Skiping file; not a JSON-RPC definition file")

                        config.GENERATED_JSON = warnings

                for n in joint_headers:
                    code_generator.CreateApiHeader(log, n, output_path, joint_headers[n])

            except json_loader.JsonParseError as err:
                log.Error("JSON loader: " + str(err))
            except header_loader.CppParseError as err:
                log.Error("Header loader: " + str(err))
            except documentation_generator.DocumentationError as err:
                log.Error("Documentation: " + str(err))
            except rpc_emitter.RPCEmitterError as err:
                log.Error("RPC emitter: " + str(err))
            except IOError as err:
                log.Error(str(err))
            except jsonref.JsonRefError as err:
                log.Error(str(err))

        log.Info("JsonGenerator: All done, {} files parsed, {} error{}.".format(len(files),
                    len(log.errors) if log.errors else 'no', '' if len(log.errors) == 1 else 's'))

        # Remove temporary files
        for tf in temp_files:
            os.remove(tf)

        # Set error code for shell
        if log.errors:
            sys.exit(1)
