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

import argparse
import sys
import os
import json
import posixpath
import importlib
from json_helper import JSON
import traceback

INDENT_SIZE = 2

sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), os.pardir))

import ProxyStubGenerator.Log as Log

NAME = "ConfigGenerator"
VERBOSE = False
SHOW_WARNINGS = True
DOC_ISSUES = False

log = Log.Log(NAME, VERBOSE, SHOW_WARNINGS, DOC_ISSUES)


def file_name(path):
    file_ext = os.path.basename(path)
    index_of_dot = file_ext.index('.')
    name = file_ext[0:index_of_dot]
    return name


def load_module(name, path):
    import importlib.machinery
    import importlib.util
    loader = importlib.machinery.SourceFileLoader(name, path)
    spec = importlib.util.spec_from_loader(loader.name, loader)
    if spec is not None:
        mod = importlib.util.module_from_spec(spec)
        try:
            loader.exec_module(mod)
        except Exception as exception:
            log.Error(exception.__class__.__name__ + ": " )
            traceback.print_exc()
            sys.exit(1)
        return mod
    return None


if __name__ == "__main__":
    argparser = argparse.ArgumentParser(
        description='JSON Config Builder',
        formatter_class=argparse.RawTextHelpFormatter)

    argparser.add_argument("-l",
                           "--locator",
                           metavar="LibName",
                           action="store",
                           type=str,
                           dest="locator",
                           help="locator of plugin e.g, libWPEFrameworkDeviceInfo.so")

    argparser.add_argument("-c",
                           "--classname",
                           dest="classname",
                           metavar="ClassName",
                           action="store",
                           type=str,
                           help="classname e.g, DeviceInfo")

    argparser.add_argument("-i",
                           "--inputconfig",
                           dest="configfile",
                           metavar="ConfigFile",
                           action="store",
                           type=str,
                           help="CONFIG to read")

    argparser.add_argument("-o",
                           "--outputfile",
                           dest="ofile",
                           metavar="OutputFile",
                           action="store",
                           type=str,
                           help="Output File")

    argparser.add_argument("--indent",
                           dest="indent_size",
                           metavar="SIZE",
                           type=int,
                           action="store",
                           default=INDENT_SIZE,
                           help="code indentation in spaces (default: %i)" % INDENT_SIZE)

    argparser.add_argument("project",
                           metavar="Name",
                           action="store",
                           type=str,
                           help="Project name e.g, DeviceInfo")

    argparser.add_argument("projectdir",
                           metavar="ConfigDir",
                           action="store",
                           type=str,
                           help="Project Working dir")

    args = argparser.parse_args(sys.argv[1:])

    #  log.Print("Preparing Config JSON")
    result = JSON()
    if args.locator:
        result.add("locator", args.locator)
    else:
        result.add("locator", "libWPEFramework" + args.project + ".so")

    if args.classname:
        result.add("classname", args.classname)
    else:
        result.add("classname", args.project)

    if not os.path.exists(args.projectdir):
        log.Error(f"Error: Config Dir path {args.projectdir} doesnt exit\n")
        sys.exit(1)

    if args.configfile:
        cf = args.configfile
    else:
        cf = args.project + ".config"

    args.projectdir = os.path.abspath(os.path.normpath(args.projectdir))
    cf = args.projectdir + "/" + cf

    if os.path.exists(cf):
        iconfig = load_module(file_name(cf), cf)
        if iconfig:
            if hasattr(iconfig, 'config'):
                result.update(iconfig.config)
            else:
                log.Print("Empty Config File")
        else:
            log.Error(f"Config File {cf} exists but couldn't load")
            sys.exit(1)
    else:
        log.Print("No Config File")

    if args.ofile:
        of = args.ofile
    else:
        of = args.projectdir + file_name(cf) + ".json"

    log.Print("Writing Config JSON")

    # Writing JSON file
    with open(of, "w") as outfile:
        outfile.write(result.to_json(args.indent_size))
