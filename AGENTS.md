# AGENTS.md

## Purpose

This file is the operating contract for Codex and other coding agents working in **SASD UI Toolkit**.

The goal is not merely to make a requested change compile. The agent should preserve and extend the
project as a small, understandable, portable C++ UI toolkit with clean architectural boundaries,
deterministic tests, explicit lifetime rules and useful documentation.

When this file conflicts with an explicit instruction from the repository owner for the current task,
the explicit instruction wins. When implementation, tests, accepted ADRs and older prose disagree,
prefer the newest concrete implementation/tests and accepted architectural decisions, then repair stale
documentation as part of the relevant change.

---

## 1. Project mission

SASD UI Toolkit is a modern **C++20** UI library intended to let application code use one semantic API
across substantially different presentation environments.

Long-term presentation families are:

- **Terminal**: ANSI/VT/console environments are first-class, not a fallback.
- **Rendered desktop**: a backend-neutral drawing/measurement layer; SDL3 is the current experimental
  concrete adapter.
- **Native desktop**: Win32/GTK/AppKit peers are intentionally later, after the shared Core contract has
  gained practical stability.

The project is inspired by VCL, AWT/Swing, Qt, wxWidgets, FLTK, FTXUI and Turbo Vision, but it is **not**
an API clone of any of them.

Core product principles:

- semantic API first, presentation second;
- backend-neutral Core;
- explicit capabilities instead of pretending every platform is identical;
- RAII and explicit ownership;
- correctness and architecture before micro-optimization;
- small coherent vertical slices before a large widget catalogue;
- deterministic, headless-testable logic wherever possible;
- permissive MIT licensing;
- no mandatory macro-heavy or code-generator-heavy application model.

---

## 2. Source-of-truth hierarchy

Before changing code, establish what the repository currently says. Do not rely on remembered state,
old task descriptions or previous agent summaries.

Use this precedence:

1. current source code on the actual target branch;
2. current tests and CI configuration;
3. accepted ADRs in `docs/adr/`;
4. current English/German architecture and development documentation;
5. roadmap and README status text;
6. historical discussion or an old prompt.

Important consequences:

- Never assume an old commit SHA is still current.
- Never implement a planned roadmap item as though its API were already decided.
- If a prose document is stale but code/tests/ADRs clearly establish newer behavior, preserve the newer
  behavior and update the relevant prose when it is in scope.
- Do not silently "correct" a deliberate behavior just because another framework behaves differently.

Start documentation discovery at:

- `README.md`
- `CONTRIBUTING.md`
- `docs/README.md`
- `docs/en/ARCHITECTURE.md`
- `docs/en/BACKENDS.md`
- `docs/en/DEVELOPMENT_GUIDELINES.md`
- `docs/en/ROADMAP.md`
- `docs/adr/`

---

## 3. Autonomous operating mode

Codex is expected to carry ordinary, well-scoped repository work as far as it can without repeatedly
asking for confirmation.

For normal code, test and documentation tasks, Codex may autonomously:

- inspect repository state and history;
- create a dedicated working branch;
- edit source/tests/docs;
- run builds and tests;
- make incremental commits;
- push the branch;
- create/update a pull request when GitHub credentials/tools are available;
- react to concrete CI failures caused by its change;
- merge its own PR **only when the task/user authorization permits autonomous merging and all required
  checks are green**.

### Git and GitHub authorization

For ordinary repository development, Codex is explicitly authorized to perform the following Git and
GitHub operations without asking the repository owner for confirmation:

- inspect repository state, history, branches, remotes, commits and diffs;
- inspect GitHub repository, issue, pull-request and CI state;
- fetch and prune `origin`;
- fast-forward local `main` from `origin/main`;
- create and switch to a dedicated task branch;
- stage only files belonging to the current task;
- create normal incremental commits;
- push the current non-`main` task branch to `origin`;
- set the upstream of the current task branch when needed;
- create and update a pull request for the current task;
- inspect and wait for GitHub Actions / CI results;
- update the task branch to repair failures caused by the current change.

These operations are pre-authorized. Do not ask for confirmation merely because they modify `.git`,
contact GitHub, create commits, push a task branch, or create/update a pull request.

Before every push:

1. run `git status`;
2. inspect the staged/current diff;
3. run the relevant tests;
4. ensure the current branch is not `main`;
5. push only the current task branch.

Never autonomously:

- push directly to `main`;
- use `git push --force`, `git push -f` or `git push --force-with-lease`;
- use `git reset --hard` or destructive `git clean` variants;
- discard unrelated local changes;
- amend or rewrite already-pushed commits;
- rebase published history;
- create, move or delete tags;
- create or delete GitHub releases;
- change repository settings, secrets, variables or permissions;
- invoke arbitrary write operations through `gh api`;
- merge a pull request unless the task explicitly authorizes autonomous merging and all required checks
  are green.

Always stop and ask instead of guessing when:

- the requested behavior is genuinely ambiguous and alternatives change public semantics;
- a new mandatory runtime dependency is being considered;
- a release/tag/publication is required;
- credentials, signing, secrets or account permissions are needed;
- destructive repository history changes would be required;
- the task would knowingly break a supported platform without an accepted decision allowing it.

### Never do these autonomously

- Never force-push.
- Never rewrite `main` history.
- Never delete tags or published releases.
- Never create a release or version tag without explicit owner approval.
- Never install system-wide software with `winget`, `choco`, MSI installers, `apt`, `brew`, etc.
  unless explicitly authorized for that environment.
- Never add a large/new dependency merely to make one implementation easier.
- Never commit build outputs, downloaded dependency trees, IDE metadata or generated binaries.
- Never bypass failing tests by weakening/removing meaningful assertions.
- Never change production semantics solely to satisfy a test that is itself inconsistent with the
  established contract.

---

## 4. Git and CI workflow

### 4.1 Start clean and synchronize

Before each coherent development slice:

1. Inspect `git status`.
2. Preserve unrelated user changes; do not reset or overwrite them.
3. Fetch the remote.
4. Update `main` with fast-forward only.
5. Record the exact base SHA.
6. Check CI for that exact SHA.

Typical commands:

```bash
git status
git fetch --prune origin
git switch main
git pull --ff-only
git rev-parse HEAD
```

If GitHub CLI is available:

```bash
gh run list --commit "$(git rev-parse HEAD)" --workflow CI --limit 5
```

If the exact base SHA has a failing CI run, investigate and repair that failure before feature work.
If the exact base SHA is still queued/in progress and the repository owner has requested strict CI
gating, do not stack a new feature on top of an unverified base.

### 4.2 Use a focused branch

Default branch pattern:

```text
codex/<short-topic>
```

or an existing repository convention such as `feat/...`, `fix/...`, or `docs/...`.

One branch/PR should represent one coherent architectural concern. Do not combine unrelated cleanup,
renaming and new functionality "because the file is already open."

### 4.3 Commit discipline

Prefer small, meaningful commits that keep the branch buildable.

Good examples:

```text
feat: add rendered ComboBox popup hit testing
fix: preserve pointer capture cleanup on detach
test: cover stale popup snapshot rejection
docs: record rendered popup interaction contract
```

Before push/PR:

- inspect `git diff --check`;
- inspect `git diff` and `git status`;
- run the relevant tests;
- run the broader suite when the change can affect shared Core behavior.

Do not claim that a build/test passed unless it was actually executed.

### 4.4 CI failures

When CI is red:

1. identify the exact failing job and assertion/compiler diagnostic;
2. reproduce locally when practical;
3. fix only the real failure;
4. do not simultaneously continue unrelated feature development;
5. add/adjust a regression test when the failure exposed a missing contract.

A failing test is evidence, not an order to mutate production behavior. First determine whether the test
or the implementation violates the established contract.

---

## 5. Repository architecture map

### 5.1 Core: `SASD::UI`

Primary locations:

- public headers: `include/sasd/ui/`
- implementation: `src/`
- core tests: `tests/*_tests.cpp` linked into `sasd_ui_tests`

Core must remain platform/backend neutral.

Core owns semantic concepts such as:

- `Application`
- `Component`
- `Widget`
- `Container`
- geometry and size constraints
- events and routing
- focus and traversal
- hit testing and pointer routing/capture
- layouts
- semantic controls
- commands/menu models/shortcuts where implemented

Core public headers must not expose:

- SDL types;
- Win32 handles/types;
- POSIX `termios` types;
- Cocoa/AppKit types;
- terminal device/session implementation types;
- renderer-native font or graphics handles.

### 5.2 Terminal: `SASD::UI::Terminal`

Primary locations:

- public terminal headers: `include/sasd/ui/terminal/`
- implementation: `src/terminal/`
- tests: terminal test executable `sasd_ui_terminal_tests`
- demo: `examples/terminal_form_demo.cpp`

The Terminal backend is a first-class reference implementation. Keep terminal protocol/device details
inside the terminal layer.

Important established patterns:

- `ScreenBuffer` is an off-screen presentation value.
- `TextMetrics` owns terminal cell-width policy.
- UTF-8 scalar width and terminal cell occupancy are not interchangeable concepts.
- `AnsiFrameEncoder` converts a completed frame to terminal bytes before OS I/O.
- `TerminalSession`/`TerminalDevice` own native terminal lifecycle through RAII.
- Native POSIX/Windows implementation details remain outside public portable headers.
- Untrusted text must never accidentally become executable terminal control sequences.

### 5.3 Rendered: `SASD::UI::Rendered`

Primary locations:

- public rendered headers: `include/sasd/ui/rendered/`
- implementation: `src/rendered/`
- tests: `sasd_ui_rendered_tests`

Rendered is **not SDL**. It owns backend-neutral graphical concepts such as:

- `DisplayList`;
- `RenderDevice`;
- `RenderedMeasurementContext`;
- rendered geometry/theme metrics;
- rendered presentation sinks and hit-test helpers.

The generic Rendered target must stay buildable/testable without a graphical session and without SDL.

### 5.4 Experimental SDL3 adapter

Primary location:

- `src/rendered/sdl3/`
- `examples/rendered_sdl3_form_demo.cpp`

Target:

- `SASD::UI::Rendered::SDL3`

The SDL3 adapter is currently opt-in and build-tree-only. It may depend on SDL3/SDL_ttf; normal Core and
Rendered public API must not.

### 5.5 Tests

CTest names currently include:

- `sasd_ui_core_tests`
- `sasd_ui_terminal_tests`
- `sasd_ui_rendered_tests`
- `sasd_ui_sdl3_tests` when the SDL3 adapter and test font are enabled
- native POSIX PTY or Windows ConPTY smoke tests where applicable

Keep Core tests free of graphical-session requirements.

---

## 6. Core architectural invariants

### 6.1 Component, Widget and Container are different concepts

Do not collapse them into one universal base abstraction.

- `Component`: lifecycle/ownership concept; may be non-visual.
- `Widget`: visual/interactive semantic component.
- `Container`: owns components and visually parents Widget children.

**Ownership and visual parenting are distinct.**

A component can be owned without being a visual child. Event bubbling follows the visual parent path,
not arbitrary ownership.

### 6.2 Ownership and lifetime

- Prefer RAII.
- Ownership must be visible.
- Containers own adopted components by default.
- Raw pointers/references may be used for non-owning observations only.
- Do not introduce raw owning pointers.
- Do not use `shared_ptr` as a universal lifetime escape hatch.
- Every long-lived non-owning observation needs an explicit detach/lifetime story.

Callbacks deserve special care:

- establish the complete coherent state **before** invoking application callbacks;
- copy callback/reference state locally before mutation when needed;
- assume a callback may detach, release or destroy the initiating control;
- after invoking a callback that may end object lifetime, do not access that object's members again.

Tests should cover destructive/reentrant callback paths when a new API crosses such a boundary.

### 6.3 UI thread model

Widgets currently belong to one UI thread. Do not make individual widgets arbitrarily thread-safe.

Background work must eventually post/dispatch results back to the UI event queue through an explicit
mechanism. Do not hide worker threads/timers inside controls as a shortcut.

### 6.4 Error semantics

Choose deliberately between:

- result/optional return;
- exception;
- non-failing/precondition contract.

Do not mix models accidentally.

Presentation/geometry helpers generally prefer conservative fail-closed behavior when state is stale,
malformed, overflowed or unrepresentable. Do not silently clip or invent semantics unless the contract
explicitly says so.

---

## 7. Events, focus and pointer interaction

Keep these responsibilities separate:

- Backend normalizes native input into semantic events.
- Application pumps events.
- Target-selection policy chooses a target.
- EventDispatcher bubbles target -> visual parent -> root.
- FocusManager owns logical keyboard-focus observation.
- FocusTraversal is a separate navigation policy.
- PointerRouter owns pointer hit routing, geometric hover and semantic capture lifetime.

Do not make `Application` know widget-specific focus order or popup geometry.

### Focus policy

Widgets are not focusable by default. Host policy may request focus on a pointer press before normal
routing. Do not quietly make `PointerRouter` a `FocusManager`.

### Pointer capture

If a control has an armed multi-event gesture:

- capture must have a deterministic owner;
- capture loss must retire transient state;
- top-level pointer-surface leave is a lifecycle boundary;
- a stale release must not complete a gesture from an older interaction scope.

### Transient overlays

Menus and ComboBox popups have established a useful pattern:

1. Core owns semantic state.
2. Presentation builds an **owned value snapshot**.
3. Placement is resolved once against an explicit viewport.
4. Rendering/composition consumes that exact final snapshot.
5. Pointer hit testing must use the exact final row/geometry snapshot rather than re-deriving placement.
6. Stale semantic/presentation identity fails closed.
7. Base frames/display lists remain immutable inputs where practical.
8. Click-through to covered widgets is forbidden.

Do not put backend geometry into Core merely to make popup interaction easier.

---

## 8. Layout, measurement and invalidation

The project uses a two-phase layout contract:

```text
measure(constraints)
    -> desiredSize

arrange(final Rect)
    -> stored bounds
    -> child arrangement
```

Rules:

- Layout units are logical; do not assume pixels.
- Terminal uses cells; Rendered uses logical graphical units.
- Content measurement belongs behind `MeasurementContext`.
- Widget measurement caches must not retain a borrowed measurement-context pointer.
- If size/intrinsic content changes, invalidate measurement.
- If only appearance changes, invalidate visual presentation.
- Geometry/structural changes may require conservative subtree replay/restoration.
- Unknown composite containers must not silently drop children from presentation.

Do not add margin/flex/span/alignment APIs speculatively to an existing layout. Add public layout surface
only for a concrete supported use case and document the contract.

---

## 9. Text and Unicode

Public text starts from UTF-8.

Never assume:

```text
1 byte == 1 Unicode scalar == 1 grapheme == 1 terminal cell == 1 rendered glyph
```

Keep physical key events separate from committed text input.

For terminal work:

- use existing UTF-8 and `TextMetrics` helpers;
- preserve wide-cell lead/continuation invariants;
- respect ambiguous-width policy;
- do not pass arbitrary application text through as raw ANSI escape/control output.

For Rendered work:

- use the active `RenderedMeasurementContext`;
- keep measurement and presentation consistent;
- when caret/hit geometry depends on shaped text, reuse the same metric model rather than guessing from
  byte counts.

---

## 10. Public API and naming

Public namespace identity is carried by namespaces, not long class prefixes.

Preferred:

```cpp
sasd::ui::Window window;
sasd::ui::Button button{"OK"};
```

Avoid public names such as:

```text
SasdButton
SASDButton
TButton
```

unless an accepted ADR gives a specific technical reason.

Style direction:

- types: `PascalCase`;
- methods/functions: `camelCase`;
- local variables: `camelCase`;
- public headers: `include/sasd/ui/...`;
- internal/private backend helpers: keep out of the installed public include surface.

Prefer small APIs and standard-library vocabulary. Pre-1.0 allows justified source-breaking corrections,
but do not churn names/contracts casually.

---

## 11. Coding style for agent changes

### Readability

Prefer explicit, unsurprising C++ over compressed cleverness.

Good agent code should make these visible:

- why a branch exists;
- what invariant is being preserved;
- who owns an object;
- whether a pointer/reference is non-owning;
- when state becomes coherent;
- what can happen during callbacks;
- why overflow/stale state is rejected;
- which layer owns a policy.

### Comments

This repository intentionally values **detailed explanatory comments**.

Add comments generously where they explain design intent, lifetime rules, state transitions, portability
constraints or non-obvious arithmetic.

Do **not** fill files with comments that merely restate syntax.

Prefer a comment such as:

```cpp
// Retire the gesture before invoking the callback. The callback may release the control,
// so no member access is permitted after notification begins.
```

over:

```cpp
// Set pressed to false.
pressed_ = false;
```

Code/API comments should normally be English. Architecture documentation is bilingual where required.

### Optimization

Do not prematurely optimize away clear architecture.

Correctness-first value copies are acceptable in early presentation/composition code when they make
ownership and transactionality obvious. Record obvious optimization opportunities, but only optimize
after measurement or when a structural problem is clear.

Still avoid obvious traps such as:

- blocking work in the UI event loop;
- unbounded accidental copies of entire widget trees;
- one UI object per cell for future huge data views;
- repeated platform conversions without need.

---

## 12. Testing expectations

Behavior changes require tests in the same change.

### Minimum test selection

Choose the smallest relevant suite first:

```bash
ctest --test-dir build-codex -R sasd_ui_core_tests --output-on-failure
ctest --test-dir build-codex -R sasd_ui_terminal_tests --output-on-failure
ctest --test-dir build-codex -R sasd_ui_rendered_tests --output-on-failure
```

Then run the full configured suite before declaring the slice complete:

```bash
ctest --test-dir build-codex --output-on-failure
```

On Visual Studio/multi-config builds add `-C Debug` (or the chosen configuration).

### Test quality

Tests should cover contracts, not implementation trivia.

For stateful UI behavior, include cases such as:

- idempotent operations;
- disabled/hidden/unfocused behavior;
- focus loss;
- capture loss;
- pointer leaving/re-entering geometry;
- stale snapshots/structural mutation;
- empty content;
- boundary/half-open geometry;
- overflow/unrepresentable coordinates where relevant;
- callback ordering;
- callbacks that release/destroy the control when that is legal;
- no duplicate notification;
- no click-through from overlays.

Do not make a test pass by changing a correct idempotent return value merely because setup expected
`true`.

---

## 13. Standard local build commands

Use out-of-source build directories. Never commit them.

### Linux/macOS - normal strict development build

```bash
cmake -S . -B build-codex \
  -DCMAKE_BUILD_TYPE=Debug \
  -DSASD_UI_BUILD_TESTS=ON \
  -DSASD_UI_BUILD_EXAMPLES=ON \
  -DSASD_UI_WARNINGS_AS_ERRORS=ON

cmake --build build-codex --parallel
ctest --test-dir build-codex --output-on-failure
```

### Windows / Visual Studio 2022

```powershell
cmake -S . -B build-codex -G "Visual Studio 17 2022" -A x64 `
  -DSASD_UI_BUILD_TESTS=ON `
  -DSASD_UI_BUILD_EXAMPLES=ON `
  -DSASD_UI_WARNINGS_AS_ERRORS=ON

cmake --build build-codex --config Debug --parallel
ctest --test-dir build-codex -C Debug --output-on-failure
```

### Clang/GCC sanitizer build

```bash
cmake -S . -B build-codex-sanitized \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_COMPILER=clang++ \
  -DSASD_UI_BUILD_TESTS=ON \
  -DSASD_UI_WARNINGS_AS_ERRORS=ON \
  -DSASD_UI_ENABLE_SANITIZERS=ON

cmake --build build-codex-sanitized --parallel
ctest --test-dir build-codex-sanitized --output-on-failure
```

### Experimental SDL3 adapter

Only enable SDL3 when the task touches the adapter or a rendered-window integration path.

If the environment is allowed to fetch the pinned upstream dependencies:

```bash
cmake -S . -B build-codex-sdl3 \
  -DCMAKE_BUILD_TYPE=Debug \
  -DSASD_UI_BUILD_TESTS=ON \
  -DSASD_UI_BUILD_EXAMPLES=ON \
  -DSASD_UI_WARNINGS_AS_ERRORS=ON \
  -DSASD_UI_BUILD_SDL3_ADAPTER=ON \
  -DSASD_UI_FETCH_SDL3=ON \
  -DSASD_UI_SDL3_TEST_FONT=/path/to/test-font.ttf

cmake --build build-codex-sdl3 --parallel
ctest --test-dir build-codex-sdl3 -R sasd_ui_sdl3_tests --output-on-failure
```

If compatible SDL3/SDL3_ttf packages already exist, prefer `SASD_UI_FETCH_SDL3=OFF` and
`find_package`. Do not install system packages autonomously just to satisfy this optional adapter.

---

## 14. Cross-platform requirements

A Core change is not complete merely because it works on the current OS.

Keep normal CI compatibility with:

- GCC/Linux;
- Clang/Linux;
- AppleClang/macOS;
- MSVC/Windows.

Warnings are errors in CI.

Be especially careful with:

- signed/unsigned conversions;
- narrowing/overflow;
- aggregate initialization warnings;
- Windows PowerShell 5.1 script encoding;
- POSIX-only headers/APIs;
- Win32 macro collisions;
- single-config vs multi-config CMake behavior;
- filesystem/path assumptions.

Native terminal smoke tests intentionally use PTY/ConPTY process boundaries. Do not replace them with
mocks just to simplify a failure.

---

## 15. Dependency policy

The normal Core/Terminal/Rendered library should remain small and dependency-light.

Before adding any runtime/build dependency evaluate:

- license;
- platform coverage;
- maintenance state;
- API/ABI risk;
- package availability;
- whether it leaks into public headers;
- whether it can remain optional/backend-local;
- whether the standard library or existing project abstractions already solve the need.

SDL3/SDL_ttf are existing **optional** Rendered-adapter dependencies and are intentionally isolated.

Do not introduce Qt/GTK/Win32/AppKit dependency into Core.

---

## 16. Documentation and ADR policy

### Public/documented behavior

When public API or durable behavior changes:

- update API comments;
- update relevant tests;
- update conceptual docs when they would otherwise become false;
- keep German and English technical intent aligned.

Do not claim planned functionality is implemented.

### ADRs

Create or extend an ADR when a decision:

- changes public API/dependency model;
- affects multiple backends;
- changes portability;
- introduces/removes a major subsystem;
- is expensive to reverse;
- has an obvious simpler-looking alternative likely to be questioned later.

ADRs are bilingual **in one file**: English first, German second.

When creating a new ADR:

1. inspect `docs/adr/` for the highest existing number;
2. never reuse/renumber existing ADRs;
3. link predecessor/follow-up decisions when useful;
4. update the ADR index when doing so does not require unrelated historical reconstruction.

Small implementation details do not need an ADR.

---

## 17. Milestone discipline and avoiding over-engineering

Do not pull later milestones forward without a real consumer.

Examples:

- Do not introduce `ListModel` merely to implement a small ComboBox; Model/View belongs to the M5
  data-heavy-widget work unless a concrete cross-widget requirement justifies earlier extraction.
- Do not create a general overlay manager until several concrete overlays demonstrate the same
  lifecycle/composition need.
- Do not create a public universal "Control" base class merely because several controls share a few
  private gesture mechanics.
- Do not add native peers before shared semantic contracts are sufficiently proven.
- Do not add flex/grid span/theme/resource/designer systems speculatively.

A small private helper is often preferable to prematurely freezing a public abstraction.

---

## 18. Definition of done for a normal development slice

A slice is complete when all applicable points are true:

- task behavior is implemented;
- architecture boundary is preserved;
- ownership/lifetime behavior is explicit;
- error/stale-state behavior is defined;
- focused regression tests exist;
- relevant test target passes locally when runnable;
- full configured tests pass when runnable;
- warnings-as-errors build succeeds for the local toolchain;
- docs/API comments are updated where behavior changed;
- ADR exists when the decision is architectural;
- no unrelated files/build artifacts are included;
- diff has been reviewed;
- commit message describes the actual change;
- PR/CI status is reported accurately;
- no local/manual test is claimed unless actually performed.

---

## 19. Agent report format

At the end of a task, report concisely:

1. what changed;
2. the architectural reason for the chosen design;
3. files/areas changed;
4. tests/builds actually run and their result;
5. tests not run and why;
6. commit/PR identifier when created;
7. any remaining deferred follow-up.

Do not bury a failure. If work is blocked by CI, environment, dependency availability or ambiguity,
state the blocker and stop rather than pretending completion.

---

## 20. Quick decision checklist

Before writing a new piece of code, ask:

- Is this semantic behavior -> Core?
- Is this logical rendered geometry/drawing -> Rendered?
- Is this terminal-cell/protocol/session behavior -> Terminal?
- Is this SDL event/render/font plumbing -> SDL3 adapter?
- Am I putting backend types into a public Core header? If yes, stop.
- Is ownership obvious?
- Can a callback destroy/release this object?
- Is geometry computed once and then reused for rendering/hit testing?
- Does stale state fail closed?
- Is this a real current use case or speculative abstraction?
- Which deterministic test proves the contract?
- Does this need an ADR?
- Will GCC, Clang, AppleClang and MSVC understand this code the same way?

If those questions have clear answers, proceed autonomously.
