import json

true = True
false = False


class JSON:
    def to_json(self, ind=2):
        return json.dumps(self, default=lambda o: o.__dict__, indent=ind, separators=(",", ":"))

    def add_non_empty(self, key, val=None):
        if val is not None:
            if isinstance(val, str):
                if val is not "":
                    self.add(key, val)
            else:
                self.add(key, val)

    @classmethod
    def to_bool(cls, val=None):
        if val is not None:
            if val == "ON":
                return True
        return False

    def add(self, key, val):
        self.__dict__.__setitem__(key, val)

    def update(self, other):
        self.__dict__.update(other.__dict__)
