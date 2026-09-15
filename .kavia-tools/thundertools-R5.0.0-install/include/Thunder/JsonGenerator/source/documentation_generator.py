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
import json
from collections import OrderedDict

import config
import rpc_version
from emitter import Emitter


class DocumentationError(RuntimeError):
    pass

def Create(log, schema, path, indent_size = 4):
    input_basename = os.path.basename(path)
    output_path = os.path.dirname(path) + os.sep + input_basename.replace(".json", "") + ".md"

    with Emitter(output_path, config.INDENT_SIZE, 10000) as emit:
        def bold(string):
            return "**%s**" % string

        def italics(string):
            return "*%s*" % string

        def link(string, caption=None):
            return "[%s](#%s)" % (caption if caption else string.split(".", 1)[1].replace("_", " "), string)

        def weblink(string, link):
            return "[%s](%s)" % (string,link)

        def MdBr():
            emit.Line()

        def MdHeader(string, level=1, id="head", section=None):
            if level < 3:
                emit.Line("<a name=\"%s\"></a>" % (id + "." + string.replace(" ", "_")))

            if id != "head":
                string += " [<sup>%s</sup>](#head.%s)" % (id, section)

            emit.Line("%s %s" % ("#" * level, "*%s*" % string if id != "head" else string))
            MdBr()

        def MdBody(string=""):
            emit.Line(string)

        def MdParagraph(string):
            MdBody(string)
            MdBr()

        def MdCode(string, lang):
            emit.Line("```" + lang)
            emit.Line(string)
            emit.Line("```")
            emit.Line()

        def MdRow(cols):
            row = "|"
            for c in cols:
                row += " %s |" % c
            emit.Line(row)

        def MdTableHeader(columns):
            MdRow(columns)
            MdRow([":--------"] * len(columns))

        def ParamTable(name, object):
            MdTableHeader(["Name", "Type", "Description"])

            def _TableObj(name, obj, parentName="", parent=None, prefix="", parentOptional=False):
                # determine if the attribute is optional
                optional = parentOptional or (obj["optional"] if "optional" in obj else False)
                deprecated = obj["deprecated"] if "deprecated" in obj else False
                obsolete = obj["obsolete"] if "obsolete" in obj else False
                restricted = obj.get("range")

                if parent and not optional:
                    if parent["type"] == "object":
                        optional = ("required" not in parent and len(parent["properties"]) > 1) or (
                            "required" in parent
                            and name not in parent["required"]) or ("required" in parent and len(parent["required"]) == 0)

                # include information about enum values in description
                enum = ""
                if "enum" in obj and "ids" in obj:
                    enums = []
                    endmarker = obj.get("@endmarker")
                    for i,e in enumerate(obj["ids"]):
                        if e == endmarker:
                            break;
                        enums.append(obj["enum"][i])

                    if enums:
                        enum = ' (must be one of the following: *%s*)' % (", ".join(sorted(enums)))

                if parent and prefix and parent["type"] == "object":
                    prefix += "?." if optional else "."

                prefix += name
                description = obj["description"] if "description" in obj else obj["summary"] if "summary" in obj else "*...*"
                if description and description[0].islower():
                    description = description[0].upper() + description[1:]

                if name or prefix:
                    if "type" not in obj:
                        raise DocumentationError("'%s': missing 'type' for this object" % (parentName + "/" + name))

                    d = obj
                    is_buffer = False

                    if obj["type"] == "string":
                        if "range" in obj:
                            d = obj
                        elif "length" in obj:
                            is_buffer = True
                            if "range" in parent["properties"][obj.get("length")]:
                                d = parent["properties"][obj.get("length")]

                    restricted = "range" in d

                    row = (("<sup>" + italics("(optional)") + "</sup>" + " ") if optional else "")

                    if deprecated:
                        row = "<sup>" + italics("(deprecated)") + "</sup> " + row

                    if obsolete:
                        row = "<sup>" + italics("(obsolete)") + "</sup> " + row

                    row += description + enum

                    if row.endswith('.'):
                        row = row[:-1]

                    if "default" in obj:
                        row += " (default: " + (italics("%s") % str(obj["default"]) + ")")

                    if obj["type"] == "number":
                        # correct number to integer
                        if ("float" not in obj or not obj["float"]) and ("example" not in obj or '.' not in str(obj["example"])):
                            obj["type"] = "integer"

                    if restricted:
                        if row:
                            row += "<br>"

                        if d["type"] == "string" or is_buffer:
                            str_text = "Decoded data" if d.get("encode") else "String"
                            if d["range"][0]:
                                row += italics("%s length must be in range [%s..%s] bytes." % (str_text, d["range"][0], d["range"][1]))
                            else:
                                row += italics("%s length must be at most %s bytes." % (str_text, d["range"][1]))
                        else:
                            row += italics("Value must be in range [%s..%s]." % (d["range"][0], d["range"][1]))

                    if obj.get("@extract"):
                        row += " " + italics("(if only one element is present then the array will be omitted)")

                    MdRow([prefix, "opaque object" if obj.get("opaque") else "string (base64)" if obj.get("encode") else obj["type"], row])

                if obj["type"] == "object":
                    if "required" not in obj and name and len(obj["properties"]) > 1:
                        log.Warn("'%s': no 'required' field present (assuming all members optional)" % name)

                    for pname, props in obj["properties"].items():
                        _TableObj(pname, props, parentName + "/" + name, obj, prefix, False)
                elif obj["type"] == "array":
                    _TableObj("", obj["items"], parentName + "/" + name, obj, (prefix + "[#]") if name else "", optional)

            _TableObj(name, object, "")
            MdBr()

        def ErrorTable(obj):
            MdTableHeader(["Message", "Description"])

            for err in obj:
                description = err["description"] if "description" in err else ""
                MdRow(["```" + err["message"] + "```", description])

            MdBr()

        def PlainTable(obj, columns, ref="ref"):
            MdTableHeader(columns)

            for prop, val in sorted(obj.items()):
                MdRow(["<a name=\"%s.%s\">%s</a>" % (ref, (prop.split("]", 1)[0][1:]) if "]" in prop else prop, prop), val])

            MdBr()

        def ExampleObj(name, obj, root=False):
            if "deprecated" in obj and obj["deprecated"]:
                return "$deprecated"

            obj_type = obj["type"]
            default = obj["example"] if "example" in obj else obj["default"] if "default" in obj else ""

            if not default and "enum" in obj:
                default = obj["enum"][0]

            json_data = '"%s": ' % name if name else ''

            if obj_type == "string":
                json_data += "{  }" if obj.get("opaque") else ('"%s"' % (default if default else "..."))
            elif obj_type == "integer":
                if default and not str(default).lstrip('-+').isnumeric():
                    raise DocumentationError("'%s': invalid example syntax for this integer type (see '%s')" % (name, default))

                json_data += '%s' % (default if default else 0)
            elif obj_type == "number":
                if default and not str(default).replace('.','').lstrip('-+').isnumeric():
                    raise DocumentationError("'%s': invalid example syntax for this numeric (floating-point) type (see '%s')" % (name, default))

                if default and '.' not in str(default):
                    default = default * 1.0

                json_data += '%s' % (default if default else 0.0)
            elif obj_type == "boolean":
                json_data += '%s' % str(default if default else False).lower()
            elif obj_type == "null":
                json_data += 'null'
            elif obj_type == "instanceid":
                json_data += default if default else '"0x..."'
            elif obj_type == "array":
                json_data += str(default if default else ('[ %s ]' % (ExampleObj("", obj["items"]))))
            elif obj_type == "object":
                json_data += "{ %s }" % ", ".join(
                    list(map(lambda p: ExampleObj(p, obj["properties"][p]),
                             obj["properties"]))[0:obj["maxProperties"] if "maxProperties" in obj else None])
                json_data = json_data.replace("$deprecated, ", "")

            return json_data

        def MethodDump(method, props, classname, section, header, is_notification=False, is_property=False, include=None):
            method = (method.rsplit(".", 1)[1] if "." in method else method)
            type = "property" if is_property else "notification" if is_notification else "method"

            log.Info("Emitting documentation for %s '%s'..." % (type, method))

            element = ["property", "properties"] if is_property else ["method", "methods"]
            sen1 = " This %s is **deprecated** and may be removed in the future. It is not recommended for use in new implementations."
            sen2 = " This %s is **obsolete**. It is not recommended for use in new implementations."
            main_status = sen2 if props.get("obsolete") else sen1 if props.get("deprecated") else ""
            alt_status = ""
            is_alt = "alt" in props

            if is_notification:
                element = ["notification", "events"]

            if is_alt:
                if config.LEGACY_ALT:
                    alt_status = sen2 if interface[element[1]][props["alt"]].get("obsolete") else sen1 if interface[element[1]][props["alt"]].get("deprecated") else ""
                else:
                    alt_status = sen2 if props.get("altisobsolete") else sen1 if props.get("altisdeprecated") else ""

            orig_method = method
            method = props["alt"] if main_status and not alt_status and "alt" in props else method
            orig_method2 = method

            MdHeader(method, 2, type, section)

            readonly = False
            writeonly = False

            if "summary" in props:
                text = props["summary"]
                if is_property:
                    text = "Provides access to the " + \
                        (text[0].lower() if text[1].islower()
                         else text[0]) + text[1:]

                if not text.endswith('.'):
                    text += '.'

                MdParagraph(text)

            if is_property:
                if "readonly" in props and props["readonly"]:
                    MdParagraph("> This property is **read-only**.")
                    readonly = True
                elif "writeonly" in props and props["writeonly"]:
                    writeonly = True
                    MdParagraph("> This property is **write-only**.")


            if alt_status == main_status:
                if main_status:
                    MdParagraph("> %s" % (main_status % element[0]))
                if is_alt:
                    MdParagraph("> ``%s`` is an alternative name for this %s." % (props.get("alt"), element[0]))
            else:
                if main_status and not alt_status:
                    MdParagraph("> ``%s`` is an alternative name for this %s.%s" % (orig_method, element[0], main_status % "name"))
                elif alt_status and not main_status:
                    MdParagraph("> ``%s`` is an alternative name for this %s.%s" % (props.get("alt"), element[0], alt_status % "name"))
                else:
                    MdParagraph("> %s." % main_status % element[0])
                    MdParagraph("> ``%s`` is an alternative name for this %s.%s" % (props.get("alt"), element[0], alt_status % "name"))

            if "description" in props:
                MdHeader("Description", 3)
                MdParagraph(props["description"])

            if "statuslistener" in props:
                MdParagraph("> If applicable, this notification may be sent out during registration, reflecting the current status.")

            if "events" in props:
                events = [props["events"]] if isinstance(props["events"], str) else props["events"]
                MdParagraph("Also see: " + (", ".join(map(lambda x: link("event." + x), events))))

            if "@lookup" in props:
                method = method.replace("::", "#1::")

            if is_property:
                MdHeader("Value", 3)
                if "params" in props:
                    if not "description" in props["params"]:
                        if "summary" in props:
                            props["params"]["description"] = props["summary"]

                    ParamTable("(property)", props["params"])

                if "result" in props:
                    if not "description" in props["result"]:
                        if "params" in props and "description" in props["params"]:
                            props["result"]["description"] = props["params"]["description"]
                        elif "summary" in props:
                            props["result"]["description"] = props["summary"]

                if "@lookup" in props:
                    MdParagraph("> The *%s* instance ID shell be passed within the designator, e.g. ``%s.1.%s%s``." % (props["@lookup"][2].lower(), classname, orig_method2.replace("::", "<%s>::" % props["@lookup"][2].lower()) , "@" + props["index"]["example"] if "index" in props else ""))

                if "index" in props:
                    if "name" not in props["index"] or "example" not in props["index"]:
                        raise DocumentationError("'%s': index field requires 'name' and 'example' properties" % method)

                    extra_paragraph = "> The *%s* argument shall be passed as the index to the property, e.g. ``%s.1.%s@<%s>``.%s" % (
                        props["index"]["name"].lower(), classname, method, props["index"]["name"].lower(),
                        (" " + props["index"]["description"]) if "description" in props["index"] else "")

                    if not extra_paragraph.endswith('.'):
                        extra_paragraph += '.'

                    MdParagraph(extra_paragraph)

            else:
                MdHeader("Parameters", 3)

                if "@lookup" in props:
                    MdParagraph("> The *%s* argument shell be passed within the designator, e.g. ``%s.1.%s``" % (props["@lookup"][2], classname, method))

                if "params" in props:
                    ParamTable("params", props["params"])
                else:
                    if is_notification:
                        MdParagraph("This notification carries no parameters.")
                    else:
                        MdParagraph("This method takes no parameters.")

                if is_notification:
                    if "id" in props:
                        if "name" not in props["id"] or "example" not in props["id"]:
                            raise DocumentationError("'%s': id field needs 'name' and 'example' properties" % method)

                        MdParagraph("> The *%s* argument will be passed within the designator, e.g. *%s.client.%s*." %
                                    (props["id"]["name"], props["id"]["example"], method))

            if "result" in props:
                MdHeader("Result", 3)
                ParamTable("result", props["result"])

            if not is_notification and "errors" in props:
                MdHeader("Errors", 3)
                ErrorTable(props["errors"])

            MdHeader("Example", 3)

            if is_notification:
                callmethod = "client." + method
            elif is_property:
                callmethod = "%s.1.%s%s" % (classname, method, ("@" + str(props["index"]["example"])) if "index" in props and "example" in props["index"] else "")
            else:
                callmethod = "%s.1.%s" % (classname, method)

            if "id" in props and "example" in props["id"]:
                callmethod = props["id"]["example"] + ".client." + method

            jsonError = "Failed to generate JSON example for %s" % method
            jsonResponse = jsonError
            jsonRequest = jsonError

            if is_notification:
                MdHeader("Registration", 4)

                client = "client"

                if "id" in props and "example" in props["id"]:
                    client = props["id"]["example"] + "." + client

                text = '{ "jsonrpc": "2.0", "id": 42, "method": "%s.1.register", "params": {"event": "%s", "id": "%s" } }' % (classname, method, client)
                try:
                    jsonRequest = json.dumps(json.loads(text, object_pairs_hook=OrderedDict), indent=2)
                except:
                    jsonRequest = jsonError
                    log.Error(jsonError)

                MdCode(jsonRequest, "json")

            if is_property:
                if not writeonly:
                    MdHeader("Get Request", 4)

                    text = '{ "jsonrpc": "2.0", "id": 42, "method": "%s" }' % callmethod

                    try:
                        jsonRequest = json.dumps(json.loads(text, object_pairs_hook=OrderedDict), indent=2)
                    except:
                        jsonRequest = jsonError
                        log.Error(jsonError)

                    MdCode(jsonRequest, "json")
                    MdHeader("Get Response", 4)

                    parameters = (props["result"] if "result" in props else (props["params"] if "params" in props else None))
                    text = '{ "jsonrpc": "2.0", "id": 42, %s }' % ExampleObj("result", parameters, True)

                    try:
                        jsonResponse = json.dumps(json.loads(text, object_pairs_hook=OrderedDict), indent=2)
                    except:
                        jsonResponse = jsonError
                        log.Error(jsonError)

                    MdCode(jsonResponse, "json")

            if not readonly:
                if is_property:
                    MdHeader("Set Request", 4)
                elif is_notification:
                    MdHeader("Message", 4)
                else:
                    MdHeader("Request", 4)

                try:
                    jsonRequest = json.dumps(json.loads('{ "jsonrpc": "2.0", %s"method": "%s"%s }' %
                                                        ('"id": 42, ' if not is_notification else "", callmethod,
                                                        (", " + ExampleObj("params", props["params"], True)) if "params" in props else ""),
                                                        object_pairs_hook=OrderedDict), indent=2)
                except:
                    jsonRequest = jsonError
                    log.Error(jsonError)

                MdCode(jsonRequest, "json")

                if not is_notification and not is_property:
                    if "result" not in props:
                        props["result"] = { "type": "null" }

                    MdHeader("Response", 4)
                    try:
                        jsonResponse = json.dumps(json.loads('{ "jsonrpc": "2.0", "id": 42, %s }' % ExampleObj("result", props["result"], True),
                                                    object_pairs_hook=OrderedDict), indent=2)
                    except:
                        jsonResponse = jsonError
                        log.Error(jsonError)

                    MdCode(jsonResponse, "json")

                if is_property:
                    MdHeader("Set Response", 4)
                    try:
                        jsonResponse = json.dumps(json.loads('{ "jsonrpc": "2.0", "id": 42, "result": "null" }',
                                                    object_pairs_hook=OrderedDict), indent=4)
                    except:
                        jsonResponse = jsonError
                        log.Error(jsonError)

                    MdCode(jsonResponse, "json")

            return props["alt"] if is_alt and not is_notification else ""

        commons = dict()
        with open(os.path.join(os.path.dirname(os.path.realpath(__file__)), config.GLOBAL_DEFINITIONS)) as f:
            commons = json.load(f)

        global_rpc_format = config.RPC_FORMAT
        if (not config.RPC_FORMAT_FORCED) and ("info" in schema) and ("format" in schema["info"]):
            global_rpc_format = config.RpcFormat(schema["info"]["format"])

        # The interfaces defined can be a single item, a list, or a list of list.
        # So first flatten the structure and make it consistent to be always a list.
        outer_interfaces = schema

        if "interface" in schema:
            outer_interfaces = schema["interface"]

        if not isinstance(outer_interfaces, list):
            outer_interfaces = [outer_interfaces]

        interfaces = []
        for interface in outer_interfaces:
            rpc_format = config.RPC_FORMAT

            if (not config.RPC_FORMAT_FORCED) and ("info" in interface) and ("format" in interface["info"]):
                rpc_format = config.RpcFormat(interface["info"]["format"])

            if isinstance(interface, list):
                interfaces.extend(interface)
                for face in interface:
                    if "include" in face:
                        if isinstance(face["include"], list):
                            interfaces.extend(face["include"])
                        else:
                            interfaces.append(face["include"])
                    interfaces[-1]["info"]["format"] = rpc_format.value
            else:
                interfaces.append(interface)

                if "include" in interface:
                    for face in interface["include"]:
                        interfaces.append(interface["include"][face])
                        interfaces[-1]["info"]["format"] = rpc_format.value

        # Don't consider all interfaces for processing
        interfaces = [interface for interface in interfaces if ("@generated" not in interface) or (interface.get("mode") == "auto")]

        version = "1.0" # Plugin version, not interface version

        if "info" in schema:
            info = schema["info"]
        else:
            info = dict()

        # Count the total number of methods, properties and events.
        method_count = 0
        property_count = 0
        event_count = 0

        for interface in interfaces:
            if "methods" in interface:
                method_count += len(interface["methods"])

            if "properties" in interface:
                property_count += len(interface["properties"])

            if "events" in interface:
                event_count += len(interface["events"])

        status = info["status"] if "status" in info else "alpha"

        rating = 0

        if status == "dev" or status == "development":
            rating = 0
        elif status == "alpha":
            rating = 1
        elif status == "beta":
            rating = 2
        elif status == "production" or status == "prod":
            rating = 3
        else:
            raise DocumentationError("invalid status (see '%s')" % status)

        plugin_class = None

        if "callsign" in info:
            plugin_class = info["callsign"]
        elif "class" in info:
            plugin_class = info["class"]
        else:
            raise DocumentationError("missing class in 'info'")

        def SourceRevision(face):
            sourcerevision = config.DEFAULT_INTERFACE_SOURCE_REVISION
            if config.INTERFACE_SOURCE_REVISION:
                sourcerevision = config.INTERFACE_SOURCE_REVISION
            elif "sourcerevision" in face["info"]:
                sourcerevision = face["info"]["sourcerevision"]
            elif "sourcerevision" in info:
                sourcerevision = info["sourcerevision"]
            elif "common" in face and "sourcerevision" in face["common"]:
                sourcerevision = face["common"]["sourcerevision"]
            elif "sourcerevision" in commons:
                sourcerevision = commons["sourcerevision"]
            return sourcerevision

        def SourceLocation(face):
            sourcelocation = None
            if config.INTERFACE_SOURCE_LOCATION:
                sourcelocation = config.INTERFACE_SOURCE_LOCATION
            elif "sourcelocation" in face["info"]:
                sourcelocation = face["info"]["sourcelocation"]
            elif "sourcelocation" in info:
                sourcelocation = info["sourcelocation"]
            elif "common" in face and "sourcelocation" in face["common"]:
                sourcelocation = face["common"]["sourcelocation"]
            elif "sourcelocation" in commons:
                sourcelocation = commons["sourcelocation"]
            if sourcelocation:
                sourcelocation = sourcelocation.replace("{interfacefile}", face["info"]["sourcefile"] if "sourcefile" in face["info"]
                                        else ((face["info"]["class"] if "class" in face["info"] else input_basename) + ".json"))
                sourcerevision = SourceRevision(face)
                if sourcerevision:
                    sourcelocation = sourcelocation.replace("{revision}", sourcerevision)
            return sourcelocation

        document_type = "plugin"

        if ("@interfaceonly" in schema and schema["@interfaceonly"]) or ("$schema" in schema and schema["$schema"] == "interface.schema.json"):
            document_type = "interface"
            version = rpc_version.GetVersionString(info)

        MdBody("<!-- Generated automatically, DO NOT EDIT! -->")

        # Emit title bar
        if "title" in info:
            MdHeader(info["title"])

        MdParagraph(bold("Version: " + version))
        MdParagraph(bold("Status: " + rating * ":black_circle:" + (3 - rating) * ":white_circle:"))
        MdParagraph("%s %s for Thunder framework." % (plugin_class, document_type))

        if document_type == "interface":
            face = interfaces[0]
            sourcelocation = SourceLocation(interfaces[0])
            iface = (face["info"]["interface"] if "interface" in face["info"] else (face["info"]["class"] + ".json"))

            if sourcelocation:
                if "sourcefile" in face["info"]:
                    wl = "with " + iface + " in " + weblink(face["info"]["sourcefile"], sourcelocation)
                else:
                    wl = "by " + weblink(iface, sourcelocation)
            else:
                wl = iface

            MdBody("(Defined %s)" % wl)
            MdBr()

        # Emit TOC
        MdHeader("Table of Contents", 3)
        MdBody("- " + link("head.Introduction"))

        if "description" in info:
            MdBody("- " + link("head.Description"))

        if document_type == "plugin":
            MdBody("- " + link("head.Configuration"))

        if config.INTERFACES_SECTION and (document_type == "plugin") and (method_count or property_count or event_count):
            MdBody("- " + link("head.Interfaces"))

        if method_count:
            MdBody("- " + link("head.Methods"))

        if property_count:
            MdBody("- " + link("head.Properties"))

        if event_count:
            MdBody("- " + link("head.Notifications"))

        MdBr()

        def mergedict(d1, d2, prop):
            tmp = dict()

            if prop in d1:
                tmp.update(d1[prop])

            if prop in d2:
                tmp.update(d2[prop])

            return tmp

        MdHeader("Introduction")
        MdHeader("Scope", 2)

        if "scope" in info:
            MdParagraph(info["scope"])
        elif "title" in info:
            extra = ""

            if document_type == 'plugin':
                extra = "configuration"

            if method_count and property_count and event_count:
                if extra:
                    extra += ", "
                extra += "methods and properties as well as sent notifications"
            elif method_count and property_count:
                if extra:
                    extra += ", "
                extra += "methods and properties provided"
            elif method_count and event_count:
                if extra:
                    extra += ", "
                extra += "methods provided and notifications sent"
            elif property_count and event_count:
                if extra:
                    extra += ", "
                extra += "properties provided and notifications sent"
            elif method_count:
                if extra:
                    extra += " and "
                extra += "methods provided"
            elif property_count:
                if extra:
                    extra += " and "
                extra += "properties provided"
            elif event_count:
                if extra:
                    extra += " and "
                extra += "notifications sent"
            if extra:
                extra = " It includes detailed specification about its " + extra + "."

            MdParagraph("This document describes purpose and functionality of the %s %s%s.%s" %
                (plugin_class, document_type, (" (version %s)" % version if document_type == "interface" else ""), extra))

        MdHeader("Case Sensitivity", 2)
        MdParagraph((
            "All identifiers of the interfaces described in this document are case-sensitive. "
            "Thus, unless stated otherwise, all keywords, entities, properties, relations and actions should be treated as such."
        ))

        if "acronyms" in info or "acronyms" in commons or "terms" in info or "terms" in commons:
            MdHeader("Acronyms, Abbreviations and Terms", 2)

            if "acronyms" in info or "acronyms" in commons:
                MdParagraph("The table below provides and overview of acronyms used in this document and their definitions.")
                PlainTable(mergedict(commons, info, "acronyms"), ["Acronym", "Description"], "acronym")

            if "terms" in info or "terms" in commons:
                MdParagraph("The table below provides and overview of terms and abbreviations used in this document and their definitions.")
                PlainTable(mergedict(commons, info, "terms"), ["Term", "Description"], "term")

        if "standards" in info:
            MdHeader("Standards", 2)
            MdParagraph(info["standards"])

        if "references" in commons or "references" in info:
            MdHeader("References", 2)
            PlainTable(mergedict(commons, info, "references"), ["Ref ID", "Description"])

        if "description" in info:
            MdHeader("Description")
            description = " ".join(info["description"]) if isinstance(info["description"], list) else info["description"]

            if not description.endswith('.'):
                description += '.'

            MdParagraph(description)

            if document_type == "plugin":
                MdParagraph(("The plugin is designed to be loaded and executed within the Thunder framework. "
                            "For more information about the framework refer to [[Thunder](#ref.Thunder)]."))

        if document_type == "plugin":
            MdHeader("Configuration")
            commonConfig = OrderedDict()

            if "configuration" in schema and "nodefault" in schema["configuration"] and schema["configuration"]["nodefault"] and "properties" not in schema["configuration"]:
                MdParagraph("The plugin does not take any configuration.")
            else:
                MdParagraph("The table below lists configuration options of the plugin.")
                if "configuration" not in schema or ("nodefault" not in schema["configuration"] or not schema["configuration"]["nodefault"]):
                    if "callsign" in info:
                        commonConfig["callsign"] = {
                            "type": "string",
                            "description": 'Plugin instance name (default: *%s*)' % info["callsign"]
                        }

                    if plugin_class:
                        commonConfig["classname"] = {"type": "string", "description": 'Class name: *%s*' % plugin_class}

                    if "locator" in info:
                        commonConfig["locator"] = {"type": "string", "description": 'Library name: *%s*' % info["locator"]}

                    commonConfig["startmode"] = {
                        "type": "string",
                        "description": "Determines in which state the plugin should be moved to at startup of the framework"
                    }

                required = []

                if "configuration" in schema:
                    commonConfig2 = OrderedDict(list(commonConfig.items()) + list(schema["configuration"]["properties"].items()))
                    required = schema["configuration"]["required"] if "required" in schema["configuration"] else []
                else:
                    commonConfig2 = commonConfig

                totalConfig = OrderedDict()
                totalConfig["type"] = "object"
                totalConfig["properties"] = commonConfig2

                if "configuration" not in schema or ("nodefault" not in schema["configuration"] or not schema["configuration"]["nodefault"]):
                    totalConfig["required"] = ["callsign", "classname", "locator", "startmode"] + required

                ParamTable("", totalConfig)

            if config.INTERFACES_SECTION and (document_type == "plugin") and (method_count or property_count or event_count):
                MdHeader("Interfaces")
                MdParagraph("This plugin implements the following interfaces:")

                for face in interfaces:
                    iface = (face["info"]["interface"] if "interface" in face["info"] else (face["info"]["class"] + ".json"))
                    sourcelocation = SourceLocation(face)

                    if sourcelocation:
                        if "sourcefile" in face["info"]:
                            wl = iface + " (" + weblink(face["info"]["sourcefile"], sourcelocation) + ")"
                        else:
                            wl = weblink(iface, sourcelocation)
                    else:
                        wl = iface

                    format = face["info"]["format"] if "format" in face["info"] else global_rpc_format.value
                    MdBody("- %s (version %s) (%s format)" % (wl, rpc_version.GetVersionString(face["info"]), format))

                MdBr()

        def SectionDump(section_name, section, header, description=None, description2=None, event=False, prop=False):
            skip_list = []

            def InterfaceDump(interface, section, header):
                head = False
                emitted = False

                if section in interface:
                    for method, contents in interface[section].items():
                        if contents and method not in skip_list and "#" not in method:
                            ns = interface["info"]["namespace"] if "namespace" in interface["info"] else ""

                            if not head:
                                MdParagraph("%s interface %s:" % (((ns + " ") if ns else "") + interface["info"]["class"], section))
                                MdTableHeader([header.capitalize(), "Description"])
                                head = True

                            access = ""

                            if "readonly" in contents and contents["readonly"] == True:
                                access = "read-only"
                            elif "writeonly" in contents and contents["writeonly"] == True:
                                access = "write-only"

                            if access:
                                access = " (%s)" % access

                            tags = ""

                            if "obsolete" in contents and contents["obsolete"]:
                                tags += " <sup>obsolete</sup>"

                            elif "deprecated" in contents and contents["deprecated"]:
                                tags += " <sup>deprecated</sup>"

                            descr = ""

                            if "summary" in contents:
                                descr = contents["summary"]

                                if "e.g" in descr:
                                    descr = descr[0:descr.index("e.g") - 1]

                                if "i.e" in descr:
                                    descr = descr[0:descr.index("i.e") - 1]

                                descr = descr.split(".", 1)[0] if "." in descr else descr

                            ln = header + "." + (method.rsplit(".", 1)[1] if "." in method else method)
                            line = link(ln)

                            if "alt" in contents and contents["alt"]:
                                alt_tags = ""

                                if config.LEGACY_ALT:
                                    if interface[section][contents["alt"]].get("obsolete"):
                                        alt_tags += " <sup>obsolete</sup>"
                                    elif interface[section][contents["alt"]].get("deprecated"):
                                        alt_tags += " <sup>deprecated</sup>"
                                else:
                                    if contents.get("altisobsolete"):
                                        alt_tags += " <sup>obsolete</sup>"
                                    elif contents.get("altisdeprecated"):
                                        alt_tags += " <sup>deprecated</sup>"

                                if not alt_tags and tags:
                                    line = link(header + "." + contents["alt"])
                                    line += " / " + link(header + "." + contents["alt"], method.rsplit(".", 1)[1] if "." in method else method)
                                elif alt_tags and alt_tags == tags:
                                    line += " / " + link(ln, contents["alt"]) + tags
                                else:
                                    line += " / " + link(ln, contents["alt"])

                                skip_list.append(contents["alt"])
                            else:
                                line += tags

                            line += access

                            MdRow([line, descr])
                            emitted = True

                        skip_list.append(method)

                    if emitted:
                        MdBr()

            MdHeader(section_name)

            if description:
                MdParagraph(description)

            MdParagraph("The following %s are provided by the %s %s:" % (section, plugin_class, document_type))

            for interface in interfaces:
                InterfaceDump(interface, section, header)

            MdBr()

            if description2:
                MdParagraph(description2)
                MdBr()

            skip_list = []

            for interface in interfaces:
                if section in interface:
                    for method, props in interface[section].items():
                        if props and method not in skip_list and "#" not in method:
                            to_skip = MethodDump(method, props, plugin_class, section_name, header, event, prop)

                            if to_skip:
                                skip_list.append(to_skip)

                        skip_list.append(method)

        if method_count:
            SectionDump("Methods", "methods", "method")

        if property_count:
            SectionDump("Properties", "properties", "property", prop=True)

        if event_count:
            SectionDump("Notifications",
                        "events",
                        "notification",
                        ("Notifications are autonomous events triggered by the internals of the implementation "
                         "and broadcasted via JSON-RPC to all registered observers. "
                         "Refer to [[Thunder](#ref.Thunder)] for information on how to register for a notification."),
                        event=True)

        log.Success("Document created: %s" % output_path)
