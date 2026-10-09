# ADR 0141 – Owned Rendered TableView presentation snapshot

**Status:** Accepted  
**Date:** 2026-10-09

## English

The first Rendered `TableView` presentation is a value snapshot containing copied headers/cells,
final header/column/row rectangles and the model revision observed while building it. A supplied
`RenderedMeasurementContext` measures each visible lane once; rendering and hit testing consume the
stored rectangles and do not query the model or measure again. Fixed one-unit row height and measured
visible-column widths are the deliberately small first geometry contract. Malformed public snapshots
fail closed, and rendering stages changes in a copied `DisplayList` before replacing the caller's list.
Terminal presentation and table-specific interaction remain separate consumers.

## Deutsch

Die erste Rendered-`TableView`-Presentation ist ein Value-Snapshot mit kopierten Headern/Zellen,
endgültigen Header-/Spalten-/Zeilen-Rechtecken und der beim Aufbau beobachteten Model-Revision. Ein
übergebener `RenderedMeasurementContext` misst jede sichtbare Lane einmal; Rendering und Hit-Test
verwenden die gespeicherten Rechtecke und fragen Modell oder Messung nicht erneut ab. Eine feste
einheitliche Zeilenhöhe und gemessene sichtbare Spaltenbreiten bilden den bewusst kleinen ersten
Geometrievertrag. Öffentliche fehlerhafte Snapshots fail-closed; Rendering arbeitet zunächst auf einer
kopierten `DisplayList`, bevor die Liste des Aufrufers ersetzt wird. Terminal-Presentation und
tabellenspezifische Interaktion bleiben separate Consumer.
