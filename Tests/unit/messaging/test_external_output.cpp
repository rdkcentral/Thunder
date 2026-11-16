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

namespace Thunder {
namespace Tests {
namespace Messaging {

    namespace {
        using Metadata = Core::Messaging::Metadata;
        using OutputMode = Core::Messaging::OutputMode;
        using OutputTargets = Core::Messaging::OutputTargets;

        class ExternalOutputTest : public testing::Test {
        protected:
            void TearDown() override
            {
                if (_opened == true) {
                    ::Thunder::Messaging::MessageUnit::Instance().Close();
                }
            }

            void Open()
            {
                ::Thunder::Messaging::MessageUnit::Settings::Config configuration;
                configuration.Path = _T("ExternalOutputTests");
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

        private:
            bool _opened { false };
        };

        TEST_F(ExternalOutputTest, RoutesHandlerAndDirectWithoutExternalOutput)
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

        TEST_F(ExternalOutputTest, RoutesExternalDirectOnlyWhenExternallyEnabled)
        {
            Open();
            const OutputTargets targets = Targets(
                Metadata(Metadata::type::LOGGING, _T("Information"), _T("Module")), OutputMode::EXTERNAL_DIRECT, false);

            EXPECT_FALSE(targets.handler);
            EXPECT_FALSE(targets.direct);
            EXPECT_TRUE(targets.external);
        }

        TEST_F(ExternalOutputTest, RoutesAllLocalAndExternalDestinations)
        {
            Open();
            const OutputTargets targets = Targets(
                Metadata(Metadata::type::LOGGING, _T("Information"), _T("Module")), OutputMode::ALL, true);

            EXPECT_TRUE(targets.handler);
            EXPECT_TRUE(targets.direct);
            EXPECT_TRUE(targets.external);
        }

        TEST_F(ExternalOutputTest, RoutesExternalOutputWhenLocalOutputIsDisabled)
        {
            Open();
            const OutputTargets targets = Targets(
                Metadata(Metadata::type::LOGGING, _T("Information"), _T("Module")), OutputMode::EXTERNAL_DIRECT, false);

            EXPECT_TRUE(targets.Any());
            EXPECT_TRUE(targets.external);
        }

        TEST_F(ExternalOutputTest, RoutesExternallyEnabledAssertionsWhenLocalAssertionIsDisabled)
        {
            Open();
            ::Thunder::Assertion::AssertionUnit::Instance();

            const OutputTargets targets = ::Thunder::Assertion::AssertionUnitProxy::Instance().Targets(
                ::Thunder::Assertion::BaseAssertType::Metadata(), OutputMode::EXTERNAL_DIRECT, false);

            EXPECT_FALSE(targets.direct);
            EXPECT_FALSE(targets.handler);
            EXPECT_TRUE(targets.external);
        }

        TEST_F(ExternalOutputTest, DoesNotDuplicateHandlerRoutedTelemetryExternally)
        {
            Open();
            const Metadata metadata(Metadata::type::TELEMETRY, _T("Metric"), _T("TelemetryModule"));

            const OutputTargets all = Targets(metadata, OutputMode::ALL, true);
            EXPECT_TRUE(all.handler);
            EXPECT_FALSE(all.external);

            const OutputTargets external = Targets(metadata, OutputMode::EXTERNAL_DIRECT, false);
            EXPECT_TRUE(external.external);
        }

    }
}
}
}
