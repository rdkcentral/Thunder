#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>

using namespace WPEFramework;

string g_continuousConnector = _T("/tmp/testserver0");
string g_flashConnector = _T("/tmp/testserver1");
string g_multiConnector = _T("/tmp/testserver2");

namespace Messages {

class Response {
public:
    Response()
        : _result(0)
    {
    }
    Response(const uint32_t result)
        : _result(result)
    {
    }
    Response(const Response& copy)
        : _result(copy._result)
    {
    }
    ~Response()
    {
    }

    Response& operator=(const Response& RHS)
    {
        _result = RHS._result;

        return (*this);
    }

public:
    inline uint32_t Result() const
    {
        return (_result);
    }

private:
    uint32_t _result;
};

class Triplet {
public:
    Triplet()
        : _display(0)
        , _surface(1)
        , _context(2)
    {
    }
    Triplet(const uint16_t display, const uint32_t surface, const uint64_t context)
        : _display(display)
        , _surface(surface)
        , _context(context)
    {
    }
    Triplet(const Triplet& copy)
        : _display(copy._display)
        , _surface(copy._surface)
        , _context(copy._context)
    {
    }
    ~Triplet()
    {
    }

    Triplet& operator=(const Triplet& RHS)
    {
        _display = RHS._display;
        _surface = RHS._surface;
        _context = RHS._context;

        return (*this);
    }

public:
    inline uint16_t Display() const
    {
        return (_display);
    }
    inline uint32_t Surface() const
    {
        return (_surface);
    }
    inline uint64_t Context() const
    {
        return (_context);
    }

private:
    uint16_t _display;
    uint32_t _surface;
    uint64_t _context;
};

typedef Core::IPCMessageType<1, Messages::Triplet, Messages::Response> TripletResponse;
typedef Core::IPCMessageType<2, Core::Void, Messages::Triplet> VoidTriplet;
typedef Core::IPCMessageType<3, Core::IPC::Text<2048>, Core::IPC::Text<2048>> TextText;
}

class HandleTripletResponse : public Core::IIPCServer {
private:
    HandleTripletResponse(const HandleTripletResponse&);
    HandleTripletResponse& operator=(const HandleTripletResponse&);

public:
    HandleTripletResponse()
    {
    }
    virtual ~HandleTripletResponse()
    {
    }

public:
    // Here comes the actual implementation of the RPC...
    virtual void Procedure(Core::IPCChannel& source, Core::ProxyType<Core::IIPC>& data)
    {
        Core::ProxyType<Messages::TripletResponse> message(data);
        uint32_t result = message->Parameters().Display() + message->Parameters().Surface() + static_cast<uint32_t>(message->Parameters().Context());

        message->Response() = Messages::Response(result);
        source.ReportResponse(data);
    }
};

class HandleVoidTriplet : public Core::IIPCServer {
private:
    HandleVoidTriplet(const HandleVoidTriplet&);
    HandleVoidTriplet& operator=(const HandleVoidTriplet&);

public:
    HandleVoidTriplet()
    {
    }
    virtual ~HandleVoidTriplet()
    {
    }

public:
    // Here comes the actual implementation of the RPC...
    virtual void Procedure(Core::IPCChannel& source, Core::ProxyType<Core::IIPC>& data)
    {
        Core::ProxyType<Messages::VoidTriplet> message(data);
        Messages::Triplet newValue(1, 2, 3);

        message->Response() = newValue;
        source.ReportResponse(data);
    }
};

class HandleTextText : public Core::IIPCServer {
private:
    HandleTextText(const HandleTextText&);
    HandleTextText& operator=(const HandleTextText&);

public:
    HandleTextText()
    {
    }
    virtual ~HandleTextText()
    {
    }

public:
    // Here comes the actual implementation of the RPC...
    virtual void Procedure(Core::IPCChannel& source, Core::ProxyType<Core::IIPC>& data)
    {
        Core::ProxyType<Messages::TextText> message(data);
        string text = message->Parameters().Value();

        message->Response() = Core::IPC::Text<2048>(text);
        source.ReportResponse(data);
    }
};

TEST(Core_IPC, simpleSet)
{
    IPTestAdministrator::OtherSideMain otherSide = [](IPTestAdministrator & testAdmin) {
        Core::NodeId continousNode(g_continuousConnector.c_str());
        Core::NodeId flashNode(g_flashConnector.c_str());
        Core::NodeId multiNode(g_multiConnector.c_str());
        uint32_t error;

        Core::ProxyType<Core::FactoryType<Core::IIPC, uint32_t> > factory(Core::ProxyType<Core::FactoryType<Core::IIPC, uint32_t> >::Create());

        factory->CreateFactory<Messages::TripletResponse>(2);
        factory->CreateFactory<Messages::VoidTriplet>(2);
        factory->CreateFactory<Messages::TextText>(2);

        Core::IPCChannelClientType<Core::Void, true, false> continousChannel(continousNode, 32, factory);
        Core::IPCChannelClientType<Core::Void, true, false> flashChannel(flashNode, 512, factory);
        Core::IPCChannelServerType<Core::Void, false> multiChannel(multiNode, 512, factory);

        Core::ProxyType<Core::IIPCServer> handler1(Core::ProxyType<HandleTripletResponse>::Create());
        Core::ProxyType<Core::IIPCServer> handler2(Core::ProxyType<HandleVoidTriplet>::Create());
        Core::ProxyType<Core::IIPCServer> handler3(Core::ProxyType<HandleTextText>::Create());

        continousChannel.Register(Messages::TripletResponse::Id(), handler1);
        continousChannel.Register(Messages::VoidTriplet::Id(), handler2);
        continousChannel.Register(Messages::TextText::Id(), handler3);

        flashChannel.Register(Messages::TripletResponse::Id(), handler1);
        flashChannel.Register(Messages::VoidTriplet::Id(), handler2);
        flashChannel.Register(Messages::TextText::Id(), handler3);

        multiChannel.Register(Messages::TripletResponse::Id(), handler1);
        multiChannel.Register(Messages::VoidTriplet::Id(), handler2);
        multiChannel.Register(Messages::TextText::Id(), handler3);

        error = continousChannel.Source().Open(1000); // Wait for 1 Second.
        ASSERT_EQ(error, Core::ERROR_NONE);
        error = flashChannel.Source().Open(1000); // Wait for 1 Second.
        ASSERT_EQ(error, Core::ERROR_NONE);
        error = multiChannel.Open(1000); // Wait for 1 Second.
        ASSERT_EQ(error, Core::ERROR_NONE);

        testAdmin.Sync("setup server");
        testAdmin.Sync("setup client");
        testAdmin.Sync("done testing continousChannel");
        testAdmin.Sync("done testing flashChannel");
        testAdmin.Sync("done testing multiChannel");

        testAdmin.Sync("done testing");

        error = continousChannel.Source().Close(1000); // Wait for 1 second
        ASSERT_EQ(error, Core::ERROR_NONE);
        continousChannel.Unregister(Messages::TripletResponse::Id());
        continousChannel.Unregister(Messages::VoidTriplet::Id());
        continousChannel.Unregister(Messages::TextText::Id());

        error = flashChannel.Source().Close(1000); // Wait for 1 Second
        ASSERT_EQ(error, Core::ERROR_NONE);
        flashChannel.Unregister(Messages::TripletResponse::Id());
        flashChannel.Unregister(Messages::VoidTriplet::Id());
        flashChannel.Unregister(Messages::TextText::Id());

        error = multiChannel.Close(1000); // Wait for 1 Second.
        ASSERT_EQ(error, Core::ERROR_NONE);
        multiChannel.Unregister(Messages::TripletResponse::Id());
        multiChannel.Unregister(Messages::VoidTriplet::Id());
        multiChannel.Unregister(Messages::TextText::Id());

        factory->DestroyFactories();
    };

    IPTestAdministrator testAdmin(otherSide);
    {
        Core::NodeId continousNode(g_continuousConnector.c_str());
        Core::NodeId flashNode(g_flashConnector.c_str());
        Core::NodeId multiNode(g_multiConnector.c_str());
        uint32_t error;

        testAdmin.Sync("setup server");

        Core::ProxyType<Core::FactoryType<Core::IIPC, uint32_t> > factory(Core::ProxyType<Core::FactoryType<Core::IIPC, uint32_t> >::Create());

        factory->CreateFactory<Messages::TripletResponse>(2);
        factory->CreateFactory<Messages::VoidTriplet>(2);
        factory->CreateFactory<Messages::TextText>(2);

        Core::IPCChannelClientType<Core::Void, false, false> continousChannel(continousNode, 32, factory);
        Core::IPCChannelClientType<Core::Void, false, false> flashChannel(flashNode, 512, factory);
        Core::IPCChannelClientType<Core::Void, false, false> multiChannel(multiNode, 512, factory);

        Core::ProxyType<Core::IIPCServer> handler1(Core::ProxyType<HandleTripletResponse>::Create());
        Core::ProxyType<Core::IIPCServer> handler2(Core::ProxyType<HandleVoidTriplet>::Create());
        Core::ProxyType<Core::IIPCServer> handler3(Core::ProxyType<HandleTextText>::Create());

        error = continousChannel.Source().Open(1000); // Wait for 1 Second.
        ASSERT_EQ(error, Core::ERROR_NONE);
        error = multiChannel.Source().Open(1000); // Wait for 1 Second.
        ASSERT_EQ(error, Core::ERROR_NONE);

        testAdmin.Sync("setup client");

        Core::ProxyType<Messages::TripletResponse> tripletResponseData(Core::ProxyType<Messages::TripletResponse>::Create(Messages::Triplet(1, 2, 3)));
        Core::ProxyType<Messages::VoidTriplet> voidTripletData(Core::ProxyType<Messages::VoidTriplet>::Create());
        string text = "test text";
        Core::ProxyType<Messages::TextText> textTextData(Core::ProxyType<Messages::TextText>::Create(Core::IPC::Text<2048>(text)));

        uint16_t display = 1;;
        uint32_t surface = 2;
        uint64_t context = 3;
        uint32_t result = 6;

        error = continousChannel.Invoke(tripletResponseData, 5000);
        ASSERT_EQ(error, Core::ERROR_NONE);
        ASSERT_EQ(tripletResponseData->Response().Result(), result);

        error = continousChannel.Invoke(voidTripletData, 2000);
        ASSERT_EQ(error, Core::ERROR_NONE);
        ASSERT_EQ(voidTripletData->Response().Display(), display);
        ASSERT_EQ(voidTripletData->Response().Surface(), surface);
        ASSERT_EQ(voidTripletData->Response().Context(), context);

        error = continousChannel.Invoke(textTextData, 2000);
        ASSERT_EQ(error, Core::ERROR_NONE);
        ASSERT_STREQ(textTextData->Response().Value(), text.c_str());
        testAdmin.Sync("done testing continousChannel");

        error = flashChannel.Source().Open(1000); // Wait for 1 Second.
        ASSERT_EQ(error, Core::ERROR_NONE);
        error = flashChannel.Invoke(tripletResponseData, 2000);
        ASSERT_EQ(error, Core::ERROR_NONE);
        ASSERT_EQ(tripletResponseData->Response().Result(), result);
        error = flashChannel.Source().Close(1000); // Wait for 1 Second
        ASSERT_EQ(error, Core::ERROR_NONE);

        error = flashChannel.Source().Open(1000); // Wait for 1 Second.
        ASSERT_EQ(error, Core::ERROR_NONE);
        error = flashChannel.Invoke(voidTripletData, 2000);
        ASSERT_EQ(error, Core::ERROR_NONE);
        ASSERT_EQ(voidTripletData->Response().Display(), display);
        ASSERT_EQ(voidTripletData->Response().Surface(), surface);
        ASSERT_EQ(voidTripletData->Response().Context(), context);
        error = flashChannel.Source().Close(1000); // Wait for 1 Second
        ASSERT_EQ(error, Core::ERROR_NONE);

        error = flashChannel.Source().Open(1000); // Wait for 1 Second.
        ASSERT_EQ(error, Core::ERROR_NONE);
        error = flashChannel.Invoke(textTextData, 2000);
        ASSERT_EQ(error, Core::ERROR_NONE);
        ASSERT_STREQ(textTextData->Response().Value(), text.c_str());
        error = flashChannel.Source().Close(1000); // Wait for 1 Second
        ASSERT_EQ(error, Core::ERROR_NONE);
        testAdmin.Sync("done testing flashChannel");

        error = multiChannel.Invoke(tripletResponseData, 2000);
        ASSERT_EQ(error, Core::ERROR_NONE);
        ASSERT_EQ(tripletResponseData->Response().Result(), result);

        error = multiChannel.Invoke(voidTripletData, 2000);
        ASSERT_EQ(error, Core::ERROR_NONE);
        ASSERT_EQ(voidTripletData->Response().Display(), display);
        ASSERT_EQ(voidTripletData->Response().Surface(), surface);
        ASSERT_EQ(voidTripletData->Response().Context(), context);

        error = multiChannel.Invoke(textTextData, 2000);
        ASSERT_EQ(error, Core::ERROR_NONE);
        ASSERT_STREQ(textTextData->Response().Value(), text.c_str());

        testAdmin.Sync("done testing multiChannel");
        error = continousChannel.Source().Close(1000); // Wait for 1 second
        ASSERT_EQ(error, Core::ERROR_NONE);

        error = multiChannel.Source().Close(1000); // Wait for 1 Second.
        ASSERT_EQ(error, Core::ERROR_NONE);

        factory->DestroyFactories();
        Core::Singleton::Dispose();
    }
    testAdmin.Sync("done testing");
}
