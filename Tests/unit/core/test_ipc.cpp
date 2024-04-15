/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>

namespace Thunder {
namespace Tests {

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

    typedef Core::IPCMessageType<1, Triplet, Response> TripletResponse;
    typedef Core::IPCMessageType<2, Core::Void, Triplet> VoidTriplet;
    typedef Core::IPCMessageType<3, Core::IPC::Text<2048>, Core::IPC::Text<2048>> TextText;

    class HandleTripletResponse : public Core::IIPCServer {
    public:
        HandleTripletResponse(const HandleTripletResponse&) = delete;
        HandleTripletResponse& operator=(const HandleTripletResponse&) = delete;

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
            Core::ProxyType<TripletResponse> message(data);
            uint32_t result = message->Parameters().Display() + message->Parameters().Surface() + static_cast<uint32_t>(message->Parameters().Context());

            message->Response() = Response(result);
            source.ReportResponse(data);
        }
    };

    class HandleVoidTriplet : public Core::IIPCServer {
    public:
        HandleVoidTriplet(const HandleVoidTriplet&) = delete;
        HandleVoidTriplet& operator=(const HandleVoidTriplet&) = delete;

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
            Core::ProxyType<VoidTriplet> message(data);
            Triplet newValue(1, 2, 3);

            message->Response() = newValue;
            source.ReportResponse(data);
        }
    };

    class HandleTextText : public Core::IIPCServer {
    public:
        HandleTextText(const HandleTextText&) = delete;
        HandleTextText& operator=(const HandleTextText&) = delete;

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
            Core::ProxyType<TextText> message(data);
            string text = message->Parameters().Value();

            message->Response() = Core::IPC::Text<2048>(text);
            source.ReportResponse(data);
        }
    };

    TEST(DISABLED_Core_IPC, ContinuousChannel)
    {
        std::string connector = _T("/tmp/testserver0");
        auto lambdaFunc = [connector](IPTestAdministrator & testAdmin) {
            Core::NodeId continousNode(connector.c_str());
            uint32_t error;

            Core::ProxyType<Core::FactoryType<Core::IIPC, uint32_t> > factory(Core::ProxyType<Core::FactoryType<Core::IIPC, uint32_t> >::Create());

            factory->CreateFactory<TripletResponse>(2);
            factory->CreateFactory<VoidTriplet>(2);
            factory->CreateFactory<TextText>(2);

            Core::IPCChannelClientType<Core::Void, true, false> continousChannel(continousNode, 32, factory);

            Core::ProxyType<Core::IIPCServer> handler1(Core::ProxyType<HandleTripletResponse>::Create());
            Core::ProxyType<Core::IIPCServer> handler2(Core::ProxyType<HandleVoidTriplet>::Create());
            Core::ProxyType<Core::IIPCServer> handler3(Core::ProxyType<HandleTextText>::Create());

            continousChannel.Register(TripletResponse::Id(), handler1);
            continousChannel.Register(VoidTriplet::Id(), handler2);
            continousChannel.Register(TextText::Id(), handler3);

            error = continousChannel.Source().Open(1000); // Wait for 1 Second.
            EXPECT_EQ(error, Core::ERROR_NONE);

            testAdmin.Sync("setup server");
            testAdmin.Sync("setup client");
            testAdmin.Sync("done testing");

            error = continousChannel.Source().Close(1000); // Wait for 1 second
            EXPECT_EQ(error, Core::ERROR_NONE);
            continousChannel.Unregister(TripletResponse::Id());
            continousChannel.Unregister(VoidTriplet::Id());
            continousChannel.Unregister(TextText::Id());

            factory->DestroyFactories();
        };

        static std::function<void (IPTestAdministrator&)> lambdaVar = lambdaFunc;

        IPTestAdministrator::OtherSideMain otherSide = [](IPTestAdministrator& testAdmin ) { lambdaVar(testAdmin); };

        IPTestAdministrator testAdmin(otherSide);
        {
            Core::NodeId continousNode(connector.c_str());
            uint32_t error;

            testAdmin.Sync("setup server");

            Core::ProxyType<Core::FactoryType<Core::IIPC, uint32_t> > factory(Core::ProxyType<Core::FactoryType<Core::IIPC, uint32_t> >::Create());

            factory->CreateFactory<TripletResponse>(2);
            factory->CreateFactory<VoidTriplet>(2);
            factory->CreateFactory<TextText>(2);

            Core::IPCChannelClientType<Core::Void, false, false> continousChannel(continousNode, 32, factory);

            Core::ProxyType<Core::IIPCServer> handler1(Core::ProxyType<HandleTripletResponse>::Create());
            Core::ProxyType<Core::IIPCServer> handler2(Core::ProxyType<HandleVoidTriplet>::Create());
            Core::ProxyType<Core::IIPCServer> handler3(Core::ProxyType<HandleTextText>::Create());

            error = continousChannel.Source().Open(1000); // Wait for 1 Second.
            EXPECT_EQ(error, Core::ERROR_NONE);

            testAdmin.Sync("setup client");

            Core::ProxyType<TripletResponse> tripletResponseData(Core::ProxyType<TripletResponse>::Create(Triplet(1, 2, 3)));
            Core::ProxyType<VoidTriplet> voidTripletData(Core::ProxyType<VoidTriplet>::Create());
            string text = "test text";
            Core::ProxyType<TextText> textTextData(Core::ProxyType<TextText>::Create(Core::IPC::Text<2048>(text)));

            uint16_t display = 1;;
            uint32_t surface = 2;
            uint64_t context = 3;
            uint32_t result = 6;

            error = continousChannel.Invoke(tripletResponseData, 2000);
            EXPECT_EQ(error, Core::ERROR_NONE);
            EXPECT_EQ(tripletResponseData->Response().Result(), result);

            error = continousChannel.Invoke(voidTripletData, 2000);
            EXPECT_EQ(error, Core::ERROR_NONE);
            EXPECT_EQ(voidTripletData->Response().Display(), display);
            EXPECT_EQ(voidTripletData->Response().Surface(), surface);
            EXPECT_EQ(voidTripletData->Response().Context(), context);

            error = continousChannel.Invoke(textTextData, 2000);
            EXPECT_EQ(error, Core::ERROR_NONE);
            EXPECT_STREQ(textTextData->Response().Value(), text.c_str());

            error = continousChannel.Source().Close(1000); // Wait for 1 second
            EXPECT_EQ(error, Core::ERROR_NONE);

            factory->DestroyFactories();

            Core::Singleton::Dispose();
        }
        testAdmin.Sync("done testing");
    }

    TEST(Core_IPC, ContinuousChannelReversed)
    {
        std::string connector = _T("/tmp/testserver1");
        auto lambdaFunc = [connector](IPTestAdministrator & testAdmin) {
            Core::NodeId continousNode(connector.c_str());
            uint32_t error;

            testAdmin.Sync("setup client");

            Core::ProxyType<Core::FactoryType<Core::IIPC, uint32_t> > factory(Core::ProxyType<Core::FactoryType<Core::IIPC, uint32_t> >::Create());

            factory->CreateFactory<TripletResponse>(2);
            factory->CreateFactory<VoidTriplet>(2);
            factory->CreateFactory<TextText>(2);

            Core::IPCChannelClientType<Core::Void, false, false> continousChannel(continousNode, 32, factory);

            Core::ProxyType<Core::IIPCServer> handler1(Core::ProxyType<HandleTripletResponse>::Create());
            Core::ProxyType<Core::IIPCServer> handler2(Core::ProxyType<HandleVoidTriplet>::Create());
            Core::ProxyType<Core::IIPCServer> handler3(Core::ProxyType<HandleTextText>::Create());

            error = continousChannel.Source().Open(1000); // Wait for 1 Second.
            EXPECT_EQ(error, Core::ERROR_NONE);


            testAdmin.Sync("setup server");

            Core::ProxyType<TripletResponse> tripletResponseData(Core::ProxyType<TripletResponse>::Create(Triplet(1, 2, 3)));
            Core::ProxyType<VoidTriplet> voidTripletData(Core::ProxyType<VoidTriplet>::Create());
            string text = "test text";
            Core::ProxyType<TextText> textTextData(Core::ProxyType<TextText>::Create(Core::IPC::Text<2048>(text)));

            uint16_t display = 1;;
            uint32_t surface = 2;
            uint64_t context = 3;
            uint32_t result = 6;

            error = continousChannel.Invoke(tripletResponseData, 5000);
            EXPECT_EQ(error, Core::ERROR_NONE);
            EXPECT_EQ(tripletResponseData->Response().Result(), result);

            error = continousChannel.Invoke(voidTripletData, 2000);
            EXPECT_EQ(error, Core::ERROR_NONE);
            EXPECT_EQ(voidTripletData->Response().Display(), display);
            EXPECT_EQ(voidTripletData->Response().Surface(), surface);
            EXPECT_EQ(voidTripletData->Response().Context(), context);

            error = continousChannel.Invoke(textTextData, 2000);
            EXPECT_EQ(error, Core::ERROR_NONE);
            EXPECT_STREQ(textTextData->Response().Value(), text.c_str());

            error = continousChannel.Source().Close(1000); // Wait for 1 second
            EXPECT_EQ(error, Core::ERROR_NONE);

            factory->DestroyFactories();

            Core::Singleton::Dispose();

            testAdmin.Sync("done testing");
        };

        static std::function<void (IPTestAdministrator&)> lambdaVar = lambdaFunc;

        IPTestAdministrator::OtherSideMain otherSide = [](IPTestAdministrator& testAdmin ) { lambdaVar(testAdmin); };

        IPTestAdministrator testAdmin(otherSide);
        {
            Core::NodeId continousNode(connector.c_str());
            uint32_t error;

            Core::ProxyType<Core::FactoryType<Core::IIPC, uint32_t> > factory(Core::ProxyType<Core::FactoryType<Core::IIPC, uint32_t> >::Create());

            factory->CreateFactory<TripletResponse>(2);
            factory->CreateFactory<VoidTriplet>(2);
            factory->CreateFactory<TextText>(2);

            Core::IPCChannelClientType<Core::Void, true, false> continousChannel(continousNode, 32, factory);

            Core::ProxyType<Core::IIPCServer> handler1(Core::ProxyType<HandleTripletResponse>::Create());
            Core::ProxyType<Core::IIPCServer> handler2(Core::ProxyType<HandleVoidTriplet>::Create());
            Core::ProxyType<Core::IIPCServer> handler3(Core::ProxyType<HandleTextText>::Create());

            continousChannel.Register(TripletResponse::Id(), handler1);
            continousChannel.Register(VoidTriplet::Id(), handler2);
            continousChannel.Register(TextText::Id(), handler3);

            error = continousChannel.Source().Open(1000); // Wait for 1 Second.
            EXPECT_EQ(error, Core::ERROR_NONE);

            testAdmin.Sync("setup client");
            testAdmin.Sync("setup server");
            testAdmin.Sync("done testing");

            error = continousChannel.Source().Close(1000); // Wait for 1 second
            EXPECT_EQ(error, Core::ERROR_NONE);

            continousChannel.Unregister(TripletResponse::Id());
            continousChannel.Unregister(VoidTriplet::Id());
            continousChannel.Unregister(TextText::Id());

            factory->DestroyFactories();

            Core::Singleton::Dispose();
        }
    }

    TEST(DISABLED_Core_IPC, FlashChannel)
    {
        std::string connector = _T("/tmp/testserver2");
        auto lambdaFunc = [connector](IPTestAdministrator & testAdmin) {
            Core::NodeId flashNode(connector.c_str());
            uint32_t error;

            Core::ProxyType<Core::FactoryType<Core::IIPC, uint32_t> > factory(Core::ProxyType<Core::FactoryType<Core::IIPC, uint32_t> >::Create());

            factory->CreateFactory<TripletResponse>(2);
            factory->CreateFactory<VoidTriplet>(2);
            factory->CreateFactory<TextText>(2);

            Core::IPCChannelClientType<Core::Void, true, false> flashChannel(flashNode, 512, factory);

            Core::ProxyType<Core::IIPCServer> handler1(Core::ProxyType<HandleTripletResponse>::Create());
            Core::ProxyType<Core::IIPCServer> handler2(Core::ProxyType<HandleVoidTriplet>::Create());
            Core::ProxyType<Core::IIPCServer> handler3(Core::ProxyType<HandleTextText>::Create());

            flashChannel.Register(TripletResponse::Id(), handler1);
            flashChannel.Register(VoidTriplet::Id(), handler2);
            flashChannel.Register(TextText::Id(), handler3);

            error = flashChannel.Source().Open(1000); // Wait for 1 Second.
            EXPECT_EQ(error, Core::ERROR_NONE);

            testAdmin.Sync("setup server");
            testAdmin.Sync("setup client");
            testAdmin.Sync("done testing");

            error = flashChannel.Source().Close(1000); // Wait for 1 Second
            EXPECT_EQ(error, Core::ERROR_NONE);
            flashChannel.Unregister(TripletResponse::Id());
            flashChannel.Unregister(VoidTriplet::Id());
            flashChannel.Unregister(TextText::Id());

            factory->DestroyFactories();
        };

        static std::function<void (IPTestAdministrator&)> lambdaVar = lambdaFunc;

        IPTestAdministrator::OtherSideMain otherSide = [](IPTestAdministrator& testAdmin ) { lambdaVar(testAdmin); };

        IPTestAdministrator testAdmin(otherSide);
        {
            Core::NodeId flashNode(connector.c_str());
            uint32_t error;

            testAdmin.Sync("setup server");

            Core::ProxyType<Core::FactoryType<Core::IIPC, uint32_t> > factory(Core::ProxyType<Core::FactoryType<Core::IIPC, uint32_t> >::Create());

            factory->CreateFactory<TripletResponse>(2);
            factory->CreateFactory<VoidTriplet>(2);
            factory->CreateFactory<TextText>(2);

            Core::IPCChannelClientType<Core::Void, false, false> flashChannel(flashNode, 512, factory);

            Core::ProxyType<Core::IIPCServer> handler1(Core::ProxyType<HandleTripletResponse>::Create());
            Core::ProxyType<Core::IIPCServer> handler2(Core::ProxyType<HandleVoidTriplet>::Create());
            Core::ProxyType<Core::IIPCServer> handler3(Core::ProxyType<HandleTextText>::Create());

            testAdmin.Sync("setup client");

            Core::ProxyType<TripletResponse> tripletResponseData(Core::ProxyType<TripletResponse>::Create(Triplet(1, 2, 3)));
            Core::ProxyType<VoidTriplet> voidTripletData(Core::ProxyType<VoidTriplet>::Create());
            string text = "test text";
            Core::ProxyType<TextText> textTextData(Core::ProxyType<TextText>::Create(Core::IPC::Text<2048>(text)));

            uint16_t display = 1;;
            uint32_t surface = 2;
            uint64_t context = 3;
            uint32_t result = 6;

            error = flashChannel.Source().Open(1000); // Wait for 1 Second.
            EXPECT_EQ(error, Core::ERROR_NONE);
            error = flashChannel.Invoke(tripletResponseData, 2000);
            EXPECT_EQ(error, Core::ERROR_NONE);
            EXPECT_EQ(tripletResponseData->Response().Result(), result);
            error = flashChannel.Source().Close(1000); // Wait for 1 Second
            EXPECT_EQ(error, Core::ERROR_NONE);

            error = flashChannel.Source().Open(1000); // Wait for 1 Second.
            EXPECT_EQ(error, Core::ERROR_NONE);
            error = flashChannel.Invoke(voidTripletData, 2000);
            EXPECT_EQ(error, Core::ERROR_NONE);
            EXPECT_EQ(voidTripletData->Response().Display(), display);
            EXPECT_EQ(voidTripletData->Response().Surface(), surface);
            EXPECT_EQ(voidTripletData->Response().Context(), context);
            error = flashChannel.Source().Close(1000); // Wait for 1 Second
            EXPECT_EQ(error, Core::ERROR_NONE);

            error = flashChannel.Source().Open(1000); // Wait for 1 Second.
            EXPECT_EQ(error, Core::ERROR_NONE);
            error = flashChannel.Invoke(textTextData, 2000);
            EXPECT_EQ(error, Core::ERROR_NONE);
            EXPECT_STREQ(textTextData->Response().Value(), text.c_str());
            error = flashChannel.Source().Close(1000); // Wait for 1 Second
            EXPECT_EQ(error, Core::ERROR_NONE);

            factory->DestroyFactories();
            Core::Singleton::Dispose();
        }

        testAdmin.Sync("done testing");
    }

    TEST(DISABLED_Core_IPC, FlashChannelReversed)
    {
        std::string connector = _T("/tmp/testserver3");
        auto lambdaFunc = [connector](IPTestAdministrator & testAdmin) {
            Core::NodeId flashNode(connector.c_str());
            uint32_t error;

            Core::ProxyType<Core::FactoryType<Core::IIPC, uint32_t> > factory(Core::ProxyType<Core::FactoryType<Core::IIPC, uint32_t> >::Create());

            factory->CreateFactory<TripletResponse>(2);
            factory->CreateFactory<VoidTriplet>(2);
            factory->CreateFactory<TextText>(2);

            Core::IPCChannelClientType<Core::Void, true, false> flashChannel(flashNode, 512, factory);

            Core::ProxyType<Core::IIPCServer> handler1(Core::ProxyType<HandleTripletResponse>::Create());
            Core::ProxyType<Core::IIPCServer> handler2(Core::ProxyType<HandleVoidTriplet>::Create());
            Core::ProxyType<Core::IIPCServer> handler3(Core::ProxyType<HandleTextText>::Create());

            flashChannel.Register(TripletResponse::Id(), handler1);
            flashChannel.Register(VoidTriplet::Id(), handler2);
            flashChannel.Register(TextText::Id(), handler3);

            error = flashChannel.Source().Open(1000); // Wait for 1 Second.
            EXPECT_EQ(error, Core::ERROR_NONE);

            testAdmin.Sync("setup server");
            testAdmin.Sync("setup client");
            testAdmin.Sync("done testing");

            error = flashChannel.Source().Close(1000); // Wait for 1 Second
            EXPECT_EQ(error, Core::ERROR_NONE);
            flashChannel.Unregister(TripletResponse::Id());
            flashChannel.Unregister(VoidTriplet::Id());
            flashChannel.Unregister(TextText::Id());

            factory->DestroyFactories();
        };

        static std::function<void (IPTestAdministrator&)> lambdaVar = lambdaFunc;

        IPTestAdministrator::OtherSideMain otherSide = [](IPTestAdministrator& testAdmin ) { lambdaVar(testAdmin); };

        IPTestAdministrator testAdmin(otherSide);
        {
            Core::NodeId flashNode(connector.c_str());
            uint32_t error;

            testAdmin.Sync("setup server");

            Core::ProxyType<Core::FactoryType<Core::IIPC, uint32_t> > factory(Core::ProxyType<Core::FactoryType<Core::IIPC, uint32_t> >::Create());

            factory->CreateFactory<TripletResponse>(2);
            factory->CreateFactory<VoidTriplet>(2);
            factory->CreateFactory<TextText>(2);

            Core::IPCChannelClientType<Core::Void, false, false> flashChannel(flashNode, 512, factory);

            Core::ProxyType<Core::IIPCServer> handler1(Core::ProxyType<HandleTripletResponse>::Create());
            Core::ProxyType<Core::IIPCServer> handler2(Core::ProxyType<HandleVoidTriplet>::Create());
            Core::ProxyType<Core::IIPCServer> handler3(Core::ProxyType<HandleTextText>::Create());

            testAdmin.Sync("setup client");

            Core::ProxyType<TripletResponse> tripletResponseData(Core::ProxyType<TripletResponse>::Create(Triplet(1, 2, 3)));
            Core::ProxyType<VoidTriplet> voidTripletData(Core::ProxyType<VoidTriplet>::Create());
            string text = "test text";
            Core::ProxyType<TextText> textTextData(Core::ProxyType<TextText>::Create(Core::IPC::Text<2048>(text)));

            uint16_t display = 1;;
            uint32_t surface = 2;
            uint64_t context = 3;
            uint32_t result = 6;

            error = flashChannel.Source().Open(1000); // Wait for 1 Second.
            EXPECT_EQ(error, Core::ERROR_NONE);
            error = flashChannel.Invoke(tripletResponseData, 2000);
            EXPECT_EQ(error, Core::ERROR_NONE);
            EXPECT_EQ(tripletResponseData->Response().Result(), result);
            error = flashChannel.Source().Close(1000); // Wait for 1 Second
            EXPECT_EQ(error, Core::ERROR_NONE);

            error = flashChannel.Source().Open(1000); // Wait for 1 Second.
            EXPECT_EQ(error, Core::ERROR_NONE);
            error = flashChannel.Invoke(voidTripletData, 2000);
            EXPECT_EQ(error, Core::ERROR_NONE);
            EXPECT_EQ(voidTripletData->Response().Display(), display);
            EXPECT_EQ(voidTripletData->Response().Surface(), surface);
            EXPECT_EQ(voidTripletData->Response().Context(), context);
            error = flashChannel.Source().Close(1000); // Wait for 1 Second
            EXPECT_EQ(error, Core::ERROR_NONE);

            error = flashChannel.Source().Open(1000); // Wait for 1 Second.
            EXPECT_EQ(error, Core::ERROR_NONE);
            error = flashChannel.Invoke(textTextData, 2000);
            EXPECT_EQ(error, Core::ERROR_NONE);
            EXPECT_STREQ(textTextData->Response().Value(), text.c_str());
            error = flashChannel.Source().Close(1000); // Wait for 1 Second
            EXPECT_EQ(error, Core::ERROR_NONE);

            factory->DestroyFactories();
            Core::Singleton::Dispose();
        }
        testAdmin.Sync("done testing");
    }

    TEST(Core_IPC, MultiChannel)
    {
        std::string connector = _T("/tmp/testserver4");
        auto lambdaFunc = [connector](IPTestAdministrator & testAdmin) {
            Core::NodeId multiNode(connector.c_str());
            uint32_t error;

            Core::ProxyType<Core::FactoryType<Core::IIPC, uint32_t> > factory(Core::ProxyType<Core::FactoryType<Core::IIPC, uint32_t> >::Create());

            factory->CreateFactory<TripletResponse>(2);
            factory->CreateFactory<VoidTriplet>(2);
            factory->CreateFactory<TextText>(2);

            Core::IPCChannelServerType<Core::Void, false> multiChannel(multiNode, 512, factory);

            Core::ProxyType<Core::IIPCServer> handler1(Core::ProxyType<HandleTripletResponse>::Create());
            Core::ProxyType<Core::IIPCServer> handler2(Core::ProxyType<HandleVoidTriplet>::Create());
            Core::ProxyType<Core::IIPCServer> handler3(Core::ProxyType<HandleTextText>::Create());

            multiChannel.Register(TripletResponse::Id(), handler1);
            multiChannel.Register(VoidTriplet::Id(), handler2);
            multiChannel.Register(TextText::Id(), handler3);

            error = multiChannel.Open(1000); // Wait for 1 Second.
            EXPECT_EQ(error, Core::ERROR_NONE);

            testAdmin.Sync("setup server");
            testAdmin.Sync("setup client");
            testAdmin.Sync("done testing");

            error = multiChannel.Close(1000); // Wait for 1 Second.
            EXPECT_EQ(error, Core::ERROR_NONE);

            factory->DestroyFactories();

            Core::Singleton::Dispose();
        };

        static std::function<void (IPTestAdministrator&)> lambdaVar = lambdaFunc;

        IPTestAdministrator::OtherSideMain otherSide = [](IPTestAdministrator& testAdmin ) { lambdaVar(testAdmin); };

        IPTestAdministrator testAdmin(otherSide);
        {
            Core::NodeId multiNode(connector.c_str());
            uint32_t error;

            testAdmin.Sync("setup server");

            Core::ProxyType<Core::FactoryType<Core::IIPC, uint32_t> > factory(Core::ProxyType<Core::FactoryType<Core::IIPC, uint32_t> >::Create());

            factory->CreateFactory<TripletResponse>(2);
            factory->CreateFactory<VoidTriplet>(2);
            factory->CreateFactory<TextText>(2);

            Core::IPCChannelClientType<Core::Void, false, false> multiChannel(multiNode, 512, factory);

            Core::ProxyType<Core::IIPCServer> handler1(Core::ProxyType<HandleTripletResponse>::Create());
            Core::ProxyType<Core::IIPCServer> handler2(Core::ProxyType<HandleVoidTriplet>::Create());
            Core::ProxyType<Core::IIPCServer> handler3(Core::ProxyType<HandleTextText>::Create());

            error = multiChannel.Source().Open(1000); // Wait for 1 Second.
            EXPECT_EQ(error, Core::ERROR_NONE);

            testAdmin.Sync("setup client");

            Core::ProxyType<TripletResponse> tripletResponseData(Core::ProxyType<TripletResponse>::Create(Triplet(1, 2, 3)));
            Core::ProxyType<VoidTriplet> voidTripletData(Core::ProxyType<VoidTriplet>::Create());
            string text = "test text";
            Core::ProxyType<TextText> textTextData(Core::ProxyType<TextText>::Create(Core::IPC::Text<2048>(text)));

            uint16_t display = 1;;
            uint32_t surface = 2;
            uint64_t context = 3;
            uint32_t result = 6;

            error = multiChannel.Invoke(tripletResponseData, 2000);
            EXPECT_EQ(error, Core::ERROR_NONE);
            EXPECT_EQ(tripletResponseData->Response().Result(), result);

            error = multiChannel.Invoke(voidTripletData, 2000);
            EXPECT_EQ(error, Core::ERROR_NONE);
            EXPECT_EQ(voidTripletData->Response().Display(), display);
            EXPECT_EQ(voidTripletData->Response().Surface(), surface);
            EXPECT_EQ(voidTripletData->Response().Context(), context);

            error = multiChannel.Invoke(textTextData, 2000);
            EXPECT_EQ(error, Core::ERROR_NONE);
            EXPECT_STREQ(textTextData->Response().Value(), text.c_str());

            error = multiChannel.Source().Close(1000); // Wait for 1 Second.
            EXPECT_EQ(error, Core::ERROR_NONE);

            factory->DestroyFactories();

            Core::Singleton::Dispose();
        }
//        testAdmin.Sync("done testing");
    }

    TEST(Core_IPC, MultiChannelReversed)
    {
        std::string connector = _T("/tmp/testserver5");
        auto lambdaFunc = [connector](IPTestAdministrator & testAdmin) {
            Core::NodeId multiNode(connector.c_str());
            uint32_t error;

            testAdmin.Sync("setup client");

            Core::ProxyType<Core::FactoryType<Core::IIPC, uint32_t> > factory(Core::ProxyType<Core::FactoryType<Core::IIPC, uint32_t> >::Create());
            factory->CreateFactory<TripletResponse>(2);
            factory->CreateFactory<VoidTriplet>(2);
            factory->CreateFactory<TextText>(2);

            Core::IPCChannelClientType<Core::Void, false, false> multiChannel(multiNode, 512, factory);

            Core::ProxyType<Core::IIPCServer> handler1(Core::ProxyType<HandleTripletResponse>::Create());
            Core::ProxyType<Core::IIPCServer> handler2(Core::ProxyType<HandleVoidTriplet>::Create());
            Core::ProxyType<Core::IIPCServer> handler3(Core::ProxyType<HandleTextText>::Create());

            error = multiChannel.Source().Open(1000); // Wait for 1 Second.
            EXPECT_EQ(error, Core::ERROR_NONE);

            testAdmin.Sync("setup server");

            Core::ProxyType<TripletResponse> tripletResponseData(Core::ProxyType<TripletResponse>::Create(Triplet(1, 2, 3)));
            Core::ProxyType<VoidTriplet> voidTripletData(Core::ProxyType<VoidTriplet>::Create());
            string text = "test text";
            Core::ProxyType<TextText> textTextData(Core::ProxyType<TextText>::Create(Core::IPC::Text<2048>(text)));

            uint16_t display = 1;;
            uint32_t surface = 2;
            uint64_t context = 3;
            uint32_t result = 6;
            error = multiChannel.Invoke(tripletResponseData, 5000);
            EXPECT_EQ(error, Core::ERROR_NONE);
            EXPECT_EQ(tripletResponseData->Response().Result(), result);

            error = multiChannel.Invoke(voidTripletData, 2000);
            EXPECT_EQ(error, Core::ERROR_NONE);
            EXPECT_EQ(voidTripletData->Response().Display(), display);
            EXPECT_EQ(voidTripletData->Response().Surface(), surface);
            EXPECT_EQ(voidTripletData->Response().Context(), context);

            error = multiChannel.Invoke(textTextData, 2000);
            EXPECT_EQ(error, Core::ERROR_NONE);
            EXPECT_STREQ(textTextData->Response().Value(), text.c_str());

            tripletResponseData.Release();
            voidTripletData.Release();
            textTextData.Release();

            error = multiChannel.Source().Close(1000); // Wait for 1 Second.
            EXPECT_EQ(error, Core::ERROR_NONE);

            handler1.Release();
            handler2.Release();
            handler3.Release();

            factory->DestroyFactory<TripletResponse>();
            factory->DestroyFactory<VoidTriplet>();
            factory->DestroyFactory<TextText>();

            factory->DestroyFactories();

            Core::Singleton::Dispose();

            testAdmin.Sync("done testing");
        };

        static std::function<void (IPTestAdministrator&)> lambdaVar = lambdaFunc;

        IPTestAdministrator::OtherSideMain otherSide = [](IPTestAdministrator& testAdmin ) { lambdaVar(testAdmin); };

        IPTestAdministrator testAdmin(otherSide);
        {
            Core::NodeId multiNode(connector.c_str());
            uint32_t error;

            Core::ProxyType<Core::FactoryType<Core::IIPC, uint32_t> > factory(Core::ProxyType<Core::FactoryType<Core::IIPC, uint32_t> >::Create());
            factory->CreateFactory<TripletResponse>(2);
            factory->CreateFactory<VoidTriplet>(2);
            factory->CreateFactory<TextText>(2);

            Core::IPCChannelServerType<Core::Void, false> multiChannel(multiNode, 512, factory);

            Core::ProxyType<Core::IIPCServer> handler1(Core::ProxyType<HandleTripletResponse>::Create());
            Core::ProxyType<Core::IIPCServer> handler2(Core::ProxyType<HandleVoidTriplet>::Create());
            Core::ProxyType<Core::IIPCServer> handler3(Core::ProxyType<HandleTextText>::Create());

            multiChannel.Register(TripletResponse::Id(), handler1);
            multiChannel.Register(VoidTriplet::Id(), handler2);
            multiChannel.Register(TextText::Id(), handler3);

            error = multiChannel.Open(1000); // Wait for 1 Second.
            EXPECT_EQ(error, Core::ERROR_NONE);

            testAdmin.Sync("setup client");
            testAdmin.Sync("setup server");
            testAdmin.Sync("done testing");

            error = multiChannel.Close(1000); // Wait for 1 Second.
            EXPECT_EQ(error, Core::ERROR_NONE);

            handler1.Release();
            handler2.Release();
            handler3.Release();

            multiChannel.Cleanup();

            factory->DestroyFactory<TripletResponse>();
            factory->DestroyFactory<VoidTriplet>();
            factory->DestroyFactory<TextText>();

            factory->DestroyFactories();

            Core::Singleton::Dispose();

//            testAdmin.Sync("done testing");
        }
    }
} // Tests
} // Thunder
