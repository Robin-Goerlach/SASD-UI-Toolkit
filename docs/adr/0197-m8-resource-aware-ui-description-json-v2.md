# ADR 0197: M8 Resource-Aware UI Description JSON v2

- Status: Accepted
- Date: 2026-11-06

## English

### Context

ADRs 0194–0196 established stable resource keys, concrete typed TextStyle definitions, owning typed
references, exact property capability metadata and explicit caller-ordered runtime lookup. Durable
UI descriptions still could not state either resource definitions or that a property was authored by
reference.

Canonical JSON UI Description v1 is an external versioned contract with exactly five top-level
members. Its strict reader rejects unknown members and versions. Adding resources to v1 would either
change stable bytes or tempt an old reader to ignore semantics it cannot preserve. Runtime Components
also deliberately contain only concrete inline TextStyle values, so a Component-root writer cannot
infer durable resource authoring.

### Decision

Keep JSON v1 immutable and add explicit document-based v2 writer/reader APIs. v2 has top-level order
`format`, `formatVersion`, `resources`, `root`, `nodes`, `references`. It reuses all v1 graph,
property, UTF-8 and formatting contracts and adds one closed document-local scope.

Extend the format-neutral document with a concrete `std::vector<TextStyleResource>` and each node
with concrete `UiDescriptionResourceReferenceProperty` values. Reusing `TextStyleResource` aligns
durable and dictionary value semantics without inventing a generic ResourceValue. The property value
contains exact declaring type/property identity plus the existing typed `ResourceReference`.

A resource-capable descriptor has exactly one authoring value: inline XOR resource reference. No
inline fallback is stored. The existing property resource metadata authorizes exactly the six current
control `textStyle` descriptors and supplies their expected `ResourceKind::text_style`.

Every reference must resolve to a same-kind definition in the document-local scope. Resource keys
are globally unique. Definitions are normalized and canonically emitted by case-sensitive bytewise
`ResourceKey::value()` order, separating durable bytes from dictionary insertion order. The stable
JSON kind symbol is `text-style`; `resource-reference` is a property encoding tag, not a generic C++
value architecture.

Shared format-neutral validation enforces key/value validity, uniqueness, exact property identity,
capability, kind compatibility, inline XOR reference completeness and local resolution. The v2
encoder validates a private copy; the strict parser is only an adapter into the same validator.

Existing reconstruction rejects resource-bearing documents before constructing Components. Runtime
binding and resource-aware materialization are explicitly deferred to M8.28.

### Consequences and boundaries

v1 output and parsing remain byte-/behavior-compatible, including its exact top-level schema and
goldens. v2 can also encode an all-inline document with an empty resource array, but never silently
downgrades it to v1.

The v2 document is self-contained. It persists no Application, Window or Container scope, parent
scope, external scope chain or `scope_index`; that index is diagnostic provenance from an M8.26
runtime lookup, not durable identity. There are no external files/URLs/imports, cascade, merge,
fallback, live rebinding, new Component members, fonts, images, themes or generic resource registry.

This slice proves document encode/parse/encode stability only. It does not promise runtime resource
reconstruction, and accepting a document while discarding references is forbidden.

## Deutsch

### Kontext

ADRs 0194–0196 etablierten stabile Resource Keys, konkrete typisierte TextStyle-Definitionen,
besitzende typisierte References, exakte Property-Capability-Metadaten und expliziten Caller-
geordneten Runtime Lookup. Dauerhafte UI Descriptions konnten weiterhin weder Resource-Definitionen
noch Resource-basiertes Property Authoring ausdrücken.

Das kanonische JSON UI Description Format v1 ist ein externer versionierter Vertrag mit exakt fünf
Top-Level-Membern. Sein strikter Reader lehnt unbekannte Member und Versionen ab. Resources in v1
würden stabile Bytes ändern oder einen alten Reader dazu verleiten, nicht erhaltbare Semantik zu
ignorieren. Runtime Components enthalten außerdem bewusst nur konkrete Inline-TextStyle-Werte; ein
Component-Root-Writer kann daraus kein dauerhaftes Resource Authoring ableiten.

### Entscheidung

JSON v1 bleibt unveränderlich; ausdrücklich dokumentbasierte v2-Writer-/Reader-APIs kommen hinzu.
v2 verwendet die Top-Level-Reihenfolge `format`, `formatVersion`, `resources`, `root`, `nodes`,
`references`. Alle v1-Verträge für Graph, Properties, UTF-8 und Formatierung werden wiederverwendet;
hinzu kommt ein geschlossener dokumentlokaler Scope.

Das formatneutrale Dokument erhält einen konkreten `std::vector<TextStyleResource>`, jeder Node
konkrete `UiDescriptionResourceReferenceProperty`-Values. Die Wiederverwendung von
`TextStyleResource` richtet dauerhafte und Dictionary-Value-Semantik aneinander aus, ohne generischen
ResourceValue zu erfinden. Der Property-Value enthält exakte Declaring-Type-/Property-Identität und
die bestehende typisierte `ResourceReference`.

Ein Resource-fähiger Descriptor besitzt exakt einen Authoring Value: inline XOR Resource Reference.
Kein Inline-Fallback wird gespeichert. Bestehende Property-Resource-Metadaten autorisieren exakt die
sechs heutigen Control-`textStyle`-Descriptoren und liefern deren erwarteten
`ResourceKind::text_style`.

Jede Reference muss auf eine gleichartige Definition im dokumentlokalen Scope auflösen. Resource Keys
sind global eindeutig. Definitionen werden nach case-sensitive byteweiser `ResourceKey::value()`-
Ordnung normalisiert und kanonisch ausgegeben; dauerhafte Bytes hängen damit nicht von Dictionary-
Einfügereihenfolge ab. Das stabile JSON-Kind-Symbol ist `text-style`; `resource-reference` ist ein
Property-Encoding-Tag und keine generische C++-Value-Architektur.

Gemeinsame formatneutrale Validierung prüft Key-/Value-Gültigkeit, Eindeutigkeit, exakte Property-
Identität, Capability, Kind-Kompatibilität, inline-XOR-reference-Vollständigkeit und lokale
Auflösbarkeit. Der v2-Encoder validiert eine private Kopie; der strikte Parser ist nur Adapter in
denselben Validator.

Bestehende Rekonstruktion lehnt Resource-tragende Dokumente vor jeder Component-Konstruktion ab.
Runtime Binding und Resource-aware Materialisierung bleiben ausdrücklich M8.28 vorbehalten.

### Folgen und Grenzen

v1-Ausgabe und -Parsing bleiben byte-/verhaltenskompatibel, einschließlich exaktem Top-Level-Schema
und Goldens. v2 darf auch ein vollständig inline geschriebenes Dokument mit leerem Resource Array
encodieren, stuft es aber niemals still auf v1 zurück.

Das v2-Dokument ist self-contained. Es persistiert weder Application-, Window- oder Container-Scope,
Parent Scope, externe Scope Chain noch `scope_index`; dieser Index ist diagnostische Provenienz eines
M8.26-Runtime-Lookups und keine dauerhafte Identität. Es gibt keine externen Dateien/URLs/Imports,
Cascade, Merge, Fallback, Live Rebinding, neue Component-Member, Fonts, Images, Themes oder generische
Resource Registry.

Dieser Slice beweist ausschließlich Dokument-Encode-/Parse-/Encode-Stabilität. Er verspricht keine
Runtime-Resource-Rekonstruktion; ein Dokument unter Verlust seiner References zu akzeptieren ist
verboten.
