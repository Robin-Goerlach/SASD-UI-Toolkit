# ADR 0188: M8 Built-in UI Description Reconstruction

- Status: Accepted
- Date: 2026-10-27

## English

### Context

[ADR 0187](0187-m8-json-v1-parser-validated-document-model.md) adds strict JSON v1 parsing into an
owning format-neutral `BuiltInUiDescriptionDocument`. Parsing success deliberately does not promise
that every known schema type is constructible on a current runtime or that every portable value is
representable by the host's concrete C++ API. The remaining baseline needs to turn that document into
one completely owned live Component graph without coupling construction back to JSON or file I/O.

Construction and reference restoration cannot be split into independently complete public slices.
`RadioButton` establishes `RadioGroup` membership only through
`RadioButton(RadioGroup&, std::string)` and intentionally has no `setGroup()`. Conversely,
`Button::bindCommand()` immediately copies Command text/enabled, while a captured Button may legally
hold later local values. A correct reconstruction therefore needs constructor-only relations,
reference side effects and final property truth in one ordered transaction.

### Decision

Add `reconstructBuiltInUiDescription(const BuiltInUiDescriptionDocument&)` in the public
`sasd/ui/ui_description_reconstruction.hpp` Core API. It returns an owning
`std::unique_ptr<Component>` root plus a stable error code and relevant description node ID. Success
has a non-null root and `none`; failure has no root. Allocation/system failures propagate normally.
The API consumes a document only: it does not parse JSON, read files or load resources.

Because the public document is a mutable aggregate, move the parser's format-neutral semantic checks
into one internal validator used by both parsing and reconstruction. Reconstruction validates and
normalizes a private document copy before any runtime mutation. The parser keeps its existing error
codes and maps validator locations back to the original byte offsets.

Reconstruction uses these explicit phases:

1. revalidate and normalize the document;
2. construct all non-RadioButton nodes, then RadioButtons;
3. restore Command properties needed before semantic bindings;
4. restore SelectionModel -> Model, View -> Model, View -> SelectionModel, then Button -> Command;
5. restore every property base-to-derived and in direct metadata declaration order;
6. adopt ownership through `Container::adopt()` in each stored child order;
7. verify exact types, ownership/parenting, all references and RadioGroup derived state, then transfer
   the root.

Grouped RadioButtons use their existing constructor with an already staged RadioGroup; ungrouped
buttons use the default constructor. No `setGroup()`, friend injection or private-state mutation is
added. `selected` is restored later through typed property access, letting normal group semantics
derive `selectedButton()`.

Command properties are restored before `Button::bindCommand()`. Binding then establishes the live
subscription and its normal initial Command snapshot. The final property pass re-applies the exact
persisted Button text/enabled values, so the captured initial state is authoritative while later
Command changes continue to synchronize the Button.

The final property pass reuses the existing eight typed property writers. Metadata supplies
base-to-derived and direct declaration order, notably ComboBox `items` before `selectedIndex`.
Unrepresentable fixed-width persisted values fail without clamp/truncation. `std::invalid_argument`
and `std::out_of_range` from semantic setters become `property_restore_failed`; allocation failures
are not swallowed.

All instances initially remain in a flat vector of `std::unique_ptr<Component>` plus non-owning node
lookup pointers. Ownership is adopted only after construction, references and properties succeed.
Moves preserve object addresses, so nested containers may receive children after the container itself
has moved into its parent. Ordered adoption preserves `componentAt()`, FormLayout pairing and
StackLayout layer order. Any failure destroys local staging; no partial root escapes.

The construction adapter is internal and explicitly covers the 26 concrete built-in catalog types.
The known abstract `ListModel`, `TableModel` and `TreeModel` types fail with
`unconstructible_component_type`; they are never replaced by String models. Concrete
StringListModel/StringTableModel/StringTreeModel instances are empty because model payload is
application data excluded by [ADR 0177](0177-m8-serializable-ui-description-state-coverage-policy.md).

Description node IDs are temporary reconstruction identity only. They are not copied into Component,
and no runtime UUID/name/description ID is introduced. References use document edges, never ownership
or sibling inference, preserving sharing and cross-branch targets. Current selection, focus, caret,
ComboBox open/preview, tree expansion, viewports, bounds, caches and backend state remain transient.

Freshly constructed built-ins have no application handlers because callbacks are not representable in
the description. Normal setters and subscriptions may therefore run without a global restoring flag,
silent setter or callback-suppression transaction. Custom components/callback-bearing resources need
a separate evidence-based contract if introduced later.

### Consequences and boundaries

The current built-in pipeline is complete for its declared state boundary:

```text
Runtime Graph -> Capture -> Canonical JSON v1 -> Parse -> Validated Document
              -> Reconstruction -> independent Runtime Graph
```

Canonical encode -> parse -> reconstruct -> encode is byte-identical for the current built-in UI
description, including all 28 properties, ordered ownership and all eleven reference kinds. Runtime
graph identity does not carry across the pipeline. Model payload and transient state remain excluded,
so their intentional absence does not change canonical bytes.

No public generic factory/registry, custom-component reflection, new JSON field/version, file API,
resource system, callback placeholder or model payload format is introduced. Design-time validation
metadata and designer-friendly properties remain later work.

### Local validation

The implementation was validated with strict warnings-as-errors Debug and Release GCC builds, the
complete configured CTest suite in both configurations, the encode/parse/reconstruct demo, an
installed public-API consumer, the repository package consumer, the existing writer/parser tests and
a tracked-document link check. A separate local Clang build could not be run because the available
WSL environment has no `clang++`; the repository CI remains responsible for Clang, AppleClang and
MSVC coverage.

## Deutsch

### Kontext

[ADR 0187](0187-m8-json-v1-parser-validated-document-model.md) ergänzt striktes JSON-v1-Parsing in
ein besitzendes formatneutrales `BuiltInUiDescriptionDocument`. Parse-Erfolg garantiert bewusst
nicht, dass jeder bekannte Schema-Typ konstruierbar oder jeder portable Wert auf der aktuellen Host-
API darstellbar ist. Die verbleibende Baseline muss dieses Dokument in einen vollständig besitzenden
Live-Component-Graphen überführen, ohne Konstruktion an JSON oder File-I/O zu koppeln.

Konstruktion und Reference-Restore sind nicht als unabhängig vollständige öffentliche Slices
trennbar. `RadioButton` stellt `RadioGroup`-Mitgliedschaft ausschließlich durch
`RadioButton(RadioGroup&, std::string)` her und besitzt absichtlich kein `setGroup()`.
`Button::bindCommand()` kopiert sofort Command-Text/-Enabled, während ein erfasster Button legal
spätere lokale Werte besitzen kann. Korrekte Rekonstruktion braucht daher Constructor-only-
Relationen, Reference-Nebenwirkungen und finale Property-Wahrheit in einer geordneten Transaktion.

### Entscheidung

Die öffentliche Core-API `reconstructBuiltInUiDescription(const BuiltInUiDescriptionDocument&)` in
`sasd/ui/ui_description_reconstruction.hpp` liefert einen besitzenden `std::unique_ptr<Component>`-
Root sowie stabilen Fehlercode und relevante Description-Node-ID. Erfolg bedeutet Root plus `none`,
Fehler bedeutet kein Root. Allocation-/Systemfehler propagieren. Die API konsumiert nur ein Dokument;
sie parst kein JSON, liest keine Dateien und lädt keine Resources.

Da das öffentliche Dokument ein veränderliches Aggregate ist, wird die formatneutrale semantische
Parser-Validierung in einen gemeinsamen internen Validator verschoben. Reconstruction validiert und
normalisiert eine private Kopie vor jeder Runtime-Mutation. Der Parser behält seine Fehlercodes und
bildet Validator-Orte auf die bisherigen Input-Byteoffsets ab.

Die expliziten Phasen sind:

1. Dokument erneut validieren und normalisieren;
2. alle Non-RadioButtons, danach RadioButtons konstruieren;
3. Command-Properties vor semantischen Bindings restaurieren;
4. SelectionModel -> Model, View -> Model, View -> SelectionModel, dann Button -> Command;
5. alle Properties Base-to-Derived und in direkter Metadaten-Deklarationsreihenfolge restaurieren;
6. Ownership durch `Container::adopt()` in jeder gespeicherten Child-Reihenfolge herstellen;
7. exakte Typen, Ownership/Parenting, References und abgeleiteten RadioGroup-Zustand prüfen und erst
   dann den Root übertragen.

Gruppierte RadioButtons verwenden den vorhandenen Konstruktor mit bereits gestagter RadioGroup;
ungrouped Buttons den Default-Konstruktor. Kein `setGroup()`, friend injection oder private Mutation.
`selected` wird später über Typed Property Access restauriert; normale Gruppensemantik leitet
`selectedButton()` ab.

Command-Properties werden vor `Button::bindCommand()` restauriert. Binding stellt Subscription und
normalen initialen Command-Snapshot her. Der finale Property-Pass schreibt anschließend die exakten
persistierten Button-Text-/Enabled-Werte zurück: Der gespeicherte Initialzustand bleibt autoritativ,
spätere Command-Änderungen synchronisieren den Button weiterhin.

Der finale Pass verwendet die acht vorhandenen Typed Property Writer. Metadaten liefern Base-to-
Derived- und direkte Deklarationsreihenfolge, insbesondere ComboBox `items` vor `selectedIndex`.
Nicht darstellbare Fixed-width-Werte scheitern ohne Clamp/Truncation. `std::invalid_argument` und
`std::out_of_range` der semantischen Setter werden `property_restore_failed`; Allocation-Fehler
werden nicht unterdrückt.

Alle Instanzen bleiben zunächst in einem flachen Vektor aus `std::unique_ptr<Component>` plus
non-owning Node-Lookup. Ownership folgt erst nach erfolgreicher Konstruktion, References und
Properties. Moves erhalten Objektadressen, deshalb können verschobene Nested Container später eigene
Children adoptieren. Geordnete Adoption erhält `componentAt()`, FormLayout-Pairing und StackLayout-
Layering. Bei Fehler zerstört lokales Staging alles; kein partieller Root entkommt.

Der interne Construction Adapter deckt explizit die 26 konkreten Built-in-Katalogtypen ab. Die
bekannten abstrakten Typen `ListModel`, `TableModel` und `TreeModel` scheitern mit
`unconstructible_component_type`; keine Substitution durch String-Models. Konkrete String-Models sind
leer, weil Model-Payload gemäß [ADR 0177](0177-m8-serializable-ui-description-state-coverage-policy.md)
ausgeschlossene Application Data ist.

Description-Node-IDs sind nur temporäre Reconstruction-Identität. Sie werden nicht in Components
gespeichert; keine Runtime-UUID/name/description ID. References folgen ausschließlich Document-Edges,
nicht Ownership-/Sibling-Inferenz, und erhalten Sharing sowie Cross-Branch-Targets. Selection, Fokus,
Caret, ComboBox Open/Preview, Tree Expansion, Viewports, Bounds, Caches und Backend-State bleiben
transient.

Fresh Built-ins besitzen keine Application Handler, weil Callbacks nicht in der Description stehen.
Normale Setter/Subscriptions benötigen daher weder globales Restoring Flag noch Silent Setter oder
Callback-Suppression-Transaktion. Custom Components bzw. callback-bearing Resources benötigen bei
späterer Einführung einen eigenen evidenzbasierten Vertrag.

### Folgen und Grenzen

Die aktuelle Built-in-Pipeline ist innerhalb ihrer definierten State-Grenze vollständig:

```text
Runtime Graph -> Capture -> Canonical JSON v1 -> Parse -> Validated Document
              -> Reconstruction -> unabhängiger Runtime Graph
```

Kanonisches Encode -> Parse -> Reconstruct -> Encode ist für die heutige Built-in-Description mit
allen 28 Properties, geordneter Ownership und elf Reference-Kinds byte-identisch. Runtime-Identität
wird nicht übertragen. Model-Payload und transienter Zustand bleiben ausgeschlossen; ihr bewusstes
Fehlen verändert kanonische Bytes nicht.

Keine öffentliche Generic Factory/Registry, Custom-Component-Reflection, neues JSON-Feld/Version,
File-API, Resource-System, Callback-Placeholder oder Model-Payload-Format. Design-Time-Validation-
Metadaten und designerfreundliche Properties bleiben spätere Arbeit.

### Lokale Validierung

Die Implementierung wurde mit strikten Warnings-as-errors-Debug- und -Release-GCC-Builds, der
vollständigen konfigurierten CTest-Suite in beiden Konfigurationen, dem Encode/Parse/Reconstruct-Demo,
einem installierten Public-API-Consumer, dem Repository-Package-Consumer, den bestehenden Writer-/
Parser-Tests und einer Linkprüfung der getrackten Dokumentation validiert. Ein separater lokaler
Clang-Build war nicht möglich, weil die verfügbare WSL-Umgebung kein `clang++` besitzt; Clang-,
AppleClang- und MSVC-Abdeckung bleibt Aufgabe der Repository-CI.
