/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 Metrological
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

#include <core/core.h>
#include <messaging/messaging.h>

#include "RDKLoggerOutputMock.h"

namespace Thunder {
namespace Tests {
namespace Messaging {

    namespace {
        using Metadata = Core::Messaging::Metadata;
        using OutputMode = Core::Messaging::OutputMode;
        using OutputTargets = Core::Messaging::OutputTargets;

        class TestControl : public Core::Messaging::IControl {
        public:
            TestControl(const Core::Messaging::Metadata& metadata)
                : _metadata(metadata)
                , _enabled(true)
                , _routing(OutputMode::HANDLER)
            {
            }

            void Enable(const bool enabled) override
            {
                _enabled = enabled;
            }

            bool Enable() const override
            {
                return (_enabled);
            }

            void Routing(const OutputMode routing) override
            {
                _routing = routing;
            }

            OutputMode Routing() const override
            {
                return (_routing);
            }

            void Destroy() override
            {
                _enabled = false;
            }

            const Core::Messaging::Metadata& Metadata() const override
            {
                return (_metadata);
            }

        private:
            Core::Messaging::Metadata _metadata;
            bool _enabled;
            OutputMode _routing;
        };

        class RDKLoggerTest : public testing::Test {
        protected:
            void SetUp() override
            {
                ThunderExternalOutputMock_Reset();
            }

            void TearDown() override
            {
                if (_opened == true) {
                    ::Thunder::Messaging::MessageUnit::Instance().Close();
                }

                for (auto& control : _controls) {
                    Core::Messaging::IControl::Revoke(control.get());
                }
            }

            TestControl& Announce(const Metadata& metadata)
            {
                std::unique_ptr<TestControl> control(new TestControl(metadata));
                TestControl& result = *control;
                Core::Messaging::IControl::Announce(control.get());
                _controls.emplace_back(std::move(control));
                return (result);
            }

            void Open()
            {
                ::Thunder::Messaging::MessageUnit::Settings::Config configuration;
                configuration.Path = _T("RDKLoggerTests");
                configuration.Out = false;
                configuration.Error = false;

                ASSERT_EQ(Core::ERROR_NONE, ::Thunder::Messaging::MessageUnit::Instance().Open(
                    _T("/tmp/"), configuration, false, ::Thunder::Messaging::MessageUnit::OFF));
                _opened = true;
            }

            OutputTargets Targets(const Metadata& metadata, const OutputMode mode, const bool localEnabled)
            {
                return (::Thunder::Messaging::MessageUnit::Instance().Targets(metadata, mode, localEnabled));
            }

            void Submit(const Metadata& metadata, const string& payload, const OutputTargets& targets = { false, false, true })
            {
                Core::Messaging::MessageInfo information(metadata);
                Core::Messaging::TextMessage message(payload);
                ::Thunder::Messaging::MessageUnit::Instance().Push(information, &message, targets);
            }

        private:
            bool _opened { false };
            std::vector<std::unique_ptr<TestControl>> _controls;
        };

        struct MappingCase {
            Metadata::type type;
            const char* category;
            ThunderExternalLogLevel level;
        };

        class RDKLoggerMappingTest : public RDKLoggerTest, public testing::WithParamInterface<MappingCase> {
        };

        TEST_P(RDKLoggerMappingTest, MapsThunderTypeAndCategoryToExternalLevel)
        {
            const MappingCase& test = GetParam();
            Submit(Metadata(test.type, test.category, _T("Plugin.Module")), _T("payload"));

            EXPECT_EQ(1u, ThunderExternalOutputMock_SubmitCount());
            EXPECT_STREQ("Plugin.Module", ThunderExternalOutputMock_LastModule());
            EXPECT_EQ(test.level, ThunderExternalOutputMock_LastLevel());
        }

        INSTANTIATE_TEST_SUITE_P(AllMessageTypes, RDKLoggerMappingTest, testing::Values(
            MappingCase { Metadata::type::ASSERT, "Assert", THUNDER_EXTERNAL_LOG_LEVEL_FATAL },
            MappingCase { Metadata::type::REPORTING, "Warning", THUNDER_EXTERNAL_LOG_LEVEL_WARN },
            MappingCase { Metadata::type::OPERATIONAL_STREAM, "Operational", THUNDER_EXTERNAL_LOG_LEVEL_TRACE },
            MappingCase { Metadata::type::TELEMETRY, "Metric", THUNDER_EXTERNAL_LOG_LEVEL_NOTICE },
            MappingCase { Metadata::type::TRACING, "Fatal", THUNDER_EXTERNAL_LOG_LEVEL_FATAL },
            MappingCase { Metadata::type::TRACING, "Crash", THUNDER_EXTERNAL_LOG_LEVEL_FATAL },
            MappingCase { Metadata::type::TRACING, "Error", THUNDER_EXTERNAL_LOG_LEVEL_ERROR },
            MappingCase { Metadata::type::TRACING, "Warning", THUNDER_EXTERNAL_LOG_LEVEL_WARN },
            MappingCase { Metadata::type::TRACING, "Warn", THUNDER_EXTERNAL_LOG_LEVEL_WARN },
            MappingCase { Metadata::type::TRACING, "Notice", THUNDER_EXTERNAL_LOG_LEVEL_NOTICE },
            MappingCase { Metadata::type::TRACING, "Information", THUNDER_EXTERNAL_LOG_LEVEL_TRACE },
            MappingCase { Metadata::type::LOGGING, "Fatal", THUNDER_EXTERNAL_LOG_LEVEL_FATAL },
            MappingCase { Metadata::type::LOGGING, "Crash", THUNDER_EXTERNAL_LOG_LEVEL_FATAL },
            MappingCase { Metadata::type::LOGGING, "Error", THUNDER_EXTERNAL_LOG_LEVEL_ERROR },
            MappingCase { Metadata::type::LOGGING, "Warning", THUNDER_EXTERNAL_LOG_LEVEL_WARN },
            MappingCase { Metadata::type::LOGGING, "Warn", THUNDER_EXTERNAL_LOG_LEVEL_WARN },
            MappingCase { Metadata::type::LOGGING, "Notice", THUNDER_EXTERNAL_LOG_LEVEL_NOTICE },
            MappingCase { Metadata::type::LOGGING, "Information", THUNDER_EXTERNAL_LOG_LEVEL_INFO }));

        TEST_F(RDKLoggerTest, PrefixesDefaultLoggingAndTracingPayloads)
        {
            Submit(Metadata(Metadata::type::LOGGING, _T("Information"), _T("LoggingModule")), _T("log payload"));
            EXPECT_STREQ("Information: log payload", ThunderExternalOutputMock_LastPayload());

            Submit(Metadata(Metadata::type::TRACING, _T("Information"), _T("TracingModule")), _T("trace payload"));
            EXPECT_STREQ("Information: trace payload", ThunderExternalOutputMock_LastPayload());
        }

        TEST_F(RDKLoggerTest, PreservesExplicitSeverityPayload)
        {
            Submit(Metadata(Metadata::type::LOGGING, _T("Error"), _T("Module")), _T("original payload"));
            EXPECT_STREQ("original payload", ThunderExternalOutputMock_LastPayload());
        }

        TEST_F(RDKLoggerTest, RoutesHandlerAndDirectWithoutExternalOutput)
        {
            Open();
            const Metadata metadata(Metadata::type::LOGGING, _T("Information"), _T("Module"));

            const OutputTargets handler = Targets(metadata, OutputMode::HANDLER, true);
            EXPECT_TRUE(handler.handler);
            EXPECT_FALSE(handler.direct);
            EXPECT_FALSE(handler.external);

            const OutputTargets direct = Targets(metadata, OutputMode::DIRECT, true);
            EXPECT_FALSE(direct.handler);
            EXPECT_TRUE(direct.direct);
            EXPECT_FALSE(direct.external);
        }

        TEST_F(RDKLoggerTest, RoutesExternalDirectOnlyWhenExternallyEnabled)
        {
            Open();
            const OutputTargets targets = Targets(
                Metadata(Metadata::type::LOGGING, _T("Information"), _T("Module")), OutputMode::EXTERNAL_DIRECT, false);

            EXPECT_FALSE(targets.handler);
            EXPECT_FALSE(targets.direct);
            EXPECT_TRUE(targets.external);
        }

        TEST_F(RDKLoggerTest, RoutesAllLocalAndExternalDestinations)
        {
            Open();
            const OutputTargets targets = Targets(
                Metadata(Metadata::type::LOGGING, _T("Information"), _T("Module")), OutputMode::ALL, true);

            EXPECT_TRUE(targets.handler);
            EXPECT_TRUE(targets.direct);
            EXPECT_TRUE(targets.external);
        }

        TEST_F(RDKLoggerTest, KeepsLocalRouteWhenExternalOutputIsDisabled)
        {
            ThunderExternalOutputMock_SetEnabled(0);
            Open();
            const OutputTargets targets = Targets(
                Metadata(Metadata::type::LOGGING, _T("Information"), _T("Module")), OutputMode::ALL, true);

            EXPECT_TRUE(targets.handler);
            EXPECT_TRUE(targets.direct);
            EXPECT_FALSE(targets.external);
        }

        TEST_F(RDKLoggerTest, RoutesExternalOutputWhenLocalOutputIsDisabled)
        {
            Open();
            const OutputTargets targets = Targets(
                Metadata(Metadata::type::LOGGING, _T("Information"), _T("Module")), OutputMode::EXTERNAL_DIRECT, false);

            EXPECT_TRUE(targets.Any());
            EXPECT_TRUE(targets.external);
        }

        TEST_F(RDKLoggerTest, RoutesExternallyEnabledAssertionsWhenLocalAssertionIsDisabled)
        {
            Open();
            ::Thunder::Assertion::AssertionUnit::Instance();

            const OutputTargets targets = ::Thunder::Assertion::AssertionUnitProxy::Instance().Targets(
                ::Thunder::Assertion::BaseAssertType::Metadata(), OutputMode::EXTERNAL_DIRECT, false);

            EXPECT_FALSE(targets.direct);
            EXPECT_FALSE(targets.handler);
            EXPECT_TRUE(targets.external);
        }

        TEST_F(RDKLoggerTest, ReportsNoInterestWhenBothGatesAreDisabled)
        {
            ThunderExternalOutputMock_SetEnabled(0);
            Open();
            const OutputTargets targets = Targets(
                Metadata(Metadata::type::LOGGING, _T("Information"), _T("Module")), OutputMode::EXTERNAL_DIRECT, false);

            EXPECT_FALSE(targets.Any());
            EXPECT_EQ(0u, ThunderExternalOutputMock_SubmitCount());
        }

        TEST_F(RDKLoggerTest, DoesNotDuplicateHandlerRoutedTelemetryExternally)
        {
            Open();
            const Metadata metadata(Metadata::type::TELEMETRY, _T("Metric"), _T("TelemetryModule"));

            const OutputTargets all = Targets(metadata, OutputMode::ALL, true);
            EXPECT_TRUE(all.handler);
            EXPECT_FALSE(all.external);

            const OutputTargets external = Targets(metadata, OutputMode::EXTERNAL_DIRECT, false);
            EXPECT_TRUE(external.external);
        }

        TEST_F(RDKLoggerTest, EagerlyPopulatesAndDeduplicatesKnownControlPairs)
        {
            Announce(Metadata(Metadata::type::TRACING, _T("Information"), _T("CachedModule")));
            Announce(Metadata(Metadata::type::TRACING, _T("Debug"), _T("CachedModule")));

            Open();

            EXPECT_EQ(1u, ThunderExternalOutputMock_QueryCountFor("CachedModule", THUNDER_EXTERNAL_LOG_LEVEL_TRACE));
        }

        TEST_F(RDKLoggerTest, QueriesNewlyAnnouncedControlImmediately)
        {
            Open();
            EXPECT_EQ(0u, ThunderExternalOutputMock_QueryCountFor("LateModule", THUNDER_EXTERNAL_LOG_LEVEL_WARN));

            Announce(Metadata(Metadata::type::LOGGING, _T("Warning"), _T("LateModule")));

            EXPECT_EQ(1u, ThunderExternalOutputMock_QueryCountFor("LateModule", THUNDER_EXTERNAL_LOG_LEVEL_WARN));
        }

        TEST_F(RDKLoggerTest, ReusesCachedEnablementDuringRouting)
        {
            Open();
            const Metadata metadata(Metadata::type::LOGGING, _T("Notice"), _T("CachedModule"));

            Targets(metadata, OutputMode::EXTERNAL_DIRECT, false);
            Targets(metadata, OutputMode::EXTERNAL_DIRECT, false);

            EXPECT_EQ(1u, ThunderExternalOutputMock_QueryCountFor("CachedModule", THUNDER_EXTERNAL_LOG_LEVEL_NOTICE));
        }

        TEST_F(RDKLoggerTest, InitializesAndDeinitializesWithMessaging)
        {
            Open();
            EXPECT_EQ(1u, ThunderExternalOutputMock_InitializeCount());

            ::Thunder::Messaging::MessageUnit::Instance().Close();
            EXPECT_EQ(1u, ThunderExternalOutputMock_DeinitializeCount());
        }

        TEST_F(RDKLoggerTest, InitializationFailureDisablesOnlyExternalOutput)
        {
            ThunderExternalOutputMock_SetInitializeResult(1);
            Open();
            const OutputTargets targets = Targets(
                Metadata(Metadata::type::LOGGING, _T("Information"), _T("Module")), OutputMode::ALL, true);

            EXPECT_TRUE(targets.handler);
            EXPECT_TRUE(targets.direct);
            EXPECT_FALSE(targets.external);
        }

        TEST_F(RDKLoggerTest, SubmissionFailureDoesNotChangeSelectedLocalRoutes)
        {
            ThunderExternalOutputMock_SetSubmitResult(1);
            Open();
            const Metadata metadata(Metadata::type::LOGGING, _T("Error"), _T("Module"));
            const OutputTargets targets = Targets(metadata, OutputMode::ALL, true);

            Submit(metadata, _T("payload"), targets);

            EXPECT_TRUE(targets.handler);
            EXPECT_TRUE(targets.direct);
            EXPECT_EQ(1u, ThunderExternalOutputMock_SubmitCount());
        }

        TEST_F(RDKLoggerTest, AllSubmitsExternallyExactlyOnce)
        {
            Open();
            const Metadata metadata(Metadata::type::LOGGING, _T("Error"), _T("Module"));
            const OutputTargets targets = Targets(metadata, OutputMode::ALL, true);

            Submit(metadata, _T("payload"), targets);

            EXPECT_EQ(1u, ThunderExternalOutputMock_SubmitCount());
        }
    }
}
}
}
