# ADR 0116 – Deterministic terminal safe-triangle sibling deferral

**Status:** Accepted  
**Date:** 2026-10-06

## English

### Context

ADR 0115 introduced `TerminalMenuPointerIntent`, a geometry-only classifier for the classic menu-aim/safe-triangle problem. It can recognize that the pointer is still travelling from the row that owns an open child submenu toward that child even when the physical path temporarily crosses a neighbouring row in the parent popup.

The classifier is intentionally advisory. It does not suppress the sibling row, wait, or mutate menu state. Without a timing consumer, however, the ordinary immediate pointer policy still selects that crossed sibling immediately and Core correctly closes the old child.

The next step therefore needs to add a bounded deferral while preserving the architectural boundaries already established by the terminal menu work:

- terminal geometry stays outside Core;
- `MenuInteractionController` remains the sole owner of semantic selection and `MenuPath` repair;
- no hidden thread or OS timer is introduced;
- stale numeric popup identities must not be replayed in a different semantic scope;
- all-motion traffic must not extend the delay forever.

### Decision

Add `terminal::TerminalMenuPointerDeferralInteraction` with:

```cpp
struct TerminalMenuPointerDeferralOptions {
    std::chrono::milliseconds submenu_switch_delay{300};
};

enum class TerminalMenuPointerDeferralDecision {
    process_now,
    defer,
};
```

The class receives the advisory `TerminalMenuPointerIntentKind` produced for the same pre-interaction pointer sample.

Only one narrow event class is eligible for deferral:

1. the event is `PointerAction::move`;
2. pointer intent is `toward_open_submenu`;
3. a child popup is still open;
4. semantic popup depth and frame popup depth agree;
5. the physical hit is a row in the popup that owns the deepest child;
6. that hit is a different row from the one that currently owns the child.

Everything else is processed immediately and clears an older deferred candidate.

### Host order

The intended order is:

```text
Pointer event + current menu frame
        ↓
TerminalMenuPointerIntent::observe(...)
        ↓
TerminalMenuPointerDeferralInteraction::observe(..., intent, now)
        ↓
process_now                         defer
     ↓                                ↓
normal TerminalMenuPointerInteraction  keep Core unchanged
                                       ↓
                                  host loop advances
                                       ↓
                  TerminalMenuPointerDeferralInteraction::advance(..., now)
                                       ↓
                         optional copied PointerEvent
                                       ↓
                         normal pointer interaction
```

The deferral object does not call `TerminalMenuPointerInteraction` itself. At expiration it only returns the retained value event. The host replays that event through the normal pointer adapter against the current presentation frame.

This separation is deliberate: eventually selecting the sibling must still go through `MenuInteractionController::selectPopupItem()`, because that existing Core transaction owns descendant truncation, selectability checks, and stale-model repair.

### Gap and child motion are never delayed

`toward_open_submenu` can also describe a point in the geometric gap between parent and child. Such a point is not dangerous: ordinary pointer interaction does not replace the parent selection there. Gap lifetime is already handled by `TerminalMenuCloseInteraction`.

Likewise, motion that reaches the child popup has completed the transfer and must run immediately.

The deferral policy therefore requires a real **sibling row hit at the owning parent level**. This avoids adding latency where no sibling-selection threat exists.

### First deadline, latest event

A terminal in all-motion mode can emit many samples while the pointer crosses the safe triangle. Two pieces of state have different update rules:

```text
first-observed time  → keep
latest deferred event → replace
```

Keeping the first time guarantees a hard upper bound on suppression. Replacing the event means that if the pointer crosses parent row 1 and then row 2 before the timeout, expiration replays row 2 rather than an obsolete earlier row.

The delay therefore cannot be prolonged indefinitely by motion traffic.

### Deferred state is structural value state

A pending transaction stores:

```text
selected top-level menu index
complete currently-open MenuPath
owning parent {level,item_index}
first-observed TimePoint
latest copied PointerEvent
```

It stores no `MenuModel*`, `MenuItem*`, Widget pointer, frame pointer, native handle, callback, thread, or timer object.

This matters because `{level,item_index}` and even a complete `MenuPath` can have the same numeric shape in another top-level menu. The top-level index and full path are both part of the retained scope.

### Revalidation before replay

`advance()` does not trust the retained event merely because its deadline expired. It rechecks current state and the current frame:

- menu interaction is still active;
- a child popup is still open;
- root selection is unchanged;
- the complete `MenuPath` is unchanged;
- path length still agrees with popup depth;
- frame popup count still agrees with semantic depth;
- the owning parent item still exists;
- the retained position still hit-tests as a different row at that parent level.

If any proof fails, the event is discarded.

The final hit-test against the current frame provides an additional fail-closed guard for presentation changes. Hosts should still reset the object explicitly on resize/mode changes, but an accidentally retained event is not blindly replayed against shifted geometry.

### Expiration returns an event; it does not mutate Core

At the deadline the pending state is retired first and a copied `PointerEvent` is returned.

The host then uses its ordinary path:

```cpp
TerminalMenuPointerInteraction::handle(
    menu_bar,
    menu_interaction,
    current_frame,
    expired_event,
    ...);
```

This means all existing semantics stay authoritative. For example, when the replayed event selects a sibling command row, Core recognizes that the old open child no longer belongs to the selected parent and truncates the path immediately.

No duplicate descendant-close algorithm is added to the deferral layer.

### Tests

Deterministic tests verify that:

- a sibling row inside the safe triangle is held until the configured deadline while Core remains unchanged;
- expiration returns a value event that normal pointer interaction can replay to select the sibling and close the old child;
- repeated intent-qualified sibling samples preserve the first deadline but update the retained event to the latest row;
- reaching the child before the deadline cancels the sibling candidate;
- a semantic popup-scope change invalidates the pending event;
- negative delay configuration is rejected.

The tests use explicit `steady_clock::time_point` values and never sleep.

### Consequences

- safe-triangle geometry now has a bounded timing consumer;
- sibling-row replacement can be postponed without moving terminal coordinates into Core;
- eventual selection still follows the existing pointer/controller transaction;
- timing remains deterministic and host-owned;
- all-motion traffic cannot create unbounded suppression;
- stale semantic or presentation scope fails closed;
- demo integration remains a separate slice so host ordering/reset boundaries can be reviewed independently.

### Deferred scope

This ADR does not yet add:

- wiring the deferral policy into `terminal_form_demo`;
- adaptive delay based on pointer velocity;
- different delays by submenu depth;
- coupling the safe-triangle deadline to close-grace deadlines;
- rendered/native menu-aim policies.

---

## Deutsch

### Kontext

ADR 0115 hat mit `TerminalMenuPointerIntent` den rein geometrischen Classifier für das klassische Menu-Aim-/Safe-Triangle-Problem eingeführt. Er kann erkennen, dass sich der Pointer weiterhin von der Zeile, die ein geöffnetes Child-Submenu besitzt, in Richtung dieses Childs bewegt, obwohl der physische Weg kurz eine benachbarte Zeile im Parent-Popup kreuzt.

Der Classifier ist bewusst nur beratend. Er unterdrückt die Sibling-Zeile nicht, wartet nicht und verändert keinen Menüzustand. Ohne eine Timing-Policy würde die normale unmittelbare Pointer-Interaktion die gekreuzte Sibling-Zeile weiterhin sofort auswählen; Core würde daraufhin korrekt das bisherige Child schließen.

Der nächste Schritt muss deshalb eine begrenzte Verzögerung ergänzen, ohne die bisher aufgebauten Architekturgrenzen zu verletzen:

- Terminal-Geometrie bleibt außerhalb von Core;
- `MenuInteractionController` bleibt alleiniger Besitzer von Selection- und `MenuPath`-Semantik;
- es gibt keinen versteckten Thread oder OS-Timer;
- alte numerische Popup-Identitäten dürfen nicht in einem anderen semantischen Scope wiederverwendet werden;
- All-Motion-Traffic darf die Verzögerung nicht unbegrenzt verlängern.

### Entscheidung

Wir ergänzen `terminal::TerminalMenuPointerDeferralInteraction` mit:

```cpp
struct TerminalMenuPointerDeferralOptions {
    std::chrono::milliseconds submenu_switch_delay{300};
};

enum class TerminalMenuPointerDeferralDecision {
    process_now,
    defer,
};
```

Die Klasse erhält für dasselbe Pre-Interaction-Pointer-Sample das beratende `TerminalMenuPointerIntentKind`.

Nur ein eng definierter Fall darf verzögert werden:

1. das Event ist `PointerAction::move`;
2. Pointer Intent lautet `toward_open_submenu`;
3. ein Child-Popup ist weiterhin geöffnet;
4. semantische Popup-Tiefe und Frame-Popup-Tiefe stimmen überein;
5. der physische Hit liegt in dem Popup, das das tiefste Child besitzt;
6. der Hit ist eine andere Zeile als diejenige, die aktuell das Child besitzt.

Alles andere wird sofort verarbeitet und löscht einen älteren Deferred-Kandidaten.

### Host-Reihenfolge

Die vorgesehene Reihenfolge lautet:

```text
Pointer-Event + aktueller Menü-Frame
        ↓
TerminalMenuPointerIntent::observe(...)
        ↓
TerminalMenuPointerDeferralInteraction::observe(..., intent, now)
        ↓
process_now                         defer
     ↓                                ↓
normale TerminalMenuPointerInteraction  Core unverändert lassen
                                       ↓
                                  Host-Loop läuft weiter
                                       ↓
                  TerminalMenuPointerDeferralInteraction::advance(..., now)
                                       ↓
                         optionales kopiertes PointerEvent
                                       ↓
                         normale Pointer-Interaktion
```

Das Deferral-Objekt ruft `TerminalMenuPointerInteraction` nicht selbst auf. Bei Ablauf liefert es ausschließlich das gespeicherte Value-Event zurück. Der Host spielt dieses Event anschließend über den normalen Pointer-Adapter gegen den aktuellen Presentation-Frame ein.

Diese Trennung ist Absicht: Die spätere Auswahl des Siblings soll weiterhin über `MenuInteractionController::selectPopupItem()` laufen, weil diese bestehende Core-Transaktion Descendant-Truncation, Selectability-Prüfungen und Stale-Model-Reparatur besitzt.

### Gap- und Child-Motion werden niemals verzögert

`toward_open_submenu` kann auch einen Punkt in der geometrischen Lücke zwischen Parent und Child beschreiben. Dieser Punkt ist nicht gefährlich: Die normale Pointer-Interaktion ersetzt dort die Parent-Selection nicht. Die Lebensdauer dieser Lücke wird bereits von `TerminalMenuCloseInteraction` behandelt.

Ebenso hat eine Bewegung, die das Child-Popup erreicht, den Transfer bereits erfolgreich beendet und muss sofort verarbeitet werden.

Die Deferral-Policy verlangt deshalb einen echten **Sibling-Zeilen-Hit auf der owning Parent-Ebene**. So entsteht keine zusätzliche Latenz, wenn gar keine Sibling-Selection droht.

### Erste Deadline, jüngstes Event

Ein Terminal im All-Motion-Modus kann beim Durchqueren des Safe Triangle viele Samples liefern. Zwei Zustandsanteile haben bewusst unterschiedliche Aktualisierungsregeln:

```text
erster beobachteter Zeitpunkt → behalten
jüngstes verzögertes Event    → ersetzen
```

Der erste Zeitpunkt garantiert eine harte Obergrenze für die Unterdrückung. Das jüngste Event sorgt dafür, dass beim Weg über Parent-Zeile 1 und danach Zeile 2 nach Ablauf Zeile 2 und nicht die inzwischen veraltete frühere Zeile verwendet wird.

Die Verzögerung kann damit nicht durch Motion-Traffic unbegrenzt verlängert werden.

### Deferred-State ist struktureller Value-State

Eine Pending-Transaktion speichert:

```text
ausgewählten Top-Level-Menüindex
vollständigen aktuell offenen MenuPath
owning Parent {level,item_index}
ersten beobachteten TimePoint
jüngstes kopiertes PointerEvent
```

Nicht gespeichert werden `MenuModel*`, `MenuItem*`, Widget-, Frame-, Native-Handle-, Callback-, Thread- oder Timer-Zeiger/Objekte.

Das ist notwendig, weil `{level,item_index}` und sogar ein vollständiger `MenuPath` in einem anderen Top-Level-Menü numerisch identisch aussehen können. Deshalb gehören Top-Level-Index und vollständiger Pfad gemeinsam zum Scope.

### Erneuter Beweis vor Replay

`advance()` vertraut dem gespeicherten Event nicht nur deshalb, weil die Zeit abgelaufen ist. Aktueller Zustand und aktueller Frame werden erneut geprüft:

- Menüinteraktion ist weiterhin aktiv;
- ein Child-Popup ist weiterhin geöffnet;
- Root-Selection ist unverändert;
- der vollständige `MenuPath` ist unverändert;
- Pfadlänge und Popup-Tiefe stimmen weiterhin überein;
- Frame-Popup-Zahl und semantische Tiefe stimmen weiterhin überein;
- das owning Parent-Item existiert weiterhin;
- die gespeicherte Position trifft im aktuellen Frame weiterhin eine andere Zeile auf dieser Parent-Ebene.

Scheitert einer dieser Beweise, wird das Event verworfen.

Der abschließende Hit-Test gegen den aktuellen Frame bietet zusätzlich Fail-Closed-Schutz bei Presentation-Änderungen. Hosts sollen das Objekt bei Resize-/Mode-Grenzen weiterhin explizit zurücksetzen; ein versehentlich verbliebenes Event wird aber nicht blind gegen verschobene Geometrie abgespielt.

### Ablauf liefert ein Event und mutiert Core nicht

Beim Erreichen der Deadline wird der Pending-State zuerst gelöscht und anschließend ein kopiertes `PointerEvent` zurückgegeben.

Der Host verwendet danach wieder seinen normalen Pfad:

```cpp
TerminalMenuPointerInteraction::handle(
    menu_bar,
    menu_interaction,
    current_frame,
    expired_event,
    ...);
```

Damit bleiben alle vorhandenen Semantiken maßgeblich. Wählt das wieder eingespielte Event beispielsweise eine Sibling-Command-Zeile, erkennt Core, dass das bisher offene Child nicht mehr zu diesem Parent gehört, und kürzt den Pfad sofort.

Im Deferral-Layer entsteht kein zweiter Algorithmus zum Schließen von Descendants.

### Tests

Deterministische Tests prüfen:

- eine Sibling-Zeile innerhalb des Safe Triangle wird bis zur konfigurierten Deadline zurückgehalten, während Core unverändert bleibt;
- nach Ablauf wird ein Value-Event zurückgegeben, das die normale Pointer-Interaktion zur Sibling-Selection und zum Schließen des alten Childs verwenden kann;
- wiederholte intent-qualifizierte Sibling-Samples behalten die erste Deadline, aktualisieren aber das gespeicherte Event auf die jüngste Zeile;
- das Erreichen des Childs vor Ablauf löscht den Sibling-Kandidaten;
- eine semantische Änderung des Popup-Scopes invalidiert das Pending-Event;
- negative Delay-Konfiguration wird abgelehnt.

Die Tests verwenden explizite `steady_clock::time_point`-Werte und schlafen nie.

### Konsequenzen

- die Safe-Triangle-Geometrie besitzt jetzt einen begrenzten Timing-Verbraucher;
- Sibling-Row-Replacement kann verzögert werden, ohne Terminal-Koordinaten nach Core zu verschieben;
- die spätere Auswahl läuft weiterhin über die vorhandene Pointer-/Controller-Transaktion;
- Timing bleibt deterministisch und host-eigen;
- All-Motion-Traffic kann keine unbegrenzte Unterdrückung erzeugen;
- veralteter semantischer oder Presentation-Scope scheitert fail-closed;
- Demo-Integration bleibt ein eigener Slice, damit Host-Reihenfolge und Reset-Grenzen separat geprüft werden können.

### Aufgeschobener Umfang

Diese ADR ergänzt noch nicht:

- die Einbindung der Deferral-Policy in `terminal_form_demo`;
- adaptive Verzögerung abhängig von Pointer-Geschwindigkeit;
- unterschiedliche Delays je Submenu-Tiefe;
- Kopplung der Safe-Triangle-Deadline an Close-Grace-Deadlines;
- Rendered-/Native-Menu-Aim-Policies.
