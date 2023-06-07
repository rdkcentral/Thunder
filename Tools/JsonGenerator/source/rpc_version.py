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

def GetVersion(info):
    version = []

    if "version" in info:
        if isinstance(info["version"], list):
            version = info["version"][:3]
        elif isinstance(info["version"], str):
            version = [int(x) for x in info["version"].split('.')[:3]]
        else:
            raise RuntimeError('version needs to be a "major.min.patch" string or a [major, minor, patch] list of integers')

    if len(version) == 0:
        version.append(1)
    if len(version) <= 1:
        version.append(0)
    if len(version) <= 2:
        version.append(0)

    return version

def GetVersionString(info):
    return ".".join(str(x) for x in GetVersion(info))
