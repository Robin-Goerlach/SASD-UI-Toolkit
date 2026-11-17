# SASD UI Toolkit Documentation

This directory contains the project documentation in **German** and **English**.

> **Project status:** M8 Developer Productivity Foundations is complete. Version 0.2.0 remains
> unreleased; the visible macOS/Cocoa M3 validation and a separate release-readiness review remain
> external gates. The documented API is pre-1.0 and is not yet a long-term source-compatibility
> promise.

## SASD governance and traceability

- [Project classification](SASD_PROJECT_CLASSIFICATION.md)
- [Canonical Core requirement matrix](SASD_CORE_REQUIREMENT_MATRIX.md)
- [Canonical project requirements](SASD_PROJECT_REQUIREMENTS.md)
- [Security assessment](SASD_SECURITY_ASSESSMENT.md)
- [English alignment declaration](en/SASD_COMPLIANCE.md)
- [German alignment summary](de/SASD_COMPLIANCE.md)
- [Root Security Policy](../SECURITY.md)
- [Code of Conduct](../CODE_OF_CONDUCT.md)

English is authoritative for the canonical classification, compliance declaration, requirement
matrix, project requirements and security assessment. German compliance text is an informative
same-commit summary that links to those sources. Other paired English/German technical documents are
maintained with the same technical intent in one repository commit; bilingual ADRs remain
authoritative within their single files.

## Content

### English

- [Goals and scope](en/GOALS_AND_SCOPE.md)
- [Architecture](en/ARCHITECTURE.md)
- [Headless tooling integration](en/TOOLING_INTEGRATION.md)
- [UI Description JSON v1](en/UI_DESCRIPTION_FORMAT.md)
- [UI Description JSON v2](en/UI_DESCRIPTION_FORMAT_V2.md)
- [Backend and platform strategy](en/BACKENDS.md)
- [Development guidelines](en/DEVELOPMENT_GUIDELINES.md)
- [Roadmap](en/ROADMAP.md)
- [SASD alignment declaration](en/SASD_COMPLIANCE.md)
- [M8 exit review](en/M8_EXIT_REVIEW.md)
- [M2 terminal smoke test](en/M2_TERMINAL_SMOKE_TEST.md)
- [M3 rendered desktop smoke test](en/M3_RENDERED_DESKTOP_SMOKE_TEST.md)
- [v0.1.0 release checklist](en/V0_1_0_RELEASE_CHECKLIST.md)
- [v0.1.0 release notes](en/V0_1_0_RELEASE_NOTES.md)
- [Inspirations and references](en/REFERENCES.md)

### Deutsch

- [Projektziele und Umfang](de/ZIELE_UND_UMFANG.md)
- [Architektur](de/ARCHITEKTUR.md)
- [Headless-Tooling-Integration](de/TOOLING_INTEGRATION.md)
- [UI Description JSON v1](de/UI_DESCRIPTION_FORMAT.md)
- [UI Description JSON v2](de/UI_DESCRIPTION_FORMAT_V2.md)
- [Backend- und Plattformstrategie](de/BACKENDS.md)
- [Entwicklungsrichtlinien](de/ENTWICKLUNGSRICHTLINIEN.md)
- [Roadmap](de/ROADMAP.md)
- [SASD-Alignment-Zusammenfassung](de/SASD_COMPLIANCE.md)
- [M8 Exit Review](de/M8_EXIT_REVIEW.md)
- [M2-Terminal-Smoke-Test](de/M2_TERMINAL_SMOKE_TEST.md)
- [M3-Rendered-Desktop-Smoke-Test](de/M3_RENDERED_DESKTOP_SMOKE_TEST.md)
- [v0.1.0-Release-Checkliste](de/V0_1_0_RELEASE_CHECKLIST.md)
- [v0.1.0-Release-Notes](de/V0_1_0_RELEASE_NOTES.md)
- [Vorbilder und Referenzen](de/REFERENZEN.md)

### Architecture Decision Records

- [ADR index / ADR-Übersicht](adr/README.md)

The ADR series records durable rationale for decisions such as C++20/CMake, public naming, component ownership, backend/peer architecture, the headless/mock backend, terminal support, Model/View, designer separation and release compatibility.

## Documentation policy

Architecture decisions should be documented before they become difficult to reverse. The German and English documents should describe the same technical intent; they do not have to be literal translations, but neither language should contain important decisions that are missing from the other.

ADRs are bilingual in a single file so their rationale cannot silently diverge between language versions.

When implementation starts, API reference documentation should be generated from the C++ source (for example with Doxygen) while these Markdown documents remain the place for concepts, decisions, tutorials and contributor guidance.

## Third-party notices

- [Terminal Unicode width-table data](third-party/terminal-unicode-width-tables.md)
