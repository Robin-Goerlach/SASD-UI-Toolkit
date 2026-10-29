# Changelog

All notable changes to SASD UI Toolkit are documented in this file.

The project is still pre-1.0. Source-breaking corrections may occur between feature releases when
they improve the architecture before compatibility commitments become expensive.

## [0.2.0] - Unreleased

### Added

- M8 Slice 22: immutable, instance-free reference metadata defines source/target type contracts for
  all eleven built-in reference kinds, including model base-chain compatibility and the optional
  singular source-plus-kind contract. The format-neutral document validator consumes this catalog
  and separate RadioGroup graph-validation metadata, removing its hardcoded relation table and
  selected-group branch. The JSON parser no longer duplicates endpoint types; all JSON v1 symbols,
  writer bytes, capture and runtime reconstruction semantics remain unchanged. The current built-in
  validation-metadata coverage is complete; designer-friendly attributes, resources and IDE
  integration remain open (ADR 0192).

- M8 Slice 21: instance-free same-node cross-property validation metadata describes
  `ComboBox.selectedIndex` as an optional index into the exact `ComboBox.items`
  `string_collection` descriptor. The format-neutral document validator now resolves and evaluates
  this descriptor dependency, removing its hardcoded ComboBox type branch while preserving null
  selection, safe `uint64_t`/collection-size comparison and input-order independence. Runtime
  ComboBox behavior, reconstruction order and JSON v1 remain unchanged; reference/graph validation
  is still separate (ADR 0191).

- M8 Slice 20: instance-free string normalization metadata for all six current built-in string
  properties and atomic validation metadata for `Widget.sizeConstraints`. `TextField.text` declares
  `single_line_utf8`; Command, Label, Button, CheckBox and RadioButton text explicitly declare
  identity normalization. The format-neutral document validator now reuses both exact-descriptor
  policy families, removing its hardcoded TextField branch and implicit all-size-constraints rule.
  Runtime setters and JSON v1 remain unchanged; cross-property and graph rules are still concrete
  validator concerns rather than a generic rule engine (ADR 0190).

- M8 Slice 19: instance-free static integer property validation metadata with separate signed and
  unsigned inclusive range types. Six layout spacing properties expose `[0, Coordinate::max]` and
  `GridLayout.columnCount` exposes the host-independent `[1, UINT64_MAX]` semantic domain. Complete
  descriptor identity is validated by value. The common document validator now reuses the metadata,
  rejecting negative or out-of-Coordinate spacing before reconstruction and replacing its hardcoded
  zero-column special case. Runtime setters and host representability checks remain authoritative at
  their existing layers; no generic validation engine is introduced (ADR 0189).

- M8 Slice 18: built-in UI-description reconstruction from a revalidated format-neutral document.
  Phased concrete construction, constructor-based RadioGroup membership, Command pre-restore,
  explicit reference ordering, final base-to-derived property restoration and ordered ownership
  adoption produce one atomic owning root. The internal construction adapter covers all concrete
  built-ins; abstract model bases fail explicitly. Canonical encode/parse/reconstruct roundtrips are
  byte-identical while model payload and transient state remain excluded (ADR 0188).

- M8 Slice 17: dependency-free JSON v1 reader and format-neutral owning document model with
  eight concrete property types, stable parse error codes and input byte offsets. Strict schema,
  required-property and unknown/duplicate-member validation; exact UTF-8/Unicode escapes and
  integer parsing; ownership preorder, reference types and value-level semantic invariants.
  Cosmetic input order is accepted; canonical writer bytes stay unchanged. No is_default,
  Component construction/reconstruction or file I/O (ADR 0187).

- M8 Slice 16: dependency-free canonical JSON UI-description encoding v1 through
  encodeBuiltInUiDescriptionJson; recommended `.sasd-ui.json` extension and stable format/version
  markers independent of toolkit releases. Deterministic node/property/reference order, two-space
  indentation and LF-only bytes; symbolic property kinds, references and Color names; all default
  values explicit, is_default excluded. Invalid UTF-8 and unknown Color fail closed. No file I/O,
  model payload persistence, parser, deserializer or reconstruction yet (ADR 0186).

- M8 Slice 15: current inline TextStyle is covered as durable UI-description authoring state with
  the concrete text_style kind on Label, Button, TextField, CheckBox, RadioButton and ComboBox.
  Typed access, TextStyle{} defaults and owned snapshots preserve presentation-only setter behavior.
  Backend overlays remain excluded; the future theme/resource system stays separate. The current
  built-in coverage is ready for persisted encoding, which is still not implemented (ADR 0185).

- M8 Slice 14: Widget.sizeConstraints joins built-in property metadata with the concrete
  size_constraints kind, typed access/defaults and owned snapshots using the existing public value.
  Semantic setter validation, exceptions and measurement invalidation are preserved; no generic
  compound carrier is introduced. The public API coverage review completes the current
  collection/compound property milestone (ADR 0184); encoding and reconstruction remain open.

- M8 Slice 13: concrete string_collection and optional_unsigned_integer property kinds cover
  ComboBox.items / selectedIndex through typed runtime access, effective defaults and owned snapshots.
  Checked integral conversions preserve semantic setter/domain exception and callback behavior.
  Future reconstruction requires items before selectedIndex; no serializer or reconstruction yet.

- M8 Slice 12: List/Table/Tree SelectionModels now participate in non-visual Component lifecycle
  with three stable built-in type IDs, ordinary ownership/node identity and empty scalar snapshots.
  View-to-SelectionModel and SelectionModel-to-Model reference edges preserve sharing and independent
  bindings. Selected row/cell/node remains transient; no generic SelectionModel abstraction is
  introduced. External live targets fail closed; null/expired targets produce no edge (ADR 0182).

- M8 Slice 11: owned Component cross-reference edge snapshots compose the ownership snapshot with
  Button/Command, RadioButton/RadioGroup and List/Table/Tree View-to-Model edges using local node IDs.
  External live targets fail closed; null and expired targets produce no edge. SelectionModel
  association policy remains open; model payloads and serialization remain deferred (ADR 0181).

- M8 Slice 10: description-local Component node IDs and ordered built-in ownership snapshots,
  including non-visual owned Components, composed with existing property snapshots. Unknown exact
  types and ownership/Widget-parent invariant violations fail closed atomically. Cross-reference
  edges, model payloads and serialization/reconstruction remain deferred (ADR 0180).

- M8 Slice 9: RadioButton `text`/`selected` now use built-in metadata, typed access, effective
  defaults and owned snapshots. Selected writes preserve RadioGroup semantics through the public
  setter. Group-reference serialization remains unimplemented (ADR 0179).

- M8 Numeric Property Value-Kind Foundation: added `signed_integer` / `unsigned_integer` with
  fixed-width `std::int64_t` / `std::uint64_t` tooling values. The seventeen-descriptor catalog now
  covers HBox/VBox spacing, GridLayout column count/spacing and FormLayout spacing. Typed runtime
  access checks representability before conversion, delegates domain validation to semantic setters,
  and preserves same-value write success. Constant typed defaults require no Component construction;
  owned signed/unsigned snapshot collections retain complete fail-closed capture. No generic value
  carrier, serializer, deserializer or additional control coverage is introduced (ADR 0178).

- M8 Serializable UI-Description State Coverage Policy: documented the boundary between durable
  declarative configuration, structural/reference relations, style/resources, runtime interaction
  state, derived presentation state, application data and callbacks across all 26 current built-ins.
  The policy defines coverage completeness; its proposed numeric property value-kind foundation
  is now implemented separately in Slice 8. The policy slice itself added no property kinds,
  properties, serializer, persistence format or deserializer.

- M8 Built-in Component Property Snapshot foundation: added the owned, backend-neutral
  `captureBuiltInComponentPropertySnapshot(const Component&)` consumer for the currently catalogued
  built-in properties. Exact runtime type identification, inherited base-to-derived schema order,
  semantic boolean/string runtime reads and target-type-relative default comparisons are combined
  without a generic value carrier. Snapshots include all catalogued properties, fail closed instead
  of returning partial results, and explicitly do not claim complete Component state; child/tree
  serialization, persistence formats and deserialization remain deferred.

- M8 exact built-in runtime type identification foundation: added the allocation-free,
  backend-neutral `builtInComponentTypeOf(const Component&)` query. Exact dynamic built-in C++
  types return the existing immutable catalog descriptor, while unknown derived types fail closed
  with `nullptr`; internal RTTI names are never exposed or persisted. Serialization, factories and
  third-party registration remain deferred.

- M8 Component Type Metadata foundation: added stable explicit toolkit type IDs, direct base-type
  references, `component`/`widget`/`container` classification, an immutable built-in catalog and a
  backend-neutral headless consumer. Factories, serialization, resources and designer/tooling
  integration remain deferred.

- M8 Component Property Metadata schema foundation: added stable local property IDs, direct declaring
  type IDs, `boolean`/`string` value kinds, immutable built-in schema descriptors, direct-declared
  queries and inherited lookup without instance reflection or property-value access. Defaults,
  validation attributes, serialization and factories remain deferred.

- M8 typed runtime property value access foundation: added explicit boolean/string reads and writes for
  the ten built-in properties through existing semantic Component getters/setters. Descriptor identity
  is validated by value, string reads are owned, writes report access rather than value change, and
  unsupported or incompatible access fails closed. Generic value carriers, RTTI identities,
  third-party registration, serialization and design-time attributes remain deferred.

- M8 effective property default metadata foundation: added target-type-relative immutable boolean and
  string default queries with complete descriptor identity validation. Widget visibility/enabled
  defaults, interactive focusable overrides and direct Command/Label/Button/TextField/CheckBox
  defaults are represented without constructing Components or adding a `default_value` to schema
  metadata. Reset, validation, serialization and designer metadata remain deferred.

- M7 Desktop Integration exit review: native menus and the currently defined file-dialog scope are
  complete across Win32, AppKit and GTK4. The visible manual macOS/Cocoa M3 gate, native child
  peers and later desktop integrations remain explicitly open or deferred; M8 starts separately with
  the component type metadata foundation documented above.

- M7 Open File multiple-selection slice: added the separate `OpenFilesDialogRequest`,
  `requestOpenFiles()` and `OpenFilesDialogResultEvent` contract. Accepted results contain one or
  more local filesystem paths in native order, preserving duplicates without sorting or
  normalization; cancellation and failure are empty, and any unrepresentable member fails the
  whole completion atomically. Win32 uses `FOS_ALLOWMULTISELECT`/`GetResults`, AppKit uses
  `NSOpenPanel.allowsMultipleSelection`/`URLs`, and GTK uses `open_multiple`/`GListModel` with the
  older chooser multiple-selection fallback. Single Open, Save, Folder, capabilities, filters,
  initial-directory policy and shared request/pending semantics remain unchanged.

- M7 File Dialog foundation: optional backend `FileDialogService` with request IDs and queued
  application-level `FileDialogResultEvent` completion for accepted, cancelled and failed results.
- First M7 File Dialog consumer: Win32 `IFileOpenDialog` for one existing filesystem file with the
  existing HWND as owner, strict UTF-8/UTF-16 handling and no filters, save or folder semantics yet.
- Second M7 File Dialog consumer: AppKit `NSOpenPanel` as an asynchronous sheet owned by the
  existing `NSWindow`, with lifetime-safe completion state, generation/double-completion protection,
  strict UTF-8 titles and native filesystem-representation paths. Save, folder and richer selection
  options remain deferred.
- Third M7 File Dialog consumer: GTK4 Open File owned by the existing `GtkWindow`, using the
  discovered GTK API level's asynchronous `GtkFileDialog` or compatible `GtkFileChooserNative`
  response path, queued local `GFile` filesystem paths and deterministic lifetime-safe tests.
- Completed the M7 three-consumer file-dialog contract review: Win32, AppKit and GTK retain private
  native projections over the same Core request/result, pending, lifecycle and capability contract;
  Save, folder, filters and multiple selection remain deferred.
- M7 File Dialog Save File slice: added the small `SaveFileDialogRequest`/`requestSaveFile()` contract,
  Win32 `IFileSaveDialog`, AppKit `NSSavePanel` and GTK save consumers. Open and Save share request
  IDs, one pending-operation boundary and queued `FileDialogResultEvent` completion; the toolkit only
  returns a destination path and never creates or writes the file.
- M7 File Dialog Folder Selection slice: added `SelectFolderDialogRequest`/`requestSelectFolder()`
  with Win32 `IFileOpenDialog` folder picking, AppKit directories-only `NSOpenPanel` and GTK4 folder
  selection. Open, Save and Folder share IDs, one pending-operation boundary, native lifecycle
  protection and queued `FileDialogResultEvent` completion; the toolkit never mutates the filesystem
  or working directory.
- M7 Open File extension-filter slice: added one portable literal UTF-8 extension set for Open File,
  projected to Win32 `SetFileTypes`, AppKit content types and one GTK `GtkFileFilter`. Empty means
  unfiltered; malformed tokens and exact duplicates are rejected before request-ID allocation. The
  filter is presentation guidance only and does not change accepted results or validate file content.
- M7 Save File extension-filter/default-extension slice: Save reuses the ordered literal extension
  contract; the first token is a native preference where supported. Win32 uses `SetFileTypes` plus
  `SetDefaultExtension`, AppKit projects ordered content types, and GTK installs one filter without
  synthetic extension appending. Native accepted paths, overwrite confirmation, result events and
  capabilities remain authoritative and unchanged.
- M7 Save File suggested-filename slice: `SaveFileDialogRequest` now carries an optional editable
  filename-component hint. Win32 uses `SetFileName`, AppKit `nameFieldStringValue`, and GTK uses
  `set_initial_name` or its older `set_current_name` fallback. UTF-8, NUL and separator validation
  happens before request-ID allocation; filters and suggested names remain independent, and native
  accepted paths are returned unchanged without filesystem mutation.
- M7 File Dialog initial-directory slice: Open, Save and Folder requests now accept an optional
  absolute native `std::filesystem::path` start-location preference. Relative, empty and embedded-NUL
  paths are rejected before request-ID allocation; no existence check, normalization, working-directory
  mutation or result containment is performed. Win32 uses `SetDefaultFolder`, AppKit `directoryURL`,
  and GTK `set_initial_folder` or the GFile-based legacy `set_current_folder` route.

- First M7 Win32 native-menu consumer: explicit `Win32WindowBackend::syncMenuBar()` projection of the
  Core `MenuBarModel` into an owned HMENU tree, Windows-owned popup interaction, live command-state
  refresh and lifetime-safe queued `CommandActivationEvent` activation. Native callbacks do not
  execute application code; literal ampersands are escaped and menu replacement is transactional.
  AppKit/GTK native menus remain deferred.
- Second M7 native-menu consumer: `AppKitWindowBackend::syncMenuBar()` projects the same Core
  `MenuBarModel` into an application-global, owned `NSMenu` tree. AppKit uses explicit single-owner
  `mainMenu` installation/restoration, menu validation for live command state, and queued
  `CommandActivationEvent` delivery without native shortcut activation. GTK remains the next
  validation consumer.
- Third M7 native-menu consumer: `GtkWindowBackend::syncMenuBar()` projects the same Core semantics
  into a window-local GTK4 `GMenu`/`GSimpleActionGroup`/`GtkPopoverMenuBar` tree. GTK activation queues
  `CommandActivationEvent`, live command state is refreshed without structural resync, the existing
  DrawingArea remains the Rendered surface, and native accelerator routing remains deferred.
- Completed the three-consumer native-menu architecture review: the reusable portability boundary is
  `MenuBarModel`/`Command`/`CommandActivationEvent` plus backend capability, not a public NativeMenu
  hierarchy.

- M3 Rendered Desktop Preview development started.
- Separate `SASD::UI::Rendered` target with a deterministic, SDL-independent `rendered::DisplayList`.
- Initial rendered command vocabulary for filled rectangles, stroked rectangles and owned UTF-8 text.
- Cross-platform tests for command ordering, malformed geometry, ownership and package/export behavior.
- Optional per-text-command clipping bounds for safe widget-local rendering without a premature graphics-state stack.
- `RenderedPresentationSink` for Window refreshes, Label and Button presentation, including incremental repaint and conservative subtree rebuild behavior.
- `RenderedMeasurementContext` with logical line-height and deferrable Unicode-scalar boundary advances for rendered font/shaping metrics.
- Geometry-only `RenderedThemeMetrics` shared by rendered control measurement, Button/TextField chrome, TextField viewport/caret layout and click-to-caret mapping.
- Metric-aware rendered `TextField` presentation with full-run clipping, scalar-boundary horizontal viewport and insertion caret, while unsupported caret mappings remain explicitly deferred.
- `RenderDevice` and `DisplayListExecutor` execution boundary, centralizing deterministic command replay for later SDL3/native adapters without exposing their types.
- `FillRole` on rendered rectangle fills so `Color::default_color` preserves foreground-vs-background intent (notably TextField caret versus widget erasure).
- Experimental build-tree-only `SASD::UI::Rendered::SDL3` adapter using a real SDL3 software renderer and SDL_ttf for UTF-8 rendering, clipping, real font metrics and shaped caret-boundary queries.
- Dedicated headless SDL3 adapter CI with pinned SDL/SDL_ttf releases and externally supplied test font; normal toolkit builds remain SDL-independent.
- Reproducible visible Linux M3 smoke helper with pinned SDL fetch, font discovery, X11/Wayland build validation, session diagnostics and explicit human PASS gate.
- SDL_ttf FetchContent configuration no longer FORCE-overwrites a parent project's generic `BUILD_SHARED_LIBS` cache policy; CI guards the embedding invariant.
- Fetched static SDL3/SDL_ttf dependencies are built position-independent so a parent `BUILD_SHARED_LIBS=ON` policy can produce shared SASD adapter libraries without ELF relocation failures.
- SDL3 command-local text clipping now restores pre-existing renderer clip state instead of assuming an unclipped outer renderer.
- Window-backed experimental SDL3 backend with transactional lifecycle, complete-frame presentation, resize/close/key/text event translation and logical/pixel/display-scale separation.
- High-DPI regression coverage keeps pixel-size/display-scale changes presentation-only: they request replay without manufacturing logical resize events or metric revisions.
- `PresentationCoordinator::replay()` for deterministic full-tree reconstruction after native presentation-surface loss/expose without mutating semantic dirty state.
- Runnable `sasd_ui_sdl3_demo` plus end-to-end offscreen SDL window tests.
- Backend-neutral `PointerEvent`, visual-tree `HitTest` and lifetime-safe `PointerRouter` with capture assigned to the Widget that handles the press.
- Lifetime-safe root-to-target geometric hover state independent from capture, plus a geometry-neutral Rendered Button hover overlay.
- Backend-neutral `PointerSurfaceEvent` plus SDL3 window enter/leave translation; surface leave clears hover and conservatively retires semantic capture.
- Semantic `Button::isPressed()` state with capture-loss cleanup, release-inside activation / release-outside cancellation, and Terminal/Rendered pressed presentation.
- SDL3 pointer coordinate conversion and private deterministic semantic translation seam; the SDL3 backend now advertises pointer input capability.
- Shared private Rendered TextField viewport geometry plus `RenderedTextFieldHitTest` for metric-correct pointer click-to-caret mapping without leaking font/pixel state into Core.
- SDL3 integration coverage from native mouse-button representation through semantic cursor placement to the final rendered TextField caret command.
- Semantic `TextField::selectAll()` operation shared by future menu/command Select All integration and rendered triple-click complete-content selection.
- Optional GTK4 native host target `SASD::UI::Native::GTK` with Cairo/Pango RenderDevice and measurement.
- GTK4 lifecycle, GLib event pump, logical input translation and complete DisplayList redraw replay.
- Xvfb-backed GTK integration tests and a build-tree GTK demo; native peers, IME preedit and installed GTK export remain deferred.
- Optional macOS AppKit native rendered host with CoreGraphics/CoreText replay, committed text input,
  dedicated AppKit CI and a manual visible smoke-test procedure; native child peers and IME preedit
  remain deferred.
- Snapshot-exact Rendered ComboBox popup hit testing and modal pointer interaction with hover preview,
  Primary click commit, outside-press dismissal, stale-state rejection and SDL3 demo integration.
- Backend-neutral logical Latin-letter `Key` identities and deterministic `Ctrl+A/C/X/V` ShortcutMap
  bindings, with SDL3 logical-key and Terminal C0-byte translation kept separate from TextInputEvent.
- SDL3-owned UTF-8 text Clipboard service with capability/lifetime discovery, plus explicit TextField
  Select All/Copy/Cut/Paste command composition in the rendered demo and Select All in the Terminal demo.
- First Rendered menu-popup presentation slice: owned backend-neutral snapshots, one-time logical row
  placement, transactional DisplayList rendering and hit testing against the exact painted snapshot.
- Complete Rendered menu frame with persistent menu-bar presentation, nested popup placement with
  right-side/left-side fallback, transactional composition and exact-frame hit testing.
- Rendered menu pointer interaction through `MenuInteractionController`, including modal outside
  dismissal, press/release identity matching and two-phase Command activation.
- SDL3 form demo integration with Actions/Edit/Help menus, persistent menu-bar content reservation,
  F10 keyboard menu mode, overlay precedence over ComboBox/widgets and resize-safe frame rebuilding.
- M4 form and layout slices across Core, Terminal and Rendered presentation: `GridLayout`, `FormLayout`,
  `StackLayout`, `CheckBox`, `RadioButton`/`RadioGroup` and the initial `ComboBox` interaction contract.
- Cross-backend editing commands for TextField selection, including logical Ctrl+A/C/X/V shortcut
  identity and an optional UTF-8 clipboard service; richer Action, binding, validation, IME and
  clipboard-format systems remain intentionally deferred until a concrete consumer requires them.
- M5 List vertical slice: lifetime-safe backend-neutral `ListModel` observation, concrete
  `StringListModel` storage, single-row `ListSelectionModel`, virtualized Core `ListView`, and
  mutation/selection normalization tests covering destructive observers, model replacement and
  model destruction.
- Owned visible-row snapshots for Terminal and Rendered `ListView` presentation with snapshot-exact
  hit testing, stale-state rejection, UTF-8/wide-cell coverage and large-model tests proving that
  presentation queries only the visible range instead of walking the complete model.
- SDL3 rendered demo integration for `ListView`, including shared model usage, keyboard/pointer
  selection and viewport behavior. The later TableModel/TableView and TreeModel/TreeView slices are
  recorded below; delegate/cell renderer abstractions remain deferred.
- Initial M5 `TableModel` slice: a rectangular textual Core contract, lifetime-safe observation,
  concrete `StringTableModel` storage and mutation/lifetime tests; TableView presentation and
  selection were added as separate follow-up slices. Tree and delegate/cell-renderer abstractions
  were separate follow-up work; the Tree slice is now complete while delegate/cell-renderer remains
  deferred.
- Initial virtualized Core `TableView` slice: owned visible header/cell values, rectangular viewport
  normalization and large-model query-boundary tests. Table-specific selection and backend
  presentation were added as separate follow-up slices; delegates and editors remain deferred.
- Initial SDL-independent Rendered `TableView` presentation snapshot with one-time visible-lane
  measurement, owned final rectangles, transactional DisplayList composition and exact cell hit tests.
- Dedicated two-dimensional `TableSelectionModel` with lifetime-safe TableModel observation,
  bounded cell navigation and conservative row mutation normalization.
- Rendered `TableView` selection styling and revision-checked exact-snapshot selection application;
  stale presentation frames fail closed before changing semantic selection.
- Owned Terminal `TableView` presentation snapshot with UTF-8/TextMetrics cell lanes, transactional
  ScreenBuffer rendering, semantic scrolled-column hit identities and stale-revision rejection.
- Rendered `TableView` sink integration and an SDL3 form-demo consumer using the shared semantic
  `TableModel`/`TableSelectionModel` path for keyboard and exact-snapshot pointer selection without
  per-cell Widgets.
- Initial backend-neutral `TreeModel`, `TreeSelectionModel` and virtualized `TreeView` core slice:
  positional `TreeNodePath` identity, reset/content-change observation, iterative visible depth-first
  traversal, expansion/collapse normalization and large-tree query-count coverage. Terminal/Rendered
  presentation and SDL3 integration remain follow-up slices.
- Owned Terminal `TreeView` presentation with final row/disclosure geometry, portable ASCII markers,
  transactional `ScreenBuffer` rendering, exact row/indicator hit testing and stale-revision rejection.
- Owned SDL-independent Rendered `TreeView` presentation with final row/disclosure geometry,
  transactional `DisplayList` rendering, exact row/indicator hit testing and stale-revision rejection.
- `RenderedPresentationSink` now consumes the TreeView snapshot through the same public rendered
  synchronization path as ListView and TableView.
- SDL3 form demo TreeView consumer with application-owned model/selection, keyboard navigation and
  exact-snapshot pointer row selection/disclosure toggling.
- Initial M6 native-host boundary and separately linkable `SASD::UI::Native::Win32` target: one RAII-
  owned Win32 top-level window translates resize/close, keyboard, committed text and pointer messages
  into the existing Core event pipeline. Native child-control semantics, IME and accessibility remain
  deferred; Linux/macOS/Core/Terminal/Rendered builds do not include Windows SDK code.
- M5 Model/View review completed: List, Table and Tree contracts are intentionally kept explicit;
  their proven common boundary is lifetime-safe observation plus owned, revision-checked presentation
  snapshots. Delegate/cell-renderer and general binding/validation systems remain deferred because
  no current consumer requires them.

## [0.1.0] - 2026-09-28

### Added

- C++20 platform-neutral Core with `Application`, `Component`, `Widget`, `Container`, geometry,
  event routing, focus management, two-phase layout, measurement contexts and presentation
  synchronization.
- Deterministic headless/mock backend and reusable backend-contract tests.
- First-class Terminal backend with off-screen `ScreenBuffer`, ANSI/VT frame encoding, native
  POSIX and Windows terminal sessions, non-blocking input decoding and resize handling.
- M2 widgets and layouts: `Window`, `Label`, `Button`, single-line UTF-8 `TextField`,
  `VBox` and `HBox`.
- Tab/Shift+Tab focus traversal, F1-F12 key identities, Enter/Space button activation and
  backend-neutral text styling.
- Versioned terminal Unicode cell-width handling with conservative deferral for unsupported
  combining/ZWJ sequences.
- Runnable `sasd_ui_terminal_demo` used by the Linux/xterm-like and Windows Terminal manual
  release gate.
- Native process smoke tests through POSIX PTY and Windows ConPTY, plus GCC/Clang/AppleClang/MSVC
  and ASan/UBSan CI coverage.
- Installable CMake package exporting the same consumer targets as the build tree:
  `SASD::UI` and `SASD::UI::Terminal`.
- Cross-platform installed-package consumer smoke test in CI.

### Validation

- Linux/xterm-like M2 smoke test: PASS.
- Windows Terminal M2 smoke test: PASS, including `Robin AΩ界` Unicode input/editing.
- Terminal state/code-page restoration validated on all required exit paths.

### Not yet included

- Mouse/pointer input.
- Complete grapheme-cluster/emoji/ZWJ cell editing.
- Bracketed paste and extended terminal keyboard protocols.
- RGB/true-color and background/theme cascade.
- Rendered desktop backend and native desktop peers.
