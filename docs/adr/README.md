# Architecture Decision Records (ADR)

This directory records the important architectural decisions of **SASD UI Toolkit**. The ADRs complement the higher-level documents in `docs/de` and `docs/en`: the general documentation explains the current architecture, while ADRs preserve **why** a decision was made, which alternatives were considered, and what consequences follow from it.

Each ADR is bilingual. English is followed by German in the same file so the rationale cannot diverge silently between two separate documents.

## Status vocabulary

- **Proposed** – under discussion; not yet binding.
- **Accepted** – current project decision.
- **Superseded** – replaced by a newer ADR.
- **Deprecated** – still present for history but should no longer guide new work.

## Complete index

The table below is generated from every numbered ADR file currently present in this directory. Filenames, titles and status values are kept in sync with those files; no ADR is renumbered or removed.

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
| [0009](0009-designer-as-separate-project.md) | Visual designer as a separate project | Accepted |
| [0010](0010-release-and-compatibility-strategy.md) | Release and compatibility strategy | Accepted |
| [0011](0011-two-phase-layout-measure-arrange.md) | Two-phase layout measurement and arrangement | Accepted |
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
| [0037](0037-initial-checkbox-semantics.md) | Initial CheckBox semantics / Initiale CheckBox-Semantik | Accepted |
| [0038](0038-explicit-radio-group-and-radio-button-semantics.md) | Explicit RadioGroup and initial RadioButton semantics / Explizite RadioGroup- und initiale RadioButton-Semantik | Accepted |
| [0039](0039-radio-group-arrow-key-navigation.md) | Explicit RadioGroup arrow-key navigation policy / Explizite RadioGroup-Pfeiltasten-Navigation | Accepted |
| [0040](0040-initial-grid-layout-track-semantics.md) | Initial GridLayout track semantics / Initiale GridLayout-Track-Semantik | Accepted |
| [0041](0041-initial-form-layout-semantics.md) | Initial FormLayout semantics / Initiale FormLayout-Semantik | Accepted |
| [0042](0042-initial-stack-layout-overlay-semantics.md) | Initial StackLayout overlay semantics / Initiale StackLayout-Overlay-Semantik | Accepted |
| [0043](0043-initial-command-semantics.md) | Initial Command semantics / Initiale Command-Semantik | Accepted |
| [0044](0044-command-state-observation.md) | Lifetime-safe Command state observation / Lifetime-sichere Command-State-Observation | Accepted |
| [0045](0045-button-command-binding.md) | Initial Button-to-Command binding / Initiales Button-zu-Command-Binding | Accepted |
| [0046](0046-initial-shortcut-map-semantics.md) | Initial ShortcutMap semantics / Initiale ShortcutMap-Semantik | Accepted |
| [0047](0047-initial-semantic-menu-model.md) | Initial semantic menu model / Initiales semantisches Menümodell | Accepted |
| [0048](0048-menu-bar-ownership.md) | Menu bar structural ownership / Strukturelles Ownership der Menüleiste | Accepted |
| [0049](0049-recursive-submenu-ownership.md) | Recursive submenu ownership / Rekursives Ownership von Untermenüs | Accepted |
| [0050](0050-semantic-menu-navigation.md) | Semantic menu navigation / Semantische Menünavigation | Accepted |
| [0051](0051-popup-menu-key-interpretation.md) | Popup menu key interpretation / Tastaturinterpretation für Popup-Menüs | Accepted |
| [0052](0052-indexed-menu-popup-paths.md) | Indexed menu popup paths / Indexbasierte Menü-Popup-Pfade | Accepted |
| [0053](0053-menu-bar-key-navigation.md) | Menu-bar key navigation / Tastaturnavigation der Menüleiste | Accepted |
| [0054](0054-menu-interaction-controller.md) | Backend-neutral menu interaction controller / Backend-neutraler Menü-Interaktionscontroller | Accepted |
| [0055](0055-root-popup-horizontal-menu-switching.md) | Root-popup horizontal menu switching / Horizontales Umschalten geöffneter Hauptmenüs | Accepted |
| [0056](0056-vertical-arrow-root-menu-entry.md) | Vertical-arrow entry into root menus / Öffnen von Root-Menüs per vertikaler Pfeiltaste | Accepted |
| [0057](0057-ephemeral-menu-popup-presentation-views.md) | Ephemeral popup presentation views / Flüchtige Popup-Presentation-Views | Accepted |
| [0058](0058-ephemeral-top-level-menu-interaction-view.md) | Ephemeral top-level menu interaction view / Flüchtige Top-Level-Menü-Interaktionssicht | Accepted |
| [0059](0059-owned-menu-presentation-snapshots.md) | Owned menu presentation snapshots / Besitzende Menü-Presentation-Snapshots | Accepted |
| [0060](0060-deterministic-shortcut-display-text.md) | Deterministic shortcut display text / Deterministische Shortcut-Anzeigetexte | Accepted |
| [0061](0061-terminal-menu-measurement-boundary.md) | Terminal menu measurement boundary / Terminal-Menü-Messgrenze | Accepted |
| [0062](0062-headless-terminal-popup-rendering.md) | Headless terminal popup rendering / Headless-Terminal-Popup-Rendering | Accepted |
| [0063](0063-headless-terminal-menu-bar-rendering.md) | Headless Terminal Menu-Bar Rendering | Accepted |
| [0064](0064-transactional-terminal-menu-frame-composition.md) | Transactional Terminal Menu-Frame Composition | Accepted |
| [0065](0065-natural-terminal-menu-popup-placement.md) | Natural Terminal Menu-Popup Placement | Accepted |
| [0066](0066-terminal-menu-popup-viewport-fitting.md) | Terminal Menu Popup Viewport Fitting | Accepted |
| [0067](0067-direction-aware-terminal-submenu-viewport-placement.md) | Direction-Aware Terminal Submenu Viewport Placement | Accepted |
| [0068](0068-direction-sensitive-terminal-submenu-indicators.md) | Direction-Sensitive Terminal Submenu Indicators | Accepted |
| [0069](0069-terminal-menu-frame-direction-metadata.md) | Direction Metadata Belongs to Owned Terminal Menu Frame Layers | Accepted |
| [0070](0070-terminal-menu-frame-builder.md) | Build Owned Terminal Menu Frames from Interaction and Placement State | Accepted |
| [0071](0071-terminal-menu-interaction-presentation.md) | Compose Terminal Menu Frame Building and Rendering Behind a Thin Interaction Presentation Boundary | Accepted |
| [0072](0072-terminal-menu-base-frame-composition.md) | Compose Transient Terminal Menus from an Explicit Base Frame | Accepted |
| [0073](0073-terminal-menu-caret-composition.md) | Compose Terminal Menu Cursor Metadata with the Overlay Frame | Accepted |
| [0074](0074-generic-terminal-presentation-frame.md) | Generalize the Owned Terminal Presentation Frame | Accepted |
| [0075](0075-terminal-presentation-frame-transport.md) | TerminalSession Accepts Complete Terminal Presentation Frames | Accepted |
| [0076](0076-terminal-presentation-frame-capture.md) | Capture Owned Terminal Presentation Frames from the Presentation Sink | Accepted |
| [0077](0077-terminal-presentation-frame-menu-composition.md) | Compose Terminal Menus Directly from Complete Presentation Frames | Accepted |
| [0078](0078-terminal-form-demo-menu-integration.md) | Integrate Semantic Menus into the Terminal Form Demo | Accepted |
| [0079](0079-two-phase-shortcut-resolution.md) | Two-phase shortcut resolution | Accepted |
| [0080](0080-backend-neutral-text-clipboard-foundation.md) | Backend-neutral text clipboard foundation | Accepted |
| [0081](0081-explicit-textfield-paste-through-clipboard.md) | Explicit TextField paste through the clipboard service | Accepted |
| [0082](0082-textfield-scalar-selection-anchor-cursor.md) | TextField scalar selection as anchor and active cursor | Accepted |
| [0083](0083-selection-based-textfield-copy-and-transactional-cut.md) | Selection-based TextField copy and transactional cut | Accepted |
| [0084](0084-textfield-shift-navigation-extends-selection.md) | TextField Shift navigation extends the active selection end | Accepted |
| [0085](0085-terminal-textfield-selection-presentation.md) | Terminal TextField selection presentation by inverse-style toggling | Accepted |
| [0086](0086-rendered-textfield-selection-presentation.md) | Rendered TextField selection presentation by full-run replay and clipped contrast overlay | Accepted |
| [0087](0087-rendered-textfield-captured-drag-caret-mapping.md) | Rendered TextField captured-drag caret mapping | Accepted |
| [0088](0088-rendered-textfield-pointer-drag-selection.md) | Rendered TextField pointer-drag selection through Core capture | Accepted |
| [0089](0089-rendered-textfield-shift-click-selection.md) | Exact Shift+click extends Rendered TextField selection | Accepted |
| [0090](0090-rendered-textfield-scalar-under-pointer-mapping.md) | Rendered TextField scalar-under-pointer mapping | Accepted |
| [0091](0091-rendered-textfield-double-click-word-selection.md) | Basic word boundaries and atomic rendered TextField double-click selection | Accepted |
| [0092](0092-rendered-textfield-triple-click-selection.md) | Rendered TextField triple-click selects complete single-line content | Accepted |
| [0093](0093-rendered-textfield-captured-scalar-drag-mapping.md) | Captured rendered TextField scalar-span drag mapping | Accepted |
| [0094](0094-rendered-textfield-word-drag-selection.md) | Stateful rendered TextField word-granular pointer dragging | Accepted |
| [0095](0095-terminal-sgr-mouse-input-decoding.md) | Terminal SGR mouse input decoding | Accepted |
| [0096](0096-terminal-pointer-reporting-session-lifetime.md) | RAII lifetime for terminal pointer reporting | Accepted |
| [0097](0097-terminal-pointer-events-route-through-core-pointer-router.md) | Terminal pointer events route through Core PointerRouter | Accepted |
| [0098](0098-terminal-textfield-caret-hit-testing.md) | Terminal TextField caret hit testing in cell geometry | Accepted |
| [0099](0099-terminal-textfield-pointer-selection-through-core-capture.md) | Terminal TextField pointer selection through Core capture | Accepted |
| [0100](0100-terminal-multi-click-synthesis-in-event-pump.md) | Terminal multi-click synthesis in the event pump | Accepted |
| [0101](0101-terminal-textfield-strict-scalar-hit-testing.md) | Terminal TextField strict scalar hit testing | Accepted |
| [0102](0102-terminal-textfield-double-click-word-selection.md) | Terminal TextField double-click word selection | Accepted |
| [0103](0103-terminal-textfield-triple-click-select-all.md) | Terminal TextField triple-click select-all | Accepted |
| [0104](0104-terminal-textfield-word-drag-selection.md) | Terminal TextField word-granular double-click dragging | Accepted |
| [0105](0105-terminal-menu-pointer-hit-testing.md) | Terminal menu pointer hit testing uses presentation-frame geometry | Accepted |
| [0106](0106-terminal-menu-pointer-interaction.md) | Terminal menu pointer interaction opens top-level menus and owns outside dismissal | Accepted |
| [0107](0107-terminal-popup-pointer-row-selection.md) | Terminal popup pointer presses select semantic menu rows | Accepted |
| [0108](0108-terminal-popup-pointer-command-activation.md) | Terminal popup Commands activate on matching pointer release | Accepted |
| [0109](0109-terminal-popup-pointer-submenu-opening.md) | Terminal popup submenu clicks use a backend-neutral opening transaction | Accepted |
| [0110](0110-terminal-popup-pointer-motion-selection.md) | Terminal popup pointer motion selects rows without completing menu items | Accepted |
| [0111](0111-terminal-pointer-tracking-mode.md) | Terminal pointer tracking policy is explicit and can opt into all-motion reporting | Accepted |
| [0112](0112-terminal-top-level-menu-pointer-motion-switching.md) | Active terminal menus switch top-level root popups on pointer motion | Accepted |
| [0113](0113-terminal-submenu-hover-delay.md) | Deterministic delayed submenu hover policy for terminal menus | Accepted |
| [0114](0114-terminal-submenu-close-grace.md) | Delayed terminal submenu close grace for popup transfer gaps | Accepted |
| [0115](0115-terminal-submenu-pointer-intent-corridor.md) | Terminal submenu pointer-intent corridor foundation | Accepted |
| [0116](0116-terminal-safe-triangle-sibling-deferral.md) | Deterministic terminal safe-triangle sibling deferral | Accepted |
| [0117](0117-initial-core-combobox-selection-contract.md) | Initial Core ComboBox selection contract | Accepted |
| [0118](0118-terminal-collapsed-combobox-presentation.md) | Terminal collapsed ComboBox presentation | Accepted |
| [0119](0119-rendered-collapsed-combobox-presentation.md) | Rendered collapsed ComboBox presentation | Accepted |
| [0120](0120-combobox-dropdown-intent.md) | Core ComboBox drop-down intent and focus lifetime | Accepted |
| [0121](0121-combobox-preview-commit-cancel.md) | ComboBox preview, commit and cancel transaction | Accepted |
| [0122](0122-backend-neutral-anchored-popup-layout.md) | Backend-neutral anchored popup placement and fixed-row geometry | Accepted |
| [0123](0123-terminal-combobox-popup-presentation.md) | Headless Terminal ComboBox popup presentation | Accepted |
| [0124](0124-terminal-demo-combobox-popup-integration.md) | Terminal demo host integration for ComboBox popup overlays | Accepted |
| [0125](0125-terminal-combobox-popup-hover-preview.md) | Terminal ComboBox popup row hit testing and hover preview | Accepted |
| [0126](0126-terminal-combobox-popup-primary-click-commit.md) | Terminal ComboBox popup Primary click commit transaction | Accepted |
| [0127](0127-terminal-combobox-popup-outside-dismissal.md) | Terminal ComboBox Primary outside-press dismissal | Accepted |
| [0128](0128-combobox-collapsed-primary-pointer-activation.md) | Backend-neutral collapsed ComboBox Primary pointer activation | Accepted |
| [0129](0129-rendered-combobox-popup-presentation.md) | Rendered ComboBox popup presentation snapshot and DisplayList overlay | Accepted |
| [0130](0130-sdl3-rendered-combobox-popup-integration.md) | SDL3 form demo integration for Rendered ComboBox popup overlays | Accepted |
| [0131](0131-rendered-combobox-popup-pointer-interaction.md) | Rendered ComboBox popup pointer interaction uses final snapshots | Accepted |
| [0132](0132-logical-latin-letter-key-identity.md) | Logical Latin-letter key identity / Logische Tastaturidentität für lateinische Buchstaben | Accepted |
| [0133](0133-rendered-menu-popup-snapshot.md) | Rendered menu popup snapshots | Accepted |
| [0134](0134-rendered-menu-frame-and-sdl3-overlay-policy.md) | Rendered menu frame and SDL3 overlay interaction policy | Accepted |
| [0135](0135-list-model-observation-and-lifetime.md) | ListModel observation and lifetime contract | Accepted |
| [0136](0136-list-selection-normalization.md) | Initial single-selection normalization for ListView | Accepted |
| [0137](0137-terminal-list-view-snapshot-and-hit-testing.md) | Terminal ListView snapshots and exact row hit testing | Accepted |
| [0138](0138-sdl3-list-view-demo-consumer.md) | SDL3 demo ListView consumer | Accepted |
| [0139](0139-table-model-rectangular-text-contract.md) | Rectangular textual TableModel contract | Accepted |
| [0140](0140-table-view-core-virtualization.md) | TableView core virtualization and owned visible values | Accepted |
| [0141](0141-rendered-table-view-owned-presentation.md) | Owned Rendered TableView presentation snapshot | Accepted |
| [0142](0142-table-selection-model.md) | Dedicated two-dimensional TableSelectionModel | Accepted |

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
