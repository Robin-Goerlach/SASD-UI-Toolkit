# ADR 0139 – Rectangular textual TableModel contract

**Status:** Accepted  
**Date:** 2026-10-09

## English

### Context

The first M5 List slice established that large data sets need a small semantic model boundary,
lifetime-safe observation and owned presentation snapshots. Tables add a second positional dimension,
but do not yet have a concrete editing, role, sorting or delegate consumer in the repository.

### Decision

`TableModel` exposes a rectangular textual contract: row count, column count, copied-on-observation
header text and copied-on-observation cell text. It has the same lifetime-safe `Reference` and
move-only `Subscription` pattern as `ListModel`, plus a monotonic revision and post-mutation change
descriptions. The initial concrete `StringTableModel` is test/demo storage only.

Rows and columns are positional, not durable application identities. A model mutation must restore
the rectangular invariant before notifying observers. A future `TableView` may virtualize visible
rows and columns, but it must own all values retained by a presentation snapshot and must fail closed
when a model reference or revision is stale.

No roles, sorting, editing, cell delegates, renderer objects or general binding system are introduced
until a concrete consumer requires them.

### Consequences

- Core remains backend-neutral and does not depend on Terminal, Rendered or SDL types.
- A model can represent a large or generated data set without creating a Widget per cell.
- Borrowed model text cannot leak into a retained presentation snapshot.
- The initial API is intentionally smaller than a spreadsheet model; later additions must preserve the
  explicit observation and lifetime boundary.

## Deutsch

### Kontext

Der erste M5-List-Slice hat gezeigt, dass große Datenmengen eine kleine semantische Modellgrenze,
lebenszeitsichere Observation und besitzende Presentation-Snapshots benötigen. Tabellen ergänzen eine
zweite Positionsdimension; im Repository gibt es jedoch noch keinen konkreten Consumer für Editing,
Rollen, Sortierung oder Delegates.

### Entscheidung

`TableModel` stellt einen rechteckigen Textvertrag bereit: Zeilenanzahl, Spaltenanzahl, beim Beobachten
zu kopierende Header- und Zelltexte. Es verwendet wie `ListModel` eine lebenszeitsichere `Reference`,
eine verschiebbare `Subscription`, eine monotone Revision und Änderungsbeschreibungen nach der Mutation.
`StringTableModel` dient zunächst nur als Test-/Demo-Speicher.

Zeilen und Spalten sind positionale Positionen, keine dauerhaften Anwendungsidentitäten. Vor einer
Benachrichtigung muss die Mutation die Rechteck-Invariante wiederherstellen. Eine spätere `TableView`
kann sichtbare Zeilen und Spalten virtualisieren, muss alle behaltenen Werte im Presentation-Snapshot
besitzen und bei veralteter Model-Referenz oder Revision fail-closed reagieren.

Rollen, Sortierung, Editing, Cell-Delegates, Renderer-Objekte und ein allgemeines Binding-System werden
erst bei einem konkreten Consumer eingeführt.

### Konsequenzen

- Der Core bleibt backendneutral und kennt weder Terminal-, Rendered- noch SDL-Typen.
- Große oder generierte Datenmengen benötigen kein Widget pro Zelle.
- Geliehener Modelltext kann nicht in einen länger lebenden Presentation-Snapshot gelangen.
- Die erste API ist bewusst kleiner als ein Spreadsheet-Modell; spätere Erweiterungen müssen die klare
  Observation- und Lebenszeitgrenze bewahren.
