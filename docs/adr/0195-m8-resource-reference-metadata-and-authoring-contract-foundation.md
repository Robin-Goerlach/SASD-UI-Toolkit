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

### Implementation and validation record

This work session started from the fetched and externally verified `origin/main` commit
`5b15dd7ce9df5622236abdb86fecd58f4d36906d`, which contained M8 Slice 24 through PR #98. The
dedicated implementation branch was `codex/m8-resource-reference-metadata-foundation`. SSH
authentication was unavailable in the environment, so affected fetches and remote writes used
temporary HTTPS plus GitHub CLI credentials without changing the permanent SSH `origin`
configuration.

The implementation added the public `ResourceKind::text_style` identity and fail-closed
`isBuiltInResourceKind()` query. The owning `ResourceReference` factory rejects fabricated kinds
and combines one validated `ResourceKey` with its expected resource family without retaining caller,
dictionary, Component or backend addresses. Separate `ComponentPropertyResourceMetadata` and
`builtInComponentPropertyResourceMetadata()` use an explicit private six-entry policy table. The
query compares complete declaring-type/property/value-kind identity by value, so copied exact
descriptors work while malformed, unknown, wrong-case, wrong-kind and fabricated descriptors fail
closed. Complete-catalog tests establish exactly six resource-capable descriptors and 22
non-resource-capable descriptors.

The headless resource demo now distinguishes a typed dictionary lookup from reference resolution,
shows `sasd.ui.Label.textStyle` as TextStyle-resource-capable and prints `theme.heading` as reference
identity while explicitly deferring resolution and scoping to M8.26. The installed package consumer,
an independent installed public-API consumer and release-install verification exercise both new
public headers and the non-inline Core implementation. Existing inline TextStyle runtime behavior,
ResourceDictionary duplicate handling, property validation and reference metadata, snapshots, canonical
JSON v1 bytes, parser, reconstruction and every backend remain unchanged.

Local validation used the existing WSL GCC 12.2 toolchain. Strict warnings-as-errors Debug and
Release builds succeeded. The Core suite passed 541/541 cases, including the new ResourceKind,
ResourceReference ownership/copy/move, fabricated-kind and complete property-resource-metadata
coverage plus all existing ResourceKey, ResourceDictionary, inline TextStyle, snapshot, JSON writer,
JSON parser and reconstruction cases. The complete configured suite passed 4/4 targets in both
configurations, including Core, Terminal, Rendered and the native POSIX PTY smoke test. The resource
demo, repository package consumer, independent installed public-API consumer, Release install and
source-tree hygiene checks passed. All 420 relative links in 224 Markdown files resolved, and
`git diff --check` passed. Clang and MSVC were not locally available; CI provided the corresponding
cross-platform coverage.

The implementation commit was
[`f2d96cee0fffcf8dad34de165ae60b4cea102334`](https://github.com/Robin-Goerlach/SASD-UI-Toolkit/commit/f2d96cee0fffcf8dad34de165ae60b4cea102334)
on [PR #99](https://github.com/Robin-Goerlach/SASD-UI-Toolkit/pull/99); it was also the final PR head.
All eleven required gates passed for that exact head: Linux GCC, Linux Clang, Linux Clang ASan/UBSan,
Linux SDL3, Windows MSVC, macOS AppleClang, GTK Native, AppKit Native and Linux, Windows and macOS
Release. The PR was merged normally as `0b04f1ebed8d63a75148fe9984d92de5b2618b57`. The complete
post-merge Main CI, GTK Native CI and AppKit Native CI workflows, including the full Release matrix,
then passed for that exact merge commit.

After post-merge CI succeeded, the remote and local feature branches were deleted, local `main` was
fast-forwarded to the merge commit and the remote was pruned. Seven Slice-25-specific build, install,
consumer and link-check artifacts were removed; unrelated build directories were retained. Fourteen
pre-existing modified ADR files, including the additionally detected ADR 0194 change, plus the
untracked `..Codex.txt` remained byte-for-byte equal to all 15 pre-work SHA-256 records. Historical
PRs #30, #32 and #63 remained open and were inspected read-only. No old remote branch was deleted,
and no release or tag was created. The visible macOS/Cocoa M3 gate remains open. M8 Slice 25 is
complete; M8.26 resource resolution and scoping was not started.

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

### Implementierungs- und Validierungsprotokoll

Dieser Arbeitsabschnitt begann auf dem gefetchten und extern verifizierten `origin/main`-Commit
`5b15dd7ce9df5622236abdb86fecd58f4d36906d`, der M8 Slice 24 über PR #98 enthielt. Der dedizierte
Implementierungsbranch hieß `codex/m8-resource-reference-metadata-foundation`. Da in der Umgebung
keine SSH-Authentifizierung verfügbar war, verwendeten betroffene Fetches und Remote-Schreibvorgänge
temporäres HTTPS mit GitHub-CLI-Credentials, ohne die permanente SSH-`origin`-Konfiguration zu
verändern.

Die Implementierung ergänzte die öffentliche Identität `ResourceKind::text_style` und die
fail-closed Query `isBuiltInResourceKind()`. Die besitzende `ResourceReference`-Factory lehnt
fabrizierte Kinds ab und verbindet einen validierten `ResourceKey` mit seiner erwarteten Resource-
Familie, ohne Caller-, Dictionary-, Component- oder Backend-Adressen zu speichern. Getrennte
`ComponentPropertyResourceMetadata` und `builtInComponentPropertyResourceMetadata()` verwenden eine
explizite private Policy-Tabelle mit sechs Einträgen. Die Query vergleicht die vollständige
Declaring-Type-/Property-/Value-Kind-Identität per Value. Kopierte exakte Deskriptoren funktionieren;
malformed, unbekannte, wrong-case, wrong-kind und fabrizierte Deskriptoren arbeiten fail-closed.
Complete-Catalog-Tests belegen exakt sechs resource-capable und 22 non-resource-capable
Deskriptoren.

Die headless Resource-Demo unterscheidet nun einen typisierten Dictionary-Lookup von Reference-
Resolution, zeigt `sasd.ui.Label.textStyle` als TextStyle-resource-capable und gibt `theme.heading`
als Reference-Identität aus, während Resolution und Scoping ausdrücklich auf M8.26 verschoben
bleiben. Der installierte Package-Consumer, ein unabhängiger installierter Public-API-Consumer und
die Release-Install-Prüfung verwenden beide neuen öffentlichen Header und die nicht-inline Core-
Implementierung. Bestehendes Inline-TextStyle-Runtime-Verhalten, ResourceDictionary-Duplicate-
Semantik, Property-Validation- und Reference-Metadaten, Snapshots, kanonische JSON-v1-Bytes, Parser,
Rekonstruktion und sämtliche Backends blieben unverändert.

Die lokale Validierung verwendete die vorhandene WSL-GCC-12.2-Toolchain. Strikte Warnings-as-errors-
Debug- und -Release-Builds waren erfolgreich. Die Core-Suite bestand 541/541 Fälle einschließlich
der neuen ResourceKind-, ResourceReference-Ownership-/Copy-/Move-, Fabricated-Kind- und vollständigen
Property-Resource-Metadaten-Abdeckung sowie aller bestehenden ResourceKey-, ResourceDictionary-,
Inline-TextStyle-, Snapshot-, JSON-Writer-, JSON-Parser- und Rekonstruktionsfälle. Die vollständige
konfigurierte Suite bestand in beiden Konfigurationen 4/4 Targets einschließlich Core, Terminal,
Rendered und nativem POSIX-PTY-Smoke-Test. Resource-Demo, Repository-Package-Consumer, unabhängiger
installierter Public-API-Consumer, Release-Install- und Source-Tree-Hygiene-Prüfung bestanden. Alle
420 relativen Links in 224 Markdown-Dateien wurden aufgelöst; `git diff --check` bestand. Clang und
MSVC waren lokal nicht verfügbar; CI lieferte die entsprechende Cross-Platform-Abdeckung.

Der Implementierungscommit war
[`f2d96cee0fffcf8dad34de165ae60b4cea102334`](https://github.com/Robin-Goerlach/SASD-UI-Toolkit/commit/f2d96cee0fffcf8dad34de165ae60b4cea102334)
auf [PR #99](https://github.com/Robin-Goerlach/SASD-UI-Toolkit/pull/99); er war zugleich der finale
PR-Head. Alle elf erforderlichen Gates bestanden für exakt diesen Head: Linux GCC, Linux Clang,
Linux Clang ASan/UBSan, Linux SDL3, Windows MSVC, macOS AppleClang, GTK Native, AppKit Native sowie
Linux-, Windows- und macOS-Release. Der PR wurde regulär als
`0b04f1ebed8d63a75148fe9984d92de5b2618b57` gemergt. Die vollständigen Post-Merge-Workflows Main CI,
GTK Native CI und AppKit Native CI einschließlich kompletter Release-Matrix bestanden anschließend
für exakt diesen Merge-Commit.

Nach grüner Post-Merge-CI wurden Remote- und lokaler Feature-Branch gelöscht, der lokale `main` auf
den Merge-Commit fast-forwarded und der Remote geprunt. Sieben Slice-25-spezifische Build-, Install-,
Consumer- und Linkprüfungsartefakte wurden entfernt; unabhängige Buildverzeichnisse blieben erhalten.
Vierzehn bereits veränderte ADR-Dateien einschließlich der zusätzlich erkannten Änderung an ADR 0194
sowie das ungetrackte `..Codex.txt` stimmten weiterhin bytegenau mit allen 15 vor Arbeitsbeginn
erfassten SHA-256-Werten überein. Die historischen PRs #30, #32 und #63 blieben offen und wurden nur
read-only geprüft. Kein alter Remote-Branch wurde gelöscht und weder Release noch Tag erzeugt. Das
sichtbare macOS/Cocoa-M3-Gate bleibt offen. M8 Slice 25 ist vollständig abgeschlossen; M8.26 Resource
Resolution und Scoping wurde nicht begonnen.
