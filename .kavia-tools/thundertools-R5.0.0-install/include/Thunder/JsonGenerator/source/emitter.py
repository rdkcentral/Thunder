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

class Emitter():
    def __init__(self, file_name, indent_size, max_line_length = 160):
        self.file = open(file_name, "w") if file_name else None
        self.indent_size = indent_size
        self.indent = 0
        self.threshold = max_line_length
        self.lines = []

    def __del__(self):
        pass

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        if self.file:
            self.Flush()
            self.file.close()

    def Line(self, text = ""):
        if text != "":
            commented = "// " if "//" in text else ""
            text = (" " * self.indent) + str(text)
            iteration = 1

            while len(text) > (self.threshold * iteration):
                index = text.rfind(",", 0, self.threshold * iteration)
                if index > 0:
                    text = text[0:index+1] + "\n" + " " * (self.indent + self.indent_size * 2) + commented + text[index + 1 + (1 if text[index] == ', ' else 0):]
                    iteration += 1
                else:
                    break

            self.lines.append(text)

        elif len(self.lines) and self.lines[-1] != "":
            self.lines.append("")


    def Indent(self):
        self.indent += self.indent_size

    def Unindent(self):
        if self.indent >= self.indent_size:
            self.indent -= self.indent_size
        else:
            self.indent = 0

    def Prepend(self, other):
        self.lines = other.lines + self.lines

    def Append(self, other):
        self.lines.extend(other.lines)

    def FileName(self):
        return self.file.name if self.file else None

    def Flush(self):
        if self.file:
            for line in self.lines:
                self.file.write(line + "\n")
