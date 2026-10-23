# ADR 0178: M8 Numeric Property Value-Kind Foundation

- Status: Accepted
- Date: 2026-10-23

## Predecessors

- [0172: Property schema](0172-m8-component-property-metadata-schema-foundation.md)
- [0173: Typed runtime access](0173-m8-typed-runtime-property-value-access-foundation.md)
- [0174: Effective defaults](0174-m8-effective-property-default-metadata-foundation.md)
- [0176: Owned snapshots](0176-m8-built-in-component-property-snapshot-foundation.md)
- [0177: State coverage policy](0177-m8-serializable-ui-description-state-coverage-policy.md)

## English

### Context

The verified Slice-7 baseline has 26 built-in types and ten boolean/string property descriptors.
The state coverage policy identifies seven existing numeric layout properties as durable authoring
configuration. Their C++ APIs use `Coordinate` (`std::int32_t`) and `std::size_t`; these implementation
types must not define the public tooling value representation.

### Decision

Add `ComponentPropertyValueKind::signed_integer` and `unsigned_integer`, with separate typed
`std::int64_t` and `std::uint64_t` APIs. The catalog grows to seventeen descriptors:

| Declaring type | Property | Kind | Default |
|---|---|---|---:|
| `sasd.ui.HBox` | `spacing` | signed_integer | 0 |
| `sasd.ui.VBox` | `spacing` | signed_integer | 0 |
| `sasd.ui.GridLayout` | `columnCount` | unsigned_integer | 1 |
| `sasd.ui.GridLayout` | `columnSpacing` | signed_integer | 0 |
| `sasd.ui.GridLayout` | `rowSpacing` | signed_integer | 0 |
| `sasd.ui.FormLayout` | `columnSpacing` | signed_integer | 1 |
| `sasd.ui.FormLayout` | `rowSpacing` | signed_integer | 0 |

`readBuiltInSignedIntegerProperty` / `readBuiltInUnsignedIntegerProperty` return typed optionals;
`writeBuiltInSignedIntegerProperty` / `writeBuiltInUnsignedIntegerProperty` return access success,
including same-value writes. Validate the complete descriptor identity by value against the immutable
catalog and check Component compatibility internally. A copied valid descriptor is valid; sibling
properties with the same local ID remain distinct. No HBox descriptor may address VBox spacing, nor
may Grid spacing descriptors address Form spacing.

Both conversion directions use C++20 `std::in_range` before casting. An unrepresentable read returns
`std::nullopt`; an unrepresentable write returns false before mutation. Domain validation belongs
exclusively to the semantic setter: representable negative spacing and zero column count propagate
the existing `std::invalid_argument`. No duplicate validation or exception translation is added.

`builtInSignedIntegerPropertyDefault` / `builtInUnsignedIntegerPropertyDefault` validate the exact
target type and descriptor identity, then return immutable constants matching public construction
defaults. They construct no Component.

Snapshots own separate `signed_integer_properties` and `unsigned_integer_properties`, using
`ComponentSignedIntegerPropertySnapshot` / `ComponentUnsignedIntegerPropertySnapshot` entries with
owned `declaring_type_id`, `property_id`, typed `value` and `is_default`. Base-to-derived traversal
and declaration order within each collection remain unchanged. Any failed read or default query
rejects the entire snapshot. Capturing catalogued properties still does not promise serialization
or complete UI-description coverage.

### Alternatives and consequences

Exposing `Coordinate` or `size_t` would couple tooling to layout implementation or host word size.
A generic `PropertyValue`, `std::variant` or `std::any` is unnecessary for these explicit consumers.
Separate typed APIs and collections extend the established boolean/string contract without adding
a universal value abstraction, dependency, backend policy or object graph.

RadioButton properties, ComboBox optional/collection state, compound values, style/resources,
references, ownership reconstruction, validation metadata, serializer, encoding, deserializer,
factory and designer remain outside this slice. M8 remains active; serialization remains open.
The recommended next slice is **M8 Slice 9 – RadioButton Scalar Property Coverage** (`text`,
`selected`), without implementing it here. The macOS/Cocoa M3 gate remains open. No release or tag.

## Deutsch

### Kontext

Der verifizierte Slice-7-Ausgangsstand enthält 26 Built-in-Typen und zehn Boolean-/String-
Deskriptoren. Die Coverage-Policy klassifiziert sieben bestehende numerische Layout-Properties als
dauerhafte Authoring-Konfiguration. Die C++-APIs verwenden `Coordinate` (`std::int32_t`) und
`std::size_t`; diese Implementierungstypen sollen nicht die öffentliche Tooling-Repräsentation sein.

### Entscheidung

`signed_integer` und `unsigned_integer` erweitern `ComponentPropertyValueKind`. Separate typisierte
APIs verwenden `std::int64_t` bzw. `std::uint64_t`. Die Tabelle oben definiert alle sieben neuen
Properties samt deklarierendem Typ, Kind und Default; der Katalog wächst auf siebzehn Deskriptoren.

Die Signed-/Unsigned-Read-Funktionen liefern typisierte Optionals, die Write-Funktionen melden
Zugriffserfolg einschließlich Same-Value-Writes. Vollständige Deskriptoridentität wird nach Wert
gegen den unveränderlichen Katalog geprüft, Component-Kompatibilität intern sicher festgestellt.
Kopierte gültige Deskriptoren bleiben gültig. Gleiche lokale Property-IDs verschiedener Typen sind
verschieden: HBox adressiert kein VBox-Spacing, Grid-Spacing adressiert kein Form-Spacing.

Beide Konvertierungsrichtungen prüfen vor dem Cast mit C++20 `std::in_range` die Darstellbarkeit.
Nicht darstellbare Reads liefern `std::nullopt`; Writes liefern false ohne Mutation. Domain-
Validierung bleibt ausschließlich beim semantischen Setter. Darstellbares negatives Spacing und
Spaltenzahl null propagieren dessen bestehende `std::invalid_argument`; keine doppelte Prüfung und
keine Exception-Übersetzung. Numerische Default-Queries prüfen exakten Zieltyp und vollständige
Property-Identität und liefern unveränderliche Konstanten ohne Component-Instanziierung.

Snapshots besitzen getrennte `signed_integer_properties` und `unsigned_integer_properties` mit
`ComponentSignedIntegerPropertySnapshot` bzw. `ComponentUnsignedIntegerPropertySnapshot`. Jeder
Eintrag besitzt `declaring_type_id`, `property_id`, typisierten `value` und `is_default`. Basis-zu-
Derived-Traversierung und Deklarationsreihenfolge innerhalb jeder Collection bleiben erhalten.
Fehlt ein Read oder Default, scheitert der gesamte Snapshot. Erfolg bedeutet weiterhin nur die
Erfassung katalogisierter Properties, keine vollständige UI-Description oder Serialisierung.

### Alternativen und Folgen

`Coordinate` oder `size_t` als Tooling-Typ würden Layout-Implementierung bzw. Host-Wortbreite
exponieren. Ein generischer `PropertyValue`, `std::variant` oder `std::any` ist für diese Consumer
unnötig. Getrennte APIs und Collections erweitern den bisherigen Vertrag ohne universelle Value-
Abstraktion, neue Abhängigkeit, Backend-Policy oder Objektgraph.

RadioButton-Properties, optionale/Collection-Werte der ComboBox, Compound-Werte, Style/Resources,
References, Ownership-Rekonstruktion, Validation-Metadaten, Serializer, Encoding, Deserializer,
Factory und Designer bleiben außerhalb. M8 bleibt aktiv und Serialisierung offen. Empfohlen wird
**M8 Slice 9 – RadioButton Scalar Property Coverage** (`text`, `selected`), hier nicht implementiert.
Das macOS/Cocoa-M3-Gate bleibt offen. Kein Release und kein Tag.

## Implementation and validation record

### Recovery and verified baseline

The recovery inspected working/staged diffs, local and remote branches, the recent and property-
related history, reflog and existing pull requests before editing source. No previous Slice-8 work
or `codex/m8-numeric-property-value-kinds` branch existed locally or remotely. The only untracked
file was the unrelated `..Codex.txt`, which was preserved unchanged throughout the work.

Remote `main` still pointed to `3295808a463970930c922b28f93d0e1a1fdda6b1`, matching the supplied
baseline. Its CI, GTK Native CI and AppKit Native CI were all successful. Fetch/prune and
fast-forward-only synchronization confirmed that no newer main state needed adoption. SSH Git
access failed because no usable public key was available; Git operations then used a temporary
HTTPS URL rewrite and the existing GitHub CLI credentials. No permanent remote URL change,
history reset, history rewrite or software installation was needed.

### Implementation and review

Source review confirmed ten existing property descriptors, the seven layout consumers/defaults in
the table above and 0178 as the next available ADR number. The implementation was committed as
`9262659b6565b908d9f47fe639cb11290e7ef10a` on branch
`codex/m8-numeric-property-value-kinds` and published as
[PR #81](https://github.com/Robin-Goerlach/SASD-UI-Toolkit/pull/81). This remained the final PR head.

The twenty changed files covered the four property header/implementation pairs, the existing
metadata schema tests, the new numeric regression tests and their CMake registration, the headless
Component Metadata demo, this ADR and its index, the Slice-7 coverage matrix, both architecture
documents, both roadmaps and the changelog. The catalog grew from ten to seventeen descriptors;
the built-in type catalog remained at 26. No backend-specific or semantic layout setter change
was required.

Tests covered all seven descriptors and defaults; declaring type and sibling identity; copied
valid descriptors; typed reads/writes and same-value writes; wrong kind, malformed descriptor and
incompatible Component rejection; signed narrowing in both directions; portable unsigned limits;
propagation of setter exceptions for negative spacing and zero columns; and owned HBox/VBox,
GridLayout and FormLayout snapshots, including default flags, order and source lifetime. Existing
boolean/string snapshot tests were left unchanged. Working/staged diffs were reviewed, and only
task files were staged; build outputs and unrelated notes were excluded.

### Local validation

The existing Debian WSL GCC 12.2.0 / CMake toolchain was used because CMake was not on the Windows
PATH. Both configurations enabled tests, examples and warnings as errors:

- Debug: `build-codex-m8-numeric`, configured with `CMAKE_BUILD_TYPE=Debug`;
- Release: `out/m8-numeric-release`, configured with `CMAKE_BUILD_TYPE=Release`;
- final builds: `cmake --build <directory> --parallel 4`;
- focused Debug Core CTest and four standalone numeric regression cases: successful;
- full `ctest --test-dir <directory> --output-on-failure`: 4/4 targets successful in each
  configuration (Core, Terminal, Rendered and native POSIX PTY smoke);
- Debug and Release `sasd_ui_component_metadata_demo`: successful, including numeric schema,
  runtime access, defaults and snapshot values/default flags;
- `git diff --check`, staged diff checks, ADR numbering/index and 209 relative documentation
  links: successful, with no broken links.

Final builds included the last review changes. The complete Debug CTest suite was also rerun
successfully before the final remote branch deletion. No local Windows/macOS, sanitizer, SDL3,
GTK or AppKit execution is claimed; those configurations were validated by CI.

### PR, merge and exact-SHA post-merge validation

All eleven jobs below succeeded for both the final PR head
`9262659b6565b908d9f47fe639cb11290e7ef10a` and the exact merge commit
`b52c7e1655102b95498b7d5a8647727f659729ec`:

| Job | Final PR head | Exact merge SHA |
|---|---|---|
| Linux GCC | success | success |
| Linux Clang | success | success |
| Linux Clang ASan + UBSan | success | success |
| Linux SDL3 adapter | success | success |
| Windows MSVC | success | success |
| macOS AppleClang | success | success |
| GTK4 Native Host | success | success |
| macOS AppKit Native | success | success |
| Linux Release | success | success |
| Windows Release | success | success |
| macOS Release | success | success |

PR-head workflow evidence: [CI run 39916757510](https://github.com/Robin-Goerlach/SASD-UI-Toolkit/actions/runs/39916757510),
[GTK run 39916757512](https://github.com/Robin-Goerlach/SASD-UI-Toolkit/actions/runs/39916757512)
and [AppKit run 39916757556](https://github.com/Robin-Goerlach/SASD-UI-Toolkit/actions/runs/39916757556).

After rechecking the unchanged head, all eleven successful checks and GitHub's clean/mergeable
state, PR #81 was merged on 2026-10-23 at 07:38:28 UTC by normal two-parent merge commit
`b52c7e1655102b95498b7d5a8647727f659729ec`. Its parents are the verified main baseline and the
final implementation commit above. Its source tree is identical to the tested PR head.

Exact merge-SHA workflow evidence: [CI run 39919119169](https://github.com/Robin-Goerlach/SASD-UI-Toolkit/actions/runs/39919119169),
[GTK run 39919119211](https://github.com/Robin-Goerlach/SASD-UI-Toolkit/actions/runs/39919119211)
and [AppKit run 39919119189](https://github.com/Robin-Goerlach/SASD-UI-Toolkit/actions/runs/39919119189).
All three workflows and all eleven jobs were checked for successful completion on that exact SHA
before cleanup. The final macOS Release result was confirmed at 08:00:27 UTC.

### Cleanup and stop boundary

After full post-merge success, the CI evidence was recorded in PR #81, the merged ancestry and
unchanged remote branch tip were verified, and only the Slice-8 branch was deleted remotely.
Local `main` was fast-forwarded to the merge SHA, then the local Slice-8 branch was deleted with
normal `git branch -d`. Local and remote absence were verified. Tracked files were clean; the
pre-existing untracked `..Codex.txt` remained unchanged. Local build/log artifacts stayed outside
the committed tree.

PRs #30, #32 and #63 were not modified. No release or tag was created. The macOS/Cocoa M3 gate
remains open. M8 stays active, and serialization remains open. Work stopped after Slice 8;
**M8 Slice 9 – RadioButton Scalar Property Coverage** (`RadioButton.text`, `RadioButton.selected`)
was recommended only and was not implemented. This record documents that completed implementation
and its validation; it adds no new API or runtime behavior.

## Implementierungs- und Validierungsprotokoll

### Recovery und Ausgangsstand

Vor Änderungen wurden Working-/Staging-Diffs, lokale und Remote-Branches, aktuelle und Property-
bezogene History, Reflog und vorhandene PRs geprüft. Es gab weder vorherige Slice-8-Arbeit noch den
erwarteten Branch `codex/m8-numeric-property-value-kinds`. Die einzige unversionierte Datei
`..Codex.txt` blieb unverändert erhalten. Remote `main` entsprach weiterhin dem vorgegebenen SHA
`3295808a463970930c922b28f93d0e1a1fdda6b1`; alle drei zugehörigen CI-Workflows waren grün.
Fetch/Prune und Fast-Forward-only-Synchronisierung bestätigten den Stand. Wegen fehlenden nutzbaren
SSH-Schlüssels wurden Git-Operationen über eine temporäre HTTPS-Umschreibung und vorhandene
GitHub-CLI-Zugangsdaten ausgeführt. Keine dauerhafte Remote-URL-Änderung, kein Reset, kein History-
Rewrite und keine Softwareinstallation waren erforderlich.

### Umsetzung und lokale Prüfung

Der Source-Abgleich bestätigte zehn Ausgangsdeskriptoren, die sieben Layout-Properties samt
Defaults und die freie ADR-Nummer 0178. Die Umsetzung entstand im Commit
`9262659b6565b908d9f47fe639cb11290e7ef10a` auf dem erwarteten Slice-Branch und wurde als
[PR #81](https://github.com/Robin-Goerlach/SASD-UI-Toolkit/pull/81) veröffentlicht. Der finale
PR-Head blieb unverändert. Zwanzig Dateien umfassten die vier Property-Header-/Source-Paare,
Schema-/Numeric-Tests und CMake, Metadata-Demo, ADR/Index, Coverage-Matrix, deutsch/englische
Architektur und Roadmaps sowie Changelog. Der Property-Katalog wuchs auf siebzehn Deskriptoren;
der Typkatalog blieb bei 26. Backend-Code und semantische Layout-Setter mussten nicht geändert
werden.

Die Tests prüfen alle sieben Deskriptoren und Defaults, deklarierenden Typ und Sibling-Identität,
kopierte Deskriptoren, typisierte Reads/Writes einschließlich Same-Value-Writes, falsche Kinds,
ungültige Deskriptoren und inkompatible Components, beide Signed-Narrowing-Grenzen, portable
Unsigned-Grenzen, Setter-Exceptions sowie besitzende Layout-Snapshots mit Reihenfolge, Default-
Flags und Lebenszeitunabhängigkeit. Bestehende Boolean-/String-Snapshot-Tests blieben unverändert.
Nur Task-Dateien wurden nach Diff-Review gestaged; Build-Artefakte und fremde Notizen blieben außen.

Mangels CMake im Windows-PATH kam die vorhandene Debian-WSL-Toolchain mit GCC 12.2.0 und CMake
zum Einsatz. Debug und Release wurden mit Tests, Beispielen und Warnings-as-Errors gebaut, jeweils
einschließlich der letzten Review-Anpassungen. Die oben genannten Build-Verzeichnisse und Befehle
gelten für beide Sprachfassungen. Fokussierte Core-/Numeric-Tests, jeweils 4/4 vollständige CTest-
Ziele und beide Metadata-Demos waren erfolgreich. Diff-, ADR-/Index- und 209 relative Link-Prüfungen
waren grün. Vor der abschließenden Remote-Branch-Löschung wurde die Debug-CTest-Suite nochmals
erfolgreich ausgeführt. Lokale Windows-/macOS-/Sanitizer-/SDL3-/GTK-/AppKit-Läufe werden nicht
behauptet; dafür stehen die CI-Nachweise.

### CI, Merge, Bereinigung und Abschluss

Die vollständige Tabelle und die sechs Workflow-Links oben dokumentieren jeweils elf erfolgreiche
Jobs für den finalen PR-Head und exakt den Merge-SHA. Nach erneuter Prüfung von Head, vollständiger
grüner Matrix und sauberem Merge-Status wurde PR #81 am 23.10.2026 um 07:38:28 UTC mit dem normalen
Zwei-Eltern-Merge `b52c7e1655102b95498b7d5a8647727f659729ec` gemergt. Die Eltern entsprechen dem
verifizierten Main-Ausgangsstand und dem Implementierungscommit. Der Quellbaum ist identisch mit
dem getesteten PR-Head.

Main CI einschließlich aller Release-Jobs, GTK Native CI und AppKit Native CI wurden danach für
exakt diesen Merge-SHA vollständig abgewartet. Der letzte macOS-Release-Erfolg wurde um 08:00:27 UTC
bestätigt. Erst nach allen elf grünen Jobs wurden die Ergebnisse im PR dokumentiert, Merge-Ancestry
und unveränderter Remote-Branch-Tip geprüft und der Slice-Branch remote gelöscht. Lokales `main`
wurde auf den Merge-SHA vorgespult, anschließend der lokale Slice-Branch regulär mit `git branch -d`
gelöscht. Das Fehlen beider Branches wurde verifiziert. Versionierte Dateien waren sauber;
`..Codex.txt` blieb unverändert unversioniert, lokale Build-/Log-Artefakte wurden nicht committet.

PRs #30, #32 und #63 blieben unverändert. Kein Release, kein Tag; das macOS/Cocoa-M3-Gate bleibt
offen. M8 bleibt aktiv und Serialisierung offen. Nach Slice 8 wurde gestoppt. **M8 Slice 9 –
RadioButton Scalar Property Coverage** mit `RadioButton.text` und `RadioButton.selected` wurde nur
empfohlen und nicht implementiert. Dieses Protokoll ergänzt ausschließlich den Nachweis der
abgeschlossenen Arbeit; es führt keine weitere API oder Runtime-Funktionalität ein.
