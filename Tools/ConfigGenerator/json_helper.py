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
        try:
            val = ast.literal_eval(s)
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


def to_bool(val):
    if val.casefold() == "on" or val.casefold() == "true":
        return True
    return False


class JSON:
    def __init__(self):
        self.__dict = {}  # Private variable

    def to_json(self, ind=2):
        json_string = json.dumps(self, default=lambda o: o.__dict, indent=ind, separators=(",", ":"))
        out = ""
        for line in json_string.splitlines():
            spaces = len(line) - len(line.lstrip(' '))
            if "{}" in line:
                out += line.format("{" + "\n" + " " * spaces + "}\n")
            else:
                out += line + "\n"
        return out

    def add_non_empty(self, key, val):
        if val is not None:
            if isinstance(val, str):
                if val != "":
                    self.add(key, val)
            else:
                self.add(key, val)

    def add(self, key, val):
        self.__dict.__setitem__(key, convert_string(val))

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
