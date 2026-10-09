# ADR 0140 – TableView core virtualization and owned visible values

**Status:** Accepted  
**Date:** 2026-10-09

## English

### Context

The rectangular `TableModel` contract now has a first consumer. A view must expose a useful visible
range without walking a large model or creating one Widget per cell. The repository does not yet have
a table-specific two-dimensional selection or delegate consumer.

### Decision

`TableView` stores only a lifetime-safe `TableModel::Reference` and a logical row/column viewport. Its
`visibleHeaders()` and `visibleRows()` methods copy only the requested rectangle into owned values.
Viewport origins are clamped to the current model dimensions; an expired model produces empty values.
The view remains non-interactive until a concrete table selection contract exists.

The first core slice intentionally does not reuse `ListSelectionModel`: that type is bound to
`ListModel`, and pretending a row-only selection is a complete table selection would freeze the wrong
public abstraction. Delegates, editors, roles, sorting and exact backend geometry remain later slices.

### Consequences

- Large rectangular data sets are queried only for visible headers/cells.
- Backends receive owned values and cannot retain model pointers or `string_view` accidentally.
- Table-specific interaction can be added without breaking a misleading list-selection API.

## Deutsch

### Kontext

Der rechteckige `TableModel`-Vertrag hat nun einen ersten Consumer. Eine View muss einen sichtbaren
Bereich bereitstellen, ohne ein großes Modell vollständig zu durchlaufen oder ein Widget pro Zelle zu
erzeugen. Eine zweidimensionale Selection oder ein konkreter Delegate-Consumer existiert noch nicht.

### Entscheidung

`TableView` speichert nur eine lebenszeitsichere `TableModel::Reference` und einen logischen Zeilen-/
Spalten-Viewport. `visibleHeaders()` und `visibleRows()` kopieren nur das angeforderte Rechteck in
besitzende Werte. Viewport-Ursprünge werden an die aktuellen Modelldimensionen angepasst; ein
abgelaufenes Modell liefert leere Werte. Die View bleibt bis zu einem konkreten Table-Selection-Vertrag
nicht interaktiv.

`ListSelectionModel` wird bewusst nicht wiederverwendet: Dieser Typ ist an `ListModel` gebunden, und
eine reine Zeilen-Selection als vollständige Tabellen-Selection würde die falsche öffentliche
Abstraktion festschreiben. Delegates, Editoren, Rollen, Sortierung und exakte Backend-Geometrie folgen
erst in späteren Slices.

### Konsequenzen

- Große rechteckige Datenmengen fragen nur sichtbare Header/Zellen ab.
- Backends erhalten besitzende Werte und können nicht versehentlich Model-Pointer oder `string_view`
  behalten.
- Tabelleninteraktion kann später ergänzt werden, ohne eine irreführende List-Selection-API zu brechen.
