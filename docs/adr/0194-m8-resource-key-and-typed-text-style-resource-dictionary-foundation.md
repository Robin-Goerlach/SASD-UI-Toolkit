# ADR 0194: M8 Resource Key and Typed TextStyle Resource Dictionary Foundation

- Status: Accepted
- Date: 2026-10-31

## English

### Context

ADR 0185 established the six controls' current inline `TextStyle` as durable Component authoring
state and deliberately deferred shared styles, resource dictionaries, theme tokens and richer
resource-backed styling. This gives M8 a concrete first resource use case: several future consumers
may name and share one backend-neutral `TextStyle`. It does not yet justify a universal resource
object model, runtime binding, lookup scopes or persistence.

Resource identity must outlive caller buffers and remain stable across platforms, formats and
locales. A display label cannot provide that contract: labels may be localized or edited for
presentation. A filesystem path, URL, UUID or runtime pointer would also impose semantics that a
style resource does not need. Hidden process-global ownership would conflict with RAII and make
future Application/document scope and tests difficult to reason about.

### Decision

Add the public owning `ResourceKey` value. Its `create(std::string_view)` operation copies valid
input and returns `std::nullopt` for ordinary validation failures; allocation failure may propagate. Its
stable grammar is one or more bytes from `A-Z`, `a-z`, `0-9`, `_`, `-`, `.` and `/`. Every other
byte, including whitespace, controls, backslash, colon and non-ASCII UTF-8 bytes, is invalid. The
grammar is deliberately locale-free and avoids Unicode normalization. Equality is exact and
case-sensitive. Dots and slashes are opaque characters: even punctuation that resembles a path
does not create parent, traversal, namespace or scope behavior.

Add the public, explicitly owned `ResourceDictionary` and `TextStyleResource { ResourceKey key;
TextStyle value; }`. The first implementation contains one concrete `std::vector<TextStyleResource>`
bucket. It copies/moves caller values into owned storage and is itself an ordinary copyable and
movable value with no shared ownership. There is no singleton, mutable global registry or implicit
Application attachment.

Keys are globally unique within one dictionary across present and future resource families. The
current private key-existence scan contains only the TextStyle bucket; every later concrete family
must join that check. This preserves unambiguous future references without inventing a generic
payload carrier today.

`addTextStyle()` appends a new exact key and returns true. An exact duplicate returns false and
leaves the original entry unchanged. It never applies last-writer-wins replacement. Text-style
enumeration preserves insertion order. The vector is appropriate for the expected small foundation
sets and provides deterministic authoring output without freezing a hash contract; a private lookup
index may be added later without changing the API.

`findTextStyle()` performs exact case-sensitive lookup. Unknown, malformed or wrong-case text returns
`nullptr`; there is no default key, normalization or fallback. Returned pointers and enumeration
spans borrow dictionary storage and remain valid only until the next mutating dictionary operation.
This makes vector reallocation lifetime explicit. The only current mutation is `addTextStyle()`.

TextStyle remains the existing small backend-neutral value. It gains no ResourceKey member, virtual
base or shared pointer. Concrete typed storage is preferred over `std::variant`, `std::any`, `void*`,
type erasure, reflection or a generic `ResourceValue`: there is only one proven payload family, and
later fonts/images can add concrete typed families if real backends establish their contracts.

The six controls keep their complete inline `setTextStyle(TextStyle)`/`textStyle()` runtime behavior.
This slice adds no Component resource reference, binding, tree lookup, inheritance, cascade, theme,
scope or resolution policy. It also does not change UI Description JSON v1, property snapshots,
format version, parsing or reconstruction. Persisting unreferenced resources before reference and
description contracts exist would create dead data and prematurely freeze ordering/encoding.

### Consequences and boundaries

Applications and later UI descriptions can own independent dictionaries with deterministic typed
contents. Callers can build a dictionary and read it, while replace/remove/rename/clear and editor
history remain later authoring concerns. Global key uniqueness is ready for unambiguous references,
but no such reference exists yet.

The resource-system roadmap remains open for explicit references and scope/resolution, durable
description/encoding/reconstruction, and richer resource families justified by consumers. Fonts,
images/icons, theme cascade and automatic style inheritance are not promised by this foundation.
Human-facing property authoring attributes also remain deferred until a real designer/tooling
consumer defines localization and editing requirements.

## Deutsch

### Kontext

ADR 0185 definierte den heutigen inline `TextStyle` der sechs Controls als dauerhaften Component-
Authoring-Zustand und stellte Shared Styles, Resource Dictionaries, Theme Tokens sowie umfangreicheres
Resource-basiertes Styling bewusst zurück. Damit besitzt M8 einen konkreten ersten Resource-
Anwendungsfall: Mehrere zukünftige Consumer können einen backendneutralen `TextStyle` benennen und
teilen. Dies rechtfertigt noch kein universelles Resource-Objektmodell, Runtime Binding, Lookup-
Scopes oder Persistenz.

Resource-Identität muss Caller-Buffer überleben und über Plattformen, Formate und Locales stabil
bleiben. Ein Display Label erfüllt diesen Vertrag nicht: Labels können lokalisiert oder für die
Darstellung geändert werden. Dateipfad, URL, UUID oder Runtime-Pointer würden ebenfalls Semantik
erzwingen, die eine Style Resource nicht benötigt. Verstecktes prozessglobales Ownership widerspräche
RAII und würde spätere Application-/Document-Scopes sowie Tests schwer nachvollziehbar machen.

### Entscheidung

Der öffentliche besitzende Value `ResourceKey` wird ergänzt. `ResourceKey::create(std::string_view)`
kopiert gültigen Input und liefert bei normalen Validierungsfehlern `std::nullopt`; Allocation Failure
darf propagieren. Die stabile Grammatik besteht aus mindestens einem Byte aus `A-Z`, `a-z`, `0-9`,
`_`, `-`, `.` und `/`. Jedes andere Byte einschließlich Whitespace, Controls, Backslash, Doppelpunkt
und Nicht-ASCII-UTF-8 ist ungültig. Die Grammatik ist bewusst locale-frei und vermeidet Unicode-
Normalisierung. Gleichheit ist exakt und case-sensitive. Punkt und Slash sind opake Zeichen: Auch
pfadähnliche Interpunktion erzeugt kein Parent-, Traversal-, Namespace- oder Scope-Verhalten.

Das öffentliche, explizit besitzende `ResourceDictionary` und `TextStyleResource { ResourceKey key;
TextStyle value; }` werden ergänzt. Die erste Implementierung enthält einen konkreten
`std::vector<TextStyleResource>`-Bucket. Caller-Werte werden in eigenen Storage kopiert/verschoben;
das Dictionary selbst ist ein normaler kopier- und verschiebbarer Value ohne Shared Ownership. Es
gibt weder Singleton noch veränderliche globale Registry oder implizite Application-Anbindung.

Keys sind innerhalb eines Dictionary global über heutige und zukünftige Resource-Familien eindeutig.
Der heutige private Key-Existence-Scan enthält nur den TextStyle-Bucket; jede spätere konkrete Familie
muss sich an dieser Prüfung beteiligen. Dies ermöglicht später eindeutige References, ohne heute
einen generischen Payload-Carrier zu erfinden.

`addTextStyle()` hängt einen neuen exakten Key an und liefert true. Ein exaktes Duplicate liefert
false und lässt den ursprünglichen Entry unverändert. Last-writer-wins-Ersetzung findet nie statt.
Die TextStyle-Enumeration bewahrt Einfügereihenfolge. Der Vector passt zu den erwarteten kleinen
Foundation-Mengen und liefert deterministische Authoring-Ausgabe ohne eingefrorenen Hash-Vertrag;
ein privater Lookup-Index kann später ohne API-Änderung ergänzt werden.

`findTextStyle()` führt exakten case-sensitive Lookup aus. Unbekannter, ungültiger oder anders
geschriebener Text liefert `nullptr`; es gibt weder Default Key noch Normalisierung oder Fallback.
Zurückgegebene Pointer und Enumeration-Spans leihen Dictionary-Storage und bleiben nur bis zur
nächsten mutierenden Dictionary-Operation gültig. Damit ist die Vector-Reallocation-Lebensdauer
explizit. Die einzige heutige Mutation ist `addTextStyle()`.

TextStyle bleibt der bestehende kleine backendneutrale Value. Er erhält weder ResourceKey-Member noch
virtuelle Basis oder Shared Pointer. Konkreter typisierter Storage wird `std::variant`, `std::any`,
`void*`, Typerasure, Reflection oder einem generischen `ResourceValue` vorgezogen: Es existiert nur
eine belegte Payload-Familie; spätere Fonts/Images können konkrete typisierte Familien ergänzen,
wenn reale Backends ihre Verträge belegen.

Die sechs Controls behalten ihr vollständiges inline `setTextStyle(TextStyle)`-/`textStyle()`-
Runtime-Verhalten. Dieser Slice ergänzt weder Component Resource Reference, Binding, Tree Lookup,
Inheritance, Cascade, Theme, Scope noch Resolution Policy. Ebenso unverändert bleiben UI Description
JSON v1, Property Snapshots, Formatversion, Parsing und Rekonstruktion. Unreferenzierte Resources vor
existierenden Reference-/Description-Verträgen zu persistieren würde tote Daten erzeugen und
Ordering/Encoding zu früh einfrieren.

### Folgen und Grenzen

Applications und spätere UI Descriptions können unabhängige Dictionaries mit deterministischen
typisierten Inhalten besitzen. Caller können ein Dictionary aufbauen und lesen; Replace/Remove/
Rename/Clear sowie Editor-Historie bleiben spätere Authoring-Fragen. Globale Key-Eindeutigkeit ist
für eindeutige References vorbereitet, aber eine solche Reference existiert noch nicht.

Die Resource-System-Roadmap bleibt offen für explizite References und Scope/Resolution, dauerhafte
Description/Encoding/Reconstruction sowie durch Consumer begründete weitere Resource-Familien. Fonts,
Images/Icons, Theme Cascade und automatische Style Inheritance sind durch diese Grundlage nicht
versprochen. Menschenlesbare Property-Authoring-Attribute bleiben ebenfalls zurückgestellt, bis ein
realer Designer-/Tooling-Consumer Lokalisierungs- und Bearbeitungsanforderungen definiert.
