#include <gtest/gtest.h>

#ifndef MODULE_NAME
#include "../Module.h"
#endif

#include <core/core.h>

namespace Thunder {
namespace Tests {
namespace {

    // PUBLIC_INTERFACE
    /** A synchronous receive-only transport for deterministic chunk delivery. */
    class MemoryJSONSource {
    public:
        // PUBLIC_INTERFACE
        /** Deliver exactly one chunk through the StreamJSON handler override. */
        uint16_t Feed(const string& chunk)
        {
            std::vector<uint8_t> bytes(chunk.begin(), chunk.end());
            return ReceiveData(bytes.data(), static_cast<uint16_t>(bytes.size()));
        }
        // PUBLIC_INTERFACE
        /** Close this inert test transport; no external resource is owned. */
        uint32_t Close(uint32_t)
        {
            return Core::ERROR_NONE;
        }

    protected:
        virtual uint16_t ReceiveData(uint8_t*, uint16_t) = 0;
    };

    // PUBLIC_INTERFACE
    /** A minimal JSON message whose completion can be observed by the test. */
    class RecoveryMessage : public Core::JSON::Container {
    public:
        // PUBLIC_INTERFACE
        /** Register a numeric ID in the real JSON parser. */
        RecoveryMessage()
            : Core::JSON::Container()
            , Id(0)
        {
            Add(_T("id"), &Id);
        }
        Core::JSON::DecUInt32 Id;
    };

    // PUBLIC_INTERFACE
    /** Allocate actual JSON elements without a socket or singleton factory. */
    class RecoveryFactory : public Core::ProxyPoolType<RecoveryMessage> {
    public:
        // PUBLIC_INTERFACE
        /** Initialize a bounded pool for the synchronous test stream. */
        RecoveryFactory()
            : Core::ProxyPoolType<RecoveryMessage>(2)
        {
        }
        // PUBLIC_INTERFACE
        /** Return a pooled message through the JSON element interface. */
        Core::ProxyType<Core::JSON::IElement> Element(const string&)
        {
            return Core::ProxyType<Core::JSON::IElement>(Core::ProxyPoolType<RecoveryMessage>::Element());
        }
    };

    // PUBLIC_INTERFACE
    /** Capture messages delivered by the real StreamJSON deserializer. */
    class RecoveryStream : public Core::StreamJSONType<MemoryJSONSource, RecoveryFactory&, Core::JSON::IElement> {
        using Base = Core::StreamJSONType<MemoryJSONSource, RecoveryFactory&, Core::JSON::IElement>;
    public:
        // PUBLIC_INTERFACE
        /** Bind the already-constructed factory to the receive stream. */
        explicit RecoveryStream(RecoveryFactory& factory)
            : Base(2, factory)
        {
        }
        // PUBLIC_INTERFACE
        /** Record each completed JSON message for exact count/content checks. */
        void Received(Core::ProxyType<Core::JSON::IElement>& element) override
        {
            string text;
            element->ToString(text);
            Messages.push_back(text);
        }
        // PUBLIC_INTERFACE
        /** No outbound messages are sent by this receive-only fixture. */
        void Send(Core::ProxyType<Core::JSON::IElement>&) override { }
        // PUBLIC_INTERFACE
        /** The synchronous fixture has no asynchronous state changes. */
        void StateChange() override { }

        std::vector<string> Messages;
    };

} // namespace

TEST(Core_StreamJSONRecovery, InvalidPrefixThenTwoMessagesInOneChunk)
{
    RecoveryFactory factory;
    RecoveryStream stream(factory);
    // Repeated errors also cross the log-suppression threshold without pinning
    // the receive buffer or delivering spurious empty messages.
    const string chunk = string(210, '\x02') + R"({"id":1}{"id":2})";
    EXPECT_EQ(chunk.size(), stream.Link().Feed(chunk));
    ASSERT_EQ(2u, stream.Messages.size());
    EXPECT_EQ(R"({"id":1})", stream.Messages[0]);
    EXPECT_EQ(R"({"id":2})", stream.Messages[1]);
}

TEST(Core_StreamJSONRecovery, InvalidPrefixPreservesPartialTrailingMessage)
{
    RecoveryFactory factory;
    RecoveryStream stream(factory);
    const string first = string(1, '\x02') + R"({"id":)";
    EXPECT_EQ(first.size(), stream.Link().Feed(first));
    EXPECT_TRUE(stream.Messages.empty());
    EXPECT_EQ(2u, stream.Link().Feed("7}"));
    ASSERT_EQ(1u, stream.Messages.size());
    EXPECT_EQ(R"({"id":7})", stream.Messages[0]);
}

TEST(Core_StreamJSONRecovery, ErrorOnlyChunkThenValidMessage)
{
    RecoveryFactory factory;
    RecoveryStream stream(factory);
    EXPECT_EQ(1u, stream.Link().Feed(string(1, '\x02')));
    EXPECT_TRUE(stream.Messages.empty());
    const string valid = R"({"id":3})";
    EXPECT_EQ(valid.size(), stream.Link().Feed(valid));
    ASSERT_EQ(1u, stream.Messages.size());
    EXPECT_EQ(valid, stream.Messages[0]);
}

} // namespace Tests
} // namespace Thunder
