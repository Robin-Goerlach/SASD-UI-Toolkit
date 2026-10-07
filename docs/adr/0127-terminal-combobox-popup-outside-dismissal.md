# ADR 0127 – Terminal ComboBox Primary outside-press dismissal

**Status:** Accepted  
**Date:** 2026-10-07

## English

### Context

ADR 0126 added safe Primary press/release click-to-commit for popup rows, but an open Terminal ComboBox
still ignored outside presses except for modal consumption. The user could cancel with Escape/F4/Alt+Up
or focus loss, yet the familiar pointer behavior "click elsewhere closes the drop-down" was missing.

Outside dismissal must not become click-through. The visible popup is a transient modal overlay: the
physical press that dismisses it must be consumed rather than replayed to an underlying Button,
TextField or menu title. At the same time, outside dismissal does not require cross-event identity or
PointerRouter capture; the current validated popup snapshot is enough to prove that the press belongs to
no popup row.

### Decision

A **Primary press** while the ComboBox popup is open and the pointer is outside all painted popup rows
immediately cancels/dismisses the drop-down by calling `ComboBox::setDropDownOpen(false)`.

This policy applies to both pointer-interaction overloads:

- the stateless hover/modal-consumption overload;
- the stateful `GestureState` press/release overload.

Before dismissal, any armed popup-row gesture is reset. The outside press is returned as an engaged
interaction result with action `dismissed` and is never routed to underlying Widgets.

Dismissal uses the existing Core cancellation semantics:

- committed `selectedIndex()` is preserved;
- transient `previewIndex()` is cleared;
- `DropDownChanged(false)` is delivered by Core;
- no `SelectionChanged` callback is emitted merely because the popup closed.

`setDropDownOpen(false)` may synchronously execute application callbacks that release or destroy the
ComboBox. Therefore the pointer helper resets its gesture first and performs no ComboBox member access
after invoking the close operation.

Only **press** dismisses. An outside release after an armed row press merely cancels that click gesture
and leaves the popup open. This preserves the ADR 0126 requirement that a click completes only when press
and release identify the same row; release alone never acquires independent dismissal meaning.

Secondary/non-primary outside presses remain consumed but do not dismiss. This first policy keeps
context-button behavior neutral until the toolkit defines a broader secondary-pointer convention.

### Consequences

Positive:

- Terminal ComboBox now supports the expected pointer cancellation gesture;
- dismissal reuses Core's existing cancel semantics instead of duplicating selection state;
- the dismissing press cannot click through to an underlying Widget;
- stateless and stateful hosts receive the same outside-close behavior;
- armed row identity cannot survive dismissal;
- callback lifetime safety is preserved.

Trade-offs:

- the first click on another control dismisses the ComboBox but does not also activate that control;
- secondary-button outside presses do not dismiss;
- no PointerRouter capture is introduced yet.

### Deliberately deferred

- PointerRouter capture/captured release policy;
- pointer opening/toggling from the collapsed ComboBox surface;
- whether a future desktop-style host should optionally replay a dismissing press to a new target;
- scrolling/maximum visible rows;
- Rendered popup pointer interaction.

---

## Deutsch

### Kontext

ADR 0126 hat ein sicheres Primary-Press-/Release-Click-to-Commit für Popup-Rows eingeführt. Eine offene
Terminal-ComboBox konsumierte Outside-Press bisher jedoch nur modal. Der Benutzer konnte über
Escape/F4/Alt+Up oder Focus-Loss abbrechen, aber das vertraute Pointer-Verhalten "woanders klicken
schließt das Drop-down" fehlte noch.

Outside-Dismissal darf nicht zu Click-through werden. Das sichtbare Popup ist ein transientes modales
Overlay: Der physische Press, der es schließt, muss konsumiert werden und darf nicht erneut an einen
darunterliegenden Button, TextField oder Menütitel gehen. Gleichzeitig benötigt Outside-Dismissal keine
Cross-Event-Identität und noch kein `PointerRouter`-Capture; der aktuell validierte Popup-Snapshot
reicht aus, um zu beweisen, dass der Press keine Popup-Row trifft.

### Entscheidung

Ein **Primary Press** bei geöffnetem ComboBox-Popup und Pointer außerhalb aller gezeichneten Popup-Rows
cancelt/dismissed das Drop-down sofort über `ComboBox::setDropDownOpen(false)`.

Diese Policy gilt für beide Pointer-Interaction-Overloads:

- den stateless Hover-/Modal-Consumption-Overload;
- den stateful Press-/Release-Overload mit `GestureState`.

Vor dem Dismissal wird eine eventuell armed Popup-Row-Gesture gelöscht. Der Outside-Press wird als
konsumiertes Ergebnis mit Action `dismissed` zurückgegeben und niemals an darunterliegende Widgets
weitergereicht.

Das Dismissal verwendet die bestehende Core-Cancel-Semantik:

- committed `selectedIndex()` bleibt erhalten;
- transiente `previewIndex()` wird gelöscht;
- `DropDownChanged(false)` kommt aus dem Core;
- nur wegen des Schließens wird kein `SelectionChanged` gesendet.

`setDropDownOpen(false)` darf synchron Anwendungscallbacks ausführen, die die ComboBox freigeben oder
zerstören. Deshalb löscht der Pointer-Helfer zuerst seine Gesture und greift nach Aufruf der Close-
Operation nicht mehr auf ComboBox-Member zu.

Nur **Press** dismissed. Ein Outside-Release nach einem armed Row-Press verwirft lediglich die
Click-Gesture und lässt das Popup offen. Damit bleibt die ADR-0126-Regel erhalten: Ein Klick completed
nur, wenn Press und Release dieselbe Row identifizieren; Release bekommt keine unabhängige
Dismissal-Bedeutung.

Secondary-/Non-Primary-Outside-Presses werden weiterhin konsumiert, dismissen aber nicht. Diese erste
Policy bleibt bei Kontext-Buttons neutral, bis das Toolkit eine allgemeinere Secondary-Pointer-
Konvention definiert.

### Konsequenzen

Positiv:

- die Terminal-ComboBox besitzt jetzt das erwartete Pointer-Cancel-Verhalten;
- Dismissal nutzt die bestehende Core-Cancel-Semantik statt Selection-Zustand zu duplizieren;
- der dismissende Press kann nicht zu einem darunterliegenden Widget durchklicken;
- stateless und stateful Hosts erhalten dasselbe Outside-Close-Verhalten;
- armed Row-Identität kann Dismissal nicht überleben;
- Callback-Lifetime-Sicherheit bleibt erhalten.

Abwägungen:

- der erste Klick auf ein anderes Control schließt die ComboBox, aktiviert dieses Control aber noch
  nicht gleichzeitig;
- Secondary-Outside-Press dismissed nicht;
- `PointerRouter`-Capture wird noch nicht eingeführt.

### Bewusst vertagt

- `PointerRouter`-Capture/Captured-Release-Policy;
- Pointer-Öffnen/-Toggling über die collapsed ComboBox-Oberfläche;
- optionale spätere Replay-Policy eines dismissenden Presses an ein neues Target;
- Scrolling/maximal sichtbare Rows;
- Rendered-Popup-Pointer-Interaktion.
