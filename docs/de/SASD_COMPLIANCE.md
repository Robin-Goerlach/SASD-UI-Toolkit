# SASD Development Standard – Alignment-Erklärung

Diese deutsche Zusammenfassung ist informativ. Die
[englische Alignment Declaration](../en/SASD_COMPLIANCE.md) und die einzige
[kanonische Requirement Matrix](../SASD_CORE_REQUIREMENT_MATRIX.md) sind für dieses Assessment
maßgeblich; es gibt keine zweite übersetzte Evidence-Matrix.

## Bewertungsrahmen

- **Projektstand:** M8-Basis `3304658409e163fc7e667b9b03acf2e1d086b94e` plus die Governance-Remediations dieses Reviews
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

## Grenzen der Aussage

Dies ist keine Zertifizierung, keine offizielle Compliance-Bestätigung und keine Behauptung stabiler
SASD-1.0-Ausrichtung. Der sichtbare macOS/Cocoa-M3-Gate bleibt offen. CMake bleibt `0.2.0`, der
Changelog bleibt `Unreleased`; dieser Slice erzeugt weder RC noch Tag noch Release.

## Standard Feedback

Das Standard-Repository bleibt unverändert. Als Pilotfeedback sind dokumentiert: das fehlende
C++-/Cross-Platform-Profil, die hohe manuelle Last der großen bedingten Requirement-Matrix sowie eine
Scope-Unklarheit zwischen Security-Meldepflichten für Dienste und öffentlich verteilte Bibliotheken.
Details stehen in der englischen autoritativen Erklärung.
