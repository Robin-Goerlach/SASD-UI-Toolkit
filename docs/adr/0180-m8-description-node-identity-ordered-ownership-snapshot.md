# ADR 0180: M8 UI Description Node Identity and Ordered Ownership Snapshot Foundation

- Status: Accepted
- Date: 2026-10-23
- Scope: M8 Slice 10
- Follows: [ADR 0177](0177-m8-serializable-ui-description-state-coverage-policy.md),
  [ADR 0179](0179-m8-radio-button-scalar-property-coverage.md)

## English

### Context

Persistent scalar coverage is complete for the current baseline, but property capture is not a
complete UI description. The persistent relationships classified by ADR 0177 need a shared node
identity before cross-references can name their targets. Implementing a complete reference graph
now would combine ownership, reference policy and reconstruction before that foundation is proven.

### Decision

Introduce the separate `component_ownership_snapshot` layer. `ComponentDescriptionNodeId` is a
`std::uint64_t` valid only inside one snapshot. IDs are dense 0..N-1, assigned deterministically
in pre-order ownership traversal; the root is 0. Separate captures promise no identity stability.
These are neither runtime Component IDs nor application/resource identities. Component gains no
ID, name, UUID, GUID or virtual type API. Random IDs and persisted pointer identities are unnecessary;
encodings may later choose their own representation.

`BuiltInComponentOwnershipSnapshot` owns `root_node_id` and `nodes`. Each
`ComponentOwnershipNodeSnapshot` owns `node_id`, one `BuiltInComponentPropertySnapshot` and
`owned_component_ids`. Reusing property snapshots preserves exact stable type identity, values
and effective-default markings without a second type/property truth. Ordered ownership edges are
the structural truth; consumers must use them rather than infer siblings from node-vector order.
No redundant owner ID is stored.

Capture accepts only `root.owner() == nullptr`; an embedded root would silently lose its persistent
owner relation. Unowned non-Container Components, including Command, RadioGroup and Label, are valid
single-node captures. Fragment/subtree semantics are deferred until a consumer requires them.

Traversal uses `Container::componentCount()` / `componentAt()`, never visual-child traversal.
Adoption order is persistent structural state, including non-visual Commands, RadioGroups and
concrete StringListModel/StringTableModel/StringTreeModel instances. Abstract model catalog entries
need no artificial instances. Model payloads remain application data and are not captured.

Ownership and visual parenting remain conceptually distinct. Current Container adoption derives
Widget parenting from ownership, so no second visual-parent edge list is stored. Capture checks
each owned Component's owner and each owned Widget's parent against its owning Container.

The exact-type property capture rejects unknown/third-party derived types without base-class
fallback. Any invalid property snapshot, owner/parent inconsistency or repeated/cyclic node rejects
the whole result with `std::nullopt`; no partial tree escapes. A temporary visited-pointer set checks
duplicates/cycles, but pointers never enter the snapshot or determine its IDs. Iterative traversal
avoids a recursive capture stack. Like other Core operations capture runs on the UI thread, without
concurrent mutation; normal allocation failures propagate as exceptions.

### Boundaries and consequences

This foundation is deliberately not complete UI-description coverage. Button -> Command,
RadioButton -> RadioGroup, ListView/TableView/TreeView -> their models and selection-model
associations remain open. Their future edges can use the common description-local node identity.
RadioGroup.selectedButton stays derived, never a second truth. No reference edges, model payloads,
collection/compound or ComboBox optional state, resource system, serializer/encoding, factory,
deserializer, reference resolution or reconstruction is introduced.

Tests cover standalone roots, mixed ownership order, deterministic nested pre-order, exact types,
concrete model nodes, owned-root rejection, unknown descendants and value ownership after mutation
and destruction. Public ownership APIs cannot manufacture duplicate/cyclic or mismatched owner/parent
structures; defensive rejection is implemented without adding corruption APIs or undefined-behavior
test fixtures. The headless metadata demo prints nodes and explicit ordered ownership edges.

## Deutsch

### Kontext

Die skalare Baseline-Abdeckung ist vollständig, aber ein Property-Snapshot ist keine vollständige
UI Description. Die persistenten Beziehungen aus ADR 0177 brauchen zunächst gemeinsame Node-Identität,
damit Cross-References ihre Ziele benennen können. Ein vollständiger Reference Graph würde jetzt
Ownership, Referenz-Policy und Rekonstruktion vor Erprobung dieser Grundlage vermischen.

### Entscheidung

Die separate Schicht `component_ownership_snapshot` verwendet `ComponentDescriptionNodeId` als
`std::uint64_t`, gültig ausschließlich innerhalb eines Snapshots. Dichte IDs 0..N-1 werden
deterministisch in Pre-order der Ownership vergeben; Root ist 0. Zwischen Captures besteht keine
Stabilitätsgarantie. Dies ist keine Runtime-, Application- oder Resource-Identität. Component erhält
keine ID, Namen, UUID, GUID oder virtuelle Type-API. Zufalls- und Pointer-Identitäten sind unnötig;
spätere Encodings können ihre Darstellung unabhängig wählen.

`BuiltInComponentOwnershipSnapshot` besitzt `root_node_id` und `nodes`. Jeder
`ComponentOwnershipNodeSnapshot` besitzt `node_id`, einen `BuiltInComponentPropertySnapshot` und
`owned_component_ids`. Damit werden exakter stabiler Typ, Property-Werte und Default-Markierungen
wiederverwendet, ohne Typ-/Property-Zustand zu duplizieren. Die geordneten Ownership-Edges sind die
strukturelle Wahrheit; Consumer dürfen Geschwister nicht aus der globalen Vector-Reihenfolge ableiten.
Ein redundanter Owner-ID-Wert wird nicht gespeichert.

Nur `root.owner() == nullptr` ist erlaubt. Ein eingebetteter Root würde seine persistente
Owner-Beziehung verlieren. Unowned non-Container wie Command, RadioGroup und Label sind gültige
Ein-Knoten-Captures. Fragment-/Subtree-Semantik bleibt bis zu einem konkreten Consumer offen.

Traversiert wird mit `Container::componentCount()` / `componentAt()`, nicht über visuelle Children.
Adoption Order ist persistenter struktureller Zustand, einschließlich non-visueller Commands,
RadioGroups und konkreter StringListModel/StringTableModel/StringTreeModel-Instanzen. Abstrakte
Model-Katalogeinträge benötigen keine künstlichen Instanzen. Model-Inhalte bleiben Application Data.

Ownership und visuelles Parenting bleiben konzeptionell verschieden. Die aktuelle Container-Adoption
leitet Widget-Parenting aus Ownership ab; eine zweite Parent-Edge-Liste wird deshalb nicht gespeichert.
Capture prüft Owner jedes besessenen Components und Parent jedes besessenen Widgets gegen den Container.

Exact-Type-Property-Capture lehnt unbekannte/Third-Party-Derived-Typen ohne Base-Fallback ab.
Ungültiger Property-Snapshot, inkonsistenter Owner/Parent oder mehrfach/zyklisch erreichter Node
verwerfen atomar das gesamte Ergebnis mit `std::nullopt`. Eine temporäre Pointer-Visit-Menge prüft
Duplikate/Zyklen; Pointer gelangen weder in den Snapshot noch in IDs. Iterative Traversierung
vermeidet einen rekursiven Capture-Stack. Capture läuft im UI-Thread ohne konkurrierende Mutation;
normale Allokationsfehler propagieren als Exceptions.

### Grenzen und Folgen

Button -> Command, RadioButton -> RadioGroup, ListView/TableView/TreeView -> Model und SelectionModel-
Assoziationen bleiben offen. Spätere Edges können die gemeinsame beschreibungslokale Node-Identität
nutzen. RadioGroup.selectedButton bleibt abgeleitet und keine zweite Wahrheit. Keine Referenz-Edges,
Model-Payloads, Collection-/Compound-/optionalen ComboBox-Werte, Resources, Serializer/Encoding,
Factory, Deserialisierung, Referenzauflösung oder Rekonstruktion werden eingeführt.

Tests prüfen einzelne Roots, gemischte Ownership-Reihenfolge, deterministische verschachtelte
Pre-order, exakte Typen, konkrete Models, owned Root-/Unknown-Type-Ablehnung und besitzende Werte
nach Mutation/Zerstörung. Öffentliche Ownership-APIs erlauben keine doppelten/zyklischen oder
inkonsistenten Owner-/Parent-Strukturen; defensive Prüfungen werden ohne neue Korruptions-API oder
undefiniertes Verhalten in Tests implementiert. Die headless Metadaten-Demo zeigt Nodes und geordnete
Ownership-Edges. Dies bleibt eine Grundlage, keine vollständige UI Description.
