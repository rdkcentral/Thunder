import sys

class Log:
    def __init__(self,name,verbose = False,warning = True):
        self.warnings = []
        self.errors = []
        self.infos = []
        self.BE_VERBOSE = verbose
        self.warning = warning
        self.name= name

    def Info(self, text, file=""):
        if self.BE_VERBOSE:
            self.infos.append("%s: INFO: %s%s%s" % (self.name, file, ": " if file else "", text))
            print(self.infos[-1])

    def Warn(self, text, file=""):
        if self.SHOW_WARNINGS:
            self.warnings.append("%s: WARNING: %s%s%s" % (self.name, file, ": " if file else "", text))
            print(self.warnings[-1])

    def Error(self, text, file=""):
        self.errors.append("%s: ERROR: %s%s%s" % (self.name, file, ": " if file else "", text))
        print(self.errors[-1], file=sys.stderr)

    def Print(self, text, file=""):
        print("%s: %s%s%s" % (self.name, file, ": " if file else "", text))

    def Dump(self):
        if self.errors or self.warnings or self.infos:
            print("")
            for item in self.errors + self.warnings + self.infos:
                print(item)

