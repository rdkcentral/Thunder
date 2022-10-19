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
import inspect
import sys
import os
from importlib import machinery, util
import types
import re

from json_helper import JSON
import traceback

INDENT_SIZE = 2
PARAM_CONFIG = "params.config"
boiler_plate = "from json_helper import *"

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
    out = False
    mod = None
    loader = machinery.SourceFileLoader(name, path)
    spec = util.spec_from_loader(loader.name, loader)
    if spec is not None:
        mod = util.module_from_spec(spec)
        try:
            loader.exec_module(mod)
            out = True
        except Exception as exception:
            log.Error(exception.__class__.__name__ + ": ")
            traceback.print_exc()

    return out, mod


def prepend_file(file, line):
    try:
        f = open(file, 'r+')
    except IOError:
        log.Error(f"Error opening File {file}")
    else:
        with f:
            content = f.read()
            f.seek(0, 0)
            f.write(line.rstrip('\r\n') + '\n' + content)


def get_config_params(file):
    try:
        f = open(file)
    except IOError:
        log.Error(f"Error opening File {file}")
        lines = []
    else:
        with f:
            lines = [line.rstrip() for line in f]

    return lines

# Routine to find if a given variable is copied to another variable
# Basically we are finding out if a var is found within brackets as the only way to add
# any variable is by add() method.
def check_assignment(file, var):
    out = False
    try:
        f = open(file)
    except IOError:
        log.Error(f"Error opening File {file}")
    else:
        with f:
            for line in f:
                slist = re.findall(r'\(.*?\)', line)
                for s in slist:
                    if var in s:
                        out = True
                        break;
                if out:
                    break
    return out


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

    argparser.add_argument("-p",
                           "--paramconfig",
                           dest="params_config",
                           metavar="ParamConfig",
                           action="store",
                           type=str,
                           default=PARAM_CONFIG,
                           help="Full path of File containing Whitelisted params in config file")

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

    if args.classname:
        result.add("classname", args.classname)


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
        prepend_file(cf, boiler_plate)
        res, iconfig = load_module(file_name(cf), cf)
        if not res:
            sys.exit(1)

        if iconfig:
            params = get_config_params(args.params_config)
            if not params:
                log.Error(f"Whitelisted Params not available")
                sys.exit(1)
            isEmpty = True
            for param in iconfig.__dict__:
                if param in params:
                    result.add(param, iconfig.__dict__[param])
                    isEmpty = False
                else:
                    if not param.startswith('__') \
                            and not isinstance(iconfig.__dict__[param], types.ModuleType) \
                            and not isinstance(iconfig.__dict__[param], types.FunctionType) \
                            and not inspect.isclass(iconfig.__dict__[param]):  # Skip the Dunders, Modules
                        if not check_assignment(cf, param):
                            log.Error(f"Unrecognized parameter \"{param}\". \n"
                                      f"*****************************************************************\n"
                                      f"If you are defining a private variable, one way to use it is to\n"
                                      f"prefix it with Double Underscores.\n"
                                      f"e.g, __conf = line.split(',') \n\n"
                                      f"Another way is to define a function and isolate functionality\n"
                                      f"def func():\n"
                                      f"    do_stuff\n"
                                      f"    return result\n"
                                      f"******************************************************************")
                            sys.exit(1)
            if isEmpty:
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

    try:
        outfile = open(of, "w")
    except IOError:
        log.Error(f"Error opening Output File {of}")
        sys.exit(1)
    else:
        with outfile:
            outfile.write(result.serialize(args.indent_size))