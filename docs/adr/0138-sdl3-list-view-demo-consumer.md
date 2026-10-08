# ADR 0138 – SDL3 demo ListView consumer

**Status:** Accepted  
**Date:** 2026-10-08

## English

The SDL3 form demo now consumes the same backend-neutral `StringListModel`, `ListSelectionModel` and
`ListView` used by headless Core/Terminal/Rendered tests. The demo keeps only a small list and handles
primary pointer selection through the Rendered snapshot hit-test contract; large-model behavior remains
in deterministic headless tests. The demo does not introduce SDL-specific row widgets, model storage or
a second selection state machine.

## Deutsch

Das SDL3-Formular-Demo verwendet nun dasselbe backendneutrale `StringListModel`,
`ListSelectionModel` und `ListView` wie die headless Core-/Terminal-/Rendered-Tests. Das Demo hält nur
eine kleine Liste und verarbeitet Primary-Pointer-Selektion über den Rendered-Snapshot-Hit-Test;
Verhalten bei großen Modellen bleibt in deterministischen headless Tests. Es gibt keine SDL-spezifischen
Zeilen-Widgets, keine doppelte Modelldatenhaltung und keine zweite Selection-State-Machine.
