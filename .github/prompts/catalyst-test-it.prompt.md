---
description: Comprehensive analysis of test coverage gaps across functional and use-case dimensions, from function-level to user-scenario level.
---

# Test It

## Description

As part of this test coverage analysis, you will conduct a comprehensive, multi-dimensional audit of test coverage gaps across an application codebase. This goes far beyond simple line coverage metrics to identify functional and use-case gaps at every level—from missing function inputs and edge cases to untested user scenarios and integration points.

**IMPORTANT: This is a TEST GAP ANALYSIS task**. This analysis involves:

1. Deep research into the application structure, usage patterns, and existing tests
2. Identifying high-level systems and mapping their use cases
3. Cross-referencing use cases against existing test coverage
4. Documenting gaps at multiple levels (function-level to user-scenario level)
5. Prioritizing gaps by criticality (security, user-facing, error handling, integration)
6. Creating a comprehensive report suitable for planning or test generation tools

**This task does NOT involve**:

1. Writing actual test code or test implementations
2. Modifying existing tests or application code
3. Relying solely on line coverage metrics
4. Making assumptions about usage without research
5. Creating test specifications or test plans
6. Generating tests for identified gaps

## ⚠️ CRITICAL: ANALYSIS ONLY - NO TEST CREATION

**Your ONLY output is a test gap analysis report documenting what SHOULD be tested but ISN'T.**

This task is for **TEST COVERAGE GAP ANALYSIS ONLY**. Your role is to:

1. **RESEARCH**: Read and analyze code, tests, documentation, and workflows to understand the application
2. **IDENTIFY**: Map systems, use cases, and existing test coverage
3. **ANALYZE**: Cross-reference use cases against tests to find gaps at all levels
4. **DOCUMENT**: Create comprehensive report of gaps from function-level to user-scenario level
5. **PRIORITIZE**: Categorize gaps by criticality (security, user-facing, error handling, integration)

**You must NEVER**:

1. Write test code or test implementations
2. Modify existing tests or application code
3. Skip research phases and jump to conclusions
4. Rely solely on line coverage metrics
5. Focus on only one level of granularity (must include function, use-case, and scenario levels)
6. Make assumptions about application usage without evidence
7. Create test specifications or test plans
8. Generate tests for the identified gaps

**All output must be in the test gap analysis report file only.**

## ⚠️ Critical Technical Constraint

> **⚠️ CRITICAL TECHNICAL CONSTRAINT: You CANNOT create the entire report in one operation. Due to output context limitations, you MUST work phase-by-phase, building the report incrementally through multiple file updates. Attempting to write the complete report in one shot WILL FAIL. ⚠️**

This constraint applies specifically to **report and tracking document generation** — NOT to code changes:

1. **Build reports incrementally**: Write each section of a report or tracking document as a separate file operation — never buffer all content and write once at the end
2. **Respect phase boundaries**: Write the report section corresponding to each phase as that phase completes
3. **Never one-shot a report**: Do not attempt to generate the entire content of a report or tracking document in a single operation
4. **Code changes are unaffected**: This constraint does NOT apply to source code edits, which should follow normal practices

## Purpose & Audience

### Purpose

This analysis serves multiple critical purposes:

1. **Strategic Planning**: Provides a roadmap for improving test coverage beyond superficial metrics
2. **Risk Assessment**: Identifies untested critical paths, security vulnerabilities, and integration points
3. **Quality Improvement**: Reveals blind spots in current testing approach
4. **Resource Allocation**: Helps prioritize testing efforts based on risk and impact
5. **Tool Integration**: Structures findings for consumption by test generation tools like OpenSpec
6. **Confidence Building**: Creates foundation for robust test suite that gives teams confidence in their code

### Audience

This report is designed for:

- **Developers**: Engineers who need to understand what functionality lacks proper test coverage
- **QA Engineers**: Testing professionals planning test suite improvements
- **Engineering Managers**: Leaders making decisions about testing investment and priorities
- **DevOps Teams**: Teams responsible for CI/CD pipeline quality gates
- **Tool Consumers**: Systems like OpenSpec that generate tests from gap specifications

### Quality Standards

The analysis must be:

- **Comprehensive**: Covers all systems/components with no blind spots
- **Multi-Level**: Addresses gaps from granular (function inputs) to strategic (user scenarios)
- **Evidence-Based**: Every gap claim supported by research findings
- **Actionable**: Each gap specific enough to guide test creation
- **Prioritized**: Gaps categorized by criticality to guide resource allocation
- **Language-Agnostic**: Works for any programming language or testing framework

## Test Coverage Dimensions

Test coverage must be analyzed across multiple dimensions. Line coverage is only one metric and is often misleading. This analysis examines:

### 1. Functional Coverage

**What it is**: Whether each function/method is tested with appropriate inputs

**Gap Examples**:

- Function tested only with happy path, missing error conditions
- Method tested with one input type but not boundary values
- Function tested in isolation but not in context

### 2. Use Case Coverage

**What it is**: Whether each use case or workflow is tested end-to-end

**Gap Examples**:

- User registration tested but not password reset flow
- Data retrieval tested but not data modification workflow
- Success path tested but not cancellation or rollback

### 3. Error Path Coverage

**What it is**: Whether error conditions and exception paths are tested

**Gap Examples**:

- Network failure scenarios not tested
- Invalid input handling not covered
- Timeout conditions not verified

### 4. Edge Case Coverage

**What it is**: Whether boundary conditions and special cases are tested

**Gap Examples**:

- Empty list handling not tested
- Maximum value boundaries not verified
- Concurrent access scenarios not covered

### 5. Integration Coverage

**What it is**: Whether integration points between systems are tested

**Gap Examples**:

- API contract changes not caught by tests
- Database migration effects not tested
- Third-party service integration not verified

### 6. Security Coverage

**What it is**: Whether security-critical paths are tested

**Gap Examples**:

- Authentication bypass attempts not tested
- Authorization boundary violations not covered
- Input sanitization not verified

### 7. User Scenario Coverage

**What it is**: Whether real-world user workflows are tested

**Gap Examples**:

- Multi-step user journeys not tested end-to-end
- Mobile vs desktop usage patterns not covered
- Accessibility scenarios not verified

### Prioritization Strategy

Gaps should be prioritized by:

1. **Critical**: Security vulnerabilities, data loss scenarios, user-blocking issues
2. **High**: User-facing features, payment flows, core business logic
3. **Medium**: Error handling, edge cases, integration points
4. **Low**: Nice-to-have scenarios, rare edge cases, internal tooling

## Report Structure & Template

The final test gap analysis report must follow this exact structure:

```markdown
# Test Coverage Gap Analysis Report

**Project**: [Project Name]
**Analysis Date**: [Date]
**Analyzed By**: AI Agent
**Report Version**: 1.0

---

## Executive Summary

**Overview**: [2-3 sentence summary of findings]

**Total Gaps Identified**: [Number]

- Critical: [Number]
- High: [Number]
- Medium: [Number]
- Low: [Number]

**Key Findings**:

1. [Most significant gap finding]
2. [Second most significant finding]
3. [Third most significant finding]

---

## Application Overview

**High-Level Systems**:

1. **[System Name]**: [Brief description]
2. **[System Name]**: [Brief description]

**Technology Stack**: [Languages, frameworks, testing tools detected]

**Existing Test Coverage Summary**:

- Total test files: [Number]
- Test types: Unit ([Number]), Integration ([Number]), E2E ([Number])
- Coverage metrics: [If available, otherwise note "Not relying on coverage metrics"]

---

## Detailed Gap Analysis by System

### System: [System Name]

**Responsibility**: [What this system does]

**Key Components**:

- [Component 1]
- [Component 2]

#### Use Cases That Should Be Tested

1. **Use Case**: [Name]
   - **Description**: [What the use case involves]
   - **Current Coverage**: [What IS tested]
   - **Gaps Identified**:
     - ❌ **[Gap Type]**: [Specific gap description]
     - ❌ **[Gap Type]**: [Specific gap description]
   - **Priority**: Critical | High | Medium | Low
   - **Rationale**: [Why this priority]

2. **Use Case**: [Name]
   [... repeat structure ...]

[... repeat for each system ...]

---

## Gap Summary by Priority

### Critical Gaps

1. **[System Name] - [Use Case/Function]**
   - **Gap**: [Description]
   - **Risk**: [What could go wrong]
   - **Recommendation**: [What should be tested]

### High Priority Gaps

[... same structure ...]

### Medium Priority Gaps

[... same structure ...]

### Low Priority Gaps

[... same structure ...]

---

## Gap Categories

### Functional Coverage Gaps

[List function-level gaps: missing inputs, untested parameters, etc.]

### Use Case Coverage Gaps

[List workflow/use-case gaps: missing scenarios, untested paths]

### Error Path Coverage Gaps

[List error handling gaps: missing exception tests, error scenarios]

### Edge Case Coverage Gaps

[List boundary condition gaps: empty inputs, max values, concurrent access]

### Integration Coverage Gaps

[List integration gaps: API contracts, database interactions, third-party services]

### Security Coverage Gaps

[List security gaps: auth bypasses, authorization issues, input validation]

### User Scenario Coverage Gaps

[List user workflow gaps: multi-step journeys, real-world usage patterns]

---

## Recommendations

### Immediate Actions (Critical & High Priority)

1. [Specific recommendation]
2. [Specific recommendation]

### Short-Term Improvements (Medium Priority)

1. [Specific recommendation]
2. [Specific recommendation]

### Long-Term Enhancements (Low Priority)

1. [Specific recommendation]

### Testing Strategy Improvements

[Observations about overall testing approach and suggested improvements]

---

## Appendix

### Research Notes

[Key findings from research phases that provide context]

### Assumptions

[Any assumptions made during analysis]

### Limitations

[Any limitations of this analysis]
```

## Analysis Guidelines

### 1. Be Specific, Not Generic

**Good**: "❌ Missing test for `calculateDiscount()` with negative price input - should throw `ValueError`"

**Bad**: "❌ calculateDiscount needs more tests"

**Why**: Specific gaps are actionable. Generic statements don't guide test creation.

### 2. Identify Gaps at Multiple Levels

**Good**: Report includes function-level gap ("missing null input test for `parseUser()`"), use-case gap ("user profile update workflow not tested"), and scenario gap ("mobile user editing profile on slow network not covered")

**Bad**: Report only states "need more tests for user module"

**Why**: Comprehensive analysis requires examining granular details AND high-level scenarios.

### 3. Provide Evidence for Each Gap

**Good**: "❌ Authentication timeout not tested - searched all test files, found no tests with `auth_timeout` or similar timeout scenarios in authentication context"

**Bad**: "❌ Authentication might need timeout tests"

**Why**: Evidence-based gaps are credible. Speculation undermines the report.

### 4. Prioritize Based on Risk, Not Ease

**Good**: "Critical - Payment processing errors not tested - could result in financial loss or double-charging users"

**Bad**: "Low - Payment errors not tested because payments are complex"

**Why**: Priority should reflect business risk and user impact, not implementation difficulty.

### 5. Distinguish Between "Not Tested" and "Partially Tested"

**Good**: "**Current Coverage**: User login happy path tested in `test_auth.py:test_successful_login()`. **Gaps**: ❌ Invalid password handling not tested, ❌ Account lockout after failed attempts not tested, ❌ MFA flow not tested"

**Bad**: "User login not tested"

**Why**: Acknowledging existing coverage focuses attention on real gaps and prevents duplicate work.

### 6. Include Both Code-Level and User-Level Gaps

**Good**: Report includes "function `splitPayment()` not tested with amount less than minimum" AND "user attempting split payment with insufficient funds not tested as end-to-end scenario"

**Bad**: Report only includes low-level function gaps

**Why**: Tests should verify both technical correctness and user-facing behavior.

### 7. Connect Gaps to Real-World Consequences

**Good**: "❌ Data import rollback not tested - if import fails mid-process, partial data could corrupt database, requiring manual cleanup"

**Bad**: "❌ Data import rollback not tested"

**Why**: Explaining consequences helps stakeholders understand priority and justifies testing investment.

### 8. Use Consistent Gap Format

**Good**: All gaps follow format: "❌ **[Gap Type]**: [Specific scenario] - [Expected behavior or test approach]"

**Bad**: Mix of formats: "needs tests", "test this", "missing coverage for..."

**Why**: Consistent format makes report scannable and suitable for tool consumption.

### 9. Ground Analysis in Application's Actual Usage

**Good**: "Based on API documentation showing `/users/:id/deactivate` endpoint, deactivation workflow should be tested but no tests found in test suite"

**Bad**: "Users might want to deactivate accounts so we should test that"

**Why**: Gaps should come from application's actual capabilities, not speculation about possible features.

### 10. Identify Testing Anti-Patterns

**Good**: "NOTE: Multiple tests mock database without verifying actual queries would succeed - integration gap"

**Bad**: Ignore test quality issues

**Why**: Test coverage isn't just about quantity; quality gaps should be identified too.

## Output Requirements

> **⚠️ CRITICAL TECHNICAL CONSTRAINT: You CANNOT create the entire report in one operation. Due to output context limitations, you MUST work phase-by-phase, building the report incrementally through multiple file updates. Attempting to write the complete report in one shot WILL FAIL. ⚠️**

### File Location

- **Final Report**: `ai_reports/test-it-report.md` (created in Phase 7)
- **Working Document**: `./tmp/test-coverage-research.md` (created in Phase 0, used throughout analysis)
- **Create Directories**: Create `./tmp/` and `ai_reports/` directories if they don't exist

### File Format

- **Format**: Markdown
- **Line Length**: Natural line breaks (no artificial wrapping)
- **Heading Levels**: Proper hierarchy (H1 for title, H2 for major sections, H3 for systems, H4 for use cases)
- **Code Blocks**: Use triple backticks with language identifier when showing code examples
- **Lists**: Use `-` for unordered lists, `1.` for ordered lists
- **Emphasis**: Use `**bold**` for gap types and priorities, use ❌ for gaps and ✅ for coverage

### Content Requirements

The test gap analysis report must include:

1. **Executive Summary**: High-level findings with gap counts by priority
2. **Application Overview**: Systems identified and existing test inventory
3. **Detailed Gap Analysis by System**: For each system, enumerate use cases and identify gaps
4. **Gap Summary by Priority**: All gaps organized by Critical/High/Medium/Low
5. **Gap Categories**: All gaps organized by dimension (functional, use-case, error path, edge case, integration, security, user scenario)
6. **Recommendations**: Actionable next steps organized by priority
7. **Appendix**: Research notes, assumptions, and limitations

### Quality Standards

The report must be:

- **Comprehensive**: All systems/components analyzed, no blind spots
- **Multi-Level**: Includes function-level, use-case level, and user-scenario level gaps
- **Evidence-Based**: Every gap supported by research findings (not speculation)
- **Actionable**: Each gap specific enough to guide test creation
- **Prioritized**: Every gap assigned Critical/High/Medium/Low priority with justification
- **Structured**: Follows the exact template structure for consistency
- **Tool-Compatible**: Formatted for potential consumption by test generation tools (consistent gap format)
- **Complete**: No placeholder sections or "To be determined" entries

# ⚠️ MANDATORY TEST GAP ANALYSIS PROCESS ⚠️

**READ THIS SECTION COMPLETELY BEFORE STARTING ANY WORK**

This section contains the **MANDATORY step-by-step process** you MUST follow to conduct a comprehensive test coverage gap analysis. **DO NOT SKIP ANY STEPS**. **DO NOT JUMP AHEAD**. Each phase must be completed in order, with research tracked in the working document before proceeding to the next phase.

## Critical Process Rules

**YOU MUST:**

1. ✅ Complete **ALL 9 PHASES** in sequential order (0 through 8)
2. ✅ Create a research tracking document at `./tmp/test-coverage-research.md` BEFORE any analysis
3. ✅ Update the research tracking document **AFTER EACH PHASE** before proceeding
4. ✅ Analyze gaps at **MULTIPLE LEVELS**: function-level inputs, use-case workflows, AND user scenarios
5. ✅ Ground every gap in **EVIDENCE** from code, tests, or documentation (no speculation)
6. ✅ Prioritize gaps by **RISK AND IMPACT**, not by ease of testing
7. ✅ Research how the application is **ACTUALLY USED** (docs, APIs, workflows) before identifying gaps
8. ✅ Distinguish between "not tested" and "partially tested" with specific missing scenarios
9. ✅ Follow the exact **REPORT STRUCTURE TEMPLATE** provided in this prompt

**YOU MUST NOT:**

1. ❌ Write any test code or test implementations
2. ❌ Rely solely on line coverage metrics or coverage reports
3. ❌ Skip research phases and jump directly to gap identification
4. ❌ Focus on only one level of granularity (must include function, use-case, AND scenario levels)
5. ❌ Make assumptions about usage without evidence from code/docs/APIs
6. ❌ Leave gaps as generic statements like "needs more tests" (must be specific)
7. ❌ Create the report without first completing research in tracking document
8. ❌ Modify any application code or existing test files

**FAILURE TO FOLLOW THIS PROCESS WILL RESULT IN AN INCOMPLETE OR SUPERFICIAL ANALYSIS**

## PHASE 0: Create Research Tracking Document

**⚠️ MANDATORY FIRST STEP - DO THIS BEFORE ANY ANALYSIS ⚠️**

**Action**: Create `./tmp/test-coverage-research.md` with a structured template for tracking research findings

**Why This Phase Exists**: Creating a research tracking document prevents you from skipping research and jumping to conclusions. It forces systematic investigation of the codebase before identifying gaps.

**Process**:

1. **Create the directory**: `mkdir -p ./tmp`

2. **Create `./tmp/test-coverage-research.md`** with this exact structure:

```markdown
# Test Coverage Research Tracking Document

Last Updated: [Date]
Status: In Progress

---

## Section 1: Application Structure

**Status**: ❌ Not Started

### High-Level Systems

[To be filled in Phase 1]

### System Details

[To be filled in Phase 1]

---

## Section 2: Application Usage

**Status**: ❌ Not Started

### Usage Patterns Identified

[To be filled in Phase 2]

### Integration Points

[To be filled in Phase 2]

---

## Section 3: Existing Test Inventory

**Status**: ❌ Not Started

### Test Files Found

[To be filled in Phase 3]

### Coverage Analysis

[To be filled in Phase 3]

---

## Section 4: Use Cases Per System

**Status**: ❌ Not Started

[To be filled in Phase 4]

---

## Section 5: Identified Gaps

**Status**: ❌ Not Started

[To be filled in Phase 5]

---

## Section 6: Gap Prioritization

**Status**: ❌ Not Started

[To be filled in Phase 6]
```

3. **Verify the file was created** at `./tmp/test-coverage-research.md`

**REQUIRED OUTPUT**: Research tracking document created with all 6 sections

**Validation**: Before proceeding to Phase 1, verify:

- File `./tmp/test-coverage-research.md` exists
- All 6 section headers are present
- Each section has a status field

## PHASE 1: Understand Application Structure

**⚠️ MANDATORY: Update tracking document Section 1 BEFORE proceeding to Phase 2 ⚠️**

**What to Research**: Identify the high-level systems, components, and architectural boundaries of the application

**What to Extract**:

- System/component names and their responsibilities
- Key directories and their purposes
- Major classes, modules, or services
- System boundaries and dependencies
- Technology stack (languages, frameworks, libraries)

**Key Files/Locations to Examine**:

- Root directory structure
- `README.md`, `ARCHITECTURE.md`, or similar documentation
- Main source directories (e.g., `src/`, `lib/`, `app/`)
- Configuration files (`package.json`, `requirements.txt`, `pom.xml`, etc.)
- Directory organization and naming conventions

**Process**:

1. **List the workspace directory structure** to understand organization
2. **Read architecture documentation** if it exists (README, ARCHITECTURE, CONTRIBUTING files)
3. **Identify major directories** representing different systems or components
4. **For each system/component**:
   - Determine its responsibility (what does it do?)
   - Identify key files or classes
   - Note dependencies on other systems
5. **Compile technology stack** (programming languages, frameworks, testing tools)

**Update analysis document Section 1** with:

```markdown
## Section 1: Application Structure

**Status**: ✅ Completed
**Completed**: [Date]

### High-Level Systems

1. **[System Name]** (`path/to/system/`)
   - **Responsibility**: [What this system does]
   - **Key Files**: [Main files or entry points]

2. **[System Name]** (`path/to/system/`)
   - **Responsibility**: [What this system does]
   - **Key Files**: [Main files or entry points]

[... continue for each system ...]

### Technology Stack

- **Languages**: [e.g., Python 3.9, JavaScript ES6]
- **Frameworks**: [e.g., Flask, React, Spring Boot]
- **Testing Tools**: [e.g., pytest, Jest, JUnit]
- **Notable Libraries**: [Key dependencies]

### System Dependencies

[Describe how systems interact, if applicable]
```

**REQUIRED OUTPUT**: Section 1 of tracking document filled with at least 2-3 identified systems

**Validation**: Before proceeding to Phase 2, verify:

- At least 2-3 high-level systems identified
- Each system has clear responsibility description
- Technology stack documented
- Section 1 status changed to ✅ Completed

## PHASE 2: Research Application Usage

**⚠️ MANDATORY: Update tracking document Section 2 BEFORE proceeding to Phase 3 ⚠️**

**What to Research**: Understand how the application is actually used - user workflows, API contracts, integration points, and real-world scenarios

**What to Extract**:

- User-facing workflows and scenarios
- API endpoints and their purposes
- Integration points with external systems
- Data flows through the application
- Common usage patterns from documentation
- Error scenarios and edge cases mentioned in docs

**Key Files/Locations to Examine**:

- User documentation, API documentation, README files
- API route definitions (e.g., `routes.py`, `api/`, `controllers/`)
- Configuration files showing external integrations
- Workflow definitions (e.g., GitHub Actions, CI/CD configs)
- Example usage in documentation or example directories
- Comments in code describing intended usage

**Process**:

1. **Read user-facing documentation**:
   - README files
   - User guides or tutorials
   - API documentation
   - Installation and setup guides

2. **Analyze API definitions**:
   - List all endpoints/routes
   - Identify what each endpoint does
   - Note expected inputs and outputs

3. **Identify integration points**:
   - Database connections
   - External API calls
   - Message queues, event systems
   - File system interactions

4. **Map user workflows**:
   - Authentication/authorization flows
   - Core user journeys (e.g., registration, data upload, checkout)
   - Admin or privileged operations

5. **Note error scenarios mentioned**:
   - Invalid input handling
   - Failure recovery
   - Timeout or retry logic

**Update analysis document Section 2** with:

```markdown
## Section 2: Application Usage

**Status**: ✅ Completed
**Completed**: [Date]

### User Workflows Identified

1. **[Workflow Name]** (e.g., "User Registration")
   - **Description**: [What the workflow involves]
   - **Systems Involved**: [Which systems from Section 1]
   - **Key Operations**: [Main steps]

2. **[Workflow Name]**
   [... same structure ...]

### API Endpoints

| Endpoint         | Method | Purpose       | System          |
| ---------------- | ------ | ------------- | --------------- |
| `/api/users`     | POST   | Create user   | User Management |
| `/api/users/:id` | GET    | Retrieve user | User Management |

[... continue for major endpoints ...]

### Integration Points

1. **[Integration Name]** (e.g., "PostgreSQL Database")
   - **Type**: Database / External API / Message Queue / etc.
   - **Purpose**: [Why this integration exists]
   - **Systems Using It**: [Which systems interact with it]

2. **[Integration Name]**
   [... same structure ...]

### Data Flows

[Describe major data flows, e.g., "User submits form → API validates → Database stores → Email sent"]

### Error Scenarios Mentioned

- [Error scenario 1 from docs/code comments]
- [Error scenario 2]
```

**REQUIRED OUTPUT**: Section 2 of tracking document filled with user workflows, API endpoints, and integration points

**Validation**: Before proceeding to Phase 3, verify:

- At least 3-5 user workflows identified
- API endpoints listed (if applicable)
- Integration points documented
- Section 2 status changed to ✅ Completed

## PHASE 3: Analyze Existing Tests

**⚠️ MANDATORY: Update tracking document Section 3 BEFORE proceeding to Phase 4 ⚠️**

**What to Analyze**: Inventory all existing tests to understand what IS currently tested

**What to Extract**:

- Test file locations and naming conventions
- Test types (unit, integration, end-to-end)
- What systems/components have tests
- What scenarios are covered by existing tests
- Test quality indicators (assertions, mocks, test data)
- Coverage gaps visible in test organization

**Key Files/Locations to Examine**:

- Test directories (e.g., `tests/`, `test/`, `__tests__/`, `spec/`)
- Test files (e.g., `*_test.py`, `*.test.js`, `*Test.java`, `*.spec.ts`)
- Test configuration files (`pytest.ini`, `jest.config.js`, test runner configs)
- CI/CD pipeline definitions (what tests are run)
- Coverage reports if available (but don't rely solely on them)

**Process**:

1. **Find all test files** using file search patterns:
   - Search for test directories
   - Search for test file naming patterns
   - List all test files found

2. **Categorize tests by type**:
   - **Unit tests**: Test individual functions/classes in isolation
   - **Integration tests**: Test interactions between components
   - **End-to-end tests**: Test complete workflows

3. **For each test file, document**:
   - What system/component it tests
   - What scenarios are covered
   - Test quality observations (good assertions? adequate test data?)

4. **Identify patterns**:
   - Which systems have good test coverage?
   - Which systems lack tests entirely?
   - Are tests mostly happy-path or do they include error cases?
   - Are integration points tested?

5. **Note testing anti-patterns**:
   - Tests that mock everything (no real integration testing)
   - Tests with weak assertions (just checking no errors)
   - Tests with minimal test data variation

**Update analysis document Section 3** with:

```markdown
## Section 3: Existing Test Inventory

**Status**: ✅ Completed
**Completed**: [Date]

### Test Summary

- **Total Test Files**: [Number]
- **Unit Tests**: [Number] files
- **Integration Tests**: [Number] files
- **End-to-End Tests**: [Number] files

### Test Coverage by System

#### System: [System Name]

**Test Files**:

- `path/to/test_file.py`: Tests [specific functionality]
- `path/to/another_test.py`: Tests [specific functionality]

**Scenarios Covered**:

- ✅ [Scenario that IS tested]
- ✅ [Scenario that IS tested]

**Test Quality Observations**:

- [e.g., "Good assertion coverage", "Heavy mocking", "Only happy paths tested"]

#### System: [System Name]

[... repeat structure ...]

### Systems WITHOUT Tests

1. **[System Name]**: No test files found
2. **[System Name]**: No test files found

### Testing Anti-Patterns Observed

- [e.g., "Authentication tests mock database without integration tests"]
- [e.g., "Payment processing only tested in isolation, no end-to-end tests"]

### Coverage Metrics (if available)

[Include line coverage, branch coverage if available, but note: "These metrics alone are insufficient for gap analysis"]
```

**REQUIRED OUTPUT**: Section 3 of tracking document with comprehensive test inventory

**Validation**: Before proceeding to Phase 4, verify:

- All test files discovered and categorized
- Each system's test coverage documented
- Untested systems identified
- Section 3 status changed to ✅ Completed

## PHASE 4: Identify Use Cases Per System

**⚠️ MANDATORY: Update tracking document Section 4 BEFORE proceeding to Phase 5 ⚠️**

**What to Identify**: For each system, enumerate ALL use cases that SHOULD be tested, based on the system's responsibilities and actual usage

**What to Extract**:

- Happy path use cases
- Error path use cases
- Edge cases and boundary conditions
- Integration scenarios
- User-facing scenarios
- Security-critical scenarios

**Sources to Use**:

- Section 1 (system responsibilities)
- Section 2 (user workflows, API endpoints, integration points)
- Code analysis (what functions/methods exist)

**Process**:

1. **For each system identified in Section 1**:

2. **Review the system's code** to understand its functions/methods

3. **Enumerate use cases at multiple levels**:

   **Function-Level Use Cases** (granular):
   - For each public function/method:
     - Happy path with valid inputs
     - Invalid input handling
     - Boundary values (empty, null, max, min)
     - Error conditions that can be thrown

   **Workflow-Level Use Cases** (medium):
   - Multi-step operations within the system
   - State transitions
   - Data validation and processing flows

   **Integration-Level Use Cases** (broad):
   - Interaction with other systems
   - External API calls
   - Database operations
   - File system or network operations

   **User-Scenario-Level Use Cases** (highest level):
   - End-to-end user journeys involving this system
   - Real-world usage patterns from Section 2

4. **For each use case, document**:
   - Use case name
   - Description of what should happen
   - Expected outcome
   - What level it operates at (function/workflow/integration/scenario)

**Update analysis document Section 4** with:

```markdown
## Section 4: Use Cases Per System

**Status**: ✅ Completed
**Completed**: [Date]

### System: [System Name]

**Responsibility**: [From Section 1]

#### Function-Level Use Cases

1. **Use Case**: `functionName()` with valid input
   - **Expected**: Returns expected output format
   - **Level**: Function

2. **Use Case**: `functionName()` with null input
   - **Expected**: Throws appropriate error or handles gracefully
   - **Level**: Function

3. **Use Case**: `functionName()` with boundary value (e.g., empty list)
   - **Expected**: Returns empty result or default value
   - **Level**: Function

[... continue for major functions ...]

#### Workflow-Level Use Cases

1. **Use Case**: [Multi-step operation name]
   - **Description**: [What the workflow does]
   - **Expected**: [End state]
   - **Level**: Workflow

[... continue for workflows ...]

#### Integration-Level Use Cases

1. **Use Case**: [Integration scenario]
   - **Description**: [What integration involves]
   - **Expected**: [Successful integration behavior]
   - **Level**: Integration

[... continue for integrations ...]

#### User-Scenario-Level Use Cases

1. **Use Case**: [User journey name]
   - **Description**: [User's goal and steps]
   - **Expected**: [Successful outcome]
   - **Level**: User Scenario

[... continue for user scenarios ...]

---

### System: [Next System Name]

[... repeat entire structure ...]
```

**REQUIRED OUTPUT**: Section 4 filled with comprehensive use cases for each system at multiple levels

**Validation**: Before proceeding to Phase 5, verify:

- Every system from Section 1 has use cases documented
- Use cases cover multiple levels (function, workflow, integration, scenario)
- At least 5-10 use cases per system (depending on system complexity)
- Section 4 status changed to ✅ Completed

## PHASE 5: Identify Test Gaps

**⚠️ MANDATORY: Update tracking document Section 5 BEFORE proceeding to Phase 6 ⚠️**

**What to Identify**: Cross-reference the use cases from Section 4 against the existing tests from Section 3 to identify what SHOULD be tested but ISN'T

**What to Extract**:

- Use cases with NO test coverage
- Use cases with PARTIAL test coverage (identify what's missing)
- Missing error path tests
- Missing edge case tests
- Missing integration tests
- Missing user scenario tests

**Process**:

1. **For each system and its use cases from Section 4**:

2. **Check Section 3** to see if that use case is tested:
   - Is there a test file covering this system?
   - Does any test in that file cover this specific use case?
   - Is the coverage complete or partial?

3. **For each use case, determine coverage status**:
   - **✅ Fully Covered**: Test exists and adequately covers the scenario
   - **📑 Partially Covered**: Test exists but missing some aspects (document what's missing)
   - **❌ Not Covered**: No test found for this use case

4. **Document gaps with specificity**:
   - Don't just say "not tested"
   - Say specifically what's missing: "Error handling for null input not tested"
   - Explain what the test should verify

5. **Categorize gaps by dimension**:
   - Functional coverage gaps
   - Use case coverage gaps
   - Error path coverage gaps
   - Edge case coverage gaps
   - Integration coverage gaps
   - Security coverage gaps
   - User scenario coverage gaps

**Update analysis document Section 5** with:

```markdown
## Section 5: Identified Gaps

**Status**: ✅ Completed
**Completed**: [Date]

### System: [System Name]

#### Use Case: [Use Case Name from Section 4]

**Current Coverage**: ✅ Fully Covered | 📑 Partially Covered | ❌ Not Covered

**If Fully Covered**:

- ✅ Tested in: `path/to/test_file.py::test_function_name`

**If Partially Covered**:

- ✅ Happy path tested in: `path/to/test_file.py::test_happy_path`
- ❌ **Gap - Error Path**: Null input error handling not tested
- ❌ **Gap - Edge Case**: Empty list input not tested

**If Not Covered**:

- ❌ **Gap - Functional**: `functionName()` with valid input not tested at all

---

[... repeat for every use case in Section 4 ...]

---

### Gap Summary by Dimension

#### Functional Coverage Gaps

1. **[System Name] - `functionName()`**
   - ❌ Missing test for valid input
   - ❌ Missing test for return value format

[... continue ...]

#### Use Case Coverage Gaps

1. **[System Name] - [Workflow Name]**
   - ❌ Multi-step workflow not tested end-to-end

[... continue ...]

#### Error Path Coverage Gaps

1. **[System Name] - [Function/Workflow Name]**
   - ❌ Null input handling not tested
   - ❌ Network failure scenario not tested

[... continue ...]

#### Edge Case Coverage Gaps

1. **[System Name] - [Function Name]**
   - ❌ Empty list input not tested
   - ❌ Maximum value boundary not tested

[... continue ...]

#### Integration Coverage Gaps

1. **[System Name] - [Integration Point]**
   - ❌ Database interaction not tested (only mocked)
   - ❌ API contract changes not caught

[... continue ...]

#### Security Coverage Gaps

1. **[System Name] - [Security-Critical Path]**
   - ❌ Authorization bypass not tested
   - ❌ Input sanitization not verified

[... continue ...]

#### User Scenario Coverage Gaps

1. **[User Journey Name]**
   - ❌ End-to-end user flow not tested
   - ❌ Mobile user scenario not covered

[... continue ...]
```

**REQUIRED OUTPUT**: Section 5 filled with comprehensive gap analysis for every use case

**Validation**: Before proceeding to Phase 6, verify:

- Every use case from Section 4 analyzed for coverage
- Coverage status marked (✅/📑/❌) for each use case
- Gaps categorized by dimension
- Each gap is specific (not generic "needs tests")
- Section 5 status changed to ✅ Completed

## PHASE 6: Prioritize Gaps

**⚠️ MANDATORY: Update tracking document Section 6 BEFORE proceeding to Phase 7 ⚠️**

**What to Prioritize**: Assign priority levels (Critical/High/Medium/Low) to each identified gap based on risk, impact, and criticality

**Prioritization Criteria**:

**Critical Priority**:

- Security vulnerabilities (auth bypass, injection, privilege escalation)
- Data loss or corruption scenarios
- Payment or financial transaction errors
- User-blocking issues (can't complete core workflows)
- Compliance violations

**High Priority**:

- User-facing feature failures
- Core business logic errors
- Data integrity issues
- Integration failures with critical systems
- Error handling for common failure modes

**Medium Priority**:

- Edge cases in supported features
- Error handling for uncommon scenarios
- Integration with non-critical systems
- Performance degradation scenarios
- Non-blocking UX issues

**Low Priority**:

- Rare edge cases
- Internal tooling or admin features
- Nice-to-have scenarios
- Cosmetic or convenience features

**Process**:

1. **Review all gaps from Section 5**

2. **For each gap, assess**:
   - What could go wrong if this isn't tested?
   - How likely is this scenario in production?
   - What is the impact if it fails (data loss, security, user experience)?
   - Is this a user-facing feature or internal component?
   - Is this security-critical?

3. **Assign priority** based on risk and impact (not testing difficulty)

4. **Provide justification** for each priority assignment

5. **Group gaps by priority** for easy review

**Update analysis document Section 6** with:

```markdown
## Section 6: Gap Prioritization

**Status**: ✅ Completed
**Completed**: [Date]

### Critical Priority Gaps

1. **[System Name] - [Gap Description]**
   - **Gap**: [Specific gap from Section 5]
   - **Risk**: [What could go wrong]
   - **Impact**: [Consequence of failure]
   - **Justification**: [Why this is Critical priority]

2. **[System Name] - [Gap Description]**
   [... same structure ...]

### High Priority Gaps

1. **[System Name] - [Gap Description]**
   - **Gap**: [Specific gap from Section 5]
   - **Risk**: [What could go wrong]
   - **Impact**: [Consequence of failure]
   - **Justification**: [Why this is High priority]

[... continue ...]

### Medium Priority Gaps

1. **[System Name] - [Gap Description]**
   - **Gap**: [Specific gap from Section 5]
   - **Risk**: [What could go wrong]
   - **Impact**: [Consequence of failure]
   - **Justification**: [Why this is Medium priority]

[... continue ...]

### Low Priority Gaps

1. **[System Name] - [Gap Description]**
   - **Gap**: [Specific gap from Section 5]
   - **Risk**: [What could go wrong]
   - **Impact**: [Consequence of failure]
   - **Justification**: [Why this is Low priority]

[... continue ...]

### Priority Distribution

- **Critical**: [Number] gaps
- **High**: [Number] gaps
- **Medium**: [Number] gaps
- **Low**: [Number] gaps
- **TOTAL**: [Number] gaps
```

**REQUIRED OUTPUT**: Section 6 filled with all gaps assigned priority levels and justifications

**Validation**: Before proceeding to Phase 7, verify:

- Every gap from Section 5 has been assigned a priority
- Each priority has justification based on risk/impact
- Priority counts summarized
- Section 6 status changed to ✅ Completed

## PHASE 7: Create Final Report

> **⚠️ CRITICAL TECHNICAL CONSTRAINT: You CANNOT create the entire report in one operation. Due to output context limitations, you MUST work phase-by-phase, building the report incrementally through multiple file updates. Attempting to write the complete report in one shot WILL FAIL. ⚠️**

**⚠️ MANDATORY: Create the final test gap analysis report using ALL research from tracking document ⚠️**

**Action**: Synthesize all findings from the research tracking document into a comprehensive, well-structured test gap analysis report

**Process**:

1. **Create directory**: `mkdir -p ai_reports`
2. **Create the report file** at `ai_reports/test-it-report.md`

3. **Follow the exact Report Structure & Template** provided in this prompt

4. **Populate each section** using data from the tracking document:

   **Executive Summary**:
   - High-level overview of findings
   - Gap counts from Section 6 priority distribution
   - Top 3 most critical findings

   **Application Overview**:
   - Systems from Section 1
   - Technology stack from Section 1
   - Test summary from Section 3

   **Detailed Gap Analysis by System**:
   - For each system from Section 1:
     - List its responsibility
     - For each use case from Section 4:
       - Show current coverage from Section 5
       - List gaps from Section 5
       - Include priority from Section 6

   **Gap Summary by Priority**:
   - All gaps from Section 6, organized by priority
   - Critical gaps listed first with risk/impact

   **Gap Categories**:
   - Gaps from Section 5, reorganized by dimension
   - Functional, Use Case, Error Path, Edge Case, Integration, Security, User Scenario

   **Recommendations**:
   - Immediate actions for Critical/High gaps
   - Short-term improvements for Medium gaps
   - Long-term enhancements for Low gaps
   - Strategic observations about testing approach

   **Appendix**:
   - Key research findings
   - Assumptions made
   - Limitations of the analysis

5. **Ensure report quality**:
   - Use consistent formatting (follow template)
   - Use ❌ for gaps, ✅ for coverage
   - Be specific (no generic "needs tests" statements)
   - Include evidence from research
   - Make gaps actionable

6. **Make report tool-compatible**:
   - Use consistent gap format: "❌ **[Gap Type]**: [Specific scenario]"
   - Structure data in tables where appropriate
   - Use clear section headers for machine parsing

**REQUIRED OUTPUT**: Complete test gap analysis report at specified file path

**Validation**: Before proceeding to Phase 8, verify:

- Report file created at correct location
- All sections from template are present
- No placeholder text remains (all sections filled)
- Report includes data from all 6 sections of tracking document
- Gaps are specific and actionable
- Priority assignments are clear

## PHASE 8: Quality Review

**⚠️ MANDATORY: Validate the report before considering the analysis complete ⚠️**

**Action**: Perform comprehensive quality checks on the final report to ensure it meets all requirements

**Quality Checks**:

### 1. Completeness Check

- [ ] All systems from Section 1 are analyzed in the report
- [ ] Every system has use cases documented
- [ ] Every use case has coverage assessment
- [ ] All gaps from Section 5 appear in the report
- [ ] All priorities from Section 6 are included
- [ ] Report follows the exact template structure
- [ ] No placeholder sections remain

### 2. Multi-Level Gap Check

- [ ] Report includes **function-level** gaps (specific function inputs, edge cases)
- [ ] Report includes **use-case level** gaps (workflow and scenario gaps)
- [ ] Report includes **user-scenario level** gaps (end-to-end user journeys)
- [ ] Report includes **integration** gaps (cross-system scenarios)
- [ ] Gaps span all dimensions (functional, error path, edge case, security, etc.)

### 3. Specificity Check

- [ ] No generic statements like "needs more tests" or "improve coverage"
- [ ] Each gap identifies specific missing scenario
- [ ] Each gap explains what should be tested
- [ ] Function names, file paths, or workflows referenced where applicable
- [ ] Gaps are actionable (someone could write a test from the description)

### 4. Evidence Check

- [ ] Gap claims are based on research (not speculation)
- [ ] "Not tested" claims verified against Section 3 test inventory
- [ ] "Partially tested" claims specify what's missing
- [ ] Application usage claims reference documentation or code
- [ ] Priority justifications reference real risks

### 5. Prioritization Check

- [ ] Every gap has a priority (Critical/High/Medium/Low)
- [ ] Critical gaps involve security, data loss, or user-blocking issues
- [ ] Priority based on risk/impact, not testing difficulty
- [ ] Priority justifications are clear
- [ ] Executive summary shows gap distribution by priority

### 6. Structure Check

- [ ] Report follows the template structure exactly
- [ ] Headings use proper hierarchy (H1, H2, H3, H4)
- [ ] Consistent formatting (❌ for gaps, ✅ for coverage)
- [ ] Tables formatted properly if used
- [ ] Gap format consistent: "❌ **[Gap Type]**: [Specific scenario]"

### 7. Tool Compatibility Check

- [ ] Gaps use consistent format for potential machine parsing
- [ ] Structure is clear for tool consumption (e.g., OpenSpec)
- [ ] Each gap is a discrete, identifiable item
- [ ] Priority levels clearly marked

### 8. Actionability Check

- [ ] Recommendations section provides concrete next steps
- [ ] Each gap specific enough to guide test creation
- [ ] Report suitable for planning test improvements
- [ ] Report suitable for piping to test generation tools

**If any quality check fails**, fix the issue in the report before completing the analysis.

**Final Status Update**: Update analysis document with:

```markdown
---

## Analysis Complete

**Status**: ✅ Complete
**Completion Date**: [Date]
**Report Location**: `ai_reports/test-it-report.md`

**Quality Review**: All checks passed

**Summary**:

- Systems Analyzed: [Number]
- Use Cases Identified: [Number]
- Gaps Identified: [Number]
  - Critical: [Number]
  - High: [Number]
  - Medium: [Number]
  - Low: [Number]
```

**REQUIRED OUTPUT**: Quality-validated test gap analysis report

**Validation**: Analysis is complete when:

- All 8 quality check categories pass
- Report is comprehensive, specific, evidence-based, and actionable
- Research tracking document marked as complete

# MANDATORY COMPLETION CHECKLIST

**Before considering this test gap analysis complete, you MUST verify ALL items in this checklist:**

## Phase Completion

- [ ] **Phase 0**: Created research tracking document at `./tmp/test-coverage-research.md` with all 6 sections
- [ ] **Phase 1**: Documented application structure with at least 2-3 systems, responsibilities, and tech stack in Section 1
- [ ] **Phase 2**: Researched application usage including workflows, API endpoints, and integration points in Section 2
- [ ] **Phase 3**: Inventoried all existing tests by type and system in Section 3
- [ ] **Phase 4**: Identified use cases for each system at multiple levels (function/workflow/integration/scenario) in Section 4
- [ ] **Phase 5**: Cross-referenced use cases against tests to identify gaps, documented in Section 5
- [ ] **Phase 6**: Assigned priority to every gap with justification in Section 6
- [ ] **Phase 7**: Created final report at specified path using tracking document data
- [ ] **Phase 8**: Performed quality review and all checks passed

## Research Tracking Verification

- [ ] Research tracking document exists at `./tmp/test-coverage-research.md`
- [ ] All 6 sections of tracking document are filled (no placeholders)
- [ ] Each section status updated to ✅ Completed
- [ ] Tracking document contains evidence supporting gap claims
- [ ] Final status update added to tracking document

## Report Structure Verification

- [ ] Report file exists at `ai_reports/test-it-report.md`
- [ ] Report follows the exact template structure
- [ ] Executive Summary present with gap counts by priority
- [ ] Application Overview includes systems and test summary
- [ ] Detailed Gap Analysis by System section complete for all systems
- [ ] Gap Summary by Priority section includes all Critical/High/Medium/Low gaps
- [ ] Gap Categories section organizes gaps by dimension (7 dimensions covered)
- [ ] Recommendations section provides actionable next steps
- [ ] Appendix includes research notes, assumptions, and limitations
- [ ] No placeholder sections remain in report

## Gap Analysis Quality Verification

- [ ] **Multi-Level**: Gaps identified at function-level, use-case level, AND user-scenario level
- [ ] **Specificity**: No generic "needs more tests" statements - all gaps are specific
- [ ] **Evidence-Based**: Every gap supported by research findings (not speculation)
- [ ] **Completeness**: Every system analyzed, every use case assessed
- [ ] **Distinction**: Report distinguishes "not tested" from "partially tested" with specific missing scenarios
- [ ] **Prioritization**: Every gap has Critical/High/Medium/Low priority with justification
- [ ] **Actionability**: Each gap specific enough to guide test creation
- [ ] **Dimension Coverage**: Gaps span all 7 dimensions (functional, use-case, error path, edge case, integration, security, user scenario)
- [ ] **Risk-Based Priority**: Priorities based on risk/impact, not testing ease
- [ ] **Consistency**: All gaps use consistent format: "❌ **[Gap Type]**: [Specific scenario]"

## Integration & Usage Coverage Verification

- [ ] Integration points identified and tested/untested status documented
- [ ] User workflows from documentation analyzed for test coverage
- [ ] API endpoints identified and coverage assessed
- [ ] Error scenarios mentioned in docs/code checked for test coverage
- [ ] Security-critical paths identified and gaps noted if untested

## Process Verification

- [ ] Did NOT skip any phases (0 through 8 all completed in order)
- [ ] Did NOT write any test code or modify application/test files
- [ ] Did NOT rely solely on line coverage metrics
- [ ] Did NOT jump to gap identification without completing research phases
- [ ] Did NOT make assumptions about usage without evidence
- [ ] Updated research tracking document after EACH phase before proceeding
- [ ] Used evidence from code, tests, and documentation to support all claims
- [ ] Analyzed actual application usage patterns, not just speculated scenarios

**IF ANY CHECKBOX ABOVE IS UNCHECKED, THE ANALYSIS IS NOT COMPLETE.**

## Success Criteria

This test gap analysis is successful when:

1. **Comprehensiveness**: All systems and components analyzed with no blind spots
2. **Multi-Level Analysis**: Gaps identified at granular (function inputs), medium (workflows), and strategic (user scenarios) levels
3. **Evidence-Based**: Every gap claim supported by research findings, not speculation
4. **Actionability**: Each gap specific enough that a developer or test generation tool knows exactly what to test
5. **Prioritization**: All gaps categorized by risk and impact (Critical/High/Medium/Low) with clear justifications
6. **Coverage Dimensions**: Analysis spans all 7 coverage dimensions (functional, use-case, error path, edge case, integration, security, user scenario)
7. **Tool Compatibility**: Report structured for both human reading and potential machine consumption (e.g., OpenSpec)
8. **Research Depth**: Analysis grounded in actual application structure, usage patterns, and existing test inventory
9. **Quality**: Report follows template structure, uses consistent formatting, contains no placeholders
10. **Completeness**: Research tracking document and final report both complete with all sections filled
