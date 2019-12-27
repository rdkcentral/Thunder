#include "memoryoverview.h"

#include <iostream>
#include <fstream>
#include <sstream>

namespace WPEFramework {
namespace MemoryOverview {

void MemoryOverview::ClearOSCaches() {
    std::ofstream file {"/proc/sys/vm/drop_caches", std::ofstream::out};
    file << "3";
    file.close();
    if( file.good() == true ) {
        TRACE_L1("OS Caches cleared");
    }
    else {
        TRACE_L1("OS Caches could not be cleared");
    }
}

static void VisitChilds(Core::ProcessInfo::Iterator& it, std::ostringstream& stream) {
    bool available = it.Next();
    while( available == true ) {
        stream << "{\"executable\" : " << '"' << it.Current().Executable() << "\",";

        Core::ProcessInfo::LibraryIterator libiterator( it.Current().Libraries() );
        stream << "\"libraries\" : [\n";
        bool libavailable = libiterator.Next();
        while( libavailable == true ) {
            stream << "\t{\"name\" : \"" << libiterator.Current().Shortname() << "\",\"fullname\" : \"" <<  libiterator.Current().Name() << "\"}";
            libavailable = libiterator.Next();
            if( libavailable == true ) {
                stream << ",\n";
            }
        }
        stream << "\n],\n\"childs\" : [" << std::endl;

        Core::ProcessInfo::Iterator childs(it.Current().Children());

        VisitChilds(childs, stream);

        stream << "]\n}" << std::endl;

        available = it.Next();
        if( available == true ) {
            stream << ",\n";
        }
    }
}

std::string MemoryOverview::GetDependencies() {
    Core::ProcessInfo::Iterator childiterator("bash", "memapp_parent", true);

    std::ostringstream stream;

    stream << "{\n";
    stream << "\"process\" : \"" << childiterator.Current().Executable() << "\"\n";
    stream << "\"instances\" : [\n";

    VisitChilds(childiterator, stream);
 
    stream << "\n]\n}\n";   
    return stream.str();
}


}
}