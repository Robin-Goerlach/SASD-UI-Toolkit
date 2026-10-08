# ADR 0137 – Terminal ListView snapshots and exact row hit testing

**Status:** Accepted
**Date:** 2026-10-08

## English

Terminal ListView presentation copies only the Core view's visible rows into an owned snapshot. The
snapshot keeps copied UTF-8, semantic row identity, selection and focus presentation state. Rendering
and pointer hit testing consume that same snapshot; hit testing never re-queries the model or derives a
second row geometry. One terminal cell row is assigned to each visible item. Unsupported text or
malformed snapshot geometry fails closed before modifying the buffer.

## Deutsch

Die Terminal-Presentation einer ListView kopiert ausschließlich die sichtbaren Zeilen der Core-View in
einen besitzenden Snapshot. Der Snapshot enthält kopierten UTF-8-Text, semantische Zeilenidentität sowie
Selection- und Fokusdarstellung. Rendering und Pointer-Hit-Test verwenden genau diesen Snapshot; der
Hit-Test fragt das Modell nicht erneut ab und leitet keine zweite Zeilengeometrie ab. Jede sichtbare
Zeile erhält genau eine Terminalzeile. Nicht darstellbarer Text oder fehlerhafte Snapshot-Geometrie wird
vor einer Buffer-Änderung sicher verworfen.
