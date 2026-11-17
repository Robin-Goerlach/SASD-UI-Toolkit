# ADR 0201: Adopt the SASD Development Standard for Project Governance

- Status: Accepted
- Date: 2026-11-17
- Predecessor: [0200](0200-m8-developer-productivity-foundations-exit-decision.md)

## English

### Context

After M8 completion and before v0.2.0 release readiness, the project needed a formal, reproducible
governance baseline. The SASD Development Standard is currently a Version 1.0 Specification Candidate
whose normative Foundation/Governance 0.8.0 and Core/Profile/Process 0.9.0 documents are Approved, but
the Standard has not published `1.0.0` or `1.0.0-rc.N`.

The toolkit is a C++20 cross-platform library spanning Terminal, Rendered and optional native hosts.
The Standard's Desktop profile is currently scoped primarily to C#/.NET Windows applications using
WinForms or WPF, so claiming complete Desktop-profile adoption would be misleading.

### Decision

The project adopts the SASD Development Standard as project governance with:

- primary quality level **SASD Recommended**;
- formal profile **Core**;
- selected technology-neutral Desktop guidance considered informatively, without complete profile
  adoption;
- immutable pilot reference commit
  `f12e2c038205892107440b4a58bfd41f21214036`;
- an explicitly labelled **Pilot Alignment Self-Assessment** until a published stable Standard
  baseline and any required reassessment support a stronger statement.

The English classification, compliance declaration, requirement matrix, project requirements and
security assessment are canonical. The German compliance document is a linked human-readable
summary; it does not duplicate the requirement/evidence matrix. Bilingual ADRs remain authoritative
inside their single files.

The adoption makes no certification claim. An `Aligned` result is permitted only when the complete
matrix has no Open, Exception or Not Assessed item beyond the Standard's exact status rules. The
initial assessment is Partially Aligned because a confidential security-reporting route is not yet
verified.

### Consequences

- Quality-level, profile and evidence decisions are reviewable in the repository rather than in chat
  history.
- Existing product contracts receive stable traceability IDs without changing runtime semantics or
  defining a new milestone.
- Security, repository and CI controls are reviewed proportionately to a public C++ library.
- The project must reassess after a material scope/security change, before a major release, when the
  primary quality level changes, or when moving to a different Standard baseline.
- Standard issues found in this pilot are recorded as Standard Feedback in the project declaration;
  the Standard repository remains read-only.
- This decision does not close the visible macOS/Cocoa M3 gate and does not authorise an RC, tag,
  release or v0.2.0 release-readiness decision.

## Deutsch

### Kontext

Nach dem Abschluss von M8 und vor der v0.2.0 Release Readiness benötigt das Projekt eine formale,
reproduzierbare Governance-Basis. Der SASD Development Standard ist derzeit ein Version 1.0
Specification Candidate. Seine normativen Foundation-/Governance-Dokumente 0.8.0 und
Core-/Profil-/Prozess-Dokumente 0.9.0 sind Approved, aber weder `1.0.0` noch `1.0.0-rc.N` ist
veröffentlicht.

Das Toolkit ist eine plattformübergreifende C++20-Bibliothek mit Terminal-, Rendered- und optionalen
Native Hosts. Das Desktop-Profil des Standards richtet sich derzeit primär an C#/.NET-
Windows-Anwendungen mit WinForms oder WPF. Eine vollständige Desktop-Profilübernahme wäre daher
irreführend.

### Entscheidung

Das Projekt übernimmt den SASD Development Standard als Projekt-Governance mit:

- primärer Qualitätsstufe **SASD Recommended**;
- formalem Profil **Core**;
- selektiv und informativ berücksichtigten technology-neutralen Desktop-Hinweisen, ohne vollständige
  Profilübernahme;
- unveränderlichem Pilot-Referenzcommit
  `f12e2c038205892107440b4a58bfd41f21214036`;
- ausdrücklicher Kennzeichnung als **Pilot Alignment Self-Assessment**, solange keine veröffentlichte
  stabile Standardbasis und eine entsprechende Neubewertung eine stärkere Aussage tragen.

Die englische Klassifikation, Compliance Declaration, Requirement Matrix, Projektanforderungen und
Security-Bewertung sind kanonisch. Das deutsche Compliance-Dokument ist eine verlinkte,
menschenlesbare Zusammenfassung und dupliziert die Requirement-/Evidence-Matrix nicht. Bilinguale
ADRs bleiben innerhalb ihrer einzelnen Datei autoritativ.

Die Übernahme behauptet keine Zertifizierung. `Aligned` ist nur zulässig, wenn die vollständige Matrix
gemäß den exakten Statusregeln keine offenen, ausgenommenen oder unbewerteten Punkte enthält. Die
erste Bewertung ist Partially Aligned, weil kein vertraulicher Security-Meldeweg verifiziert ist.

### Folgen

- Qualitätsstufe, Profilumfang und Evidence-Entscheidungen sind im Repository statt in Chatverläufen
  nachvollziehbar.
- Bestehende Produktverträge erhalten stabile Traceability-IDs, ohne Runtime-Semantik zu ändern oder
  einen neuen Meilenstein zu definieren.
- Security-, Repository- und CI-Controls werden proportional zu einer öffentlichen C++-Bibliothek
  geprüft.
- Nach wesentlichen Scope-/Security-Änderungen, vor Major Releases, bei Änderung der Qualitätsstufe
  oder bei Wechsel der Standard-Baseline ist neu zu bewerten.
- Pilotbefunde am Standard werden als Standard Feedback im Projekt dokumentiert; das
  Standard-Repository bleibt read-only.
- Diese Entscheidung schließt weder den sichtbaren macOS/Cocoa-M3-Gate noch autorisiert sie RC, Tag,
  Release oder die v0.2.0 Release Readiness.
