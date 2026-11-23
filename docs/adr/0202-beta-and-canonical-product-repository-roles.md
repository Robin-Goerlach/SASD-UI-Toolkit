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

## Betrachtete Alternativen

### Transfer des persönlichen Repositorys

Nicht gewählt, weil das persönliche Repository ausdrücklich als Development/Beta erhalten bleiben
soll. Ein Transfer würde diese Rolle verschieben statt erhalten.

### Persönliches Repository dauerhaft kanonisch lassen

Nicht gewählt, weil dadurch die gewünschte organisatorische Produkt-/Release-Autorität nicht entsteht.

### Nur den aktuellen Source Tree kopieren

Verworfen, weil Historie, Autorenschaft und Tag-Provenance verloren gingen.
