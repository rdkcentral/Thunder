#!/usr/bin/env python

import argparse, sys, re, os, json, posixpath, urllib, glob
from collections import OrderedDict

VERSION="1.3.6"

class Trace:
    def __init__(self):
        self.errors = 0
    def __Print(self, text):
        print text
    def Header(self, text):
        self.__Print(text)
    def Warn(self, text):
         self.__Print("Warning: " + text)
    def Error(self, text):
        self.errors += 1
        self.__Print("Error: " + text)
    def Success(self, text):
        self.__Print("Success: " + text)

trace = Trace()

try:
    import jsonref
except:
    trace.Error("Install jsonref first")
    print "e.g. try 'pip install jsonref'"
    sys.exit(1)

INDENT_SIZE = 4
VERIFY = True
ALWAYS_COPYCTOR = False
KEEP_EMPTY = False
CLASSNAME_FROM_REF = True
DEFAULT_EMPTY_STRING = ""
DEFAULT_INT_SIZE = 32
IF_PATH = "interfaces/json"

GLOBAL_DEFINITIONS = "global.json"
FRAMEWORK_NAMESPACE = "WPEFramework"
DATA_NAMESPACE = "JsonData"
PLUGIN_NAMESPACE = "Plugin"
TYPE_PREFIX = "Core::JSON"
OBJECT_SUFFIX = "Data"
COMMON_OBJECT_SUFFIX = "Info"
ARRAY_SUFFIX = "Array"
ENUM_SUFFIX = "Type"
IMPL_ENDPOINT_PREFIX = "endpoint_"
IMPL_EVENT_PREFIX = "event_"

#
# JSON SCHEMA PARSING
#

class JsonParseError(RuntimeError):
    pass

def TypePrefix(type):
    return (TYPE_PREFIX + "::" + type)
def MakeObject(type):
    return (type + OBJECT_SUFFIX)
def MakeArray(type):
    return (type + ARRAY_SUFFIX)
def MakeEnum(type):
    return (type + ENUM_SUFFIX)
class JsonType():
    def __init__(self, name, parent, schema, included=None):
        self.name = name
        self.schema = schema
        self.duplicate = False
        self.parent = parent
        self.description = None
        self.default = None
        self.included_from = included

        if "description" in schema:
            self.description = schema["description"]
        # BUG: Due to a bug in JsonRef, need to pick up the description from the original JSON
        if isinstance(schema, jsonref.JsonRef) and "description" in schema.__reference__:
            self.description = schema.__reference__["description"]
        # do some sanity check on the description text
        if VERIFY:
            if self.name.endswith(" "):
                trace.Warn("Item '%s' name ends with a whitespace" % self.name)
            if self.description and not isinstance(self, JsonMethod):
                if self.description.endswith("."):
                    trace.Warn("Item '%s' description ends with a dot (\"%s\")" % (self.name, self.description))
                if self.description.endswith(" "):
                    trace.Warn("Item '%s' description ends with a whitespace" % self.name)
                if not self.description[0].isupper() and self.description[0].isalpha():
                    trace.Warn("Item '%s' description does not start with a capital letter (\"%s\")" % (self.name, self.description))
        if "default" in schema:
            self.default = schema["default"]
    def IsDuplicate(self):      # Whether this object is a duplicate of another
        return self.duplicate
    def Schema(self):           # Get the original schema
        return self.schema
    def Description(self):      # Item description
        return self.description
    def JsonName(self):         # Name as in JSON
        return self.name
    def Properties(self):       # Class attributes
        return []
    def Objects(self):          # Class aggregate objects
        return []
    def CppType(self):          # C++ type of the object (e.g. may be array)
        return self.CppClass()
    def CppClass(self):         # C++ class type of the object
        raise RuntimeError("can't instantiate %s" % self.name)
    def CppName(self):          # C++ name of the object
        return self.name[0].upper() + self.name[1:]
    def CppDefValue(self):      # Value to instantiate with in C++
        return ""
    def NeedsCopyCtor(self):    # Whether a copy constructor would be needed if this type is a member of a class
        return False

class JsonNull(JsonType):
    def CppDefValue(self):
        return "nullptr"
    def CppType(self):
        return "void"

class JsonBoolean(JsonType):
    #def CppDefValue(self):
    #    return "false"
    def CppClass(self):
        return TypePrefix("Boolean")
    def CppStdClass(self):
        return "bool"

class JsonNumber(JsonType):
    def __init__(self, name, parent, schema):
        JsonType.__init__(self, name, parent, schema)
        self.size = DEFAULT_INT_SIZE
        self.signed = False
        # NOTE: Take a hint on the size and signedness of the number/integer
        if "size" in schema:
            self.size = schema["size"]
        if "signed" in schema:
            self.signed = schema["signed"]
    #def CppDefValue(self):
    #    return "0"
    def CppClass(self):
        return TypePrefix("Dec%sInt%i" % ("S" if self.signed else "U", self.size))
    def CppStdClass(self):
        return "%sint%i_t" % ("" if self.signed else "u", self.size)

class JsonInteger(JsonNumber):
    pass # Identical as Number

class JsonString(JsonType):
    def CppClass(self):
        return TypePrefix("String")
    def CppStdClass(self):
        return "string"

class JsonEnum(JsonType):
    def __init__(self, name, parent, schema, enumType, included=None):
        JsonType.__init__(self, name, parent, schema, included)
        if enumType != "string":
            raise JsonParseError("Only strings are supported in enums")
        self.type = enumType
        self.enumName = MakeEnum(self.name.capitalize())
        self.enumerators = schema["enum"]
        self.values = schema["enumvalues"] if "enumvalues" in schema else []
        if self.values and (len(self.enumerators) != len(self.values)):
            raise JsonParseError("Mismatch in enumeration values in enum '%s'" % self.JsonName())
        self.strongly_typed = schema["enumtyped"] if "enumtyped" in schema else True
        self.default = self.CppClass()+"::"+self.CppEnumerators()[0]
        self.duplicate = False
        self.origRef = None
        self.refs = []
        self.AddRef(self)
        obj = enumTracker.Add(self)
        if obj:
            self.duplicate = True
            self.origRef = obj
            if obj.parent != parent:
                obj.AddRef(self)
    def CppType(self):
        return TypePrefix("EnumType<%s>" % self.CppClass())
    def CppClass(self):
        if self.IsDuplicate():
            # Use the original (ie. first seen) name
            return self.origRef.CppClass()
        else:
            classname = ""
            if "class" in self.schema:
                # Override class name if "class" property present
                classname = self.schema["class"].capitalize()
            elif CLASSNAME_FROM_REF and isinstance(self.schema, jsonref.JsonRef):
                # NOTE: Abuse the ref feature to construct a name for the enum!
                classname = MakeEnum(self.schema.__reference__["$ref"].rsplit(posixpath.sep,1)[1].capitalize())
            elif isinstance(self.parent, JsonProperty):
                classname = MakeEnum(self.parent.name.capitalize())
            else:
                classname = MakeEnum(self.CppName())
            return classname
    def CppEnumerators(self):
        return map(lambda x: ("E" if x[0].isdigit() else "") + x.upper(), self.enumerators)
    def CppEnumeratorValues(self):
        return self.values
    def IsDuplicate(self):
        return self.duplicate
    def OrigName(self):
        return self.JsonName()
    def AddRef(self, obj):
        self.refs.append(obj)
    def RefCount(self):
        return len(self.refs)
    def CppStdClass(self):
        return self.CppClass()
    def IsStronglyTyped(self):
        return self.strongly_typed

class JsonObject(JsonType):
    def __init__(self, name, parent, schema, origName=None, included=None):
        self.origName = ((parent.JsonName() + ".") if parent.JsonName() else "") + name
        JsonType.__init__(self, name, parent, schema, included)
        self.properties = []
        self.objects = []
        self.enums = []
        self.duplicate = False
        self.origRef = None
        self.refs = []
        self.AddRef(self)
        # Handle duplicate objects...
        if "properties" in schema:
            obj = objTracker.Add(self)
            if obj:
                self.duplicate = True
                self.origRef = obj
                if obj.parent != parent:
                    obj.AddRef(self)
            for prop_name, prop in schema["properties"].iteritems():
                newObject = JsonItem(prop_name, self, prop, included=included)
                self.properties.append(newObject)
                # Handle aggregate objects
                if isinstance(newObject, JsonObject):
                    self.objects.append(newObject)
                elif isinstance(newObject, JsonArray):
                    # Also add nested objects within arrays
                    o = newObject.Items()
                    while isinstance(o, JsonArray):
                        o = o.Items()
                    if isinstance(o, JsonObject):
                        self.objects.append(o)
                    if isinstance(o, JsonEnum):
                        self.enums.append(newObject.Items())
                elif isinstance(newObject, JsonEnum):
                    self.enums.append(newObject)
        if not self.Properties():
            trace.Warn("No properties in object %s" % self.origName)
    def CppName(self):
        # NOTE: Special cases for names for Methods and Arrays
        if isinstance(self.parent, JsonMethod):
            return self.parent.CppName() + JsonType.CppName(self)
        elif isinstance(self.parent, JsonArray):
            return self.parent.CppName()
        else:
            return JsonType.CppName(self)
    def CppClass(self):
        if self.IsDuplicate():
            # Use the original (ie. first seen) class name
            return self.origRef.CppClass()
        else:
            classname = ""
            if "class" in self.schema:
                # Override class name if "class" property present
                classname = self.schema["class"].capitalize()
            else:
                if not self.properties:
                    return TypePrefix("Container")
                elif CLASSNAME_FROM_REF and isinstance(self.schema, jsonref.JsonRef):
                    # NOTE: Abuse the ref feature to construct a name for the class!
                    classname = MakeObject(self.schema.__reference__["$ref"].rsplit(posixpath.sep,1)[1].capitalize())
                else:
                    # Make the name out of properties, but not for params/result types
                    if len(self.Properties()) == 1 and not isinstance(self.parent, JsonMethod):
                        classname = MakeObject(self.Properties()[0].CppName())
                    elif isinstance(self.parent, JsonProperty):
                        classname = MakeObject(self.parent.CppName())
                    elif self.parent.parent and isinstance(self.parent.parent, JsonProperty):
                        classname = MakeObject(self.parent.parent.CppName())
                    else:
                        classname = MakeObject(self.CppName())
            # For common classes append special suffix
            if self.RefCount() > 1:
                classname = classname.replace(OBJECT_SUFFIX, COMMON_OBJECT_SUFFIX)
            return classname
    def JsonName(self):
        return self.name
    def Objects(self):
        return self.objects
    def Enums(self):
        return self.enums
    def Properties(self):
        return self.properties
    def NeedsCopyCtor(self):
        # Check if a copy constructory is needed by scanning all duplicate classes
        return ALWAYS_COPYCTOR or self.parent.NeedsCopyCtor() or filter(lambda obj: obj.parent.NeedsCopyCtor() if self != obj else False, self.refs)
    def AddRef(self, obj):
        self.refs.append(obj)
    def RefCount(self):
        return len(self.refs)
    def IsDuplicate(self):
        return self.duplicate
    def OrigName(self):
        return self.origName
    def CppStdClass(self):
        return TypePrefix("Container")

class JsonArray(JsonType):
    def __init__(self, name, parent, schema, origName=None, included=None):
        JsonType.__init__(self, name, parent, schema, included)
        self.items = None
        if "items" in schema:
            self.items = JsonItem(name, self, schema["items"], origName, included)
        else:
            raise JsonParseError("no items in array '%s'" % name)
    def CppName(self):
        # Take the name of the array from the method if this array is result type
        if isinstance(self.parent, JsonMethod):
            return self.parent.CppName() + JsonType.CppName(self)
        else:
            return JsonType.CppName(self)
    def CppType(self):
        return TypePrefix("ArrayType<%s>" % self.items.CppType())
    def NeedsCopyCtor(self):
        # Important, blame it on the arrays!
        return True
    def Items(self):
        return self.items
    # Delegate all other methods to the underlying type
    def CppClass(self):
        return self.Items().CppClass()
    def Enums(self):
        return self.Items().Enums()
    def Objects(self):
        return self.Items().Objects()
    def Properties(self):
        return self.Items().Properties()
    def IsDuplicate(self):
        return self.Items().IsDuplicate()
    def RefCount(self):
        return self.Items().RefCount()
    def CppStdClass(self):
        return "/* TODO */"

class JsonMethod(JsonObject):
    def __init__(self, name, parent, schema, included = None):
        objName = name.rsplit(".", 1)[1] if "." in name else name
        # Mimic a JSON object to fit rest of the parsing...
        self.errors = schema["errors"] if "errors" in schema else OrderedDict()
        newschema = { "type": "object" }
        props = OrderedDict()
        props["params"] = schema["params"] if "params" in schema else { "type": "null" }
        props["result"] = schema["result"] if "result" in schema else { "type": "null" }
        newschema["properties"] = props
        JsonObject.__init__(self, objName, parent, newschema, included=included)
        self.summary = None
        self.tags = []
        if "summary" in schema:
            self.summary = schema["summary"]
        if "tags" in schema:
            self.tags = schema["tags"]
    def Errors(self):
        return self.errors
    def MethodName(self):
        return IMPL_ENDPOINT_PREFIX + JsonObject.JsonName(self)
    def Summary(self):
        return self.summary

class JsonNotification(JsonMethod):
    def __init__(self, name, parent, schema, included = None):
        JsonMethod.__init__(self, name, parent, schema, included)
        self.sendif = "id" in schema
        self.statuslistener = schema["statuslistener"] if "statuslistener" in schema else False
    def HasSendif(self):
        return self.sendif
    def StatusListener(self):
        return self.statuslistener
    def MethodName(self):
        return IMPL_EVENT_PREFIX + JsonObject.JsonName(self)

class JsonProperty(JsonMethod):
    def __init__(self, name, parent, schema, included = None):
        JsonMethod.__init__(self, name, parent, schema, included)
        self.readonly = "readonly" in schema and schema["readonly"] == True
        self.writeonly = "writeonly" in schema and schema["writeonly"] == True
        self.has_index ="index" in schema
    def SetMethodName(self):
        return "set_" + JsonObject.JsonName(self)
    def GetMethodName(self):
        return "get_" + JsonObject.JsonName(self)

class JsonRpcSchema(JsonType):
    def __init__(self, name, schema):
        JsonType.__init__(self, name, None, schema)
        self.info = None
        self.base_schema = None
        self.jsonrpc_version = None
        self.methods = []
        self.includes = []
        if "$schema" in schema:
            self.base_schema = schema["$schema"]
        if "jsonrpc" in schema:
            self.jsonrpc_version = schema["jsonrpc"]
        if "info" in schema:
            self.info = schema["info"]
        if "interface" in schema:
            schema = schema["interface"]
        if "include" in schema:
            for name, s in schema["include"].iteritems():
                include = s["info"]["class"]
                self.includes.append(include)
                if "methods" in s:
                    for name, method in s["methods"].iteritems():
                        newMethod = JsonMethod(name, self, method, include)
                        self.methods.append(newMethod)
                if "events" in s:
                    for name, method in s["events"].iteritems():
                        newMethod = JsonNotification(name, self, method, include)
                        self.methods.append(newMethod)
                if "properties" in s:
                    for name, method in s["properties"].iteritems():
                        newMethod = JsonProperty(name, self, method, include)
                        self.methods.append(newMethod)

        method_list = map(lambda x: x.name, self.methods)

        def __AddMethods(section, schema, ctor):
            if section in schema:
                for name, method in schema[section].iteritems():
                    if name in method_list:
                        del self.methods[method_list.index(name)]
                        method_list.remove(name)
                    if method:
                        newMethod = ctor(name, self, method)
                        self.methods.append(newMethod)

        __AddMethods("methods", schema, lambda name, obj,  method: JsonMethod(name, obj, method))
        __AddMethods("properties", schema, lambda name, obj, method: JsonProperty(name, obj, method))
        __AddMethods("events", schema, lambda name, obj, method: JsonNotification(name, obj, method))

        if not self.methods:
            raise JsonParseError("no methods, properties or events defined in '%s'" % name)
    def CppClass(self):
        return JsonType.CppName(self)
    def Properties(self):
        return self.methods
    def Objects(self):
        return self.Properties()
    def NeedsCopyCtor(self):
        return False
    def RefCount(self):
        return 1

def JsonItem(name, parent, schema, origName=None, included=None):
    # Create the appropriate Python object based on the JSON type
    if "type" in schema:
        if schema["type"] == "object":
            return JsonObject(name, parent, schema, origName, included)
        elif schema["type"] == "array":
            return JsonArray(name, parent, schema, origName, included)
        elif schema["type"] == "null":
            return JsonNull(name, parent, schema)
        elif schema["type"] == "boolean":
            return JsonBoolean(name, parent, schema)
        elif "enum" in schema:
            return JsonEnum(name, parent, schema, schema["type"], included)
        elif schema["type"] == "string":
            return JsonString(name, parent, schema)
        elif schema["type"] == "integer":
            return JsonInteger(name, parent, schema)
        elif schema["type"] == "number":
            return JsonNumber(name, parent, schema)
        else:
            raise JsonParseError("unsupported JSON type: %s" % schema["type"])
    else:
        raise JsonParseError("undefined type for item: %s" % name)

def LoadSchema(file, include_path):
    def PreprocessJson(file, string, include_path = None):
        def __Tokenize(contents):
            # Tokenize the JSON first to be able to preprocess it easier
            formula = ( \
                    r"(/\*(.|[\r\n])*?\*/)"                                                 # multi-line comments
                    r"|(//.*)"                                                              # single line comments
                    r'|("(?:[^\\"]|\\.)*")'                                                 # double quotes
                    r"|('(?:[^\\']|\\.)*')"                                                 # singe quotes
                    r"|([~,:;?=^/*-\+&<>\{\}\(\)\[\]])"                                     # single-char operators
                    )
            tokens = [s.strip() for s in re.split(formula, contents, flags=re.MULTILINE) if s]
            # Remove comments from the JSON
            tokens = [s for s in tokens if (s and (s[:2] != '/*' and s[:2] != '//'))]
            return tokens
        path = os.path.abspath(os.path.dirname(file))
        tokens = __Tokenize(string)
        # BUG?: jsonref (urllib) needs file:// and absolute path to a ref'd file
        for c, t in enumerate(tokens):
            if t == '"$ref"' and tokens[c + 1] == ":" and tokens[c + 2][:2] != '"#':
                ref_file = tokens[c + 2].strip('"')
                ref_tok = ref_file.split("#", 1) if "#" in ref_file else [ref_file, ""]
                if "{interfacedir}/" in ref_file:
                    ref_tok[0] = ref_tok[0].replace("{interfacedir}/", (include_path + os.sep) if include_path else "")
                    if not include_path:
                        ref_tok[0] = os.path.join(path, ref_tok[0])
                else:
                    if os.path.isfile(os.path.join(path, ref_tok[0])):
                        ref_tok[0] = os.path.join(path, ref_tok[0])
                if not os.path.isfile(ref_tok[0]):
                    raise RuntimeError("$ref file '%s' not found" % ref_tok[0])
                ref_file = '"file:%s#%s"' % (urllib.pathname2url(ref_tok[0]), ref_tok[1])
                tokens[c + 2] = ref_file
        # Return back the preprocessed JSON as a string
        return " ".join(tokens)
    with open(file, "r") as json_file:
        return jsonref.loads(PreprocessJson(file, json_file.read(), include_path), object_pairs_hook=OrderedDict)

def ParseJsonRpcSchema(schema):
    objTracker.Reset()
    enumTracker.Reset()
    if "interface" in schema:
        schema = schema["interface"]
    if "info" in schema:
        if "class" in schema["info"]:
            pluginClass = schema["info"]["class"]
        else:
            pluginClass = "undefined_class"
            trace.Error("no \"class\" defined in \"info\"")
        return JsonRpcSchema(pluginClass, schema)
    else:
        return None

def SortByDependency(objects):
    sortedObjects = []
    # This will order objects by their relations
    for obj in sorted(objects, key=lambda x: x.CppClass(), reverse=False):
        found = filter(lambda sortedObj: obj.CppClass() in map(lambda x: x.CppClass(), sortedObj.Objects()), sortedObjects)
        if found:
            index = min(map(lambda x: sortedObjects.index(x), found))
            movelist = filter(lambda x: x.CppClass() in map(lambda x: x.CppClass(), sortedObjects), obj.Objects())
            sortedObjects.insert(index, obj)
            for m in movelist:
                if m in sortedObjects:
                    sortedObjects.insert(index, sortedObjects.pop(sortedObjects.index(m)))
        else:
            sortedObjects.append(obj)
    return sortedObjects

def IsInRef(obj):
    while obj:
        if (isinstance(obj.Schema(), jsonref.JsonRef)):
            return True
        obj = obj.parent
    return False

class ObjectTracker:
    def __init__(self):
        self.objects = []
        self.Reset()
    def Add(self, newObj):
        def __Compare(lhs, rhs):
            # NOTE: Two objects are considered identical if they have the same property names and types only!
            for name, prop in lhs.iteritems():
                if name not in rhs:
                    return False
                elif rhs[name]["type"] != prop["type"]:
                    return False
                elif "enum" in prop:
                    if "enum" in rhs[name]:
                        return prop["enum"] == rhs[name]["enum"]
                    else:
                        return False
            for name, prop in rhs.iteritems():
                if name not in lhs:
                    return False
                elif lhs[name]["type"] != prop["type"]:
                    return False
                elif "enum" in prop:
                    if "enum" in lhs[name]:
                        return prop["enum"] == lhs[name]["enum"]
                    else:
                        return False
            return True
        if "properties" in newObj.Schema() and not isinstance(newObj, JsonMethod):
            self.objects.append(newObj)
            is_ref = IsInRef(newObj)
            props = newObj.Schema()["properties"]
            for obj in self.Objects()[:-1]:
                if __Compare(obj.Schema()["properties"], props):
                    if not is_ref or not IsInRef(obj):
                        trace.Warn("Duplicate object '%s' (same as '%s') - consider using $ref" % (newObj.OrigName(), obj.OrigName()))
                    return obj
            return None
    def Objects(self):
        return self.objects
    def Reset(self):
        self.objects = []
    def CommonObjects(self):
        return SortByDependency(filter(lambda obj: obj.RefCount() > 1, self.Objects()))

class EnumTracker(ObjectTracker):
    def __IsTopmost(self, obj):
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
                return True
            else:
                return False
        if "enum" in newObj.Schema() and not isinstance(newObj, JsonMethod):
            self.objects.append(newObj)
            is_ref = IsInRef(newObj)
            for obj in self.Objects()[:-1]:
                if __Compare(obj.Schema(), newObj.Schema()):
                    if not is_ref or not IsInRef(obj):
                        trace.Warn("Duplicate enums '%s' (same as '%s') - consider using $ref" % (newObj.OrigName(), obj.OrigName()))
                    return obj
            return None
    def CommonObjects(self):
        return SortByDependency(filter(lambda obj: obj.RefCount() > 1 or self.__IsTopmost(obj), self.Objects()))

#
# THE EMITTER
#

class Emitter():
    def __init__(self, file, indentSize):
        self.indent_size = indentSize
        self.indent = 0
        self.file = file
    def Line(self, text = ""):
        if text != "":
            self.file.write(" " * self.indent + str(text) + "\n")
        else:
            self.file.write("\n")
    def Indent(self):
        self.indent += self.indent_size
    def Unindent(self):
        if self.indent >= self.indent_size:
            self.indent -= self.indent_size
        else:
            self.indent = 0

#
# JSON OBJECT GENERATION
#

def GetNamespace(root, obj, full = True):
    namespace = ""
    if isinstance(obj, (JsonObject, JsonEnum, JsonArray)):
        if isinstance(obj, JsonObject) and not obj.properties:
            return namespace
        fullname = ""
        e = obj
        while e.parent and not isinstance(e, JsonMethod) and (not e.IsDuplicate() or e.RefCount() > 1):
            if not isinstance(e, (JsonEnum, JsonArray)):
                fullname = e.CppClass()+ "::" + fullname
            if e.RefCount() > 1:
                break
            e = e.parent
        if full:
            namespace = "%s::%s::" % (DATA_NAMESPACE, root.CppClass())
        namespace = namespace + "%s" % fullname
    return namespace


def EmitEnumRegs(root, emit, header_file):
    def EmitEnumRegistration(root, enum, full = True):
        fullname = (GetNamespace(root, enum) if full else "%s::%s::" % (DATA_NAMESPACE, root.CppClass())) + enum.CppClass()
        emit.Line("ENUM_CONVERSION_BEGIN(%s)" % fullname)
        emit.Indent()
        for c, item in enumerate(enum.enumerators):
            emit.Line("{ %s::%s, _TXT(\"%s\") }," % (fullname, enum.CppEnumerators()[c], item))
        emit.Unindent()
        emit.Line("ENUM_CONVERSION_END(%s);" % fullname)

    # Enumeration conversion code
    emit.Line("#include \"../definitions.h\"")
    emit.Line("#include <core/Enumerate.h>")
    emit.Line("#include \"%s_%s.h\"" % (DATA_NAMESPACE, header_file))
    emit.Line()
    emit.Line("namespace %s {" % FRAMEWORK_NAMESPACE)
    count = 0
    if enumTracker.Objects():
        for obj in enumTracker.Objects():
            if not obj.IsDuplicate() and not obj.included_from:
                count += 1
        if count:
            for obj in enumTracker.Objects():
                if not obj.IsDuplicate() and not obj.included_from:
                    emit.Line()
                    EmitEnumRegistration(root, obj, obj.RefCount() == 1)
    emit.Line()
    emit.Line("}")
    return count

def EmitHelperCode(root, emit, header_file):
    if root.Objects():
        namespace = DATA_NAMESPACE + "::" + root.JsonName()

        def __NsName(obj):
            ns = DATA_NAMESPACE + "::" + (root.JsonName() if not obj.included_from else obj.included_from)
            objName = obj.CppType()
            if objName != "void":
                if not objName.startswith(TYPE_PREFIX):
                    objName = ns + "::" + objName
                p = objName.find("<", 0)
                while p != -1:
                    if not objName.startswith(TYPE_PREFIX, p+1):
                        objName = objName[:p+1] + ns + "::" + objName[p+1:]
                    p = objName.find("<", p + 1)
            return objName

        emit.Line("#include \"Module.h\"")
        emit.Line("#include \"%s.h\"" % root.JsonName())
        emit.Line("#include <%s%s>" % (IF_PATH, header_file))
        for inc in root.includes:
            emit.Line("#include <%s%s_%s.h>" % (IF_PATH, DATA_NAMESPACE, inc))
        emit.Line()

        # Registration prototypes
        has_statuslistener = False
        for method in root.Properties():
            if isinstance(method, JsonNotification) and method.StatusListener():
                has_statuslistener = True
                break

        print "Emitting registration code..."
        emit.Line("/*")
        emit.Indent()
        emit.Line("// Copy the code below to %s class definition" % root.JsonName())
        emit.Line("// Note: The %s class must inherit from PluginHost::JSONRPC%s" % (root.JsonName(), "SupportsEventStatus" if has_statuslistener else ""))
        emit.Line()
        emit.Line("private:")
        emit.Indent()
        emit.Line("void RegisterAll();")
        emit.Line("void UnregisterAll();")
        for method in root.Properties():
            if not isinstance(method, JsonProperty) and not isinstance(method, JsonNotification):
                params = __NsName(method.Properties()[0])
                response  = __NsName(method.Properties()[1])
                line = ("uint32_t %s(%s%s%s);" % (method.MethodName(), \
                                ("const " + params + "& params") if params != "void" else "", \
                                ", " if params != "void" and response != "void" else "", \
                                (response + "& response") if response != "void" else ""))
                if method.included_from:
                    line += " // %s" % method.included_from
                emit.Line(line)

        for method in root.Properties():
            if isinstance(method, JsonProperty):
                if not method.writeonly:
                    line = "uint32_t %s(%s%s& response) const;" % (method.GetMethodName(), "const string& index, " if method.has_index else "", __NsName(method.Properties()[0]))
                    if method.included_from:
                        line += " // %s" % method.included_from
                    emit.Line(line)
                if not method.readonly:
                    line = "uint32_t %s(%sconst %s& param);" % (method.SetMethodName(), "const string& index, " if method.has_index else "", __NsName(method.Properties()[0]))
                    if method.included_from:
                        line += " // %s" % method.included_from
                    emit.Line(line)

        for method in root.Properties():
            if isinstance(method, JsonNotification):
                params = __NsName(method.Properties()[0])
                par = ""
                if params != "void":
                    par = ", ".join(map(lambda x: "const " + GetNamespace(root, x) + x.CppStdClass() + "& " + x.JsonName(), method.Properties()[0].Properties()))
                line = ('void %s(%s%s);' % (method.MethodName(), "const string& id, " if method.HasSendif() else "", par))
                if method.included_from:
                    line += " // %s" % method.included_from
                emit.Line(line)

        emit.Unindent()
        emit.Unindent()
        emit.Line("*/")
        emit.Line()

        # Registration code
        emit.Line("namespace %s {" % FRAMEWORK_NAMESPACE)
        emit.Line()
        emit.Line("namespace %s {" % PLUGIN_NAMESPACE)
        emit.Indent()
        emit.Line()
        emit.Line("using namespace %s;" % namespace)
        for inc in root.includes:
            emit.Line("using namespace %s::%s;" % (DATA_NAMESPACE, inc))
        emit.Line()
        emit.Line("// Registration")
        emit.Line("//")
        emit.Line()
        emit.Line("void %s::RegisterAll()" % root.JsonName())
        emit.Line("{")
        emit.Indent()
        for method in root.Properties():
            if isinstance(method, JsonNotification) and method.StatusListener():
                emit.Line("RegisterEventStatusListener(_T(\"%s\"), [this](const string& client, Status status) {" % method.JsonName())
                emit.Indent()
                emit.Line("const string id = client.substr(0, client.find('.'));")
                emit.Line("// TODO...")
                emit.Unindent()
                emit.Line("});")
                emit.Line()
        for method in root.Properties():
            if not isinstance(method, JsonNotification) and not isinstance(method, JsonProperty):
                line = 'Register<%s,%s>(_T("%s"), &%s::%s, this);' % (method.Properties()[0].CppType(), method.Properties()[1].CppType(), method.JsonName(), root.JsonName(), method.MethodName())
                if method.included_from:
                    line += " /* %s */" % method.included_from
                emit.Line(line)
        for method in root.Properties():
            if isinstance(method, JsonProperty):
                line = 'Property<%s>(_T("%s")' % (method.Properties()[0].CppType(), method.JsonName())
                line += ", &%s::%s" % (root.JsonName(), method.GetMethodName()) if not method.writeonly else ", nullptr"
                line += ", &%s::%s" % (root.JsonName(), method.SetMethodName()) if not method.readonly else ", nullptr"
                line += ', this);'
                if method.included_from:
                    line += " /* %s */" % method.included_from
                emit.Line(line)
        emit.Unindent()
        emit.Line("}")
        emit.Line()
        emit.Line("void %s::UnregisterAll()" % root.JsonName())
        emit.Line("{")
        emit.Indent()
        for method in reversed(root.Properties()):
            if not isinstance(method, JsonNotification) and not isinstance(method, JsonProperty):
                emit.Line('Unregister(_T("%s"));' % method.JsonName())
        for method in reversed(root.Properties()):
            if isinstance(method, JsonProperty):
                emit.Line('Unregister(_T("%s"));' % method.JsonName())
        for method in reversed(root.Properties()):
            if isinstance(method, JsonNotification) and method.StatusListener():
                emit.Line("UnregisterEventStatusListener(_T(\"%s\");" % method.JsonName())

        emit.Unindent()
        emit.Line("}")
        emit.Line()

        # Method/property/event stubs
        print "Emitting stubs..."
        emit.Line("// API implementation")
        emit.Line("//")
        emit.Line()
        for method in root.Properties():
            if not isinstance(method, JsonNotification) and not isinstance(method, JsonProperty):
                print "Emitting method '%s'" % method.JsonName()
                params = method.Properties()[0].CppType()
                if method.Summary():
                    emit.Line("// Method: %s - %s" % (method.JsonName(), method.Summary().split(".",1)[0]))
                emit.Line("// Return codes:")
                emit.Line("//  - ERROR_NONE: Success")
                for e in method.Errors():
                    description = e["description"] if "description" in e else ""
                    if isinstance(e, jsonref.JsonRef) and "description" in e.__reference__:
                        description = e.__reference__["description"]
                    emit.Line("//  - %s: %s" % (e["message"], description))
                response  = method.Properties()[1].CppType()
                line = ("uint32_t %s::%s(%s%s%s)" % (root.JsonName(), method.MethodName(), \
                                ("const " + params + "& params") if params != "void" else "", \
                                ", " if params != "void" and response != "void" else "", \
                                (response + "& response") if response != "void" else ""))
                if method.included_from:
                    line += " /* %s */" % method.included_from
                emit.Line(line)
                emit.Line("{")
                emit.Indent()
                emit.Line("uint32_t result = Core::ERROR_NONE;")
                if params != "void":
                    for p in method.Properties()[0].Properties():
                        if not isinstance(p, (JsonObject, JsonArray)):
                            emit.Line("const %s& %s = params.%s.Value();" % (p.CppStdClass(), p.JsonName(), p.CppName()))
                        else:
                            emit.Line("// params.%s ..." %p.CppName())
                emit.Line()
                emit.Line("// TODO...")
                emit.Line()
                if response != "void":
                    for p in method.Properties()[1].Properties():
                        emit.Line("// response.%s = ..." % (p.CppName()))
                    emit.Line()
                emit.Line("return result;")
                emit.Unindent()
                emit.Line("}")
                emit.Line()

        for method in root.Properties():
            if isinstance(method, JsonProperty):
                def EmitPropertyFc(method, name, getter):
                    params = method.Properties()[0].CppType()
                    if method.Summary():
                        emit.Line("// Property: %s - %s" % (method.JsonName(), method.Summary().split(".",1)[0]))
                    emit.Line("// Return codes:")
                    emit.Line("//  - ERROR_NONE: Success")
                    for e in method.Errors():
                        description = e["description"] if "description" in e else ""
                        if isinstance(e, jsonref.JsonRef) and "description" in e.__reference__:
                            description = e.__reference__["description"]
                        emit.Line("//  - %s: %s" % (e["message"], description))
                    line = "uint32_t %s::%s(%s%s%s& %s)%s" % (root.JsonName(), name, "const string& index, " if method.has_index else "", "const " if not getter else "", params, "response" if getter else "param", " const" if getter else "")
                    if method.included_from:
                        line += " /* %s */" % method.included_from
                    emit.Line(line)
                    emit.Line("{")
                    emit.Indent()
                    if not getter:
                        emit.Line("uint32_t result = Core::ERROR_NONE;")
                        emit.Line()
                        emit.Line("// TODO...")
                    else:
                        emit.Line("// response = ...")
                    emit.Line()
                    emit.Line("return %s;" % ("Core::ERROR_NONE" if getter else "result"))
                    emit.Unindent()
                    emit.Line("}")
                    emit.Line()
                print "Emitting property '%s'%s" % (method.JsonName(), " (write-only)" if method.writeonly else " (read-only)" if method.readonly else "")
                if not method.writeonly:
                    EmitPropertyFc(method, method.GetMethodName(), True)
                if not method.readonly:
                    EmitPropertyFc(method, method.SetMethodName(), False)

        for method in root.Properties():
            if isinstance(method, JsonNotification):
                print "Emitting notification '%s'" % method.JsonName()
                params = method.Properties()[0].CppType()
                if method.Summary():
                    emit.Line("// Event: %s - %s" % (method.JsonName(), method.Summary().split(".",1)[0]))
                par = ", ".join(map(lambda x: "const " + GetNamespace(root, x, False) + x.CppStdClass() + "& " + x.JsonName(), method.Properties()[0].Properties()))
                line = ("void %s::%s(%s%s)" % (root.JsonName(), method.MethodName(), "const string& id, " if method.HasSendif() else "", par))
                if method.included_from:
                    line += " /* %s */" % method.included_from
                emit.Line(line)
                emit.Line("{")
                emit.Indent()
                if params != "void":
                    emit.Line("%s params;" % params)
                    for p in method.Properties()[0].Properties():
                        emit.Line("params.%s = %s;" % (p.CppName(), p.JsonName()))
                    emit.Line()
                if method.HasSendif():
                    emit.Line('Notify(_T("%s")%s, [&](const string& designator) -> bool {' % (method.JsonName(), ", params" if params != "void" else ""))
                    emit.Indent()
                    emit.Line("const string designator_id = designator.substr(0, designator.find('.'));")
                    emit.Line("return (id == designator_id);")
                    emit.Unindent()
                    emit.Line("});")
                else:
                    emit.Line('Notify(_T("%s")%s);' % (method.JsonName(), ", params" if params != "void" else ""))
                emit.Unindent()
                emit.Line("}")
                emit.Line()

        emit.Unindent()
        emit.Line("} // namespace %s" % PLUGIN_NAMESPACE)
        emit.Line()
        emit.Line("}")
        emit.Line()



def EmitObjects(root, emit, emitCommon = False):
    global emittedItems
    emittedItems = 0

    def EmitEnumConversionHandler(root, enum):
        fullname = GetNamespace(root, enum) + enum.CppClass()
        emit.Line("ENUM_CONVERSION_HANDLER(%s);" % fullname)

    def EmitEnum(enum):
        global emittedItems
        emittedItems += 1
        print "Emitting enum %s" % enum.CppClass()
        root = enum.parent.parent
        while root.parent:
            root = root.parent
        if enum.Description():
            emit.Line("// " + enum.Description())
        emit.Line("enum%s %s {" % (" class" if enum.IsStronglyTyped() else "", enum.CppClass()))
        emit.Indent()
        for c, item in enumerate(enum.CppEnumerators()):
            emit.Line("%s%s%s" % (item.upper(), (" = " + str(enum.CppEnumeratorValues()[c])) if enum.CppEnumeratorValues() else "", "," if not c == len(enum.CppEnumerators()) -1 else ""))
        emit.Unindent()
        emit.Line("};")
        emit.Line()

    def EmitClass(jsonObj, allowDup = False):
        def EmitInit(jsonObject):
            for prop in jsonObj.Properties():
                emit.Line("Add(_T(\"%s\"), &%s);" % (prop.JsonName(), prop.CppName()))

        def EmitCtor(jsonObj, noInitCode = False, copyCtor = False):
            if copyCtor:
                emit.Line("%s(const %s& other)" % (jsonObj.CppClass(), jsonObj.CppClass()))
            else:
                emit.Line("%s()" % (jsonObj.CppClass()))
            emit.Indent()
            emit.Line(": %s" % TypePrefix("Container()"))
            for prop in jsonObj.Properties():
               if copyCtor:
                    emit.Line(", %s(other.%s)" % (prop.CppName(), prop.CppName()))
               elif prop.CppDefValue() != '""' and prop.CppDefValue() != "":
                    emit.Line(", %s(%s)" % (prop.CppName(), prop.CppDefValue()))
            emit.Unindent()
            emit.Line("{")
            emit.Indent()
            if not noInitCode:
                EmitInit(jsonObj)
            else:
                emit.Line("Init();")
            emit.Unindent()
            emit.Line("}")

        # Bail out if a duplicated class!
        if isinstance(jsonObj, JsonObject) and not jsonObj.properties:
            return
        if jsonObj.IsDuplicate() or (not allowDup and jsonObj.RefCount() > 1):
            return
        if not isinstance(jsonObj, (JsonRpcSchema, JsonMethod)):
            print "Emitting class '%s' (source: '%s')" % (jsonObj.CppClass(), jsonObj.OrigName())
            emit.Line("class %s : public %s {" % (jsonObj.CppClass(), TypePrefix("Container")))
            emit.Line("public:")
            if jsonObj.Enums():
                for enum in jsonObj.Enums():
                    if not enum.IsDuplicate() and enum.RefCount() == 1:
                        emit.Indent()
                        EmitEnum(enum)
                        emit.Unindent()
            emit.Indent()
        else:
            if isinstance(jsonObj, JsonMethod):
                if jsonObj.included_from:
                    return

        # Handle nested classes!
        for obj in SortByDependency(jsonObj.Objects()):
            EmitClass(obj)

        if not isinstance(jsonObj, (JsonRpcSchema, JsonMethod)):
            global emittedItems
            emittedItems += 1
            EmitCtor(jsonObj, jsonObj.NeedsCopyCtor())
            emit.Line()
            if jsonObj.NeedsCopyCtor():
                EmitCtor(jsonObj, True, True)
                emit.Line()
                # Also emit the assignment operator
                emit.Line("%s& operator=(const %s& rhs)" % (jsonObj.CppClass(), jsonObj.CppClass()))
                emit.Line("{")
                emit.Indent()
                for prop in jsonObj.Properties():
                    emit.Line("%s = rhs.%s;" % (prop.CppName(), prop.CppName()))
                emit.Line("return (*this);")
                emit.Unindent()
                emit.Line("}")
            if jsonObj.NeedsCopyCtor():
                emit.Unindent()
                emit.Line()
                emit.Line("private:")
                emit.Indent()
                emit.Line("void Init()")
                emit.Line("{")
                emit.Indent()
                EmitInit(jsonObj)
                emit.Unindent()
                emit.Line("}")
            else:
                emit.Line("%s(const %s&) = delete;" % (jsonObj.CppClass(), jsonObj.CppClass()))
                emit.Line("%s& operator=(const %s&) = delete;" % (jsonObj.CppClass(), jsonObj.CppClass()))
            emit.Line()
            emit.Unindent()
            emit.Line("public:")
            emit.Indent()
            for prop in jsonObj.Properties():
                comment = prop.OrigName() if isinstance(prop, JsonMethod) else prop.Description()
                emit.Line("%s %s;%s" % (prop.CppType(), prop.CppName(), (" // " + comment) if comment else ""))
            emit.Unindent()
            emit.Line("}; // class %s" % jsonObj.CppClass())
            emit.Line()

    count = 0
    if enumTracker.Objects():
        count = 0
        for obj in enumTracker.Objects():
            if not obj.IsDuplicate() and not obj.included_from:
                count += 1

    emit.Line("#pragma once")
    emit.Line()
    emit.Line("#include <core/JSON.h>")
    if count:
        emit.Line("#include <core/Enumerate.h>")
    emit.Line()
    emit.Line("namespace %s {" % FRAMEWORK_NAMESPACE)
    emit.Line()
    emit.Line("namespace %s {" % DATA_NAMESPACE)
    emit.Indent()
    emit.Line()
    emit.Line("namespace %s {" % root.JsonName())
    emit.Indent()
    emit.Line()
    if emitCommon and enumTracker.CommonObjects():
        print "Emitting common enums..."
        emit.Line("// Common enums")
        emit.Line("//")
        emit.Line()
        for obj in enumTracker.CommonObjects():
            if not obj.IsDuplicate() and not obj.included_from:
                EmitEnum(obj)
    if emitCommon and objTracker.CommonObjects():
        print "Emitting common classes..."
        emit.Line("// Common classes")
        emit.Line("//")
        emit.Line()
        for obj in objTracker.CommonObjects():
            if not obj.included_from:
                EmitClass(obj, True)
    if root.Objects():
        print "Emitting params/result classes..."
        emit.Line("// Method params/result classes")
        emit.Line("//")
        emit.Line()
        EmitClass(root)
    emit.Unindent()
    emit.Line("} // namespace %s" % root.JsonName())
    emit.Unindent()
    emit.Line()
    emit.Line("} // namespace %s" % DATA_NAMESPACE)
    emit.Line()
    if count:
        emit.Line("// Enum conversion handlers")
        for obj in enumTracker.Objects():
            if not obj.IsDuplicate() and not obj.included_from:
                EmitEnumConversionHandler(root, obj)
        emit.Line()
    emit.Line("}")
    emit.Line()
    return emittedItems

def CreateCode(schema, path, generateClasses, generateStubs):
    directory = os.path.dirname(path)
    filename = os.path.basename(path.replace("Plugin", ""))
    rpcObj = ParseJsonRpcSchema(schema)
    if rpcObj:
        header_file = os.path.join(directory, DATA_NAMESPACE + "_" + filename + ".h")
        enum_file = os.path.join(directory, "JsonEnum_" + filename + ".cpp")
        if generateClasses:
            emitted = 0
            with open(header_file, "w") as output_file:
                emitter = Emitter(output_file, INDENT_SIZE)
                emitter.Line()
                emitter.Line("// C++ classes for %s JSON-RPC API." % rpcObj.info["title"].replace("Plugin", "").strip())
                emitter.Line("// Generated automatically from '%s.json'." % os.path.basename(path))
                emitter.Line()
                emitter.Line("// Note: This code is inherently not thread safe. If required, proper synchronisation must be added.")
                emitter.Line()
                emitted = EmitObjects(rpcObj, emitter, True)
                if emitted:
                    trace.Success("JSON data classes generated in '%s'." % output_file.name)
                else:
                    trace.Success("No JSON data classes generated for '%s'." % filename)
            if not emitted and not KEEP_EMPTY:
                try:
                    os.remove(header_file)
                except:
                    pass

            with open(enum_file, "w") as output_file:
                emitter = Emitter(output_file, INDENT_SIZE)
                emitter.Line()
                emitter.Line("// Enumeration code for %s JSON-RPC API." % rpcObj.info["title"].replace("Plugin", "").strip())
                emitter.Line("// Generated automatically from '%s.json'." % os.path.basename(path))
                emitter.Line()
                emitted = EmitEnumRegs(rpcObj, emitter, filename)
                if emitted:
                    trace.Success("JSON enumeration code generated in '%s'." % output_file.name)
                else:
                    trace.Success("No JSON enumeration code generated for '%s'." % filename)
            if not emitted and not KEEP_EMPTY:
                try:
                    os.remove(enum_file)
                except:
                    pass

        if generateStubs:
            with open(os.path.join(directory, filename + "JsonRpc.cpp"), "w") as output_file:
                emitter = Emitter(output_file, INDENT_SIZE)
                emitter.Line()
                EmitHelperCode(rpcObj, emitter, os.path.basename(header_file))
                trace.Success("JSON-RPC stubs generated in '%s'." % output_file.name)
    else:
        trace.Success("No code to generate.")


#
# DOCUMENTATION GENERATION
#

def CreateDocument(schema, path):
    output_path = path + ".md"
    with open(output_path, "w") as output_file:
        emit = Emitter(output_file, INDENT_SIZE)

        def bold(string):
            return "**%s**" % string

        def italics(string):
            return "*%s*" % string

        def link(string):
            return "[%s](#%s)" % (string.split(".", 1)[1].replace("_", " "), string)

        def MdBr():
            emit.Line()

        def MdHeader(string, level=1, id="head", include=None):
            if level < 3:
                emit.Line("<a name=\"%s\"></a>" % (id + "." + string.replace(" ", "_")))
            if id != "head":
                string += " <sup>%s</sup>" % id
            emit.Line("%s %s" % ("#"*level, "*%s*" % string if id != "head" else string))
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
            def __TableObj(name, obj, parentName = "", parent=None, prefix="", parentOptional=False):
                # determine if the attribute is optional
                optional = parentOptional or (obj["optional"] if "optional" in obj else False)
                if parent and not optional:
                    if parent["type"] == "object":
                        optional = ("required" not in parent and len(parent["properties"]) > 1) or ("required" in parent and name not in parent["required"]) or ("required" in parent and len(parent["required"])==0)

                #include information about enum values in description
                enum = ' (must be one of the following: %s)' % (", ".join('*{0}*'.format(w) for w in obj["enum"])) if "enum" in obj else ""
                if parent and prefix and parent["type"] == "object":
                    prefix += "?." if optional else "."
                prefix += name
                description = obj["description"] if "description" in obj else obj["summary"] if "summary" in obj else ""
                if isinstance(obj, jsonref.JsonRef) and "description" in obj.__reference__:
                    description = obj.__reference__["description"]
                if name or prefix:
                    if "type" not in obj:
                        raise RuntimeError("missing 'type' for object %s" % (parentName+"/"+name))
                    row = (("<sup>"+italics("(optional)") + "</sup>" +" ") if optional else "") + description + enum
                    if row.endswith('.'):
                        row = row[:-1]
                    MdRow([prefix, obj["type"], row])
                if obj["type"] == "object":
                    if "required" not in obj and name and len(obj["properties"]) > 1:
                        trace.Warn('No "required" field for object "%s"' % name)
                    for pname, props in obj["properties"].iteritems():
                        __TableObj(pname, props, parentName+"/"+name, obj, prefix, False)
                elif obj["type"] == "array":
                    __TableObj("", obj["items"], parentName+"/"+name, obj, (prefix + "[#]") if name else "", optional)
            __TableObj(name, object, "")
            MdBr()

        def ErrorTable(obj):
            MdTableHeader(["Code", "Message", "Description"])
            for err in obj:
                description = err["description"] if "description" in err else ""
                if isinstance(err, jsonref.JsonRef) and "description" in err.__reference__:
                    description = err.__reference__["description"]
                MdRow([err["code"], "```"+err["message"]+"```", description])
            MdBr()

        def PlainTable(obj, columns, ref="ref"):
            MdTableHeader(columns)
            for prop,val in sorted(obj.iteritems()):
                MdRow(["<a name=\"%s.%s\">%s</a>" % (ref, (prop.split("]",1)[0][1:]) if "]" in prop else prop, prop), val])
            MdBr()

        def __ExampleObj(name, obj):
            objType = obj["type"]
            default = obj["example"] if "example" in obj else obj["default"] if "default" in obj else ""
            if not default and "enum" in obj:
                default = obj["enum"][0]
            jsonData = '"%s": ' % name if name else ''
            if objType == "string":
                jsonData += '"%s"' % (default)
            elif objType in ["integer", "number"]:
                jsonData += '%s' % (default if default else 0)
            elif objType == "boolean":
                jsonData += '%s' % str(default if default else False).lower()
            elif objType == "null":
                jsonData += 'null'
            elif objType == "array":
                jsonData += str(default if default else ('[ %s ]' % (__ExampleObj("", obj["items"]))))
            elif objType == "object":
                jsonData += "{ %s }" % ", ".join(map(lambda p: __ExampleObj(p, obj["properties"][p]), obj["properties"])[0:obj["maxProperties"] if "maxProperties" in obj else None])
            return jsonData

        def MethodDump(method, props, classname, is_notification=False, is_property=False, include=None):
            method = method.rsplit(".", 1)[1] if "." in method else method
            MdHeader(method, 2, "property" if is_property else "event" if is_notification else "method", include)
            readonly = False
            writeonly = False
            if "summary" in props:
                text = props["summary"]
                if is_property:
                    text = "Provides access to the " + (text[0].lower() if text[1].islower() else text[0]) + text[1:]
                if not text.endswith('.'):
                    text += '.'
                MdParagraph(text)
            if is_property:
                if "readonly" in props and props["readonly"] == True:
                    MdParagraph("> This property is **read-only**.")
                    readonly = True
                elif "writeonly" in props and props["writeonly"] == True:
                    writeonly = True
                    MdParagraph("> This property is **write-only**.")
            if "description" in props:
                MdHeader("Description", 3)
                MdParagraph(props["description"])
            if "events" in props:
                MdParagraph("Also see: " + (", ".join(map(lambda x: link("event." + x), props["events"]))))
            if is_property:
                MdHeader("Value", 3)
                if not "description" in props["params"]:
                    props["params"]["description"] = props["summary"]
                ParamTable("(property)", props["params"])
                if "index" in props:
                    if "name" not in props["index"] or "example" not in props["index"]:
                        raise RuntimeError("in %s: index field needs 'name' and 'example' properties" % method)
                    extra_paragraph = "> The *%s* shall be passed as the index to the property, e.g. *%s.1.%s@%s*.%s" % (props["index"]["name"].lower(), classname, method, props["index"]["example"], (" " + props["index"]["description"]) if "description" in props["index"] else "")
                    if not extra_paragraph.endswith('.'):
                        extra_paragraph += '.'
                    MdParagraph(extra_paragraph)
            else:
                MdHeader("Parameters", 3)
                if "params" in props:
                    ParamTable("params", props["params"])
                else:
                    if is_notification:
                        MdParagraph("This event carries no parameters.")
                    else:
                        MdParagraph("This method takes no parameters.")
                if is_notification:
                    if "id" in props:
                        if "name" not in props["id"] or "example" not in props["id"]:
                            raise RuntimeError("in %s: id field needs 'name' and 'example' properties" % method)
                        MdParagraph("> The *%s* shall be passed within the designator, e.g. *%s.client.events.1*." % (props["id"]["name"], props["id"]["example"]))

            if "result" in props:
                MdHeader("Result", 3)
                ParamTable("result", props["result"])
            if not is_notification and "errors" in props:
                MdHeader("Errors", 3)
                ErrorTable(props["errors"])

            MdHeader("Example", 3)

            if is_notification:
                method = "client.events.1." + method
            elif is_property:
                method = "%s.1.%s%s" % (classname, method, ("@" + props["index"]["example"]) if "index" in props and "example" in props["index"] else "")
            else:
                method = "%s.1.%s" % (classname, method)
            if "id" in props and "example" in props["id"]:
                method = props["id"]["example"] + "." + method
            parameters =  props["params"] if "params" in props else None

            if is_property:
                if not writeonly:
                    MdHeader("Get Request", 4)
                    jsonRequest = json.dumps(json.loads('{ "jsonrpc": "2.0", "id": 1234567890, "method": "%s" }' % method, object_pairs_hook=OrderedDict), indent=4)
                    MdCode(jsonRequest, "json")
                    MdHeader("Get Response", 4)
                    jsonResponse = json.dumps(json.loads('{ "jsonrpc": "2.0", "id": 1234567890, %s }' % __ExampleObj("result", parameters), object_pairs_hook=OrderedDict), indent=4)
                    MdCode(jsonResponse, "json")

            if not readonly:
                if not is_notification:
                    if is_property:
                        MdHeader("Set Request", 4)
                    else:
                        MdHeader("Request", 4)

                jsonRequest = json.dumps(json.loads('{ "jsonrpc": "2.0", %s"method": "%s"%s }' % ('"id": 1234567890, ' if not is_notification else "", method, (", " + __ExampleObj("params", parameters)) if parameters else ""), object_pairs_hook=OrderedDict), indent=4)
                MdCode(jsonRequest, "json")

                if not is_notification and not is_property:
                    if "result" in props:
                        MdHeader("Response", 4)
                        jsonResponse = json.dumps(json.loads('{ "jsonrpc": "2.0", "id": 1234567890, %s }' % __ExampleObj("result", props["result"]), object_pairs_hook=OrderedDict), indent=4)
                        MdCode(jsonResponse, "json")
                    elif "noresult" not in props or not props["noresult"]:
                        raise RuntimeError("missing 'result' in %s" % method)

                if is_property:
                    MdHeader("Set Response", 4)
                    jsonResponse = json.dumps(json.loads('{ "jsonrpc": "2.0", "id": 1234567890, "result": "null" }', object_pairs_hook=OrderedDict), indent=4)
                    MdCode(jsonResponse, "json")

        MdBody("<!-- Generated automatically, DO NOT EDIT! -->")
        commons = dict()
        with open(os.path.join(os.path.dirname(os.path.realpath(__file__)), GLOBAL_DEFINITIONS)) as f:
            commons = json.load(f)

        info = schema["info"]
        method_count = 0
        property_count = 0
        event_count = 0
        interface = dict()

        interface = schema
        if "interface" in schema:
            interface = schema["interface"]
        if "methods" in interface:
            method_count = len(interface["methods"])
        if "properties" in interface:
            property_count = len(interface["properties"])
        if "events" in interface:
            event_count = len(interface["events"])
        if "include" in interface:
            for name, iface in interface["include"].iteritems():
                if "methods" in iface:
                    method_count += len(iface["methods"])
                if "properties" in iface:
                    property_count += len(iface["properties"])
                if "events" in iface:
                    event_count += len(iface["events"])

        if "title" in info:
            MdHeader(info["title"])

        version = info["version"] if "version" in info else "1.0"
        MdParagraph(bold("Version: " + version))
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
            raise RuntimeError("invalid status")
        MdParagraph(bold("Status: " + rating*":black_circle:" + (3-rating)*":white_circle:"))

        plugin_class = None
        if "class" in info:
            plugin_class = info["class"]
        elif "info" in interface and "class" in interface["info"]:
            plugin_class = interface["info"]["class"]
        else:
            raise RuntimeError("missing class in info or interface/info")

        MdParagraph("%s plugin for Thunder framework." % plugin_class)

        MdHeader("Table of Contents",3)
        MdBody("- " + link("head.Introduction"))
        if "description" in info:
            MdBody("- " + link("head.Description"))
        MdBody("- " + link("head.Configuration"))
        if method_count:
            MdBody("- " + link("head.Methods"))
        if property_count:
            MdBody("- " + link("head.Properties"))
        if event_count:
            MdBody("- " + link("head.Notifications"))
        MdBr()

        def mergedict(d1, d2, prop):
            return dict((d1[prop] if prop in d1 else dict()).items() + (d2[prop] if prop in d2 else dict()).items())

        MdHeader("Introduction")
        MdHeader("Scope", 2)
        if "scope" in info:
            MdParagraph(info["scope"])
        elif "title" in info:
            extra = ""
            if method_count and property_count and event_count:
                extra = ", methods and properties provided, as well as notifications sent"
            elif method_count and property_count:
                extra = ", methods and properties provided"
            elif method_count and event_count:
                extra = ", methods provided and notifications sent"
            elif property_count and event_count:
                extra = ", properties provided and notifications sent"
            elif method_count:
                extra = " and methods provided"
            elif property_count:
                extra = " and properties provided"
            elif event_count:
                extra = " and notifications sent"
            MdParagraph("This document describes purpose and functionality of the %s plugin. It includes detailed specification of its configuration%s." % (plugin_class, extra))

        MdHeader("Case Sensitivity",2)
        MdParagraph("All identifiers on the interface described in this document are case-sensitive. Thus, unless stated otherwise, all keywords, entities, properties, relations and actions should be treated as such.")
        if "acronyms" in info or "acronyms" in commons or "terms" in info or "terms" in commons:
            MdHeader("Acronyms, Abbreviations and Terms",2)
            if "acronyms" in info or "acronyms" in commons:
                MdParagraph("The table below provides and overview of acronyms used in this document and their definitions.")
                PlainTable(mergedict(commons, info, "acronyms"), ["Acronym", "Description"], "acronym")
            if "terms" in info or "terms" in commons:
                MdParagraph("The table below provides and overview of terms and abbreviations used in this document and their definitions.")
                PlainTable(mergedict(commons, info, "terms"), ["Term", "Description"], "term")

        if "standards" in info:
            MdHeader("Standards",2)
            MdParagraph(info["standards"])

        if "references" in commons or "references" in info:
            MdHeader("References",2)
            PlainTable(mergedict(commons, info, "references"), ["Ref ID", "Description"])

        if "description" in info:
            MdHeader("Description")
            MdParagraph(" ".join(info["description"]) if isinstance(info["description"], list) else info["description"])
            MdParagraph("The plugin is designed to be loaded and executed within the Thunder framework. For more information about the framework refer to [[Thunder](#ref.Thunder)].")

        MdHeader("Configuration")
        commonConfig = OrderedDict()
        if "configuration" in schema and "nodefault" in schema["configuration"] and schema["configuration"]["nodefault"] and "properties" not in schema["configuration"]:
            MdParagraph("The plugin does not take any configuration.")
        else:
            MdParagraph("The table below lists configuration options of the plugin.")
            if "configuration" not in schema or ("nodefault" not in schema["configuration"] or not schema["configuration"]["nodefault"]):
                if "callsign" in info:
                    commonConfig["callsign"] = { "type": "string", "description": 'Plugin instance name (default: *%s*)' % info["callsign"] }
                if plugin_class:
                    commonConfig["classname"] = { "type": "string", "description": 'Class name: *%s*' % plugin_class }
                if "locator" in info:
                    commonConfig["locator"] = { "type": "string", "description":  'Library name: *%s*' % info["locator"] }
                commonConfig["autostart"] = {"type": "boolean", "description": "Determines if the plugin is to be started automatically along with the framework" }

            required = []
            if "configuration" in schema:
                commonConfig2 = OrderedDict(commonConfig.items() + schema["configuration"]["properties"].items())
                required = schema["configuration"]["required"] if "required" in schema["configuration"] else []
            else:
                commonConfig2 = commonConfig

            totalConfig = OrderedDict()
            totalConfig["type"] = "object"
            totalConfig["properties"] = commonConfig2
            if "configuration" not in schema or ("nodefault" not in schema["configuration"] or not schema["configuration"]["nodefault"]):
               totalConfig["required"] = ["callsign", "classname", "locator", "autostart"] + required

            ParamTable("", totalConfig)

        def SectionDump(section_name, section, header, description=None, description2=None, event=False, prop=False):
            skip_list = []

            def InterfaceDump(interface, section, header):
                head = False
                if section in interface:
                    for method, contents in interface[section].iteritems():
                        if contents and method not in skip_list:
                            if not head:
                                MdParagraph("%s interface %s:" % (interface["info"]["class"], section))
                                MdTableHeader([header.capitalize(), "Description"])
                                head = True
                            access = ""
                            if "readonly" in contents and contents["readonly"] == True:
                                access = "RO"
                            elif "writeonly" in contents and contents["writeonly"] == True:
                                access = "WO"
                            if access:
                                access = " <sup>%s</sup>" % access
                            descr = ""
                            if "summary" in contents:
                                descr = contents["summary"]
                                if "e.g" in descr:
                                    descr = descr[0:descr.index("e.g") - 1]
                                if "i.e" in descr:
                                    descr = descr[0:descr.index("i.e") - 1]
                                descr = descr.split(".",1)[0] if "." in descr else descr
                            MdRow([link(header + "." + (method.rsplit(".", 1)[1] if "." in method else method)) + access, descr])
                        skip_list.append(method)

            MdHeader(section_name)
            if description:
                MdParagraph(description)

            MdParagraph("The following %s are provided by the %s plugin:" % (section, plugin_class))
            InterfaceDump(interface, section, header)
            if "include" in interface:
                for name, s in interface["include"].iteritems():
                    if s:
                        if section in s:
                            MdBr()
                            InterfaceDump(s, section, header)
            MdBr()
            if description2:
                MdParagraph(description2)
                MdBr()

            if section in interface:
                for method, props in interface[section].iteritems():
                    if props:
                        MethodDump(method, props, plugin_class, event, prop)

            if "include" in interface:
                for name, s in interface["include"].iteritems():
                    if s:
                        cl = s["info"]["class"]
                        if section in s:
                            for method, props in s[section].iteritems():
                                MethodDump(method, props, plugin_class, event, prop, cl)

        if method_count:
            SectionDump("Methods", "methods", "method")

        if property_count:
            SectionDump("Properties", "properties", "property", prop=True)

        if event_count:
            SectionDump("Notifications", "events", "event", "Notifications are autonomous events, triggered by the internals of the plugin, and broadcasted via JSON-RPC to all registered observers. Refer to [[Thunder](#ref.Thunder)] for information on how to register for a notification.", event=True)

        trace.Success("Document created: %s" % output_path)



# -------------------------------------------------------------------------
# entry point

objTracker = ObjectTracker()
enumTracker = EnumTracker()

if __name__ == "__main__":
    argparser = argparse.ArgumentParser(description='Generate JSON C++ classes, stub code and API documentation from JSON definition files.', formatter_class=argparse.RawTextHelpFormatter )
    argparser.add_argument('path', nargs="*", help="JSON file(s), wildcards are allowed")
    argparser.add_argument("--version", dest="version", action="store_true", default=False, help="display version")
    argparser.add_argument("-d", "--docs", dest="docs", action="store_true", default=False, help="generate documentation")
    argparser.add_argument("-c", "--code", dest="code", action="store_true", default=False, help="generate JSON classes")
    argparser.add_argument("-s", "--stubs", dest="stubs", action="store_true", default=False, help="generate JSON-RPC stub code")
    argparser.add_argument("-p", dest="if_path",  metavar="PATH", action="store", type=str, default=IF_PATH, help="relative path for #include'ing JsonData header file (default: 'interfaces/json', '.' for no path)")
    argparser.add_argument("-i", dest="if_dir", metavar="DIR", action="store", type=str, default=None, help="a directory with API interfaces that will substitute the {interfacedir} tag (default: same directory as source file)")
    argparser.add_argument("-o", "--output", dest="output_dir",  metavar="DIR", action="store", default=None, help="output directory (default: output in the same directory as the source json)")
    argparser.add_argument("--indent", dest="indent_size", metavar="SIZE", type=int, action="store", default=INDENT_SIZE, help="code indentation in spaces (default: %i)" % INDENT_SIZE)
    argparser.add_argument("--copy-ctor", dest="copy_ctor", action="store_true", default=False, help="always emit a copy constructor and assignment operator for a class (default: emit only when it appears to be needed)")
    argparser.add_argument("--keep-empty", dest="keep_empty", action="store_true", default=False, help="keep generated files that have no content (default: remove empty cpp/h files)")
    argparser.add_argument("--no-ref-names", dest="no_ref_names", action="store_true", default=False, help="do not derive class names from $refs (default: derive class names from $ref)")
    argparser.add_argument("--def-string", dest="def_string", metavar="STRING", type=str, action="store", default=DEFAULT_EMPTY_STRING, help="default string initialisation (default: \"%s\")" % DEFAULT_EMPTY_STRING)
    argparser.add_argument("--def-int-size", dest="def_int_size", metavar="SIZE", type=int, action="store", default=DEFAULT_INT_SIZE, help="default integer size in bits (default: %i)" % DEFAULT_INT_SIZE)
    argparser.add_argument("--no-warnings", dest="no_warnings", action="store_true", default=False, help="suppress style/wording warnings (default: show all warnings)")
    args = argparser.parse_args(sys.argv[1:])
    VERIFY = not args.no_warnings
    INDENT_SIZE = args.indent_size
    ALWAYS_COPYCTOR = args.copy_ctor
    KEEP_EMPTY = args.keep_empty
    CLASSNAME_FROM_REF = not args.no_ref_names
    DEFAULT_EMPTY_STRING = args.def_string
    DEFAULT_INT_SIZE = args.def_int_size
    if args.if_path and args.if_path != ".":
        IF_PATH = args.if_path
    IF_PATH = posixpath.normpath(IF_PATH) + os.sep

    if args.if_dir:
        args.if_dir = os.path.normpath(args.if_dir)
    generateCode = args.code
    generateDocs = args.docs
    generateStubs = args.stubs

    if args.version:
        print "Version: %s" % VERSION
        sys.exit(1)
    elif not args.path or (not generateCode and not generateStubs and not generateDocs):
        argparser.print_help()
    else:
        files = []
        for p in args.path:
            files.extend(glob.glob(p))
        for path in files:
            try:
                trace.Header("\nProcessing file '%s'" % path)
                schema = LoadSchema(path, os.path.abspath(args.if_dir) if args.if_dir else "")
                output_path = path
                if args.output_dir:
                    output_path = os.path.join(args.output_dir, os.path.basename(output_path))
                output_path = output_path.replace(".json", "")
                if generateCode or generateStubs:
                    CreateCode(schema, output_path, generateCode, generateStubs)
                if generateDocs:
                    title = schema["info"]["title"] if "title" in schema else schema["info"]["class"] if "class" in schema else os.path.basename(output_path)
                    CreateDocument(schema, os.path.join(os.path.dirname(output_path), title.replace(" ","")))
            except JsonParseError as err:
                trace.Error(str(err))
            except RuntimeError as err:
                trace.Error(str(err))
            except IOError as err:
                trace.Error(str(err))
            except ValueError as err:
                trace.Error(str(err))
        print "\nJsonGenerator: All done. %s error%s." % (trace.errors if trace.errors else "No", "" if trace.errors == 1 else "s")
        if trace.errors:
            sys.exit(1)
