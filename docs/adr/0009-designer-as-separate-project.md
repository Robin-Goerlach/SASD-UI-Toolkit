# ADR 0009 – Visual designer as a separate project

**Status:** Accepted  
**Date:** 2026-09-17

## English

### Context

VCL's visual RAD workflow was one of its strongest productivity features, and a future SASD visual designer is desirable. However, implementing a component framework and a complete form designer/IDE at the same time would multiply scope and force unstable metadata and serialization decisions too early.

### Decision

The **visual designer/RAD environment is not part of the SASD UI Toolkit core**.

The toolkit may later expose the foundations a designer needs, such as:

- component metadata;
- inspectable/editable properties;
- stable identifiers;
- serialization/deserialization of UI descriptions;
- design-time validation;
- component palette metadata.

A full visual designer should be developed as a separate sister project consuming those public capabilities.

### Rationale

This lets the toolkit stabilize independently and avoids coupling the runtime API to one editor implementation. It also permits more than one designer or IDE integration in the future.

### Consequences

- No M1/M2 milestone is blocked on drag-and-drop design tooling.
- Runtime APIs should remain designer-friendly where doing so does not damage the runtime design.
- Metadata/serialization should be introduced from real requirements, not by prematurely recreating Delphi's DFM system or Qt Designer formats.

## Deutsch

### Kontext

Der visuelle RAD-Workflow war eine große Stärke der VCL, und ein zukünftiger SASD-Designer wäre wertvoll. Würden wir jedoch Komponentenframework und vollständigen Form-Designer bzw. IDE gleichzeitig entwickeln, würde der Scope stark wachsen und wir müssten Metadaten- und Serialisierungsentscheidungen treffen, bevor das Komponentenmodell stabil ist.

### Entscheidung

Der **visuelle Designer bzw. die RAD-Umgebung gehört nicht zum Core des SASD UI Toolkit**.

Das Toolkit darf später die dafür notwendigen Grundlagen bereitstellen: Komponentenmetadaten, editierbare Properties, stabile Identifikatoren, UI-Serialisierung, Design-Time-Validierung und Palette-Metadaten.

Der eigentliche visuelle Designer soll als **separates Schwesterprojekt** auf diesen öffentlichen Funktionen aufbauen.

### Konsequenz

Die frühen Releases werden nicht durch Drag-and-Drop-Tooling blockiert. Gleichzeitig soll die Runtime-Architektur Designer-Fähigkeit nicht unnötig verhindern.