# ADR 0202: Separate Beta Development and Canonical Product Repository Roles

- **Status:** Accepted
- **Date:** 2026-11-23
- **Decision owners:** repository owner / maintainer
- **Related:** [ADR 0201](0201-sasd-development-standard-adoption.md),
  [migration-readiness review](../en/SASDGMBH_MIGRATION_READINESS.md)

## Context

SASD UI Toolkit currently lives in the personal `Robin-Goerlach/SASD-UI-Toolkit` repository. The
owner intends to keep that repository for development and beta work while a future repository owned
by `SASDGMBH` becomes the canonical product and release location.

A second repository changes governance, issue/report routing, release authority and supply-chain
controls. Git history can carry source, tags and workflows, but it cannot carry GitHub rulesets,
permissions, secrets, Actions runs, releases, issues or Private Vulnerability Reporting settings.
The destination repository was not verified as existing during this decision. The design must
therefore define a future boundary without pretending that cutover already happened.

## Decision

Until every migration exit criterion is satisfied and an authorized owner approves cutover, the
personal repository remains the current source of truth.

After cutover:

1. The personal repository is the **development/beta repository**. It may host early integration,
   beta validation and candidate changes.
2. The verified `SASDGMBH` repository is the **canonical product repository**. Product issues,
   supported source, official release tags and GitHub Releases belong there.
3. Promotion uses reviewed exact commits with preserved ancestry. Force pushes, rebases of published
   history and source-snapshot imports are not permitted migration mechanisms.
4. The product repository must pass its own CI, install/package-consumer validation and security
   control checks before becoming canonical. Success in the beta repository is necessary evidence,
   not a substitute.
5. Repository-specific controls are configured separately: branch protection/rulesets, least-
   privilege Actions, scanning, secrets and Private Vulnerability Reporting.
6. Both repositories must state their role clearly. Historical personal-repository PR, CI and release
   links remain provenance evidence; operational links in the product repository point to the
   product repository.
7. Existing license, copyright, attribution and third-party notices are preserved. Repository
   ownership does not silently transfer copyright or change the MIT license.
8. Migration and v0.2.0 release readiness remain independent decisions. Migration creates no RC,
   version, tag or release and does not close the manual macOS/Cocoa M3 gate.

The executable checks and complete exit criteria are maintained in the migration-readiness review.

## Consequences

- Development can remain flexible without making the beta repository the long-term release authority.
- Product consumers receive one explicit canonical source after cutover.
- Promotion requires deliberate evidence and creates some duplicated CI cost.
- Issues, releases, security reports and settings cannot be assumed to synchronize automatically.
- `SASD-REP-004` and `SASD-REP-005` must be reassessed when two project-controlled copies actually
  exist; they remain conditionally inapplicable before that event.
- The personal `v0.1.0` release remains historical evidence unless a separately approved process
  recreates its GitHub Release metadata without claiming a new release.

## Implementation and review record

This decision was prepared by the security-closure and migration-readiness review performed on
2026-11-23. The record below makes the executed work independently reviewable; it does not claim
that migration or release approval has occurred.

1. The protected starting point was `main` commit
   `30d4270ab772a5ee545da63c64d4e0e46eb35928`. Its Main, GTK Native and AppKit Native workflows were
   green. Pre-existing modified ADR files and the untracked `..Codex.txt` in the owner's working tree
   were left untouched; the review was performed in a separate clone and branch.
2. GitHub REST API verification of
   `GET /repos/Robin-Goerlach/SASD-UI-Toolkit/private-vulnerability-reporting` returned
   `{"enabled":true}`. `SECURITY.md` now identifies GitHub Private Vulnerability Reporting as the
   confidential route without inventing an email address.
3. The complete 610-row Core matrix was reassessed against the immutable SASD Development Standard
   commit `f12e2c038205892107440b4a58bfd41f21214036`. The resulting counts are 515 Satisfied,
   95 Not Applicable, 0 Exception, 0 Open and 0 Not Assessed. `SASD-REP-003` and `SASD-MNT-010`
   therefore moved to Satisfied, and the pilot self-assessment moved to `Aligned` without making a
   certification claim.
4. The review covered repository structure and history, branches, tags and releases, licensing and
   third-party attribution, repository URLs, issue/PR/CI links, CMake package identity, installation
   and package consumers, dependencies, Actions and release gates, security reporting and the future
   beta/product repository split. The executable exit criteria and pre-/post-creation checklist are
   recorded in the linked migration-readiness report.
5. Local validation passed repository-integrity and release-tree checks, Markdown relative-link
   validation, a strict GCC Debug build under WSL, all four configured CTest tests, staged install
   verification and an installed-package consumer build/test. Native local MSVC and macOS execution
   were unavailable; no result was invented for them.
6. Commit `822c61d00858b68e84f5a1db643357ee78ca03ac` was pushed on
   `docs/security-migration-readiness` and opened as
   [PR 112](https://github.com/Robin-Goerlach/SASD-UI-Toolkit/pull/112). All eleven PR checks passed:
   GCC, Clang, MSVC, AppleClang, ASan/UBSan, SDL3, GTK4, AppKit Native and Linux, Windows and macOS
   Release/package-consumer validation.
7. The version remains `0.2.0` and the changelog remains `Unreleased`. No setting, tag, release,
   target repository, repository transfer or history was changed. The visible manual macOS/Cocoa M3
   validation remains open. PR 112 was intentionally left open and unmerged pending explicit owner
   approval.
8. A subsequent merge-closure review re-fetched the remote and verified that PR 112 was open,
   mergeable and `CLEAN`, that `main` was still
   `30d4270ab772a5ee545da63c64d4e0e46eb35928`, and that all eleven checks for head
   `23168e390b16f6cb146c626500a34ca07c7ea96b` had succeeded. The owner's merge authorization was
   explicitly conditional on the older head `822c61d00858b68e84f5a1db643357ee78ca03ac`. Because the
   requested ADR execution record had legitimately advanced the PR head, that exact-SHA condition
   was not met. The merge, post-merge validation and branch cleanup were therefore deliberately not
   performed. The original inventory of fifteen modified ADR files plus untracked `..Codex.txt` in
   the owner's working tree remained present and untouched.

## Alternatives considered

### Transfer the personal repository

Not selected because the owner explicitly requires the personal repository to continue as the
development/beta repository. A transfer would move rather than preserve that role.

### Keep the personal repository canonical indefinitely

Not selected because it would not establish the requested organizational product/release authority.

### Copy only the current source tree

Rejected because it loses history, authorship and tag provenance and weakens auditability.

---

# ADR 0202: Getrennte Rollen für Beta-Entwicklung und kanonisches Produkt-Repository

- **Status:** Akzeptiert
- **Datum:** 23.11.2026
- **Entscheidungsverantwortung:** Repository Owner / Maintainer
- **Bezug:** [ADR 0201](0201-sasd-development-standard-adoption.md),
  [Migration-Readiness-Review](../de/SASDGMBH_MIGRATION_READINESS.md)

## Kontext

SASD UI Toolkit liegt derzeit im persönlichen Repository
`Robin-Goerlach/SASD-UI-Toolkit`. Dieses Repository soll für Development und Beta erhalten bleiben;
ein zukünftiges Repository unter `SASDGMBH` soll kanonischer Produkt- und Release-Ort werden.

Ein zweites Repository verändert Governance, Issue-/Meldewege, Release-Autorität und Supply-Chain-
Controls. Git überträgt Quellcode, Tags und Workflows, aber keine GitHub Rulesets, Berechtigungen,
Secrets, Actions-Läufe, Releases, Issues oder Private-Vulnerability-Reporting-Settings. Ein
existierendes Ziel-Repository wurde in diesem Review nicht verifiziert. Die Entscheidung definiert
daher die zukünftige Grenze, ohne einen bereits erfolgten Cutover vorzutäuschen.

## Entscheidung

Bis alle Migration-Exit-Kriterien erfüllt sind und ein autorisierter Owner den Cutover freigibt,
bleibt das persönliche Repository aktueller Source of Truth.

Nach dem Cutover:

1. ist das persönliche Repository das **Development-/Beta-Repository** für frühe Integration und
   Beta-Validierung;
2. ist das verifizierte `SASDGMBH`-Repository das **kanonische Produkt-Repository** für Produkt-
   Issues, unterstützten Quellstand, offizielle Release-Tags und GitHub Releases;
3. erfolgt Promotion über geprüfte exakte Commits mit erhaltener Abstammung; Force Pushes, Rebase
   veröffentlichter Historie und Source-Snapshot-Importe sind ausgeschlossen;
4. muss das Produkt-Repository eigene CI-, Install-/Package-Consumer- und Security-Control-Prüfungen
   bestehen; Beta-Evidence ersetzt diese nicht;
5. werden Repository-spezifische Controls wie Rulesets, Actions-Rechte, Scanning, Secrets und PVR
   separat konfiguriert;
6. benennen beide Repositorys ihre Rolle klar; historische Links bleiben Provenance-Evidence,
   operative Produktlinks zeigen auf das Produkt-Repository;
7. bleiben Lizenz, Copyright, Attribution und Third-Party-Hinweise erhalten; und
8. bleiben Migration und v0.2.0 Release Readiness getrennte Entscheidungen. Migration erzeugt weder
   RC, Version, Tag noch Release und schließt nicht das manuelle macOS/Cocoa-M3-Gate.

Ausführbare Prüfungen und vollständige Exit-Kriterien stehen im Migration-Readiness-Review.

## Folgen

- Beta-Entwicklung bleibt flexibel, ohne dauerhaft Release-Autorität zu sein.
- Produktnutzer erhalten nach Cutover eine eindeutige kanonische Quelle.
- Promotion benötigt bewusste Evidence und verursacht teilweise doppelte CI-Kosten.
- Issues, Releases, Security Reports und Settings synchronisieren sich nicht automatisch.
- `SASD-REP-004` und `SASD-REP-005` werden neu bewertet, sobald tatsächlich zwei kontrollierte
  Kopien existieren; vorher bleibt ihr Trigger abwesend.
- Das persönliche `v0.1.0`-Release bleibt historische Evidence, sofern ein separat freigegebener
  Prozess seine GitHub-Release-Metadaten nicht ohne neue Release-Behauptung nachbildet.

## Umsetzungs- und Review-Nachweis

Diese Entscheidung wurde durch das Security-Closure- und Migration-Readiness-Review vom 23.11.2026
vorbereitet. Der folgende Nachweis macht die tatsächlich ausgeführten Arbeiten unabhängig prüfbar;
er behauptet weder eine erfolgte Migration noch eine Release-Freigabe.

1. Geschützter Ausgangspunkt war `main`-Commit
   `30d4270ab772a5ee545da63c64d4e0e46eb35928`. Dessen Main-, GTK-Native- und AppKit-Native-Workflows
   waren grün. Bereits veränderte ADR-Dateien und die unversionierte Datei `..Codex.txt` im Working
   Tree des Owners blieben unangetastet; das Review erfolgte in einem separaten Clone und Branch.
2. Die GitHub-REST-Abfrage
   `GET /repos/Robin-Goerlach/SASD-UI-Toolkit/private-vulnerability-reporting` lieferte
   `{"enabled":true}`. `SECURITY.md` benennt GitHub Private Vulnerability Reporting jetzt als
   vertraulichen Meldeweg, ohne eine E-Mail-Adresse zu erfinden.
3. Die vollständige Core-Matrix mit 610 Zeilen wurde gegen den unveränderlichen Commit
   `f12e2c038205892107440b4a58bfd41f21214036` des SASD Development Standard neu bewertet. Ergebnis:
   515 Satisfied, 95 Not Applicable, 0 Exception, 0 Open und 0 Not Assessed. `SASD-REP-003` und
   `SASD-MNT-010` wechselten damit zu Satisfied; das Pilot Self-Assessment wechselte ohne
   Zertifizierungsbehauptung zu `Aligned`.
4. Geprüft wurden Repository-Struktur und Historie, Branches, Tags und Releases, Lizenz und
   Drittanbieter-Attribution, Repository-URLs, Issue-/PR-/CI-Links, CMake-Paketidentität,
   Installation und Package Consumer, Abhängigkeiten, Actions und Release-Gates, Security Reporting
   sowie die zukünftige Trennung von Beta- und Produkt-Repository. Ausführbare Exit-Kriterien und die
   Checkliste vor und nach der Zielerstellung stehen im verlinkten Migration-Readiness-Bericht.
5. Lokal bestanden Repository-Integrität und Release-Tree-Hygiene, die Prüfung relativer
   Markdown-Links, ein strikter GCC-Debug-Build unter WSL, alle vier konfigurierten CTest-Tests, die
   Staged-Install-Prüfung und Build/Test eines Installed-Package-Consumers. Native lokale MSVC- und
   macOS-Ausführung waren nicht verfügbar; dafür wurde kein Ergebnis erfunden.
6. Commit `822c61d00858b68e84f5a1db643357ee78ca03ac` wurde auf
   `docs/security-migration-readiness` gepusht und als
   [PR 112](https://github.com/Robin-Goerlach/SASD-UI-Toolkit/pull/112) eröffnet. Alle elf PR-Checks
   bestanden: GCC, Clang, MSVC, AppleClang, ASan/UBSan, SDL3, GTK4, AppKit Native sowie die
   Linux-, Windows- und macOS-Release-/Package-Consumer-Validierung.
7. Die Version bleibt `0.2.0`, der Changelog bleibt `Unreleased`. Weder Settings, Tag, Release,
   Ziel-Repository, Repository-Transfer noch Historie wurden verändert. Die sichtbare manuelle
   macOS/Cocoa-M3-Validierung bleibt offen. PR 112 blieb bis zu einer ausdrücklichen Owner-Freigabe
   bewusst offen und ungemergt.
8. Ein anschließendes Merge-Closure-Review holte den Remote-Stand erneut ab und bestätigte: PR 112
   war offen, mergebar und `CLEAN`, `main` stand weiterhin auf
   `30d4270ab772a5ee545da63c64d4e0e46eb35928`, und alle elf Checks für Head
   `23168e390b16f6cb146c626500a34ca07c7ea96b` waren erfolgreich. Die Merge-Freigabe des Owners war
   ausdrücklich an den älteren Head `822c61d00858b68e84f5a1db643357ee78ca03ac` gebunden. Weil der
   angeforderte ADR-Ausführungsnachweis den PR-Head berechtigt weitergeführt hatte, war diese exakte
   SHA-Bedingung nicht erfüllt. Merge, Post-Merge-Validierung und Branch-Bereinigung wurden daher
   bewusst nicht ausgeführt. Der ursprüngliche Bestand von fünfzehn veränderten ADR-Dateien plus der
   unversionierten Datei `..Codex.txt` im Working Tree des Owners blieb vorhanden und unangetastet.

## Betrachtete Alternativen

### Transfer des persönlichen Repositorys

Nicht gewählt, weil das persönliche Repository ausdrücklich als Development/Beta erhalten bleiben
soll. Ein Transfer würde diese Rolle verschieben statt erhalten.

### Persönliches Repository dauerhaft kanonisch lassen

Nicht gewählt, weil dadurch die gewünschte organisatorische Produkt-/Release-Autorität nicht entsteht.

### Nur den aktuellen Source Tree kopieren

Verworfen, weil Historie, Autorenschaft und Tag-Provenance verloren gingen.
