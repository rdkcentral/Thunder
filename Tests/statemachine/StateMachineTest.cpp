#include "Module.h"

#include "ObserverControl.h"
#include "ReentrantControl.h"
#include "StateControlControl.h"

#include "Thunder/PluginServer.h"
#include "ThunderTestRuntime.h"

#include <gtest/gtest.h>
#include <vector>

namespace Thunder {
namespace TestCore {
    namespace Tests {

        class StateMachineBaseline : public ::testing::Test {
        protected:
            static ThunderTestRuntime _runtime;

            static void SetUpTestSuite()
            {
                std::vector<ThunderTestRuntime::PluginConfig> plugins;

                ThunderTestRuntime::PluginConfig brave;
                brave.Callsign = _T("Brave");
                brave.ClassName = _T("BravePlugin"); // must match the bare class name
                brave.Locator = _T(""); // empty -> in-process lookup
                brave.StartMode = PluginHost::IShell::startmode::DEACTIVATED;
                plugins.push_back(brave);

                ThunderTestRuntime::PluginConfig broken;
                broken.Callsign = _T("Broken");
                broken.ClassName = _T("DoesNotExist"); // not registered in-process
                broken.Locator = _T("");
                broken.StartMode = PluginHost::IShell::startmode::DEACTIVATED;
                plugins.push_back(broken);

                ThunderTestRuntime::PluginConfig failInit;
                failInit.Callsign = _T("FailInit");
                failInit.ClassName = _T("FailInitPlugin");
                failInit.Locator = _T("");
                failInit.StartMode = PluginHost::IShell::startmode::DEACTIVATED;
                plugins.push_back(failInit);

                ThunderTestRuntime::PluginConfig streamer;
                streamer.Callsign = _T("Streamer");
                streamer.ClassName = _T("StreamingProvider");
                streamer.Locator = _T(TEST_PLUGIN_LOCATOR); // real .so from the build tree
                streamer.StartMode = PluginHost::IShell::startmode::DEACTIVATED;
                plugins.push_back(streamer);

                ThunderTestRuntime::PluginConfig depStreaming;
                depStreaming.Callsign = _T("DepStreaming");
                depStreaming.ClassName = _T("BravePlugin");
                depStreaming.Locator = _T("");
                depStreaming.StartMode = PluginHost::IShell::startmode::DEACTIVATED;
                depStreaming.Precondition.Add() = PluginHost::ISubSystem::STREAMING;
                depStreaming.Termination.Add() = PluginHost::ISubSystem::NOT_STREAMING;
                plugins.push_back(depStreaming);

                ThunderTestRuntime::PluginConfig reentrant;
                reentrant.Callsign = _T("Reentrant");
                reentrant.ClassName = _T("ReentrantPlugin");
                reentrant.Locator = _T("");
                reentrant.StartMode = PluginHost::IShell::startmode::DEACTIVATED;
                plugins.push_back(reentrant);

                ThunderTestRuntime::PluginConfig observer;
                observer.Callsign = _T("Observer");
                observer.ClassName = _T("ObserverPlugin");
                observer.Locator = _T("");
                observer.StartMode = PluginHost::IShell::startmode::ACTIVATED;
                plugins.push_back(observer);

                ThunderTestRuntime::PluginConfig stateCtrl;
                stateCtrl.Callsign = _T("StateCtrl");
                stateCtrl.ClassName = _T("StateControlPlugin");
                stateCtrl.Locator = _T("");
                stateCtrl.StartMode = PluginHost::IShell::startmode::DEACTIVATED;
                plugins.push_back(stateCtrl);

                ThunderTestRuntime::PluginConfig stateCtrlNoResume;
                stateCtrlNoResume.Callsign = _T("StateCtrlNoResume");
                stateCtrlNoResume.ClassName = _T("StateControlPlugin");
                stateCtrlNoResume.Locator = _T("");
                stateCtrlNoResume.StartMode = PluginHost::IShell::startmode::DEACTIVATED;
                stateCtrlNoResume.Resumed = false;
                plugins.push_back(stateCtrlNoResume);

                ThunderTestRuntime::PluginConfig stateCtrlOOP;
                stateCtrlOOP.Callsign = _T("StateCtrlOOP");
                stateCtrlOOP.ClassName = _T("StateControlOOPPlugin");
                stateCtrlOOP.Locator = _T(STATECTRL_OOP_LOCATOR); // real .so from the build tree
                stateCtrlOOP.StartMode = PluginHost::IShell::startmode::DEACTIVATED;
                plugins.push_back(stateCtrlOOP);

                // ASSERT_EQ(_runtime.Initialize(plugins), Core::ERROR_NONE);
                ASSERT_EQ(_runtime.Initialize(plugins, _T(TEST_PLUGIN_DIR)), Core::ERROR_NONE);
            }

            static void TearDownTestSuite()
            {
                _runtime.Deinitialize();
                Core::Singleton::Dispose();
            }

            static void EnsureDeactivated(const string& callsign)
            {
                auto shell = _runtime.GetShell(callsign);
                ASSERT_TRUE(shell.IsValid());
                if (shell->State() != PluginHost::IShell::DEACTIVATED) {
                    shell->Deactivate(PluginHost::IShell::reason::REQUESTED);
                }
                ASSERT_EQ(shell->State(), PluginHost::IShell::DEACTIVATED);
            }

            static bool WaitForState(const Core::ProxyType<PluginHost::IShell>& shell,
                const PluginHost::IShell::state expected,
                const uint32_t timeoutMs = 5000)
            {
                uint32_t waited = 0;
                while ((shell->State() != expected) && (waited < timeoutMs)) {
                    SleepMs(10);
                    waited += 10;
                }
                return (shell->State() == expected);
            }

            void SetUp() override
            {
                // Dependent before provider: deactivating the provider unsets STREAMING,
                // which would otherwise async-deactivate the dependent mid-teardown.
                EnsureDeactivated("Brave");
                EnsureDeactivated("DepStreaming");
                EnsureDeactivated("Streamer");
                EnsureDeactivated("Reentrant");
            }
        };

        ThunderTestRuntime StateMachineBaseline::_runtime;

        TEST_F(StateMachineBaseline, BravePluginLoadsDeactivated)
        {
            auto shell = _runtime.GetShell("Brave");

            ASSERT_TRUE(shell.IsValid());
            EXPECT_EQ(shell->State(), PluginHost::IShell::DEACTIVATED);
        }

        TEST_F(StateMachineBaseline, ActivateThenDeactivateRoundTrip)
        {
            auto shell = _runtime.GetShell("Brave");

            ASSERT_TRUE(shell.IsValid());

            EXPECT_EQ(shell->Activate(PluginHost::IShell::reason::REQUESTED), Core::ERROR_NONE);
            EXPECT_EQ(shell->State(), PluginHost::IShell::ACTIVATED);

            EXPECT_EQ(shell->Deactivate(PluginHost::IShell::reason::REQUESTED), Core::ERROR_NONE);
            EXPECT_EQ(shell->State(), PluginHost::IShell::DEACTIVATED);
        }

        TEST_F(StateMachineBaseline, IllegalTriggersFromDeactivated)
        {
            auto shell = _runtime.GetShell("Brave");
            ASSERT_TRUE(shell.IsValid());

            EXPECT_EQ(shell->Deactivate(PluginHost::IShell::reason::REQUESTED), Core::ERROR_ILLEGAL_STATE);

#ifdef HIBERNATE_SUPPORT_ENABLED
            EXPECT_EQ(shell->Hibernate(0), Core::ERROR_ILLEGAL_STATE);
#else
            EXPECT_EQ(shell->Hibernate(0), Core::ERROR_NOT_SUPPORTED);
#endif

            EXPECT_EQ(shell->State(), PluginHost::IShell::DEACTIVATED);
        }

        TEST_F(StateMachineBaseline, IllegalTriggersFromActivated)
        {
            auto shell = _runtime.GetShell("Brave");
            ASSERT_TRUE(shell.IsValid());
            ASSERT_EQ(shell->Activate(PluginHost::IShell::reason::REQUESTED), Core::ERROR_NONE);

            EXPECT_EQ(shell->Activate(PluginHost::IShell::reason::REQUESTED), Core::ERROR_ILLEGAL_STATE);
            EXPECT_EQ(shell->Unavailable(PluginHost::IShell::reason::REQUESTED), Core::ERROR_ILLEGAL_STATE);
            EXPECT_EQ(shell->State(), PluginHost::IShell::ACTIVATED);
        }

        TEST_F(StateMachineBaseline, ConditionsDeactivationEndsInPrecondition)
        {
            auto shell = _runtime.GetShell("Brave");
            ASSERT_TRUE(shell.IsValid());
            ASSERT_EQ(shell->Activate(PluginHost::IShell::reason::REQUESTED), Core::ERROR_NONE);

            EXPECT_EQ(shell->Deactivate(PluginHost::IShell::reason::CONDITIONS), Core::ERROR_NONE);
            EXPECT_EQ(shell->State(), PluginHost::IShell::PRECONDITION);
        }

        TEST_F(StateMachineBaseline, UnavailableFromDeactivated)
        {
            auto shell = _runtime.GetShell("Brave");
            ASSERT_TRUE(shell.IsValid());

            EXPECT_EQ(shell->Unavailable(PluginHost::IShell::reason::REQUESTED), Core::ERROR_NONE);
            EXPECT_EQ(shell->State(), PluginHost::IShell::UNAVAILABLE);
        }

        TEST_F(StateMachineBaseline, IllegalActivateFromUnavailable)
        {
            auto shell = _runtime.GetShell("Brave");
            ASSERT_TRUE(shell.IsValid());
            ASSERT_EQ(shell->Unavailable(PluginHost::IShell::reason::REQUESTED), Core::ERROR_NONE);
            ASSERT_EQ(shell->State(), PluginHost::IShell::UNAVAILABLE);

            EXPECT_EQ(shell->Activate(PluginHost::IShell::reason::REQUESTED), Core::ERROR_ILLEGAL_STATE);
            EXPECT_EQ(shell->State(), PluginHost::IShell::UNAVAILABLE);

            EXPECT_EQ(shell->Deactivate(PluginHost::IShell::reason::REQUESTED), Core::ERROR_NONE);
            EXPECT_EQ(shell->State(), PluginHost::IShell::DEACTIVATED);
        }

        TEST_F(StateMachineBaseline, InstantiationFailureStaysDeactivated)
        {
            auto shell = _runtime.GetShell("Broken");
            ASSERT_TRUE(shell.IsValid());
            EXPECT_EQ(shell->State(), PluginHost::IShell::DEACTIVATED);

            EXPECT_EQ(shell->Activate(PluginHost::IShell::reason::REQUESTED), Core::ERROR_UNAVAILABLE);
            EXPECT_EQ(shell->State(), PluginHost::IShell::DEACTIVATED);
        }

        TEST_F(StateMachineBaseline, InitializationFailureEndsDeactivated)
        {
            auto shell = _runtime.GetShell("FailInit");
            ASSERT_TRUE(shell.IsValid());
            EXPECT_EQ(shell->State(), PluginHost::IShell::DEACTIVATED);

            EXPECT_EQ(shell->Activate(PluginHost::IShell::reason::REQUESTED), Core::ERROR_GENERAL);
            EXPECT_EQ(shell->State(), PluginHost::IShell::DEACTIVATED);
            EXPECT_EQ(shell->Reason(), PluginHost::IShell::reason::INITIALIZATION_FAILED);
        }

        TEST_F(StateMachineBaseline, ActivatePendingWhenSubsystemDown)
        {
            auto dep = _runtime.GetShell("DepStreaming");
            ASSERT_TRUE(dep.IsValid());

            // STREAMING is unset at baseline: the provider controls it and is deactivated.
            EXPECT_EQ(dep->Activate(PluginHost::IShell::reason::REQUESTED), Core::ERROR_PENDING_CONDITIONS);
            EXPECT_EQ(dep->State(), PluginHost::IShell::PRECONDITION);

            EXPECT_EQ(dep->Deactivate(PluginHost::IShell::reason::REQUESTED), Core::ERROR_NONE);
            EXPECT_EQ(dep->State(), PluginHost::IShell::DEACTIVATED);
        }

        TEST_F(StateMachineBaseline, DependentAutoDeactivatesWhenSubsystemDisappears)
        {
            auto provider = _runtime.GetShell("Streamer");
            auto dep = _runtime.GetShell("DepStreaming");
            ASSERT_TRUE(provider.IsValid());
            ASSERT_TRUE(dep.IsValid());

            // Provider up -> sets STREAMING.
            ASSERT_EQ(provider->Activate(PluginHost::IShell::reason::REQUESTED), Core::ERROR_NONE);

            // Dependent activates (precondition met).
            ASSERT_EQ(dep->Activate(PluginHost::IShell::reason::REQUESTED), Core::ERROR_NONE);
            ASSERT_EQ(dep->State(), PluginHost::IShell::ACTIVATED);

            // Provider down -> framework unsets STREAMING -> termination -> auto-deactivate.
            ASSERT_EQ(provider->Deactivate(PluginHost::IShell::reason::REQUESTED), Core::ERROR_NONE);

            EXPECT_TRUE(WaitForState(dep, PluginHost::IShell::PRECONDITION));
            EXPECT_EQ(dep->State(), PluginHost::IShell::PRECONDITION);
        }

        TEST_F(StateMachineBaseline, DependentAutoActivatesWhenSubsystemAppears)
        {
            auto provider = _runtime.GetShell("Streamer");
            auto dep = _runtime.GetShell("DepStreaming");
            ASSERT_TRUE(provider.IsValid());
            ASSERT_TRUE(dep.IsValid());

            // STREAMING down at baseline -> dependent is pending.
            ASSERT_EQ(dep->Activate(PluginHost::IShell::reason::REQUESTED), Core::ERROR_PENDING_CONDITIONS);
            ASSERT_EQ(dep->State(), PluginHost::IShell::PRECONDITION);

            // Provider up -> sets STREAMING -> framework should auto-activate the dependent.
            ASSERT_EQ(provider->Activate(PluginHost::IShell::reason::REQUESTED), Core::ERROR_NONE);

            EXPECT_TRUE(WaitForState(dep, PluginHost::IShell::ACTIVATED));
            EXPECT_EQ(dep->State(), PluginHost::IShell::ACTIVATED);
        }

        TEST_F(StateMachineBaseline, ReentrantTriggerFromInitializeIsRejected)
        {
            auto shell = _runtime.GetShell("Reentrant");
            ASSERT_TRUE(shell.IsValid());

            const std::vector<TestSupport::ReentrantTrigger> triggers = {
                TestSupport::ReentrantTrigger::Deactivate, // headline
                TestSupport::ReentrantTrigger::Activate,
#ifdef HIBERNATE_SUPPORT_ENABLED
                TestSupport::ReentrantTrigger::Hibernate,
#endif
                TestSupport::ReentrantTrigger::Unavailable,
            };

            for (const TestSupport::ReentrantTrigger trigger : triggers) {
                TestSupport::g_reentrantTrigger = trigger;
                TestSupport::g_reentrantResult = Core::ERROR_NONE;

                // Activation completes; the re-entrant trigger is rejected by the guard.
                EXPECT_EQ(shell->Activate(PluginHost::IShell::reason::REQUESTED), Core::ERROR_NONE);
                EXPECT_EQ(shell->State(), PluginHost::IShell::ACTIVATED);
                EXPECT_EQ(TestSupport::g_reentrantResult, Core::ERROR_ILLEGAL_STATE);

                EXPECT_EQ(shell->Deactivate(PluginHost::IShell::reason::REQUESTED), Core::ERROR_NONE);
                EXPECT_EQ(shell->State(), PluginHost::IShell::DEACTIVATED);
            }
        }

        TEST_F(StateMachineBaseline, DeactivatedNotificationFiresWithHandlerResolvable)
        {
            auto shell = _runtime.GetShell("Brave");
            ASSERT_TRUE(shell.IsValid());
            ASSERT_EQ(shell->Activate(PluginHost::IShell::reason::REQUESTED), Core::ERROR_NONE);

            TestSupport::g_observedCallsign = "Brave";
            TestSupport::g_deactivatedFired = false;
            TestSupport::g_shellQiWorked = false;
            TestSupport::g_handlerQiResolved = false;

            ASSERT_EQ(shell->Deactivate(PluginHost::IShell::reason::REQUESTED), Core::ERROR_NONE);

            EXPECT_TRUE(TestSupport::g_deactivatedFired);
            EXPECT_TRUE(TestSupport::g_shellQiWorked); // IShell stays resolvable
            EXPECT_TRUE(TestSupport::g_handlerQiResolved); // handler QI forwards during DEACTIVATION
        }

        namespace {
            Core::ProxyType<PluginHost::IShell> ToPrecondition(ThunderTestRuntime& runtime)
            {
                auto dep = runtime.GetShell("DepStreaming");
                EXPECT_TRUE(dep.IsValid());
                EXPECT_EQ(dep->Activate(PluginHost::IShell::reason::REQUESTED), Core::ERROR_PENDING_CONDITIONS);
                EXPECT_EQ(dep->State(), PluginHost::IShell::PRECONDITION);
                return dep;
            }
        }

        // ACTIVATED -> Hibernate on an in-process plugin: _connection is null, so the
        // guard returns ERROR_INPROC before any state change. One of the few Hibernate
        // branches reachable without an OOP plugin.
        TEST_F(StateMachineBaseline, HibernateInProcessReturnsInProc)
        {
            auto shell = _runtime.GetShell("Brave");
            ASSERT_TRUE(shell.IsValid());
            ASSERT_EQ(shell->Activate(PluginHost::IShell::reason::REQUESTED), Core::ERROR_NONE);

#ifdef HIBERNATE_SUPPORT_ENABLED
            EXPECT_EQ(shell->Hibernate(0), Core::ERROR_INPROC);
#else
            EXPECT_EQ(shell->Hibernate(0), Core::ERROR_NOT_SUPPORTED);
#endif
            EXPECT_EQ(shell->State(), PluginHost::IShell::ACTIVATED);
        }

        // PRECONDITION rejects Activate / Hibernate / Unavailable (inherited base
        // ILLEGAL_STATE). Only Deactivate and Reevaluate are legal in this state.
        TEST_F(StateMachineBaseline, IllegalTriggersFromPrecondition)
        {
            auto dep = ToPrecondition(_runtime);

            EXPECT_EQ(dep->Activate(PluginHost::IShell::reason::REQUESTED), Core::ERROR_ILLEGAL_STATE);
#ifdef HIBERNATE_SUPPORT_ENABLED
            EXPECT_EQ(dep->Hibernate(0), Core::ERROR_ILLEGAL_STATE);
#else
            EXPECT_EQ(dep->Hibernate(0), Core::ERROR_NOT_SUPPORTED);
#endif
            EXPECT_EQ(dep->Unavailable(PluginHost::IShell::reason::REQUESTED), Core::ERROR_ILLEGAL_STATE);
            EXPECT_EQ(dep->State(), PluginHost::IShell::PRECONDITION);

            EXPECT_EQ(dep->Deactivate(PluginHost::IShell::reason::REQUESTED), Core::ERROR_NONE);
            EXPECT_EQ(dep->State(), PluginHost::IShell::DEACTIVATED);
        }

        // Deactivate(CONDITIONS) from PRECONDITION unloads the plugin but stays in
        // PRECONDITION (the CONDITIONS branch of PreconditionState::Deactivate). The
        // non-CONDITIONS branch is already covered by ActivatePendingWhenSubsystemDown.
        TEST_F(StateMachineBaseline, ConditionsDeactivationFromPreconditionStaysPrecondition)
        {
            auto dep = ToPrecondition(_runtime);

            EXPECT_EQ(dep->Deactivate(PluginHost::IShell::reason::CONDITIONS), Core::ERROR_NONE);
            EXPECT_EQ(dep->State(), PluginHost::IShell::PRECONDITION);

            EXPECT_EQ(dep->Deactivate(PluginHost::IShell::reason::REQUESTED), Core::ERROR_NONE);
            EXPECT_EQ(dep->State(), PluginHost::IShell::DEACTIVATED);
        }

        // UNAVAILABLE rejects Hibernate / Unavailable (inherited ILLEGAL_STATE); only
        // Deactivate is legal. Complements IllegalActivateFromUnavailable, which covers
        // the Activate rejection.
        TEST_F(StateMachineBaseline, IllegalTriggersFromUnavailable)
        {
            auto shell = _runtime.GetShell("Brave");
            ASSERT_TRUE(shell.IsValid());
            ASSERT_EQ(shell->Unavailable(PluginHost::IShell::reason::REQUESTED), Core::ERROR_NONE);
            ASSERT_EQ(shell->State(), PluginHost::IShell::UNAVAILABLE);

#ifdef HIBERNATE_SUPPORT_ENABLED
            EXPECT_EQ(shell->Hibernate(0), Core::ERROR_ILLEGAL_STATE);
#else
            EXPECT_EQ(shell->Hibernate(0), Core::ERROR_NOT_SUPPORTED);
#endif

            EXPECT_EQ(shell->Unavailable(PluginHost::IShell::reason::REQUESTED), Core::ERROR_ILLEGAL_STATE);
            EXPECT_EQ(shell->State(), PluginHost::IShell::UNAVAILABLE);

            EXPECT_EQ(shell->Deactivate(PluginHost::IShell::reason::REQUESTED), Core::ERROR_NONE);
            EXPECT_EQ(shell->State(), PluginHost::IShell::DEACTIVATED);
        }

        // QueryInterface dispatch per state: DEACTIVATED and PRECONDITION return null
        // for the handler interface; ACTIVATED forwards to the live handler. Pins the
        // per-state QI contract so a later refactor cannot silently change it.
        TEST_F(StateMachineBaseline, QueryInterfaceNullWhenDeactivated)
        {
            auto* plugin = _runtime.QueryInterfaceByCallsign<PluginHost::IPlugin>("Brave");
            EXPECT_EQ(plugin, nullptr);
            if (plugin != nullptr) {
                plugin->Release();
            }
        }

        TEST_F(StateMachineBaseline, QueryInterfaceValidWhenActivated)
        {
            auto shell = _runtime.GetShell("Brave");
            ASSERT_TRUE(shell.IsValid());
            ASSERT_EQ(shell->Activate(PluginHost::IShell::reason::REQUESTED), Core::ERROR_NONE);

            auto* plugin = _runtime.QueryInterfaceByCallsign<PluginHost::IPlugin>("Brave");
            EXPECT_NE(plugin, nullptr);
            if (plugin != nullptr) {
                plugin->Release();
            }
        }

        TEST_F(StateMachineBaseline, QueryInterfaceNullWhenPrecondition)
        {
            ToPrecondition(_runtime);

            auto* plugin = _runtime.QueryInterfaceByCallsign<PluginHost::IPlugin>("DepStreaming");
            EXPECT_EQ(plugin, nullptr);
            if (plugin != nullptr) {
                plugin->Release();
            }
        }

        // On Activate the Service acquires IStateControl and registers its Composit
        // sink. Pins that the live wiring is established.
        TEST_F(StateMachineBaseline, StateControlSinkRegisteredOnActivate)
        {
            TestSupport::ResetStateControlObservation();

            auto shell = _runtime.GetShell("StateCtrl");
            ASSERT_TRUE(shell.IsValid());
            ASSERT_EQ(shell->Activate(PluginHost::IShell::reason::REQUESTED), Core::ERROR_NONE);

            EXPECT_EQ(TestSupport::g_stateControlSinkCount, 1);

            EXPECT_EQ(shell->Deactivate(PluginHost::IShell::reason::REQUESTED), Core::ERROR_NONE);
        }

        // A state change requested directly on the live interface propagates to the
        // registered framework sink (Composit::StateChange -> StateControlStateChange).
        // Pins the outbound notification path that must survive any cleanup.
        TEST_F(StateMachineBaseline, StateControlChangePropagatesToFrameworkSink)
        {
            TestSupport::ResetStateControlObservation();

            auto shell = _runtime.GetShell("StateCtrl");
            ASSERT_TRUE(shell.IsValid());
            ASSERT_EQ(shell->Activate(PluginHost::IShell::reason::REQUESTED), Core::ERROR_NONE);

            auto* control = _runtime.QueryInterfaceByCallsign<PluginHost::IStateControl>("StateCtrl");
            ASSERT_NE(control, nullptr);

            const int before = TestSupport::g_stateControlNotifyCount;
            EXPECT_EQ(control->Request(PluginHost::IStateControl::SUSPEND), Core::ERROR_NONE);
            EXPECT_EQ(control->State(), PluginHost::IStateControl::SUSPENDED);
            EXPECT_GT(TestSupport::g_stateControlNotifyCount, before); // framework sink was notified

            control->Release();
            EXPECT_EQ(shell->Deactivate(PluginHost::IShell::reason::REQUESTED), Core::ERROR_NONE);
        }

        // On Deactivate the sink is unregistered (Detach) before teardown. Pins the
        // lifetime ordering: no sink is left dangling over DeinitializePlugin/UnloadPlugin.
        TEST_F(StateMachineBaseline, StateControlSinkUnregisteredOnDeactivate)
        {
            TestSupport::ResetStateControlObservation();

            auto shell = _runtime.GetShell("StateCtrl");
            ASSERT_TRUE(shell.IsValid());
            ASSERT_EQ(shell->Activate(PluginHost::IShell::reason::REQUESTED), Core::ERROR_NONE);
            ASSERT_EQ(TestSupport::g_stateControlSinkCount, 1);

            ASSERT_EQ(shell->Deactivate(PluginHost::IShell::reason::REQUESTED), Core::ERROR_NONE);
            EXPECT_EQ(TestSupport::g_stateControlSinkCount, 0);
        }

        // The money test for the removal question: over a full activate/deactivate
        // cycle the framework must never drive a SUSPEND into the plugin. If this holds,
        // RequestSuspend has no behavioural role and the helpers are safe to delete.
        TEST_F(StateMachineBaseline, FrameworkNeverDrivesSuspend)
        {
            TestSupport::ResetStateControlObservation();

            auto shell = _runtime.GetShell("StateCtrl");
            ASSERT_TRUE(shell.IsValid());
            ASSERT_EQ(shell->Activate(PluginHost::IShell::reason::REQUESTED), Core::ERROR_NONE);

            // string response;

            // const std::string params = "{\"callsign\": \"StateCtrl\"}";

            // ASSERT_EQ(_runtime.Invoke("Controller.resume", params, response), Core::ERROR_NONE);
            // ASSERT_EQ(_runtime.Invoke("Controller.suspend", params, response), Core::ERROR_NONE);

            ASSERT_EQ(shell->Deactivate(PluginHost::IShell::reason::REQUESTED), Core::ERROR_NONE);

            for (int command : TestSupport::g_stateControlRequests) {
                EXPECT_NE(command, static_cast<int>(PluginHost::IStateControl::SUSPEND))
                    << "framework issued a SUSPEND during the lifecycle";
            }
        }

        TEST_F(StateMachineBaseline, PluginStartSuspendedAndResume)
        {
            TestSupport::ResetStateControlObservation();

            auto shell = _runtime.GetShell("StateCtrlNoResume");
            ASSERT_TRUE(shell.IsValid());

            ASSERT_EQ(shell->Activate(PluginHost::IShell::reason::REQUESTED), Core::ERROR_NONE);

            EXPECT_EQ(TestSupport::g_stateControlSinkCount, 1);
            EXPECT_TRUE(TestSupport::g_stateControlRequests.empty());

            string response;
            const string params = _T("{\"callsign\":\"StateCtrlNoResume\"}");

            ASSERT_EQ(_runtime.Invoke(_T("Controller.resume"), params, response), Core::ERROR_NONE);
            ASSERT_EQ(TestSupport::g_stateControlRequests.size(), 1u);
            EXPECT_EQ(TestSupport::g_stateControlRequests.back(), static_cast<int>(PluginHost::IStateControl::RESUME));

            ASSERT_EQ(_runtime.Invoke(_T("Controller.suspend"), params, response), Core::ERROR_NONE);
            ASSERT_EQ(TestSupport::g_stateControlRequests.size(), 2u);
            EXPECT_EQ(TestSupport::g_stateControlRequests.back(), static_cast<int>(PluginHost::IStateControl::SUSPEND));

            ASSERT_EQ(shell->Deactivate(PluginHost::IShell::reason::REQUESTED), Core::ERROR_NONE);

            // Detach heeft de sink uitgeregistreerd vóór teardown.
            EXPECT_EQ(TestSupport::g_stateControlSinkCount, 0);
        }

        // 1) The Controller JSON-RPC lifecycle drives the same state transitions as the
        // direct IShell triggers. Pins the path real clients use against the direct path.
        TEST_F(StateMachineBaseline, ControllerLifecycleMatchesShell)
        {
            auto shell = _runtime.GetShell("Brave");
            ASSERT_TRUE(shell.IsValid());

            string response;
            const string params = _T("{\"callsign\":\"Brave\"}");

            ASSERT_EQ(_runtime.Invoke(_T("Controller.activate"), params, response), Core::ERROR_NONE);
            EXPECT_TRUE(WaitForState(shell, PluginHost::IShell::ACTIVATED));

            ASSERT_EQ(_runtime.Invoke(_T("Controller.deactivate"), params, response), Core::ERROR_NONE);
            EXPECT_TRUE(WaitForState(shell, PluginHost::IShell::DEACTIVATED));

            ASSERT_EQ(_runtime.Invoke(_T("Controller.unavailable"), params, response), Core::ERROR_NONE);
            EXPECT_TRUE(WaitForState(shell, PluginHost::IShell::UNAVAILABLE));

            // Recovery: deactivate is the only legal trigger out of UNAVAILABLE.
            ASSERT_EQ(_runtime.Invoke(_T("Controller.deactivate"), params, response), Core::ERROR_NONE);
            EXPECT_TRUE(WaitForState(shell, PluginHost::IShell::DEACTIVATED));
        }

        // 2) Illegal lifecycle operations surface their error over JSON-RPC instead of
        // being swallowed. Grounded, no assumptions:
        //  - Controller::Deactivate preserves ERROR_ILLEGAL_STATE and maps an unknown
        //    callsign to ERROR_UNKNOWN_KEY (Controller.cpp 1017-1041).
        //  - ThunderTestRuntime::Invoke returns dispatcher->Invoke's result, i.e. the
        //    method hresult; the passing ControllerLifecycleMatchesShell test confirms
        //    the method is actually invoked and ERROR_NONE round-trips.
        //
        // If a case ever comes back ERROR_NONE with the error in the response body
        // instead, move that assertion to parsing `response`; the EXPECT_EQ failure will
        // show the actual returned code.
        TEST_F(StateMachineBaseline, ControllerDeactivateSurfacesErrorCodes)
        {
            auto shell = _runtime.GetShell("Brave");
            ASSERT_TRUE(shell.IsValid());
            ASSERT_EQ(shell->State(), PluginHost::IShell::DEACTIVATED);

            string response;

            // Already DEACTIVATED -> ILLEGAL_STATE, preserved by the Controller.
            EXPECT_EQ(_runtime.Invoke(_T("Controller.deactivate"), _T("{\"callsign\":\"Brave\"}"), response),
                Core::ERROR_ILLEGAL_STATE);

            // Unknown callsign -> UNKNOWN_KEY.
            EXPECT_EQ(_runtime.Invoke(_T("Controller.deactivate"), _T("{\"callsign\":\"NoSuchPlugin\"}"), response),
                Core::ERROR_UNKNOWN_KEY);

            // Legal deactivate of an ACTIVATED plugin -> ERROR_NONE (contrast).
            ASSERT_EQ(shell->Activate(PluginHost::IShell::reason::REQUESTED), Core::ERROR_NONE);
            EXPECT_EQ(_runtime.Invoke(_T("Controller.deactivate"), _T("{\"callsign\":\"Brave\"}"), response),
                Core::ERROR_NONE);
            EXPECT_EQ(shell->State(), PluginHost::IShell::DEACTIVATED);
        }

        // 3) The deactivation reason round-trips into Reason(). Two distinct, confirmed
        // reasons prove Reason() reflects the trigger rather than a hardcoded value, and
        // that the CONDITIONS reason also routes to PRECONDITION rather than DEACTIVATED.
        TEST_F(StateMachineBaseline, DeactivationReasonIsRecorded)
        {
            auto shell = _runtime.GetShell("Brave");
            ASSERT_TRUE(shell.IsValid());

            ASSERT_EQ(shell->Activate(PluginHost::IShell::reason::REQUESTED), Core::ERROR_NONE);
            ASSERT_EQ(shell->Deactivate(PluginHost::IShell::reason::REQUESTED), Core::ERROR_NONE);
            EXPECT_EQ(shell->State(), PluginHost::IShell::DEACTIVATED);
            EXPECT_EQ(shell->Reason(), PluginHost::IShell::reason::REQUESTED);

            ASSERT_EQ(shell->Activate(PluginHost::IShell::reason::REQUESTED), Core::ERROR_NONE);
            ASSERT_EQ(shell->Deactivate(PluginHost::IShell::reason::CONDITIONS), Core::ERROR_NONE);
            EXPECT_EQ(shell->State(), PluginHost::IShell::PRECONDITION);
            EXPECT_EQ(shell->Reason(), PluginHost::IShell::reason::CONDITIONS);

            EXPECT_EQ(shell->Deactivate(PluginHost::IShell::reason::REQUESTED), Core::ERROR_NONE); // cleanup
        }

        TEST_F(StateMachineBaseline, SubsystemCascadeRoundTrip)
        {
            auto provider = _runtime.GetShell("Streamer");
            auto dep = _runtime.GetShell("DepStreaming");
            ASSERT_TRUE(provider.IsValid());
            ASSERT_TRUE(dep.IsValid());

            // Provider up -> STREAMING set -> dependent activates synchronously.
            ASSERT_EQ(provider->Activate(PluginHost::IShell::reason::REQUESTED), Core::ERROR_NONE);
            ASSERT_EQ(dep->Activate(PluginHost::IShell::reason::REQUESTED), Core::ERROR_NONE);
            ASSERT_EQ(dep->State(), PluginHost::IShell::ACTIVATED);

            // Provider down -> STREAMING unset -> termination -> async auto-deactivate.
            ASSERT_EQ(provider->Deactivate(PluginHost::IShell::reason::REQUESTED), Core::ERROR_NONE);
            EXPECT_TRUE(WaitForState(dep, PluginHost::IShell::PRECONDITION));
            EXPECT_EQ(dep->Reason(), PluginHost::IShell::reason::CONDITIONS);

            // Provider up again -> dependent auto-activates again, no manual Activate.
            ASSERT_EQ(provider->Activate(PluginHost::IShell::reason::REQUESTED), Core::ERROR_NONE);
            EXPECT_TRUE(WaitForState(dep, PluginHost::IShell::ACTIVATED));

            // Provider down again -> back to PRECONDITION. Repeatable, no leaked state.
            ASSERT_EQ(provider->Deactivate(PluginHost::IShell::reason::REQUESTED), Core::ERROR_NONE);
            EXPECT_TRUE(WaitForState(dep, PluginHost::IShell::PRECONDITION));

            EXPECT_EQ(dep->Deactivate(PluginHost::IShell::reason::REQUESTED), Core::ERROR_NONE); // cleanup
        }

        // Guards the precondition-freshness fix across repeated cycling. A single
        // round-trip proves the fix once; edge-triggered conditions are prone to drift
        // that only shows after several cycles, so this drives the controlling subsystem
        // up and down repeatedly and asserts the dependent tracks it every time.
        //
        // Pairs with SubsystemReactivatesAfterTerminationDeactivation (the single
        // round-trip). Both rely on the entry-point evaluating BOTH conditions on every
        // event (StateMachine::Reevaluate), so the dependent's _precondition stays fresh
        // while it is ACTIVATED and re-arms when the subsystem reappears.
        TEST_F(StateMachineBaseline, SubsystemCascadeSurvivesRepeatedCycling)
        {
            auto provider = _runtime.GetShell("Streamer");
            auto dep = _runtime.GetShell("DepStreaming");
            ASSERT_TRUE(provider.IsValid());
            ASSERT_TRUE(dep.IsValid());

            // Bring the dependent up the first time.
            ASSERT_EQ(provider->Activate(PluginHost::IShell::reason::REQUESTED), Core::ERROR_NONE);
            ASSERT_EQ(dep->Activate(PluginHost::IShell::reason::REQUESTED), Core::ERROR_NONE);
            ASSERT_EQ(dep->State(), PluginHost::IShell::ACTIVATED);

            for (int cycle = 0; cycle < 5; ++cycle) {
                ASSERT_EQ(provider->Deactivate(PluginHost::IShell::reason::REQUESTED), Core::ERROR_NONE);
                ASSERT_TRUE(WaitForState(dep, PluginHost::IShell::PRECONDITION)) << "down, cycle " << cycle;
                EXPECT_EQ(dep->Reason(), PluginHost::IShell::reason::CONDITIONS) << "cycle " << cycle;

                ASSERT_EQ(provider->Activate(PluginHost::IShell::reason::REQUESTED), Core::ERROR_NONE);
                ASSERT_TRUE(WaitForState(dep, PluginHost::IShell::ACTIVATED)) << "up, cycle " << cycle;
            }

            // cleanup
            ASSERT_EQ(provider->Deactivate(PluginHost::IShell::reason::REQUESTED), Core::ERROR_NONE);
            ASSERT_TRUE(WaitForState(dep, PluginHost::IShell::PRECONDITION));
            dep->Deactivate(PluginHost::IShell::reason::REQUESTED);
        }

        // 6) The Controller emits its lifecycle "statechange" event to a JSON-RPC
        // subscriber. Closes the outbound half: a state transition is not only applied
        // but announced. Event name verified against JLifeTime.h (registered as
        // "statechange", line 152/211); the payload carries the callsign, which is what
        // the handler checks, so no assumption is made about the exact JSON structure.
        //
        // Requires <atomic>, <thread> and <chrono> (add the includes if the harness does
        // not already pull them in).
        TEST_F(StateMachineBaseline, ControllerEmitsStateChangeEvent)
        {
            auto link = _runtime.CreateJSONRPCLink("Controller");
            ASSERT_TRUE(link.IsValid());

            std::atomic<int> eventCount{ 0 };
            std::atomic<bool> sawBrave{ false };

            ASSERT_EQ(link->Subscribe(_T("statechange"),
                          [&](const string& /* designator */, const string& /* index */, const string& params) {
                              eventCount++;
                              if (params.find(_T("Brave")) != string::npos) {
                                  sawBrave = true;
                              }
                          }),
                Core::ERROR_NONE);

            string response;
            ASSERT_EQ(_runtime.Invoke(_T("Controller.activate"), _T("{\"callsign\":\"Brave\"}"), response), Core::ERROR_NONE);

            // Delivered asynchronously; bounded poll rather than assuming a WaitFor helper.
            for (int i = 0; (i < 200) && (sawBrave.load() == false); ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(25));
            }

            EXPECT_TRUE(sawBrave.load())
                << "no 'statechange' event carrying 'Brave' arrived after " << eventCount.load() << " events";

            link->Unsubscribe(_T("statechange"));

            auto shell = _runtime.GetShell("Brave");
            if (shell.IsValid()) {
                shell->Deactivate(PluginHost::IShell::reason::REQUESTED);
            }
        }
    } // namespace Tests
} // namespace TestCore
} // namespace Thunder
