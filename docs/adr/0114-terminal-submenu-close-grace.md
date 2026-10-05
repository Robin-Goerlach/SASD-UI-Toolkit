# ADR 0114 – Delayed terminal submenu close grace for popup transfer gaps

**Status:** Accepted  
**Date:** 2026-10-05

## English

### Context

ADR 0113 added deterministic delayed submenu opening. The terminal demo can now keep a pointer over a submenu row and open its child after a short delay. Once a child is open, another interaction problem appears: the pointer may have to cross terminal cells that belong to no popup row while travelling from the parent popup into the child.

Closing the child as soon as the pointer enters such a geometric gap would make the transfer fragile. Keeping every child open indefinitely after the pointer leaves all popup surfaces would be equally undesirable.

The existing immediate pointer policy already handles a different case correctly: if the pointer lands on another selectable ancestor row, `MenuInteractionController::selectPopupItem()` closes descendants immediately because they no longer belong to the selected parent. The new policy must not delay or weaken that semantic invariant.

### Decision

Add `terminal::TerminalMenuCloseInteraction`, a host-owned deterministic grace-period object with one option:

```cpp
struct TerminalMenuCloseOptions {
    std::chrono::milliseconds submenu_close_delay{250};
};
```

The intended host order is:

```text
PointerAction::move
        ↓
TerminalMenuPointerInteraction::handle(...)
        ↓ immediate semantic selection/path repair
TerminalMenuCloseInteraction::observe(..., now)
        ↓ arm only when child popups remain open
          and the pointer is outside every popup row
host loop continues
        ↓
TerminalMenuCloseInteraction::advance(..., now)
        ↓ after grace period
close descendants back to the root popup
```

The class owns no thread, OS timer, callback, terminal device, Widget pointer, `MenuModel*`, `MenuItem*`, or presentation-frame pointer. The host supplies `std::chrono::steady_clock::time_point` values explicitly.

### The grace period is only for geometric gaps

The close policy is not a replacement for normal popup selection.

If motion selects another ancestor row, Core may immediately truncate descendants because that row no longer owns the child. `TerminalMenuCloseInteraction` observes state only after that transaction. If the popup depth has already returned to one, there is no close candidate to arm.

A candidate is armed only when all of the following remain true after immediate pointer processing:

- menu interaction is active;
- a root popup is open;
- at least one child popup is still open;
- the event is `PointerAction::move`;
- the pointer does not hit any popup row in the supplied presentation frame.

This gives a narrow meaning to the timer: preserve a valid child chain briefly while the pointer crosses non-popup geometry.

### Returning to any popup cancels closing

If the pointer reaches any visible popup row before the deadline, the candidate is cleared immediately. The timing object does not need to know whether that row belongs to the parent, child, or a deeper popup; the immediate interaction/controller layers already own those semantics.

This is sufficient for a basic parent-to-child transfer:

```text
parent submenu row
      ↓
short non-popup gap       ← grace candidate armed
      ↓
child popup row           ← candidate cancelled
```

No safe-triangle or pointer-trajectory heuristic is introduced yet.

### Repeated outside motion does not restart the deadline

All-motion terminals may emit many motion reports while the pointer travels through the same gap. Restarting the timer for every packet could keep descendants open indefinitely.

A candidate therefore stores the first outside timestamp and keeps it while semantic scope is unchanged.

### Candidate identity is structural value state

The candidate stores:

```text
selected top-level menu index
complete currently-open MenuPath
first-observed TimePoint
```

A popup depth alone is not sufficient. Two unrelated submenu routes can both have depth two. Requiring the complete path prevents an old timer from collapsing a sibling submenu that opened after the candidate was armed.

No borrowed semantic or presentation pointers survive between calls.

### Scope is revalidated at the deadline

Before closing anything, `advance()` verifies that:

- menu interaction is still active;
- child popups are still open;
- the selected top-level index is unchanged;
- the complete open `MenuPath` is exactly the path that armed the candidate;
- popup depth still agrees with path length.

If any proof fails, the candidate is discarded with no semantic mutation.

### Existing Core Left semantics close descendants

This slice deliberately does not add a new public `MenuInteractionController::closePopupDescendants()` API.

The controller already has a canonical backend-neutral transition for leaving a nested popup: an unmodified pressed `Key::left` closes exactly one child level. `TerminalMenuCloseInteraction::advance()` reuses that transition while `popupDepth() > 1` and stops before the root popup, where Left would instead switch top-level menus.

Conceptually:

```text
Depth 3
  ↓ Left
Depth 2
  ↓ Left
Depth 1 (root only)
  ↓ stop
```

A defensive progress guard stops if a future controller change no longer reduces depth. This prevents an accidental host-loop spin.

The choice mirrors ADR 0112, where terminal top-level pointer switching reuses an established controller transition rather than expanding Core speculatively. If another backend later needs direct descendant-collapse semantics, a dedicated Core transaction can be promoted with multiple concrete consumers.

### Press/release and inactive scope reset timing

Non-motion pointer events clear the close candidate. A click therefore cannot inherit a previous gap timer.

Likewise, inactive menu state, root-only popup state, or an inconsistent path/depth relation clears the candidate immediately.

Hosts should also reset this object when another input policy takes ownership, for example keyboard navigation or a resize that invalidates presentation geometry. That integration remains a separate host-wiring slice, just as ADR 0113 separated hover-policy definition from demo integration.

### Tests

Regression coverage verifies:

- leaving a three-level popup chain closes no descendants before the deadline;
- the same outside all-motion reports do not restart the deadline;
- at the deadline all descendants close while the root popup and its selected owning submenu remain active;
- entering any popup before the deadline cancels closing;
- a stale candidate cannot collapse a sibling submenu that reuses the same popup depth;
- root-only menus do not arm close timing;
- negative close delays are rejected.

### Consequences

- parent-to-child popup transfer can tolerate short non-popup gaps without hidden threads or timers;
- normal ancestor-row selection continues to close invalid descendants immediately;
- structural menu invariants stay owned by `MenuInteractionController`;
- close timing is deterministic and host-owned;
- no new Core public mutation API is introduced prematurely;
- safe-triangle/pointer-intent heuristics remain a later refinement rather than being mixed into the first close policy.

### Deferred scope

This decision does not yet add:

- wiring `TerminalMenuCloseInteraction` into `terminal_form_demo`;
- diagonal safe-triangle/pointer-intent heuristics;
- delayed switching between sibling ancestor rows;
- different close delays by popup depth;
- rendered/native menu close adapters.

---

## Deutsch

### Kontext

ADR 0113 hat das deterministische verzögerte Öffnen von Submenus eingeführt. Das Terminal-Demo kann inzwischen einen Pointer über einer Submenu-Zeile verweilen lassen und nach kurzer Verzögerung das Child öffnen. Sobald ein Child geöffnet ist, entsteht ein weiteres Interaktionsproblem: Auf dem Weg vom Parent-Popup in das Child kann der Pointer Terminalzellen überqueren, die zu keiner Popup-Zeile gehören.

Würde das Child beim ersten solchen Gap sofort geschlossen, wäre der Übergang unnötig empfindlich. Würden dagegen alle Children nach Verlassen der Popup-Flächen unbegrenzt offen bleiben, wäre das ebenfalls unerwünscht.

Die vorhandene unmittelbare Pointer-Policy behandelt einen anderen Fall bereits korrekt: Landet der Pointer auf einer anderen auswählbaren Ancestor-Zeile, schließt `MenuInteractionController::selectPopupItem()` Descendants sofort, weil sie nicht mehr zum ausgewählten Parent gehören. Die neue Policy darf diese semantische Invariante weder verzögern noch abschwächen.

### Entscheidung

Wir ergänzen `terminal::TerminalMenuCloseInteraction`, ein host-eigenes deterministisches Grace-Period-Objekt mit einer Option:

```cpp
struct TerminalMenuCloseOptions {
    std::chrono::milliseconds submenu_close_delay{250};
};
```

Die vorgesehene Host-Reihenfolge lautet:

```text
PointerAction::move
        ↓
TerminalMenuPointerInteraction::handle(...)
        ↓ unmittelbare semantische Selection/Path-Reparatur
TerminalMenuCloseInteraction::observe(..., now)
        ↓ nur arming, wenn Children weiterhin offen sind
          und der Pointer keine Popup-Zeile trifft
Host-Loop läuft weiter
        ↓
TerminalMenuCloseInteraction::advance(..., now)
        ↓ nach Ablauf der Grace Period
Descendants bis auf das Root-Popup schließen
```

Die Klasse besitzt keinen Thread, OS-Timer, Callback, Terminal-Device-, Widget-, `MenuModel*`-, `MenuItem*`- oder Presentation-Frame-Zeiger. Der Host übergibt `std::chrono::steady_clock::time_point` explizit.

### Die Grace Period gilt nur für geometrische Lücken

Die Close-Policy ersetzt nicht die normale Popup-Selection.

Wählt Motion eine andere Ancestor-Zeile aus, kann Core Descendants sofort truncaten, weil diese Zeile das Child nicht mehr besitzt. `TerminalMenuCloseInteraction` beobachtet den Zustand erst nach dieser Transaktion. Ist die Popup-Tiefe bereits wieder eins, wird kein Close-Kandidat armed.

Ein Kandidat entsteht nur, wenn nach der unmittelbaren Pointer-Verarbeitung weiterhin alle folgenden Bedingungen gelten:

- Menüinteraktion ist aktiv;
- ein Root-Popup ist geöffnet;
- mindestens ein Child-Popup ist noch geöffnet;
- das Event ist `PointerAction::move`;
- der Pointer trifft im gelieferten Presentation-Frame keine Popup-Zeile.

Der Timer besitzt damit eine enge Bedeutung: Eine gültige Child-Kette bleibt kurz erhalten, während der Pointer Nicht-Popup-Geometrie überquert.

### Rückkehr in ein beliebiges Popup bricht das Schließen ab

Erreicht der Pointer vor Ablauf der Frist irgendeine sichtbare Popup-Zeile, wird der Kandidat sofort gelöscht. Das Timing-Objekt muss nicht unterscheiden, ob die Zeile zu Parent, Child oder einem tieferen Popup gehört; diese Semantik besitzen bereits Immediate-Interaction und Controller.

Damit funktioniert der grundlegende Parent-zu-Child-Transfer:

```text
Parent-Submenu-Zeile
      ↓
kurzes Nicht-Popup-Gap      ← Grace-Kandidat armed
      ↓
Child-Popup-Zeile           ← Kandidat gelöscht
```

Eine Safe-Triangle- oder Pointer-Trajektorien-Heuristik kommt noch nicht hinzu.

### Wiederholte Outside-Motion startet die Frist nicht neu

All-Motion-Terminals können während des Weges durch dasselbe Gap viele Reports senden. Würde jedes Paket den Timer neu starten, könnten Descendants unbegrenzt offen bleiben.

Ein Kandidat speichert deshalb den ersten Outside-Zeitpunkt und behält ihn, solange der semantische Scope gleich bleibt.

### Kandidatenidentität ist struktureller Value-State

Der Kandidat speichert:

```text
ausgewählten Top-Level-Menüindex
vollständigen aktuell offenen MenuPath
ersten beobachteten TimePoint
```

Nur die Popup-Tiefe reicht nicht. Zwei unabhängige Submenu-Routen können beide Tiefe zwei besitzen. Der vollständige Pfad verhindert, dass ein alter Timer ein später geöffnetes Sibling-Submenu mit gleicher Tiefe schließt.

Zwischen Aufrufen bleiben keine geliehenen Semantic- oder Presentation-Zeiger erhalten.

### Scope wird bei Ablauf erneut geprüft

Bevor etwas geschlossen wird, beweist `advance()` erneut:

- Menüinteraktion ist weiterhin aktiv;
- Child-Popups sind weiterhin offen;
- der Top-Level-Index ist unverändert;
- der vollständige offene `MenuPath` ist exakt der Pfad, der den Kandidaten erzeugt hat;
- Popup-Tiefe und Pfadlänge stimmen weiterhin überein.

Scheitert einer dieser Beweise, wird der Kandidat ohne semantische Änderung verworfen.

### Vorhandene Core-Left-Semantik schließt Descendants

Dieser Slice führt bewusst keine neue öffentliche `MenuInteractionController::closePopupDescendants()`-API ein.

Der Controller besitzt bereits eine kanonische backend-neutrale Transition zum Verlassen eines verschachtelten Popups: Ein unverändertes gedrücktes `Key::left` schließt genau eine Child-Ebene. `TerminalMenuCloseInteraction::advance()` verwendet diese Transition wiederholt, solange `popupDepth() > 1` gilt, und stoppt vor dem Root-Popup, wo Left stattdessen das Top-Level-Menü wechseln würde.

Konzeptionell:

```text
Tiefe 3
  ↓ Left
Tiefe 2
  ↓ Left
Tiefe 1 (nur Root)
  ↓ Stop
```

Ein defensiver Progress-Guard beendet die Schleife, falls eine spätere Controller-Änderung die Tiefe nicht mehr reduziert. Dadurch kann keine Endlosschleife im Host entstehen.

Diese Entscheidung entspricht ADR 0112: Dort nutzt der Terminal-Top-Level-Pointer-Wechsel ebenfalls eine vorhandene Controller-Transition, statt Core vorsorglich zu verbreitern. Wenn später weitere Backends direktes Descendant-Collapse benötigen, kann daraus mit mehreren konkreten Verbrauchern eine eigene Core-Transaktion werden.

### Press/Release und inaktiver Scope setzen Timing zurück

Nicht-Motion-Pointer-Events löschen den Close-Kandidaten. Ein Klick kann daher keinen früheren Gap-Timer übernehmen.

Ebenso löschen inaktiver Menüzustand, Root-only-Popup-Zustand oder ein inkonsistentes Verhältnis von Pfad und Tiefe den Kandidaten sofort.

Hosts sollten das Objekt außerdem zurücksetzen, wenn eine andere Input-Policy übernimmt, etwa Tastaturnavigation oder ein Resize, das Presentation-Geometrie invalidiert. Diese Host-Integration bleibt ein eigener Wiring-Slice, genauso wie ADR 0113 Policy-Definition und Demo-Integration getrennt hat.

### Tests

Regressionstests prüfen:

- das Verlassen einer dreistufigen Popup-Kette schließt vor Ablauf der Frist keine Descendants;
- wiederholte Outside-All-Motion-Reports starten die Frist nicht neu;
- beim Ablauf werden alle Descendants geschlossen, während Root-Popup und dessen ausgewähltes owning Submenu aktiv bleiben;
- das Erreichen eines beliebigen Popups vor Ablauf bricht das Schließen ab;
- ein veralteter Kandidat kann kein Sibling-Submenu schließen, das dieselbe Popup-Tiefe wiederverwendet;
- Root-only-Menüs erzeugen keinen Close-Kandidaten;
- negative Close-Delays werden abgelehnt.

### Konsequenzen

- Parent-zu-Child-Popup-Transfer toleriert kurze Nicht-Popup-Lücken ohne versteckte Threads oder Timer;
- normale Ancestor-Zeilen-Selection schließt ungültige Descendants weiterhin sofort;
- strukturelle Menüinvarianten bleiben Eigentum des `MenuInteractionController`;
- Close-Timing bleibt deterministisch und host-eigen;
- keine neue öffentliche Core-Mutations-API wird vorschnell eingeführt;
- Safe-Triangle-/Pointer-Intent-Heuristiken bleiben eine spätere Verfeinerung und werden nicht mit der ersten Close-Policy vermischt.

### Aufgeschobener Umfang

Diese Entscheidung ergänzt noch nicht:

- die Einbindung von `TerminalMenuCloseInteraction` in `terminal_form_demo`;
- diagonale Safe-Triangle-/Pointer-Intent-Heuristiken;
- verzögerten Wechsel zwischen Sibling-Ancestor-Zeilen;
- unterschiedliche Close-Delays je Popup-Tiefe;
- Rendered-/Native-Menü-Close-Adapter.
