#!/bin/bash

# If not stated otherwise in this file or this component's license file the
# following copyright and licenses apply:
#
# Copyright 2023 Metrological
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
# Generates interface data in lua
#
# Typical usage:
#   ./GenerateLua.sh [<Thunder_dir> <ThunderInterfaces_dir> <ThunderClientLibraries_dir>]
#   ./GenerateLua.sh
#   ./GenerateLua.sh ../../../Thunder ../../../ThunderInterfaces ../../../ThunderClientLibraries
#   ./GenerateLua.sh ~/work/Thunder ~/work/ThunderInterfaces ~/work/ThunderClientLibraries
#

command -v ../ProxyStubGenerator/StubGenerator.py >/dev/null 2>&1 || { echo >&2 "StubGenerator.py is not available. Aborting."; exit 1; }

THUNDER_DIR="${1:-../../../Thunder}"
INTERFACES_DIR="${2:-../../../ThunderInterfaces}"
CLIENTLIBRARIES_DIR="${3:-../../../ThunderClientLibraries}"

files="$THUNDER_DIR/Source/com/ICOM.h $THUNDER_DIR/Source/com/ITrace.h"
files="$files $THUNDER_DIR/Source/plugins/IPlugin.h $THUNDER_DIR/Source/plugins/IShell.h $THUNDER_DIR/Source/plugins/IStateControl.h $THUNDER_DIR/Source/plugins/ISubSystem.h"
files="$files $INTERFACES_DIR/interfaces/I*.h"
files="$files $CLIENTLIBRARIES_DIR/Source/cryptography/I*.h"
# add more interface files if needed..

echo "Generating lua data file..."

../ProxyStubGenerator/StubGenerator.py --lua-code $files --outdir . -I $THUNDER_DIR/Source -i $THUNDER_DIR/Source/com/Ids.h

echo "Complete."
