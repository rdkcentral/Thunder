@rem If not stated otherwise in this file or this component's LICENSE file the
@rem following copyright and licenses apply:
@rem
@rem Copyright 2023 Metrological
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
@rem Generates interface data in lua
@rem
@rem Typical usage:
@rem    GenerateLua.bat [<Thunder_dir> <ThunderInterfaces_dir> <ThunderClientLibraries_dir>]
@rem    GenerateLua.bat ..\..\..\Thunder ..\..\..\ThunderInterfaces ..\..\..\ThunderClientLibraries
@rem

@echo off

if not exist ..\ProxyStubGenerator\StubGenerator.py (
    echo StubGenerator is not available! Aborting.
    exit /b 0
)

set THUNDER_DIR=..\..\..\Thunder
set INTERFACES_DIR=..\..\..\ThunderInterfaces
set CLIENTLIBRARIES_DIR=..\..\..\ThunderClientLibraries

set files=%THUNDER_DIR%\Source\com\ICOM.h %THUNDER_DIR%\Source\com\ITrace.h
set files=%files% %THUNDER_DIR%\Source\plugins\IPlugin.h %THUNDER_DIR%\Source\plugins\IShell.h %THUNDER_DIR%\Source\plugins\IStateControl.h %THUNDER_DIR%\Source\plugins\ISubSystem.h
set files=%files% %INTERFACES_DIR%\interfaces\I*.h
set files=%files% %CLIENTLIBRARIES_DIR%\Source\cryptography\I*.h
rem add more interface files if needed..

echo Generating lua data file...

python ..\ProxyStubGenerator\StubGenerator.py --lua-code %files% --outdir . -I %THUNDER_DIR%\Source -i %THUNDER_DIR%\Source\com\Ids.h

echo Complete.
