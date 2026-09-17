# ADR 0004 – Component model and ownership

**Status:** Accepted  
**Date:** 2026-09-17

## English

### Context

The VCL demonstrated the productivity benefits of a coherent component model that includes both visual and non-visual components. AWT/Swing and Qt demonstrate retained UI object trees. Modern C++ additionally requires explicit lifetime and ownership semantics.

### Decision

SASD UI Toolkit uses a **retained, component-oriented object model**.

The conceptual hierarchy begins with:

```text
Component
├── non-visual components (for example Command, Action, Timer, DataSource)
└── Widget
    ├── leaf widgets (Label, Button, TextField, ...)
    └── Container
        ├── Panel
        └── Window
```

Rules:

- `Component` is the common base for toolkit components that need lifecycle/ownership/component semantics; it is not a universal base for every value type in the library.
- `Widget` represents visual or interactively presentable UI elements.
- `Container` is a widget that owns or manages a child component/widget tree.
- Lifetime management follows RAII and explicit C++ ownership.
- The public API must make ownership unambiguous. Raw pointers may be used as non-owning observations but should not encode ownership.
- Events and properties belong to component semantics, but a large reflection system is not required for the first release.
- Layout relationships and parent/child relationships must remain separable enough to support multiple backends and later designer metadata.

### Rationale

This keeps the approachability of VCL-style components without importing Delphi-specific object rules. A retained tree also gives the backend a stable semantic structure for layout, focus, accessibility and rendering.

## Deutsch

### Kontext

Die VCL zeigt den Produktivitätsgewinn eines gemeinsamen Komponentenmodells für visuelle und nicht-visuelle Komponenten. AWT/Swing und Qt arbeiten ebenfalls mit gehaltenen Objekt- bzw. Widget-Bäumen. In modernem C++ müssen Lebensdauer und Ownership zusätzlich klar und überprüfbar sein.

### Entscheidung

Das SASD UI Toolkit verwendet ein **retained, komponentenorientiertes Objektmodell**.

`Component` ist die gemeinsame Basis für Toolkit-Komponenten, die Lifecycle-/Ownership-/Komponentensemantik benötigen. `Widget` steht für visuelle bzw. interaktive UI-Elemente. `Container` verwaltet einen Kindbaum.

Ownership folgt RAII und muss in der öffentlichen API eindeutig sein. Raw Pointer dürfen Beobachter sein, sollen aber kein Eigentum ausdrücken. Ein großes Reflection-System wird für die ersten Releases nicht vorausgesetzt; Properties und Events werden schrittweise auf Basis realer Anforderungen aufgebaut.

Das Modell übernimmt damit die Stärke der VCL-Idee, ohne Delphi-spezifische Objektregeln nachzubauen.