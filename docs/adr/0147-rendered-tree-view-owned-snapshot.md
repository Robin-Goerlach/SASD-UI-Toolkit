# ADR 0147 – Owned Rendered TreeView snapshot and exact hit testing

**Status:** Accepted  
**Date:** 2026-10-10

## English

### Decision

Rendered TreeView presentation is an SDL-independent owned value snapshot of the Core view's
currently visible rows. Each row copies its positional `TreeNodePath`, UTF-8 text, depth,
expansion/selection state, model revision and final row/disclosure rectangles. The snapshot is the
single geometry source for rendering and hit testing; neither operation re-measures or re-places a
node.

The builder rejects unrepresentable depth/coordinate arithmetic and malformed visible-row geometry.
Rendering first validates the complete public snapshot and paints a staged `DisplayList`, so a
malformed or stale value cannot leave a partially composed frame. Pointer helpers validate the
observed model revision before delegating row selection or disclosure toggling to Core
`TreeSelectionModel` and `TreeView`. Core therefore remains the owner of node identity, expansion
and selection semantics, while Rendered owns graphical geometry.

The first slice uses portable ASCII disclosure markers (`[-]` and `[+]`) and one logical row per
visible node. Copying visible text as a value keeps lifetime boundaries explicit and makes stale
presentation fail closed.

## Deutsch

### Entscheidung

Die Rendered-TreeView-Presentation ist ein SDL-unabhängiger, besitzender Value-Snapshot der aktuell
sichtbaren Core-Zeilen. Jede Zeile kopiert ihren positionalen `TreeNodePath`, UTF-8-Text, Tiefe,
Expansion-/Selection-Zustand, die Model-Revision und die finalen Zeilen-/Disclosure-Rechtecke. Das
Snapshot ist die einzige Geometriequelle für Rendering und Hit-Testing; keine der beiden Operationen
misst oder platziert einen Node erneut.

Der Builder verwirft nicht darstellbare Tiefen-/Koordinatenarithmetik und fehlerhafte sichtbare
Zeilengeometrie. Das Rendering prüft zuerst das vollständige öffentliche Snapshot und malt in eine
temporäre `DisplayList`, sodass ein fehlerhafter oder veralteter Value keinen halbfertigen Frame
hinterlassen kann. Pointer-Helfer prüfen die beobachtete Model-Revision, bevor sie Selection oder
Disclosure an Core-`TreeSelectionModel` beziehungsweise `TreeView` delegieren. Core bleibt damit
Besitzer von Node-Identität, Expansion und Selection, während Rendered die grafische Geometrie besitzt.

Der erste Slice verwendet portable ASCII-Disclosure-Marker (`[-]` und `[+]`) und genau eine logische
Zeile pro sichtbarem Node. Sichtbarer Text wird als Value kopiert; dadurch bleiben Lifetime-Grenzen
explizit und stale Presentation wird fail-closed behandelt.
