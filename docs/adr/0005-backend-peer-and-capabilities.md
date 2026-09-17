# ADR 0005 – Backend/peer architecture and capabilities

**Status:** Accepted  
**Date:** 2026-09-17

## English

### Context

The toolkit must support very different environments: native desktop systems, rendered desktop windows and terminals. AWT's peer concept is a useful precedent, while Swing shows that not every component needs its own native operating-system object.

### Decision

SASD UI Toolkit separates public component semantics from platform representation through a **backend/peer architecture**.

- Application code works against backend-independent components.
- A backend maps those semantics to native controls, rendered controls, terminal cells or a mixture.
- Native and rendered peers may coexist within a backend when that produces the best platform result.
- Backend-specific types must not leak into normal public application APIs.
- Backends expose explicit capabilities instead of pretending every platform supports the same features.

Representative capabilities may include:

```text
mouse / pointer
clipboard
true color
native menus
multiple native windows
drag and drop
IME
accessibility bridge
system tray
notifications
printing
```

A feature can therefore be unavailable, emulated or natively supported without corrupting the semantic component API.

### Rationale

This model provides a stable application-facing abstraction while preserving access to native system resources where valuable. It avoids the false choice between 'everything native' and 'everything custom-rendered'.

### Consequences

- Backend contracts must remain smaller than public widget APIs.
- Capability queries are part of portability rather than an afterthought.
- Backend conformance tests are required as implementations grow.

## Deutsch

### Kontext

Das Toolkit soll sehr unterschiedliche Umgebungen bedienen: native Desktop-Systeme, selbst gerenderte Desktop-Fenster und Terminals. AWT liefert mit dem Peer-Prinzip ein bewährtes Vorbild; Swing zeigt gleichzeitig, dass nicht jedes UI-Element ein eigenes natives Betriebssystemobjekt benötigt.

### Entscheidung

Das SASD UI Toolkit trennt die öffentliche Komponentensemantik über eine **Backend-/Peer-Architektur** von ihrer plattformspezifischen Darstellung.

Anwendungscode arbeitet mit backendneutralen Komponenten. Das Backend bildet diese auf native Controls, gerenderte Controls, Terminal-Zellen oder Mischformen ab. Native und gerenderte Peers dürfen in einem Backend nebeneinander existieren.

Unterschiede zwischen Plattformen werden über explizite `BackendCapabilities` ausgedrückt. Das Toolkit behauptet nicht, dass Maus, Drag & Drop, IME, Accessibility, native Menüs oder mehrere Fenster überall identisch verfügbar sind.

### Konsequenz

Portabilität bedeutet damit nicht, Plattformunterschiede zu verstecken, sondern sie kontrolliert hinter einem stabilen semantischen API-Vertrag zu behandeln.