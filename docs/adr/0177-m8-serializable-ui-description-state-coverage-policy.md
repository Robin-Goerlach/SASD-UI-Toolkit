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

The current built-in catalog contains 26 types: `Component`, `Widget`, `Container`, `Window`,
`Command`, `Label`, `Button`, `TextField`, `CheckBox`, `RadioButton`, `RadioGroup`, `ComboBox`,
`HBox`, `VBox`, `GridLayout`, `FormLayout`, `StackLayout`, `ListModel`, `StringListModel`,
`ListView`, `TableModel`, `StringTableModel`, `TableView`, `TreeModel`, `StringTreeModel` and
`TreeView`.

### Decision

The baseline policy has seven categories:

1. **Persistent property**: durable component configuration or semantic initial state. It belongs
   in the UI Description even when typed metadata support is added later.
2. **Persistent structural/reference relation**: ownership, child order, visual parenting, command
   binding, group membership and model/reference relationships. These need identities and graph/tree
   reconstruction rules; they are not scalar properties.
3. **Presentation/style/resource state**: durable authoring appearance such as `TextStyle`. It
   is not transient, but belongs to a later style/resource policy.
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

| Built-in / state | Classification | Description | Current metadata | Future mechanism / deferral |
|---|---|---:|---|---|
| `Component` ownership | persistent structural relation | yes | no | ownership/tree identities and reconstruction |
| `Widget.visible`, `enabled`, `focusable` | persistent property | yes | bool, supported | existing scalar schema/access |
| `Widget.sizeConstraints` | persistent compound candidate | yes | no | compound authoring value; defer until consumer |
| `Widget.parent` | structural relation | yes | no | visual-parent/tree reconstruction |
| `Widget.hasFocus`, pointer-over, measure validity, desired size, dirty flags | transient/derived | no | no | runtime and presentation rebuild |
| `Widget.bounds` | derived layout state | no baseline | no | `arrange()` output; future free-form layout may define authoring geometry |
| `Window` own title/chrome state | none currently | no | no | source has no such property; do not invent one |
| `Container` adopted child order | persistent structural relation | yes | no | ownership/tree description |
| `Command.text`, `enabled` | persistent property | yes | string/bool, supported | existing scalar schema/access |
| `Command` execution callback and observer state | executable/transient | no | no | callbacks are not serializable values |
| `Label.text` | persistent property | yes | string, supported | existing scalar schema/access |
| `Label.textStyle` | style/resource | potentially | no | later style/resource policy |
| `Button.text` | persistent property | yes | string, supported | existing scalar schema/access |
| `Button` -> `Command` binding | persistent reference relation | yes | no | component identity/reference graph |
| `Button.textStyle` | style/resource | potentially | no | later style/resource policy |
| `Button.pressed`, pointer gesture | transient interaction | no | no | capture/gesture state is rebuilt |
| `Button.onActivated` | executable behavior | no | no | no function persistence |
| `TextField.text` | persistent property | yes | string, supported | existing scalar schema/access |
| `TextField.textStyle` | style/resource | potentially | no | later style/resource policy |
| `TextField` cursor, selection, pointer gesture, clipboard operation | transient interaction | no | no | current editing/session state is rebuilt |
| `CheckBox.text`, `checked` | persistent property | yes | string/bool, supported | existing scalar schema/access |
| `CheckBox.textStyle` | style/resource | potentially | no | later style/resource policy |
| `CheckBox` pressed/gesture and callback | transient/executable | no | no | runtime gesture and callbacks excluded |
| `RadioButton.text`, `selected` | persistent property | yes | string/bool, not supported | scalar coverage; grouped reconstruction order required |
| `RadioButton` -> `RadioGroup` membership | persistent reference relation | yes | no | group identity/reference graph |
| `RadioButton.textStyle`, selected callback | style/resource + executable | style potentially; callback no | no | style policy; callbacks excluded |
| `RadioButton` pressed/gesture | transient interaction | no | no | rebuilt by runtime routing |
| `RadioGroup` membership | persistent structural/reference relation | yes | no | membership graph; `selectedButton` is derived |
| `RadioGroup.selectedButton` | derived group state | no as independent value | no | derive from membership; avoid two truths |
| `ComboBox.items` | persistent owned collection | yes | no | future string-collection kind |
| `ComboBox.selectedIndex` | persistent optional scalar | yes | no | future optional unsigned integral kind |
| `ComboBox.selectedText` | derived | no | no | derive from items and selected index |
| `ComboBox` preview/open/pressed/pointer gesture | transient interaction | no | no | popup/capture state is rebuilt |
| `ComboBox.textStyle` and callbacks | style/resource + executable | style potentially; callbacks no | no | style policy; no callback persistence |
| `HBox.spacing`, `VBox.spacing` | persistent property | yes | signed_integer, supported (ADR 0178) | existing scalar schema/access |
| `GridLayout.columnCount`, `columnSpacing`, `rowSpacing` | persistent properties | yes | unsigned_integer / signed_integer, supported (ADR 0178) | existing scalar schema/access |
| `FormLayout.columnSpacing`, `rowSpacing` | persistent properties | yes | signed_integer, supported (ADR 0178) | existing scalar schema/access; rows derive from child order |
| `StackLayout` layer order | persistent structural relation | yes | no | child/adoption order; no scalar configuration currently |
| `ListModel`/`TableModel`/`TreeModel` contract | reference target, not payload | no as property | no | model identity/reference policy |
| String model contents | application data | no baseline | no | later data/resource policy; never implicit layout data |
| `ListView` -> `ListModel` | persistent reference relation | yes | no | model identity/reference graph |
| `ListView` -> `ListSelectionModel` | persistent reference candidate | evaluate separately | no | separate reference policy; not scalar |
| `ListView` viewport/current selection | transient/application runtime | no | no | navigation and selection are session/application state |
| `TableView` -> `TableModel` | persistent reference relation | yes | no | model identity/reference graph |
| `TableView` -> `TableSelectionModel` | persistent reference candidate | evaluate separately | no | separate reference policy; not scalar |
| `TableView` viewport/current selected cell | transient/application runtime | no | no | navigation and selection are session/application state |
| `TreeView` -> `TreeModel` | persistent reference relation | yes | no | model identity/reference graph |
| `TreeView` -> `TreeSelectionModel` | persistent reference candidate | evaluate separately | no | separate reference policy; not scalar |
| `TreeView` viewport/current selection/expanded paths | transient/application runtime | no | no | navigation, selection and expansion are session state |

The model contract itself is not a property payload. View-to-model references may become part of a
future description graph, while model contents remain application data unless a separate resource
contract says otherwise.

### Future value-kind consequences

The Slice-7 baseline needed only `boolean` and `string`. Slice 8 now supports `signed_integer`
(`std::int64_t`) and `unsigned_integer` (`std::uint64_t`) for the seven layout properties in the
matrix; see [ADR 0178](0178-m8-numeric-property-value-kind-foundation.md). The original review
identified the following additional kinds (the first two are now implemented):

- signed integral / coordinate: `HBox`/`VBox` spacing and Grid/Form spacing;
- unsigned integral: `GridLayout.columnCount`;
- optional unsigned integral: `ComboBox.selectedIndex`;
- string collection: `ComboBox.items`;
- compound structure: potentially `Widget.sizeConstraints` and `TextStyle`;
- references/graph edges: ownership/children, Button-to-Command, RadioButton-to-RadioGroup and
  view-to-model (with selection-model associations evaluated separately).

No generic `PropertyValue`, `std::variant` or `std::any` is introduced. Scalar/owned-value
properties may use the property schema later; structures, references, resources and application data
need separate architectural layers.

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
properties independently of reference reconstruction. RadioButton's missing bool/string coverage is
a focused follow-up and should not be mixed into the numeric foundation. Slice 7 implements no
new property kind, property, collection, compound, reference, serializer or deserializer.

## Deutsch

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
(3) dauerhafte Style-/Resource-Konfiguration wie `TextStyle`, die nicht transient ist, aber eine
spätere Policy benötigt; (4) transiente Runtime-/Interaktionszustände wie Fokus, Hover,
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

Die englische Matrix oben ist die verbindliche Zustandsabdeckung für alle aktuell 26 katalogisierten
Built-ins. Sie klassifiziert `Widget`-Grund-Properties, `Window` ohne erfundene Title-/Chrome-
Property, `Command`-Text/Enabled, Control-Texte und deklarative Zustände, Layout-Spacings,
`ComboBox.items`/`selectedIndex`, View-Model-Referenzen sowie Ownership und Child-Reihenfolge.
`Button`-Command-Binding und RadioGroup-Mitgliedschaft sind Referenzbeziehungen;
`RadioGroup.selectedButton` ist abgeleitet und keine zweite Wahrheit. Viewport, aktuelle Selection
und Tree-Expansion bleiben Runtime-/Session-State. Model-Inhalte bleiben Application Data;
View-Model-Referenzen sind davon getrennt. `TextStyle` ist Style/Resource und nicht transient.

Zusätzlich zu den bei Slice 7 unterstützten `boolean`- und `string`-Kinds ergaben sich signed
integral/Coordinate für Layout-Spacings, unsigned integral für `GridLayout.columnCount`, optional
unsigned integral für `ComboBox.selectedIndex`, eine String-Collection für `ComboBox.items`,
Compound-Strukturen für mögliche `Widget.sizeConstraints` und `TextStyle` sowie References/Graph
Edges für Ownership, Bindings, Gruppen und Models. Slice 7 implementiert keinen dieser Kinds und
führt kein generisches `PropertyValue`, `std::variant` oder `std::any` ein. Slice 8 unterstützt
inzwischen `signed_integer` (`std::int64_t`) und `unsigned_integer` (`std::uint64_t`) für die sieben
Layout-Properties der aktualisierten Matrix; siehe [ADR 0178](0178-m8-numeric-property-value-kind-foundation.md).
Optionale Werte, Collections, Compound-Werte und References bleiben offen.

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
`GridLayout` und `FormLayout`. Die fehlenden RadioButton-Properties bleiben ein fokussierter
Folgeschritt, werden aber nicht mit der Numeric-Grundlage vermischt. Slice 7 beginnt keinen
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
