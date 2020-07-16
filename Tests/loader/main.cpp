#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../Source/core/core.h"

static std::list<string> _loadLocation;

static const char* _defaultSet[] = {
    "/usr/lib/libWPEFrameworkCore.so"
    "/usr/lib/libWPEFrameworkTracing.so", 
    "/usr/lib/libWPEFrameworkCryptalgo.so", 
    "/usr/lib/libWPEFrameworkProtocols.so",
    "/usr/lib/libWPEFrameworkPlugins.so",
    "/usr/lib/wpeframework/plugins",
    nullptr };

bool ParseOptions (int argc, char** argv)
{
    int index = 1;
    bool showHelp = false;

    while ( (index < argc) && (showHelp == false) ) {

        if (strcmp (argv[index], "-l") == 0) {

            if ( ((index + 1) < argc) && (argv[index+1][0] != '-') ) {

                index++;
                _loadLocation.push_back(string(argv[index]));
            }
        }
        else if (strcmp (argv[index], "-d") == 0) {

            const char** setIndex = &(_defaultSet[0]);
            string prefix;

            if ( ((index + 1) < argc) && (argv[index+1][0] != '-') ) {

                index++;
                prefix = (argv[index]);
            }

            while (*setIndex != nullptr) {
                _loadLocation.push_back (prefix + string(*setIndex));
                setIndex++;
            } 

            index++;
        }
        else {
            showHelp = true;
        }
        index++;
    }

    if (showHelp == true)
    {
        printf("Verification and Analyze tool for WPEFramework Software.\n");
        printf("verify -d [prefix_path] -l [directories or files]\n");
        printf("  -d:  Use the default WPEFramework Software libraries deployed.\n");
        printf("  -l:  Location of a file, or a directory that holds *.so files. These files\n");
        printf("       will be loaded and their versione (build tags) will be printed.\n\n");
    }

    return (showHelp == false);
}

typedef const char* (*ModuleName)();
typedef const char* (*ModuleBuildRef)();

void ReadFile (const WPEFramework::Core::File& file) {

    WPEFramework::Core::Library library(file.Name().c_str());

    if (library.IsLoaded() == true)
    {
        // extern "C" { namespace WPEFramework { namespace Core { namespace System {  
        //     const char* ModuleName();
        //     const char* ModuleBuildRef();
        // }}}}
        ModuleName moduleName = reinterpret_cast<ModuleName>(library.LoadFunction(_T("ModuleName")));
        ModuleBuildRef moduleBuildRef = reinterpret_cast<ModuleBuildRef>(library.LoadFunction(_T("ModuleBuildRef")));

        printf ("\nLoaded library:  %s\n", file.Name().c_str());
        if (moduleName != nullptr) {
            printf ("   Module name:  %s\n", moduleName());
        }
        else {
            printf ("   Module name:  [UNKNOWN]\n");
        }
        if (moduleBuildRef != nullptr) {
            printf ("   Module build: %s\n", moduleBuildRef());
        }
        else {
            printf ("   Module build: [UNKNOWN]\n");
        }
    }
    else
    {
        printf ("Loaded library:  %s\n", file.Name().c_str());
        printf ("   Error:        %s\n", library.Error().c_str());
    }
}

void ReadDirectory (const string& name) {

    WPEFramework::Core::Directory pluginDirectory (name.c_str());

    while (pluginDirectory.Next() == true) {

        WPEFramework::Core::File file (pluginDirectory.Current(), true);

        if (file.Exists()) {
            if (file.IsDirectory()) {
                if ( (file.FileName() != ".") && (file.FileName() != "..") ) {
                    ReadDirectory(file.Name());
                }
            }
            else {
                ReadFile(file);
            }
        }
    } 
}

int main(int argc, char *argv[]) {

    if (ParseOptions(argc, argv) == true) {

        printf ("Iterating over %u entries.\n\n", static_cast<unsigned int>(_loadLocation.size()));

        std::list<string>::const_iterator index(_loadLocation.begin());

        while (index != _loadLocation.end()) {
            WPEFramework::Core::File file (*index, true);
            if (file.Exists() == true) {
                if (file.IsDirectory() == true) {
                    ReadDirectory(index->c_str());
                }
                else {
                    ReadFile(file);
                }
            }
            index++;
        }
    }
  
    return 0;
}
