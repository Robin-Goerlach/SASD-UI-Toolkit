# ADR 0195: M8 Resource Reference Metadata and Authoring Contract Foundation

- Status: Accepted
- Date: 2026-11-04

## English

### Context

ADR 0194 introduced an owning, case-sensitive `ResourceKey` and an owning typed
`ResourceDictionary` whose first concrete family is `TextStyle`. The dictionary deliberately does
not bind resources to Components. M8 now needs the smaller contract that precedes lookup: an
authoring or description consumer must be able to state which resource it means and which exact
built-in properties permit such a reference.

Reference identity and resolution policy are different responsibilities. Combining them now would
force decisions about Application/Window/local scope, parent traversal, fallback, rebinding and
inline-value precedence before a concrete resolver has been designed. Likewise, putting references
into Component snapshots or JSON v1 would prematurely freeze durable representation and
reconstruction semantics.

The current property catalog contains exactly six persistable `textStyle` descriptors, declared by
`Label`, `Button`, `TextField`, `CheckBox`, `RadioButton` and `ComboBox`. All six continue to hold
complete inline `TextStyle` values. They are the only real resource-authoring use case today.

### Decision

Add the public `ResourceKind` enum with exactly `text_style`. A kind is machine-readable family type
information carried in addition to the dictionary-wide unique key; it is not part of key equality.
Its enum ordinal is not an ABI, JSON or persistence value. Fonts, images, icons, brushes and themes
are not added speculatively. `isBuiltInResourceKind()` recognizes `text_style` and rejects fabricated
enum values fail closed.

Add the public owning `ResourceReference` value. It contains one `ResourceKind` and one
`ResourceKey`, owns the key completely and stores no dictionary, Component or backend address.
`ResourceReference::create()` rejects unknown kinds; the key has already been validated and copied by
`ResourceKey`. References therefore remain ordinary copyable/movable values whose identity is exact
and case-sensitive.

A `ResourceReference` is authoring identity, not a runtime binding. Creating one does not perform
lookup, select a scope, observe changes, mutate a Component or replace its inline property value.
The eventual relationship between a reference and an inline value—replacement, override, fallback
or materialization—is deliberately undecided until resolution, persistence and reconstruction are
designed together.

Add the separate public `ComponentPropertyResourceMetadata` layer. It contains only the expected
`ResourceKind`. `builtInComponentPropertyResourceMetadata()` accepts one property descriptor and
uses full value identity: `declaring_type_id`, `property_id` and `value_kind`. Copied exact
descriptors succeed; unknown, malformed, wrong-kind, wrong-case and fabricated descriptors return
`std::nullopt`.

The capability mapping is explicit, not inferred from the name `textStyle` or the
`ComponentPropertyValueKind::text_style` schema kind. Exactly these directly declared descriptors
opt in:

- `sasd.ui.Label.textStyle`;
- `sasd.ui.Button.textStyle`;
- `sasd.ui.TextField.textStyle`;
- `sasd.ui.CheckBox.textStyle`;
- `sasd.ui.RadioButton.textStyle`;
- `sasd.ui.ComboBox.textStyle`.

If a future resource-capable property is inherited, metadata remains attached to its declaring
descriptor; derived-type duplicates are not created. A future `text_style` property may intentionally
remain inline-only until it explicitly opts in.

### Consequences and boundaries

Tooling can now discover resource-capable property slots and transport a typed owning reference
without constructing Components or depending on a backend. The six existing inline TextStyle
getters, setters, presentation paths and defaults remain authoritative and unchanged.

This decision adds no Component `ResourceDictionary*`, tree traversal, resolver, scope, global
registry, cascade, fallback, live rebinding or observer. It adds no generic resource base, variant,
`std::any`, type registry, reflection or scripting model. `ResourceDictionary` keeps its typed
TextStyle bucket and duplicate-key behavior unchanged; entries do not redundantly store a kind.

UI Description JSON v1, format version, canonical bytes, parser, document model, snapshots and
reconstruction are unchanged. No Component-node reference collection or inline-versus-resource
precedence is defined. M8.26 is the explicit next boundary for resource resolution and scoping;
durable description/encoding and reconstruction remain later slices.

## Deutsch

### Kontext

ADR 0194 führte einen besitzenden, case-sensitiven `ResourceKey` und ein besitzendes typisiertes
`ResourceDictionary` ein, dessen erste konkrete Familie `TextStyle` ist. Das Dictionary bindet
Resources bewusst nicht an Components. M8 benötigt nun den kleineren Vertrag vor einem Lookup: Ein
Authoring- oder Description-Consumer muss ausdrücken können, welche Resource gemeint ist und welche
exakten Built-in-Properties eine solche Reference zulassen.

Reference-Identität und Resolution Policy sind unterschiedliche Verantwortlichkeiten. Eine heutige
Kopplung würde Entscheidungen über Application-/Window-/lokale Scopes, Parent-Traversal, Fallback,
Rebinding und Inline-Value-Priorität erzwingen, bevor ein konkreter Resolver entworfen ist. References
in Component-Snapshots oder JSON v1 würden außerdem dauerhafte Repräsentation und Rekonstruktion zu
früh festschreiben.

Der aktuelle Property-Katalog enthält exakt sechs persistierbare `textStyle`-Deskriptoren, direkt
deklariert von `Label`, `Button`, `TextField`, `CheckBox`, `RadioButton` und `ComboBox`. Alle sechs
halten weiterhin vollständige inline `TextStyle`-Werte. Sie sind heute der einzige reale Resource-
Authoring-Anwendungsfall.

### Entscheidung

Das öffentliche Enum `ResourceKind` wird mit exakt `text_style` ergänzt. Ein Kind ist
maschinenlesbare Resource-Familien-Typinformation zusätzlich zum dictionaryweit eindeutigen Key; es
ist kein Teil der Key-Gleichheit. Der Enum-Ordinalwert ist weder ABI- noch JSON- oder Persistenzwert.
Fonts, Images, Icons, Brushes und Themes werden nicht spekulativ ergänzt.
`isBuiltInResourceKind()` erkennt `text_style` und lehnt fabrizierte Enum-Werte fail-closed ab.

Der öffentliche besitzende Value `ResourceReference` wird ergänzt. Er enthält einen `ResourceKind`
und einen `ResourceKey`, besitzt den Key vollständig und speichert weder Dictionary-, Component-
noch Backend-Adresse. `ResourceReference::create()` lehnt unbekannte Kinds ab; der Key wurde bereits
durch `ResourceKey` validiert und kopiert. References bleiben dadurch normale kopier- und
verschiebbare Values mit exakter case-sensitiver Identität.

Eine `ResourceReference` ist Authoring-Identität, kein Runtime Binding. Ihre Erzeugung führt keinen
Lookup aus, wählt keinen Scope, beobachtet keine Änderungen, mutiert keine Component und ersetzt
keinen inline Property-Wert. Die spätere Beziehung zwischen Reference und Inline-Wert—Ersetzung,
Override, Fallback oder Materialisierung—bleibt bewusst offen, bis Resolution, Persistenz und
Rekonstruktion gemeinsam entworfen werden.

Die getrennte öffentliche Schicht `ComponentPropertyResourceMetadata` wird ergänzt. Sie enthält nur
den erwarteten `ResourceKind`. `builtInComponentPropertyResourceMetadata()` nimmt einen Property-
Deskriptor entgegen und verwendet die vollständige Value-Identität aus `declaring_type_id`,
`property_id` und `value_kind`. Kopierte exakte Deskriptoren funktionieren; unbekannte, malformed,
wrong-kind, wrong-case und fabrizierte Deskriptoren liefern `std::nullopt`.

Das Capability-Mapping ist explizit und wird weder aus dem Namen `textStyle` noch aus dem Schema-
Kind `ComponentPropertyValueKind::text_style` abgeleitet. Exakt diese direkt deklarierten
Deskriptoren nehmen teil:

- `sasd.ui.Label.textStyle`;
- `sasd.ui.Button.textStyle`;
- `sasd.ui.TextField.textStyle`;
- `sasd.ui.CheckBox.textStyle`;
- `sasd.ui.RadioButton.textStyle`;
- `sasd.ui.ComboBox.textStyle`.

Falls künftig eine resource-capable Property geerbt wird, bleiben die Metadaten am deklarierenden
Deskriptor; Derived-Type-Duplikate entstehen nicht. Eine künftige `text_style`-Property darf bewusst
inline-only bleiben, bis sie explizit teilnimmt.

### Folgen und Grenzen

Tooling kann nun resource-capable Property-Slots erkennen und eine typisierte besitzende Reference
transportieren, ohne Components zu konstruieren oder von einem Backend abzuhängen. Die sechs
vorhandenen inline TextStyle Getter, Setter, Presentation-Pfade und Defaults bleiben autoritativ und
unverändert.

Diese Entscheidung ergänzt weder `ResourceDictionary*` in Components noch Tree-Traversal, Resolver,
Scopes, globale Registry, Cascade, Fallback, Live-Rebinding oder Observer. Sie ergänzt keine
generische Resource-Basis, Variant, `std::any`, Typ-Registry, Reflection oder Scripting-Modell.
`ResourceDictionary` behält seinen typisierten TextStyle-Bucket und sein Duplicate-Key-Verhalten;
Entries speichern keinen redundanten Kind.

UI Description JSON v1, Formatversion, kanonische Bytes, Parser, Dokumentmodell, Snapshots und
Rekonstruktion bleiben unverändert. Es entsteht keine Component-Node-Reference-Collection und keine
Inline-vs.-Resource-Prioritätsregel. M8.26 ist die explizite nächste Grenze für Resource Resolution
und Scoping; dauerhafte Description/Encoding und Rekonstruktion bleiben spätere Slices.
