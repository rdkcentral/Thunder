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

import config
import logger
from json_loader import *

log = None

def SortByDependency(objects):
    sorted_objects = []

    # This will order objects by their relations
    for obj in sorted(objects, key=lambda x: x.cpp_class, reverse=False):
        found = filter(lambda sorted_obj: obj.cpp_class in map(lambda x: x.cpp_class, sorted_obj.objects), sorted_objects)
        try:
            index = min(map(lambda x: sorted_objects.index(x), found))
            movelist = filter(lambda x: x.cpp_class in map(lambda x: x.cpp_class, sorted_objects), obj.objects)
            sorted_objects.insert(index, obj)

            for m in movelist:
                if m in sorted_objects:
                    sorted_objects.insert(index, sorted_objects.pop(sorted_objects.index(m)))
        except ValueError:
            sorted_objects.append(obj)

    return sorted_objects

def IsInRef(obj):
    while obj:
        if "@ref" in obj.schema:
            return True
        obj = obj.parent

    return False

def IsInCustomRef(obj):
    while obj:
        if "@ref" in obj.schema and obj.schema["@ref"] == "#../params":
            return True
        obj = obj.parent

    return False

class ObjectTracker:
    def __init__(self):
        self.objects = []
        self.Reset()

    def Add(self, newObj):
        def _CompareObject(lhs, rhs):
            def _CompareType(lhs, rhs):
                if rhs["type"] != lhs["type"]:
                    return False

                if "size" in lhs:
                    if "size" in rhs:
                        if lhs["size"] != rhs["size"]:
                            return False
                    elif "size" != 32:
                        return False
                elif "size" in rhs:
                    if rhs["size"] != 32:
                        return False

                if "signed" in lhs:
                    if "signed" in rhs:
                        if lhs["signed"] != rhs["signed"]:
                            return False
                    elif lhs["signed"] != False:
                        return False
                elif "signed" in rhs:
                    if rhs["signed"] != False:
                        return False

                if "enum" in lhs:
                    if "enum" in rhs:
                        if lhs["enum"] != rhs["enum"]:
                            return False
                    else:
                        return False

                    if "enumvalues" in lhs:
                        if "enumvalues" in rhs:
                            if lhs["enumvalues"] != rhs["enumvalues"]:
                                return False
                        else:
                            return False
                    elif "enumvalues" in rhs:
                        return False

                    if "values" in lhs:
                        if "values" in rhs:
                            if lhs["values"] != rhs["values"]:
                                return False
                        else:
                            return False
                    elif "values" in rhs:
                        return False

                    if "enumids" in lhs:
                        if "enumids" in rhs:
                            if lhs["enumids"] != rhs["enumids"]:
                                return False
                        else:
                            return False
                    elif "enumids" in rhs:
                        return False

                    if "ids" in lhs:
                        if "ids" in rhs:
                            if lhs["ids"] != rhs["ids"]:
                                return False
                        else:
                            return False
                    elif "ids" in rhs:
                        return False
                elif "enum" in rhs:
                    return False

                if "items" in lhs:
                    if "items" in rhs:
                        if not _CompareType(lhs["items"], rhs["items"]):
                            return False
                    else:
                        return False
                elif "items" in rhs:
                    return False

                if "properties" in lhs:
                    if "properties" in rhs:
                        if not _CompareObject(lhs["properties"], rhs["properties"]):
                            return False
                    else:
                        return False

                return True

            # NOTE: Two objects are considered identical if they have the same property names and types only!
            for name, prop in lhs.items():
                if name not in rhs:
                    return False
                else:
                    if not _CompareType(prop, rhs[name]):
                        return False

            for name, prop in rhs.items():
                if name not in lhs:
                    return False
                else:
                    if not _CompareType(prop, lhs[name]):
                        return False

            return True

        if "properties" in newObj.schema and not isinstance(newObj, JsonMethod):
            self.objects.append(newObj)
            is_ref = IsInRef(newObj)
            props = newObj.schema["properties"]
            for obj in self.objects[:-1]:
                if _CompareObject(obj.schema["properties"], props):
                    if not config.GENERATED_JSON and (not is_ref and not IsInRef(obj)):
                        warning = "'%s': duplicate object (same as '%s') - consider using $ref" % (newObj.print_name, obj.print_name)

                        if log:
                            if not config.NO_DUP_WARNINGS:
                                log.Warn(warning)
                            else:
                                log.Info(warning)

                    return obj

        return None

    def Remove(self, obj):
        self.objects.remove(obj)

    def Reset(self):
        self.objects = []

    def CommonObjects(self):
        return SortByDependency(filter(lambda obj: obj.RefCount() > 1, self.objects))


class EnumTracker(ObjectTracker):
    def _IsTopmost(self, obj):
        while isinstance(obj.parent, JsonArray):
            obj = obj.parent

        return isinstance(obj.parent, JsonMethod)

    def Add(self, newObj):
        def __Compare(lhs, rhs):
            # NOTE: Two enums are considered identical if they have the same enumeration names and types
            if (lhs["enum"] == rhs["enum"]) and (lhs["type"] == rhs["type"]):
                if "enumvalues" in lhs:
                    if "enumvalues" not in rhs:
                        return False
                    else:
                        return lhs["enumvalues"] == rhs["enumvalues"]
                elif "enumvalues" in rhs:
                    return False

                if "enumids" in lhs:
                    if "enumids" not in rhs:
                        return False
                    else:
                        return lhs["enumids"] == rhs["enumids"]
                elif "enumids" in rhs:
                    return False

                if "values" in lhs:
                    if "values" not in rhs:
                        return False
                    else:
                        return lhs["values"] == rhs["values"]
                elif "values" in rhs:
                    return False

                if "ids" in lhs:
                    if "ids" not in rhs:
                        return False
                    else:
                        return lhs["ids"] == rhs["ids"]
                elif "ids" in rhs:
                    return False

                return True
            else:
                return False

        if "enum" in newObj.schema and not isinstance(newObj, JsonMethod):
            self.objects.append(newObj)
            is_ref = IsInRef(newObj)
            for obj in self.objects[:-1]:
                if __Compare(obj.schema, newObj.schema):
                    if not config.GENERATED_JSON and (not is_ref and not IsInRef(obj)):
                        warning = "'%s': duplicate enums (same as '%s') - consider using $ref" % (newObj.print_name, obj.print_name)

                        if not config.NO_DUP_WARNINGS:
                            log.Warn(warning)
                        else:
                            log.Info(warning)

                    return obj

        return None

    def CommonObjects(self):
        return SortByDependency(filter(lambda obj: ((obj.RefCount() > 1) or self._IsTopmost(obj)), self.objects))

def SetLogger(logger):
    global log
    log = logger

object_tracker = ObjectTracker()
enum_tracker = EnumTracker()
