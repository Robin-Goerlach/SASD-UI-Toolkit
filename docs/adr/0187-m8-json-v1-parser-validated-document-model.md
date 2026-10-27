# ADR 0187: M8 Canonical JSON v1 Parser and Validated UI Description Document Model

- Status: Accepted
- Date: 2026-10-27

## English

### Context

[ADR 0186](0186-m8-canonical-json-ui-description-encoding-v1.md) defines canonical JSON v1
above the built-in capture graph. Parsing and live reconstruction have different failure classes:
syntax/schema/graph validity can be established without choosing constructors, setter ordering,
constructor-only relations, ownership adoption, reference resolution or callback transactions.

### Decision

Add `ui_description_document.hpp` with owning format-neutral `BuiltInUiDescriptionDocument`,
`BuiltInUiDescriptionNode`, `UiDescriptionReferenceEdge` and eight concrete typed property values.
Reuse `ComponentDescriptionNodeId`, `BuiltInComponentReferenceKind`, `SizeConstraints` and `TextStyle`.
No generic value, variant, any, JSON DOM, runtime pointer or encoding/version marker belongs here.
The public aggregates are mutable values; successful parsing validates the returned state, not
arbitrary later edits.

Loaded documents deliberately differ from capture snapshots. `is_default` is not persisted and
cannot be reconstructed honestly: inventing false or consulting current defaults would conflate
capture/tooling metadata with durable semantics. Every property, including inherited properties
and explicit default values, is required exactly once. No default completion occurs.

`parseBuiltInUiDescriptionJson(string_view)` is a dependency-free specialized adapter, not a
reconstruction engine. It returns either an owning document with error `none`, or no document
with a stable `UiDescriptionJsonParseErrorCode` and a zero-based input byte offset at/near detection.
EOF may be reported at input size; semantic errors may identify the containing node/edge.
Allocation failures propagate normally; parse failures use no exception contract and no file I/O.

The writer's canonical bytes remain unchanged. The reader accepts JSON whitespace and arbitrary
object-member/property/node/reference array order. Unknown and duplicate object members fail
closed at every schema level, including escaped equivalent keys. There are no extension fields.
Node objects normalize by ID without renumbering; references normalize by source ID and fixed
reference-kind order. Typed property collections normalize by declaring type/property identity;
this order never defines future restoration dependencies (items must precede selectedIndex there).
Ownership child order and string-collection order remain semantic and unchanged.

The complete input must be valid UTF-8 without BOM. JSON string escapes, including UTF-16 surrogate
pairs, decode to owning UTF-8 without replacement or Unicode normalization. Lone surrogates,
invalid escapes, raw controls, malformed/overlong/truncated UTF-8 and trailing garbage fail.
Comments, trailing commas, single quotes, NaN/Infinity, plus signs and leading zeros fail.
Integer tokens use `from_chars` directly, never floating point: int64/uint64 and Coordinate bounds
are exact; fractions/exponents are rejected. No JavaScript safe-integer restriction is imposed.

The finite schema bounds parsing depth; arrays and ownership traversal are iterative. Only a known
property value may be scanned ahead to permit value-before-kind order, with nesting bounded by
the compound-value schema. Unknown member values are never generically skipped. No file-size
limit or external parser library is introduced.

Validation uses immutable built-in metadata and pure values, never constructed Components:

- exact format/version, complete top-level/node/property/reference/compound shapes;
- known exact types, complete effective properties, declaring identities and eight value kinds;
- root zero, dense unique IDs, one owner per non-root, Container owners, no root/self/duplicate
  children, valid endpoints, reachability, acyclicity and exact ownership preorder;
- eleven symbolic reference kinds, existing distinct endpoints, one edge per source/kind,
  metadata base-chain compatibility; view.model and selection.model stay independent (ADR 0182);
- valid SizeConstraints ranges (negative coordinates and out-of-range preferred remain valid),
  ComboBox optional selection within items, positive GridLayout columns, unchanged single-line
  TextField sanitization, and at most one selected RadioButton per referenced RadioGroup.

### Consequences and exclusions

The pipeline is JSON -> validated document -> **future** reconstruction. No factory, Components,
setter/property restore, ownership adoption, resolver or callback suppression is added. Host-size_t
representability belongs to later reconstruction. Model payloads, resources and transient state
remain excluded. Parsing success establishes the current v1 semantic contract, not a promise that
future runtime construction cannot fail. Alternatives such as parsing into capture snapshots,
a generic DOM/dependency, strict canonical-byte acceptance, or silent default filling were rejected
because they obscure these boundaries or change persisted meaning.

## Deutsch

### Kontext

[ADR 0186](0186-m8-canonical-json-ui-description-encoding-v1.md) definiert kanonisches JSON v1.
Parserfehler (Syntax, Schema, Graph) sind von späteren Runtime-Rekonstruktionsfehlern getrennt.
Konstruktorwahl, Setter-Reihenfolge, Constructor-only-Relationen, Ownership-Adoption,
Referenzauflösung und Callback-Transaktionen gehören in einen späteren Slice.

### Entscheidung

`ui_description_document.hpp` enthält besitzende formatneutrale Werte: Document, Node, ReferenceEdge
und acht konkrete Property-Typen. Vorhandene Node-ID-, Reference-Kind-, SizeConstraints- und
TextStyle-Typen werden wiederverwendet. Kein generischer Value, variant/any, JSON-DOM, Runtime-
Pointer oder Format-/Versionsmarker. Öffentliche Aggregate sind veränderbar; die Parse-Garantie
bezieht sich auf das zurückgegebene Dokument vor weiteren Änderungen.

Loaded Document und Capture Snapshot bleiben absichtlich getrennt: `is_default` wurde nicht
gespeichert. Weder false noch heutige Defaults dürfen diese Information erfinden. Alle effektiven
Properties einschließlich geerbter Properties und expliziter Default-Werte sind exakt einmal nötig.
Keine Default-Ergänzung.

Die dependency-freie spezialisierte API `parseBuiltInUiDescriptionJson(string_view)` liefert entweder
ein besitzendes Dokument mit Fehlercode none oder kein Dokument mit stabilem Fehlercode und
0-basiertem Input-Byteoffset an/nahe der Erkennung. EOF darf input.size() ergeben; semantische
Fehler dürfen den enthaltenden Node/Edge markieren. Allocation-Fehler propagieren; keine Parse-
Exception und keine Datei-API.

Der Writer bleibt bytekanonisch unverändert. Der Reader akzeptiert JSON-Whitespace und andere
Object-Member-, Property-, Node- und Reference-Array-Reihenfolgen. Unbekannte und doppelte Member
werden auf jeder Schemaebene abgelehnt, auch bei äquivalent escapeten Keys. Keine Extensions.
Nodes werden nach ID angeordnet, niemals umnummeriert. References folgen Source-ID und fester
Reference-Kind-Reihenfolge. Konkrete Property-Collections werden nach deklarierendem Typ und
Property-ID normalisiert; das definiert keine Restore-Reihenfolge. Ownership-Children und
String-Collections behalten ihre semantische Reihenfolge.

Der gesamte Input muss gültiges UTF-8 ohne BOM sein. JSON-Escapes einschließlich UTF-16-Surrogat-
Paaren werden in besitzendes UTF-8 decodiert, ohne Ersatzzeichen oder Unicode-Normalisierung.
Einzelne Surrogate, ungültige Escapes, rohe Controls, malformed/overlong/truncated UTF-8 sowie
Trailing Garbage werden abgelehnt. Ebenso Kommentare, Trailing Commas, Single Quotes, NaN/Infinity,
Pluszeichen und führende Nullen. Ganzzahlen werden exakt per from_chars ohne Float gelesen;
int64/uint64 und Coordinate-Grenzen gelten, Fractions/Exponenten sind ungültig. Keine JavaScript-
Safe-Integer-Grenze.

Die endliche Schemaform begrenzt die Parse-Tiefe. Arrays und Ownership-Traversierung sind iterativ.
Nur bekannte Property-Werte dürfen für value-vor-kind begrenzt vorgelesen werden. Unbekannte
Member-Werte werden nicht generisch übersprungen. Keine Dateigrößenlimits oder Parser-Dependency.

Validierung nutzt ausschließlich Metadaten und reine Werte, keine konstruierten Components:

- exaktes Format/Version und vollständige Top-Level-/Node-/Property-/Reference-/Compound-Formen;
- bekannte Typen, vollständige effektive Property-Liste, deklarierende Identitäten und acht Kinds;
- Root 0, dichte eindeutige IDs, ein Owner pro Non-root, Container-Owner, keine Root-/Self-/doppelten
  Children, gültige Endpunkte, Erreichbarkeit, Zyklusfreiheit und exakte Ownership-Preorder;
- elf symbolische Reference-Kinds, vorhandene verschiedene Endpunkte, höchstens eine Edge pro
  Source/Kind, Typkompatibilität über Base-Chain; view.model und selection.model bleiben unabhängig;
- gültige SizeConstraints-Range (negative Werte und Preferred außerhalb bleiben gültig), gültiger
  optionaler ComboBox-Index, positive GridLayout-Spaltenzahl, unverändert single-line TextField-
  Text und höchstens ein ausgewählter RadioButton je referenzierter RadioGroup.

### Konsequenzen und Grenzen

JSON -> validiertes Dokument -> **zukünftige** Rekonstruktion. Keine Factory, Component-Konstruktion,
Property-Restore, Ownership-Adoption, Resolver oder Callback-Suppression. Host-size_t-Grenzen
gehören zur späteren Rekonstruktion. Model-Payloads, Resources und transienter Zustand bleiben
außen vor. Parse-Erfolg garantiert den heutigen v1-Vertrag, keine unfehlbare spätere Konstruktion.
Capture-Snapshot als Ladeziel, generischer DOM/Dependency, Byte-Kanonizität als Lesepflicht und
stille Default-Ergänzung wurden wegen vermischter Schichten bzw. veränderter Bedeutung verworfen.
