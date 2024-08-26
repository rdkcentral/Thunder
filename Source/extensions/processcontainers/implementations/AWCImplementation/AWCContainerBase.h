#pragma once

#include "common/BaseRefCount.h"
#include "processcontainers/ProcessContainer.h"

namespace Thunder {
namespace ProcessContainers {

class AWCContainerBase:
    public BaseRefCount<IContainer>
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
