import sys
import os

class Log:
    def __init__(self,name,verbose = False,warning = True, doc_issues = False):
        self.warnings = []
        self.errors = []
        self.infos = []
        self.be_verbose = verbose
        self.warning = warning
        self.doc_issues = doc_issues
        self.name = name
        self.file = ""
        if os.name == "posix":
            self.info      = "\033[0mINFO"
            self.cwarn     = "\033[33mWARNING"
            self.cerror    = "\033[31mERROR"
            self.cdocissue = "\033[37mDOC-ISSUE"
            self.creset    = "\033[0m"
        else:
            self.info      = "INFO:"
            self.cwarn = "WARNING:"
            self.cerror = "ERROR:"
            self.cdocissue = "DOC-ISSUE:"
            self.creset = ""

    def Info(self, text, file=""):
        if self.be_verbose:
            self.infos.append("%s: %s: %s%s%s" % (self.name, self.info, file, ": " if file else "", text))
            self.Print(self.infos[-1])
    
    def DocIssue(self, text):
        if self.doc_issues:
            self.Print("%s: %s%s %s" % (self.file, self.cdocissue, self.creset, text))
            

    def Warn(self, text, file=""):
        if self.warning:
            self.warnings.append("%s: %s%s: %s%s%s" % (self.name, self.cwarn, self.creset, file, ": " if file else "", text))
            self.Print(self.warnings[-1])

    def Error(self, text, file=""):
        self.errors.append("%s: %s%s: %s%s%s" % (self.name, self.cerror, self.creset, file, ": " if file else "", text))
        self.Print(self.errors[-1], file=sys.stderr)

    def Print(self, text, file=""):
        print("%s: %s%s%s" % (self.name, file, ": " if file else "", text))

    def Dump(self):
        if self.errors or self.warnings or self.infos:
            self.Print("")
            for item in self.errors + self.warnings + self.infos:
                self.Print(item)

    def Header(self, text):
        self.Info("Processing file %s..." % text)
        self.file = text

    def Ellipsis(self,text, front=True):
        if front:
            return (text[:32] + '...') if len(text) > 32 else text
        else:
            return ("..." + text[-32:]) if len(text) > 32 else text
    
    def Success(self, text):
        self.Print("Success: {}".format(text))



