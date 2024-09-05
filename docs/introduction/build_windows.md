To build the Thunder framework and its components on Windows, you need to have Visual Studio installed. The free Community Edition is sufficient if you are entitled to use it in your situation, and to make sure of that please check the license.

The main solution file, containing all projects and their dependencies, is located in the [ThunderOnWindows](https://github.com/WebPlatformForEmbedded/ThunderOnWindows) repository. This repository also includes some binaries and the header files required to build the Thunder framework on Windows.

!!! note
	The `ThunderOnWindows` repository includes submodules for other Thunder repositories. These submodules may not always be up to date, so it is recommended to manually clone the necessary repositories to ensure you have the correct versions.

## 1. Install Dependencies
Thunder uses Python 3 for code and documentation generation scripts. Ensure that you have at least **Python 3.5** installed. On Windows 10 and above, Python can be installed via the Microsoft Store. Refer to [this guide](https://learn.microsoft.com/en-us/windows/python/beginners) for instructions.

Next, install the required Python packages:

- Install the [**jsonref**](https://pypi.org/project/jsonref/) package:
	```
	pip install jsonref
	```

- Install the [**six**](https://pypi.org/project/six/) package:
	```
	pip install six
	```

## 2. Clone All Repositories

- Make a dedicated folder called `ThunderWin` directly on the drive `C:\`, clone ThunderOnWindows into it and change the directory.
	```
	mkdir C:\ThunderWin
	cd C:\ThunderWin
	git clone https://github.com/WebPlatformForEmbedded/ThunderOnWindows.git
	cd ThunderOnWindows
	```

- Then, clone the remaining repos.
	```
 	git clone https://github.com/rdkcentral/Thunder.git
	git clone https://github.com/rdkcentral/ThunderTools.git
	git clone https://github.com/rdkcentral/ThunderInterfaces.git
	git clone https://github.com/rdkcentral/ThunderClientLibraries.git
	git clone https://github.com/rdkcentral/ThunderNanoServices.git
	git clone https://github.com/WebPlatformForEmbedded/ThunderNanoServicesRDK.git
	git clone https://github.com/rdkcentral/ThunderUI.git
	```

## 3. Build the Thunder Framework

1. **Open the Solution File**: Open the main solution file located in the `ThunderOnWindows` repository using Visual Studio.

2. **Restore NuGet Packages**: Before building, make sure all required NuGet packages are restored. You can do this by right-clicking on the solution in the Solution Explorer and selecting `Restore NuGet Packages`.

3. **Build the Solution**: To build the entire solution, click on `Build > Build Solution` in the Visual Studio menu. This will build all the projects in the solution and generate the necessary binaries.

4. **Check for Errors**: Ensure that the build completes without errors. If there are errors, they will be listed in the `Error List` window at the bottom of the Visual Studio interface.

!!! hint
	Some of the project names in the Visual Studio solution reflect old project names - e.g. Thunder is known as `Bridge`, reflecting its original WebBridge codename.

If you are interested in building only a specific part of Thunder, for example just ThunderInterfaces, you can build only the `Interfaces` project file and it will automatically build its dependencies, so in this case `bridge`.

## 4. Configure Artifacts

After the building process is finished, you still need to make a few adjustments before running Thunder.

- First, create a volatile and a persistent directory in a specific location, as well as a directory for the necessary dlls, which can be done with the following commands:
	```
	mkdir ..\artifacts\Debug
	mkdir ..\artifacts\Persistent
	mkdir ..\artifacts\temp\MessageDispatcher
	```

- Next, move two dlls into the artifacts folder:
	```
	move lib\static_x64\libcrypto-1_1-x64.dll ..\artifacts\Debug\libcrypto-1_1-x64.dll
	move lib\static_x64\libssl-1_1-x64.dll ..\artifacts\Debug\libssl-1_1-x64.dll
	```

- To use ThunderUI on Windows, copy it into the artifacts folder:
	```
	robocopy ThunderUI\dist ..\artifacts\Debug\Plugins\Controller\UI /S
	```

## 5. Run the Thunder Framework

Once the build process is complete, you can run the Thunder framework:

1. **Set the Startup Project**: In Visual Studio, right-click on the project you want to run (usually the `bridge` project) and select `Set as StartUp Project`.

2. **Set Command Arguments**: Right click on the `bridge` project file and select `Properties`. Go into the `Debugging` tab, and make sure to put the following line into `Command Arguments`:
	```
	-f -c "$(ProjectDir)ExampleConfigWindows.json"
	```

!!! note
	Remove the ```-f``` flag if you want to see the messages in ThunderUI under the ```Messaging``` tab, otherwise they will be displayed in the console.

3. **Run the Project**: Press `F5` to run the project in Debug mode or `Ctrl + F5` to run it without debugging. Visual Studio will start the project, and you should see the Thunder framework running in the foreground.

4. **Verify Functionality**: Check the output window in Visual Studio for any logs or error messages. This will help ensure that the framework is running as expected.

5. **ThunderUI**: If you want to display the ThunderUI, you can do that by going into this address in your browser: [127.0.0.1:25555](http://127.0.0.1:25555/Service/Controller/UI).
