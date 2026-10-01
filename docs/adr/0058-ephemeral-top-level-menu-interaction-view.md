# ADR 0058 – Ephemeral top-level menu interaction view / Flüchtige Top-Level-Menü-Interaktionssicht

- **Status:** Accepted
- **Date:** 2026-10-01

## English

### Context

ADR 0057 introduced read-only popup-level views so Terminal, Rendered, and future native presenters can inspect currently open popup structure without gaining access to `MenuInteractionController` internals or retaining structural pointers. The top-level menu-bar selection still required presenters to combine `menuBarSelection()` with `MenuBarModel` themselves.

That leaves the same lifetime question solved for popup levels unresolved at the menu-bar level: application code may clear or shorten the bar after an input transaction but before presentation builds the next frame. A presenter should not turn a stale controller index into an exception or retain a pointer from an earlier frame. At the same time, observation must not silently normalize controller state because input transactions are the explicit mutation boundary established by ADR 0054.

### Decision

Add `MenuBarInteractionView` and the side-effect-free `menuBarInteractionView(bar, controller)` helper to `menu_interaction_view.hpp`.

A valid view contains:

- the current top-level selection index;
- a borrowed `const MenuModel*` re-resolved from the supplied current `MenuBarModel`;
- whether the controller currently represents an open popup below that top-level menu.

The helper returns `std::nullopt` when menu interaction is inactive, no top-level selection exists, or the retained selection index is outside the current bar. It never calls controller normalization and never guesses another menu. The returned `MenuModel*` is explicitly ephemeral and may be used only for immediate synchronous presentation work; presenters must resolve a fresh view after structural mutation.

This helper deliberately preserves the existing transient-index contract. If application code destroys and rebuilds the menu bar with a different semantic menu at the same still-valid index, an index alone cannot prove that semantic identity changed. Detecting that case would require stable menu identities or generations and is outside this slice. The view must not invent such an identity layer implicitly.

### Consequences

Presenters now have one consistent read-only boundary for both top-level menu-bar state and nested popup state. They can fail closed when an index becomes obviously stale while `MenuInteractionController` remains value-only and mutation continues to happen only at explicit interaction transactions.

No backend types, native menu handles, rendering geometry, or focus objects are introduced. The helper is intentionally small and may perform a fresh indexed lookup for every frame; correctness and clear lifetime rules remain more important than avoiding trivial menu-model lookup work before profiling demonstrates a need.

## Deutsch

### Kontext

ADR 0057 führte read-only Sichten auf einzelne Popup-Ebenen ein, damit Terminal-, Rendered- und spätere native Presenter die aktuell geöffnete Popup-Struktur auslesen können, ohne Zugriff auf interne Zustände des `MenuInteractionController` zu erhalten oder strukturelle Pointer dauerhaft zu halten. Für die Top-Level-Auswahl der Menüleiste mussten Presenter `menuBarSelection()` und `MenuBarModel` dagegen noch selbst kombinieren.

Damit blieb auf Menüleistenebene dieselbe Lifetime-Frage offen, die für Popup-Ebenen bereits gelöst ist: Anwendungscode kann die Menüleiste nach einer Input-Transaktion, aber vor dem Aufbau des nächsten Frames leeren oder verkürzen. Ein Presenter darf dann einen veralteten Controller-Index weder in eine Exception noch in einen aus einem früheren Frame behaltenen Pointer verwandeln. Gleichzeitig darf reine Beobachtung den Controller nicht heimlich normalisieren, weil ADR 0054 Input-Transaktionen bewusst als explizite Mutationsgrenze definiert.

### Entscheidung

`menu_interaction_view.hpp` erhält `MenuBarInteractionView` und den nebenwirkungsfreien Helper `menuBarInteractionView(bar, controller)`.

Eine gültige Sicht enthält:

- den aktuellen Top-Level-Selection-Index;
- einen aus dem aktuell übergebenen `MenuBarModel` neu aufgelösten, ausgeliehenen `const MenuModel*`;
- die Information, ob der Controller unter diesem Top-Level-Menü aktuell ein geöffnetes Popup repräsentiert.

Der Helper liefert `std::nullopt`, wenn die Menüinteraktion inaktiv ist, keine Top-Level-Auswahl existiert oder der gespeicherte Index außerhalb der aktuellen Menüleiste liegt. Er ruft keine Controller-Normalisierung auf und errät kein Ersatzmenü. Der zurückgegebene `MenuModel*` ist ausdrücklich flüchtig und darf nur für unmittelbare synchrone Presentation-Arbeit verwendet werden; nach strukturellen Änderungen muss eine neue Sicht aufgelöst werden.

Der Helper bewahrt bewusst den bestehenden Vertrag transienter Indizes. Wenn Anwendungscode die Menüleiste zerstört und mit einem semantisch anderen Menü am selben weiterhin gültigen Index neu aufbaut, kann ein reiner Index diese Identitätsänderung nicht beweisen. Dafür wären stabile Menüidentitäten oder Generationen nötig; das liegt außerhalb dieses Slices. Die View-Schicht soll eine solche Identitätsebene nicht implizit erfinden.

### Konsequenzen

Presenter besitzen nun eine einheitliche read-only Grenze sowohl für den Top-Level-Menüleistenzustand als auch für verschachtelte Popup-Zustände. Sie können bei offensichtlich veralteten Indizes fail-closed reagieren, während der `MenuInteractionController` weiterhin ausschließlich Wertzustand hält und Mutation nur in expliziten Interaktionstransaktionen erfolgt.

Es werden keine Backendtypen, nativen Menühandles, Rendering-Geometrien oder Fokusobjekte eingeführt. Der Helper bleibt bewusst klein und darf pro Frame erneut per Index nachschlagen; Korrektheit und klare Lifetime-Regeln haben weiterhin Vorrang vor dem Vermeiden trivialer Menümodell-Lookups, solange Profiling keinen Optimierungsbedarf zeigt.
