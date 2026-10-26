# ADR 0186: M8 Canonical JSON UI Description Encoding v1

- Status: Accepted
- Date: 2026-10-26

## English

### Context

[ADR 0177](0177-m8-serializable-ui-description-state-coverage-policy.md) and the
[Slice-15 review](0185-m8-inline-text-style-authoring-persistence-coverage.md) establish sufficient
current built-in semantic coverage for persisted encoding: exact types, owned property values,
description-local IDs, ordered ownership and validated reference edges. Reconstruction and resources
remain distinct unfinished concerns. No JSON dependency exists in Core.

### Decision

Provide `encodeBuiltInUiDescriptionJson(const Component&) -> std::optional<std::string>` in
`sasd/ui/ui_description_json.hpp`, implemented in Core. It calls the existing reference graph capture
once and encodes only its values. The root API benefits from capture's exact-type, completeness,
ownership and target validation without accepting arbitrary hand-assembled snapshots or introducing
a second large validator. No runtime getters/relations are re-read after capture.

JSON is the first persisted reference representation because it is readable, inspectable with
existing tools, useful in Git diffs and has established escaping/Unicode rules. Inventing a SASD
grammar alongside semantics would add unnecessary scope. JSON remains an external encoding: no JSON
type enters Component, metadata or property/ownership/reference snapshots. Future SASD, binary or
DFM-like encodings remain possible. A small specialized dependency-free writer is sufficient; no
general JSON DOM/library, parser or third-party dependency is added.

The normative [English format specification](../en/UI_DESCRIPTION_FORMAT.md) and
[German specification](../de/UI_DESCRIPTION_FORMAT.md) define canonical JSON v1, recommended suffix
`.sasd-ui.json`, `format: "sasd.ui-description"` and positive integer `formatVersion: 1`.
Format version is independent of toolkit release version. Incompatible changes need a new version;
a future reader must check it explicitly, never silently assume unknown-version semantics.

All persistent values, including defaults, are explicit to protect saved meaning from later toolkit
default changes. `is_default` is tooling metadata and excluded; defaults are not queried again.
The eight kind-separated C++ collections flatten into one property list in base-to-derived metadata
declaration order, then direct descriptor order (ComboBox items before selectedIndex). Reconstruction
will still require its own dependency policy. Ownership has one ordered representation; reference
edges remain separate and retain capture order/IDs. No pointer or redundant visual parent is stored.

Property kinds, all eleven reference kinds and all seventeen Color values use the stable symbols in
the specification, never enum ordinals. This defines the symbolic Color follow-up anticipated by
ADR 0185 without changing its historical runtime decision: unknown Color is still legal at runtime,
but has no v1 symbol and rejects encoding. Unknown kinds and missing/duplicate captured values also
fail closed. Every persisted string must be valid UTF-8; invalid bytes reject the whole operation
instead of sanitizing, replacing or dropping content. Allocation failures propagate normally.

Fixed keys/arrays/property/reference order, two-space indentation, LF-only output and one final LF
make output deterministic for Git/review and independent of host metadata. Exact decimal integers use
`std::to_chars`, with no locale, floating conversion or JavaScript-safe-integer truncation.
Golden tests freeze complete canonical bytes; runtime tests cover symbol vocabularies, defaults,
UTF-8, escaping, integer boundaries, repeat determinism and capture rejection.

### Boundaries and consequences

Model/SelectionModel identity and edges persist, not application model payloads or current selection.
Transient, derived, backend and executable state stays excluded under ADR 0177: no focus/hover/press,
capture, caret, preview, expansion, viewport, layout caches, handles, callbacks or observers.
No resource IDs/system, file I/O, parser, JSON Schema, deserializer, factory, reference resolver or
reconstruction is introduced. A schema can follow a concrete reader/tooling need. UI-description
serialization as a whole remains incomplete until reconstruction; this slice finishes encoding only.

### Local validation

Base: `a4fc034154c1de61aa9ae13007c442da66045fd5`; branch:
`codex/m8-canonical-json-ui-description-v1`. Strict GCC 12.2 Debug and Release builds used the
existing Debian WSL toolchain, with tests/examples and warnings-as-errors enabled. Final full CTest
runs passed 4/4 each: Core 493/493, Terminal 321/321, Rendered 129/129 and native POSIX PTY smoke.
The focused standalone encoding executable passed 8/8. Both headless JSON demos produced identical
canonical bytes verified with an external JSON inspector. A Release install passed the repository
package consumer and an independent public-header/API consumer linked only to installed SASD::UI.
381 local Markdown file links had no missing targets; external URLs/fragments were not checked.
`git diff --check` passed. Native Windows/macOS, SDL3, GTK, AppKit and sanitizers are CI validation,
not local executions. No tests were disabled or assertions weakened.

Eight pre-existing foreign files (ADRs 0178–0183, ADR 0185 and `..Codex.txt`) were protected with
initial diffs and SHA-256 hashes and remained untouched. ADR 0185's optional Color follow-up is
recorded in this decision instead of editing that locally modified file. Build/audit artifacts
remain ignored under build-codex/slice16. An overlapping initial build caused an archive collision;
after both processes ended, orderly final builds and tests passed.

## Deutsch

### Kontext

[ADR 0177](0177-m8-serializable-ui-description-state-coverage-policy.md) und der
[Slice-15-Review](0185-m8-inline-text-style-authoring-persistence-coverage.md) bestätigen ausreichende
aktuelle Built-in-Semantik: exakte Typen, eigene Property-Werte, lokale Node-IDs, geordnete Ownership
und validierte Reference-Edges. Rekonstruktion und Resources bleiben getrennte offene Themen.
Core besitzt keine JSON-Abhängigkeit.

### Entscheidung

`encodeBuiltInUiDescriptionJson(const Component&) -> std::optional<std::string>` wird in
`sasd/ui/ui_description_json.hpp` und Core implementiert. Einmaliger Reference-Graph-Capture liefert
alle codierten Werte. Die Root-API nutzt dessen Typ-/Vollständigkeits-/Ownership-/Target-Prüfungen;
kein öffentlicher Encoder beliebiger handgebauter Snapshots und kein zweiter großer Validator.
Nach Capture werden keine Runtime-Getter oder Relationen erneut gelesen.

JSON ist erste persistierte Referenzrepräsentation: menschenlesbar, mit vorhandenen Tools prüfbar,
diff-freundlich und mit etablierten Escape-/Unicode-Regeln. Eine eigene SASD-Grammatik gleichzeitig
mit der Semantik wäre unnötiger Scope. JSON bleibt externes Encoding; keine JSON-Typen in Component,
Metadata oder Property-/Ownership-/Reference-Snapshots. Alternative SASD-/Binary-/DFM-artige Encodings
bleiben möglich. Ein spezialisierter dependency-freier Writer genügt; kein allgemeines JSON-DOM,
keine JSON-Library, kein Parser und keine Third-Party-Abhängigkeit.

Die normative [Format-Spezifikation EN](../en/UI_DESCRIPTION_FORMAT.md) und
[DE](../de/UI_DESCRIPTION_FORMAT.md) definiert kanonisches JSON v1, `.sasd-ui.json`,
`format: "sasd.ui-description"` und positive Ganzzahl `formatVersion: 1`.
Format- und Toolkit-Version sind unabhängig. Inkompatible Änderungen brauchen eine neue Version;
ein späterer Reader muss sie explizit prüfen und unbekannte Versionen nicht still interpretieren.

Alle persistenten Werte einschließlich Defaults werden explizit gespeichert, damit spätere
Toolkit-Defaults alte Semantik nicht ändern. `is_default` bleibt ausgeschlossenes Tooling-Metadatum;
keine neue Default-Abfrage. Acht C++-Kind-Collections werden zu einer Property-Liste in
base-to-derived-Metadata- und direkter Descriptor-Reihenfolge (items vor selectedIndex).
Rekonstruktion braucht dennoch eine eigene Dependency-Policy. Ownership bleibt einmalig geordnet;
Reference-Edges separat in Capture-Reihenfolge mit unveränderten IDs. Keine Pointer oder redundanten
Visual-Parent-Angaben.

Property-Kinds, elf Reference-Kinds und 17 Color-Werte verwenden stabile Symbole der Spezifikation,
niemals Enum-Ordinals. Dies erfüllt das Color-Follow-up von ADR 0185, ohne dessen Runtime-Entscheidung
umzuschreiben: unbekannte Color-Werte bleiben zur Laufzeit legal, haben aber kein v1-Symbol und
verwerfen Encoding. Unbekannte Kinds sowie fehlende/doppelte Capture-Werte schlagen ebenfalls
geschlossen fehl. Jeder persistente String muss gültiges UTF-8 sein; ungültige Bytes verwerfen alles,
ohne Sanitizing, Ersetzung oder Auslassung. Allocation-Fehler propagieren normal.

Feste Key-/Array-/Property-/Reference-Reihenfolge, zwei Leerzeichen, ausschließlich LF und ein finales
LF sichern deterministische Git-Diffs ohne Host-Metadaten. Exakte Dezimalinteger durch `std::to_chars`
vermeiden Locale-/Float-Abhängigkeit und JavaScript-Safe-Integer-Kürzung. Vollständige Golden-Bytes
und Tests für Symbole, Defaults, UTF-8, Escapes, Integergrenzen, Determinismus und Capture-Fehler
sichern den Vertrag.

### Grenzen und Folgen

Model-/SelectionModel-Identität und Edges werden erfasst, Application-Payloads und aktuelle Selection
nicht. Transienter, abgeleiteter, Backend- und ausführbarer Zustand bleibt gemäß ADR 0177 ausgeschlossen:
kein Fokus/Hover/Pressed/Capture, Caret/Preview/Expansion/Viewport, Layout-Cache, Handle, Callback oder
Observer. Keine Resource-IDs/-Systeme, File-I/O, Parser, JSON Schema, Deserializer, Factory,
Reference-Resolver oder Rekonstruktion. Schema erst bei konkretem Reader-/Tooling-Bedarf.
UI-Description-Serialisierung bleibt bis zur Rekonstruktion insgesamt offen; nur Encoding ist fertig.

### Lokale Validierung

Ausgangs-main war `a4fc034154c1de61aa9ae13007c442da66045fd5`, Branch
`codex/m8-canonical-json-ui-description-v1`. Strikte GCC-12.2-Debug-/Release-Builds liefen mit der
vorhandenen Debian-WSL-Toolchain, Tests/Beispielen und Warnings-as-Errors. Beide finalen vollen
CTest-Läufe bestanden 4/4: Core 493/493, Terminal 321/321, Rendered 129/129 und nativer POSIX-PTY-Smoke.
Der isolierte Encoding-Test bestand 8/8. Beide headless JSON-Demos lieferten identische kanonische
Bytes, mit externem JSON-Inspector geprüft. Release-Installation, Repository-Package-Consumer und
unabhängiger Public-API-Consumer ausschließlich gegen installiertes SASD::UI bestanden.
381 lokale Markdown-Dateilinks ohne fehlendes Ziel und `git diff --check` bestanden; externe URLs
und Fragmente wurden nicht geprüft. Windows/macOS, SDL3, GTK, AppKit und Sanitizer laufen über CI,
nicht lokal. Keine Tests deaktiviert oder Assertions abgeschwächt.

Acht vorhandene Fremddateien (ADRs 0178–0183, ADR 0185 und `..Codex.txt`) wurden mit Ausgangs-Diffs
und SHA-256 geschützt und blieben unangetastet. Das optionale Color-Follow-up für ADR 0185 steht
deshalb hier statt in der lokal geänderten Datei. Build-/Audit-Artefakte bleiben unter
build-codex/slice16 ignoriert. Ein anfangs überlappender Build verursachte eine Archivkollision;
nach Ende beider Prozesse bestanden die geordneten finalen Builds und Tests.
