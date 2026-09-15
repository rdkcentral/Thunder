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

import json
import ast

def convert_string(s):
    """
    Converts a given string to Python types int, float or boolean values
    Returns the string if it cannot be parsed.

    Known limitations:
    Octal number not in Python 3.x octal format e.g, "0100" will not be parsed properly.
    0o0100 will be parsed to octal integer.
    String will be returned
    """
    if isinstance(s, str):
        if s == 'null':
            val = None
            return val
        else:
            try:
                val = ast.literal_eval(s)
                if not isinstance(val, (int, float, str, object)):
                    val = s
            except:
                # Let's try if it is JSON Bool values
                bool_lits = {"true": True, "false": False}
                if s in bool_lits:
                    val = bool_lits[s]
                else:
                    val = s
            return val
    else:
        return s


def boolean(val):
    if val.casefold() in ["on", "true"]:
        return True
    return False


class JSON:
    def __init__(self):
        self.__dict = {}  # Private variable

    def serialize(self, ind=2):
        json_string = json.dumps(self, default=lambda o: o.__dict, indent=ind, separators=(",", ":"))
        out = ""
        for line in json_string.splitlines():
            spaces = len(line) - len(line.lstrip(' '))
            if "{}" in line:
                out += line.format("{" + "\n" + " " * spaces + "}\n")
            else:
                out += line + "\n"
        return out

    def __bool__(self):
        """
        Override the __bool__ to return True or False based on JSON class private var __dict is empty.
        """
        return bool(self.__dict)

    def add(self, key, val):
        if val is not None:
            if isinstance(val, str) and val:
                self.__dict.__setitem__(key, convert_string(val))
            else:
                if val:
                    self.__dict.__setitem__(key, val)

    def update(self, other):
        self.__dict.update(other.__dict)

    def __setattr__(self, key, value):
        """
        Override the __setattr__ to block users from assigning instance variables directly
        But allow the initialisation of private variables as part of __init__
        Key of private variables begin with _<classname>__
        """
        if key.startswith(f"_{self.__class__.__name__}__"):
            object.__setattr__(self, key, value)
            return
        raise TypeError('Attributes can only be set with add() or add_non_empty() methods')
