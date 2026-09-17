# SASD UI Toolkit Documentation

This directory contains the project documentation in **German** and **English**.

> **Project status:** architecture/bootstrap phase. The documented API and roadmap describe the intended direction and are not yet a stability promise.

## Content

- [Goals and scope](en/GOALS_AND_SCOPE.md)
- [Architecture](en/ARCHITECTURE.md)
- [Backend and platform strategy](en/BACKENDS.md)
- [Development guidelines](en/DEVELOPMENT_GUIDELINES.md)
- [Roadmap](en/ROADMAP.md)
- [Inspirations and references](en/REFERENCES.md)

## Documentation policy

Architecture decisions should be documented before they become difficult to reverse. The German and English documents should describe the same technical intent; they do not have to be literal translations, but neither language should contain important decisions that are missing from the other.

When implementation starts, API reference documentation should be generated from the C++ source (for example with Doxygen) while these Markdown documents remain the place for concepts, decisions, tutorials and contributor guidance.
