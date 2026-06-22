// ===========================================================================
// IStateControl wiring characterization. Answers "can the framework-driven
// Suspend/Resume helpers go away" by pinning that the only live state-control
// path is the Composit notification wiring, and that no framework path drives
// SUSPEND into a plugin.
// ===========================================================================





// OPEN: auto-resume-on-activate pin. The Service issues Request(RESUME) on
// activate only when Resumed() is true. I could not confirm from the source how
// Resumed() is set via PluginConfig in your tree (a startmode::RESUMED value, or
// a separate Resumed/StartupOrder flag on Plugin::Config). Tell me which, and
// this becomes:
//
//   TEST_F(StateMachineBaseline, AutoResumeIssuedWhenConfiguredResumed) {
//       // configure a second callsign with Resumed == true, activate it, then:
//       ASSERT_EQ(g_stateControlRequests.size(), 1u);
//       EXPECT_EQ(g_stateControlRequests[0], (int)PluginHost::IStateControl::RESUME);
//   }
