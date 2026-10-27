# ADR 0177: M8 Serializable UI-Description State Coverage Policy

- Status: Accepted
- Date: 2026-10-23

## Predecessors

- [0171](0171-m8-component-type-metadata-foundation.md)
- [0172](0172-m8-component-property-metadata-schema-foundation.md)
- [0173](0173-m8-typed-runtime-property-value-access-foundation.md)
- [0174](0174-m8-effective-property-default-metadata-foundation.md)
- [0175](0175-m8-exact-built-in-runtime-type-identification-foundation.md)
- [0176](0176-m8-built-in-component-property-snapshot-foundation.md)

## English

### Context and definition

M8 Slice 6 provides an owned snapshot of every property currently present in the immutable
property catalog. That snapshot is deliberately not a complete Component state and is not a
persistence format. Before adding property kinds or choosing an encoding, the toolkit needs a
reviewable definition of which public state belongs to a durable UI description.

A UI Description is the durable declarative construction and initial configuration of a UI. It is
intended to reconstruct a logically equivalent configured UI. It is not a session snapshot,
application-state snapshot, savegame, heap snapshot, widget memento, presentation frame or backend
state dump. Reconstruction need not restore current interaction state.

The current built-in catalog contains 29 types: `Component`, `Widget`, `Container`, `Window`,
`Command`, `Label`, `Button`, `TextField`, `CheckBox`, `RadioButton`, `RadioGroup`, `ComboBox`,
`HBox`, `VBox`, `GridLayout`, `FormLayout`, `StackLayout`, `ListModel`, `StringListModel`,
`ListSelectionModel`, `ListView`, `TableModel`, `StringTableModel`, `TableSelectionModel`,
`TableView`, `TreeModel`, `StringTreeModel`, `TreeSelectionModel` and `TreeView`.

### Decision

The baseline policy has seven categories:

1. **Persistent property**: durable component configuration or semantic initial state. It belongs
   in the UI Description even when typed metadata support is added later.
2. **Persistent structural/reference relation**: ownership, child order, visual parenting, command
   binding, group membership and model/reference relationships. These need identities and graph/tree
   reconstruction rules; they are not scalar properties.
3. **Presentation/style/resource state**: current inline `TextStyle` is durable presentation
   authoring value, technically covered by `text_style` (ADR 0185). Future resource-backed styling
   remains separate and is not part of the current baseline snapshot.
4. **Transient runtime/interaction state**: focus, hover, pressed/capture gestures, editing
   caret/selection, popup state, viewport/navigation, current selection and tree expansion. These
   are rebuilt during normal runtime and are excluded from a static UI Description.
5. **Derived layout/presentation state**: arranged bounds, desired sizes, measurement validity and
   caches, dirty flags, popup geometry, rendered snapshots, terminal frames and native handles.
   These are recomputed from declarative state and backend capabilities.
6. **Application data**: model contents are not silently included. A model reference and model
   payload are different concerns; a later data/resource policy may define explicit persistence for
   `StringListModel`, `StringTableModel` or `StringTreeModel`.
7. **Executable behavior/callbacks**: function objects, handlers and observers are not ordinary
   persistent values. No function-pointer or invented symbolic callback serialization is allowed.

Ownership and visual parenting remain distinct: a `Container` owns adopted components, while only
`Widget` children are visually parented. Child order is structural state. Property metadata must
not become a universal reflection or object-graph system.

### Coverage matrix

“Future mechanism” describes the layer that must eventually cover the state; it does not authorize
implementation in this slice.

| Built-in / state | Classification | Description | Current technical coverage | Future mechanism / deferral |
|---|---|---:|---|---|
| `Component` ownership | persistent structural relation | yes | ordered ownership snapshot foundation (ADR 0180) | description-local node identities; reconstruction still open |
| `Widget.visible`, `enabled`, `focusable` | persistent property | yes | bool, supported | existing scalar schema/access |
| `Widget.sizeConstraints` | persistent compound property | yes | size_constraints, supported (ADR 0184) | existing typed compound access/default/snapshot |
| `Widget.parent` | structural relation | yes | checked adoption invariant (ADR 0180) | derived from current ownership adoption; no duplicate edge list |
| `Widget.hasFocus`, pointer-over, measure validity, desired size, dirty flags | transient/derived | no | no | runtime and presentation rebuild |
| `Widget.bounds` | derived layout state | no baseline | no | `arrange()` output; future free-form layout may define authoring geometry |
| `Window` own title/chrome state | none currently | no | no | source has no such property; do not invent one |
| `Container` adopted component order | persistent structural relation | yes | ordered ownership snapshot foundation (ADR 0180) | componentAt order includes non-visual Components |
| `Command.text`, `enabled` | persistent property | yes | string/bool, supported | existing scalar schema/access |
| `Command` execution callback and observer state | executable/transient | no | no | callbacks are not serializable values |
| `Label.text` | persistent property | yes | string, supported | existing scalar schema/access |
| `Label.textStyle` | durable inline presentation authoring value | yes | text_style, supported (ADR 0185) | richer resource/theme system remains separate |
| `Button.text` | persistent property | yes | string, supported | existing scalar schema/access |
| `Button` -> `Command` binding | persistent reference relation | yes | reference edge snapshot (ADR 0181) | self-contained live target |
| `Button.textStyle` | durable inline presentation authoring value | yes | text_style, supported (ADR 0185) | richer resource/theme system remains separate |
| `Button.pressed`, pointer gesture | transient interaction | no | no | capture/gesture state is rebuilt |
| `Button.onActivated` | executable behavior | no | no | no function persistence |
| `TextField.text` | persistent property | yes | string, supported | existing scalar schema/access |
| `TextField.textStyle` | durable inline presentation authoring value | yes | text_style, supported (ADR 0185) | richer resource/theme system remains separate |
| `TextField` cursor, selection, pointer gesture, clipboard operation | transient interaction | no | no | current editing/session state is rebuilt |
| `CheckBox.text`, `checked` | persistent property | yes | string/bool, supported | existing scalar schema/access |
| `CheckBox.textStyle` | durable inline presentation authoring value | yes | text_style, supported (ADR 0185) | richer resource/theme system remains separate |
| `CheckBox` pressed/gesture and callback | transient/executable | no | no | runtime gesture and callbacks excluded |
| `RadioButton.text`, `selected` | persistent property | yes | string/bool, supported (ADR 0179) | scalar metadata/access/default/snapshot; grouped reconstruction order still required |
| `RadioButton` -> `RadioGroup` membership | persistent reference relation | yes | reference edge snapshot (ADR 0181) | self-contained live target |
| `RadioButton.textStyle` | durable inline presentation authoring value | yes | text_style, supported (ADR 0185) | richer resource/theme system remains separate |
| `RadioButton` selected callback | executable | no | excluded | no callback persistence |
| `RadioButton` pressed/gesture | transient interaction | no | no | rebuilt by runtime routing |
| `RadioGroup` membership | persistent structural/reference relation | yes | RadioButton membership edges (ADR 0181) | `selectedButton` is derived, not independently persisted |
| `RadioGroup.selectedButton` | derived group state | no as independent value | no | derive from membership; avoid two truths |
| `ComboBox.items` | persistent owned collection | yes | string_collection, supported (ADR 0183) | owned typed access/default/snapshot |
| `ComboBox.selectedIndex` | persistent optional scalar | yes | optional_unsigned_integer, supported (ADR 0183) | future reconstruction: items before selectedIndex |
| `ComboBox.selectedText` | derived | no | no | derive from items and selected index |
| `ComboBox` preview/open/pressed/pointer gesture | transient interaction | no | no | popup/capture state is rebuilt |
| `ComboBox.textStyle` | durable inline presentation authoring value | yes | text_style, supported (ADR 0185) | richer resource/theme system remains separate |
| `ComboBox` callbacks | executable | no | excluded | no callback persistence |
| `HBox.spacing`, `VBox.spacing` | persistent property | yes | signed_integer, supported (ADR 0178) | existing scalar schema/access |
| `GridLayout.columnCount`, `columnSpacing`, `rowSpacing` | persistent properties | yes | unsigned_integer / signed_integer, supported (ADR 0178) | existing scalar schema/access |
| `FormLayout.columnSpacing`, `rowSpacing` | persistent properties | yes | signed_integer, supported (ADR 0178) | existing scalar schema/access; rows derive from child order |
| `StackLayout` layer order | persistent structural relation | yes | ordered ownership snapshot foundation (ADR 0180) | component/adoption order; no scalar configuration currently |
| `ListModel`/`TableModel`/`TreeModel` contract | reference target, not payload | no as property | target nodes/edges (ADRs 0180/0181) | exact built-in type; no model payload |
| SelectionModels: current selected row/cell/node, previous selection, revision, observers/subscriptions | transient/application runtime | no | excluded | associations persist independently; no selection properties |
| String model contents | application data | no baseline | no | later data/resource policy; never implicit layout data |
| `ListView` -> `ListModel` | persistent reference relation | yes | reference edge snapshot (ADR 0181) | self-contained live target |
| `ListView` -> `ListSelectionModel` | persistent reference relation | yes | reference edge snapshot (ADR 0182) | shared Component target; no current selection |
| `ListSelectionModel` -> `ListModel` | persistent reference relation | yes | reference edge snapshot (ADR 0182) | independent model binding; self-contained live target |
| `ListView` viewport/current selection | transient/application runtime | no | no | navigation and selection are session/application state |
| `TableView` -> `TableModel` | persistent reference relation | yes | reference edge snapshot (ADR 0181) | self-contained live target |
| `TableView` -> `TableSelectionModel` | persistent reference relation | yes | reference edge snapshot (ADR 0182) | shared Component target; no current selection |
| `TableSelectionModel` -> `TableModel` | persistent reference relation | yes | reference edge snapshot (ADR 0182) | independent model binding; self-contained live target |
| `TableView` viewport/current selected cell | transient/application runtime | no | no | navigation and selection are session/application state |
| `TreeView` -> `TreeModel` | persistent reference relation | yes | reference edge snapshot (ADR 0181) | self-contained live target |
| `TreeView` -> `TreeSelectionModel` | persistent reference relation | yes | reference edge snapshot (ADR 0182) | shared Component target; no current selection |
| `TreeSelectionModel` -> `TreeModel` | persistent reference relation | yes | reference edge snapshot (ADR 0182) | independent model binding; self-contained live target |
| `TreeView` viewport/current selection/expanded paths | transient/application runtime | no | no | navigation, selection and expansion are session state |

The model contract itself is not a property payload. View-to-model references are captured by the
ADR-0181 graph, while model contents remain application data unless a separate resource
contract says otherwise.

Slice 10 adds description-local node identity and the ordered ownership snapshot foundation
([ADR 0180](0180-m8-description-node-identity-ordered-ownership-snapshot.md)). Visual parenting stays
conceptually distinct: current Container adoption derives it, and capture checks consistency without
storing duplicate edges. Slice 11 captures Button -> Command, RadioButton -> RadioGroup and all three
View -> Model relations as self-contained live reference edges
([ADR 0181](0181-m8-built-in-component-cross-reference-edge-snapshot-foundation.md)).
Slice 12 makes SelectionModels non-visual Components and captures View -> SelectionModel and
SelectionModel -> Model using the same Node IDs ([ADR 0182](0182-m8-selection-model-component-lifecycle-and-reference-coverage.md)).
Sharing and independent model bindings are preserved; current selection remains transient.
RadioGroup.selectedButton remains derived; model payloads remain application data. Neither property
nor reference/ownership snapshot success implies complete UI-description coverage or reconstruction.

Slice 13 covers items and selectedIndex ([ADR 0183](0183-m8-combo-box-collection-and-optional-property-coverage.md)).
Future reconstruction must restore ComboBox.items before selectedIndex; this dependency is only
documented, not implemented. selectedText remains derived, preview/open/pressed transient.

### Future value-kind consequences

The Slice-7 baseline needed only `boolean` and `string`. Slice 8 now supports `signed_integer`
(`std::int64_t`) and `unsigned_integer` (`std::uint64_t`) for the seven layout properties in the
matrix; see [ADR 0178](0178-m8-numeric-property-value-kind-foundation.md). The original review
identified the following additional kinds (the first four are now implemented; see ADR 0183):

- signed integral / coordinate: `HBox`/`VBox` spacing and Grid/Form spacing;
- unsigned integral: `GridLayout.columnCount`;
- optional unsigned integral: `ComboBox.selectedIndex`;
- string collection: `ComboBox.items`;
- references/graph edges: ownership/children, Button-to-Command, RadioButton-to-RadioGroup and
  view-to-model, view-to-selection-model and selection-model-to-model (ADR 0182).

No generic `PropertyValue`, `std::variant` or `std::any` is introduced. Scalar/owned-value
properties use the typed property schema; references, resources and application data
need their separate architectural layers.

Slice 14 covers the last identified compound property, Widget.sizeConstraints, with the existing
public SizeConstraints value and concrete size_constraints access/default/snapshot (ADR 0184).
The public API review of all 29 built-ins found no further baseline persistent collection/compound
gaps. Slice 15 now covers current inline TextStyle via text_style (ADR 0185). Bounds, desiredSize and measure validity
remain derived; MeasureConstraints is parent runtime input, not a Widget property.

### Inline style boundary and current readiness (Slice 15)

[ADR 0185](0185-m8-inline-text-style-authoring-persistence-coverage.md) closes current inline TextStyle coverage
before encoding: this pointer-free public value needs no resource node or style identity. The six
owning controls use public getters/setters, TextStyle{} defaults and owned value snapshots.
Backend focus/disabled/selection overlays remain excluded. Future fonts, images, shared styles,
theme tokens, RGB/alpha and resource dictionaries remain a separate open resource-system concern.
Future durable Color encoding must be symbolic, never the current enum ordinal; spelling and
unknown-value handling belong to the encoding ADR, with no Color string API introduced here.

A fresh review of all 29 built-in public APIs (documented in ADR 0185) found no durable state missing
both technical coverage and an explicit resource/application/executable/transient/derived category.
Current built-in UI-description semantic coverage is ready for a persisted representation/encoding
slice. [ADR 0186](0186-m8-canonical-json-ui-description-encoding-v1.md) now provides persisted representation/encoding.
[ADR 0187](0187-m8-json-v1-parser-validated-document-model.md) completes JSON v1 parsing and validated
format-neutral document loading. Live Component reconstruction and the resource system remain open.

### Coverage-complete rule

A built-in is **baseline UI-Description coverage complete** only when every relevant public
semantic state has exactly one outcome: **A**, a defined persistent property; **B**, a defined
persistent structure/reference/resource relation; or **C**, an explicit exclusion from UI
Description (for example transient, derived, executable or application data).

Slice-6 snapshot success is weaker: it proves only that every property currently catalogued for the
exact built-in was captured. It must never be reported as serialization completeness. Future
property or relation additions must update this matrix or explicitly record outcome C; there must
be no silent coverage gaps.

### Consequences and next step

The policy keeps a static UI description distinct from a session/application snapshot and prevents
transient interaction state from leaking into persistence. It also prevents property metadata from
becoming an accidental object-graph reflection system.

Serialization remains open. At the Slice-7 baseline, the recommended next technical slice was
**M8 Slice 8 – Numeric Property Value-Kind Foundation**, starting with existing layout consumers: `HBox`/`VBox` spacing,
`GridLayout` column count/spacing and `FormLayout` spacing. This unlocks several authoring
properties independently of reference reconstruction. RadioButton bool/string coverage was completed
separately in Slice 9 (ADR 0179); group membership is now covered by ADR 0181 and SelectionModel
associations by ADR 0182. Slice 7 itself implements no
new property kind, property, collection, compound, reference, serializer or deserializer.

## Deutsch

Der aktuelle Source-Katalog enthält 29 Typen, einschließlich ListSelectionModel,
TableSelectionModel und TreeSelectionModel; die vollständige englische Liste oben ist abgeglichen.
Slice 13 deckt ComboBox.items (string_collection) und selectedIndex (optional_unsigned_integer)
über ADR 0183 ab. Zukünftige Rekonstruktion muss items vor selectedIndex setzen; nur dokumentiert,
nicht implementiert. selectedText bleibt abgeleitet, Preview/Open/Pressed transient.

### Kontext und Definition

M8 Slice 6 liefert einen besitzenden Snapshot jeder aktuell im unveränderlichen Property-Katalog
vorhandenen Property. Dieser Snapshot ist ausdrücklich kein vollständiger Component-Zustand und
kein Persistenzformat. Vor weiteren Value-Kinds oder einer Entscheidung für JSON, XML oder ein
anderes Encoding muss festgelegt werden, welcher öffentliche Zustand zu einer dauerhaften
UI-Beschreibung gehört.

Eine UI Description beschreibt dauerhafte deklarative Konstruktion und Initialkonfiguration einer
UI. Sie soll eine logisch gleich konfigurierte UI rekonstruieren können. Sie ist kein Session-
Snapshot, Application-State-Snapshot, Savegame, Heap-Snapshot, Widget-Memento, Presentation-Frame
oder Backend-State-Dump. Der aktuelle Interaktionszustand muss nicht wiederhergestellt werden.

### Entscheidung

Die Policy trennt sieben Kategorien: (1) persistente Properties für dauerhafte Konfiguration und
semantischen Initialzustand; (2) persistente strukturelle/Referenz-Beziehungen für Ownership,
Child-Reihenfolge, visuelles Parenting, Command-Binding, Group-Mitgliedschaft und Model-Referenzen;
(3) dauerhafte inline Presentation-Authoring-Werte wie `TextStyle`, jetzt durch text_style
erfasst (ADR 0185); zukünftiges Resource-basiertes Styling bleibt separat; (4) transiente Runtime-/Interaktionszustände wie Fokus, Hover,
Pressed-/Capture-Gesten, Caret/Selection, Popup, Viewport, aktuelle Selection und Tree-Expansion;
(5) abgeleitete Layout-/Presentation-Zustände wie Bounds, Desired Size, Measurement-Caches,
Dirty-Flags, Popup-Geometrie, Frames, Snapshots und Native Handles; (6) Application Data,
insbesondere Model-Inhalte, die nicht still in die UI-Beschreibung gelangen; und (7) ausführbares
Verhalten wie Function-Objekte, Callbacks und Observer-Implementierungszustand.

Ownership und visuelles Parenting bleiben verschieden: Ein `Container` besitzt angenommene
Components, nur `Widget`-Kinder werden visuell geparentet. Child-Reihenfolge ist struktureller
Zustand. Der Property-Mechanismus wird nicht zu einer universellen Reflection- oder Objektgraph-
Schicht erweitert.

### Coverage-Matrix und Value-Kinds

Slice 10 liefert beschreibungslokale Node-Identität und die geordnete Ownership-Snapshot-Grundlage
([ADR 0180](0180-m8-description-node-identity-ordered-ownership-snapshot.md)). componentAt-Reihenfolge
umfasst non-visuelle Components. Visuelles Parenting bleibt konzeptionell getrennt; aktuelle
Container-Adoption leitet es ab und Capture prüft Konsistenz ohne doppelte Edges. Slice 11 erfasst
Button -> Command, RadioButton -> RadioGroup und alle drei View -> Model als selbständig geschlossene
lebende Reference-Edges ([ADR 0181](0181-m8-built-in-component-cross-reference-edge-snapshot-foundation.md)).
Slice 12 erfasst SelectionModels als non-visuelle Components und View -> SelectionModel sowie
SelectionModel -> Model über dieselben Node-IDs ([ADR 0182](0182-m8-selection-model-component-lifecycle-and-reference-coverage.md)).
Sharing und unabhängige Model-Bindings bleiben erhalten; aktuelle Selection bleibt transient.
RadioGroup.selectedButton bleibt abgeleitet, Model-Payloads bleiben Application Data.
Erfolgreiche Property-/Ownership-Snapshots bedeuten keine vollständige UI Description/Rekonstruktion.

Die englische Matrix oben ist die verbindliche Zustandsabdeckung für alle aktuell 29 katalogisierten
Built-ins. Sie klassifiziert `Widget`-Grund-Properties, `Window` ohne erfundene Title-/Chrome-
Property, `Command`-Text/Enabled, Control-Texte und deklarative Zustände, Layout-Spacings,
`ComboBox.items`/`selectedIndex`, View-Model-Referenzen sowie Ownership und Child-Reihenfolge.
`Button`-Command-Binding und RadioGroup-Mitgliedschaft sind Referenzbeziehungen;
`RadioGroup.selectedButton` ist abgeleitet und keine zweite Wahrheit. Viewport, aktuelle Selection
und Tree-Expansion bleiben Runtime-/Session-State. Model-Inhalte bleiben Application Data;
View-Model-Referenzen sind davon getrennt. `TextStyle` ist dauerhafter inline Presentation-Authoring-Wert und durch text_style erfasst (ADR 0185).

Zusätzlich zu den bei Slice 7 unterstützten `boolean`- und `string`-Kinds ergaben sich signed
integral/Coordinate für Layout-Spacings, unsigned integral für `GridLayout.columnCount`, optional
unsigned integral für `ComboBox.selectedIndex`, eine String-Collection für `ComboBox.items`,
den inzwischen konkreten Compound-Kind `size_constraints` für `Widget.sizeConstraints` sowie References/Graph
Edges für Ownership, Bindings, Gruppen und Models. Slice 7 implementiert keinen dieser Kinds und
führt kein generisches `PropertyValue`, `std::variant` oder `std::any` ein. Slice 8 unterstützt
inzwischen `signed_integer` (`std::int64_t`) und `unsigned_integer` (`std::uint64_t`) für die sieben
Layout-Properties der aktualisierten Matrix; siehe [ADR 0178](0178-m8-numeric-property-value-kind-foundation.md).
Optionale ComboBox-Indizes und String-Collections sind durch ADR 0183 abgedeckt; ADR 0184 ergänzt
Widget.sizeConstraints als persistente Compound-Property mit typisiertem Access/Default/Snapshot
des bestehenden Public Values. Der Abgleich aller 29 Built-ins ergab keine weitere persistente
Collection-/Compound-Lücke der Baseline. Der heutige inline TextStyle ist durch ADR 0185 technisch erfasst; reichere Style-/Resource-Systeme bleiben separat.
Bounds, desiredSize und Measure-Validität bleiben abgeleitet; MeasureConstraints ist Parent-Runtime-
Input, keine Widget-Property. Reference-Edges sind durch ADRs 0181/0182 abgedeckt.

### Inline-Style-Grenze und Readiness (Slice 15)

ADR 0185 erfasst heutigen inline TextStyle als dauerhaften Authoring-Value vor dem Encoding,
ohne Resource-Node oder Style-ID. Sechs Controls nutzen öffentliche Getter/Setter, TextStyle{}-
Defaults und besitzende Snapshots. Backend-Overlays bleiben ausgeschlossen. Zukünftige Fonts,
Images, Shared Styles, Theme-Tokens, RGB/Alpha und Resource-Dictionaries bleiben ein separates,
offenes Resource-System. Dauerhaftes Color-Encoding muss symbolisch statt Enum-Ordinal sein;
Schreibweise und unbekannte Werte entscheidet erst das Encoding-ADR, ohne neue Color-String-API.

Der erneute Review aller 29 Built-in-Public-APIs (ADR 0185) fand keine weitere dauerhafte Lücke ohne
technische Abdeckung oder explizite Resource-/Application-/Executable-/Transient-/Derived-Kategorie.
Die heutige Built-in-Semantik ist für einen Persistenzrepräsentations-/Encoding-Slice bereit.
[ADR 0186](0186-m8-canonical-json-ui-description-encoding-v1.md) liefert jetzt Persistenzrepräsentation/Encoding.
[ADR 0187](0187-m8-json-v1-parser-validated-document-model.md) schließt JSON-v1-Parsing und validiertes
formatneutrales Dokumentladen ab. Live-Component-Rekonstruktion und Resource-System bleiben offen.

### Regel für vollständige Abdeckung

Ein Built-in ist für die Baseline-UI-Description erst vollständig abgedeckt, wenn jeder relevante
öffentliche semantische Zustand entweder (A) durch eine definierte persistente Property, (B) durch
eine definierte persistente Struktur-/Reference-/Resource-Beziehung oder (C) ausdrücklich als
außerhalb der UI Description klassifiziert ist. Slice-6-Snapshot-Erfolg beweist nur die Erfassung
aller aktuell katalogisierten Properties des exakten Built-ins und niemals Serialisierungs-
Vollständigkeit. Jede spätere Erweiterung muss die Matrix aktualisieren oder explizit Kategorie C
dokumentieren; stille Lücken sind unzulässig.

### Folgen und nächster Schritt

Die Serialisierung bleibt offen. Zum Stand von Slice 7 wurde **M8 Slice 8 – Numeric
Property Value-Kind Foundation** empfohlen, zunächst für die realen Layout-Consumer `HBox`/`VBox`,
`GridLayout` und `FormLayout`. Die RadioButton-Properties wurden separat in Slice 9 abgedeckt (ADR 0179);
Gruppenmitgliedschaft ist inzwischen durch ADR 0181 und SelectionModel-Assoziationen durch ADR 0182
abgedeckt. Slice 7 selbst beginnt keinen
Serializer, keine Collections, Compound-Werte, References oder Deserialisierung.

## Implementation and validation record

This is a documentation/architecture slice. It adds no public C++ API, property kind, property,
serializer, persistence format, parser, factory or deserializer. The source review was performed
against the 26-entry built-in catalog and current public headers.

The policy was implemented in commit `bd6e305218214ed924c0e119b9618b749bec2e58` on branch
`codex/m8-serializable-state-coverage-policy` and published as PR #79. The PR head remained this
commit. Local validation included `git diff --check`, a relative Markdown-link check with zero
broken links, an ADR-index check and a source re-check confirming the 26 catalogued built-ins and
ten current boolean/string properties. No local C++ build was run because no C++, CMake or test
file changed.

PR #79 passed the complete configured CI: Linux GCC, Linux Clang, Linux Clang ASan/UBSan, Linux
SDL3 adapter, Windows MSVC, macOS AppleClang, GTK4 Native, macOS AppKit Native and the Linux,
Windows and macOS Release jobs. It was merged by normal merge commit
`0f077c1f6f26a6991be2f4d3dcd66fa54b295333`. Post-merge Main CI, GTK Native CI, AppKit Native CI
and the complete Release matrix all passed for exactly that merge SHA.

The remote and local feature branches were deleted after post-merge validation, `main` was
fast-forwarded to the merge SHA, and the final working tree was clean. PRs #30, #32 and #63 were
not changed. No release or tag was created. The macOS/Cocoa M3 gate remains open. M8 Slice 8 was
recommended but not started.

## Implementierungs- und Validierungsprotokoll

Dies ist ein Dokumentations-/Architektur-Slice. Es ergänzt keine öffentliche C++-API, keinen
Property-Kind, keine Property, keinen Serializer, kein Persistenzformat, keinen Parser, keine
Factory und keinen Deserializer. Der Source-Review erfolgte gegen den 26 Einträge umfassenden
Built-in-Katalog und die aktuellen öffentlichen Header.

Die Policy wurde im Commit `bd6e305218214ed924c0e119b9618b749bec2e58` auf dem Branch
`codex/m8-serializable-state-coverage-policy` umgesetzt und als PR #79 veröffentlicht. Der finale
PR-Head blieb dieser Commit. Die lokale Validierung umfasste `git diff --check`, eine Prüfung der
relativen Markdown-Links mit null fehlerhaften Links, die Prüfung des ADR-Indexes sowie einen
erneuten Source-Abgleich mit 26 katalogisierten Built-ins und zehn aktuellen Boolean-/String-
Properties. Ein lokaler C++-Build wurde nicht ausgeführt, weil keine C++-, CMake- oder Testdatei
geändert wurde.

PR #79 bestand die vollständige konfigurierte CI: Linux GCC, Linux Clang, Linux Clang ASan/UBSan,
Linux SDL3-Adapter, Windows MSVC, macOS AppleClang, GTK4 Native, macOS AppKit Native sowie die
Linux-, Windows- und macOS-Release-Jobs. Der PR wurde mit dem normalen Merge-Commit
`0f077c1f6f26a6991be2f4d3dcd66fa54b295333` gemergt. Main CI, GTK Native CI, AppKit Native CI und
die vollständige Release-Matrix waren für exakt diesen Merge-SHA erfolgreich.

Nach der Post-Merge-Validierung wurden Remote- und lokaler Feature-Branch gelöscht, `main` auf den
Merge-SHA vorgespult und der finale Working Tree sauber bestätigt. PRs #30, #32 und #63 wurden nicht
verändert. Es wurde kein Release und kein Tag erstellt. Das macOS/Cocoa-M3-Gate bleibt offen. M8
Slice 8 wurde empfohlen, aber nicht begonnen.
