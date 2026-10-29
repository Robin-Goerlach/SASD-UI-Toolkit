# Architecture

## Overview

SASD UI Toolkit separates the **semantic UI layer** from its concrete presentation. Application code works with `Window`, `Button`, `Label`, layouts, models and events. A backend decides how these concepts are implemented in a target environment.

```text
                     SASD Application
                           │
                     Public UI API
                           │
        ┌──────────────────┼──────────────────┐
        │                  │                  │
   Components            Layouts            Models
        │                  │                  │
        └──────────────────┼──────────────────┘
                           │
                      Events/Commands
                           │
                         Toolkit
                           │
                    Backend/Peer API
                           │
       ┌───────────────────┼────────────────────┐
       │                   │                    │
 Native Desktop       Rendered Desktop        Terminal
 Win32/GTK/AppKit        e.g. SDL3          ANSI/VT/Console
```

## Core architectural idea

The public UI API must not depend on a concrete platform. At the same time, the project should not pretend that every platform is identical.

This leads to three rules:

1. **Unify semantics.** A button remains a button regardless of backend.
2. **Allow different presentation.** A native Windows button and a terminal button do not have to look identical.
3. **Expose capabilities explicitly.** Platform differences are queried rather than hidden.

## Component model

Inspired by VCL and AWT, the architecture distinguishes generic components from visible widgets.

```text
Component
├── Command
├── Action
├── Timer
├── DataSource
└── Widget
    ├── Label
    ├── Button
    ├── CheckBox
    ├── TextField
    └── Container
        ├── Panel
        ├── ScrollView
        └── Window
            └── Dialog
```

`Component` is not necessarily visible. This leaves room for actions, commands, timers and data sources to share lifecycle and ownership concepts with visual components.

### Component type metadata

The Core now exposes a deliberately small type-metadata foundation through
`sasd/ui/component_metadata.hpp`. `ComponentTypeMetadata` carries an explicit stable toolkit type
ID, the direct metadata base type ID and the coarse `component`/`widget`/`container` kind. The
immutable built-in catalog describes the public Component-derived types currently implemented by
the Core and can be queried without creating instances or initializing a backend.

These IDs are not C++ RTTI names and are not localized. The catalog does not provide runtime
instance reflection, factories, serialization or a mutable plugin registry; those decisions remain
future M8 slices. Component metadata describes types only and has no effect on Component ownership,
visual parenting, events or layout.

### Exact built-in runtime identification

The separate `sasd/ui/component_runtime_metadata.hpp` layer answers which exact catalogued built-in
type a live `Component` instance has. It compares the dynamic `typeid(component)` with each known
built-in C++ type and then returns the corresponding pointer from the existing immutable catalog.
It never uses `type_info::name()`, RTTI hashes or compiler-specific names as identity. Unknown
third-party derived types fail closed with `nullptr`; they are not approximated by their nearest
built-in base type, because a future serialization consumer must not silently discard derived state.

This is an internal foundation for the next serialization-oriented slice, not serialization itself:
there is no description format, factory, deserializer, child enumeration, mutable registry or
third-party registration. `Component` remains free of a virtual `typeId()` reflection method.

### Component property metadata

The separate `sasd/ui/component_property_metadata.hpp` layer describes a small immutable schema for
selected built-in component types. `ComponentPropertyMetadata` carries a stable local property ID,
its direct declaring type ID and one of eight concrete semantic kinds: `boolean`, `string`,
`signed_integer`, `unsigned_integer`, `string_collection`, `optional_unsigned_integer`, `size_constraints` or `text_style`. Directly declared properties are queried independently from the
type catalog; inherited lookup follows the
existing direct `base_type_id` chain and returns the original declaring descriptor. No property is
duplicated into a derived type.

This remains schema metadata only. The separate `sasd/ui/component_property_access.hpp` layer
provides sixteen explicit typed read/write functions for the
currently catalogued built-ins.
It validates descriptors by value, returns owned strings, uses the existing semantic getters/setters,
and fails closed for unknown kinds, identities or incompatible Components. Writes report access
success rather than value change. The access layer uses internal safe RTTI checks, but exposes no
RTTI names or runtime type-enumeration contract. It adds no `std::variant`, `std::any`, `void*`,
getter/setter pointers, mutable registry, defaults, validation, serialization or third-party
registration, and remains backend-neutral.

M8 Slice 14 adds `Widget.sizeConstraints` as one concrete typed compound property ([ADR 0184](../adr/0184-m8-widget-size-constraints-compound-property-coverage.md)).
Access, defaults and owned snapshots reuse the public backend-neutral `SizeConstraints` value directly;
there is no generic compound abstraction. The atomic semantic setter owns range validation and
measurement invalidation; same-value access succeeds, domain exceptions propagate, and preferred
outside min/max or negative hints gain no additional restrictions. Bounds, desired size, measure
validity and parent-provided MeasureConstraints remain outside the UI Description.

M8 Slice 15 covers inline `TextStyle` on exactly Label, Button, TextField, CheckBox, RadioButton and
ComboBox ([ADR 0185](../adr/0185-m8-inline-text-style-authoring-persistence-coverage.md)). The typed `text_style`
path reuses the public value with TextStyle{} defaults, not a resource node or Widget-wide property. Setter-only writes preserve
presentation invalidation without measurement invalidation and same-value no-ops. No style inheritance
or cascade exists; backend focus/disabled/selection overlays are excluded. Fonts, RGB/alpha, themes
and shared resources remain future work. Canonical JSON v1 uses symbolic Color names, never enum ordinals (ADR 0186).
The review of all 29 built-ins established semantic readiness. Encoding, validated parsing and
built-in reconstruction are implemented; resources remain open.

### Canonical JSON description encoding

`sasd/ui/ui_description_json.hpp` sits above the owned reference graph snapshot. The capture model
remains format-neutral: JSON is only an external encoding. The dependency-free Component-root writer
captures once, emits deterministic canonical v1 with all persistent values explicit, and returns
nullopt on capture, UTF-8 or symbolic-enum failure. The writer bytes remain unchanged with the reader;
file I/O remains an application concern.
See the [format specification](UI_DESCRIPTION_FORMAT.md) and [ADR 0186](../adr/0186-m8-canonical-json-ui-description-encoding-v1.md).

### Validated description loading

The pipelines deliberately have different value types:

```text
Runtime Graph -> Capture Snapshot -> Canonical JSON v1
Canonical/valid JSON v1 -> Parser -> Validated Document -> Reconstruction -> Runtime Graph
```

The owning format-neutral document uses eight concrete property collections and the existing
reference-kind enum. Capture snapshots include is_default tooling metadata; loaded documents omit
it because it was never persisted. No default filling or recalculation occurs. The dependency-free
reader accepts cosmetic JSON/member/array order differences while rejecting unknown/duplicate
members, incomplete schemas, malformed UTF-8/escapes and inexact integers. Metadata/value-only
validation checks ownership preorder, reference type compatibility and reconstructability invariants.
No Components are constructed, no setters/callbacks run, and no resource or file layer is added.
See [ADR 0187](../adr/0187-m8-json-v1-parser-validated-document-model.md).

### Built-in description reconstruction

`sasd/ui/ui_description_reconstruction.hpp` reconstructs a complete owned built-in graph from an
already decoded `BuiltInUiDescriptionDocument`; it neither parses JSON nor performs file I/O. Because
the document is a mutable public aggregate, parser and reconstruction share one internal semantic
validator. Reconstruction validates and normalizes a private copy before creating any Component.

Construction is phased. All non-RadioButton instances are staged first, then grouped RadioButtons are
constructed through `RadioButton(RadioGroup&, std::string)`; no serialization-only `setGroup()` exists.
Command properties are restored before `Button::bindCommand()` snapshots them. SelectionModel-to-model,
view-to-model, view-to-selection-model and Button-to-Command references follow an explicit semantic
order independent of document array order. A final base-to-derived metadata pass restores every
persisted property, including the Button's independent captured values and ComboBox `items` before
`selectedIndex`.

Every instance remains in flat `std::unique_ptr` staging until semantic restoration succeeds. Only
then does `Container::adopt()` reproduce each ordered ownership list; final ownership, parenting,
reference, group-membership and selected-group invariants are checked before the root is transferred.
Failures return no partial graph. Description IDs exist only in the staging lookup and are not copied
into runtime Components, so the source and reconstructed graphs have no identity carry-over.

The internal closed construction adapter supports all 26 concrete catalog types. Exact abstract
`ListModel`, `TableModel` and `TreeModel` nodes report an unconstructible type rather than being
substituted. Concrete String models start empty because payload is application data. Fresh controls
contain no application callbacks, so normal setters are used without a suppression mode. Focus,
selection, viewport, expansion, bounds and other transient state remain excluded. See
[ADR 0188](../adr/0188-m8-built-in-ui-description-reconstruction.md).

### Effective property defaults

The separate `sasd/ui/component_property_defaults.hpp` layer describes effective defaults for a
known built-in target type through eight typed query families. Defaults are target-type-relative because inherited properties can have
different construction policies: `Widget.focusable` is false for `Widget` and structural widgets,
while the interactive built-ins override it to true. The query validates the complete property
identity against inherited schema lookup and returns `std::nullopt` for unknown or incompatible
requests. String defaults are immutable process-lifetime views; no Component is constructed to answer
the query. This layer adds no reset operation, validation, serialization or designer metadata.

### Static property validation and normalization metadata

The separate `sasd/ui/component_property_validation_metadata.hpp` layer describes instance-free,
inclusive integer domains for exact built-in property descriptors. Schema, effective defaults,
static validation metadata and typed runtime access remain distinct: the metadata consumer does not
construct a Component, while concrete setters remain the authoritative runtime enforcement boundary.
Complete declaring-type/property/kind identity is compared by value rather than catalog address.

All six signed layout spacing properties expose `[0, Coordinate::max]`; the fixed-width semantic
property type, rather than the wider `int64_t` carrier, defines the usable maximum.
`GridLayout.columnCount` exposes `[1, UINT64_MAX]`, not `[1, size_t::max]`, so portable semantic
validation does not depend on the current host. Typed property access and reconstruction continue to
reject a semantically valid value that the host cannot represent. The common format-neutral document
validator reuses these ranges, so negative or out-of-Coordinate spacing is rejected before
reconstruction. Runtime setters keep their independent checks. This is only the static integer
foundation, not a generic rule engine; see
[ADR 0189](../adr/0189-m8-static-integer-property-validation-metadata-foundation.md).

The same header now also exposes two deliberately concrete single-property policy families.
`builtInStringPropertyNormalization()` returns explicit `identity` for Command, Label, Button,
CheckBox and RadioButton text, and `single_line_utf8` for `TextField.text`.
`builtInSizeConstraintsPropertyValidation()` identifies the exact
`Widget.sizeConstraints` minimum-not-greater-than-maximum invariant. Preferred values outside the
range and negative dimensions remain valid. An engaged identity policy is distinct from an invalid
descriptor, so every future string descriptor must make a conscious choice.

These metadata queries are instance-free descriptions, not setter implementations. TextField still
normalizes through `utf8::sanitizeSingleLine()` and Widget still rejects an invalid range in its
runtime setter. The format-neutral document validator first enforces general UTF-8 validity, then
reuses exact-descriptor normalization/validation metadata and fails closed when a policy is missing.
Its former hardcoded TextField branch and implicit rule for every `size_constraints` kind are gone.

The concrete optional-unsigned cross-property metadata describes `ComboBox.selectedIndex` as an
index into the exact same-node `ComboBox.items` string-collection descriptor. Null selection remains
valid even for an empty collection. The format-neutral validator uses descriptor identity rather
than node type, vector position or input order, and compares the persistent `uint64_t` safely with
collection size. Missing metadata/dependencies fail closed. Runtime setters remain authoritative,
and reconstruction still follows declaration order independently.

Schema, effective defaults, static numeric domains, single-property normalization, atomic compound
validation, same-node cross-property validation, typed runtime access, document validation and
runtime enforcement remain separate layers. No generic validation engine or dependency DSL is
introduced. RadioGroup exclusivity and reference compatibility remain separate concrete graph rules;
see [ADR 0190](../adr/0190-m8-single-property-validation-normalization-metadata-foundation.md) and
[ADR 0191](../adr/0191-m8-combo-box-cross-property-validation-metadata-foundation.md).

### Built-in component property snapshots

The separate `sasd/ui/component_property_snapshot.hpp` layer is the first small consumer that
combines exact runtime type identification, inherited property schema, semantic runtime access and
effective defaults. `captureBuiltInComponentPropertySnapshot(const Component&)` owns the stable
type ID, property identities and captured string values in eight explicit collections: boolean, string, signed-integer, unsigned-integer,
string-collection, optional-unsigned-integer, size-constraints and text-style properties. It enumerates the exact target's metadata chain from base to derived and keeps
the existing declaration order within each collection.

The snapshot reads values only through the established runtime access layer and compares each value
with the effective default for the exact target type. It includes every property currently present
in the schema, including properties whose value is default; canonical JSON v1 also writes every value explicitly
without persisting is_default metadata. Any unknown derived type, invalid metadata chain, failed
runtime read or failed default lookup rejects the complete snapshot rather than returning a partial
description.

This is an owned snapshot of currently catalogued properties, not a complete Component or UI-tree
description. The current twenty-eight-property schema does not cover all semantic state of controls, models,
layouts, bindings or ownership/visual children. No child recursion, ownership serialization,
PropertyValue carrier, `std::variant`, `std::any`, persistence format, serializer or deserializer is
introduced here; property-schema coverage remains independently extensible in later slices.

### Serializable UI-description state coverage

ADR [0177](../adr/0177-m8-serializable-ui-description-state-coverage-policy.md) defines the
boundary for a future UI Description. A UI Description is durable declarative construction and
initial configuration, not a session snapshot, application-data snapshot, heap snapshot or
presentation/backend dump. Persistent scalar properties are only one part of the future model:
ownership, child order, visual parenting, command bindings, radio-group membership and view/model
associations are structural or reference relations and must not be forced into scalar property
metadata.

The policy explicitly excludes focus, hover, pressed/capture gestures, text editing selection and
caret state, ComboBox popup/preview state, viewports, current selections, tree expansion, layout
caches, arranged bounds, rendered snapshots and backend handles. `TextStyle` is durable
presentation authoring state, captured inline through text_style (ADR 0185), not transient state.
Future resource-backed styling remains separate.
Model contents remain application data by default; a view's model reference is a separate future
relation. A Slice-6 snapshot covers only currently catalogued properties, never complete Component
serialization coverage. A built-in becomes coverage-complete only when every relevant public
semantic state is represented by a persistent property, a defined structural/reference/resource
relation, or an explicit exclusion.

RadioButton scalar metadata/access/defaults/snapshots now cover `text` (empty string default)
and `selected` (false default), using existing string/boolean paths (ADR 0179). Writes delegate
to `setText()`/`setSelected()`, preserving group exclusivity and callbacks. RadioGroup membership
is reconstructed through the existing constructor before restoring `selected`, so
`RadioGroup::selectedButton()` remains normal derived runtime state.

The current catalog has 29 built-in types and twenty-eight property descriptors. ADR 0177 records
the coverage matrix; ADR [0178](../adr/0178-m8-numeric-property-value-kind-foundation.md) adds
numeric layout coverage. Six spacing properties (`HBox.spacing`, `VBox.spacing`, Grid/Form
`columnSpacing` and `rowSpacing`) use `signed_integer` with `std::int64_t` tooling values;
`GridLayout.columnCount` uses `unsigned_integer` with `std::uint64_t`. Both conversion directions
are range-checked; unrepresentable writes return false without mutation. Semantic setter exceptions
(negative spacing or zero columns) propagate unchanged. The runtime access layer does not replace
setter validation; the separate static metadata layer described above exposes the same usable domains
for tooling and document validation. Numeric defaults are immutable constants: spacing is zero except Form column spacing
(one), and column count is one. No Component is constructed for default queries. Numeric snapshots
own `signed_integer_properties` and `unsigned_integer_properties` with the same complete-snapshot
failure contract. Current compound authoring values are covered by ADRs 0184/0185; reference edges use their separate layer. No serializer or generic
`PropertyValue` carrier is introduced.

M8 Slice 13 adds concrete `string_collection` / `optional_unsigned_integer` kinds for ComboBox
items / selectedIndex ([ADR 0183](../adr/0183-m8-combo-box-collection-and-optional-property-coverage.md)).
Runtime reads and collection snapshots own all strings. Optional reads/defaults use two levels:
outer failure versus valid inner absence/value. Semantic setters preserve callbacks and selection
clearing; checked integer conversion precedes setter domain validation. Reconstruction traverses
metadata declaration order and therefore restores items before selectedIndex. selectedText is derived;
previewIndex/previewText/dropDownOpen/pressed and callbacks are excluded.

### Description-local identity and ownership snapshots

The separate `sasd/ui/component_ownership_snapshot.hpp` layer composes existing property snapshots
into an owned tree. Each node owns a description-local `std::uint64_t` ID, its property snapshot
(including exact type identity), and ordered owned-component IDs. Root is 0; dense IDs follow
pre-order `componentAt()` traversal, including non-visual Commands, groups and concrete models.
The explicit edge lists preserve ownership order independently of global node-vector order.
IDs are neither permanent Runtime Component identity nor application/resource identity; no stability
across captures is promised. Capture requires an unowned root. Unknown exact types, invalid properties,
repeated nodes and inconsistent owner/Widget-parent relations reject the entire snapshot. Current
Container adoption derives Widget parenting from ownership, so it is checked without a duplicate
visual-parent edge list; see [ADR 0180](../adr/0180-m8-description-node-identity-ordered-ownership-snapshot.md).

### Component reference graph snapshots

`sasd/ui/component_reference_snapshot.hpp` composes the ownership snapshot with owned edges using
description-local source/target Node IDs and eleven explicit kinds: Button -> Command, RadioButton ->
RadioGroup, List/Table/Tree View -> Model, View -> SelectionModel and SelectionModel -> Model.
Every live target must belong to the same ownership snapshot; external targets reject the entire capture. Null/unbound/expired getters produce no edge.
Pointers exist only during lookup and are never persisted. Edge order follows source nodes, then
fixed kind order. RadioGroup selection remains derived; model payloads stay Application Data.
SelectionModels are separate non-visual Components, with no generic SelectionModel hierarchy.
Normal Component ownership/identity preserves sharing through Node IDs. View and SelectionModel
model bindings remain independent; capture imposes no equality invariant. Current selected row/cell/node
and revision remain transient and have no scalar properties; see [ADR 0182](../adr/0182-m8-selection-model-component-lifecycle-and-reference-coverage.md).
Canonical JSON v1 encoding and built-in reconstruction are implemented; see
[ADR 0181](../adr/0181-m8-built-in-component-cross-reference-edge-snapshot-foundation.md) and
[ADR 0188](../adr/0188-m8-built-in-ui-description-reconstruction.md).

### Ownership

Initial core rules:

- a `Container` owns its children;
- RAII and explicit ownership should represent lifetime relationships;
- non-owning references must not silently extend lifetimes;
- callback connections must be safely disconnectable when either side is destroyed.

The exact C++ API will be validated by the first core prototype. Raw owning pointers are not a design goal.

## Toolkit and backend

The `Toolkit` bridges the semantic widget tree and platform implementation. It is responsible for global services such as:

- creation and destruction of top-level windows;
- event pump / event loop;
- backend capability discovery;
- clipboard and cursor services;
- theme and metric access;
- platform integration.

A backend may implement widgets in two ways.

### Native peers

A SASD widget is mapped to an operating-system-native control.

Examples:

- `Button` → Win32 button
- `Button` → GTK button
- `Button` → AppKit button

### Rendered peers

The backend draws the widget itself onto a drawing surface.

This is important for:

- terminal backends;
- SDL-based backends;
- widgets without a native equivalent;
- specialized controls that need consistent custom rendering.

A backend may mix both strategies.

## BackendCapabilities

Platform differences should be explicit. A backend reports capabilities such as:

```cpp
struct BackendCapabilities {
    bool mouse;
    bool clipboard;
    bool dragAndDrop;
    bool nativeMenus;
    bool multipleWindows;
    bool trueColor;
    bool accessibility;
};
```

This is conceptual, not yet a stable API.

## Event model

The event model is backend-neutral. Raw platform input is translated into semantic events.

Planned event families include:

- keyboard;
- text input;
- pointer/mouse;
- focus;
- activation/click;
- window/lifecycle;
- resize;
- command/action.

**Key events and text input are intentionally separate.** A character is not the same thing as a physical key; international keyboards, IMEs and accessibility depend on that distinction.

The initial threading model assumes a single UI thread with an event queue. Background work should explicitly dispatch results back to the UI thread rather than making all widgets arbitrarily thread-safe.

### Routing and focus in the current M1 core

The implemented M1 core deliberately separates event pumping, target selection, routing and focus:

```text
Backend
   │
   ▼
Application
   │
   ├── QuitEvent → application lifecycle
   │
   └── semantic input event
             │
             ▼
      EventTargetResolver
             │
             ▼
        target Widget
             │
             ▼
      EventDispatcher
             │
             └── Target → Parent → ... → Root
```

`EventDispatcher` performs synchronous bubbling along the **visual parent path**. A widget may consume an event with `EventResult::handled`; `ignored` offers the same event to the visual parent. Ownership relationships to non-visual components are intentionally not part of this routing path.

`FocusManager` is a separate target-selection component for logical keyboard focus. It does not own widgets; it holds exactly one non-owning observation of the currently focused widget. Widget and FocusManager detach that observation from either destruction path so no dangling pointer remains.

Widgets are **not focusable by default**. In the M1 core, local focus eligibility is `focusable && visible && enabled`. Inherited state from future `Window`/container focus-scope rules is deliberately deferred until real widgets and backends can validate those semantics.

A `FocusEvent` generated by `FocusManager` is a direct state notification to the affected widget. The focus state has already changed when the handler runs. The notification is not vetoable and is not treated as ordinary input bubbling. Normal keyboard and text input continues to use `EventDispatcher`.

For M2, `FocusTraversal` complements stateful focus management with deterministic Tab/Shift+Tab target selection. Traversal follows visual preorder/adoption order, wraps at both ends and skips hidden/disabled subtrees completely. It is deliberately an unhandled-event policy: a control may consume Tab first; only an unhandled Tab is then interpreted as focus navigation. `Application` and `EventDispatcher` therefore remain unaware of focus order.

## Layout

Layouts should not assume pixels. Terminals use cells, while desktop systems use logical units and font metrics.

### Measure/arrange contract in the M1 core

The implemented core uses a two-phase, backend-neutral layout lifecycle:

```text
Parent
  │
  ├── measure(MeasureConstraints)
  │        │
  │        ▼
  │   onMeasure(...)
  │        │
  │        ▼
  │   desiredSize
  │
  └── arrange(final Rect)
           │
           ▼
      bounds are stored
           │
           ▼
      onArrange(...)
```

`SizeConstraints` belong to the widget and describe its minimum, preferred and maximum size hints. `MeasureConstraints` come from the parent and describe only the minimum/maximum interval actually offered. The intrinsic result from `onMeasure()` is constrained first by the widget's own hints and then by the parent's constraints, so a child cannot manufacture space that the parent does not have.

`measure()` caches `desiredSize()` for identical parent constraints. In addition, `measure(context, constraints)` can use a backend-neutral `MeasurementContext`. A `Label` can therefore request text metrics without knowing terminal, font or native types. The measurement cache separates context-free and context-aware measurements and identifies a context by dynamic type plus `revision()`; Widget retains no context pointer. The first `TerminalMeasurementContext` supplies column/row metrics from the same `TextMetrics` policy used by terminal presentation. Size-relevant state changes call `invalidateMeasure()`, which propagates along the visual parent path. A future text change in a `Label`, for example, can therefore mark its `VBox` and `Window` ancestors stale as well. Adding or removing visual children also invalidates the container.

`arrange()` assigns the final logical rectangle. Bounds are stored before `onArrange()` runs so a container can use its own final geometry while arranging children. Zero-sized rectangles are valid; negative extents are rejected. Direct `setBounds()` intentionally follows the same arrangement path.

Concrete `VBox`/`HBox` allocation, margin/padding/spacing APIs and the layout effect of invisible widgets remain deliberately open. The backend boundary for text measurement is now defined through `MeasurementContext`; richer font/style metrics will be added only when additional backends demonstrate a concrete need.

The layout process should therefore work with concepts such as:

- minimum size;
- preferred size;
- maximum size;
- available constraints;
- padding, margins and spacing;
- backend-specific text and widget measurement.

Initial implemented layouts:

- `HBox`: visible children horizontally, stretching the cross axis;
- `VBox`: visible children vertically, stretching the cross axis.

Both forward the active `MeasurementContext` to children, support non-negative spacing only between visible children, and deterministically clip later children in adoption order when final main-axis space is insufficient. Margin, padding, alignment and flex weights are not part of the initial contract yet.

Further planned layouts:

- `GridLayout`
- `FormLayout`
- `StackLayout`
- optional `AbsoluteLayout` for exceptional cases

Absolute positioning should be possible but not the default model.

## Model/View for data-heavy widgets

Lists, tables and trees should eventually avoid copying all data into UI objects. A model/view separation inspired by Qt and other mature UI frameworks is planned.

```text
Application Data
      │
    Model
      │
     View
      │
 Delegate/Renderer/Editor
```

A `TableView`, for example, should be able to virtualize large datasets instead of creating millions of per-cell UI objects.

## Commands and actions

A command represents an invokable operation independently from the widget that triggers it. The same command can later be connected to a menu item, toolbar button, keyboard shortcut or command palette.

Planned properties include:

- label/text;
- enabled/disabled state;
- checked state where applicable;
- shortcut;
- execution callback.

This avoids duplicating application logic between menus, buttons and keyboard handling.

## Rendering

The core does not mandate a specific renderer. Rendered backends either receive an abstract drawing/surface layer or fully encapsulate their rendering technology behind the backend.

### Visual invalidation in the M1 core

Layout recomputation and presentation-only changes are deliberately separate:

```text
Size/content affects layout             Presentation only
             │                                  │
             ▼                                  ▼
   invalidateMeasure()                  invalidateVisual()
             │                                  │
             ▼                                  ▼
    re-measure required                 visual update pending
                                                │
                                                ▼
                                  terminal / renderer / native peer
                                                │
                                                ▼
                                  acknowledgeVisualUpdate()
```

A newly created widget initially requires visual synchronization. `invalidateVisual()` marks that state pending and propagates it along the visual parent path. Focus, enabled state and changed final geometry can therefore require presentation updates without invalidating the cached desired size. Visibility and insertion/removal of visual children affect both layout and presentation.

`acknowledgeVisualUpdate()` acknowledges only the widget that was actually processed and does not automatically clear dirty descendants.

Geometry and structural changes additionally use a stronger `presentation subtree refresh`. `arrange()` requests it when bounds change, and removing a visual child requests it on the old container. The request propagates to the presentation root. After that root synchronizes successfully, `PresentationCoordinator` also replays otherwise-clean descendants. This removes stale old positions and restores previously covered siblings without introducing dirty-rectangle/occlusion machinery into the core yet.

### PresentationCoordinator and PresentationSink

The M1 core now contains the backend-neutral bridge that consumes this pending state:

```text
Widget tree
    │
    │ dirty/clean traversal
    ▼
PresentationCoordinator
    │
    │ pending Widgets only
    ▼
PresentationSink
    │
    ├── synchronized → acknowledge Widget
    └── deferred     → Widget remains pending
```

The coordinator traverses the complete visual subtree deterministically in preorder (parent before children). Clean widgets are not sent to the sink but are still traversed so a pending descendant below an independently acknowledged parent cannot be lost. Invisible pending widgets are offered as well because removing their previous representation can itself require synchronization.

The sink receives only `const Widget&` and must not structurally mutate the visual tree during a pass. Terminal, rendered and native backends can use the same contract even though their concrete synchronization mechanisms differ completely.

Dirty rectangles, frame scheduling, clipping, display lists and concrete renderer/terminal diff algorithms deliberately remain outside this M1 contract.

An optional SDL3 backend is attractive as an early graphical proof of concept because it can provide a cross-platform window/input foundation without exposing SDL in the public SASD API.

## Text and Unicode

Unicode must be considered from the beginning. UTF-8 is the preferred starting point at public API boundaries. Platform conversions, such as UTF-16 on Windows, belong in the backend.

Terminal rendering must eventually handle display widths, combining characters and grapheme clusters. `v0.1.0` may support a narrower subset, but the architecture must not permanently assume that one byte equals one character equals one terminal cell.

With `TextField`, the core now also has a shared `sasd::ui::utf8` utility for deterministic scalar decoding. TextField maintains valid single-line UTF-8; malformed bytes normalize to U+FFFD and line/control sequences are removed for this single-line control. The current cursor is deliberately a Unicode-scalar index. Combining/ZWJ sequences are preserved but navigation is not yet grapheme-aware. Terminal viewport and hardware-caret state remain presentation concerns and are not stored in semantic TextField.

## Accessibility

Accessibility is an architectural concern, not a late add-on. Native backends can use platform services; rendered backends will eventually require semantic accessibility bridges. The core should therefore preserve semantic roles, names, states and actions independently from visual rendering details.

## Public API versus backend API

The separation is strict:

```text
include/sasd/ui/...              public API
src/core/...                     platform-neutral core
src/backends/terminal/...        terminal backend
src/backends/sdl/...             rendered desktop backend
src/backends/win32/...           native Windows backend
src/backends/gtk/...             native Linux backend
src/backends/appkit/...          native macOS backend
```

The exact source tree will be created with the first implementation milestone once the minimal interfaces have been validated.

## Stability rule

During `0.x`, API changes are allowed. They should still be explained, documented and migration-friendly where practical. A formal API/ABI policy becomes necessary before a future stable release.
