## ADDED Requirements

### Requirement: Output routes have explicit meanings
Messaging SHALL support the following route meanings without changing the established behavior of `HANDLER` or `DIRECT`: `HANDLER` is shared-buffer delivery, `DIRECT` is DirectOutput delivery, `EXTERNAL_DIRECT` is RDKLogger delivery, and `ALL` is all three destinations.

#### Scenario: HANDLER does not imply external output
- **WHEN** `HANDLER` is selected
- **THEN** the message is sent to the shared handler buffer and is not submitted to RDKLogger by that route

#### Scenario: DIRECT does not imply external output
- **WHEN** `DIRECT` is selected
- **THEN** the message is sent to DirectOutput and is not submitted to RDKLogger by that route

#### Scenario: EXTERNAL_DIRECT does not imply MessageControl
- **WHEN** `EXTERNAL_DIRECT` is selected
- **THEN** the message is submitted only to RDKLogger, subject to external enablement

### Requirement: Local and external interest are independent
The producer SHALL evaluate local Thunder enablement and cached RDKLogger enablement independently.

#### Scenario: Local disabled but external enabled
- **WHEN** local output is disabled and the external module/level is enabled
- **THEN** the message is constructed and submitted to RDKLogger

#### Scenario: External disabled but local enabled
- **WHEN** external output is disabled and local output is enabled
- **THEN** the message is delivered to the selected local destination without an RDKLogger submission

#### Scenario: Both disabled
- **WHEN** neither local nor external output is enabled
- **THEN** the producer avoids message construction and destination submission
