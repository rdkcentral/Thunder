#!/bin/bash

#
# Generates Plugin Markdown documentation.
#
# Example:
#   ./GenerateDocs ../../../WPEFrameworkPlugins/Streamer/StreamerPlugin.json
#   ./GenerateDocs ../../../WPEFrameworkPlugins/Streamer/StreamerPlugin.json ../../../WPEFrameworkPlugins/TimeSync/TimeSyncPlugin.json
#   ./GenerateDocs -d ../../../WPEFrameworkPlugins
#

command -v ./JsonGenerator.py >/dev/null 2>&1 || { echo >&2 "JsonGenerator.py is not available. Aborting."; exit 1; }

if [ "$1" = "-d" ]; then
   files=`find $2 -name "*Plugin.json"`
elif [ $# -gt 0 ]; then
   files=$@
else
   echo >&2 "usage: $0 [-d <directory>|<json> ...]"
   exit 0
fi

echo "Generating Plugin markdown documentation..."

./JsonGenerator.py --docs -i ../../Source/interfaces/json $files -o doc

echo "Done."
