# Agent Instructions

## Architecture-First development

**Before proposing, designing or implementing any feature** read the architecture brief at 'docs/thunder-architecture-brief.md'. If the file is missing create such an architecture brief.

### Core Principles

1. **Consult first** - Read the architecture brief before proposing or desiging anything. Use Subject Matter Expert (SME) notes if available.
2. **Respect boundaries** - Follow defined service responsibilities and API contracts.
3. **Stay consistent** - Match existing patterns, stack and style.
4. **Fixed methodology** - Use the OpenSpec development approach for proposing, designing and implementing features. Archive when completed. Populate OpenSpec's 'config.yaml' with the results of the consultancy step.

### When Features Require Architecture Changes

Highlight changes in proposal, wait for approval, then update the architecture brief as a first step.

### When Updates Require Subject Matter Export Notes Changes

If the architecture brief changes, code changes, or relevant input has been given, keep the SME notes in sync, but always request approval before editing.

### Architecture Brief Commands

Invoke skill '/architecture-brief'.

### OpenSpec Commands

1. Explore, brainstorm an idea: invoke skill '/osx-explore'
1. Formalize into a proposal: invoke skill '/osx-propose'
2. Implement the proposal: invoke skill '/osx-apply'
4. Archive if the implementation is approved, completed and final: invoke skill '/osx/archive'

### Project Architecture Briefs

-----------------------------------------------------------
| Project  | Architecture Brief                           |
-----------------------------------------------------------
| Thunder | [Thunder](docs/thunder-architecture-brief.md) |
-----------------------------------------------------------