# SASD Development Standard – Alignment-Erklärung

Diese deutsche Zusammenfassung ist informativ. Die
[englische Alignment Declaration](../en/SASD_COMPLIANCE.md) und die einzige
[kanonische Requirement Matrix](../SASD_CORE_REQUIREMENT_MATRIX.md) sind für dieses Assessment
maßgeblich; es gibt keine zweite übersetzte Evidence-Matrix.

## Bewertungsrahmen

- **Projektstand:** M8-Basis `3304658409e163fc7e667b9b03acf2e1d086b94e`
- **Finaler Adoption-Implementierungsstand:** `833ff0f4436b838b7879d3813a7ab083aeaf6825`
- **Standard:** SASD Development Standard Version 1.0 Specification Candidate
- **Unveränderlicher Referenzcommit:** `f12e2c038205892107440b4a58bfd41f21214036`
- **Bewertungstyp:** Pilot Alignment Self-Assessment
- **Primäre Qualitätsstufe:** SASD Recommended
- **Formales Profil:** Core
- **Desktop:** technology-neutrale Hinweise werden selektiv berücksichtigt; eine vollständige formale Profilübernahme wird nicht behauptet
- **Bereichsweise Hochstufungen:** keine; einzelne strengere Security-/Release-Praktiken sind freiwillig
- **Datum:** 17.11.2026
- **Nächster Review:** 01.12.2026 oder vor der v0.2.0 Release Readiness

Die Begründung der Einstufung steht in der
[Projektklassifikation](../SASD_PROJECT_CLASSIFICATION.md). Das Toolkit ist ein großes, langfristig
gepflegtes, öffentliches C++-Bibliotheksprodukt mit externer Nutzungsmöglichkeit, aber kein
eigenständig betriebener geschäftskritischer Production-Dienst.

## Gesamtstatus

**Pilot Alignment Self-Assessment – Partially Aligned**

| Status | Anzahl |
|---|---:|
| Satisfied | 513 |
| Not Applicable | 95 |
| Exception | 0 |
| Open | 2 |
| Not Assessed | 0 |
| **Gesamt** | **610** |

Offen bleiben `SASD-REP-003` und `SASD-MNT-010`: GitHub Private Vulnerability Reporting ist nicht
aktiviert, und ein anderer vertraulicher Security-Kontakt wurde nicht verifiziert. `SECURITY.md`
erfindet deshalb keinen Meldeweg. Der Repository Owner muss vor der v0.2.0 Release Readiness Private
Vulnerability Reporting aktivieren oder einen geeigneten privaten Kontakt festlegen.

Die produktspezifische [Security-Bewertung](../SASD_SECURITY_ASSESSMENT.md) dokumentiert Schutzgüter,
Trust Boundaries, Risiken, Controls, Restrisiken und die Security Baseline. Die
[Projektanforderungen](../SASD_PROJECT_REQUIREMENTS.md) geben bestehenden Produktverträgen 24 stabile
IDs, ohne neue Features zu definieren.

## Abschlussnachweis seit ADR 0200

[ADR 0200](../adr/0200-m8-developer-productivity-foundations-exit-decision.md) markiert den
abgeschlossenen M8-Ausgangspunkt.
[ADR 0201](../adr/0201-sasd-development-standard-adoption.md) hält die danach getroffene dauerhafte
Governance-Entscheidung fest. Die Umsetzung erfolgte nachvollziehbar wie folgt:

1. Commit `e245afc7dd57cae9dd883fa11cb0e825ba4c5b5c` ergänzte Klassifikation, Compliance-Erklärung,
   die kanonische 610-zeilige Requirement Matrix, 24 Projektanforderungen, Security Policy und
   Security Assessment, Repository-Governance, ADR 0201 sowie CI-/Supply-Chain-Hardening.
2. [PR 108](https://github.com/Robin-Goerlach/SASD-UI-Toolkit/pull/108) bestand elf PR-Checks und
   wurde als `2f642f120180e519ebf47f19a0a663ece4dd9b1d` gemergt. Main CI und GTK waren für diesen SHA
   grün; AppKit überschritt bei verlangsamter macOS-Runner-Kapazität das neue 30-Minuten-Limit. Der
   abgebrochene Lauf wurde nicht als erfolgreiche Evidence gewertet.
3. Commit `8b009e7b682b2a71559887900c3908bda38a7dda` setzte ausschließlich das AppKit-Limit auf
   realistische, weiterhin endliche 45 Minuten. Der vollständige Build-/Test-Gate blieb bestehen.
   [PR 110](https://github.com/Robin-Goerlach/SASD-UI-Toolkit/pull/110) bestand erneut elf Checks und
   wurde als finaler Adoption-Stand `833ff0f4436b838b7879d3813a7ab083aeaf6825` gemergt.
4. Für exakt diesen finalen SHA sind
   [Main CI 44188598888](https://github.com/Robin-Goerlach/SASD-UI-Toolkit/actions/runs/44188598888),
   [GTK Native CI 44188598945](https://github.com/Robin-Goerlach/SASD-UI-Toolkit/actions/runs/44188598945) und
   [AppKit Native CI 44188598993](https://github.com/Robin-Goerlach/SASD-UI-Toolkit/actions/runs/44188598993)
   vollständig grün. Main CI umfasst GCC, Clang, MSVC, AppleClang, ASan/UBSan, SDL3 sowie
   Linux-/Windows-/macOS-Release-, Test-, Installations- und Package-Consumer-Verifikation.

Lokal wurden zusätzlich Markdown-Links, GCC-Debug-/Release-Builds mit Warnings-as-Errors, beide
vollständigen CTest-Konfigurationen, Installationsinhalte, Package Consumer und Release-Hygiene
geprüft. Produktcode, öffentliche API und Runtime-Semantik blieben unverändert. Die 16 bereits
vorhandenen user-owned Working-Tree-Pfade waren beim finalen Cleanup 16/16 SHA-256-identisch; der
Task-Branch wurde danach lokal und remote entfernt.

Beim späteren Review dieses Abschlussnachweises trat dieselbe Hosted-Runner-Verlangsamung auch im
AppleClang-Matrixjob reproduzierbar auf: Check `134655658672` erreichte sein 30-Minuten-Limit,
während GitHub Kapazitätsengpässe für macOS-arm64-Runner meldete. Die Matrix behält 30 Minuten für
Linux und Windows und verwendet für AppleClang ein weiterhin endliches 45-Minuten-Limit. Diese
Korrektur ändert ausschließlich die Scheduling-Toleranz; kein Build-, Test-, Installations- oder
Package-Consumer-Gate wird entfernt oder abgeschwächt.

## Grenzen der Aussage

Dies ist keine Zertifizierung, keine offizielle Compliance-Bestätigung und keine Behauptung stabiler
SASD-1.0-Ausrichtung. Der sichtbare macOS/Cocoa-M3-Gate bleibt offen. CMake bleibt `0.2.0`, der
Changelog bleibt `Unreleased`; dieser Slice erzeugt weder RC noch Tag noch Release.

## Standard Feedback

Das Standard-Repository bleibt unverändert. Als Pilotfeedback sind dokumentiert: das fehlende
C++-/Cross-Platform-Profil, die hohe manuelle Last der großen bedingten Requirement-Matrix sowie eine
Scope-Unklarheit zwischen Security-Meldepflichten für Dienste und öffentlich verteilte Bibliotheken.
Details stehen in der englischen autoritativen Erklärung.
