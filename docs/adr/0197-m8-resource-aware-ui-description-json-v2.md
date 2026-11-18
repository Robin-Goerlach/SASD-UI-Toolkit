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

### Implementation and validation record

This work session started from the fetched and externally verified `origin/main` commit
`fb0fd37677e5b07f8d99ea42b076141501cf6b76`, which contained M8 Slice 26 through PR #101. The
dedicated implementation branch was `codex/m8-durable-resource-description-json-v2`. SSH
authentication was unavailable in the environment, so affected fetches and remote writes used
temporary HTTPS plus GitHub CLI credentials without changing the permanent SSH `origin`
configuration.

The implementation extended `BuiltInUiDescriptionDocument` with concrete `TextStyleResource`
definitions and nodes with concrete `UiDescriptionResourceReferenceProperty` values. It added the
public document-based `encodeBuiltInUiDescriptionJsonV2()` and
`parseBuiltInUiDescriptionJsonV2()` APIs while leaving the Component-based v1 encoder and strict v1
reader contract unchanged. Shared format-neutral validation now enforces valid and globally unique
resource keys, valid TextStyle values, exact resource-capable property identity, kind compatibility,
inline-XOR-reference completeness and document-local target resolution. Resources normalize by
case-sensitive key-byte order, while resource-backed properties retain normal descriptor order in
the canonical `properties` array. Existing reconstruction rejects every resource-bearing document
before creating runtime Components, so no reference can be silently discarded.

Focused coverage proves exact v2 golden bytes, encode/parse/encode byte stability, normalized
document equality, canonical resource ordering, all supported and malformed ResourceKey cases,
duplicate definitions, unknown kinds, missing targets, wrong properties, wrong kinds, duplicate or
missing property authoring values and all six resource-capable TextStyle properties. It also proves
that an all-inline v2 document remains v2 and that resource-bearing reconstruction fails closed.
The unchanged v1 golden, parser and resource-free reconstruction coverage passed alongside these
tests. The headless JSON demo shows `theme.heading` as a document-local definition and
`sasd.ui.Label.textStyle` as its durable reference without claiming runtime binding. The installed
package consumer constructs, encodes and parses the new public model.

Local validation used Visual Studio 2022/MSVC with warnings as errors. Strict Debug and Release
builds succeeded; the complete configured CTest suite passed 5/5 targets in both configurations and
the Core executable passed 557/557 cases. Release installation, the installed repository package
consumer, release-tree hygiene, the updated documentation links and `git diff --check` passed. The
optional SDL3, GTK and AppKit paths were not built locally; the subsequent GitHub matrices covered
SDL3, GTK Native and AppKit Native as well as GCC, Clang, Clang ASan/UBSan, MSVC, AppleClang and
Linux, Windows and macOS Release builds.

The implementation commit was
[`fa2550c96fea59fbffc3ac1b5fd36bb512d0c115`](https://github.com/Robin-Goerlach/SASD-UI-Toolkit/commit/fa2550c96fea59fbffc3ac1b5fd36bb512d0c115)
on [PR #102](https://github.com/Robin-Goerlach/SASD-UI-Toolkit/pull/102); it was also the final PR head.
All eleven PR gates passed for that exact head. The PR was merged normally as
`7ce132290465970cb9dca32dcc0079577d9b5631`, and complete post-merge Main, GTK Native and AppKit
Native workflows, including the full Release matrix, passed for that exact merge commit.

After post-merge CI succeeded, the remote and local feature branches were deleted, local `main` was
fast-forwarded to the merge commit and the remote was pruned. Slice-specific build, installation,
consumer, tool and backup directories were removed. The permanent SSH remote configuration was
preserved. Fourteen pre-existing modified ADR files plus the untracked `..Codex.txt` remained
byte-for-byte equal to their fifteen pre-work SHA-256 records. Historical PRs #30, #32, #63 and #100
remained open and were inspected read-only. No release or tag was created. M8 Slice 27 is complete;
resource-aware reconstruction/materialization remains the explicit M8.28 boundary, and the visible
macOS/Cocoa M3 gate remains open. M8.28 was not started.

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

### Implementierungs- und Validierungsprotokoll

Dieser Arbeitsabschnitt begann auf dem gefetchten und extern verifizierten `origin/main`-Commit
`fb0fd37677e5b07f8d99ea42b076141501cf6b76`, der M8 Slice 26 über PR #101 enthielt. Der dedizierte
Implementierungsbranch hieß `codex/m8-durable-resource-description-json-v2`. Da in der Umgebung
keine SSH-Authentifizierung verfügbar war, verwendeten die betroffenen Fetches und Remote-
Schreibvorgänge temporäres HTTPS mit GitHub-CLI-Credentials, ohne die permanente SSH-`origin`-
Konfiguration zu verändern.

Die Implementierung erweiterte `BuiltInUiDescriptionDocument` um konkrete
`TextStyleResource`-Definitionen und Nodes um konkrete
`UiDescriptionResourceReferenceProperty`-Werte. Hinzu kamen die öffentlichen dokumentbasierten APIs
`encodeBuiltInUiDescriptionJsonV2()` und `parseBuiltInUiDescriptionJsonV2()`; der Component-basierte
v1-Encoder und der strikte v1-Reader-Vertrag blieben unverändert. Die gemeinsame formatneutrale
Validierung erzwingt nun gültige und global eindeutige Resource Keys, gültige TextStyle-Werte,
exakte Resource-fähige Property-Identität, Kind-Kompatibilität, Inline-XOR-Reference-Vollständigkeit
und dokumentlokale Target-Auflösung. Resources werden nach case-sensitive Key-Bytes normalisiert;
Resource-basierte Properties behalten gleichzeitig die normale Deskriptorreihenfolge im
kanonischen `properties`-Array. Die bestehende Rekonstruktion lehnt jedes Resource-tragende Dokument
vor der Erzeugung von Runtime Components ab, sodass keine Reference still verworfen werden kann.

Fokussierte Abdeckung beweist exakte v2-Golden-Bytes, byte-stabiles Encode/Parse/Encode,
normalisierte Dokumentgleichheit, kanonische Resource-Reihenfolge, alle unterstützten und
fehlerhaften ResourceKey-Fälle, Duplicate Definitions, unbekannte Kinds, fehlende Targets, falsche
Properties, falsche Kinds, doppelte oder fehlende Property-Authoring-Werte sowie alle sechs
Resource-fähigen TextStyle-Properties. Sie beweist außerdem, dass ein vollständig inline
geschriebenes v2-Dokument v2 bleibt und Resource-tragende Rekonstruktion fail-closed arbeitet. Die
unveränderten v1-Golden-, Parser- und Resource-freien Rekonstruktionsfälle bestanden gemeinsam mit
diesen Tests. Die headless JSON-Demo zeigt `theme.heading` als dokumentlokale Definition und
`sasd.ui.Label.textStyle` als deren dauerhafte Reference, ohne Runtime Binding zu behaupten. Der
installierte Package-Consumer konstruiert, encodiert und parst das neue öffentliche Modell.

Die lokale Validierung verwendete Visual Studio 2022/MSVC mit Warnings as Errors. Strikte Debug- und
Release-Builds waren erfolgreich; die vollständige konfigurierte CTest-Suite bestand in beiden
Konfigurationen 5/5 Targets und das Core-Testprogramm 557/557 Fälle. Release-Installation,
installierter Repository-Package-Consumer, Release-Tree-Hygiene, die aktualisierten
Dokumentationslinks und `git diff --check` bestanden. Die optionalen SDL3-, GTK- und AppKit-Pfade
wurden lokal nicht gebaut; die anschließenden GitHub-Matrizen deckten SDL3, GTK Native und AppKit
Native sowie GCC, Clang, Clang ASan/UBSan, MSVC, AppleClang und Linux-, Windows- und macOS-Release-
Builds ab.

Der Implementierungscommit war
[`fa2550c96fea59fbffc3ac1b5fd36bb512d0c115`](https://github.com/Robin-Goerlach/SASD-UI-Toolkit/commit/fa2550c96fea59fbffc3ac1b5fd36bb512d0c115)
auf [PR #102](https://github.com/Robin-Goerlach/SASD-UI-Toolkit/pull/102); er war zugleich der finale
PR-Head. Alle elf PR-Gates bestanden für exakt diesen Head. Der PR wurde regulär als
`7ce132290465970cb9dca32dcc0079577d9b5631` gemergt; die vollständigen Post-Merge-Workflows für Main,
GTK Native und AppKit Native einschließlich der vollständigen Release-Matrix bestanden für exakt
diesen Merge-Commit.

Nach grüner Post-Merge-CI wurden Remote- und lokaler Feature-Branch gelöscht, der lokale `main` auf
den Merge-Commit fast-forwarded und der Remote geprunt. Slice-spezifische Build-, Installations-,
Consumer-, Tool- und Backup-Verzeichnisse wurden entfernt. Die permanente SSH-Remote-Konfiguration
blieb unverändert. Vierzehn bereits veränderte ADR-Dateien und das ungetrackte `..Codex.txt` stimmten
weiterhin bytegenau mit ihren fünfzehn vor Arbeitsbeginn erfassten SHA-256-Werten überein. Die
historischen PRs #30, #32, #63 und #100 blieben offen und wurden ausschließlich read-only geprüft.
Es wurde weder ein Release noch ein Tag erzeugt. M8 Slice 27 ist vollständig abgeschlossen;
Resource-aware Rekonstruktion/Materialisierung bleibt die ausdrückliche M8.28-Grenze, und das
sichtbare macOS/Cocoa-M3-Gate bleibt offen. M8.28 wurde nicht begonnen.
