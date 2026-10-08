# ADR 0133: Rendered menu popup snapshots

Status: Accepted
Date: 2026-10-08
Predecessors: ADR 0122 (anchored presentation placement)

## English

The first Rendered menu slice introduces a backend-neutral popup presentation snapshot. It copies the
existing Core `MenuPopupPresentationSnapshot` into owned rows, resolves final logical rectangles once,
and uses those same rectangles for DisplayList rendering and pointer hit testing. A stale or malformed
snapshot fails closed before mutating a DisplayList. The snapshot owns no MenuModel, Command,
measurement-context or SDL object.

This deliberately covers one popup surface only. Menu-bar placement, nested-popup composition and
semantic pointer transactions remain follow-up work; no generic overlay manager is introduced until a
second concrete consumer requires one.

## Deutsch

Der erste Rendered-Menü-Slice führt einen backend-neutralen Präsentations-Snapshot für ein Popup ein.
Die bestehende Core-`MenuPopupPresentationSnapshot` wird in besitzende Zeilen kopiert, die endgültigen
logischen Rechtecke werden genau einmal bestimmt, und dieselben Rechtecke dienen Rendering und
Pointer-Hit-Test. Veraltete oder fehlerhafte Snapshots schlagen geschlossen fehl, bevor eine
DisplayList verändert wird. Der Snapshot besitzt weder MenuModel, Command, Measurement-Kontext noch
SDL-Objekte.

Bewusst umfasst dieser Slice nur eine Popup-Fläche. Menüleisten-Platzierung, verschachtelte Popup-
Komposition und semantische Pointer-Transaktionen folgen separat; ein allgemeiner Overlay-Manager wird
erst bei einem zweiten konkreten Verbraucher eingeführt.
