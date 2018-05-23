#ifndef __OBJECTMESSAGEHANDLER_H
#define __OBJECTMESSAGEHANDLER_H

namespace WPEFramework {
namespace RPC {
    class ObjectMessageHandler : public Core::IPCServerType<RPC::ObjectMessage> {
    private:
        ObjectMessageHandler(const ObjectMessageHandler&);
        ObjectMessageHandler& operator=(const ObjectMessageHandler&);

    public:
        ObjectMessageHandler()
        {
        }
        ~ObjectMessageHandler()
        {
        }

    public:
        virtual void Procedure(Core::IPCChannel& channel, Core::ProxyType<RPC::ObjectMessage>& data)
        {
            // Oke, see if we can reference count the IPCChannel
            Core::ProxyType<Core::IPCChannel> refChannel(dynamic_cast<Core::IReferenceCounted*>(&channel), &channel);

            ASSERT(refChannel.IsValid());

            if (refChannel.IsValid() == true) {
                const string className(data->Parameters().ClassName());
                const uint32_t interfaceId(data->Parameters().InterfaceId());
                const uint32_t versionId(data->Parameters().VersionId());
                Core::Library emptyLibrary;


                // Allright, respond with the interface.
				data->Response().Value(Core::ServiceAdministrator::Instance().Instantiate(emptyLibrary, className.c_str(), versionId, interfaceId));
			}

            Core::ProxyType<Core::IIPC> baseData(Core::proxy_cast<Core::IIPC>(data));

            channel.ReportResponse(baseData);
        }
    };
}
}

#endif /* __OBJECTMESSAGEHANDLER_H */


