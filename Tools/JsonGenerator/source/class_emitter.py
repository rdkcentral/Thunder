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
from json_loader import *

def EmitEnumRegs(root, emit, header_file, if_file):
    def _EmitEnumRegistration(root, enum):
        name = enum.original_type if enum.original_type else (Scoped(root, enum) + enum.cpp_class)

        emit.Line("ENUM_CONVERSION_BEGIN(%s)" % name)
        emit.Indent()

        for i, _ in enumerate(enum.enumerators):
            emit.Line("{ %s::%s, _TXT(\"%s\") }," % (name, enum.cpp_enumerators[i], enum.enumerators[i]))

        emit.Unindent()
        emit.Line("ENUM_CONVERSION_END(%s);" % name)

    emit.Line()
    emit.Line("// Enumeration code for %s JSON-RPC API." % root.info["title"].replace("Plugin", "").strip())
    emit.Line("// Generated automatically from '%s'." % os.path.basename(if_file))
    emit.Line()

    # Enumeration conversion code
    emit.Line("#include <core/Enumerate.h>")
    emit.Line()
    emit.Line("#include \"definitions.h\"")

    if not config.NO_INCLUDES:
        if if_file.endswith(".h"):
            emit.Line("#include <%s%s>" % (config.CPP_INTERFACE_PATH, if_file))

    emit.Line("#include \"%s_%s.h\"" % (config.DATA_NAMESPACE, header_file))
    emit.Line()
    emit.Line("namespace %s {" % config.FRAMEWORK_NAMESPACE)
    count = 0

    for obj in trackers.enum_tracker.objects:
        if not obj.is_duplicate and not obj.included_from:
            emit.Line()
            _EmitEnumRegistration(root, obj)
            count += 1

    emit.Line()
    emit.Line("}")

    return count


def EmitObjects(log, root, emit, if_file, additional_includes, emitCommon = False):
    global emittedItems
    emittedItems = 0

    def _EmitEnumConversionHandler(root, enum):
        name = enum.original_type if enum.original_type else (Scoped(root, enum) + enum.cpp_class)
        emit.Line("ENUM_CONVERSION_HANDLER(%s);" % name)

    def _EmitEnum(enum):
        global emittedItems
        emittedItems += 1

        log.Info("Emitting enum {}".format(enum.cpp_class))

        if enum.description:
            emit.Line("// " + enum.description)

        emit.Line("enum%s %s : uint%i_t {" % (" class" if enum.is_scoped else "", enum.cpp_class, enum.size))
        emit.Indent()

        for c, item in enumerate(enum.cpp_enumerators):
            emit.Line("%s%s%s" % (item.upper(),
                                  (" = " + str(enum.cpp_enumerator_values[c])) if enum.cpp_enumerator_values else "",
                                  "," if not c == len(enum.cpp_enumerators) - 1 else ""))

        emit.Unindent()
        emit.Line("};")
        emit.Line()

    def _EmitClass(json_obj, allow_duplicates=False):
        def EmitInit(json_object):
            for prop in json_obj.properties:
                emit.Line("Add(%s, &%s);" % (Tstring(prop.json_name), prop.cpp_name))

        def EmitCtor(json_obj, no_init_code=False, copy_ctor=False, conversion_ctor=False):
            if copy_ctor:
                emit.Line("%s(const %s& _other)" % (json_obj.cpp_class, json_obj.cpp_class))
            elif conversion_ctor:
                emit.Line("%s(const %s& _other)" % (json_obj.cpp_class, json_obj.cpp_native_type))
            else:
                emit.Line("%s()" % (json_obj.cpp_class))

            emit.Indent()
            emit.Line(": %s()" % CoreJson("Container"))
            for prop in json_obj.properties:
                if copy_ctor:
                    emit.Line(", %s(_other.%s)" % (prop.cpp_name, prop.cpp_name))
                elif prop.cpp_def_value != '""' and prop.cpp_def_value != "":
                    emit.Line(", %s(%s)" % (prop.cpp_name, prop.cpp_def_value))
            emit.Unindent()
            emit.Line("{")
            emit.Indent()

            if conversion_ctor:
                for prop in json_obj.properties:
                    emit.Line("%s = _other.%s;" % (prop.cpp_name, prop.actual_name))

            if no_init_code:
                emit.Line("_Init();")
            else:
                EmitInit(json_obj)

            emit.Unindent()
            emit.Line("}")

        def _EmitAssignmentOperator(json_obj, copy_ctor=False, conversion_ctor=False):
            if copy_ctor:
                emit.Line("%s& operator=(const %s& _rhs)" % (json_obj.cpp_class, json_obj.cpp_class))
            elif conversion_ctor:
                emit.Line("%s& operator=(const %s& _rhs)" % (json_obj.cpp_class, json_obj.cpp_native_type))

            emit.Line("{")
            emit.Indent()

            for prop in json_obj.properties:
                if copy_ctor:
                    emit.Line("%s = _rhs.%s;" % (prop.cpp_name, prop.cpp_name))
                elif conversion_ctor:
                    emit.Line("%s = _rhs.%s;" % (prop.cpp_name, prop.actual_name))

            emit.Line("return (*this);")
            emit.Unindent()
            emit.Line("}")

        def _EmitConversionOperator(json_obj):
            emit.Line("operator %s() const" % (json_obj.cpp_native_type))
            emit.Line("{")
            emit.Indent();
            emit.Line("%s _value{};" % (json_obj.cpp_native_type))

            for prop in json_obj.properties:
                emit.Line("_value.%s = %s;" % ( prop.actual_name, prop.cpp_name))

            emit.Line("return (_value);")
            emit.Unindent()
            emit.Line("}")

        # Bail out if a duplicated class!
        if isinstance(json_obj, JsonObject) and not json_obj.properties:
            return

        if json_obj.is_duplicate or (not allow_duplicates and json_obj.RefCount() > 1):
            return

        if not isinstance(json_obj, (JsonRpcSchema, JsonMethod)):
            log.Info("Emitting class '{}' (source: '{}')".format(json_obj.cpp_class, json_obj.print_name))
            emit.Line("class %s : public %s {" % (json_obj.cpp_class, CoreJson("Container")))
            emit.Line("public:")
            for enum in json_obj.enums:
                if (enum.do_create and not enum.is_duplicate and (enum.RefCount() == 1)):
                    emit.Indent()
                    _EmitEnum(enum)
                    emit.Unindent()

            emit.Indent()
        else:
            if isinstance(json_obj, JsonMethod):
                if json_obj.included_from:
                    return

        # Handle nested classes!
        for obj in trackers.SortByDependency(json_obj.objects):
            _EmitClass(obj)

        if not isinstance(json_obj, (JsonRpcSchema, JsonMethod)):
            global emittedItems
            emittedItems += 1
            EmitCtor(json_obj, json_obj.is_copy_ctor_needed or "original_type" in json_obj.schema, False, False)
            if json_obj.is_copy_ctor_needed:
                emit.Line()
                EmitCtor(json_obj, True, True, False)
                emit.Line()
                _EmitAssignmentOperator(json_obj, True, False)

            if "original_type" in json_obj.schema:
                emit.Line()
                EmitCtor(json_obj, True, False, True)
                emit.Line()
                _EmitAssignmentOperator(json_obj, False, True)
                emit.Line()
                _EmitConversionOperator(json_obj)

            if json_obj.is_copy_ctor_needed or ("original_type" in json_obj.schema):
                emit.Unindent()
                emit.Line()
                emit.Line("private:")
                emit.Indent()
                emit.Line("void _Init()")
                emit.Line("{")
                emit.Indent()
                EmitInit(json_obj)
                emit.Unindent()
                emit.Line("}")
                emit.Line()
            else:
                emit.Line()
                emit.Line("%s(const %s&) = delete;" % (json_obj.cpp_class, json_obj.cpp_class))
                emit.Line("%s& operator=(const %s&) = delete;" % (json_obj.cpp_class, json_obj.cpp_class))
                emit.Line()

            emit.Unindent()
            emit.Line("public:")
            emit.Indent()

            for prop in json_obj.properties:
                comment = prop.print_name if isinstance(prop, JsonMethod) else prop.description
                emit.Line("%s %s;%s" % (prop.short_cpp_type, prop.cpp_name, (" // " + comment) if comment else ""))

            emit.Unindent()
            emit.Line("}; // class %s" % json_obj.cpp_class)
            emit.Line()

    count = 0

    if trackers.enum_tracker.objects:
        count = 0
        for obj in trackers.enum_tracker.objects:
            if obj.do_create and not obj.is_duplicate and not obj.included_from:
                count += 1

    emit.Line()
    emit.Line("// C++ classes for %s JSON-RPC API." % root.info["title"].replace("Plugin", "").strip())
    emit.Line("// Generated automatically from '%s'. DO NOT EDIT." % os.path.basename(if_file))
    emit.Line()
    emit.Line("// Note: This code is inherently not thread safe. If required, proper synchronisation must be added.")
    emit.Line()

    emit.Line("#pragma once")
    emit.Line()
    emit.Line("#include <core/JSON.h>")

    if not config.NO_INCLUDES:
        if if_file.endswith(".h"):
            emit.Line("#include <%s%s>" % (config.CPP_INTERFACE_PATH, if_file))

        for ai in additional_includes:
            emit.Line("#include <%s%s>" % (config.CPP_INTERFACE_PATH, os.path.basename(ai)))

    if count:
        emit.Line("#include <core/Enumerate.h>")

    emit.Line()
    emit.Line("namespace %s {" % config.FRAMEWORK_NAMESPACE)
    emit.Line()
    emit.Line("namespace %s {" % config.DATA_NAMESPACE)
    emit.Indent()
    emit.Line()

    if "info" in root.schema and "namespace" in root.schema["info"]:
        emit.Line("namespace %s {" % root.schema["info"]["namespace"])
        emit.Indent()
        emit.Line()

    emit.Line("namespace %s {" % root.json_name)
    emit.Indent()
    emit.Line()

    if emitCommon and trackers.enum_tracker.CommonObjects():
        emittedPrologue = False
        for obj in trackers.enum_tracker.CommonObjects():
            if obj.do_create and not obj.is_duplicate and not obj.included_from:
                if not emittedPrologue:
                    log.Info("Emitting common enums...")
                    emit.Line("// Common enums")
                    emit.Line("//")
                    emit.Line()
                    emittedPrologue = True
                _EmitEnum(obj)

    if emitCommon and trackers.object_tracker.CommonObjects():
        log.Info("Emitting common classes...")
        emittedPrologue = False
        for obj in trackers.object_tracker.CommonObjects():
            if not obj.included_from:
                if not emittedPrologue:
                    emit.Line("// Common classes")
                    emit.Line("//")
                    emit.Line()
                    emittedPrologue = True
                _EmitClass(obj, True)

    if root.objects:
        log.Info("Emitting params/result classes...")
        emit.Line("// Method params/result classes")
        emit.Line("//")
        emit.Line()
        _EmitClass(root)

    emit.Unindent()
    emit.Line("} // namespace %s" % root.json_name)
    emit.Line()

    if "info" in root.schema and "namespace" in root.schema["info"]:
        emit.Unindent()
        emit.Line("} // namespace %s" % root.schema["info"]["namespace"])
        emit.Line()

    emit.Unindent()
    emit.Line("} // namespace %s" % config.DATA_NAMESPACE)
    emit.Line()
    emittedPrologue = False

    for obj in trackers.enum_tracker.objects:
        if not obj.is_duplicate and not obj.included_from:
            if not emittedPrologue:
                emit.Line("// Enum conversion handlers")
                emittedPrologue = True

            _EmitEnumConversionHandler(root, obj)
            emittedItems += 1

    emit.Line()
    emit.Line("}")
    emit.Line()

    return emittedItems
