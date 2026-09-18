# SASD UI Toolkit Documentation

This directory contains the project documentation in **German** and **English**.

> **Project status:** architecture/bootstrap phase. The documented API and roadmap describe the intended direction and are not yet a stability promise.

## Content

### English

- [Goals and scope](en/GOALS_AND_SCOPE.md)
- [Architecture](en/ARCHITECTURE.md)
- [Backend and platform strategy](en/BACKENDS.md)
- [Development guidelines](en/DEVELOPMENT_GUIDELINES.md)
- [Roadmap](en/ROADMAP.md)
- [Inspirations and references](en/REFERENCES.md)

### Deutsch

- [Projektziele und Umfang](de/ZIELE_UND_UMFANG.md)
- [Architektur](de/ARCHITEKTUR.md)
- [Backend- und Plattformstrategie](de/BACKENDS.md)
- [Entwicklungsrichtlinien](de/ENTWICKLUNGSRICHTLINIEN.md)
- [Roadmap](de/ROADMAP.md)
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
