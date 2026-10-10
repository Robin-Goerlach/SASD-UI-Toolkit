# ADR 0146 – Owned Terminal TreeView snapshot and exact hit testing

**Status:** Accepted
**Date:** 2026-10-10

## English

### Decision

Terminal TreeView presentation copies only the Core view's visible rows into an owned snapshot.
Each row stores its positional `TreeNodePath`, final one-cell row rectangle and final disclosure
indicator rectangle, together with copied text, depth, expansion and selection state plus the observed
model revision. ASCII disclosure markers keep the first slice portable and do not require Unicode
glyph support.

Rendering and hit testing consume this exact geometry and never re-query the model. Rendering
preflights text, identity and geometry and publishes through a copied `ScreenBuffer`, so malformed
public snapshots cannot partially paint a frame. Row selection and indicator toggling validate the
snapshot revision before delegating to Core `TreeSelectionModel` or `TreeView`.

## Deutsch

### Entscheidung

Die Terminal-TreeView-Presentation kopiert ausschließlich die sichtbaren Core-Zeilen in ein
besitzendes Snapshot. Jede Zeile speichert ihren positionalen `TreeNodePath`, das finale einzeilige
Zeilenrechteck und das finale Disclosure-Indikator-Rechteck sowie kopierten Text, Tiefe,
Expansion-/Selection-Zustand und die beobachtete Model-Revision. ASCII-Indikatoren halten den ersten
Slice portabel und setzen keine Unicode-Glyphen voraus.

Rendering und Hit-Testing verwenden genau diese Geometrie und fragen das Modell nicht erneut ab.
Rendering prüft Text, Identität und Geometrie vollständig vor und veröffentlicht über einen kopierten
`ScreenBuffer`; fehlerhafte öffentliche Snapshots können daher keinen halbgemalten Frame erzeugen.
Zeilen-Selection und Indikator-Toggle prüfen die Snapshot-Revision und delegieren danach an das Core-
`TreeSelectionModel` beziehungsweise `TreeView`.
