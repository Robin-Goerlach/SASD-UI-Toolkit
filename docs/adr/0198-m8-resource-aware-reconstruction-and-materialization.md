# ADR 0198: M8 Resource-Aware Reconstruction and Materialization

- Status: Accepted
- Date: 2026-11-07

## English

### Context

ADRs 0194–0197 established stable resource keys, a typed owning `TextStyle` dictionary, typed
references, exact property-resource metadata, explicit caller-ordered resolution and a validated,
self-contained format-neutral v2 document. Reconstruction still rejected every resource-bearing
document. The missing step is to turn a validated authoring reference into the concrete runtime
property value expected by the existing six controls.

Runtime Components already own normal `TextStyle` values and their setters drive established
presentation invalidation. Retaining a reference or dictionary in a Component would introduce new
lifetime, scope-mutation, notification, cache-invalidation and property-precedence contracts that M8
does not require. Conversely, silently ignoring v2 references would discard the document's sole
authoring truth.

### Decision

Keep the existing format-neutral `reconstructBuiltInUiDescription(document)` entry point for both
resource-free v1 documents and resource-bearing v2 documents. JSON versions remain adapter concerns.
Reconstruction continues to copy and fully validate/normalize the mutable public document before
constructing the first Component. Duplicate or invalid resources, dangling/wrong-kind/wrong-property
references, missing properties and inline-plus-reference conflicts therefore fail as
`invalid_document` without exposing a partial graph.

After validation, the Reconstructor builds one private `ResourceDictionary` from
`document.text_style_resources` through `addTextStyle()`. An unexpected insertion failure is treated
as a defensive invariant violation and fails closed before construction. Unreferenced valid
resources are allowed; no use-count policy is added.

During the existing property-restore phase, inline `TextStyle` values retain the v1 path unchanged.
Otherwise the exact `UiDescriptionResourceReferenceProperty` is found, its
`ComponentPropertyResourceMetadata` is checked, and the reference is passed to the existing
`resolveTextStyleResource()` with one explicit document-local `ResourceScope` at index 0. The
resolver's owned `TextStyle` copy is written through `writeBuiltInTextStyleProperty()`. An unexpected
post-validation resolution or write failure is reported as `property_restore_failed` for that node.

This is one-time materialization. The resulting Component stores only its concrete `TextStyle`.
No `ResourceReference`, `ResourceKey`, dictionary pointer, resolver, scope chain, observer or binding
is retained. The private dictionary dies with the Reconstructor. Later dictionary changes cannot
affect the runtime graph, and controls that referenced the same resource still own independent
values.

The established reconstruction order remains validation, construction, Command-property
preparation, Component references, properties, ownership adoption and final verification. Resource
materialization belongs to the normal property phase; RadioButton/RadioGroup, Command/Button and
Model/SelectionModel/View ordering is unchanged. Staged unique ownership preserves atomic failure.

### Consequences and boundaries

All six resource-capable properties—`Label`, `Button`, `TextField`, `CheckBox`, `RadioButton` and
`ComboBox` `textStyle`—can now materialize from v2. Inline and referenced styles may coexist in one
document. JSON v1 and v2 schemas, symbols, canonical ordering and format versions are unchanged.
Resource-free v1 reconstruction remains behavior-compatible.

One-time materialization intentionally loses resource authoring identity in the runtime graph.
Calling the existing Component-root capture/writer after v2 reconstruction produces v1 with the
concrete inline `TextStyle`; it cannot recreate the original resource definitions, keys or
references. Tooling that must preserve v2 authoring identity must retain and edit the
`BuiltInUiDescriptionDocument` instead of trying to infer it from Components.

There is no live reload, theme switching, cascade, field merge, fallback, external dictionary,
Application/Window/Component resource owner, global registry or hidden scope mechanism. TextStyle
remains the only concrete resource family. Richer families remain deferred until a real backend
consumer establishes their contracts.

## Deutsch

### Kontext

ADRs 0194–0197 etablierten stabile Resource Keys, ein typisiertes besitzendes `TextStyle`-
Dictionary, typisierte References, exakte Property-Resource-Metadaten, explizite Caller-geordnete
Resolution und ein validiertes, self-contained formatneutrales v2-Dokument. Die Rekonstruktion
lehnte weiterhin jedes Resource-tragende Dokument ab. Es fehlt die Umsetzung einer validierten
Authoring-Reference in den konkreten Runtime-Property-Wert der bestehenden sechs Controls.

Runtime Components besitzen bereits normale `TextStyle`-Werte; ihre Setter steuern die etablierte
Presentation-Invalidierung. Eine dauerhaft gehaltene Reference oder ein Dictionary in einer
Component würde neue Verträge für Lifetime, Scope-Mutation, Notifications, Cache-Invalidierung und
Property-Priorität einführen, die M8 nicht benötigt. v2-References still zu ignorieren würde dagegen
die einzige Authoring-Wahrheit des Dokuments verwerfen.

### Entscheidung

Der bestehende formatneutrale Entry Point `reconstructBuiltInUiDescription(document)` bleibt für
resource-freie v1- und Resource-tragende v2-Dokumente erhalten. JSON-Versionen bleiben Aufgabe der
Adapter. Die Rekonstruktion kopiert und validiert/normalisiert das veränderliche öffentliche
Dokument weiterhin vollständig vor der ersten Component-Konstruktion. Duplicate oder ungültige
Resources, dangling/wrong-kind/wrong-property References, fehlende Properties und Inline-plus-
Reference-Konflikte schlagen daher als `invalid_document` fehl, ohne einen partiellen Graphen
auszugeben.

Nach der Validierung baut der Reconstructor über `addTextStyle()` genau ein privates
`ResourceDictionary` aus `document.text_style_resources`. Ein unerwarteter Insert-Fehler gilt als
defensive Invariantenverletzung und arbeitet vor der Konstruktion fail-closed. Gültige ungenutzte
Resources bleiben erlaubt; es entsteht keine Use-Count-Policy.

In der bestehenden Property-Restore-Phase behalten inline `TextStyle`-Werte unverändert den v1-Pfad.
Andernfalls wird die exakte `UiDescriptionResourceReferenceProperty` gefunden, ihre
`ComponentPropertyResourceMetadata` geprüft und die Reference mit genau einem expliziten
dokumentlokalen `ResourceScope` an Index 0 an `resolveTextStyleResource()` übergeben. Dessen
besitzende `TextStyle`-Kopie wird über `writeBuiltInTextStyleProperty()` geschrieben. Ein
unerwarteter Resolution- oder Write-Fehler nach erfolgreicher Validierung wird für den Node als
`property_restore_failed` gemeldet.

Dies ist einmalige Materialisierung. Die resultierende Component speichert ausschließlich ihren
konkreten `TextStyle`. Weder `ResourceReference`, `ResourceKey`, Dictionary-Pointer, Resolver, Scope
Chain, Observer noch Binding werden gehalten. Das private Dictionary endet mit dem Reconstructor.
Spätere Dictionary-Änderungen können den Runtime-Graphen nicht verändern; Controls mit derselben
Resource besitzen weiterhin unabhängige Values.

Die etablierte Reihenfolge bleibt Validierung, Konstruktion, Command-Property-Vorbereitung,
Component References, Properties, Ownership-Adoption und Abschlussprüfung. Resource-
Materialisierung gehört in die normale Property-Phase; RadioButton/RadioGroup-, Command/Button- und
Model/SelectionModel/View-Reihenfolgen bleiben unverändert. Gestagtes `unique_ptr`-Ownership erhält
atomare Fehlerbehandlung.

### Folgen und Grenzen

Alle sechs Resource-fähigen Properties—`textStyle` von `Label`, `Button`, `TextField`, `CheckBox`,
`RadioButton` und `ComboBox`—können nun aus v2 materialisiert werden. Inline und referenzierte Styles
dürfen in einem Dokument koexistieren. JSON-v1-/v2-Schemas, Symbole, kanonische Reihenfolge und
Formatversionen bleiben unverändert. Resource-freie v1-Rekonstruktion bleibt verhaltenskompatibel.

Einmalige Materialisierung verliert bewusst die Resource-Authoring-Identität im Runtime-Graphen.
Der bestehende Component-Root-Capture/Writer erzeugt nach einer v2-Rekonstruktion v1 mit dem
konkreten inline `TextStyle`; ursprüngliche Resource-Definitionen, Keys und References können nicht
rekonstruiert werden. Tooling, das v2-Authoring-Identität bewahren muss, hält und bearbeitet deshalb
das `BuiltInUiDescriptionDocument`, statt sie aus Components zurückzuerraten.

Es gibt weder Live Reload, Theme Switching, Cascade, Feld-Merge, Fallback, externe Dictionaries,
Application-/Window-/Component-Resource-Owner, globale Registry noch versteckte Scope-Mechanismen.
TextStyle bleibt die einzige konkrete Resource-Familie. Weitere Familien bleiben zurückgestellt,
bis ein realer Backend-Consumer ihre Verträge begründet.
