# WPEFRAMEWORK

A C++ platform abstraction layer for generic functionality.

## Create a Eclipse project 
### Prerequisites: 
* Cmake installed
* Toolchain e.g. host directory of buildroot
* Eclipse IDE for C/C++ developers 

### Generate project
If using the buildroot toolchain. Copy the _toolchainfile.cmake_ from _output/host/usr/share/buildroot_ to _output/host_

Enter the source directory and execute:

`mkdir build`

`cmake -G"Eclipse CDT4 - Unix Makefiles" -D_ECLIPSE_VERSION=<Your Eclipse version e.g 4.5> -D CMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=<path to copied toolchainfile.cmake> -DWPEFRAMEWORK_PLATFORM=<platform>  <WPEFramework options> --build ./build .`

In Eclipse
_File > Import... > General > Existing Projects into Workspace_

Check _Select root directory_ and choose your WPEFRAMEWORK source directory . Make sure _Copy projects into workspace_ is **NOT** checked.

Click _Finish_ and you have a WPEFRAMEWORK as a project in your workspace.

### WPEFRAMEWORK Options
**Platforms:**
* INTELCE
* RPI
* PC_UNIX
* PC_WINDOWS
* EOS
* DAWN
* Xi5

**WPEFRAMEWORK options:**
*  -DWPEFRAMEWORK_CORE=ON ,Include the generics library.
*  -DWPEFRAMEWORK_CRYPTALGO=ON ,Include the encyption algorithm library.
*  -DWPEFRAMEWORK_PROTOCOLS=ON ,Include the protocols library.
*  -DWPEFRAMEWORK_TRACING=ON ,Include the tracing library.
*  -DWPEFRAMEWORK_PLUGINS=ON ,Include the plugin library.

*  -DWPEFRAMEWORK_DEBUG=ON ,Enable debug build for ccpsdk.
*  -DWPEFRAMEWORK_WCHAR_SUPPORT=ON ,Enable support for WCHAR in ccpsdk.
*  -DWPEFRAMEWORK_TEST_APPS=ON ,Include test applications. (See test apps for options)
*  -DWPEFRAMEWORK_UNIT_TESTS=ON ,Include unit tests.
*  -DWPEFRAMEWORK_TEST_MEASUREMENTS=ON ,Include code coverage measurments.

**WPEFRAMEWORK Test apps:**
* -DWPEFRAMEWORK_TEST_IPCTEST=ON ,include the IPC tests.
* -DWPEFRAMEWORK_TEST_JSONTESTS=ON ,include the JSON tests.
* -DWPEFRAMEWORK_TEST_MEMORYTEST=ON ,include the memory tests.
* -DWPEFRAMEWORK_TEST_NETWORKTEST=ON ,include the network tests.
* -DWPEFRAMEWORK_TEST_QTWEBINSPECTOR=ON ,include the QT web inspector tests. 
* -DWPEFRAMEWORK_TEST_TESTCONSOLE=ON ,include the test console tests.
* -DWPEFRAMEWORK_TEST_TESTSERVER=ON ,include the test server tests.
* -DWPEFRAMEWORK_TEST_WEBINSPECTOR=ON ,include the web inspector tests.
* -DWPEFRAMEWORK_TEST_DEVICEID=ON ,include the web inspector tests.
