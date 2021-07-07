#!/usr/bin/env python3

# If not stated otherwise in this file or this component's LICENSE file the
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

from . import CppParser

CLASS_IUNKNOWN = "::WPEFramework::Core::IUnknown"


class Interface():
    def __init__(self, obj, iid, file):
        self.obj = obj
        self.id = iid
        self.file = file


# Looks for interface clasess (ie. classes inheriting from Core::Unknown and specifying ID enum).
def FindInterfaceClasses(tree, interface_namespace, source_file):
    interfaces = []

    def __Traverse(tree, faces):
        if isinstance(tree, CppParser.Namespace) or isinstance(tree, CppParser.Class):
            for c in tree.classes:
                if not isinstance(c, CppParser.TemplateClass) and c.methods:
                    if (interface_namespace + "::") in c.full_name:
                        inherits_iunknown = False
                        for a in c.ancestors:
                            if CLASS_IUNKNOWN in str(a[0]):
                                inherits_iunknown = True
                                break

                        if inherits_iunknown:
                            for e in c.enums:
                                if not e.scoped:
                                    for item in e.items:
                                        if item.name == "ID":
                                            faces.append(Interface(c, item.value, source_file))
                                            break
                __Traverse(c, faces)

        if isinstance(tree, CppParser.Namespace):
            for n in tree.namespaces:
                __Traverse(n, faces)

    __Traverse(tree, interfaces)
    return interfaces
