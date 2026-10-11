# ADR 0149 – SDL3 demo consumes the backend-neutral TreeView pipeline

**Status:** Accepted  
**Date:** 2026-10-11

## English

### Decision

The rendered SDL3 form demo includes a small application-owned `StringTreeModel`, a dedicated
`TreeSelectionModel` and one Core `TreeView` widget. The demo expands and presents ordinary
application data through `RenderedPresentationSink`; pointer input builds the same owned
`RenderedTreeViewPresentationSnapshot` used for painting and delegates disclosure toggling or row
selection to Core.

The demo stores no node pointer, row index or duplicate expansion state. SDL3 remains only the
measurement, event-normalization and final device adapter. Menus and ComboBox retain their existing
modal overlay precedence; TreeView is part of the normal Widget tree and therefore cannot compete
with an active transient overlay.

## Deutsch

### Entscheidung

Die gerenderte SDL3-Form-Demo enthält ein kleines, von der Anwendung besessenes `StringTreeModel`,
ein dediziertes `TreeSelectionModel` und genau ein Core-`TreeView`-Widget. Die Demo erweitert und
präsentiert normale Anwendungsdaten über `RenderedPresentationSink`; Pointer-Eingaben erzeugen
dieselben besitzenden `RenderedTreeViewPresentationSnapshot`-Daten wie das Rendering und delegieren
Disclosure-Toggle beziehungsweise Zeilen-Selection an Core.

Die Demo speichert weder Node-Pointer noch Zeilenindex oder eine doppelte Expansion-State-Machine.
SDL3 bleibt ausschließlich Mess-, Event-Normalisierungs- und finales Device-Backend. Menüs und
ComboBox behalten ihre bestehende modale Overlay-Priorität; TreeView gehört zum normalen Widget-Baum
und konkurriert daher nicht mit einem aktiven transienten Overlay.
