# Presentation and backend selection

**Do not substitute planned product direction for capabilities in the consumer's selected build.** Identify available targets in `CMakeLists.txt`, installed `SASDUIToolkitTargets.cmake`, public headers and tests.

| Family | General intent | Verification gate |
| --- | --- | --- |
| Core | Semantic widgets, ownership, commands, focus, Model/View | Headless tests and `SASD::UI` |
| Terminal | ANSI/VT first-class host, terminal cell presentation and keyboard input | `SASD::UI::Terminal`, demo + suitable terminal |
| Rendered | SDL-independent `DisplayList` / measurement / drawing API | `SASD::UI::Rendered`, rendered tests |
| SDL3 | Experimental concrete rendered adapter (opt-in/build-tree-only in the reviewed 0.2 branch) | CMake option, SDL3 dependencies, smoke tests |
| Native | Evolving host/peer integrations; Win32 host preview starts in M6 | Platform-specific CMake target, current tests, actual backend feature support |

These are **conditional capability descriptions**, not promises that every family is usable or installed on every OS.

## Visible terminal application: the host matters

Terminal input/output involves several distinct stages:

1. Terminal device/session starts with appropriate resource lifetime and cleanup.
2. Backend transforms bytes/native console input into semantic events.
3. Focus, menus, shortcuts and pointer routing choose what receives each event.
4. Layout invokes measure/arrange against terminal metrics.
5. Presentation updates screen cells/frames and emits ANSI/VT output.
6. Host loop handles resize, exit and session restoration.

Do not claim that `Window` or `Application` has a universal `run()` API unless the actual selected headers do. Copy/adapt **the matching version's** `examples/terminal_form_demo.cpp` host rather than assembling a guessed event loop.

Terminal constraints: UTF-8 bytes, Unicode scalar positions and terminal cell widths differ. UI text can be untrusted; keep raw control sequences out of application-provided strings. Escape, Tab/Shift+Tab and core commands must remain usable without mouse support.

## Rendered and native decisions

- `Rendered` drawing primitives are not inherently SDL3 code. Do not inject SDL headers into Core.
- SDL3 is an explicitly enabled experiment in the reviewed snapshot and may require SDL_ttf and test font configuration. Confirm build-tree/install status on the checkout actually in use.
- A Win32 native-host slice is *not* proof of complete native widget parity; verify that the desired widget, interactions and packaging exist.
- Later GTK/AppKit backends are goals until their corresponding source, tests and packaging demonstrate otherwise.

For mixed platforms, prove the smallest end-to-end interaction on **each** intended backend; unit tests on Core are necessary but not sufficient.
