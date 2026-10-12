# ADR 0150 – M5 Model/View review keeps List, Table and Tree contracts explicit

**Status:** Accepted
**Date:** 2026-10-12

## English

### Context

M5 now contains three real Model/View consumers: List, Table and Tree. They all need lifetime-safe
observation, conservative revision handling and owned presentation snapshots, but their identities,
mutation semantics and selection dimensions differ. List selects a row, Table selects a two-dimensional
cell, and Tree selects a positional node path whose visibility also depends on expansion state.

### Decision

Keep `ListModel`/`ListSelectionModel`, `TableModel`/`TableSelectionModel` and
`TreeModel`/`TreeSelectionModel` as explicit public contracts. Do not introduce a universal model,
selection, delegate or renderer base class as part of M5.

The reusable architectural boundary is procedural rather than a public type hierarchy:

- models expose lifetime-safe observation and monotonically changing revisions;
- views materialize only the visible semantic range;
- backend presentation copies the data it needs into owned final snapshots;
- rendering and hit testing consume the same final geometry;
- stale or malformed snapshots fail closed before mutating a target frame or semantic state.

Delegate/cell-renderer and general binding/validation facilities remain deferred. The current three
consumers all present owned UTF-8 text and do not require divergent editor or renderer strategies.
They may be reconsidered when a concrete control demonstrates that need.

### Consequences

The APIs remain small and honest, and Tree-specific path/expansion semantics do not get hidden behind
a lowest-common-denominator abstraction. Some implementation patterns remain duplicated across the
three models, but that cost is preferable to freezing a false public common contract before a fourth
consumer or a materially different presentation strategy exists.

## Deutsch

### Kontext

M5 enthält nun drei echte Model-/View-Consumer: List, Table und Tree. Alle benötigen lebenszeitsichere
Observation, konservative Revisionsprüfung und besitzende Presentation-Snapshots. Ihre Identitäten,
Mutationssemantik und Selection-Dimensionen unterscheiden sich jedoch: List selektiert eine Zeile,
Table eine zweidimensionale Zelle und Tree einen positionsbezogenen Node-Pfad, dessen Sichtbarkeit
zusätzlich vom Expansion-State abhängt.

### Entscheidung

`ListModel`/`ListSelectionModel`, `TableModel`/`TableSelectionModel` und
`TreeModel`/`TreeSelectionModel` bleiben eigenständige öffentliche Verträge. Im Rahmen von M5 wird
keine universelle Model-, Selection-, Delegate- oder Renderer-Basisklasse eingeführt.

Die wiederverwendbare Architekturgrenze ist ein Ablaufmuster und keine öffentliche Typhierarchie:

- Models bieten lebenszeitsichere Observation und monoton steigende Revisionen;
- Views materialisieren nur den sichtbaren semantischen Bereich;
- Backend-Presentation kopiert benötigte Daten in besitzende finale Snapshots;
- Rendering und Hit-Testing verwenden dieselbe finale Geometrie;
- veraltete oder malformed Snapshots fail-closed, bevor Frame oder semantischer State verändert wird.

Delegate-/Cell-Renderer- und allgemeine Binding-/Validation-Grundlagen bleiben zurückgestellt. Die
aktuellen drei Consumer präsentieren ausschließlich besitzenden UTF-8-Text und benötigen keine
unterschiedlichen Editor- oder Renderer-Strategien. Die Entscheidung kann bei einem konkreten Bedarf
eines Controls erneut geprüft werden.

### Konsequenzen

Die APIs bleiben klein und ehrlich; Tree-spezifische Pfad-/Expansion-Semantik wird nicht in einer
Lowest-Common-Denominator-Abstraktion versteckt. Einige Implementierungsmuster bleiben in den drei
Models getrennt, was jedoch einer falschen öffentlichen Gemeinsamkeit vorgezogen wird, solange kein
weiterer Consumer oder eine wesentlich andere Presentation-Strategie diesen Vertrag rechtfertigt.
