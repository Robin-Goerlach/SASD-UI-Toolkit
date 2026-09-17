# Development Guidelines

## Language standard

The initial target standard is **C++20**. New language features should be used when they improve readability, safety or maintainability; novelty by itself is not a reason to add complexity.

## Principles

- Clarity before clever code.
- Correctness before micro-optimization.
- Small, focused interfaces.
- RAII for resources and lifetimes.
- No hidden global ownership.
- Keep the public API free of backend-specific types wherever practical.
- Prefer the standard library before inventing general-purpose project utilities.
- Platform-specific code belongs in backend/platform modules.
- Documentation must clearly distinguish implemented functionality from planned functionality.

## Naming direction

Planned style:

```cpp
namespace sasd::ui {
    class Component;
    class Widget;
    class Button;
    class Window;
}
```

- Types: `PascalCase`
- Functions/methods: `camelCase`
- Local variables: `camelCase`
- Constants: use one consistent project convention once selected
- Avoid historical prefixes such as `TButton` unless there is a technical reason

## Headers and namespaces

Public headers should live under a stable include prefix:

```cpp
#include <sasd/ui/button.hpp>
#include <sasd/ui/window.hpp>
```

Internal backend headers are not part of the public API.

## Ownership and lifetime

- Ownership should be visible in the object/type model.
- Containers own their children by default.
- Non-owning pointers/references are allowed but must be clearly non-owning.
- `shared_ptr` should not become a universal default.
- Callback/event connections need a lifecycle model that prevents dangling callbacks.

## Error handling

Before implementation, each API family should deliberately choose whether errors are represented through:

- return/result types;
- exceptions;
- or non-failing preconditions.

An accidental mixture should be avoided. User/data errors must be distinguished from programmer errors.

## Threading

Widgets initially belong to one UI thread. Background threads must not perform arbitrary widget operations. An explicit dispatch/post API is planned for transferring work back to the UI event queue.

This keeps the core understandable and follows the broad model used by many established UI toolkits.

## Tests

At least four testing levels are planned:

1. **Unit tests** for core, events, layout and models.
2. **Contract tests** shared by all backends.
3. **Integration tests** for platform adapters.
4. **Example/smoke tests** that build small complete applications.

Tests should be deterministic where possible. Core tests must not require a graphical session.

## Build system

CMake is planned. Early CI should cover at least:

- GCC on Linux;
- Clang on Linux/macOS;
- MSVC on Windows.

Exact minimum compiler versions will be documented when the first build skeleton is introduced.

## Dependencies

Every new runtime dependency needs justification. At minimum evaluate:

- license;
- platform coverage;
- maintenance state;
- API/ABI risk;
- packaging availability;
- whether it leaks into public API.

Optional backend dependencies must not unnecessarily burden the core.

## Documentation

Public classes and functions should receive API documentation comments in code. Conceptual documentation remains under `docs/`.

For architecture-relevant changes:

1. explain the decision;
2. describe impact on backends;
3. update German and English documentation;
4. only then treat the direction as established architecture.

## API compatibility

Breaking changes are allowed before `1.0`. They still should not happen casually. Good naming, small interfaces and early examples should reduce needless churn.

## Performance

Early releases optimize primarily for clean architecture and correct behavior. Obvious structural performance traps should still be avoided, including:

- one UI object per table cell for very large data sets;
- unnecessary full copies of widget trees;
- repeated platform string conversions without reason;
- blocking work inside the UI event loop.

Measurements should justify later micro-optimizations.

## Security and robustness

UI code frequently handles external text, filenames, clipboard data and terminal escape sequences. Backends must treat such input robustly. In particular, the terminal backend must never emit untrusted text as control sequences without proper handling/escaping.
