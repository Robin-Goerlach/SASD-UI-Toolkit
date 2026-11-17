# SASD UI Toolkit Project Requirements

- **Authority:** canonical requirements overview (English)
- **Scope:** stable summary of existing accepted product contracts; it does not create a new milestone or feature commitment
- **Owner:** repository owner / maintainer
- **Baseline:** M8-complete `main` at `3304658409e163fc7e667b9b03acf2e1d086b94e`
- **Status vocabulary:** `Verified`, `Implemented`, `Deferred`, `Open`
- **Last reviewed:** 2026-11-17

Detailed behavior remains authoritative in current source/tests, accepted ADRs and architecture
documentation. This index gives essential contracts stable IDs and points to their acceptance
evidence; it does not replace those sources.

| ID | Requirement | Acceptance / evidence | Status |
|---|---|---|---|
| SASD-UI-REQ-001 | The toolkit shall use C++20 and CMake while keeping normal Core builds dependency-light. | `CMakeLists.txt`; `docs/adr/0001-core-language-and-build-system.md`; cross-platform CI. | Verified |
| SASD-UI-REQ-002 | Application code shall use a semantic API whose Core is neutral to presentation and operating-system backends. | `docs/en/ARCHITECTURE.md`; ADRs 0002, 0005 and 0006; public-header build tests. | Verified |
| SASD-UI-REQ-003 | Terminal presentation shall remain a first-class backend rather than a degraded fallback. | `docs/adr/0007-terminal-first-class-backend.md`; terminal target/tests/demo; v0.1.0 release evidence. | Verified |
| SASD-UI-REQ-004 | Supported development targets shall include Windows, Linux and macOS with GCC, Clang, MSVC and AppleClang coverage. | `.github/workflows/ci.yml`; `docs/en/M8_EXIT_REVIEW.md`; exact-SHA CI runs. | Verified |
| SASD-UI-REQ-005 | Backend differences shall be represented through explicit capabilities instead of false uniformity. | `docs/en/BACKENDS.md`; `include/sasd/ui/backend.hpp`; backend tests. | Verified |
| SASD-UI-REQ-006 | Ownership and lifetime shall use RAII and explicit ownership; raw pointers/references shall be non-owning observations with detach behavior. | `docs/adr/0004-component-model-and-ownership.md`; `docs/en/ARCHITECTURE.md`; destructive-callback/lifetime tests. | Verified |
| SASD-UI-REQ-007 | Core public headers shall not expose SDL, Win32, POSIX terminal, GTK or AppKit implementation types. | Target separation in `CMakeLists.txt`; public headers; installed-package consumers; M8 packaging evidence. | Verified |
| SASD-UI-REQ-008 | Semantic and presentation logic shall be deterministically testable without an interactive graphical session where practical. | `SASD::UI::Mock`, Terminal/Rendered headless tests, offscreen SDL3 tests and CTest integration. | Verified |
| SASD-UI-REQ-009 | Core, Terminal and Rendered shall remain separately linkable targets; optional concrete/native adapters shall remain isolated. | `CMakeLists.txt`; `docs/en/BACKENDS.md`; Debug/Release installed-package consumer. | Verified |
| SASD-UI-REQ-010 | Native input shall be normalised into semantic events; dispatch, focus, traversal, hit testing and pointer capture shall keep separate responsibilities. | `docs/en/ARCHITECTURE.md`; ADRs 0020, 0032, 0035 and 0036; Core tests. | Verified |
| SASD-UI-REQ-011 | Layout shall use backend-neutral two-phase `measure`/`arrange` contracts and presentation invalidation distinct from measurement. | ADRs 0011–0016; layout and presentation tests. | Verified |
| SASD-UI-REQ-012 | Public text shall be UTF-8 and code shall not equate bytes, scalars, graphemes, terminal cells and rendered glyphs. | `docs/adr/0014-terminal-unicode-cell-width-policy.md`; UTF-8/TextMetrics tests; Rendered measurement ADRs. | Verified |
| SASD-UI-REQ-013 | Transient overlays shall render and hit-test the same owned final geometry snapshot and shall reject stale identity without click-through. | Menu/ComboBox ADRs 0059, 0122–0134; Terminal/Rendered tests. | Verified |
| SASD-UI-REQ-014 | List, Table and Tree views shall use model/view contracts with bounded visible-value snapshots rather than one UI object per data item. | ADRs 0135–0150; Core/Terminal/Rendered view tests. | Verified |
| SASD-UI-REQ-015 | UI Description JSON v1 and v2 shall use strict validated owning documents with deterministic canonical encoding. | `docs/en/UI_DESCRIPTION_FORMAT.md`; `docs/en/UI_DESCRIPTION_FORMAT_V2.md`; ADRs 0186, 0187 and 0197; parser/encoder tests. | Verified |
| SASD-UI-REQ-016 | Reconstruction shall validate before phased construction and shall not return a partial root after validation or construction failure. | ADRs 0188 and 0198; reconstruction tests; `docs/en/M8_EXIT_REVIEW.md`. | Verified |
| SASD-UI-REQ-017 | The current resource pipeline shall provide stable keys, explicit dictionary ownership, typed references, explicit resolution/scoping, persistence and one-time materialisation for `TextStyle`. | ADRs 0194–0198; JSON v2/resource tests; M8 Exit Review. | Verified |
| SASD-UI-REQ-018 | Public tooling metadata shall support instance-free type/property discovery, defaults, validation, references, resources and owning document editing. | `docs/en/TOOLING_INTEGRATION.md`; ADRs 0171–0199; installed public consumer. | Verified |
| SASD-UI-REQ-019 | The project shall provide an installable, versioned CMake package whose consumer target names match build-tree names. | Package configuration in `CMakeLists.txt`; `tests/package_consumer`; Release install verification and CI. | Verified |
| SASD-UI-REQ-020 | Pre-1.0 compatibility may change for justified architectural corrections; releases shall document breaking changes and migration impact. | `docs/adr/0010-release-and-compatibility-strategy.md`; README compatibility section; CHANGELOG. | Implemented |
| SASD-UI-REQ-021 | Optional SDL3, GTK, AppKit and Win32 integration shall not become mandatory Core dependencies. | Build options and target boundaries in `CMakeLists.txt`; native/SDL adapter CI; architecture docs. | Verified |
| SASD-UI-REQ-022 | Untrusted terminal text, JSON, references and resource input shall pass through structured validation/encoding boundaries and fail closed on malformed or stale state. | `docs/SASD_SECURITY_ASSESSMENT.md`; encoder/parser/graph tests; ASan/UBSan CI. | Verified |
| SASD-UI-REQ-023 | Release claims shall distinguish automated CI from visible human platform validation; the macOS/Cocoa M3 gate shall remain open until manually passed. | `docs/en/M3_RENDERED_DESKTOP_SMOKE_TEST.md`; `docs/en/M8_EXIT_REVIEW.md`; ADR 0200; README/ROADMAP. | Verified |
| SASD-UI-REQ-024 | New public abstractions and resource/widget systems shall require a concrete consumer and shall not pull later milestones forward speculatively. | `docs/en/DEVELOPMENT_GUIDELINES.md`; ADRs 0008, 0009, 0150 and 0200. | Implemented |

## Traceability policy

For essential requirements, the stable ID above may be referenced by future ADRs, issues, pull
requests and release reviews. Acceptance remains based on the cited implementation, tests and
executed evidence. When an existing contract changes, this table and the authoritative detailed
source must change together; historical ADRs are not rewritten to disguise the change.

German architecture, roadmap and format documentation mirrors the same technical intent. This
English requirements index is the sole canonical requirement list, avoiding two independently
maintained matrices.
