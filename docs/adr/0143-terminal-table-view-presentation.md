# ADR 0143 – Owned Terminal TableView presentation

**Status:** Accepted
**Date:** 2026-10-09

## English

Terminal `TableView` presentation uses an owned value snapshot containing the visible headers, rows,
semantic model column identities, final header/column/row/cell rectangles, focus/selection metadata,
the Unicode ambiguous-width policy and the observed model revision. `TextMetrics` measures visible
content once while building the snapshot. Rendering decodes only already accepted single-line text
into the stored cell lanes and never asks the model for replacement values or geometry.

The renderer preflights the complete public snapshot and draws into a copied `ScreenBuffer` before
publishing it. Malformed geometry, unsupported text, a width-policy mismatch or overflow therefore
cannot leave a partially painted frame. Hit testing uses the same final cell rectangles and returns
semantic `(row,column)` identities. Selection application additionally requires the snapshot
revision to match the currently observed model, so stale frames fail closed.

## Deutsch

Die Terminal-`TableView`-Presentation verwendet ein besitzendes Value-Snapshot mit sichtbaren
Headern und Zeilen, semantischen Model-Spaltenidentitäten, endgültigen Header-/Spalten-/Zeilen- und
Zell-Rechtecken, Focus-/Selection-Metadaten, der Unicode-Ambiguous-Width-Policy und der beobachteten
Model-Revision. `TextMetrics` misst sichtbare Inhalte beim Snapshot-Aufbau genau einmal. Das
Rendering dekodiert nur den bereits akzeptierten einzeiligen Text in die gespeicherten Zell-Lanes
und fragt weder Modellwerte noch Geometrie erneut ab.

Der Renderer prüft das vollständige öffentliche Snapshot vor und zeichnet zunächst in einen kopierten
`ScreenBuffer`, bevor dieser veröffentlicht wird. Fehlerhafte Geometrie, nicht unterstützter Text,
eine abweichende Width-Policy oder Overflow können daher keinen halb gemalten Frame hinterlassen.
Der Hit-Test verwendet dieselben finalen Zell-Rechtecke und liefert semantische `(row,column)`-
Identitäten. Eine Selection-Anwendung verlangt zusätzlich dieselbe Model-Revision; stale Frames
werden fail-closed verworfen.
