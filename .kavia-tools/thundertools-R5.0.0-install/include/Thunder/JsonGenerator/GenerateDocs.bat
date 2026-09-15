@rem If not stated otherwise in this file or this component's LICENSE file the
@rem following copyright and licenses apply:
@rem
@rem Copyright 2020 Metrological
@rem
@rem Licensed under the Apache License, Version 2.0 (the "License");
@rem you may not use this file except in compliance with the License.
@rem You may obtain a copy of the License at
@rem
@rem http://www.apache.org/licenses/LICENSE-2.0
@rem
@rem Unless required by applicable law or agreed to in writing, software
@rem distributed under the License is distributed on an "AS IS" BASIS,
@rem WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
@rem See the License for the specific language governing permissions and
@rem limitations under the License.

@rem
@rem Generates plugin Markdown documentation
@rem 
@rem Typical usage:
@rem    GenerateDocs.bat ..\..\..\ThunderNanoServices
@rem    GenerateDocs.bat ..\..\..\ThunderNanoServices\BluetoothControl
@rem    GenerateDocs.bat ..\..\..\ThunderNanoServices\BluetoothControl\BluetoothControlPlugin.json
@rem 

@echo off

if not exist JsonGenerator.py (
    echo JsonGenerator not found!
    exit /b 0
)

set COMMAND=python JsonGenerator.py --docs -i ../../Source/interfaces/json -j ../../Source/interfaces -o doc 

if "%1"=="" (
    echo Usage %0 [JSONFILE] OR [DIRECTORY]
    echo Note: Directory scan is recursive
    exit /b 0
)

echo Generating plugin Markdown documentation...

if exist %1\NUL (
    for /f "delims=" %%i in ('dir /s/b %1\*Plugin.json') do (
        %COMMAND% %%i
    )
) else (
    %COMMAND% %1
)

echo Complete.
