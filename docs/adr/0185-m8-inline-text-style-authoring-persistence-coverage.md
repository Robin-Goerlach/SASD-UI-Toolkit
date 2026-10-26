# ADR 0185: M8 Inline TextStyle Authoring Persistence Coverage

- Status: Accepted
- Date: 2026-10-26

## English

### Context

[ADR 0177](0177-m8-serializable-ui-description-state-coverage-policy.md) classifies TextStyle as
durable presentation authoring state, not transient interaction state. Before selecting a persisted
representation, the current built-in semantic coverage must include this existing public state.
[ADR 0025](0025-minimal-backend-neutral-text-styling.md) defines a small
backend-neutral value with foreground, bold, dim, underline and inverse, value equality, and no
pointers, handles, strings, fonts, resources or parent relationships. Its original styling decision
remains unchanged; this follow-up decides UI-description coverage, not historical M2 persistence.

### Decision

Append the concrete `text_style` kind after the seven existing value kinds, preserving their values.
Use the public `TextStyle` directly in typed access, defaults and owned snapshots, without a second
value type, generic compound carrier, variant or reflection engine.

Exactly six owning controls declare distinct `textStyle` identities, after their existing direct
properties. There is no Widget.textStyle: structural widgets and views have no such public contract.

| Declaring built-in | Direct property order |
|---|---|
| Label | text, textStyle |
| Button | text, textStyle |
| TextField | text, textStyle |
| CheckBox | text, checked, textStyle |
| RadioButton | text, selected, textStyle |
| ComboBox | items, selectedIndex, textStyle |

`readBuiltInTextStyleProperty` returns an optional owned value copy using only public getters.
`writeBuiltInTextStyleProperty` uses only existing setTextStyle setters. Both validate the complete
schema identity (declaring type, property ID, kind); copied valid descriptors work, sibling/malformed
identities fail closed. Write success means compatible access, including same-value writes, not change.
The adapter imposes no new Color validation, preserving even unknown enum values accepted at runtime.

The setter alone owns same-value no-op and visual invalidation. Real changes invalidate presentation,
not measurement, and do not alter text, selection, focusability, ownership or references.
`builtInTextStylePropertyDefault` returns TextStyle{} for the six valid target/descriptor pairs
without constructing Components: default foreground, all four flags false. There is no inheritance,
cascade, parent-style relation or theme lookup.

`ComponentTextStylePropertySnapshot` owns declaring_type_id, property_id, TextStyle value and
is_default. `BuiltInComponentPropertySnapshot.text_style_properties` is the eighth concrete collection.
Direct value equality marks defaults. A fresh Label captures three Widget booleans, default
Widget.sizeConstraints, empty Label.text and default Label.textStyle. A styled Label can capture
"Important" and bright_cyan/bold/underline with dim/inverse false and is_default false. Copies remain
valid after mutation or destruction of the source. Reference/ownership snapshots compose this value.

Only component.textStyle() is captured. Focus, disabled, pressed and selection overlays, terminal
Cell styles, rendered DrawTextCommand styles and other backend-resolved snapshots are excluded.

### Inline value and future resource boundary

Current inline TextStyle is durable Component authoring state, not a Component or resource node.
No Color/style/theme/resource IDs are introduced. Richer resource-backed styling remains separate:
font family/size/files/resources, RGB/alpha, brushes, images/icons, theme tokens, style classes,
cascade/inheritance, resource dictionaries and shared/native-theme resources are deferred. Inline
coverage does not complete the resource-system roadmap item.

A future durable encoding **must use stable symbolic Color values, never the current uint8_t or enum
ordinal**. Illustrative names are default, black, red, green, yellow, blue, magenta, cyan, white and
bright-black, bright-red, bright-green, bright-yellow, bright-blue, bright-magenta, bright-cyan,
bright-white. The encoding ADR must choose final spelling and unknown-symbol handling. This slice
adds no Color string conversion, encoding, serializer, deserializer, factory or reconstruction.

### Baseline coverage readiness review

Reviewed the current public headers of all 29 catalogued types against ADR 0177 after this change:

| Types reviewed | Durable coverage and explicit exclusions |
|---|---|
| Component, Container, Window | ownership/order via ADR 0180; visual parenting checked against adoption; Window has no own title/chrome state |
| Widget | visible/enabled/focusable and sizeConstraints; focus/hover/gestures transient, bounds/measure/dirty state derived |
| Command | text/enabled and binding edges; execution/observers executable or transient |
| Label, Button, TextField, CheckBox, RadioButton, ComboBox | existing text/bool/collection/optional values plus six inline styles; command/group references via ADR 0181; gestures/editing/popup state transient; callbacks executable |
| RadioGroup | membership from RadioButton edges; selectedButton derived |
| HBox, VBox, GridLayout, FormLayout, StackLayout | numeric configuration and ordered children; row/layer counts derived from order |
| ListModel, StringListModel, TableModel, StringTableModel, TreeModel, StringTreeModel | reference target nodes/edges; model payload including headers/text/hierarchy is application data; revisions/observers transient |
| ListSelectionModel, TableSelectionModel, TreeSelectionModel | model association via ADR 0182; selection, previous selection, revisions and subscriptions transient/application runtime |
| ListView, TableView, TreeView | model and selection-model associations via ADRs 0181/0182; viewport/current selection/expansion transient, visible rows/headers derived |

No further durable baseline state was found that lacks both technical coverage and an explicit
resource/application/executable/transient/derived classification. Current built-in UI-description
semantic coverage is ready for a persisted representation/encoding slice. This does **not** mean
serialization is complete: persisted encoding and deserialization/reconstruction remain open.
The catalog now has 29 types, 28 descriptors, 8 kinds, 16 typed access functions, 8 default query
families and 8 snapshot collections. Future public API changes must repeat the coverage review.

## Deutsch

### Kontext und Entscheidung

ADR 0177 klassifiziert TextStyle als dauerhaften Presentation-Authoring-Zustand. Diese bestehende
Lücke wird vor dem Persistenz-Encoding geschlossen. ADR 0025 bleibt die historische Styling-
Entscheidung; erst dieser Follow-up definiert UI-Description-Coverage. Der heutige TextStyle ist ein
kleiner backendneutraler, kopierbarer Value mit Value Equality und ohne Pointer, Handles, Strings,
Fonts oder Ressourcen. Er ist inline Component-Authoring-Zustand, kein Resource-Node.

Der konkrete Kind `text_style` wird an die sieben bestehenden Werte angehängt. Genau Label, Button,
TextField, CheckBox, RadioButton und ComboBox erhalten direkte textStyle-Deskriptoren nach ihren
bisherigen Properties (Reihenfolge siehe Tabelle). Kein Widget.textStyle und keine künstlichen Styles
für Views, Container, Layouts, Models, Command oder RadioGroup werden eingeführt.

Typed Read/Write und Defaults nutzen direkt den Public Value TextStyle. Vollständige Descriptor-
Identität wird geprüft; semantisch gültige Kopien funktionieren, Sibling-/Kind-/Case-Fehler werden
abgewiesen. Read liefert eine besitzende Kopie. Write delegiert ausschließlich an setTextStyle und
meldet gültigen Zugriff auch beim gleichen Wert. Keine zusätzliche Color-Validierung wird erfunden.
Nur der Setter besitzt Same-value-No-op und Visual-Invalidierung: echte Änderungen invalidieren
Presentation, nicht Measurement; Text, Selection, Focusability, Ownership und Referenzen bleiben
unverändert. Alle sechs Defaults sind TextStyle{}, ohne Component-Konstruktion.

ComponentTextStylePropertySnapshot besitzt Identität, Value und is_default; text_style_properties
ist die achte konkrete Collection. Direkte Value Equality bestimmt Defaults. Der frische Label-
Snapshot enthält Widget-Booleans, sizeConstraints, leeren Text und Default-Style. Ein Label mit
"Important" kann bright_cyan/bold/underline als nicht-default erfassen. Mutation und Zerstörung der
Quelle ändern die besitzenden Werte nicht. Backend-Overlays für Fokus/Disabled/Pressed/Selection,
Terminal-Cell-Styles und Rendered-DrawTextCommand-Styles bleiben ausgeschlossen.

### Style-/Resource-Grenze und Encoding

Keine Style-/Theme-/Resource-IDs, keine Resource-Nodes und kein Resource-System entstehen. Spätere
Font-, RGB-/Alpha-, Brush-, Image-/Icon-, Token-, Shared-Style-, Native-Theme- und Dictionary-
Ressourcen bleiben separat. Es gibt keine Cascade, Vererbung oder Parent-Style-Beziehung. Der
Resource-Roadmap-Punkt bleibt offen.

Ein dauerhaftes Color-Encoding muss stabile symbolische Werte statt uint8_t/Enum-Ordinal verwenden.
Die englische Namensliste oben ist illustrativ; finale Schreibweise und unbekannte Symbole gehören
in das spätere Encoding-ADR. Dieser Slice ergänzt keine Color-String-API, keinen Serializer,
Deserializer, Persistenzformat, Factory oder Rekonstruktion.

### Baseline-Review und Folgen

Die öffentlichen APIs aller 29 Built-ins wurden gegen ADR 0177 abgeglichen; die gruppierte Tabelle
oben dokumentiert jedes Built-in einschließlich der expliziten Exklusionen. Ownership/Referenzen und
alle heutigen dauerhaften Properties einschließlich inline TextStyle sind technisch erfasst.
Model-Payloads bleiben Application Data, Callbacks ausführbar, Interaktionszustand transient und
Layout-/Presentation-Ergebnisse abgeleitet. Keine weitere unklassifizierte dauerhafte Baseline-Lücke
wurde gefunden. Die aktuelle Built-in-Semantik ist bereit für einen Persistenzrepräsentations-/
Encoding-Slice; Serialisierung ist damit nicht abgeschlossen. Encoding und Rekonstruktion bleiben
offen, ebenso das Resource-System und das externe macOS/Cocoa-M3-Gate.

Der Katalog umfasst weiterhin 29 Typen, jetzt 28 Deskriptoren, 8 Kinds, 16 typisierte Access-Funktionen,
8 Default-Abfragefamilien und 8 Snapshot-Collections. Künftige Public-API-Erweiterungen erfordern
erneut einen Coverage-Abgleich.

## Local validation / Lokale Validierung

Strict GCC 12.2 Debug and Release builds ran in the existing Debian WSL environment with tests,
examples and warnings-as-errors enabled. Both full CTest suites passed (Core 485 cases, Terminal
321, Rendered 129, plus the native POSIX PTY smoke test). The Core runs include metadata, runtime
access, defaults, snapshots, style and measurement/visual invalidation regressions. Both headless
metadata demos passed. A staged Release install passed a six-control public style API consumer and
the repository package consumer. The 361 local Markdown links checked had no broken targets;
git diff --check passed. Automated comparison confirmed all previous descriptor identities/order
and seven kind values unchanged, 29 types unchanged, and the new 28/8/8 catalog counts. Optional
SDL3/native desktop builds were not run locally; cross-platform validation belongs to CI.

Strikte GCC-12.2-Debug-/Release-Builds liefen in der vorhandenen Debian-WSL-Umgebung mit Tests,
Beispielen und Warnings-as-Errors. Beide vollständigen CTest-Suiten bestanden (Core 485 Fälle,
Terminal 321, Rendered 129 sowie nativer POSIX-PTY-Smoke-Test). Core umfasst Metadaten, Runtime-
Zugriff, Defaults, Snapshots, Style und Measurement-/Visual-Invalidierung. Beide headless Demos,
der installierte Public-API-Consumer für alle sechs Controls und der Repository-Package-Consumer
bestanden. 361 lokale Markdown-Links hatten keine fehlenden Ziele; git diff --check war erfolgreich.
Der automatisierte Vergleich bestätigte unveränderte bisherige Deskriptor-Identitäten/-Reihenfolge,
die sieben bisherigen Kind-Werte, weiterhin 29 Typen und die neuen Katalogzahlen 28/8/8. Optionale
SDL3-/Native-Desktop-Builds liefen lokal nicht; die plattformübergreifende Validierung erfolgt in CI.
