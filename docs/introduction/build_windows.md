To build Thunder and its components on Windows, you will need Visual Studio installed -  the free community edition is just fine.

The main solution file with all projects and their  dependencies can be found in the [ThunderOnWindows](https://github.com/WebPlatformForEmbedded/ThunderOnWindows) repo. This repository also holds some binaries and the header files required to build the Thunder framework on Windows. 

!!! note
	The `ThunderOnWindows` repo contains some submodules for other Thunder repos. These aren't necessarily kept up to date, so it is recommended to just manually clone the other repos to ensure you get the versions you require

## 1. Install Dependencies
Thunder uses Python 3 for code and documentation generation scripts. Ensure you have at least **Python 3.5** installed. On Windows 10 and above, this can be done from the Microsoft Store, see [here](https://learn.microsoft.com/en-us/windows/python/beginners) for instructions.

Install the [**jsonref**](https://pypi.org/project/jsonref/) library with pip:

```
pip install jsonref
```


## 2. Clone All Repositories

Make a dedicated folder called `ThunderWin` directly on the drive `C:\`, clone ThunderOnWindows into it and change the directory.

```
mkdir C:\ThunderWin
cd C:\ThunderWin
git clone https://github.com/WebPlatformForEmbedded/ThunderOnWindows.git
cd ThunderOnWindows
```

Then, clone the remaining repos.

```
git clone https://github.com/rdkcentral/ThunderTools.git
git clone https://github.com/rdkcentral/Thunder.git
git clone https://github.com/rdkcentral/ThunderInterfaces.git
git clone https://github.com/rdkcentral/ThunderClientLibraries.git
git clone https://github.com/rdkcentral/ThunderNanoServices.git
git clone https://github.com/WebPlatformForEmbedded/ThunderNanoServicesRDK.git
git clone https://github.com/rdkcentral/ThunderUI.git
```

## 3. Build

The next step is to open the solution file `ThunderOnWindows\Thunder.sln` in  Visual Studio, right click on `Solution Thunder` and build it. This will build all project files in a similar order to the Linux cmake build. 

!!! hint
	Some of the project names in the Visual Studio solution reflect old project names - e.g. Thunder is known as `Bridge`, reflecting its original WebBridge codename

If you are interested in building only a specific part of  Thunder, for example just ThunderInterfaces, you can build only the `Interfaces` project file and it will automatically build its dependencies, so in this case `bridge`.

## 4. Configure Artifacts

After the building process is finished, you still need to make a few adjustments before running Thunder. One of them is to create a volatile and a persistent directory in a specific location, this can be done with the following commands:

```
mkdir ..\artifacts\temp
mkdir ..\artifacts\Persistent
```


Move two dlls with libs into the artifacts folder:

```
move lib\static_x64\libcrypto-1_1-x64.dll ..\artifacts\Debug\libcrypto-1_1-x64.dll
move lib\static_x64\libssl-1_1-x64.dll ..\artifacts\Debug\libssl-1_1-x64.dll
```

To use ThunderUI on Windows, copy it into the artifacts folder:

```
robocopy ThunderUI\dist ..\artifacts\Debug\Plugins\Controller\UI /S
```

## 5. Run

Right click on `bridge` project file and select `Properties`. Go into `Debugging` tab, and put the following line into `Command Arguments`:

```
-f -c "$(ProjectDir)ExampleConfigWindows.json"
```

![https://camo.githubusercontent.com/66a3000de0d428e7c860e502434b7ca9153adc3b2cabd3c9123e50a007f4a998/68747470733a2f2f692e696d6775722e636f6d2f706267415853562e706e67](https://camo.githubusercontent.com/66a3000de0d428e7c860e502434b7ca9153adc3b2cabd3c9123e50a007f4a998/68747470733a2f2f692e696d6775722e636f6d2f706267415853562e706e67)

Apply the changes, and press `F5` to run Thunder
