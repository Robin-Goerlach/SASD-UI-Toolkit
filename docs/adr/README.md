# Architecture Decision Records (ADR)

This directory records the important architectural decisions of **SASD UI Toolkit**. The ADRs complement the higher-level documents in `docs/de` and `docs/en`: the general documentation explains the current architecture, while ADRs preserve **why** a decision was made, which alternatives were considered, and what consequences follow from it.

Each ADR is bilingual. English is followed by German in the same file so the rationale cannot diverge silently between two separate documents.

## Status vocabulary

- **Proposed** – under discussion; not yet binding.
- **Accepted** – current project decision.
- **Superseded** – replaced by a newer ADR.
- **Deprecated** – still present for history but should no longer guide new work.

## Initial decisions

| ADR | Decision | Status |
|---|---|---|
| [0001](0001-core-language-and-build-system.md) | C++20 core and CMake build system | Accepted |
| [0002](0002-project-scope-and-product-boundaries.md) | Product scope and boundaries | Accepted |
| [0003](0003-public-naming-and-namespaces.md) | Public naming and namespaces | Accepted |
| [0004](0004-component-model-and-ownership.md) | Component model and ownership | Accepted |
| [0005](0005-backend-peer-and-capabilities.md) | Backend/peer architecture and capabilities | Accepted |
| [0006](0006-mock-backend-first.md) | Headless/mock backend before platform backends | Accepted |
| [0007](0007-terminal-first-class-backend.md) | Terminal as a first-class backend | Accepted |
| [0008](0008-model-view-for-data-widgets.md) | Model/View for data-heavy widgets | Accepted |
| [0009](0009-designer-as-separate-project.md) | Visual designer outside the core toolkit | Accepted |
| [0010](0010-release-and-compatibility-strategy.md) | Small vertical releases and pre-1.0 compatibility policy | Accepted |
| [0011](0011-two-phase-layout-measure-arrange.md) | Two-phase backend-neutral measure/arrange layout contract | Accepted |
| [0012](0012-visual-update-invalidation.md) | Separate visual update invalidation from layout invalidation | Accepted |
| [0013](0013-presentation-coordinator-and-sink.md) | Presentation synchronization coordinator and sink boundary | Accepted |
| [0014](0014-terminal-unicode-cell-width-policy.md) | Versioned terminal Unicode cell-width policy | Accepted |
| [0015](0015-backend-neutral-measurement-context.md) | Backend-neutral measurement context for intrinsic widget metrics | Accepted |
| [0016](0016-conservative-presentation-subtree-refresh.md) | Conservative subtree refresh for geometry and structural presentation changes | Accepted |
| [0017](0017-initial-vbox-hbox-layout-semantics.md) | Initial VBox/HBox layout semantics | Accepted |
| [0018](0018-initial-button-semantics.md) | Initial Button activation, measurement and terminal presentation semantics | Accepted |
| [0019](0019-initial-textfield-editing-and-caret.md) | Initial TextField editing, cursor and terminal-caret semantics | Accepted |
| [0020](0020-tab-focus-traversal.md) | Deterministic Tab/Shift+Tab focus traversal | Accepted |
| [0021](0021-deterministic-ansi-frame-encoding.md) | Deterministic ANSI/VT frame encoding before terminal device I/O | Accepted |
| [0022](0022-native-terminal-device-session-boundary.md) | Native terminal device/session boundary with transactional restoration | Accepted |
| [0023](0023-nonblocking-terminal-input-decoding.md) | Non-blocking terminal byte input and incremental ANSI decoding | Accepted |
| [0024](0024-terminal-event-pump-backend-and-demo.md) | Terminal event pump, Backend integration and runnable M2 loop | Accepted |
| [0025](0025-minimal-backend-neutral-text-styling.md) | Minimal backend-neutral text styling before a theme system | Accepted |
| [0026](0026-backend-neutral-function-keys.md) | Backend-neutral F1–F12 identity and terminal function-key decoding | Accepted |
| [0027](0027-rendered-display-list-and-sdl3-boundary.md) | Deterministic rendered display list and optional SDL3 adapter boundary | Accepted |
| [0028](0028-rendered-text-metrics-and-textfield-caret.md) | Rendered text metrics and TextField caret/viewport contract | Accepted |
| [0029](0029-render-device-replay-boundary.md) | Render device replay boundary below DisplayList | Accepted |
| [0030](0030-headless-sdl3-software-adapter.md) | Headless SDL3 software adapter before desktop-window lifecycle | Accepted |
| [0031](0031-window-backed-sdl3-host-and-presentation-replay.md) | Window-backed SDL3 host, presentation replay and desktop event boundary | Accepted |
| [0032](0032-pointer-routing-capture-and-button-pressed-state.md) | Backend-neutral pointer routing, capture and Button pressed state | Accepted |
| [0033](0033-rendered-textfield-click-to-caret.md) | Rendered TextField click-to-caret mapping | Accepted |
| [0034](0034-rendered-geometry-theme-metrics.md) | Rendered geometry theme metrics before a full theme system | Accepted |
| [0035](0035-geometric-pointer-hover-state.md) | Geometric pointer hover as direct Widget state | Accepted |
| [0036](0036-pointer-surface-lifecycle.md) | Pointer surface lifecycle is separate from Widget hover boundaries | Accepted |
| [0037](0037-initial-checkbox-semantics.md) | Initial two-state CheckBox semantics and input contract | Accepted |
| [0038](0038-explicit-radio-group-and-radio-button-semantics.md) | Explicit RadioGroup and initial RadioButton selection semantics | Accepted |
| [0039](0039-radio-group-arrow-key-navigation.md) | Explicit RadioGroup arrow-key navigation policy | Accepted |
| [0040](0040-initial-grid-layout-track-semantics.md) | Initial row-major GridLayout track semantics | Accepted |
| [0041](0041-initial-form-layout-semantics.md) | Initial two-column FormLayout semantics | Accepted |

## ADR policy

Create an ADR when a choice:

- changes the public API or dependency model;
- is expensive to reverse;
- affects more than one backend;
- introduces or removes a major subsystem;
- changes the portability model;
- is likely to be questioned later because a simpler-looking alternative exists.

Small implementation details do not need ADRs.

---

# Architekturentscheidungen (ADR)

Dieses Verzeichnis hält die wesentlichen Architekturentscheidungen des **SASD UI Toolkit** fest. Die ADRs ergänzen die Dokumente unter `docs/de` und `docs/en`: Die allgemeine Dokumentation beschreibt die aktuelle Architektur; die ADRs bewahren zusätzlich, **warum** eine Entscheidung getroffen wurde, welche Alternativen betrachtet wurden und welche Konsequenzen daraus entstehen.

Jedes ADR ist zweisprachig. Englisch und Deutsch stehen bewusst in derselben Datei, damit die Begründungen nicht unbemerkt auseinanderlaufen.

Ein ADR sollte insbesondere für Entscheidungen angelegt werden, die öffentliche APIs, Abhängigkeiten, mehrere Backends, Portabilität oder schwer umkehrbare Architekturgrenzen betreffen.
