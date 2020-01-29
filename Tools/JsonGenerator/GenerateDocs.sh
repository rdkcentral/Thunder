#!/bin/bash

#
# Generates Plugin Markdown documentation.
#
# Typical usage:
#   ./GenerateDocs ../../../ThunderNanoServices/
#   ./GenerateDocs ../../../WPEFrameworkPlugins/BluetoothControl
#   ./GenerateDocs ../../../WPEFrameworkPlugins/BluetoothControl/BluetoothControlPlugin.json
#

command -v ./JsonGenerator.py >/dev/null 2>&1 || { echo >&2 "JsonGenerator.py is not available. Aborting."; exit 1; }

if [ -d ]; then
   files=`find $1 -name "*Plugin.json"`
elif [ $# -gt 0 ]; then
   files=$@
else
   echo >&2 "usage: $0 [-d <directory>|<json> ...]"
   echo Note: Directory scan is recursive
   exit 0
fi

echo "Generating Plugin markdown documentation..."

./JsonGenerator.py --docs -i ../../Source/interfaces/json -j ../../Source/interfaces -o doc $files

echo "Complete."
