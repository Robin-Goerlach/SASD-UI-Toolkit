# ADR 0008 – Model/View for data-heavy widgets

**Status:** Accepted  
**Date:** 2026-09-17

## English

### Context

Lists, trees and tables may represent very large data sets. Creating one heavy UI object per row, node or cell does not scale and would be especially problematic for future SASD statistical, spreadsheet and engineering applications.

### Decision

Data-heavy widgets use a **Model/View architecture** inspired by the separation proven in mature frameworks such as Qt.

Planned concepts include:

- `ListModel`
- `TableModel`
- `TreeModel`
- selection models
- view components such as `ListView`, `TableView` and `TreeView`
- delegate/cell-renderer concepts where editing or presentation needs specialization
- virtualization so the UI only realizes what is required for the current viewport

The model layer must not depend on a concrete backend. Backends render and interact with view semantics rather than owning application data.

### Rationale

This separates application data from presentation, allows large data sets to remain efficient and makes the same data model usable by terminal, rendered and native views.

### Consequences

- Rich data widgets are intentionally later than the basic component core.
- `TableView` is not implemented as a container holding one child widget per cell.
- Binding/validation concepts should integrate with models rather than creating a separate incompatible data path.

## Deutsch

### Kontext

Listen, Bäume und Tabellen können sehr große Datenmengen repräsentieren. Für jede Zeile, jeden Knoten oder jede Zelle ein schwergewichtiges UI-Objekt zu erzeugen, skaliert schlecht und wäre für spätere SASD-Statistik-, Spreadsheet- und Engineering-Anwendungen besonders problematisch.

### Entscheidung

Datenreiche Widgets verwenden eine **Model/View-Architektur**.

Vorgesehen sind unter anderem `ListModel`, `TableModel`, `TreeModel`, Selection Models, entsprechende Views sowie bei Bedarf Delegate-/Cell-Renderer-Konzepte und Virtualisierung.

Das Modell bleibt backendneutral. Backends stellen Views dar und verarbeiten Interaktion, besitzen aber nicht die eigentlichen Anwendungsdaten.

### Konsequenzen

Komplexe Daten-Widgets kommen bewusst nach dem Basiskern. Ein `TableView` wird insbesondere nicht als Container mit einem eigenständigen Widget pro Zelle entworfen.