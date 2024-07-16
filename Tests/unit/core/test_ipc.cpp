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

#include <gtest/gtest.h>

#ifndef MODULE_NAME
#include "../Module.h"
#endif

#include <core/core.h>

#include "../IPTestAdministrator.h"

namespace Thunder {
namespace Tests {
namespace Core {

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

    typedef ::Thunder::Core::IPCMessageType<1, Triplet, Response> TripletResponse;
    typedef ::Thunder::Core::IPCMessageType<2, ::Thunder::Core::Void, Triplet> VoidTriplet;
    typedef ::Thunder::Core::IPCMessageType<3, ::Thunder::Core::IPC::Text<2048>, ::Thunder::Core::IPC::Text<2048>> TextText;

    class HandleTripletResponse : public ::Thunder::Core::IIPCServer {
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
        virtual void Procedure(::Thunder::Core::IPCChannel& source, ::Thunder::Core::ProxyType<::Thunder::Core::IIPC>& data)
        {
            ::Thunder::Core::ProxyType<TripletResponse> message(data);
            uint32_t result = message->Parameters().Display() + message->Parameters().Surface() + static_cast<uint32_t>(message->Parameters().Context());

            message->Response() = Response(result);
            source.ReportResponse(data);
        }
    };

    class HandleVoidTriplet : public ::Thunder::Core::IIPCServer {
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
        virtual void Procedure(::Thunder::Core::IPCChannel& source, ::Thunder::Core::ProxyType<::Thunder::Core::IIPC>& data)
        {
            ::Thunder::Core::ProxyType<VoidTriplet> message(data);
            Triplet newValue(1, 2, 3);

            message->Response() = newValue;
            source.ReportResponse(data);
        }
    };

    class HandleTextText : public ::Thunder::Core::IIPCServer {
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
        virtual void Procedure(::Thunder::Core::IPCChannel& source, ::Thunder::Core::ProxyType<::Thunder::Core::IIPC>& data)
        {
            ::Thunder::Core::ProxyType<TextText> message(data);
            string text = message->Parameters().Value();

            message->Response() = ::Thunder::Core::IPC::Text<2048>(text);
            source.ReportResponse(data);
        }
    };

    TEST(Core_IPC, ContinuousChannel)
    {
        constexpr uint32_t initHandshakeValue = 0, maxWaitTime = 4, maxInitTime = 2000;

        const std::string connector = _T("/tmp/testserver0");

        IPTestAdministrator::Callback callback_child = [&](IPTestAdministrator& testAdmin) {
            ::Thunder::Core::NodeId continousNode(connector.c_str());

            ::Thunder::Core::ProxyType<::Thunder::Core::FactoryType<::Thunder::Core::IIPC, uint32_t> > factory(::Thunder::Core::ProxyType<::Thunder::Core::FactoryType<::Thunder::Core::IIPC, uint32_t> >::Create());

            factory->CreateFactory<TripletResponse>(2);
            factory->CreateFactory<VoidTriplet>(2);
            factory->CreateFactory<TextText>(2);

            // Listening with no internal factory
            ::Thunder::Core::IPCChannelClientType<::Thunder::Core::Void, true, false> continousChannel(continousNode, 32, factory);

            ::Thunder::Core::ProxyType<::Thunder::Core::IIPCServer> handler1(::Thunder::Core::ProxyType<HandleTripletResponse>::Create());
            ::Thunder::Core::ProxyType<::Thunder::Core::IIPCServer> handler2(::Thunder::Core::ProxyType<HandleVoidTriplet>::Create());
            ::Thunder::Core::ProxyType<::Thunder::Core::IIPCServer> handler3(::Thunder::Core::ProxyType<HandleTextText>::Create());

            continousChannel.Register(TripletResponse::Id(), handler1);
            continousChannel.Register(VoidTriplet::Id(), handler2);
            continousChannel.Register(TextText::Id(), handler3);

            ASSERT_EQ(continousChannel.Source().Open(maxWaitTime), ::Thunder::Core::ERROR_NONE);

            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);
            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);
            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            continousChannel.Unregister(TripletResponse::Id());
            continousChannel.Unregister(VoidTriplet::Id());
            continousChannel.Unregister(TextText::Id());

            handler1.Release();
            handler2.Release();
            handler3.Release();

            factory->DestroyFactory<TripletResponse>();
            factory->DestroyFactory<VoidTriplet>();
            factory->DestroyFactory<TextText>();

            // Only for internal factories
//            factory->DestroyFactories();
 
            EXPECT_EQ(continousChannel.Source().Close(maxWaitTime), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator::Callback callback_parent = [&](IPTestAdministrator& testAdmin) {
            // A small delay so the child can be set up
            SleepMs(maxInitTime);

            ::Thunder::Core::NodeId continousNode(connector.c_str());

            ::Thunder::Core::ProxyType<::Thunder::Core::FactoryType<::Thunder::Core::IIPC, uint32_t> > factory(::Thunder::Core::ProxyType<::Thunder::Core::FactoryType<::Thunder::Core::IIPC, uint32_t> >::Create());

            factory->CreateFactory<TripletResponse>(2);
            factory->CreateFactory<VoidTriplet>(2);
            factory->CreateFactory<TextText>(2);

            // NOT listening with no internal factory
            ::Thunder::Core::IPCChannelClientType<::Thunder::Core::Void, false, false> continousChannel(continousNode, 32, factory);

            ASSERT_EQ(continousChannel.Source().Open(maxWaitTime), ::Thunder::Core::ERROR_NONE);

            ::Thunder::Core::ProxyType<TripletResponse> tripletResponseData(::Thunder::Core::ProxyType<TripletResponse>::Create(Triplet(1, 2, 3)));
            ::Thunder::Core::ProxyType<VoidTriplet> voidTripletData(::Thunder::Core::ProxyType<VoidTriplet>::Create());

            const string text = "test text";
            ::Thunder::Core::ProxyType<TextText> textTextData(::Thunder::Core::ProxyType<TextText>::Create(::Thunder::Core::IPC::Text<2048>(text)));

            constexpr uint16_t display = 1;;
            constexpr uint32_t surface = 2;
            constexpr uint64_t context = 3;
            constexpr uint32_t result = 6;

            EXPECT_EQ(continousChannel.Invoke(tripletResponseData, maxWaitTime), ::Thunder::Core::ERROR_NONE);
            EXPECT_EQ(tripletResponseData->Response().Result(), result);
            ASSERT_EQ(testAdmin.Signal(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            EXPECT_EQ(continousChannel.Invoke(voidTripletData, maxWaitTime), ::Thunder::Core::ERROR_NONE);
            EXPECT_EQ(voidTripletData->Response().Display(), display);
            EXPECT_EQ(voidTripletData->Response().Surface(), surface);
            EXPECT_EQ(voidTripletData->Response().Context(), context);
            ASSERT_EQ(testAdmin.Signal(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            EXPECT_EQ(continousChannel.Invoke(textTextData, maxWaitTime), ::Thunder::Core::ERROR_NONE);
            EXPECT_STREQ(textTextData->Response().Value(), text.c_str());
            ASSERT_EQ(testAdmin.Signal(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            factory->DestroyFactory<TripletResponse>();
            factory->DestroyFactory<VoidTriplet>();
            factory->DestroyFactory<TextText>();

            // Only for internal factories
//            factory->DestroyFactories();
 
            ASSERT_EQ(continousChannel.Source().Close(maxWaitTime), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator testAdmin(callback_parent, callback_child, initHandshakeValue, maxWaitTime);

        // Code after this line is executed by both parent and child

        ::Thunder::Core::Singleton::Dispose();
    }

    TEST(Core_IPC, ContinuousChannelReversed)
    {
        const std::string connector = _T("/tmp/testserver1");

        constexpr uint32_t initHandshakeValue = 0, maxWaitTime = 4, maxInitTime = 2000;

        IPTestAdministrator::Callback callback_child = [&](IPTestAdministrator& testAdmin) {
            // A small delay so the parent can be set up
            SleepMs(maxInitTime);

            ::Thunder::Core::NodeId continousNode(connector.c_str());

            ::Thunder::Core::ProxyType<::Thunder::Core::FactoryType<::Thunder::Core::IIPC, uint32_t> > factory(::Thunder::Core::ProxyType<::Thunder::Core::FactoryType<::Thunder::Core::IIPC, uint32_t> >::Create());

            factory->CreateFactory<TripletResponse>(2);
            factory->CreateFactory<VoidTriplet>(2);
            factory->CreateFactory<TextText>(2);

            // NOT listening with no factory
            ::Thunder::Core::IPCChannelClientType<::Thunder::Core::Void, false, false> continousChannel(continousNode, 32, factory);

            ASSERT_EQ(continousChannel.Source().Open(maxWaitTime), ::Thunder::Core::ERROR_NONE);

            ::Thunder::Core::ProxyType<TripletResponse> tripletResponseData(::Thunder::Core::ProxyType<TripletResponse>::Create(Triplet(1, 2, 3)));
            ::Thunder::Core::ProxyType<VoidTriplet> voidTripletData(::Thunder::Core::ProxyType<VoidTriplet>::Create());

            const string text = "test text";
            ::Thunder::Core::ProxyType<TextText> textTextData(::Thunder::Core::ProxyType<TextText>::Create(::Thunder::Core::IPC::Text<2048>(text)));

            constexpr uint16_t display = 1;;
            constexpr uint32_t surface = 2;
            constexpr uint64_t context = 3;
            constexpr uint32_t result = 6;

            EXPECT_EQ(continousChannel.Invoke(tripletResponseData, maxWaitTime), ::Thunder::Core::ERROR_NONE);
            EXPECT_EQ(tripletResponseData->Response().Result(), result);
            ASSERT_EQ(testAdmin.Signal(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            EXPECT_EQ(continousChannel.Invoke(voidTripletData, maxWaitTime), ::Thunder::Core::ERROR_NONE);
            EXPECT_EQ(voidTripletData->Response().Display(), display);
            EXPECT_EQ(voidTripletData->Response().Surface(), surface);
            EXPECT_EQ(voidTripletData->Response().Context(), context);
            ASSERT_EQ(testAdmin.Signal(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            EXPECT_EQ(continousChannel.Invoke(textTextData, maxWaitTime), ::Thunder::Core::ERROR_NONE);
            EXPECT_STREQ(textTextData->Response().Value(), text.c_str());
            ASSERT_EQ(testAdmin.Signal(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            factory->DestroyFactory<TripletResponse>();
            factory->DestroyFactory<VoidTriplet>();
            factory->DestroyFactory<TextText>();

            // Only for internal factories
//            factory->DestroyFactories();

            EXPECT_EQ(continousChannel.Source().Close(maxWaitTime) /* Wait for 1 second */, ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator::Callback callback_parent = [&](IPTestAdministrator& testAdmin) {
            ::Thunder::Core::NodeId continousNode(connector.c_str());

            ::Thunder::Core::ProxyType<::Thunder::Core::FactoryType<::Thunder::Core::IIPC, uint32_t> > factory(::Thunder::Core::ProxyType<::Thunder::Core::FactoryType<::Thunder::Core::IIPC, uint32_t> >::Create());

            factory->CreateFactory<TripletResponse>(2);
            factory->CreateFactory<VoidTriplet>(2);
            factory->CreateFactory<TextText>(2);

            // Listening with no internal factory
            ::Thunder::Core::IPCChannelClientType<::Thunder::Core::Void, true, false> continousChannel(continousNode, 32, factory);

            ::Thunder::Core::ProxyType<::Thunder::Core::IIPCServer> handler1(::Thunder::Core::ProxyType<HandleTripletResponse>::Create());
            ::Thunder::Core::ProxyType<::Thunder::Core::IIPCServer> handler2(::Thunder::Core::ProxyType<HandleVoidTriplet>::Create());
            ::Thunder::Core::ProxyType<::Thunder::Core::IIPCServer> handler3(::Thunder::Core::ProxyType<HandleTextText>::Create());

            continousChannel.Register(TripletResponse::Id(), handler1);
            continousChannel.Register(VoidTriplet::Id(), handler2);
            continousChannel.Register(TextText::Id(), handler3);

            EXPECT_EQ(continousChannel.Source().Open(maxWaitTime), ::Thunder::Core::ERROR_NONE);

            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);
            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);
            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            continousChannel.Unregister(TripletResponse::Id());
            continousChannel.Unregister(VoidTriplet::Id());
            continousChannel.Unregister(TextText::Id());

            handler1.Release();
            handler2.Release();
            handler3.Release();

            factory->DestroyFactory<TripletResponse>();
            factory->DestroyFactory<VoidTriplet>();
            factory->DestroyFactory<TextText>();

            // Only for internal factories
//            factory->DestroyFactories();

            EXPECT_EQ(continousChannel.Source().Close(maxWaitTime), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator testAdmin(callback_parent, callback_child, initHandshakeValue, maxWaitTime);

        // Code after this line is executed by both parent and child

        ::Thunder::Core::Singleton::Dispose();
    }

    TEST(Core_IPC, FlashChannel)
    {
        constexpr uint32_t initHandshakeValue = 0, maxWaitTime = 4, maxInitTime = 2000;

        const std::string connector = _T("/tmp/testserver2");

        IPTestAdministrator::Callback callback_child = [&](IPTestAdministrator& testAdmin) {
            ::Thunder::Core::NodeId flashNode(connector.c_str());

            ::Thunder::Core::ProxyType<::Thunder::Core::FactoryType<::Thunder::Core::IIPC, uint32_t> > factory(::Thunder::Core::ProxyType<::Thunder::Core::FactoryType<::Thunder::Core::IIPC, uint32_t> >::Create());

            factory->CreateFactory<TripletResponse>(2);
            factory->CreateFactory<VoidTriplet>(2);
            factory->CreateFactory<TextText>(2);

            // Listening with no internal factory
            ::Thunder::Core::IPCChannelClientType<::Thunder::Core::Void, true, false> flashChannel(flashNode, 512, factory);

            ::Thunder::Core::ProxyType<::Thunder::Core::IIPCServer> handler1(::Thunder::Core::ProxyType<HandleTripletResponse>::Create());
            ::Thunder::Core::ProxyType<::Thunder::Core::IIPCServer> handler2(::Thunder::Core::ProxyType<HandleVoidTriplet>::Create());
            ::Thunder::Core::ProxyType<::Thunder::Core::IIPCServer> handler3(::Thunder::Core::ProxyType<HandleTextText>::Create());

            flashChannel.Register(TripletResponse::Id(), handler1);
            flashChannel.Register(VoidTriplet::Id(), handler2);
            flashChannel.Register(TextText::Id(), handler3);

            ASSERT_EQ(flashChannel.Source().Open(maxWaitTime), ::Thunder::Core::ERROR_NONE);

            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);
            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);
            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            flashChannel.Unregister(TripletResponse::Id());
            flashChannel.Unregister(VoidTriplet::Id());
            flashChannel.Unregister(TextText::Id());

            handler1.Release();
            handler2.Release();
            handler3.Release();

            factory->DestroyFactory<TripletResponse>();
            factory->DestroyFactory<VoidTriplet>();
            factory->DestroyFactory<TextText>();

            // Only for internal factories
//            factory->DestroyFactories();

            EXPECT_EQ(flashChannel.Source().Close(maxWaitTime), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator::Callback callback_parent = [&](IPTestAdministrator& testAdmin) {
            // A small delay so the child can be set up
            SleepMs(maxInitTime);

            ::Thunder::Core::NodeId flashNode(connector.c_str());

            ::Thunder::Core::ProxyType<::Thunder::Core::FactoryType<::Thunder::Core::IIPC, uint32_t> > factory(::Thunder::Core::ProxyType<::Thunder::Core::FactoryType<::Thunder::Core::IIPC, uint32_t> >::Create());

            factory->CreateFactory<TripletResponse>(2);
            factory->CreateFactory<VoidTriplet>(2);
            factory->CreateFactory<TextText>(2);

            // NOT listening with no internal factory
            ::Thunder::Core::IPCChannelClientType<::Thunder::Core::Void, false, false> flashChannel(flashNode, 512, factory);

            ASSERT_EQ(flashChannel.Source().Open(maxWaitTime), ::Thunder::Core::ERROR_NONE);

            ::Thunder::Core::ProxyType<TripletResponse> tripletResponseData(::Thunder::Core::ProxyType<TripletResponse>::Create(Triplet(1, 2, 3)));
            ::Thunder::Core::ProxyType<VoidTriplet> voidTripletData(::Thunder::Core::ProxyType<VoidTriplet>::Create());

            const string text = "test text";
            ::Thunder::Core::ProxyType<TextText> textTextData(::Thunder::Core::ProxyType<TextText>::Create(::Thunder::Core::IPC::Text<2048>(text)));

            constexpr uint16_t display = 1;;
            constexpr uint32_t surface = 2;
            constexpr uint64_t context = 3;
            constexpr uint32_t result = 6;

            EXPECT_EQ(flashChannel.Invoke(tripletResponseData, maxWaitTime), ::Thunder::Core::ERROR_NONE);
            EXPECT_EQ(tripletResponseData->Response().Result(), result);
            ASSERT_EQ(testAdmin.Signal(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            EXPECT_EQ(flashChannel.Invoke(voidTripletData, maxWaitTime), ::Thunder::Core::ERROR_NONE);
            EXPECT_EQ(voidTripletData->Response().Display(), display);
            EXPECT_EQ(voidTripletData->Response().Surface(), surface);
            EXPECT_EQ(voidTripletData->Response().Context(), context);
            ASSERT_EQ(testAdmin.Signal(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            EXPECT_EQ(flashChannel.Invoke(textTextData, maxWaitTime), ::Thunder::Core::ERROR_NONE);
            EXPECT_STREQ(textTextData->Response().Value(), text.c_str());
            ASSERT_EQ(testAdmin.Signal(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            factory->DestroyFactory<TripletResponse>();
            factory->DestroyFactory<VoidTriplet>();
            factory->DestroyFactory<TextText>();

            // Only for internal factories
//            factory->DestroyFactories();

            ASSERT_EQ(flashChannel.Source().Close(maxWaitTime), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator testAdmin(callback_parent, callback_child, initHandshakeValue, maxWaitTime);

        // Code after this line is executed by both parent and child

        ::Thunder::Core::Singleton::Dispose();
    }

    TEST(Core_IPC, FlashChannelReversed)
    {
        constexpr uint32_t initHandshakeValue = 0, maxWaitTime = 4, maxInitTime = 2000;

        const std::string connector = _T("/tmp/testserver3");

        IPTestAdministrator::Callback callback_child = [&](IPTestAdministrator& testAdmin) {
            // A small delay so the parent can be set up
            SleepMs(maxInitTime);

            ::Thunder::Core::NodeId flashNode(connector.c_str());

            ::Thunder::Core::ProxyType<::Thunder::Core::FactoryType<::Thunder::Core::IIPC, uint32_t> > factory(::Thunder::Core::ProxyType<::Thunder::Core::FactoryType<::Thunder::Core::IIPC, uint32_t> >::Create());

            factory->CreateFactory<TripletResponse>(2);
            factory->CreateFactory<VoidTriplet>(2);
            factory->CreateFactory<TextText>(2);

            // NOT listening with no internal factory
            ::Thunder::Core::IPCChannelClientType<::Thunder::Core::Void, false, false> flashChannel(flashNode, 512, factory);

            ASSERT_EQ(flashChannel.Source().Open(maxWaitTime), ::Thunder::Core::ERROR_NONE);

            ::Thunder::Core::ProxyType<TripletResponse> tripletResponseData(::Thunder::Core::ProxyType<TripletResponse>::Create(Triplet(1, 2, 3)));
            ::Thunder::Core::ProxyType<VoidTriplet> voidTripletData(::Thunder::Core::ProxyType<VoidTriplet>::Create());

            const string text = "test text";
            ::Thunder::Core::ProxyType<TextText> textTextData(::Thunder::Core::ProxyType<TextText>::Create(::Thunder::Core::IPC::Text<2048>(text)));

            constexpr uint16_t display = 1;;
            constexpr uint32_t surface = 2;
            constexpr uint64_t context = 3;
            constexpr uint32_t result = 6;

            EXPECT_EQ(flashChannel.Invoke(tripletResponseData, maxWaitTime), ::Thunder::Core::ERROR_NONE);
            EXPECT_EQ(tripletResponseData->Response().Result(), result);
            ASSERT_EQ(testAdmin.Signal(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            EXPECT_EQ(flashChannel.Invoke(voidTripletData, maxWaitTime), ::Thunder::Core::ERROR_NONE);
            EXPECT_EQ(voidTripletData->Response().Display(), display);
            EXPECT_EQ(voidTripletData->Response().Surface(), surface);
            EXPECT_EQ(voidTripletData->Response().Context(), context);
            ASSERT_EQ(testAdmin.Signal(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            EXPECT_EQ(flashChannel.Invoke(textTextData, maxWaitTime), ::Thunder::Core::ERROR_NONE);
            EXPECT_STREQ(textTextData->Response().Value(), text.c_str());
            ASSERT_EQ(testAdmin.Signal(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            factory->DestroyFactory<TripletResponse>();
            factory->DestroyFactory<VoidTriplet>();
            factory->DestroyFactory<TextText>();

            // Only for internal factories
//            factory->DestroyFactories();

            ASSERT_EQ(flashChannel.Source().Close(maxWaitTime), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator::Callback callback_parent = [&](IPTestAdministrator& testAdmin) {
            ::Thunder::Core::NodeId flashNode(connector.c_str());

            ::Thunder::Core::ProxyType<::Thunder::Core::FactoryType<::Thunder::Core::IIPC, uint32_t> > factory(::Thunder::Core::ProxyType<::Thunder::Core::FactoryType<::Thunder::Core::IIPC, uint32_t> >::Create());

            factory->CreateFactory<TripletResponse>(2);
            factory->CreateFactory<VoidTriplet>(2);
            factory->CreateFactory<TextText>(2);

            // Listening with no internal factory
            ::Thunder::Core::IPCChannelClientType<::Thunder::Core::Void, true, false> flashChannel(flashNode, 512, factory);

            ::Thunder::Core::ProxyType<::Thunder::Core::IIPCServer> handler1(::Thunder::Core::ProxyType<HandleTripletResponse>::Create());
            ::Thunder::Core::ProxyType<::Thunder::Core::IIPCServer> handler2(::Thunder::Core::ProxyType<HandleVoidTriplet>::Create());
            ::Thunder::Core::ProxyType<::Thunder::Core::IIPCServer> handler3(::Thunder::Core::ProxyType<HandleTextText>::Create());

            flashChannel.Register(TripletResponse::Id(), handler1);
            flashChannel.Register(VoidTriplet::Id(), handler2);
            flashChannel.Register(TextText::Id(), handler3);

            ASSERT_EQ(flashChannel.Source().Open(maxWaitTime), ::Thunder::Core::ERROR_NONE);

            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);
            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);
            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            flashChannel.Unregister(TripletResponse::Id());
            flashChannel.Unregister(VoidTriplet::Id());
            flashChannel.Unregister(TextText::Id());

            handler1.Release();
            handler2.Release();
            handler3.Release();

            factory->DestroyFactory<TripletResponse>();
            factory->DestroyFactory<VoidTriplet>();
            factory->DestroyFactory<TextText>();

            // Only for internal factories
//            factory->DestroyFactories();

            EXPECT_EQ(flashChannel.Source().Close(maxWaitTime), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator testAdmin(callback_parent, callback_child, initHandshakeValue, maxWaitTime);

        // Code after this line is executed by both parent and child

        ::Thunder::Core::Singleton::Dispose();
    }

    TEST(Core_IPC, MultiChannel)
    {
        constexpr uint32_t initHandshakeValue = 0, maxWaitTime = 4, maxInitTime = 2000;

        const std::string connector = _T("/tmp/testserver4");

        IPTestAdministrator::Callback callback_child = [&](IPTestAdministrator& testAdmin) {
            ::Thunder::Core::NodeId multiNode(connector.c_str());

            ::Thunder::Core::ProxyType<::Thunder::Core::FactoryType<::Thunder::Core::IIPC, uint32_t> > factory(::Thunder::Core::ProxyType<::Thunder::Core::FactoryType<::Thunder::Core::IIPC, uint32_t> >::Create());

            factory->CreateFactory<TripletResponse>(2);
            factory->CreateFactory<VoidTriplet>(2);
            factory->CreateFactory<TextText>(2);

            // Server (always listening) with no internal factory
            ::Thunder::Core::IPCChannelServerType<::Thunder::Core::Void, false> multiChannel(multiNode, 512, factory);

            ::Thunder::Core::ProxyType<::Thunder::Core::IIPCServer> handler1(::Thunder::Core::ProxyType<HandleTripletResponse>::Create());
            ::Thunder::Core::ProxyType<::Thunder::Core::IIPCServer> handler2(::Thunder::Core::ProxyType<HandleVoidTriplet>::Create());
            ::Thunder::Core::ProxyType<::Thunder::Core::IIPCServer> handler3(::Thunder::Core::ProxyType<HandleTextText>::Create());

            multiChannel.Register(TripletResponse::Id(), handler1);
            multiChannel.Register(VoidTriplet::Id(), handler2);
            multiChannel.Register(TextText::Id(), handler3);

            ASSERT_EQ(multiChannel.Open(maxWaitTime), ::Thunder::Core::ERROR_NONE);

            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);
            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);
            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            multiChannel.Unregister(TripletResponse::Id());
            multiChannel.Unregister(VoidTriplet::Id());
            multiChannel.Unregister(TextText::Id());

            handler1.Release();
            handler2.Release();
            handler3.Release();

            factory->DestroyFactory<TripletResponse>();
            factory->DestroyFactory<VoidTriplet>();
            factory->DestroyFactory<TextText>();

            // Only for internal factories
//            factory->DestroyFactories();
 
            // A server cannot 'end' its life if clients are connected
            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            // Do not unregister any / the last handler prior the call of cleanup
            multiChannel.Cleanup();

            ASSERT_EQ(multiChannel.Close(maxWaitTime), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator::Callback callback_parent = [&](IPTestAdministrator& testAdmin) {
            // A small delay so the child can be set up
            SleepMs(maxInitTime);

            ::Thunder::Core::NodeId multiNode(connector.c_str());

            ::Thunder::Core::ProxyType<::Thunder::Core::FactoryType<::Thunder::Core::IIPC, uint32_t> > factory(::Thunder::Core::ProxyType<::Thunder::Core::FactoryType<::Thunder::Core::IIPC, uint32_t> >::Create());

            factory->CreateFactory<TripletResponse>(2);
            factory->CreateFactory<VoidTriplet>(2);
            factory->CreateFactory<TextText>(2);

            // (Server client) NOT listening with no internal factory
            ::Thunder::Core::IPCChannelClientType<::Thunder::Core::Void, false, false> multiChannel(multiNode, 512, factory);

            ASSERT_EQ(multiChannel.Source().Open(maxWaitTime), ::Thunder::Core::ERROR_NONE);

            ::Thunder::Core::ProxyType<TripletResponse> tripletResponseData(::Thunder::Core::ProxyType<TripletResponse>::Create(Triplet(1, 2, 3)));
            ::Thunder::Core::ProxyType<VoidTriplet> voidTripletData(::Thunder::Core::ProxyType<VoidTriplet>::Create());

            const string text = "test text";
            ::Thunder::Core::ProxyType<TextText> textTextData(::Thunder::Core::ProxyType<TextText>::Create(::Thunder::Core::IPC::Text<2048>(text)));

            constexpr uint16_t display = 1;;
            constexpr uint32_t surface = 2;
            constexpr uint64_t context = 3;
            constexpr uint32_t result = 6;

            EXPECT_EQ(multiChannel.Invoke(tripletResponseData, maxWaitTime), ::Thunder::Core::ERROR_NONE);
            EXPECT_EQ(tripletResponseData->Response().Result(), result);
            ASSERT_EQ(testAdmin.Signal(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            EXPECT_EQ(multiChannel.Invoke(voidTripletData, maxWaitTime), ::Thunder::Core::ERROR_NONE);
            EXPECT_EQ(voidTripletData->Response().Display(), display);
            EXPECT_EQ(voidTripletData->Response().Surface(), surface);
            EXPECT_EQ(voidTripletData->Response().Context(), context);
            ASSERT_EQ(testAdmin.Signal(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            EXPECT_EQ(multiChannel.Invoke(textTextData, maxWaitTime), ::Thunder::Core::ERROR_NONE);
            EXPECT_STREQ(textTextData->Response().Value(), text.c_str());
            ASSERT_EQ(testAdmin.Signal(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            factory->DestroyFactory<TripletResponse>();
            factory->DestroyFactory<VoidTriplet>();
            factory->DestroyFactory<TextText>();

            // Only for internal factories
//            factory->DestroyFactories();

            ASSERT_EQ(multiChannel.Source().Close(maxWaitTime), ::Thunder::Core::ERROR_NONE);

            // Signal the server it can 'end' its life
            ASSERT_EQ(testAdmin.Signal(initHandshakeValue), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator testAdmin(callback_parent, callback_child, initHandshakeValue, maxWaitTime);

        // Code after this line is executed by both parent and child

        ::Thunder::Core::Singleton::Dispose();
    }

    TEST(Core_IPC, MultiChannelReversed)
    {
        constexpr uint32_t initHandshakeValue = 0, maxWaitTime = 4, maxInitTime = 2000;

        const std::string connector = _T("/tmp/testserver5");

        IPTestAdministrator::Callback callback_child = [&](IPTestAdministrator& testAdmin) {
            // A small delay so the parent can be set up
            SleepMs(maxInitTime);

            ::Thunder::Core::NodeId multiNode(connector.c_str());

            ::Thunder::Core::ProxyType<::Thunder::Core::FactoryType<::Thunder::Core::IIPC, uint32_t> > factory(::Thunder::Core::ProxyType<::Thunder::Core::FactoryType<::Thunder::Core::IIPC, uint32_t> >::Create());

            factory->CreateFactory<TripletResponse>(2);
            factory->CreateFactory<VoidTriplet>(2);
            factory->CreateFactory<TextText>(2);

            // (Server client) NOT listening with no internal factory
            ::Thunder::Core::IPCChannelClientType<::Thunder::Core::Void, false, false> multiChannel(multiNode, 512, factory);

            ASSERT_EQ(multiChannel.Source().Open(maxWaitTime), ::Thunder::Core::ERROR_NONE);

            ::Thunder::Core::ProxyType<TripletResponse> tripletResponseData(::Thunder::Core::ProxyType<TripletResponse>::Create(Triplet(1, 2, 3)));
            ::Thunder::Core::ProxyType<VoidTriplet> voidTripletData(::Thunder::Core::ProxyType<VoidTriplet>::Create());

            const string text = "test text";
            ::Thunder::Core::ProxyType<TextText> textTextData(::Thunder::Core::ProxyType<TextText>::Create(::Thunder::Core::IPC::Text<2048>(text)));

            constexpr uint16_t display = 1;;
            constexpr uint32_t surface = 2;
            constexpr uint64_t context = 3;
            constexpr uint32_t result = 6;

            EXPECT_EQ(multiChannel.Invoke(tripletResponseData, maxWaitTime), ::Thunder::Core::ERROR_NONE);
            EXPECT_EQ(tripletResponseData->Response().Result(), result);
            ASSERT_EQ(testAdmin.Signal(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            EXPECT_EQ(multiChannel.Invoke(voidTripletData, maxWaitTime), ::Thunder::Core::ERROR_NONE);
            EXPECT_EQ(voidTripletData->Response().Display(), display);
            EXPECT_EQ(voidTripletData->Response().Surface(), surface);
            EXPECT_EQ(voidTripletData->Response().Context(), context);
            ASSERT_EQ(testAdmin.Signal(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            EXPECT_EQ(multiChannel.Invoke(textTextData, maxWaitTime), ::Thunder::Core::ERROR_NONE);
            EXPECT_STREQ(textTextData->Response().Value(), text.c_str());
            ASSERT_EQ(testAdmin.Signal(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            factory->DestroyFactory<TripletResponse>();
            factory->DestroyFactory<VoidTriplet>();
            factory->DestroyFactory<TextText>();

            // Only for internal factories
//            factory->DestroyFactories();

            ASSERT_EQ(multiChannel.Source().Close(maxWaitTime), ::Thunder::Core::ERROR_NONE);

            // Signal the server it can 'end' its life
            ASSERT_EQ(testAdmin.Signal(initHandshakeValue), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator::Callback callback_parent = [&](IPTestAdministrator& testAdmin) {
            ::Thunder::Core::NodeId multiNode(connector.c_str());

            ::Thunder::Core::ProxyType<::Thunder::Core::FactoryType<::Thunder::Core::IIPC, uint32_t> > factory(::Thunder::Core::ProxyType<::Thunder::Core::FactoryType<::Thunder::Core::IIPC, uint32_t> >::Create());

            factory->CreateFactory<TripletResponse>(2);
            factory->CreateFactory<VoidTriplet>(2);
            factory->CreateFactory<TextText>(2);

            // Server (always listening) with no internal factory
            ::Thunder::Core::IPCChannelServerType<::Thunder::Core::Void, false> multiChannel(multiNode, 512, factory);

            ::Thunder::Core::ProxyType<::Thunder::Core::IIPCServer> handler1(::Thunder::Core::ProxyType<HandleTripletResponse>::Create());
            ::Thunder::Core::ProxyType<::Thunder::Core::IIPCServer> handler2(::Thunder::Core::ProxyType<HandleVoidTriplet>::Create());
            ::Thunder::Core::ProxyType<::Thunder::Core::IIPCServer> handler3(::Thunder::Core::ProxyType<HandleTextText>::Create());

            ASSERT_EQ(multiChannel.Open(maxWaitTime), ::Thunder::Core::ERROR_NONE);

            multiChannel.Register(TripletResponse::Id(), handler1);
            multiChannel.Register(VoidTriplet::Id(), handler2);
            multiChannel.Register(TextText::Id(), handler3);

            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);
            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);
            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            multiChannel.Unregister(TripletResponse::Id());
            multiChannel.Unregister(VoidTriplet::Id());
            multiChannel.Unregister(TextText::Id());

            handler1.Release();
            handler2.Release();
            handler3.Release();

            factory->DestroyFactory<TripletResponse>();
            factory->DestroyFactory<VoidTriplet>();
            factory->DestroyFactory<TextText>();

            // Only for internal factories
//           factory->DestroyFactories();

            // A server cannot 'end' its life if clients are connected
            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            multiChannel.Cleanup();

            ASSERT_EQ(multiChannel.Close(maxWaitTime), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator testAdmin(callback_parent, callback_child, initHandshakeValue, maxWaitTime);

        // Code after this line is executed by both parent and child

        ::Thunder::Core::Singleton::Dispose();
    }

} // Core
} // Tests
} // Thunder
