#!/usr/bin/env python

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
        if isinstance(tree, CppParser.Namespace) or isinstance(
                tree, CppParser.Class):
            for c in tree.classes:
                if c.methods:
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
                                            faces.append(
                                                Interface(
                                                    c, item.value, source_file))
                                            break
                __Traverse(c, faces)

        if isinstance(tree, CppParser.Namespace):
            for n in tree.namespaces:
                __Traverse(n, faces)

    __Traverse(tree, interfaces)
    return interfaces
