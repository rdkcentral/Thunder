#!/bin/bash

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

#
# Generates Plugin Markdown documentation.
#
# Typical usage:
#   ./GenerateDocs ../../ThunderNanoServices/
#   ./GenerateDocs ../../ThunderNanoServices/BluetoothControl
#   ./GenerateDocs ../../ThunderNanoServices/BluetoothControl/BluetoothControlPlugin.json
#

command -v ./JsonGenerator.py >/dev/null 2>&1 || { echo >&2 "JsonGenerator.py is not available. Aborting."; exit 1; }

if [ $# -eq 1 ] && [ -d $1 ]; then
   files=`find $1 -name "*Plugin.json"`
elif [ $# -gt 0 ]; then
   files=$@
else
   echo >&2 "usage: $0 [-d <directory>|<json> ...]"
   echo Note: Directory scan is recursive
   exit 0
fi

echo "Generating Plugin markdown documentation..."

./JsonGenerator.py --docs -i ../../ThunderInterfaces/jsonrpc -i ../../ThunderInterfaces/qa_jsonrpc -i ../../ThunderInterfaces/example_jsonrpc -j ../../ThunderInterfaces/interfaces -j ../../ThunderInterfaces/qa_interfaces -j ../../ThunderInterfaces/example_interfaces -I ../../Thunder/Source --namespace Thunder::Example --namespace Thunder::QualityAssurance --namespace Thunder::Exchange --namespace Thunder::Exchange::JSORPC -o doc --verbose $files

echo "Complete."
