import sys
import os

class Log:
    def __init__(self, name, verbose = False, warnings = True, doc_issues = False):
        self.warnings = []
        self.errors = []
        self.infos = []
        self.show_infos = verbose
        self.show_warnings = warnings
        self.show_doc_issues = doc_issues
        self.name = name
        self.file = ""
        if os.name == "posix":
            self.info      = "\033[0mINFO"
            self.cwarn     = "\033[33mWARNING"
            self.cerror    = "\033[31mERROR"
            self.cdocissue = "\033[37mDOCUMENTATION"
            self.creset    = "\033[0m"
        else:
            self.info      = "INFO:"
            self.cwarn = "WARNING:"
            self.cerror = "ERROR:"
            self.cdocissue = "DOCUMENTATION:"
            self.creset = ""

    def __Print(self, text, file=""):
        print("%s%s%s" % (file, ": " if file else "", text))

    def Info(self, text, file=""):
        if self.show_infos:
            if not file: file = self.file
            self.infos.append("%s: %s: %s%s%s" % (self.name, self.info, file, ": " if file else "", text))
            self.__Print(self.infos[-1])

    def DocIssue(self, text, file=""):
        if self.show_doc_issues:
            if not file: file = self.file
            self.__Print("%s: %s%s: %s%s%s" % (self.name, self.cdocissue, self.creset, file, ": " if file else "", text))

    def Warn(self, text, file=""):
        if self.show_warnings:
            if not file: file = self.file
            self.warnings.append("%s: %s%s: %s%s%s" % (self.name, self.cwarn, self.creset, file, ": " if file else "", text))
            self.__Print(self.warnings[-1])

    def WarnLine(self, obj, text, file=""):
        if self.show_warnings:
            if not file: file = self.file
            try:
                if not file: file = os.path.basename(obj.parser_file)
                line = str(obj.parser_line)
            except:
                try:
                    file = os.path.basename(obj.parent.parser_file)
                    line = obj.parent.parser_line
                except:
                    file = ""
                    line = ""
            self.warnings.append("%s: %s%s: %s%s" % (self.name, self.cwarn, self.creset, ("%s(%s): " % (file, line)) if file else "", text))
            self.__Print(self.warnings[-1])

    def Error(self, text, file=""):
        if not file: file = self.file
        self.errors.append("%s: %s%s: %s%s%s" % (self.name, self.cerror, self.creset, file, ": " if file else "", text))
        self.__Print(self.errors[-1])

    def Print(self, text, file=""):
        if not file: file = self.file
        print("%s: %s%s%s" % (self.name, file, ": " if file else "", text))

    def Dump(self):
        if self.errors or self.warnings or self.infos:
            self.Print("")
            for item in self.errors + self.warnings + self.infos:
                self.__Print(item)

    def Header(self, text):
        if self.infos:
            self.Print("Processing file %s..." % text)
        self.file = os.path.basename(text)

    def Ellipsis(self,text, front=True):
        if front:
            return (text[:32] + '...') if len(text) > 32 else text
        else:
            return ("..." + text[-32:]) if len(text) > 32 else text

    def Success(self, text):
        self.Print("Success: {}".format(text))



