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

def IsObjectRestricted(argument):
    if "range" in argument.schema:
        return True
    elif isinstance(argument, JsonObject):
        for prop in argument.properties:
            if "range" in prop.schema:
                return True
    return False

def IsObjectOptionalOrOpaque(argument):
        _by_required = ("required" in argument.parent.schema and argument.json_name not in argument.parent.schema["required"])
        return (argument.schema.get("opaque") or argument.schema.get("@optional") or _by_required) and not argument.optional

def IsObjectOptional(argument):
    if argument.optional or IsObjectOptionalOrOpaque(argument):
        return True
    elif isinstance(argument, JsonObject):
        for prop in argument.properties:
            if not prop.optional and not IsObjectOptional(prop):
                return False
        return True
    return False

class Restrictions:
    def __init__(self, test_set=True, reverse=False, json=True):
        self.__test_set = test_set
        self.__reverse = reverse
        self.__comp = ['<', '>', "false" ] if not reverse else ['>=', '<=', "true"]
        self.__json = json
        self.reset()

    def reset(self):
        self.__cond = []

    def present(self):
        return (len(self.__cond) > 0)

    def count(self):
        return len(self.__cond)

    def join(self):
        if (len(self.__cond) == 1):
            return self.__cond[0]
        elif (len(self.__cond) > 1):
            return (" && ".join(self.__cond))
        else:
            return self.__comp[2]

    def extend(self, element, pos=None):
        self.__cond.insert(pos if pos else len(self.__cond), element)

    def append(self, argument, relay=None, override=None, test_set=None):
        if test_set == None:
            test_set = self.__test_set

        if not relay:
            relay = argument

        name = relay.temp_name if not override else override

        if isinstance(relay, JsonObject) and self.__json and test_set:
            if IsObjectOptional(relay):
                if self.__reverse:
                    self.__cond.append("((%s.IsSet() == false) || (%s.IsDataValid() == true))" % (name, name))
                else:
                    self.__cond.append("((%s.IsSet() == true) && (%s.IsDataValid() == false))" % (name, name))
            else:
                if self.__reverse:
                    self.__cond.append("((%s.IsSet() == true) && (%s.IsDataValid() == true))" % (name, name))
                else:
                    self.__cond.append("((%s.IsSet() == false) || (%s.IsDataValid() == false))" % (name, name))

        elif IsObjectRestricted(argument):
            tests = []

            range = argument.schema.get("range")

            if isinstance(relay, JsonString):
                if range:
                    if range[0] != 0:
                        tests.append("(%s%s.size() %s %s)" % (name, (".Value()" if self.__json else ""), self.__comp[0], range[0]))

                    tests.append("(%s%s.size() %s %s)" % (name, (".Value()" if self.__json else ""), self.__comp[1], range[1]))

            elif not isinstance(relay, JsonObject):
                if range:
                    if range[0] or relay.schema.get("signed"):
                        tests.append("(%s %s %s)" % (name, self.__comp[0], range[0]))

                    tests.append("(%s %s %s)" % (name, self.__comp[1], range[1]))

            if tests:
                if test_set and self.__json:
                    if IsObjectOptional(argument):
                        if self.__reverse:
                            self.__cond.append("((%s.IsSet() == false) || (%s))" % (name, " &&".join(tests)))
                        else:
                            self.__cond.append("((%s.IsSet() == true) && (%s))" % (name, " || ".join(tests)))
                    else:
                        if self.__reverse:
                            self.__cond.append("((%s.IsSet() == true) && (%s))" % (name, " && ".join(tests)))
                        else:
                            self.__cond.append("((%s.IsSet() == false) || (%s))" % (name, " || ".join(tests)))
                else:
                    self.__cond.extend(tests)

        elif test_set and self.__json and not IsObjectOptional(relay):
            self.__cond.append("(%s.IsSet() == %s)" % (name, self.__comp[2]))

def ProcessEnums(log, action=None):
    count = 0

    for obj in trackers.enum_tracker.objects:
        if not obj.is_duplicate and not obj.included_from and ("@register" not in obj.schema or obj.schema["@register"]):
            obj.schema["@register"] = False
            count += 1
            if action:
                action(log, obj)

    return count

def EmitEnumRegs(log, root, emit, header_file, if_file):
    def _EmitEnumRegistration(log, enum):
        name = enum.original_type if enum.original_type else (Scoped(root, enum) + enum.cpp_class)

        log.Info("Emitting enum conversion table for '{}'".format(name))

        emit.Line()
        emit.Line("ENUM_CONVERSION_BEGIN(%s)" % name)
        emit.Indent()

        for i, _ in enumerate(enum.enumerators):
            emit.Line("{ %s::%s, _TXT(\"%s\") }," % (name, enum.cpp_enumerators[i], enum.enumerators[i]))

        emit.Unindent()
        emit.Line("ENUM_CONVERSION_END(%s)" % name)

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

    count = ProcessEnums(log, _EmitEnumRegistration)

    emit.Line()
    emit.Line("}")

    return count

def EmitObjects(log, root, emit, if_file, additional_includes, emitCommon = False):
    global emittedItems
    emittedItems = 0

    def _EmitEnumConversionHandler(root, enum):
        name = enum.original_type if enum.original_type else (Scoped(root, enum) + enum.cpp_class)
        emit.Line("ENUM_CONVERSION_HANDLER(%s)" % name)

    def _EmitEnum(enum):
        global emittedItems
        emittedItems += 1

        log.Info("Emitting enum '{}'".format(enum.cpp_class))

        if enum.description:
            emit.Line("// " + enum.description.split("\n", 1)[0])

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
        def __EmitInit(json_obj):
            for prop in json_obj.properties:
                emit.Line("Add(%s, &%s);" % (Tstring(prop.json_name), prop.cpp_name))
                if isinstance(prop, JsonString) and prop.schema.get("opaque"):
                    emit.Line("%s.SetQuoted(false);" % prop.cpp_name)

        def __EmitAssignment(json_obj, other, optional_type=False, conv=False):
            if optional_type:
                emit.Line("if (%s.IsSet() == true) {" % other)
                emit.Indent()

            if optional_type:
                other += ".Value()"

            for prop in json_obj.properties:
                optional_or_opaque = IsObjectOptionalOrOpaque(prop)
                _prop_name = prop.actual_name if conv else prop.cpp_name

                if (prop.optional and not prop.default_value):
                    emit.Line("if (%s.%s.IsSet() == true) {" % (other, prop.actual_name))
                    emit.Indent()

                elif optional_or_opaque:
                    if isinstance(prop, JsonString):
                        emit.Line("if (%s.%s%s.empty() == false) {" % (other, _prop_name, ".Value()" if not conv else ""))
                        emit.Indent()
                    elif isinstance(prop, JsonInteger):
                        emit.Line("if (%s.%s != 0) {" % (other, _prop_name))
                        emit.Indent()
                    else:
                        optional_or_opaque = False # invalid @optional...

                emit.Line("%s = %s.%s;" % (prop.cpp_name, other, _prop_name + prop.convert_rhs))

                if (prop.optional and not prop.default_value) or optional_or_opaque:
                    emit.Unindent()
                    emit.Line("}")

            if optional_type:
                emit.Unindent()
                emit.Line("}")

        def __EmitCtor(json_obj, no_init_code=False, copy_ctor=False, conversion_ctor=False, optional_type=False):
            _other = "_other"

            if copy_ctor:
                assert not optional_type
                emit.Line("%s(const %s& %s)" % (json_obj.cpp_class, json_obj.cpp_class, _other))
            elif conversion_ctor:
                emit.Line("%s(const %s& %s)" % (json_obj.cpp_class, json_obj.cpp_native_type_opt_v(optional_type), _other))
            else:
                assert not optional_type
                emit.Line("%s()" % (json_obj.cpp_class))

            emit.Indent()
            emit.Line(": %s()" % CoreJson("Container"))

            for prop in json_obj.properties:
                if copy_ctor:
                    emit.Line(", %s(%s.%s)" % (prop.cpp_name, _other, prop.cpp_name))
                elif prop.default_value:
                    emit.Line(", %s(%s)" % (prop.cpp_name, prop.default_value))

            emit.Unindent()
            emit.Line("{")
            emit.Indent()

            if conversion_ctor:
                __EmitAssignment(json_obj, _other, optional_type, conv=True)

            if no_init_code:
                emit.Line("_Init();")
            else:
                __EmitInit(json_obj)

            emit.Unindent()
            emit.Line("}")

        def _EmitCtor(json_obj, no_init=False):
            __EmitCtor(json_obj, no_init)

        def _EmitCopyCtor(json_obj, optional_type=False):
            __EmitCtor(json_obj, True, True, False, optional_type)

        def _EmitConvertCtor(json_obj, optional_type=False):
            __EmitCtor(json_obj, True, False, True, optional_type)

        def __EmitAssignmentOperator(json_obj, copy=False, conversion=False, optional_type=False):
            _other = "_rhs"

            if copy:
                assert not optional_type
                emit.Line("%s& operator=(const %s& %s)" % (json_obj.cpp_class, json_obj.cpp_class, _other))
            elif conversion:
                emit.Line("%s& operator=(const %s& %s)" % (json_obj.cpp_class, json_obj.cpp_native_type_opt_v(optional_type), _other))
            else:
                assert False

            emit.Line("{")
            emit.Indent()

            __EmitAssignment(json_obj, _other, optional_type, conversion)

            emit.Line("return (*this);")
            emit.Unindent()
            emit.Line("}")

        def _EmitCopyAssignmentOperator(json_obj, optional_type=False):
            __EmitAssignmentOperator(json_obj, True, False, optional_type)

        def _EmitConvertAssignmentOperator(json_obj, optional_type=False):
            __EmitAssignmentOperator(json_obj, False, True, optional_type)

        def _EmitConversionOperator(json_obj):
            emit.Line("operator %s() const" % (json_obj.cpp_native_type))
            emit.Line("{")
            emit.Indent();
            emit.Line("%s _value{};" % (json_obj.cpp_native_type))

            for prop in json_obj.properties:
                if (prop.optional and not prop.default_value):
                    emit.Line("if (%s.IsSet() == true) {" % (prop.cpp_name))
                    emit.Indent()

                conv = (prop.convert if prop.convert else "%s = %s")
                emit.Line((conv + ";") % ( ("_value." + prop.actual_name), prop.cpp_name))

                if (prop.optional and not prop.default_value):
                    emit.Unindent()
                    emit.Line("}")

            emit.Line("return (_value);")
            emit.Unindent()
            emit.Line("}")

        def _EmitValidator(json_obj):
            emit.Line()
            emit.Line("bool IsDataValid() const")
            emit.Line("{")
            emit.Indent()

            restrictions = Restrictions(test_set=True, reverse=True)

            for prop in json_obj.properties:
                restrictions.append(prop, override=prop.cpp_name)

            emit.Line("return (%s);" % restrictions.join())

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

            _EmitCtor(json_obj, (json_obj.is_copy_ctor_needed or json_obj.original_type))

            if json_obj.is_copy_ctor_needed:
                emit.Line()
                _EmitCopyCtor(json_obj)
                emit.Line()
                _EmitCopyAssignmentOperator(json_obj)

            if json_obj.original_type:
                emit.Line()
                _EmitConvertCtor(json_obj)

                if json_obj.optional:
                    emit.Line()
                    _EmitConvertCtor(json_obj, optional_type=True)

                emit.Line()
                _EmitConvertAssignmentOperator(json_obj)

                if json_obj.optional:
                    emit.Line()
                    _EmitConvertAssignmentOperator(json_obj, optional_type=True)

                emit.Line()
                _EmitConversionOperator(json_obj)

            method = json_obj

            while not isinstance(method, JsonMethod):
                method = method.parent

            _EmitValidator(json_obj)

            if json_obj.is_copy_ctor_needed or json_obj.original_type:
                emit.Unindent()
                emit.Line()
                emit.Line("private:")
                emit.Indent()
                emit.Line("void _Init()")
                emit.Line("{")
                emit.Indent()
                __EmitInit(json_obj)
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
                comment = prop.print_name if isinstance(prop, JsonMethod) else prop.description.split("\n",1)[0] if prop.description else ""
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
        log.Info("Emitting common enums...")
        emittedPrologue = False
        for obj in trackers.enum_tracker.CommonObjects():
            if obj.do_create and not obj.is_duplicate and not obj.included_from:
                if not emittedPrologue:
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
