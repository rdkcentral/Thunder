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
import posixpath
import argparse
from enum import Enum

# Constants
DATA_NAMESPACE = "JsonData"
PLUGIN_NAMESPACE = "Plugin"
TYPE_PREFIX = "Core::JSON"
OBJECT_SUFFIX = "Data"
COMMON_OBJECT_SUFFIX = "Info"
ENUM_SUFFIX = "Type"
IMPL_ENDPOINT_PREFIX = "endpoint_"
IMPL_EVENT_PREFIX = "event_"

# Configurables
CLASSNAME_FROM_REF = True
DEFAULT_INT_SIZE = 32
SHOW_WARNINGS = True
DOC_ISSUES = True
DEFAULT_DEFINITIONS_FILE = "../../ProxyStubGenerator/default.h"
FRAMEWORK_NAMESPACE = "WPEFramework"
INTERFACE_NAMESPACE = FRAMEWORK_NAMESPACE + "::Exchange"
INTERFACES_SECTION = True
INTERFACE_SOURCE_LOCATION = None
INTERFACE_SOURCE_REVISION = None
NO_INCLUDES = False
DEFAULT_INTERFACE_SOURCE_REVISION = "main"
GLOBAL_DEFINITIONS = ".." + os.sep + "global.json"
INDENT_SIZE = 4
ALWAYS_EMIT_COPY_CTOR = False
KEEP_EMPTY = False
CPP_INTERFACE_PATH = "interfaces" + os.sep
JSON_INTERFACE_PATH = CPP_INTERFACE_PATH + "json"  + os.sep
DUMP_JSON = False
FORCE = False
GENERATED_JSON = False

class RpcFormat(Enum):
    COMPLIANT = "compliant"
    EXTENDED = "uncompliant-extended"
    COLLAPSED = "uncompliant-collapsed"

RPC_FORMAT = RpcFormat.COMPLIANT
RPC_FORMAT_FORCED = False


def Parse(cmdline):
    global DEFAULT_DEFINITIONS_FILE
    global INTERFACE_NAMESPACE
    global JSON_INTERFACE_PATH
    global NO_INCLUDES
    global DEFAULT_INT_SIZE
    global INDENT_SIZE
    global DOC_ISSUES
    global INTERFACE_SOURCE_LOCATION
    global INTERFACE_SOURCE_REVISION
    global INTERFACES_SECTION
    global FORCE
    global DUMP_JSON
    global RPC_FORMAT_FORCED
    global RPC_FORMAT
    global NO_DUP_WARNINGS
    global ALWAYS_EMIT_COPY_CTOR
    global KEEP_EMPTY
    global CLASSNAME_FROM_REF

    argparser = argparse.ArgumentParser(
        description='Generate JSON C++ classes, stub code and API documentation from JSON definition files and C++ header files',
        epilog="For information about custom tags supprted in C++ code please see StubGenerator help (--help-tags).",
        formatter_class=argparse.RawTextHelpFormatter)

    argparser.add_argument('path',
            nargs="*",
            help="JSON file(s) and/or C++ header files, wildcards are allowed")
    argparser.add_argument("-d",
            "--docs",
            dest="docs",
            action="store_true",
            default=False,
            help="generate documentation")
    argparser.add_argument("-c",
            "--code",
            dest="code",
            action="store_true",
            default=False,
            help="generate C++ code building JSON classes and complete JSON-RPC functionality (the latter only if applicable)")
    argparser.add_argument("-s",
            "--stubs",
            dest="stubs",
            action="store_true",
            default=False,
            help="generate C++ stub code for JSON-RPC (i.e. J*.j header file to fill in manually)")
    argparser.add_argument("-o",
            "--output",
            dest="output_dir",
            metavar="DIR",
            action="store",
            default=None,
            help="output directory, absolute path or directory relative to output file (default: output in the same directory as the source file)")
    argparser.add_argument(
            "--force",
            dest="force",
            action="store_true",
            default=False,
            help= "force code generation even if destination appears up-to-date (default: force disabled)")

    json_group = argparser.add_argument_group("JSON parser arguments (optional)")
    json_group.add_argument("-i",
            dest="if_dir",
            metavar="DIR",
            action="store",
            type=str,
            default=None,
            help=
            "a directory with JSON API interfaces that will substitute the {interfacedir} tag (default: same directory as source file)")
    json_group.add_argument("--no-ref-names",
            dest="no_ref_names",
            action="store_true",
            default=False,
            help="do not derive class names from $refs (default: derive class names from $ref)")
    json_group.add_argument("--no-duplicates-warnings",
            dest="no_duplicates_warnings",
            action="store_true",
            default=not SHOW_WARNINGS,
            help="suppress duplicate object warnings (default: show all duplicate object warnings)")

    cpp_group = argparser.add_argument_group("C++ parser arguments (optional)")
    cpp_group.add_argument("-j",
            dest="cppif_dir",
            metavar="DIR",
            action="store",
            type=str,
            default=None,
            help=
            "a directory with C++ API interfaces that will substitute the {cppinterfacedir} tag (default: same directory as source file)")
    cpp_group.add_argument('-I',
            dest="includePaths",
            metavar="INCLUDE_DIR",
            action='append',
            default=[],
            type=str,
            help='add an include path (can be used multiple times)')
    cpp_group.add_argument("--include",
            dest="extra_include",
            metavar="FILE",
            action="store",
            default=DEFAULT_DEFINITIONS_FILE,
            help="include a C++ header file with common types (default: include '%s')" % DEFAULT_DEFINITIONS_FILE)
    cpp_group.add_argument("--namespace",
            dest="if_namespace",
            metavar="NS",
            type=str,
            action="store",
            default=INTERFACE_NAMESPACE,
            help="set namespace to look for interfaces in (default: %s)" % INTERFACE_NAMESPACE)
    cpp_group.add_argument("--format",
            dest="format",
            type=str,
            action="store",
            default="flexible",
            choices=["default-compliant", "force-compliant", "default-uncompliant-extended", "force-uncompliant-extended", "default-uncompliant-collapsed", "force-uncompliant-collapsed"],
            help="select JSON-RPC data format (default: default-compliant)")

    data_group = argparser.add_argument_group("C++ output arguments (optional)")
    data_group.add_argument("-p",
            "--data-path",
            dest="if_path",
            metavar="PATH",
            action="store",
            type=str,
            default=JSON_INTERFACE_PATH,
            help="relative path for #include'ing JsonData header file (default: 'interfaces/json', '.' for no path)")
    data_group.add_argument(
            "--no-includes",
            dest="no_includes",
            action="store_true",
            default=False,
            help="do not emit #includes (default: include data and interface headers)")
    data_group.add_argument("--copy-ctor",
            dest="copy_ctor",
            action="store_true",
            default=False,
            help="always emit a copy constructor and assignment operator (default: emit only when needed)")
    data_group.add_argument("--def-int-size",
            dest="def_int_size",
            metavar="SIZE",
            type=int,
            action="store",
            default=DEFAULT_INT_SIZE,
            help="default integer size in bits (default: %i)" % DEFAULT_INT_SIZE)
    data_group.add_argument("--indent",
            dest="indent_size",
            metavar="SIZE",
            type=int,
            action="store",
            default=INDENT_SIZE,
            help="code indentation in spaces (default: %i)" % INDENT_SIZE)

    doc_group = argparser.add_argument_group("Documentation output arguments (optional)")
    doc_group.add_argument("--no-style-warnings",
            dest="no_style_warnings",
            action="store_true",
            default=not DOC_ISSUES,
            help="suppress documentation issues (default: show all documentation issues)")
    doc_group.add_argument("--no-interfaces-section",
            dest="no_interfaces_section",
            action="store_true",
            default=False,
            help="do not include 'Interfaces' section (default: include interface section)")
    doc_group.add_argument("--source-location",
            dest="source_location",
            metavar="LN",
            type=str,
            action="store",
            default=INTERFACE_SOURCE_LOCATION,
            help="override interface source file location to the link specified")
    doc_group.add_argument("--source-revision",
            dest="source_revision",
            metavar="ID",
            type=str,
            action="store",
            default=None,
            help="override interface source file revision to the commit id specified")

    ts_group = argparser.add_argument_group("Troubleshooting arguments (optional)")
    ts_group.add_argument("--verbose",
            dest="verbose",
            action="store_true",
            default=False,
            help="enable verbose logging")
    ts_group.add_argument("--keep-empty",
            dest="keep_empty",
            action="store_true",
            default=False,
            help="keep generated files that have no code")
    ts_group.add_argument("--dump-json",
            dest="dump_json",
            action="store_true",
            default=False,
            help="dump the intermediate JSON file created while parsing a C++ header")


    args = argparser.parse_args(cmdline[1:])

    DOC_ISSUES = not args.no_style_warnings
    NO_DUP_WARNINGS = args.no_duplicates_warnings
    INDENT_SIZE = args.indent_size
    ALWAYS_EMIT_COPY_CTOR = args.copy_ctor
    KEEP_EMPTY = args.keep_empty
    CLASSNAME_FROM_REF = not args.no_ref_names
    DEFAULT_INT_SIZE = args.def_int_size
    DUMP_JSON = args.dump_json
    FORCE = args.force
    DEFAULT_DEFINITIONS_FILE = args.extra_include
    INTERFACE_NAMESPACE = "::" + args.if_namespace if args.if_namespace.find("::") != 0 else args.if_namespace
    INTERFACES_SECTION = not args.no_interfaces_section
    INTERFACE_SOURCE_LOCATION = args.source_location
    INTERFACE_SOURCE_REVISION = args.source_revision

    if RpcFormat.EXTENDED.value in args.format:
        RPC_FORMAT = RpcFormat.EXTENDED
    elif RpcFormat.COLLAPSED.value in args.format:
        RPC_FORMAT = RpcFormat.COLLAPSED
    else:
        RPC_FORMAT = RpcFormat.COMPLIANT

    if "force" in args.format:
        RPC_FORMAT_FORCED = True

    NO_INCLUDES = args.no_includes

    if args.if_path and args.if_path != ".":
        JSON_INTERFACE_PATH = args.if_path
    JSON_INTERFACE_PATH = posixpath.normpath(JSON_INTERFACE_PATH) + os.sep


    if args.if_dir:
        args.if_dir = os.path.abspath(os.path.normpath(args.if_dir))
    if args.cppif_dir:
        args.cppif_dir = os.path.abspath(os.path.normpath(args.cppif_dir))

    return argparser, args
