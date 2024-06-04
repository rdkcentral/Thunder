#include "AWCContainerBase.h"

using namespace Thunder::ProcessContainers;

struct AWCMemoryInfo : public BaseRefCount<IMemoryInfo> {
    AWCMemoryInfo()
    {
    }

    uint64_t Allocated() const
    {
        return 0;
    }

    uint64_t Resident() const
    {
        return 0;
    }

    uint64_t Shared() const
    {
        return 0;
    }
};

struct AWCProcessorInfo : public BaseRefCount<IProcessorInfo> {
    AWCProcessorInfo()
    {
    }

    uint64_t TotalUsage() const
    {
        return 0;
    }

    uint64_t CoreUsage(uint32_t coreNum) const
    {
        return 0;
    }

    uint16_t NumberOfCores() const
    {
        return 0;
    }
};

IMemoryInfo* AWCContainerBase::Memory() const
{
    TRACE_L3("%s", _TRACE_FUNCTION_);
    return new AWCMemoryInfo();
}

IProcessorInfo* AWCContainerBase::ProcessorInfo() const
{
    TRACE_L3("%s", _TRACE_FUNCTION_);
    return new AWCProcessorInfo();
}

INetworkInterfaceIterator* AWCContainerBase::NetworkInterfaces() const
{
    TRACE_L3("%s", _TRACE_FUNCTION_);
    return nullptr;
}
