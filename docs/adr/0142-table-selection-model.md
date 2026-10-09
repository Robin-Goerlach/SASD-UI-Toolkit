# ADR 0142 – Dedicated two-dimensional TableSelectionModel

**Status:** Accepted  
**Date:** 2026-10-09

## English

TableView uses a dedicated `TableSelectionModel` for initial single-cell selection. A selected value
is the owned value identity `(row, column)` and is intentionally separate from keyboard focus. The
model observes, but never owns, a `TableModel` through the existing lifetime-safe reference and
subscription contract. Selection is cleared on model replacement, model destruction and reset.

The model normalizes row insertion/removal only through the mutation ranges that `TableModel`
currently publishes. Column structure is not invented here because the current TableModel contract
has fixed columns and no column insertion/removal notifications. Navigation is bounded and fails
closed at an edge. Selection callbacks receive copied before/after values; application callbacks may
destroy the selection owner, so the implementation performs no member access after notification.

`ListSelectionModel` is not reused: its one-dimensional row identity cannot express the table cell
contract without hiding a new policy in a supposedly universal abstraction.

## Deutsch

TableView verwendet für die anfängliche Single-Cell-Selection ein eigenes
`TableSelectionModel`. Der ausgewählte Wert ist die besitzende Identität `(row, column)` und bleibt
bewusst vom Tastaturfokus getrennt. Das Modell beobachtet ein `TableModel`, besitzt es aber nie; dafür
werden die vorhandenen lebenszeitsicheren Referenz- und Subscription-Verträge verwendet. Bei
Model-Ersatz, Model-Zerstörung und Reset wird die Selection gelöscht.

Zeilen-Einfügungen und -Löschungen werden ausschließlich anhand der Änderungsbereiche normalisiert,
die der aktuelle `TableModel` tatsächlich meldet. Eine Spaltenstruktur wird hier nicht erfunden,
weil der aktuelle TableModel-Vertrag feste Spalten und keine Spalten-Einfügungs-/Löschmeldungen
besitzt. Navigation bleibt an den Rändern begrenzt und behandelt ungültigen Zustand fail-closed.
Selection-Callbacks erhalten kopierte Vorher-/Nachher-Werte; Anwendungscode darf den Besitzer der
Selection während des Callbacks zerstören, daher erfolgt danach kein Memberzugriff mehr.

`ListSelectionModel` wird nicht wiederverwendet: Seine eindimensionale Zeilenidentität kann den
Tabellenzellvertrag nicht ausdrücken, ohne eine neue Policy in eine vermeintlich universelle
Abstraktion zu verstecken.
