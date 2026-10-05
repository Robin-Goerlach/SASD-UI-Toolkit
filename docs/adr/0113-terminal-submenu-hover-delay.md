# ADR 0113 – Deterministic delayed submenu hover policy for terminal menus

**Status:** Accepted  
**Date:** 2026-10-05

## English

### Context

ADR 0110 defined immediate popup-row selection for terminal pointer motion, ADR 0111 added opt-in all-motion reporting, ADR 0112 added top-level menu switching while an active menu is open, and the terminal demo now requests all-motion tracking.

The remaining desktop-style interaction gap is automatic submenu opening while the pointer rests on a submenu row. Opening immediately on every `PointerAction::move` would be too aggressive: users frequently cross submenu rows while travelling to another target, and instant child creation would make the menu surface visually unstable.

The solution needs a small amount of time state, but that state must not leak terminal protocol concerns into Core or introduce background threads/timers into the toolkit architecture.

### Decision

Add `terminal::TerminalMenuHoverInteraction`, a host-owned deterministic timing object with one configurable option:

```cpp
struct TerminalMenuHoverOptions {
    std::chrono::milliseconds submenu_open_delay{300};
};
```

The object receives explicit monotonic `TimePoint` values from the host. It owns no thread and performs no sleeping. The event loop remains responsible for deciding when to call `advance()`.

The intended host sequence is:

```text
PointerAction::move
        ↓
TerminalMenuPointerInteraction::handle(...)
        ↓ immediate row selection / root-title switching
TerminalMenuHoverInteraction::observe(..., now)
        ↓ remember stable popup-row identity
host loop continues
        ↓
TerminalMenuHoverInteraction::advance(..., now)
        ↓ when delay expires
MenuInteractionController::openPopupSubmenu(...)
```

This keeps geometry, timing, and semantic menu mutation as separate concerns.

### Stable row motion does not restart the delay

All-motion terminals may emit many motion reports while the pointer moves horizontally within one popup row. Replacing the timestamp on every report could prevent a stable hover from ever reaching its deadline.

Therefore repeated motion over the same semantic popup identity preserves the original first-observed timestamp. Moving to a different row restarts the delay for the new identity.

### Leaving the popup cancels the candidate

Only `PointerAction::move` over a visible popup row can arm the hover state. Motion outside popup rows, pointer press/release, or an inactive menu resets the pending candidate.

This keeps click completion and delayed hover independent. A press/release gesture cannot accidentally inherit a previously armed hover timer.

### Candidate identity includes semantic scope

A popup row cannot be identified safely by `{level,item_index}` alone. The same numeric pair can occur after switching to another top-level menu or another sibling submenu.

A pending candidate therefore stores only value state, but enough value state to prove its scope:

```text
top-level menu index
owner MenuPath prefix
popup level + item index
first-observed TimePoint
```

No `MenuModel*`, `MenuItem*`, `Command*`, Widget pointer, presentation-frame pointer, or terminal-device pointer is retained.

Before a deadline is committed, `advance()` verifies that the selected root and owning path prefix still match. A root/path scope change cancels the candidate instead of reinterpreting its numeric row identity in another menu.

### Core remains the final semantic authority

Even after timing and scope checks succeed, the hover layer does not inspect `MenuItemKind` or open child state itself. It calls:

```cpp
controller.openPopupSubmenu(bar, level, item_index);
```

That existing backend-neutral transaction still proves that the item is currently selected, enabled, a live submenu, and structurally valid.

Consequently a delayed candidate over a command, separator, disabled row, or row whose selection changed simply produces no submenu transition.

### Candidate is retired before opening

At the deadline the pending identity is cleared before calling `openPopupSubmenu()`. This makes one hover deadline a one-shot transaction and prevents repeated opening attempts on every subsequent host-loop tick.

If the submenu was already open, Core returns a no-op while preserving descendants, matching the existing controller contract.

### No hidden background work

`TerminalMenuHoverInteraction` deliberately has no worker thread, callback, OS timer, asynchronous task, or dependency on `TerminalEventPump` timing.

Tests pass synthetic `steady_clock::time_point` values directly, so timing behavior is deterministic and does not rely on wall-clock sleeps. Production hosts can call `advance(Clock::now())` from their ordinary main loop.

### Tests

Regression coverage verifies:

- a stable submenu row opens only when its deadline is reached;
- repeated motion inside the same row does not restart the timer;
- moving to another row restarts the delay;
- moving outside popup rows cancels the candidate;
- switching top-level menus cannot reinterpret an equal `{level,item_index}` candidate;
- changing a nested owner path cannot reinterpret an equal child-row identity;
- negative delay configuration is rejected.

### Consequences

- terminal menus gain a deterministic foundation for delayed submenu hover;
- the timing state is host-owned and pointer-free;
- no additional Core menu API is required;
- Core remains responsible for final submenu validation/opening;
- terminal geometry remains in the terminal layer;
- tests require no sleep or background scheduling;
- actual demo-loop wiring remains a separate integration slice.

### Deferred scope

This decision does not yet add:

- wiring `TerminalMenuHoverInteraction` into `terminal_form_demo`;
- submenu close delays;
- diagonal pointer-intent/safe-triangle heuristics between parent and child popups;
- hover activation of Commands;
- rendered/native menu hover adapters.

---

## Deutsch

### Kontext

ADR 0110 hat die unmittelbare Auswahl von Popup-Zeilen durch Terminal-Pointer-Motion definiert, ADR 0111 opt-in All-Motion-Reporting ergänzt, ADR 0112 den Wechsel von Top-Level-Menüs bei bereits aktivem Menü hinzugefügt, und das Terminal-Demo fordert inzwischen All-Motion-Tracking an.

Als typische Desktop-Menüfunktion fehlt noch das automatische Öffnen eines Submenus, wenn der Pointer auf einer Submenu-Zeile verweilt. Ein sofortiges Öffnen bei jedem `PointerAction::move` wäre zu aggressiv: Beim Weg zu einem anderen Ziel werden Submenu-Zeilen häufig nur kurz überquert. Sofort aufklappende Children würden die Oberfläche unnötig unruhig machen.

Dafür ist ein kleiner zeitabhängiger Zustand nötig. Dieser Zustand darf jedoch weder Terminal-Protokolldetails in Core tragen noch Hintergrundthreads oder versteckte Timer in die Toolkit-Architektur einführen.

### Entscheidung

Wir ergänzen `terminal::TerminalMenuHoverInteraction`, ein host-eigenes deterministisches Timing-Objekt mit einer konfigurierbaren Option:

```cpp
struct TerminalMenuHoverOptions {
    std::chrono::milliseconds submenu_open_delay{300};
};
```

Der Host übergibt explizite monotone `TimePoint`-Werte. Das Objekt besitzt keinen Thread und schläft nicht selbst. Der Eventloop entscheidet weiterhin, wann `advance()` aufgerufen wird.

Die vorgesehene Host-Reihenfolge lautet:

```text
PointerAction::move
        ↓
TerminalMenuPointerInteraction::handle(...)
        ↓ sofortige Zeilenauswahl / Root-Titelwechsel
TerminalMenuHoverInteraction::observe(..., now)
        ↓ stabile Popup-Zeilenidentität merken
Host-Loop läuft weiter
        ↓
TerminalMenuHoverInteraction::advance(..., now)
        ↓ nach Ablauf der Verzögerung
MenuInteractionController::openPopupSubmenu(...)
```

Geometrie, Zeitpolicy und semantische Menüänderung bleiben damit getrennt.

### Bewegung innerhalb derselben Zeile startet die Zeit nicht neu

Ein Terminal mit All-Motion kann zahlreiche Motion-Reports liefern, während sich der Pointer horizontal innerhalb derselben Popup-Zeile bewegt. Würde jeder Report den Startzeitpunkt ersetzen, könnte ein eigentlich stabiler Hover seine Frist nie erreichen.

Deshalb behält wiederholte Bewegung über derselben semantischen Popup-Identität den ursprünglichen ersten Beobachtungszeitpunkt. Erst der Wechsel auf eine andere Zeile startet die Verzögerung für die neue Identität neu.

### Verlassen der Popup-Fläche bricht den Kandidaten ab

Nur `PointerAction::move` über einer sichtbaren Popup-Zeile kann Hover-Zustand armed machen. Bewegung außerhalb von Popup-Zeilen, Press/Release oder ein inaktives Menü setzen den Kandidaten zurück.

Damit bleiben Click-Abschluss und verzögerter Hover unabhängig voneinander. Eine Press-/Release-Geste kann keinen älteren Hover-Timer übernehmen.

### Die Kandidatenidentität enthält den semantischen Scope

Eine Popup-Zeile lässt sich nicht sicher nur über `{level,item_index}` identifizieren. Dasselbe Zahlenpaar kann nach dem Wechsel in ein anderes Top-Level-Menü oder ein anderes Sibling-Submenu erneut vorkommen.

Ein Kandidat speichert deshalb ausschließlich Value-State, aber genug davon, um seinen Scope beweisen zu können:

```text
Top-Level-Menüindex
Owner-MenuPath-Präfix
Popup-Level + Item-Index
erster Beobachtungs-TimePoint
```

Es werden keine `MenuModel*`, `MenuItem*`, `Command*`, Widget-Zeiger, Presentation-Frame-Zeiger oder Terminal-Device-Zeiger gespeichert.

Vor dem Commit einer abgelaufenen Frist prüft `advance()`, ob Root-Auswahl und Owner-Pfadpräfix noch übereinstimmen. Ein Root-/Pfadwechsel verwirft den Kandidaten, statt denselben numerischen Zeilenindex in einem anderen Menü neu zu interpretieren.

### Core bleibt letzte semantische Autorität

Auch nach erfolgreicher Timing- und Scope-Prüfung untersucht die Hover-Schicht weder `MenuItemKind` noch Child-Zustände selbst. Sie ruft ausschließlich auf:

```cpp
controller.openPopupSubmenu(bar, level, item_index);
```

Diese vorhandene backend-neutrale Transaktion beweist weiterhin, dass das Item aktuell ausgewählt, aktiviert, ein lebendes Submenu und strukturell gültig ist.

Ein verzögerter Kandidat über einem Command, Separator, deaktivierten Eintrag oder einer inzwischen nicht mehr ausgewählten Zeile führt daher einfach zu keiner Submenu-Transition.

### Kandidat wird vor dem Öffnen entfernt

Beim Erreichen der Frist wird die Pending-Identität gelöscht, bevor `openPopupSubmenu()` aufgerufen wird. Eine Hover-Frist ist damit eine einmalige Transaktion und wird nicht bei jedem folgenden Host-Loop-Tick erneut versucht.

Ist das Submenu bereits geöffnet, liefert Core einen No-op und erhält vorhandene Descendants entsprechend dem bestehenden Controller-Vertrag.

### Keine versteckte Hintergrundarbeit

`TerminalMenuHoverInteraction` besitzt bewusst keinen Worker-Thread, Callback, OS-Timer, asynchronen Task oder Abhängigkeit vom Timing des `TerminalEventPump`.

Tests übergeben synthetische `steady_clock::time_point`-Werte direkt. Dadurch ist das Timing deterministisch und benötigt keine echten Sleeps. Ein Produktions-Host kann `advance(Clock::now())` einfach aus seinem normalen Mainloop aufrufen.

### Tests

Regressionstests prüfen:

- eine stabile Submenu-Zeile öffnet erst exakt nach ihrer Frist;
- wiederholte Motion innerhalb derselben Zeile startet die Zeit nicht neu;
- der Wechsel auf eine andere Zeile startet die Verzögerung neu;
- Bewegung außerhalb von Popup-Zeilen verwirft den Kandidaten;
- ein Top-Level-Menüwechsel kann einen gleichen `{level,item_index}`-Kandidaten nicht reinterpretieren;
- ein Wechsel des verschachtelten Owner-Pfads kann dieselbe Child-Zeilenidentität nicht reinterpretieren;
- negative Delay-Konfiguration wird abgelehnt.

### Konsequenzen

- Terminal-Menüs erhalten eine deterministische Grundlage für verzögertes Submenu-Hover;
- der Timing-Zustand ist host-eigen und pointer-frei;
- keine zusätzliche Core-Menü-API ist nötig;
- Core bleibt für die endgültige Submenu-Prüfung/-Öffnung verantwortlich;
- Terminal-Geometrie bleibt in der Terminalschicht;
- Tests benötigen weder Sleep noch Background-Scheduling;
- die tatsächliche Demo-Loop-Integration bleibt ein eigener späterer Slice.

### Aufgeschobener Umfang

Diese Entscheidung ergänzt noch nicht:

- die Einbindung von `TerminalMenuHoverInteraction` in `terminal_form_demo`;
- verzögertes Schließen von Submenus;
- diagonale Pointer-Intent-/Safe-Triangle-Heuristiken zwischen Parent- und Child-Popup;
- Hover-Aktivierung von Commands;
- Rendered-/Native-Menü-Hover-Adapter.
