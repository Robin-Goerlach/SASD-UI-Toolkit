# ADR-Dokumentationskonsolidierung — 24. November 2026

Status: abgeschlossen und gemergt

Die [englische Fassung](../en/ADR_DOCUMENTATION_CONSOLIDATION_2026_11_24.md) ist das kanonische
Dokument. Dieser deutsche Bericht enthält dieselben technischen Aussagen.

## Zusammenfassung

Am 24. November 2026 schloss das Projekt eine gezielte Konsolidierung lokal vorbereiteter Ergänzungen
zu 16 Architecture Decision Records (ADRs) ab. Die Ergänzungen enthielten wertvolle
Implementierungsprovenienz, aber auch umfangreiche Codex-Sitzungsdetails: lokale Buildpfade,
temporäre Branches, Authentifizierungs-Workarounds, Shell-Diagnosen, Runner-Wartezeiten, wiederholte
Cleanup-Berichte und Selbstbezüge auf den damals uncommitteten Arbeitsstand. Ein solches operatives
Protokoll innerhalb akzeptierter ADRs hätte die Entscheidungen verdeckt und ihre langfristige Pflege
erschwert.

Die Arbeiten trennten daher drei Arten von Informationen:

1. **dauerhafte Architekturentscheidungen**, die unverändert blieben;
2. **Implementierungsnachweise**, die dort knapp und überprüfbar erhalten wurden, wo sie die
   Realisierung einer Entscheidung erklären; und
3. **historische Sitzungsprotokolle**, die entfernt wurden, wenn sie keine dauerhafte technische
   Evidenz beitrugen.

Der Konsolidierung ging ein vollständiger READ-ONLY Review voraus. Er bewahrte die bestehende
Bedeutung und historische Gültigkeit jeder ADR, korrigierte drei Dokumentationsfehler, verdichtete das
vorgeschlagene Material auf 364 gezielte Ergänzungen in 15 ADRs und ließ ADR 0178 identisch zum
versionierten Ausgangsstand. Das Ergebnis wurde über
[Pull Request #113](https://github.com/Robin-Goerlach/SASD-UI-Toolkit/pull/113) übernommen, ohne
Produktcode, Tests, Buildkonfiguration, öffentliche API oder Release-Status zu verändern.

## Verifizierter Ausgangszustand

Der geprüfte Arbeitsstand basierte auf Commit
[`8aabcfc9b97a535fc346fb794d0eb4fa8a14a2c1`](https://github.com/Robin-Goerlach/SASD-UI-Toolkit/commit/8aabcfc9b97a535fc346fb794d0eb4fa8a14a2c1).
Er enthielt:

- 16 lokal modifizierte ADR-Dateien;
- 2.647 zusätzliche Dokumentationszeilen ohne ursprüngliche Löschungen;
- eine gesonderte unversionierte Benutzerdatei namens `..Codex.txt`; und
- eine externe Sicherung der ursprünglichen Dokumentationsarbeit.

Keine vorhandene Ergänzung wurde ersetzt, bevor Sicherung und READ-ONLY Review verifiziert waren. Der
Review verglich den vollständigen hinzugefügten Text mit der aktuellen C++20-Implementierung, den
Tests, akzeptierten ADRs, der Git-Historie und der zweisprachigen Projektdokumentation. Dadurch wurde
bestimmt, welche Inhalte technisch korrekt waren, welche wertvoll, aber zu operativ oder redundant
waren und welche nicht Teil des dauerhaften ADR-Bestands werden sollten.

## Ergebnis des READ-ONLY Reviews

| Klassifikation | Anzahl | Bedeutung |
|---|---:|---|
| A — Unverändert übernehmen | 1 | Korrekt, nützlich und für die dauerhafte Projektdokumentation geeignet. |
| B — Fachlich überarbeiten | 14 | Wertvolle Evidenz, die konkrete Korrekturen, historische Einordnung oder das Entfernen von Wiederholungen und Sitzungsdetails erforderte. |
| C — Ergänzungen nicht übernehmen | 1 | Die lokalen Ergänzungen waren kein dauerhaftes ADR-Material; die versionierte ADR blieb maßgeblich. |
| D — Ungeklärt | 0 | Nach dem Review fehlte für keinen Punkt ausreichende Evidenz. |

Die Bewertung berücksichtigte die technische Übereinstimmung mit Implementierung und Tests, die
Konsistenz mit akzeptierten Entscheidungen, historische Genauigkeit, Belege für CI-/Commit-/Release-
Aussagen, die Gleichwertigkeit von Englisch und Deutsch, Dopplungen und den langfristigen
Dokumentationswert. Neuere Implementierung und Tests dienten als Evidenz, ohne Entscheidungen
rückwirkend umzudeuten, die zum Zeitpunkt ihrer Annahme gültig waren.

## Durchgeführte Konsolidierung

Pull Request #113 änderte 15 ADR-Dateien mit 364 Ergänzungen. ADR 0178 weist gegenüber dem
versionierten Ausgangscommit keinen Unterschied auf. ADR 0188 behielt ihre vollständige, fachlich
korrekte Ergänzung. Die übrigen 14 Ergänzungen der Kategorie B wurden zu knappen zweisprachigen
Provenienzabschnitten überarbeitet.

| ADR | Review | Behandlung | Ergebnis in PR #113 |
|---|---|---|---:|
| [0178](../adr/0178-m8-numeric-property-value-kind-foundation.md) | C | Nur die uncommitteten Ergänzungen entfernt; die bereits versionierte ADR unverändert erhalten. | 0 Zeilen |
| [0179](../adr/0179-m8-radio-button-scalar-property-coverage.md) | B | Implementierungs- und Testprovenienz der RadioButton-Scalar-Properties verdichtet. | 24 Ergänzungen |
| [0180](../adr/0180-m8-description-node-identity-ordered-ownership-snapshot.md) | B | Provenienz zu Node-Identität, geordnetem Ownership und Snapshots verdichtet. | 20 Ergänzungen |
| [0181](../adr/0181-m8-built-in-component-cross-reference-edge-snapshot-foundation.md) | B | Cross-Reference-Edge- und Snapshot-Evidenz erhalten. | 21 Ergänzungen |
| [0182](../adr/0182-m8-selection-model-component-lifecycle-and-reference-coverage.md) | B | SelectionModel-Lebenszyklus- und Reference-Coverage-Evidenz erhalten. | 20 Ergänzungen |
| [0183](../adr/0183-m8-combo-box-collection-and-optional-property-coverage.md) | B | ComboBox-Collection- und Optional-Property-Evidenz erhalten. | 21 Ergänzungen |
| [0185](../adr/0185-m8-inline-text-style-authoring-persistence-coverage.md) | B | Inline-TextStyle-Authoring- und Persistenz-Evidenz erhalten. | 21 Ergänzungen |
| [0186](../adr/0186-m8-canonical-json-ui-description-encoding-v1.md) | B | Implementierungs- und Validierungsprovenienz für kanonisches JSON v1 erhalten. | 20 Ergänzungen |
| [0187](../adr/0187-m8-json-v1-parser-validated-document-model.md) | B | Parser-/Dokumentmodell-Evidenz verdichtet und Root-Identifier-Formulierung korrigiert. | 22 Ergänzungen |
| [0188](../adr/0188-m8-built-in-ui-description-reconstruction.md) | A | Vollständige fachlich korrekte Rekonstruktions-Evidenz bewahrt. | 42 Ergänzungen |
| [0190](../adr/0190-m8-single-property-validation-normalization-metadata-foundation.md) | B | Provenienz der Validierungs- und Normalisierungsmetadaten erhalten. | 22 Ergänzungen |
| [0192](../adr/0192-m8-built-in-reference-and-graph-validation-metadata-foundation.md) | B | Provenienz der Reference- und Graphvalidierungsmetadaten erhalten. | 21 Ergänzungen |
| [0193](../adr/0193-m8-property-authoring-category-metadata-foundation.md) | B | Property-Authoring-Category-Evidenz erhalten. | 20 Ergänzungen |
| [0194](../adr/0194-m8-resource-key-and-typed-text-style-resource-dictionary-foundation.md) | B | ResourceKey- und typisierte TextStyle-Dictionary-Evidenz erhalten. | 20 Ergänzungen |
| [0199](../adr/0199-m8-headless-tooling-integration-contract.md) | B | Tooling-Integration-Abschluss verdichtet und Sitzungsgrenze historisch markiert. | 28 Ergänzungen |
| [0202](../adr/0202-beta-and-canonical-product-repository-roles.md) | B | Geschützten PR-#112-Zwischenstand vom später verifizierten Merge-Abschluss getrennt. | 42 Ergänzungen |

Erhalten blieben dauerhafte Architekturverträge, Implementierungsprovenienz, wichtige Begründungen
und Ausschlüsse, relevante Tests, verifizierte Commit-/PR-Bezüge und bestätigte CI-Ergebnisse.
Entfernt wurden temporäre lokale Verzeichnisse, vorübergehende Branches, Credential- und Shell-
Workarounds, zwischenzeitliche Kommandofehler, Hashes geschützter fremder Dateien, wiederholte
Runner-Verzögerungen, doppelte Cleanup-Berichte, Wiederholungen bestehenden normativen ADR-Texts und
Aussagen über die damals laufende uncommittete Sitzung. Solche Fakten können während der Ausführung
nützlich sein, erklären oder begrenzen aber nicht die Architektur und würden in akzeptierten ADRs
schnell veralten.

## Behobene Dokumentationsprobleme

### ADR 0187: numerischer Root-Identifier

Die deutsche Provenienz benennt den Dokument-Root jetzt als numerische Root-ID `0`. Dadurch wird die
missverständliche Formulierung „Root null“ vermieden, die als Nullwert statt als ganzzahliger
Node-Identifier gelesen werden konnte. Die zugrunde liegende Parser- und Dokumententscheidung wurde
nicht verändert.

### ADR 0199: historische M8.29-/M8.30-Grenze

Die Aussage, M8.30 habe noch nicht begonnen, ist ausdrücklich als Stand der damaligen Arbeitssitzung
gekennzeichnet. Sie wird nicht als heutiger Projektstand dargestellt. Der spätere M8-Abschluss ist in
[ADR 0200](../adr/0200-m8-developer-productivity-foundations-exit-decision.md) und im
[M8 Exit Review](M8_EXIT_REVIEW.md) dokumentiert.

### ADR 0202: geschützter Zwischenstand und finaler Abschluss

Der Datensatz unterscheidet jetzt chronologisch zwischen:

1. dem geschützten Zwischenstand, in dem Pull Request #112 bewusst nicht gemergt wurde, weil die
   vorliegende Freigabe eine veraltete Head-SHA nannte; und
2. dem späteren finalen Abschluss, nachdem eine aktualisierte Owner-Freigabe vorlag und Pull Request
   #112 erfolgreich gemergt wurde.

Damit bleiben beide historischen Beobachtungen korrekt, statt den früheren Zustand so umzudeuten, als
hätte der Merge bereits stattgefunden.

## Git-Historie und Provenienz

Die Konsolidierung wurde in drei fokussierte Commits gegliedert:

| Zweck | Commit |
|---|---|
| M8-Implementierungsprovenienz | [`c7dcff4abe44824d37ffb9729f0e2a9dcc9b1f18`](https://github.com/Robin-Goerlach/SASD-UI-Toolkit/commit/c7dcff4abe44824d37ffb9729f0e2a9dcc9b1f18) |
| Abschluss der M8-Tooling-Integration | [`7751eef77b2104606708dfe411730944ada21ae8`](https://github.com/Robin-Goerlach/SASD-UI-Toolkit/commit/7751eef77b2104606708dfe411730944ada21ae8) |
| Finaler Merge-Abschluss von Pull Request #112 | [`f3397f2e872f44c55482c50653d6eb190a168029`](https://github.com/Robin-Goerlach/SASD-UI-Toolkit/commit/f3397f2e872f44c55482c50653d6eb190a168029) |

Alle drei Commits sind Vorfahren des verifizierten Merge-Commits. Pull Request
[#113](https://github.com/Robin-Goerlach/SASD-UI-Toolkit/pull/113) wurde am 24. November 2026 um
08:49:28 UTC durch Merge-Commit
[`8e6493ed531d2c84b04059fa79def6520a16fb46`](https://github.com/Robin-Goerlach/SASD-UI-Toolkit/commit/8e6493ed531d2c84b04059fa79def6520a16fb46)
nach `main` übernommen.

## Tests und Qualitätssicherung

### Lokale statische Prüfungen vor dem Konsolidierungscommit

Der Dokumentationsreview verifizierte:

- `git diff --check` ohne Whitespace-Fehler;
- 202 eindeutige und indexierte ADRs;
- 528 relative Markdown-Links in 248 versionierten Markdown-Dateien ohne fehlende Ziele;
- exakt die vorgesehenen 15 ADR-Dateien im Konsolidierungs-PR; und
- eine unveränderte öffentliche C++-API und Produktimplementierung.

Für diese reine Markdown-Konsolidierung wurde kein zusätzlicher lokaler C++-Build und kein CTest-Lauf
durchgeführt. Das bedeutet nicht, dass Produkttests in der CI ausgelassen wurden; es trennt die
lokalen Dokumentationsprüfungen von der automatisierten Build- und Testmatrix des Repositorys.

### Pull-Request-CI

Der exakte Pull-Request-Head `f3397f2e872f44c55482c50653d6eb190a168029` bestand alle elf
erforderlichen Prüfungen: Linux GCC, Linux Clang, Linux Clang ASan + UBSan, Linux SDL3 adapter,
Windows MSVC, macOS AppleClang, GTK4 Native Host, macOS AppKit Native, Linux Release, Windows Release
und macOS Release. Diese Ergebnisse sind an
[Pull Request #113](https://github.com/Robin-Goerlach/SASD-UI-Toolkit/pull/113) gebunden.

### Post-Merge-CI

Die tatsächliche Merge-SHA `8e6493ed531d2c84b04059fa79def6520a16fb46` bestand unabhängig dieselbe
Abdeckung aus elf Jobs:

| Workflow | Lauf | Erfolgreiche Jobs |
|---|---|---|
| Main CI | [45359491339](https://github.com/Robin-Goerlach/SASD-UI-Toolkit/actions/runs/45359491339) | Linux GCC, Linux Clang, Linux Clang ASan + UBSan, Linux SDL3 adapter, Windows MSVC, macOS AppleClang, Linux Release, Windows Release, macOS Release |
| GTK Native CI | [45359491438](https://github.com/Robin-Goerlach/SASD-UI-Toolkit/actions/runs/45359491438) | GTK4 Native Host |
| AppKit Native CI | [45359491500](https://github.com/Robin-Goerlach/SASD-UI-Toolkit/actions/runs/45359491500) | macOS AppKit Native |

Alle drei Workflows wurden mit `success` abgeschlossen und nennen die verifizierte Merge-SHA als Head.

## Datensicherung und Wiederherstellbarkeit

Vor der Bearbeitung wurden die ursprünglichen 2.647 Ergänzungen außerhalb des Git-Repositorys
gesichert. Der dokumentierte Sicherungsumfang enthält:

- einen Patch der ursprünglichen Änderungen;
- vollständige Kopien aller 16 ADR-Dateien;
- eine Kopie der gesonderten Benutzerdatei `..Codex.txt`;
- die Diff-Statistik; und
- ein SHA-256-Prüfsummenmanifest.

Private absolute Windows-Pfade, Credentials und Sicherungsinhalte sind nicht Teil des Repositorys.
Vorhandensein, Dateianzahl, Diff-Statistik und Prüfsummenübereinstimmung der gesondert kopierten
Benutzerdatei wurden verifiziert. Ein erfolgreiches Einspielen des serialisierten Patches wurde
**nicht** nachgewiesen; dieser Bericht behauptet daher nicht, der Patch sei als
Wiederherstellungsmechanismus getestet. Die vollständigen Dateikopien und das Prüfsummenmanifest sind
eigenständige Wiederherstellungsnachweise.

## Praktische Erkenntnisse für zukünftige Dokumentation

Der Review stützt die folgenden Arbeitsempfehlungen. Sie sind Erkenntnisse aus dieser Konsolidierung,
keine neue Architekturentscheidung und kein Ersatz für die bestehende ADR-Policy:

- ADRs sollten dauerhafte Entscheidungen, Begründungen, Alternativen und Konsequenzen bewahren.
- Implementierungsnachweise sollten kurz, präzise und unabhängig überprüfbar sein.
- Lange Codex- oder Shell-Ausführungsprotokolle gehören nicht in akzeptierte ADRs.
- Historische Zwischenstände sollten datiert oder ausdrücklich als historisch markiert werden.
- Englischer und deutscher Text sollten dieselben technischen Fakten und Grenzen vermitteln.
- Commit-, Pull-Request- und CI-Referenzen sollten vor ihrer Aufnahme geprüft werden.
- Die Git-Historie bleibt die maßgebliche Quelle dafür, was tatsächlich wann geändert wurde.

## Abschlussbewertung und offene Grenzen

Die ADR-Konsolidierung ist als Dokumentations- und Qualitätssicherungsmaßnahme abgeschlossen. Sie
ergänzte keine neue Architekturentscheidung, veränderte nicht das Produkt oder eine öffentliche API
und zertifizierte kein Release.

Der Projektstand bleibt:

- M8 ist abgeschlossen; siehe [M8 Exit Review](M8_EXIT_REVIEW.md) und
  [ADR 0200](../adr/0200-m8-developer-productivity-foundations-exit-decision.md).
- Version 0.2.0 bleibt `Unreleased`.
- Die sichtbare manuelle macOS-/Cocoa-M3-Validierung bleibt offen; siehe
  [M3 Rendered Desktop Smoke Test](M3_RENDERED_DESKTOP_SMOKE_TEST.md).
- Eine separate v0.2.0-Release-Readiness-Entscheidung bleibt offen.
- Die Entscheidung, die heutigen SDL3-/SDL_ttf-Release-Tag-Referenzen durch unveränderliche
  Source-Commit-Pins zu ersetzen, bleibt offen; siehe
  [Migration-Readiness-Review](SASDGMBH_MIGRATION_READINESS.md).
- Der spätere Umzug in ein `SASDGMBH`-Repository wurde durch diese Arbeiten nicht durchgeführt; siehe
  [ADR 0202](../adr/0202-beta-and-canonical-product-repository-roles.md).

Weder Release Candidate noch Tag, GitHub Release, Repository-Transfer oder Produktzertifizierung sind
damit verbunden.
