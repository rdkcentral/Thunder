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

NAME = "JsonGenerator"

if __name__ == "__main__":
    argparser, args = config.Parse(sys.argv)

    log = logger.Create(NAME, args.verbose, not args.no_duplicates_warnings, not args.no_style_warnings)
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
            try:
                log.Header(path)

                schemas, additional_includes, temp_files = json_loader.Load(log, path, args.if_dir, args.cppif_dir, args.includePaths)

                for schema in schemas:
                    if schema:
                        warnings = config.GENERATED_JSON
                        config.GENERATED_JSON = "@generated" in schema

                        output_path = path

                        if args.output_dir:
                            if (args.output_dir[0]) == os.sep:
                                output_path = os.path.join(args.output_dir, os.path.basename(output_path))
                            else:
                                dir = os.path.join(os.path.dirname(output_path), args.output_dir)
                                if not os.path.exists(dir):
                                    os.makedirs(dir)

                                output_path = os.path.join(dir, os.path.basename(output_path))

                        if args.code or args.stubs:
                            code_generator.Create(log, schema, path, output_path, additional_includes, args.code, args.stubs, args.code)

                        if args.docs:
                            if "$schema" in schema:
                                if "info" in schema:
                                    title = schema["info"]["title"] if "title" in schema["info"] \
                                            else schema["info"]["class"] if "class" in schema["info"] \
                                            else os.path.basename(output_path)
                                else:
                                    title = os.path.basename(output_path)

                                documentation_generator.Create(log, schema, os.path.join(os.path.dirname(output_path), title.replace(" ", "")))
                            else:
                                log.Warn("Skiping file; not a JSON-RPC definition file")

                        config.GENERATED_JSON = warnings

            except json_loader.JsonParseError as err:
                log.Error(str(err))
            except header_loader.CppParseError as err:
                log.Error(str(err))
            except documentation_generator.DocumentationError as err:
                log.Error(str(err))
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
