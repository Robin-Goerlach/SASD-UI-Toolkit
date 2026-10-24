# ADR 0181: M8 Built-in Component Cross-Reference Edge Snapshot Foundation

- Status: Accepted
- Date: 2026-10-24
- Predecessors: [ADR 0177](0177-m8-serializable-ui-description-state-coverage-policy.md),
  [ADR 0180](0180-m8-description-node-identity-ordered-ownership-snapshot.md)

## Context

Slice 10 supplies description-local Component node IDs and ordered ownership snapshots. Those IDs
are the prerequisite for durable cross-reference endpoints without runtime identity, pointer values,
names or UUIDs. Scalar property metadata cannot represent these graph relations faithfully.

## Decision

The separate `component_reference_snapshot.hpp` / `.cpp` layer composes the existing
`BuiltInComponentOwnershipSnapshot` with a global ordered vector of `ComponentReferenceEdgeSnapshot`.
Each edge owns only `source_node_id`, `target_node_id` (both `ComponentDescriptionNodeId`) and a
`BuiltInComponentReferenceKind`. Ownership, root ID, exact node type, properties and owned-component
lists have exactly one representation, inside the composed ownership snapshot. Ownership is not a
reference kind. IDs refer to nodes, not to a separate public vector-index contract.

The closed enum supports exactly these typed getter relations:

| Kind | Source getter | Target family |
|---|---|---|
| `button_command` | `Button::boundCommand()` | `Command` |
| `radio_button_group` | `RadioButton::group()` | `RadioGroup` |
| `list_view_model` | `ListView::model()` | `ListModel` |
| `table_view_model` | `TableView::model()` | `TableModel` |
| `tree_view_model` | `TreeView::model()` | `TreeModel` |

Exact built-in derived model identity remains in the target node's property snapshot. Typed getters
enforce compatibility; unknown derived nodes still reject the entire ownership capture.
`RadioGroup.selectedButton` remains derived from membership plus `RadioButton.selected`, never an
independent persisted back-reference. Model payloads remain Application Data, outside baseline UI
Description payload; selection, expansion and viewport state are not captured here.

`captureBuiltInComponentReferenceGraphSnapshot(const Component&)` returns an optional owned graph.
It first captures ownership, traverses the same runtime tree in deterministic pre-order, checks
node IDs/types/ordered children and owner/visual-parent consistency against that snapshot, and builds
a temporary pointer-to-node-ID map. It then examines supported getters in source node order and the
fixed enum order within each source. Neither pointer sorting nor hash iteration defines output order.
Pointers are temporary lookup keys only; no pointer leaves capture. Capture runs on the UI thread
without callbacks or mutation; allocation failures propagate normally.

This first graph contract is **self-contained**: every non-null live supported target must resolve
inside the same ownership snapshot. Otherwise the entire capture returns `std::nullopt`; silently
dropping a live external relation would describe a semantically different UI. No automatic adoption,
external resource IDs or reference resolver is introduced.

A null getter means no edge: unbound, ungrouped and model-free controls remain valid. Lifetime-safe
getters may also become null after target destruction. Such expired targets have no reconstructible
live relation; capture records current living declarative relationships, not failure history or
lifetime tombstones. In particular, Button's internal distinction between never/unbound and expired
binding does not justify a new public binding-state API for serialization.

List/Table/Tree SelectionModels are **not Component-derived**. They have neither built-in Component
type IDs nor ADR-0180 nodes. No pseudo-nodes, inheritance changes or selection-model reference kinds
are added. The **SelectionModel association policy remains open**, without deciding persistence or
exclusion of those associations. Current selection itself remains transient application runtime state.

## Consequences and validation

The snapshot owns all edge/property values after runtime mutation or destruction. Tests cover the
five relations, exact model types without payload, deterministic nested/forward/shared-target edges,
null and expired getters, every supported external-target rejection, unknown nodes and owned-root
rejection. The headless metadata demo prints node IDs and named edge kinds without pointer addresses.

This is no serializer, encoding, deserializer, factory, reconstruction ordering, general reflection,
mutable registry or third-party relation registration. Collection/compound properties, external
resources and SelectionModel associations require separate future decisions.

---

# ADR 0181: M8 Built-in-Component-Cross-Reference-Edge-Snapshot-Grundlage

- Status: Akzeptiert
- Datum: 24.10.2026

## Kontext

Slice 10 liefert beschreibungslokale Component-Node-IDs und geordnete Ownership-Snapshots. Erst diese
IDs erlauben Referenzziele ohne Runtime-Identität, Pointerwerte, Namen oder UUIDs. Referenzbeziehungen
gehören in eine Graph-Schicht statt in skalare Property-Metadaten.

## Entscheidung

Die separate Schicht `component_reference_snapshot.hpp` / `.cpp` komponiert den bestehenden
`BuiltInComponentOwnershipSnapshot` mit einer globalen geordneten Liste von
`ComponentReferenceEdgeSnapshot`. Jede Edge besitzt ausschließlich Source-/Target-Node-ID vom Typ
`ComponentDescriptionNodeId` und `BuiltInComponentReferenceKind`. Root-ID, exakte Typen, Properties
und geordnete Ownership-Listen werden ausschließlich im enthaltenen Ownership-Snapshot gespeichert.
Ownership wird nicht nochmals als Reference-Edge abgelegt. IDs sind Node-Identität, kein zusätzlicher
öffentlicher Vector-Index-Vertrag.

Genau fünf typisierte Getter-Beziehungen werden unterstützt: `Button::boundCommand()` -> Command
(`button_command`), `RadioButton::group()` -> RadioGroup (`radio_button_group`), `ListView::model()`
-> ListModel (`list_view_model`), `TableView::model()` -> TableModel (`table_view_model`) und
`TreeView::model()` -> TreeModel (`tree_view_model`). Exakte Built-in-Derived-Model-Typen bleiben im
Target-Node erhalten. Die Getter gewährleisten Typkompatibilität; unbekannte Derived Nodes verwerfen
weiterhin den gesamten Capture. `RadioGroup.selectedButton` bleibt aus Mitgliedschaft und
`RadioButton.selected` abgeleitet und wird nicht unabhängig persistiert. Model-Payloads bleiben
Application Data außerhalb der Baseline-UI-Description; Selection, Expansion und Viewport fehlen.

Capture erstellt zuerst den Ownership-Snapshot, traversiert denselben Baum erneut in Pre-order und
prüft IDs, Typen, geordnete Kinder sowie Owner-/Visual-Parent-Konsistenz. Eine ausschließlich temporäre
Pointer-zu-Node-ID-Map löst anschließend Ziele auf. Edges folgen Source-Node-Reihenfolge und darin
der festen Enum-Reihenfolge, niemals Pointer-Sortierung oder Hash-Iteration. Keine Pointer verlassen
Capture. Die Operation läuft auf dem UI-Thread ohne Callbacks oder Mutation; Allokationsfehler
propagieren normal.

Der Vertrag ist **selbständig geschlossen**: Jedes lebende non-null Ziel muss im selben Ownership-
Snapshot enthalten sein. Andernfalls liefert der gesamte Capture `std::nullopt`; stilles Weglassen
würde eine semantisch andere UI beschreiben. Keine automatische Adoption, External Resource IDs
oder Reference Resolver.

Null-Getter erzeugen keine Edge: Ungebundene Buttons, ungruppierte RadioButtons und Views ohne Model
sind gültig. Lifetime-sichere Getter können nach Target-Zerstörung ebenfalls null liefern. Abgelaufene
Ziele sind keine rekonstruierbare lebende deklarative Beziehung. Failure-History und Lifetime-
Tombstones werden nicht persistiert. Buttons interne Unterscheidung zwischen niemals/entbunden und
abgelaufen rechtfertigt keine neue öffentliche Binding-State-API für Serialisierung.

List/Table/Tree SelectionModels sind **keine Components** und haben weder Built-in-Component-Type-ID
noch ADR-0180-Node. Keine Pseudo-Nodes, geänderte Vererbung oder SelectionModel-Reference-Kinds.
Die **SelectionModel association policy bleibt offen**; Persistenz oder Ausschluss der Assoziationen
wird nicht entschieden. Aktuelle Selection bleibt transienter Application-Runtime-Zustand.

## Konsequenzen und Validierung

Alle Edge-/Property-Werte bleiben nach Runtime-Mutation und -Zerstörung besitzend lesbar. Tests prüfen
die fünf Beziehungen, exakte Model-Typen ohne Payload, deterministische verschachtelte Vorwärts-/
Shared-Target-Edges, null/abgelaufene Getter, externe Ziele aller fünf Kinds, unbekannte Nodes und
Owned-Root-Ablehnung. Die headless Demo zeigt Node-IDs und Edge-Kinds ohne Pointer-Adressen.

Kein Serializer, Encoding, Deserializer, Factory, Rekonstruktionsablauf, allgemeine Reflection,
mutable Registry oder Third-Party-Relation-Registrierung. Collection-/Compound-Properties, externe
Ressourcen und SelectionModel-Assoziationen benötigen eigene spätere Entscheidungen.
