#!/usr/bin/env python

#
# C++ header parser
#

import re, uuid, sys
from collections import OrderedDict


class ParserError(RuntimeError):

    def __init__(self, msg):
        msg = "%s(%s): parse error: %s" % (CurrentFile(), CurrentLine(), msg)
        super(ParserError, self).__init__(msg)


# Checks if identifier is valid.
def is_valid(token):
    if "operator" in token:
        return re.match(r'^[-\+~<>=!%&^*/\|\[\]]+$', token[8:])
    else:
        return token and re.match(r'^[a-zA-Z0-9_~]+$',
                                  token) and not token[0].isdigit()


def ASSERT_ISVALID(token):
    if not is_valid(token):
        raise ParserError("invalid identifier: '" + token + "'")
    elif token in ["alignas", "alignof"]:
        raise ParserError("aligment specifiers are not supported")


def ASSERT_ISEXPECTED(token, list):
    if token not in list:
        raise ParserError("unexpected identifier: '" + token +
                          "', expected one of " + str(list))


# -------------------------------------------------------------------------
# CLASS DEFINITIONS
# -------------------------------------------------------------------------

global_namespace = None


# Holds identifier type
class Type:

    def __init__(self,
                 parent_block,
                 string,
                 valid_specifiers,
                 tags_allowed=True):
        self.parent = parent_block
        self.name = ""
        self.brief = ""
        self.details = ""
        self.specifiers = []
        self.input = False
        self.output = False
        self.is_property = False
        self.length = None
        self.maxlength = None
        self.interface = None
        self.param = OrderedDict()
        self.retval = OrderedDict()
        type = ["?"]  # indexing safety
        type_found = False
        nest1 = 0
        nest2 = 0
        array = False
        skip = 0

        if string.count("*") > 1:
            raise ParserError("pointers to pointers are not supported: '%s'" %
                              (" ".join(string)))
        elif string.count("[") > 1:
            raise ParserError(
                "multi-dimensional arrays are not supported: '%s'" %
                (" ".join(string)))
        elif "[" in string and "*" in string:
            raise ParserError("arrays of pointers are not supported: '%s'" %
                              (" ".join(string)))
        elif "&&" in string:
            raise ParserError("rvalue references are not supported: '%s'" %
                              (" ".join(string)))

        for i, token in enumerate(string):
            if skip > 0:
                skip -= 1
                continue

            # just keep together anything that comes within <> or () brackets
            if token == "(":
                type[-1] += " ("
                type_found = False
                nest1 += 1
            elif token == ")":
                type[-1] += " )"
                if nest1 == 0 and not nest2:
                    type_found = True
                nest2 -= 1
            elif token == "<":
                type[-1] += " <"
                type_found = False
                nest2 += 1
            elif token == ">":
                type[-1] += " >"
                if nest2 == 0 and not nest1:
                    type_found = True
                nest2 -= 1
            elif nest1 or nest2:
                type[-1] += " " + token

            # handle pointer/reference markers
            elif token[0] == "@":
                if token[1:] == "IN":
                    if tags_allowed:
                        self.input = True
                    else:
                        raise ParserError(
                            "in/out tags not allowed on return value")
                elif token[1:] == "OUT":
                    if tags_allowed:
                        self.output = True
                    else:
                        raise ParserError(
                            "in/out tags not allowed on return value")
                elif token[1:] == "LENGTH":
                    self.length = string[i + 1]
                    skip = 1
                    continue
                elif token[1:] == "MAXLENGTH":
                    if tags_allowed:
                        self.maxlength = string[i + 1]
                    else:
                        raise ParserError(
                            "maxlength tag not allowed on return value")
                    skip = 1
                    continue
                elif token[1:] == "INTERFACE":
                    self.interface = string[i + 1]
                    skip = 1
                elif token[1:] == "PROPERTY":
                    self.is_property = True
                elif token[1:] == "BRIEF":
                    self.brief = string[i + 1]
                    skip = 1
                elif token[1:] == "DETAILS":
                    self.details = string[i + 1]
                    skip = 1
                elif token[1:] == "PARAM":
                    self.param[string[i + 1]] = string[i + 2]
                    skip = 2
                elif token[1:] == "RETVAL":
                    self.retval[string[i + 1]] = string[i + 2]
                    skip = 2
                else:
                    raise ParserError("invalid tag: " + token)

            # skip C-style explicit struct
            elif token in ["struct", "class", "union"]:
                continue
            elif token in ["export"]:  # skip
                continue

            # keep identifers with scope operator together
            elif token == "::":
                if len(type) > 1:
                    type[-1] += "::"
                type_found = False

            # arrays are equivalent to pointers here, so make it uniform
            # disregard anything that's inside the brackets
            elif token == "[":
                array = True
            elif token == "]":
                array = False
                type.append("*")

            elif token in ["*", "&"]:
                type.append(token)

            elif token in ["const", "volatile", "constexpr"]:
                if token == "constexpr":
                    self.specifiers.append("constexpr")
                    token = "const"

                # put qualifiers in order
                if "*" in type:
                    type.insert(type.index("*") + 1, token)
                elif "&" in type:
                    type.insert(type.index("&") + 1, token)
                else:
                    type.insert(1, token)

            # include valid specifiers
            elif token in valid_specifiers:
                self.specifiers.append(token)

            elif not type_found and not array:
                # handle primitive type combinations...
                if (token in ["int"]) and (type[-1].split()[-1] in [
                        "signed", "unsigned", "short", "long"
                ]):
                    type[-1] += " " + token
                elif (token in [
                        "char", "short", "long"
                ]) and (type[-1].split()[-1] in ["signed", "unsigned"]):
                    type[-1] += " " + token
                elif (token in ["long", "double"
                                ]) and (type[-1].split()[-1] in ["long"]):
                    type[-1] += " " + token
                # keep identifers with scope operator together
                elif type[-1].endswith("::"):
                    type[-1] += token
                # keep together anything that comes within <> or () brackets
                elif nest1 == 0 and nest2 == 0:
                    type.append(token)
                else:
                    type[-1] += token

                if ((i == len(string) - 1) or (string[i + 1] not in [
                        "char", "short", "long", "int", "double"
                ])):
                    type_found = True

            elif type_found:
                if not array: self.name = token

        if array:
            raise ParserError("unmatched bracket '['")

        self.type = type[1:]

        # Try to match the type to an already defined class...
        self._Substitute(parent_block)

    def _Substitute(self, parent):
        if isinstance(parent, Method):
            parent = parent.parent

        if self.type:

            def __Search(tree, found, T):
                qualifiedT = "::" + T

                # need full qualification if the class is a subclass
                if tree.full_name.startswith(parent.full_name + "::"):
                    if T.count("::") != tree.full_name.replace(
                            parent.full_name, "").count("::"):
                        return

                enum_match = [
                    e for e in tree.enums if e.full_name.endswith(qualifiedT)
                ]
                typedef_match = [
                    td for td in tree.typedefs
                    if td.full_name.endswith(qualifiedT)
                ]
                class_match = [
                    cl for cl in tree.classes
                    if cl.full_name.endswith(qualifiedT)
                ]

                found += enum_match + typedef_match + class_match

                if isinstance(tree, (Namespace, Class)):
                    for c in tree.classes:
                        __Search(c, found, T)

                if isinstance(tree, Namespace):
                    for n in tree.namespaces:
                        __Search(n, found, T)

            # find the type to scan for...
            i = -1
            while self.type[i] in ["*", "&", "const", "volatile"]:
                i -= 1

            if self.type[i] not in [
                    "bool", "int", "char", "wchar_t", "short", "long", "signed",
                    "unsigned", "void", "float", "double", "int8_t", "uint8_t",
                    "int16_t", "uint16_t", "int32_t", "uint32_t", "int64_t",
                    "uint64_t", "size_t"
            ]:
                found = []
                __Search(global_namespace, found, self.type[i])
                if found:
                    self.type[i] = found[-1]  # take closest match

    def __str__(self):
        return " ".join(self.type)

    def __repr__(self):
        return str(self)

    def Type(self):
        return self.type


def Evaluate(identifiers):
    val = []
    if identifiers:
        for identifier in identifiers:
            try:
                val.append(
                    str(int(identifier, 16 if identifier[:2] == "0x" else 10)))
            except:

                def __Search(tree, found, T):
                    var_match = [
                        v for v in tree.vars if v.full_name.endswith(T)
                    ]
                    enumerator_match = []
                    for e in tree.enums:
                        enumerator_match += [
                            item for item in e.items
                            if item.full_name.endswith(T)
                        ]

                    found += var_match + enumerator_match

                    if isinstance(tree, (Namespace, Class)):
                        for c in tree.classes:
                            __Search(c, found, T)

                    if isinstance(tree, Namespace):
                        for n in tree.namespaces:
                            __Search(n, found, T)

                found = []
                __Search(global_namespace, found, "::" + identifier)
                if found:
                    val.append(str(found[-1].value))
                else:
                    val.append(str(identifier))

    if not val:
        val = identifiers

    val = "".join(val)

    # attempt to parse the arithmetics...
    try:
        val = eval(val)
    except:
        pass

    return val


# Holds identifier commons
class Identifier:

    def __init__(self, parent_block, name=""):
        if name:
            ASSERT_ISVALID(name)
        self.parent = parent_block
        # come up with an unique name if none given
        self.name = "__unnamed_" + self.__class__.__name__.lower(
        ) + "_" + uuid.uuid4().hex[:8] if (not name
                                           and self.parent != None) else name
        self.full_name = ("" if self.parent == None else self.parent.full_name
                          ) + ("" if not self.name else "::" + self.name)
        self.parser_file = CurrentFile()
        self.parser_line = CurrentLine()

    def __str__(self):
        return self.full_name

    def __repr__(self):
        return str(self)

    def Name(self):
        return self.full_name


# Holds type definition
class Typedef(Type, Identifier):

    def __init__(self, parent_block, string):
        Type.__init__(self, parent_block, string, [])
        Identifier.__init__(self, parent_block, self.name)
        self.parent = parent_block
        self.parent.typedefs.append(self)
        self.is_event = False

    def __str__(self):
        return "typedef " + self.full_name + " -> " + str(self.type)

    def __repr__(self):
        return str(self)


# Holds compound statements and composite types
class Block(Identifier):

    def __init__(self, parent_block, name=""):
        Identifier.__init__(self, parent_block, name)
        self.vars = []
        self.enums = []
        self.typedefs = []
        self.classes = []
        self.unions = []
        self.parser_file = CurrentFile()
        self.parser_line = CurrentLine()

    def __str__(self):
        return self.full_name

    def __repr__(self):
        return str(self)


# Holds namespaces
class Namespace(Block):

    def __init__(self, parent_block, name=""):
        Block.__init__(self, parent_block, name)
        self.namespaces = []
        self.methods = []
        self.omit = False
        self.stub = -False
        if self.parent != None:  # case for global namespace
            self.parent.namespaces.append(self)

    def __str__(self):
        return "namespace " + (self.full_name if self.full_name else "<global>")

    def __repr__(self):
        return str(self)


# Holds structs and classes
class Class(Block):

    def __init__(self, parent_block, name):
        Block.__init__(self, parent_block, name)
        self.specifiers = []
        self.methods = []
        self.classes = []
        self.ancestors = []  # parent classes
        self._current_access = "public"
        self.omit = False
        self.stub = False
        self.is_json = False
        self.is_event = False
        self.parent.classes.append(self)

    def __str__(self):
        astr = ""
        if self.ancestors:
            astr = " [: " + ", ".join(str(a[0]) for a in self.ancestors) + "]"
        return "class %s %s%s" % (self.specifiers, self.full_name, astr)

    def __repr__(self):
        return str(self)


# Holds unions
class Union(Block):

    def __init__(self, parent_block, name):
        Block.__init__(self, parent_block, name)
        self.methods = []
        self.classes = []
        self._current_access = "public"
        self.omit = False
        self.stub = False
        self.parent.unions.append(self)

    def __str__(self):
        return "union " + self.full_name

    def __repr__(self):
        return str(self)


# Holds enumeration blocks, including class enums
class Enum(Block):

    def __init__(self, parent_block, name, is_scoped, base_type="int"):
        Block.__init__(self, parent_block, name)
        self.items = []
        self.scoped = is_scoped
        self.base_type = base_type
        self.parent.enums.append(self)
        self._last_value = 0  # used for auto-incrementation

    def __str__(self):
        return ("enum"
                if not self.scoped else "enum class") + " " + self.full_name

    def __repr__(self):
        return str(self)

    def SetValue(self, value):
        self._last_value = value + 1

    def GetValue(self):
        return self._last_value


# Holds functions
class Function(Type, Block, Identifier):

    def __init__(self,
                 parent_block,
                 name,
                 ret_type,
                 valid_specifiers=["static", "extern", "inline"]):
        Type.__init__(self, parent_block, ret_type, valid_specifiers, False)
        Block.__init__(self, parent_block, name if name else self.name)
        Identifier.__init__(self, parent_block, self.name)
        self.omit = False
        self.stub = False
        self.parent.methods.append(self)

    def __str__(self):
        return "function " + str(self.specifiers) + " " + str(
            self.type) + " '" + self.name + "' (" + str(self.vars) + ")"

    def __repr__(self):
        return str(self)


# Holds variables and constants
class Variable(Type, Identifier):

    def __init__(self,
                 parent_block,
                 string,
                 value=[],
                 valid_specifiers=["static", "extern", "register"]):
        Type.__init__(self, parent_block, string, valid_specifiers)
        Identifier.__init__(self, parent_block, self.name)
        self.value = Evaluate(value) if value else None
        self.parent.vars.append(self)

    def __str__(self):
        return "variable %s %s '%s' [= %s]" % (str(
            self.specifiers), str(self.type), str(self.name), str(self.value))

    def __repr__(self):
        return str(self)


class Parameter(Variable, Type):

    def __init__(self, parent_block, string, value=[], valid_specifiers=[]):
        Variable.__init__(self, parent_block, string, value, valid_specifiers)
        if self.name in parent_block.param:
            self.brief = parent_block.param[self.name]

    def __str__(self):
        return "param %s '%s' [= %s]" % (str(self.type), str(
            self.name), str(self.value))

    def __repr__(self):
        return str(self)


# Holds member attributes
class Method(Function):

    def __init__(self, parent_block, name, ret_type):
        Function.__init__(
            self, parent_block, name, ret_type,
            ["inline", "static", "virtual", "explicit", "constexpr", "friend"])
        self.access = self.parent._current_access
        self.qualifiers = []

    def __str__(self):
        return "method [" + self.access +  ":] " + str(self.specifiers) + " " + str(self.type) + " '" + self.name + "' (" \
                    + str(self.vars) + ") " + str(self.qualifiers)

    def __repr__(self):
        return str(self)


# Holds member attributes and constants
class Attribute(Variable):

    def __init__(self, parent_block, string, value=[]):
        Variable.__init__(self, parent_block, string, value,
                          ["static", "constexpr", "thread_local", "mutable"])
        self.access = self.parent._current_access

    def __str__(self):
        return "attribute [%s:] %s %s '%s' [= %s]" % (
            self.access, str(self.specifiers), self.type, str(
                self.name), str(self.value))

    def __repr__(self):
        return str(self)


# Holds enumeration items
class Enumerator(Type, Identifier):

    def __init__(self, parent_block, name, value=None, base_type="int"):
        parent_enum = parent_block if parent_block.scoped else parent_block.parent
        Type.__init__(self, parent_enum, [base_type, name], [])
        Identifier.__init__(self, parent_enum, self.name)
        self.parent = parent_block
        self.value = parent_block.GetValue() if value == None else Evaluate(
            value)
        if isinstance(self.value, (int)):
            self.parent.SetValue(self.value)
        self.parent.items.append(self)

    def __str__(self):
        return "enumerator %s '%s' []= %s]" % (str(
            self.type), str(self.full_name), str(self.value))

    def __repr__(self):
        return str(self)


# -------------------------------------------------------------------------
# PRIVATE FUNCTIONS
# -------------------------------------------------------------------------


# Source file test into a list of tokens, removing comments and preprocessor directives.
def __Tokenize(contents):
    global current_file
    global current_line

    tokens = [
        s.strip() for s in re.split(r"([\r\n])", contents, flags=re.MULTILINE)
        if s
    ]
    eoltokens = []
    line = 1
    for token in tokens:
        if token.startswith("// @file:"):
            line = 1
        if token == '':
            eoltokens.append("// @line:" + str(line) + " ")
            line = line + 1
        elif (len(eoltokens) > 1) and eoltokens[-2].endswith("\\"):
            del eoltokens[-1]
            eoltokens[-1] = eoltokens[-1][:-1] + token
        else:
            eoltokens.append(token)

    contents = "\n".join(eoltokens)

    formula = (
        r"(#if 0[\S\s]*?#endif)"
        r"|(#.*)"  # preprocessor
        r"|(/\*[\S\s]*?\*/)"  # multi-line comments
        r"|(//.*)"  # single line comments
        r"|(\"[^\"]+\")"  # double quotes
        r"|(\'[^\']+\')"  # quotes
        r"|(::)|(==)|(!=)|(>=)|(<=)|(&&)|(\|\|)"  # two-char operators
        r"|(\+\+)|(--)|(\+=)|(-=)|(/=)|(\*=)|(%=)|(^=)|(&=)|(\|=)|(~=)"
        r"|([,:;~!?=^/*%-\+&<>\{\}\(\)\[\]])"  # single-char operators
        r"|([\r\n\t ])"  # whitespace
    )

    tokens = [
        s.strip() for s in re.split(formula, contents, flags=(re.MULTILINE))
        if s
    ]

    tagtokens = []
    # check for special metadata within comments
    for token in tokens:
        if token:

            def __ParseLength(string, tag):
                formula = (
                    r"(\"[^\"]+\")"
                    r"|(\'[^\']+\')"
                    r"|(\*/)|(::)|(==)|(!=)|(>=)|(<=)|(&&)|(\|\|)"
                    r"|(\+\+)|(--)|(\+=)|(-=)|(/=)|(\*=)|(%=)|(^=)|(&=)|(\|=)|(~=)"
                    r"|([,:;~!?=^/*%-\+&<>\{\}\(\)\[\]])"
                    r"|([\r\n\t ])")
                tagtokens.append(tag.upper())
                length_str = string[string.index(tag) + len(tag):]
                length_tokens = [
                    s.strip()
                    for s in re.split(formula, length_str, flags=re.MULTILINE)
                    if isinstance(s, str) and len(s.strip())
                ]
                if length_tokens[0] == ':':
                    length_tokens = length_tokens[1:]
                no_close_last = (length_tokens[0] == '(')
                tokens = []
                par_count = 0
                for t in length_tokens:
                    if t == '(':
                        if tokens:
                            tokens.append(t)
                        par_count += 1
                    elif t == ')':
                        par_count -= 1
                        if par_count == 0:
                            if not no_close_last:
                                tokens.append(t)
                            break
                        else:
                            tokens.append(t)
                    elif t == '*/' or t == "," or t[0] == '@':
                        break
                    else:
                        tokens.append(t)
                        if par_count == 0:
                            break
                if par_count != 0:
                    raise ParserError("unmatched parenthesis in %s expression" %
                                      tag)
                if len(tokens) == 0:
                    raise ParserError("invalid %s value" % tag)
                return tokens

            if ((token[:2] == "/*")
                    and (token.count("/*") != token.count("*/"))):
                raise ParserError("multi-line comment not closed")

            if ((token[:2] == "/*") or (token[:2] == "//")):

                def _find(word, string):
                    return re.compile(r"[ \r\n/\*]({0})([: \r\n\*]|$)".format(
                        word)).search(string) != None

                if _find("@stubgen", token):
                    if "@stubgen:skip" in token:
                        break
                    elif "@stubgen:omit" in token:
                        tagtokens.append("@OMIT")
                    elif "@stubgen:stub" in token:
                        tagtokens.append("@STUB")
                    else:
                        raise ParserError("invalid @stubgen tag")
                if _find("@in", token):
                    tagtokens.append("@IN")
                if _find("@out", token):
                    tagtokens.append("@OUT")
                if _find("@inout", token):
                    tagtokens.append("@IN")
                    tagtokens.append("@OUT")
                if _find("@property", token):
                    tagtokens.append("@PROPERTY")
                if _find("@json", token):
                    tagtokens.append("@JSON")
                if _find("@event", token):
                    tagtokens.append("@EVENT")
                if _find("@brief", token):
                    tagtokens.append("@BRIEF")
                    desc = token[token.index("@brief") +
                                 7:token.index("*/") if "*/" in
                                 token else None].strip()
                    if desc:
                        tagtokens.append(desc)
                if _find("@details", token):
                    tagtokens.append("@DETAILS")
                    desc = token[token.index("@details") +
                                 9:token.index("*/") if "*/" in
                                 token else None].strip()
                    if desc:
                        tagtokens.append(desc)
                if _find("@param", token):
                    tagtokens.append("@PARAM")
                    idx = token.index("@param")
                    paramName = token[idx + 7:].split()[0]
                    paramDesc = token[token.index(paramName) +
                                      len(paramName):token.index("*/") if "*/"
                                      in token else None].strip()
                    if paramName and paramDesc:
                        tagtokens.append(paramName)
                        tagtokens.append(paramDesc)
                if _find("@retval", token):
                    tagtokens.append("@RETVAL")
                    idx = token.index("@retval")
                    errorName = token[idx + 8:].split()[0]
                    errorDesc = token[token.index(errorName) +
                                      len(errorName):token.index("*/") if "*/"
                                      in token else None].strip()
                    if errorName and errorDesc:
                        tagtokens.append(errorName)
                        tagtokens.append(errorDesc)
                if _find("@length", token):
                    tagtokens.append(__ParseLength(token, "@length"))
                if _find("@maxlength", token):
                    tagtokens.append(__ParseLength(token, "@maxlength"))
                if _find("@interface", token):
                    tagtokens.append(__ParseLength(token, "@interface"))
                if _find("@file", token):
                    idx = token.index("@file:") + 6
                    tagtokens.append("@FILE:" + token[idx:])
                    current_file = token[idx:]
                if _find("@line", token):
                    idx = token.index("@line:") + 6
                    if len(tagtokens) and tagtokens[-1].startswith("@LINE:"):
                        del tagtokens[-1]
                    current_line = int(token[idx:].split()[0])
                    tagtokens.append("@LINE:" + token[idx:])
            elif len(token) > 0 and token[0] != '#' and token != "EXTERNAL":
                tagtokens.append(token)

    tagtokens.append(";")  # prevent potential out-of-range errors

    return tagtokens


# -------------------------------------------------------------------------
# EXPORTED FUNCTIONS
# -------------------------------------------------------------------------

i = 0
tokens = []
line_numbers = []
current_line = 0
current_file = "undefined"


def CurrentFile():
    return current_file


def CurrentLine():
    if i > 0:
        # error during c++ parsing
        return line_numbers[i]
    else:
        # error during preprocessing
        return current_line


# Builds a syntax tree (data structures only) of C++ source code
def Parse(contents):
    # Start in global namespace.
    global global_namespace
    global current_file
    global tokens
    global line_numbers
    global i

    i = 0
    tokens = []
    line_numbers = []
    line_tokens = []
    current_line = 0
    current_file = "undefined"

    # Split into tokens first
    line_tokens = __Tokenize(contents)

    for token in line_tokens:
        if isinstance(token, str) and token.startswith("@LINE:"):
            current_line = int(token[6:].split()[0])
        else:
            line_numbers.append(current_line)
            tokens.append(token)

    global_namespace = Namespace(None)
    current_block = [global_namespace]
    next_block = None
    last_template_def = []
    min_index = 0
    omit_next = False
    stub_next = False
    json_next = False
    event_next = False
    in_typedef = False

    # Main loop.
    while i < len(tokens):
        # Handle special tokens
        if not isinstance(tokens[i], str):
            i = i + 1
            continue

        if tokens[i] == "@OMIT":
            omit_next = True
            tokens[i] = ";"
            i += 1
        elif tokens[i] == "@STUB":
            stub_next = True
            tokens[i] = ";"
            i += 1
        elif tokens[i] == "@JSON":
            json_next = True
            tokens[i] = ";"
            i += 1
        elif tokens[i] == "@EVENT":
            event_next = True
            tokens[i] = ";"
            i += 1
        elif tokens[i].startswith("@FILE:"):
            current_file = tokens[i][6:]
            i += 1

        # Swallow template definitions
        elif tokens[i] == "template" and tokens[i + 1] == '<':
            s = i
            i += 1
            nest = 0
            while True:
                if tokens[i] == ">":
                    if nest == 1:
                        break
                    nest -= 1
                elif tokens[i] == "<":
                    nest += 1
                i += 1
            i += 1
            last_template_def = tokens[s:i]
            min_index = i

        # Parse namespace definition...
        elif tokens[i] == "namespace":
            namespace_name = ""
            if is_valid(tokens[i + 1]):  # is there a namespace name?
                namespace_name = tokens[i + 1]
                i += 1
            next_block = Namespace(current_block[-1], namespace_name)
            i += 1

        # Parse type alias...
        elif isinstance(current_block[-1],
                        (Namespace, Class)) and tokens[i] == "typedef":
            j = i + 1
            while tokens[j] != ";":
                j += 1
            typedef = Typedef(current_block[-1], tokens[i + 1:j])
            if event_next:
                typedef.is_event = True
                event_next = False
            if typedef.type[0] == "enum":
                in_typedef = True
                i += 1
            else:
                i = j + 1

        # Parse "using"...
        elif isinstance(current_block[-1],
                        (Namespace, Class)) and tokens[i] == "using" and tokens[
                            i + 1] != "namespace" and tokens[i + 2] == "=":
            i += 2
            j = i + 1
            while tokens[j] != ";":
                j += 1
            # reuse typedef class but correct name accordingly
            if not current_block[-1].omit:
                typedef = Typedef(current_block[-1], tokens[i + 1:j])
                if event_next:
                    typedef.is_event = True
                    event_next = False
                typedef_id = Identifier(current_block[-1], tokens[i - 1])
                typedef.name = typedef_id.name
                typedef.full_name = typedef_id.full_name
            i = j + 1

        elif isinstance(current_block[-1],
                        (Namespace, Class)) and tokens[i] == "using" and tokens[
                            i + 1] != "namespace" and tokens[i + 2] != "=":
            if not current_block[-1].omit:
                raise ParserError("using-declarations are not supported")

        elif isinstance(
                current_block[-1],
            (Namespace,
             Class)) and tokens[i] == "using" and tokens[i + 1] == "namespace":
            if not current_block[-1].omit:
                raise ParserError(
                    "'using namespace' directives are not supported")

        # Parse class definition...
        elif (tokens[i] == "class") or (tokens[i] == "struct") or (
                tokens[i] == "union"):
            new_class = Union(current_block[-1],
                              tokens[i + 1]) if tokens[i] == "union" else Class(
                                  current_block[-1], tokens[i + 1])
            new_class._current_access = "private" if tokens[
                i] == "class" else "public"

            if omit_next:
                new_class.omit = True
                omit_next = False
            elif stub_next:
                new_class.stub = True
                stub_next = False
            if json_next:
                new_class.is_json = True
                json_next = False
            if event_next:
                new_class.is_event = True
                event_next = False

            if last_template_def:
                new_class.specifiers.append(" ".join(last_template_def))
                last_template_def = []

            i += 1
            if tokens[i + 1] == "final":
                new_class.specifiers.append(tokens[i + 2])
                i += 1

            # parse class ancestors...
            # TODO: refactor!!
            if tokens[i + 1] == ':':
                i += 1
                parent_class = ""
                parent_access = "private"
                specifiers = []
                while True:
                    if tokens[i + 1] in ['{', ',']:
                        parent_ref = Type(
                            current_block[-1], [parent_class], []
                        )  # try to find a reference to an already found type
                        new_class.ancestors.append(
                            [parent_ref.type[0], parent_access, specifiers])
                        parent_access = "private"
                        if tokens[i + 1] == '{':
                            break
                    elif tokens[i + 1] in ["public", "private", "protected"]:
                        parent_access = tokens[i + 1]
                    elif tokens[i + 1] == "virtual":
                        specifiers.append(tokens[i + 1])
                    else:
                        parent_class += tokens[i + 1]
                    i += 1

            i += 1
            if tokens[i] == ';':
                i += 1
            else:
                next_block = new_class

        # Parse enum definition...
        elif isinstance(current_block[-1],
                        (Namespace, Class)) and tokens[i] == "enum":
            enum_name = ""
            enum_type = "int"
            is_scoped = False
            if (tokens[i + 1] == "class") or (tokens[i + 1] == "struct"):
                is_scoped = True
                i += 1
            if is_valid(tokens[i + 1]):  # enum name given?
                enum_name = tokens[i + 1]
                i += 1
            if tokens[i + 1] == ':':
                enum_type = tokens[i + 2]
                i += 2

            new_enum = Enum(current_block[-1], enum_name, is_scoped, enum_type)
            next_block = new_enum
            i += 1

        # Parse class access specifier...
        elif isinstance(current_block[-1], Class) and tokens[i] == ':':
            current_block[-1]._current_access = tokens[i - 1]
            ASSERT_ISEXPECTED(current_block[-1]._current_access,
                              ["private", "protected", "public"])
            i += 1

        # Parse function/method definition...
        elif isinstance(current_block[-1],
                        (Namespace, Class)) and tokens[i] == "(":
            # concatenate tokens to handle operators and destructors
            j = i - 1
            k = i - 1
            if isinstance(current_block[-1],
                          Class) and (tokens[i - 2] == "operator"):
                name = "operator" + tokens[i - 1]
                j -= 1
                k -= 1
            else:
                name = tokens[i - 1]
                if tokens[i - 2] == '~':
                    name = "~" + name  #dtor
                    j -= 1
                    k -= 1

            # locate return value
            while j >= min_index and tokens[j] not in ['{', '}', ';', ':']:
                j -= 1
            if not current_block[-1].omit and not omit_next:
                ret_type = tokens[j + 1:k]
            else:
                ret_type = []

            method = Method(current_block[-1], name, ret_type) if isinstance(current_block[-1], Class) \
                        else Function(current_block[-1], name, ret_type)

            if omit_next:
                method.omit = True
                omit_next = False
            elif method.parent.omit:
                method.omit = True
            elif stub_next:
                method.stub = True
                stub_next = False
            elif method.parent.stub:
                method.stub = True

            if last_template_def:
                method.specifiers.append(" ".join(last_template_def))
                last_template_def = []

            # try to detect a function/macro call
            function_call = not ret_type and ((name != current_block[-1].name)
                                              and
                                              (name !=
                                               ("~" + current_block[-1].name)))

            # parse method parameters...
            j = i
            nest = 0
            nest2 = 0
            while tokens[i] != ')':
                while tokens[j]:
                    if tokens[j] == '(':
                        nest += 1
                    elif tokens[j] == ')':
                        nest -= 1
                        if nest == 0 and nest2 == 0:
                            break
                    if tokens[j] == '<':
                        nest2 += 1
                    elif tokens[j] == '>':
                        nest2 -= 1
                    elif tokens[j] == ',' and nest == 1 and nest2 == 0:
                        break

                    j += 1

                param = tokens[i + 1:j]
                if len(param) and not (len(param) == 1 and param[0] == "void"
                                       ):  # remove C-style f(void)
                    value = []
                    if '=' in param:
                        assignment = param.index('=')
                        param = param[0:assignment]
                        value = param[assignment + 1:]
                    if not current_block[-1].omit and not method.omit:
                        Parameter(method, param, value)
                i = j
                j += 1

            if nest:
                raise ParserError("unmatched parenthesis '('")
            if nest2:
                raise ParserError("unmatched parenthesis '<'")

            # parse post-declaration qualifiers/specifiers...
            if isinstance(current_block[-1], Class):
                while tokens[i] not in [';', '{', ':']:
                    # const, volatile
                    if tokens[i] in ["const", "volatile"]:
                        method.qualifiers.append(tokens[i])
                    # handle pure virtual methods
                    elif (tokens[i] == "="):
                        if tokens[
                                i +
                                1] == "0" and "virtual" in method.specifiers:  # mark the virtual function as pure
                            method.specifiers[method.specifiers.index(
                                "virtual")] = "pure-virtual"
                        elif tokens[i + 1] in ["default", "delete"]:
                            method.specifiers.append(tokens[i + 1])
                        i += 1
                    elif tokens[i] in ["override", "final", "noexcept"]:
                        method.specifiers.append(tokens[i])
                    i += 1

            if function_call:  # it was apparently a function call and not declaration, so remove it
                current_block[-1].methods.pop()
            else:
                next_block = method

            if tokens[i] == ';':
                i += 1
            elif tokens[i] == ':':  # skip ctor initializers
                while tokens[i] != '{':
                    i += 1

        # Handle opening a compound block or a composite type
        elif tokens[i] == '{':
            current_block.append(next_block)
            i += 1

        # Handle closing a compound block/composite type
        elif tokens[i] == '}':
            if isinstance(current_block[-1], Class) and (tokens[i + 1] != ';'):
                raise ParserError(
                    "definitions following a class declaration is not supported (%s)"
                    % current_block[-1].full_name)
            if len(current_block) > 1:
                current_block.pop()
            else:
                raise ParserError("unmatched brace '{'")
            i += 1
            next_block = Block(current_block[-1])  # new anonymous scope

        # Parse variables and member attributes
        elif isinstance(current_block[-1],
                        (Namespace, Class)) and tokens[i] == ';' and (is_valid(
                            tokens[i - 1]) or tokens[i - 1] == "]"):
            j = i - 1
            while j >= min_index and tokens[j] not in ['{', '}', ';', ":"]:
                j -= 1
            if not current_block[-1].omit:
                Attribute(current_block[-1], tokens[j+1:i]) if isinstance(current_block[-1], Class)  \
                            else Variable(current_block[-1], tokens[j+1:i])
            i += 1

        # Parse constants and member constants
        elif isinstance(current_block[-1], (Namespace, Class)) and (
                tokens[i] == '=') and (tokens[i - 1] != "operator"):
            j = i - 1
            k = i + 1
            while tokens[j] not in ['{', '}', ';', ":"]:
                j -= 1
            while tokens[k] != ';':
                k += 1
            if not current_block[-1].omit:
                Attribute(current_block[-1], tokens[j+1:i], tokens[i+1:k]) if isinstance(current_block[-1], Class)  \
                            else Variable(current_block[-1], tokens[j+1:i], tokens[i+1:k])
            i = k

        # Parse an enum block...
        elif isinstance(current_block[-1], Enum):
            enum = current_block[-1]
            j = i
            while True:
                if tokens[i] in ['}', ',']:
                    Enumerator(
                        enum, tokens[j],
                        tokens[j + 2:i] if tokens[j + 1] == '=' else None,
                        enum.base_type)
                    if tokens[i + 1] == '}':
                        i += 1  # handle ,} situation
                        break
                    elif tokens[i] == '}':
                        break
                    else:
                        j = i + 1
                i += 1
            if in_typedef:
                current_block[-2].typedefs[-1].type[0] = enum
                in_typedef = False
        else:
            i += 1

    return global_namespace


# -------------------------------------------------------------------------


def ParseFile(source_file):
    with open(source_file) as file:
        contents = file.read()
    return Parse(contents)


def ParseFiles(source_files):
    contents = ""
    for source_file in source_files:
        try:
            with open(source_file) as file:
                contents += "// @file:%s\n" % source_file
                contents += file.read()
        except:
            pass
    return Parse(contents)


# -------------------------------------------------------------------------


def DumpTree(tree, ind=0):
    indent = ind * " "

    if isinstance(tree, (Namespace, Class)):
        print(indent + str(tree))

        for td in tree.typedefs:
            print(indent + 2 * " " + str(td))

        for e in tree.enums:
            print(indent + 2 * " " + str(e))
            for item in e.items:
                print(indent + 4 * " " + str(item))

    for v in tree.vars:
        print(indent + 2 * " " + str(v))

    if isinstance(tree, (Namespace, Class)):
        for m in tree.methods:
            print(indent + 2 * " " + str(m))

    if isinstance(tree, (Namespace, Class)):
        for c in tree.classes:
            DumpTree(c, ind + 2)

    if isinstance(tree, Namespace):
        for n in tree.namespaces:
            DumpTree(n, ind + 2)


# -------------------------------------------------------------------------
# entry point

if __name__ == "__main__":
    tree = ParseFile(sys.argv[1])
    if isinstance(tree, Namespace):
        DumpTree(tree)
    else:
        print(tree)
