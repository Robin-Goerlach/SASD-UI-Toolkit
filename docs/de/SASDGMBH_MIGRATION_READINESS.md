# SASDGMBH Repository-Migration-Readiness

## Umfang und Ergebnis

- **Review-Datum:** 23.11.2026
- **Geprüftes Quell-Repository:** `Robin-Goerlach/SASD-UI-Toolkit`
- **Geprüfter Ausgangs-SHA:** `30d4270ab772a5ee545da63c64d4e0e46eb35928`
- **Versionsstand:** CMake `0.2.0`; Changelog `Unreleased`
- **Ergebnis:** historienerhaltende Migration ist vorbereitbar, ein kanonischer Cutover ist **noch
  nicht freigabefähig**

Ein Ziel-Repository unter `SASDGMBH` konnte in diesem Review nicht verifiziert werden. Weder die
öffentliche Organization-Abfrage noch die Repository-Abfrage lösten mit dem verfügbaren API-Zugriff
ein verifiziertes Ziel auf. Dieser Bericht behauptet daher nicht, dass das Ziel bereits existiert.
Erstellung, Transfer, Setting-Änderungen, Tags und Releases liegen außerhalb dieses Auftrags.

Die [englische Fassung](../en/SASDGMBH_MIGRATION_READINESS.md) ist der kanonische Bericht; diese
deutsche Fassung hält dieselben Exit-Kriterien und ausführbaren Schritte fest.

## Geplante Repository-Rollen

Bis alle Exit-Kriterien erfüllt und der Cutover ausdrücklich freigegeben ist, bleibt das persönliche
Repository der aktuelle Projekt- und Quellstand.

Nach dem Cutover:

- bleibt das persönliche Repository das Development-/Beta-Repository;
- wird das zukünftige `SASDGMBH`-Repository das kanonische Produkt-, Issue- und Release-Repository;
- werden nur geprüfte exakte Commits ohne History Rewrite von Beta nach Produkt übernommen;
- entstehen offizielle neue Tags und Releases ausschließlich im Produkt-Repository;
- bleibt das vorhandene persönliche Release `v0.1.0` historische Evidence, sofern seine
  GitHub-Release-Metadaten nicht bewusst im Produkt-Repository nachgebildet werden;
- benötigt jedes Repository eigene Security-Settings und einen eigenen vertraulichen Meldeweg.

Diese Rollenentscheidung ist in [ADR 0202](../adr/0202-beta-and-canonical-product-repository-roles.md)
festgehalten.

## Verifizierter Ist-Stand

| Bereich | Befund | Auswirkung |
|---|---|---|
| Git-Historie | `git fsck --full` erfolgreich; 614 Commits, ein Root-Commit, keine Submodule, ca. 4,24 MiB gepackte Historie. | Keine Reparatur vor Migration erforderlich. |
| Branches | Auf GitHub existiert nur `main`. | Einfacher Promotion-Umfang. |
| Tag/Release | Annotierter Tag `v0.1.0` zeigt auf `dc34ebd4bf6db4a0f9777fc81320f47dae72b36b`; ein veröffentlichtes GitHub Release ohne hochgeladene Assets. | Tag wandert mit Git; Release-Metadaten nicht. |
| Issues/PRs | Keine offenen Issues oder PRs. | Keine offenen Tracker-Einträge zu übertragen; historische PR-/Actions-Links bleiben Evidence im Quell-Repository. |
| Lizenz | MIT; Copyright `2026 Robin Goerlach (SASD-GmbH)`; Third-Party-Hinweise für Unicode und optionale SDL-Abhängigkeiten vorhanden. | Unverändert erhalten, solange der Rechteinhaber keine separate Änderung genehmigt. |
| CMake-Paket | `SASDUIToolkit` und `SASD::UI*` enthalten keinen Repository-Owner. | Keine Umbenennung erforderlich. |
| Packaging | Debug-/Release-Installation und Package Consumer laufen in CI auf Linux, Windows und macOS. | Kein Migrationsblocker. |
| GitHub Controls | PVR, Secret Scanning und Push Protection aktiv; Workflow-Default read-only; keine Rulesets/Branch Protection; Dependabot Security Updates deaktiviert. | Im Produkt-Repository separat konfigurieren und verifizieren. |
| Security | REST lieferte `{"enabled":true}`; `SECURITY.md` enthält den aktuellen privaten Meldeweg. | Aktuelle Lücke geschlossen, aber Setting/URL sind Repository-spezifisch. |
| CI | Main CI, GTK Native CI und AppKit Native CI sind für den Ausgangs-SHA vollständig grün. | Starke Basis; der Migrations-PR und später das Produkt-Repository brauchen eigene grüne Läufe. |
| Manuelles Gate | Sichtbare macOS/Cocoa-M3-Prüfung bleibt offen. | v0.2.0-Release-Blocker, kein Migrationsblocker. |

## Validierungsnachweis

Lokal im isolierten Clone unter Debian/WSL mit CMake 3.25.1 und GCC 12.2 ausgeführt:

- strikte Debug-Konfiguration und vollständiger Build mit Tests, Beispielen und Warnings-as-Errors:
  **PASS**;
- vollständiges CTest: **4/4 PASS** (Core, Terminal, Rendered, nativer POSIX-PTY-Smoke-Test);
- gestagte CMake-Installation und `tools/verify_release_install.cmake`: **PASS**;
- externer installierter Package Consumer: Configure/Build sowie **1/1 Test PASS**;
- Matrix-Neuzählung: **610 = 515 Satisfied + 95 Not Applicable**;
- relative Markdown-Links: **0 defekt**; `git diff --check`: **PASS**.

Für den exakten Ausgangs-SHA sind außerdem GitHub CI `44277502445`, GTK Native CI `44277502392` und
AppKit Native CI `44277502335` grün. Sie decken GCC, Clang, MSVC, AppleClang, ASan/UBSan, SDL3, GTK4,
AppKit, Linux-/Windows-/macOS-Release, Installation und Package Consumer ab.

Nicht lokal ausgeführt:

- MSVC/Windows nativ, weil der Windows-Umgebung CMake/C++-Workload fehlt;
- macOS/AppleClang/AppKit, GTK4, SDL3 und Sanitizer mangels lokaler Plattform/optionaler Dependencies;
- sichtbare manuelle macOS/Cocoa-M3-Prüfung; sie benötigt eine menschlich sichtbare macOS-Session
  und bleibt offen;
- Ziel-Repository-CI, Security- und Ruleset-Tests, weil kein Ziel verifiziert ist.

## Aktuelle Migrationsblocker

Quellhistorie, Lizenz und Package-Identität blockieren die Vorbereitung nicht. Der kanonische Cutover
bleibt blockiert, bis dieser PR mit grüner CI gemergt ist, Ziel/Owner verifiziert sind, Historie und
Tags für einen freigegebenen SHA identisch kopiert wurden, die Ziel-CI einschließlich Install/Consumer
grün ist und Ruleset, Least-Privilege-Actions, Scanning sowie PVR im Ziel getestet sind. Die letzten
vier Punkte können im aktuellen Source-only-Review nicht abgeschlossen werden.

## Vor Erstellung oder Kopie zu erledigen

- [ ] Diesen Security-/Migration-PR erst nach vollständig grüner CI mergen.
- [ ] Den freigegebenen Cutover-Commit als vollständigen 40-stelligen SHA festhalten und seine
      Erreichbarkeit von Quell-`main` prüfen.
- [ ] Rechtlichen Ziel-Owner und exakten Repository-Namen mit einem autorisierten Organization Owner
      bestätigen; keine Ableitung aus Anzeigenamen.
- [ ] Behandlung der `v0.1.0`-GitHub-Release-Metadaten entscheiden: nachbilden, historisch verlinken
      oder nur im persönlichen Repository belassen.
- [ ] Nach Cutover kanonischen Issue-Tracker und Hinweisführung des Beta-Repositorys festlegen.
- [ ] Vor dem ersten Remote-Write ein wiederherstellbares Mirror-Bundle/Bare-Mirror-Backup und ein
      Ref-Inventar erstellen.
- [ ] Für den finalen Ref-Vergleich den Produkt-Promotion-Stand kurz einfrieren.

Eine Migration benötigt keine neue Version, keinen Tag und kein Release.

## Erst nach Erstellung des Ziels möglich

- [ ] `main` als Default Branch und die freigegebene Sichtbarkeit setzen.
- [ ] Ruleset/Branch Protection für `main`: PR-Pflicht, kein Force Push, keine Branch-Löschung,
      erforderliche Checks und nur ausdrücklich genehmigte Bypass-Rollen.
- [ ] Nach Entstehung der Check-Kontexte alle elf etablierten Checks verlangen: Linux GCC, Linux
      Clang, Windows MSVC, macOS AppleClang, Linux/macOS/Windows Release, Linux SDL3 Adapter, Linux
      Clang ASan + UBSan, GTK4 Native Host und macOS AppKit Native.
- [ ] Workflow-Default read-only halten und PR-Genehmigungen durch Workflows deaktivieren.
- [ ] Zulässige Actions prüfen und SHA-Pinning-Enforcement aktivieren, sofern mit der genehmigten
      Workflow-Policy vereinbar.
- [ ] Secret Scanning und Push Protection aktivieren; Dependabot Security Updates sowie zusätzliche
      Scanning-Modi bewusst entscheiden.
- [ ] Private Vulnerability Reporting aktivieren und per REST auf `{"enabled":true}` prüfen.
- [ ] Privates Meldeformular angemeldet öffnen und `SECURITY.md` auf die verifizierte Ziel-URL ändern.
- [ ] Beschreibung, Topics, License Detection, Templates, Teams/CODEOWNERS, Notifications und
      Merge-Policy festlegen.
- [ ] Secrets, Variables, Environments und Release-Rechte einzeln mit Owner und Zweck prüfen; nichts
      durch Vermutung kopieren.

## Ausführbare historienerhaltende Kopie

Nur gegen ein verifiziertes, leeres und für Writes freigegebenes Ziel ausführen. Platzhalter durch
geprüfte literale Werte ersetzen. Niemals einen nicht leeren Zielstand per Force Push überschreiben.

```bash
SOURCE_URL=https://github.com/Robin-Goerlach/SASD-UI-Toolkit.git
TARGET_URL=https://github.com/VERIFIED_OWNER/VERIFIED_REPOSITORY.git
CUTOVER_SHA=FULL_40_CHARACTER_APPROVED_SHA

git clone --mirror "$SOURCE_URL" sasd-ui-toolkit-migration.git
git -C sasd-ui-toolkit-migration.git fsck --full
git -C sasd-ui-toolkit-migration.git show-ref
git -C sasd-ui-toolkit-migration.git merge-base --is-ancestor \
  "$CUTOVER_SHA" refs/heads/main

git -C sasd-ui-toolkit-migration.git push "$TARGET_URL" \
  refs/heads/main:refs/heads/main
git -C sasd-ui-toolkit-migration.git push "$TARGET_URL" \
  'refs/tags/*:refs/tags/*'

git ls-remote --heads --tags "$SOURCE_URL"
git ls-remote --heads --tags "$TARGET_URL"
```

Beide Ref-Inventare müssen für übernommene Heads/Tags einschließlich des peeled annotated tag object
übereinstimmen. Ein Source-Snapshot ist unzulänglich, weil Autorenschaft und Historie verloren gehen.
Issues, PRs, Actions-Läufe, GitHub Releases, Settings und Security Advisories sind keine Git-Refs.

## Post-Copy- und Cutover-Checkliste

- [ ] Produkt-Repository frisch und ohne lokale Object Alternates klonen.
- [ ] `git fsck --full`, Root-Commit, Commit-Anzahl, Cutover-SHA und Ziel von `v0.1.0` prüfen.
- [ ] Alle drei Workflows im Produkt-Repository vollständig grün ausführen.
- [ ] Build, Tests, Installation und Package Consumer aus dem Produkt-Clone prüfen.
- [ ] Relative Links, License Detection und Third-Party-Hinweise prüfen.
- [ ] Nach `Robin-Goerlach/SASD-UI-Toolkit` suchen: historische PR-/CI-Evidence bewusst erhalten,
      operative Issue-, Release-, Badge- und Security-Links auf das Produkt-Repository umstellen.
- [ ] PVR per REST und geöffnetem privatem Formular verifizieren.
- [ ] Ruleset mit einem ungefährlichen Test-PR prüfen; direkte/Force Pushes müssen für normale
      Contributor abgewiesen werden.
- [ ] In beiden Repositorys klar Produkt- versus Beta-Rolle benennen.
- [ ] `SASD-REP-004` und `SASD-REP-005` neu bewerten, sobald beide Kopien existieren.
- [ ] Bei nachgebildeten Release-Metadaten Tag-Ziel, Notes und eventuelle Assets/Checksums vergleichen.
- [ ] Finale URL, Cutover-SHA, Validierungsläufe und Freigabeverantwortlichen dokumentieren.

## Exit-Kriterien

Der kanonische Cutover ist erst zulässig, wenn Ziel und Berechtigungen verifiziert sind, SHA und Tags
objektidentisch vorliegen, Produkt-CI/Install/Consumer grün sind, Ruleset und Security Controls aktiv
sind, PVR funktioniert, operative Links/Rollen stimmen, Lizenz/Attribution erhalten sind, Issue- und
Release-Historie bewusst behandelt ist und ein autorisierter Owner freigibt.

Die Migration behauptet keine v0.2.0-Freigabe. Das manuelle macOS/Cocoa-M3-Gate, der separate
Release-Readiness-Review und die Entscheidung über immutable SDL3/SDL_ttf-Source-Pins bleiben offen.
Kein RC, Tag oder Release ist durch diesen Bericht autorisiert.
