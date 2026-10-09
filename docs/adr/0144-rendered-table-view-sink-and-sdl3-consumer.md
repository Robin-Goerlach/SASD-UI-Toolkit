# ADR 0144 – Rendered TableView sink integration and SDL3 consumer

**Status:** Accepted
**Date:** 2026-10-09

## English

The generic `RenderedPresentationSink` recognizes `TableView` and consumes the existing owned
`RenderedTableViewPresentationSnapshot` contract. It does not measure a model or retain a model
pointer: Core supplies the visible semantic values, the Rendered builder measures and fixes final
lanes once, and rendering consumes that value snapshot. Hosts use the same snapshot contract for
exact pointer selection with model-revision validation.

The SDL3 form demo now owns a small semantic `StringTableModel`, `TableSelectionModel` and `TableView`
consumer. Keyboard navigation remains in `TableView`; pointer selection uses
`RenderedTableViewPresentation::selectAt()`. The demo creates no Widget per cell and keeps no second
selection state machine. SDL3 remains only the measurement, event and device adapter; Core and generic
Rendered APIs remain SDL-independent.

## Deutsch

Der generische `RenderedPresentationSink` erkennt `TableView` und verwendet den bestehenden
besitzenden `RenderedTableViewPresentationSnapshot`-Vertrag. Er misst kein Model neu und hält keinen
Model-Pointer: Core liefert die sichtbaren semantischen Werte, der Rendered-Builder misst und fixiert
die finalen Lanes einmalig, und das Rendering verwendet diesen Value-Snapshot. Hosts verwenden denselben
Snapshot-Vertrag für exakte Pointer-Selection mit Model-Revision-Prüfung.

Die SDL3-Form-Demo besitzt nun einen kleinen semantischen `StringTableModel`, ein
`TableSelectionModel` und einen `TableView`-Consumer. Tastaturnavigation bleibt in `TableView`,
Pointer-Selection verwendet `RenderedTableViewPresentation::selectAt()`. Die Demo erzeugt kein Widget
pro Zelle und führt keine zweite Selection-State-Machine. SDL3 bleibt ausschließlich Measurement-,
Event- und Device-Adapter; Core und die generische Rendered-API bleiben SDL-unabhängig.
