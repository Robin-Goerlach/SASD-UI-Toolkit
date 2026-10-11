# ADR 0148 – RenderedPresentationSink consumes the TreeView snapshot

**Status:** Accepted  
**Date:** 2026-10-11

## English

### Decision

`RenderedPresentationSink` recognizes `TreeView` as a normal backend-neutral rendered widget. It
passes the widget's arranged logical bounds to `RenderedTreeViewPresentation::snapshot()` and
publishes only the resulting owned snapshot through transactional `DisplayList` rendering. The sink
does not retain a model, node path, or presentation snapshot between synchronizations.

Tree traversal and semantic state remain Core responsibilities. Rendered owns only the final
disclosure/row geometry and visual style. An invisible TreeView is erased using its arranged bounds;
an unrepresentable or malformed visible tree is deferred without replacing the existing display
list. This keeps the sink's existing incremental synchronization contract and makes TreeView a
consumer of the same public Rendered pipeline as ListView and TableView.

## Deutsch

### Entscheidung

`RenderedPresentationSink` behandelt `TreeView` als normales backendneutrales Rendered-Widget. Er
übergibt die arrangierten logischen Bounds an `RenderedTreeViewPresentation::snapshot()` und
veröffentlicht ausschließlich das daraus entstehende besitzende Snapshot über transaktionales
`DisplayList`-Rendering. Der Sink hält zwischen Synchronisierungen weder Model, Node-Pfad noch
Presentation-Snapshot.

Tree-Traversierung und semantischer Zustand bleiben Aufgaben von Core. Rendered besitzt nur finale
Disclosure-/Zeilen-Geometrie und visuellen Stil. Ein unsichtbarer TreeView wird über seine
arrangierten Bounds gelöscht; ein nicht darstellbarer oder fehlerhafter sichtbarer Baum wird ohne
Ersetzen der bestehenden DisplayList zurückgestellt. Damit bleibt der bestehende inkrementelle
Synchronisationsvertrag erhalten und TreeView verwendet dieselbe öffentliche Rendered-Pipeline wie
ListView und TableView.
