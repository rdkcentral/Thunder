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
