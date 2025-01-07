#pragma once

#include "processcontainers/ContainerAdministrator.h"

namespace Thunder {
namespace ProcessContainers {

class AWCContainerBase:
    public IContainer
{
private:
    string id_;
public:
    AWCContainerBase(std::string id): id_{std::move(id)} {}

    const string& Id() const override {return id_;}
    IMemoryInfo* Memory() const override;
    IProcessorInfo* ProcessorInfo() const override;
    INetworkInterfaceIterator* NetworkInterfaces() const override;
};

} /* ProcessContainers */
} /* Thunder */
