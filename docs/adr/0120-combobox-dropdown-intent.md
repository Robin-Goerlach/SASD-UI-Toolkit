# ADR 0120 – Core ComboBox drop-down intent and focus lifetime

**Status:** Accepted  
**Date:** 2026-10-07

## English

### Context

ADR 0117 established committed ComboBox selection, ADR 0118 added the collapsed Terminal
presentation and ADR 0119 added the collapsed Rendered presentation. Both presentation paths already
reserve stable drop-indicator chrome, but Core still has no semantic state saying whether a future
popup is intended to be open.

Adding popup geometry immediately would mix several independent questions: focus lifetime, keyboard
ownership, preview versus committed selection, popup placement, clipping, outside-pointer capture and
backend presentation. The next useful contract is therefore only the smallest state transition that
all later popup work can share.

### Decision

`ComboBox` gains one backend-neutral boolean drop-down intent:

- `isDropDownOpen()` observes it;
- `setDropDownOpen(bool)` changes it;
- `setOnDropDownChanged()` optionally observes real transitions.

The state is **transient interaction state**, not a durable application property. Entering the open
state is accepted only while the ComboBox owns logical focus and is visible/enabled. Closing is always
allowed. This produces one strong invariant for later popup hosts: an open ComboBox is associated with
the control that currently owns keyboard focus.

Focus loss closes the drop-down synchronously from `FocusEvent{false}`. This fits the existing
`FocusManager` ordering: the manager publishes the unfocused state first, then delivers the event.
Consequently `setDropDownOpen(false)` must remain legal after `hasFocus()` has already become false.
Hiding or disabling a focused ComboBox already clears focus through `Widget`, so the same focus-loss
path also retires open state without adding ComboBox-specific hooks to `Widget::setVisible()` or
`Widget::setEnabled()`.

Opening/closing changes presentation only. It does not invalidate measurement and does not alter
committed selection. The callback runs only after state and invalidation are coherent; it is copied
before invocation and no ComboBox member is touched afterwards, preserving the toolkit's existing
callback lifetime rule.

The first keyboard ownership is deliberately narrow and does not depend on popup geometry:

- unmodified F4 toggles open/closed;
- exact Alt+Down opens;
- exact Alt+Up closes;
- matching key-up is consumed without a second mutation.

Existing unmodified Up/Down/Home/End behavior continues to change committed selection. Preview
selection is still undefined and is not simulated with temporary state in this slice.

Terminal and Rendered presentation make the new state observable without pretending the popup already
exists. The reserved right-side marker is `v` while closed and `^` while open. Both Terminal glyphs
are one cell. Rendered measurement reserves the maximum width/height required by both markers, so the
state flip cannot change intrinsic size even with a proportional font.

### Deliberately deferred

This ADR does **not** define:

- popup item rows or popup placement;
- highlighted/preview selection;
- Enter commit or Escape rollback;
- pointer opening, outside click or pointer capture;
- popup focus scopes;
- type-ahead search;
- native ComboBox peers.

Those rules should be introduced with concrete popup consumers and deterministic tests rather than
being inferred from a boolean open flag.

### Consequences

Positive:

- later Terminal/Rendered/native popup hosts share one small Core state contract;
- focus transfer, disable and hide have one deterministic cleanup route;
- open/closed visual feedback is immediately testable in both existing presentation backends;
- measurement remains stable across open/closed transitions;
- no backend geometry enters Core.

Trade-offs:

- the `^` marker currently communicates open intent before actual popup rows exist;
- arrow navigation while open still uses committed-selection semantics until the separate preview/
  commit contract is designed.

---

## Deutsch

### Kontext

ADR 0117 hat die committed Selection der ComboBox festgelegt, ADR 0118 die geschlossene
Terminaldarstellung und ADR 0119 die geschlossene Rendered-Darstellung ergänzt. Beide Presentation-
Pfade reservieren bereits stabiles Drop-Indikator-Chrome, im Core fehlt jedoch noch ein semantischer
Zustand, der ausdrückt, ob ein späteres Popup geöffnet sein soll.

Popup-Geometrie sofort einzubauen würde mehrere unabhängige Fragen vermischen: Focus-Lifetime,
Keyboard-Ownership, Preview gegenüber committed Selection, Popup-Platzierung, Clipping,
Outside-Pointer-Capture und backend-spezifische Darstellung. Der nächste sinnvolle Vertrag ist daher
nur der kleinste gemeinsame Zustandsübergang, auf dem spätere Popup-Arbeit aufbauen kann.

### Entscheidung

`ComboBox` erhält einen backend-neutralen booleschen Drop-down-Intent:

- `isDropDownOpen()` liest ihn;
- `setDropDownOpen(bool)` ändert ihn;
- `setOnDropDownChanged()` beobachtet optional echte Zustandswechsel.

Der Zustand ist **transienter Interaktionszustand**, keine dauerhafte Anwendungseigenschaft. Öffnen ist
nur erlaubt, wenn die ComboBox logischen Fokus besitzt und sichtbar/aktiviert ist. Schließen ist immer
erlaubt. Dadurch entsteht eine starke Invariante für spätere Popup-Hosts: Eine offene ComboBox gehört
zu dem Control, das aktuell Keyboard-Fokus besitzt.

Focus-Verlust schließt das Drop-down synchron aus `FocusEvent{false}`. Das passt zur vorhandenen
Reihenfolge des `FocusManager`: Zuerst wird der unfokussierte Zustand veröffentlicht, danach wird das
Event zugestellt. `setDropDownOpen(false)` muss deshalb auch dann legal sein, wenn `hasFocus()` bereits
false ist. Das Verstecken oder Deaktivieren einer fokussierten ComboBox löscht den Fokus schon über
`Widget`; derselbe Focus-Loss-Pfad beendet dadurch auch den Open-State, ohne ComboBox-Sonderlogik in
`Widget::setVisible()` oder `Widget::setEnabled()` einzubauen.

Öffnen/Schließen verändert nur die Presentation. Measurement wird nicht invalidiert und die committed
Selection bleibt unverändert. Der Callback läuft erst nach konsistentem Zustand und Invalidation,
wird vor dem Aufruf kopiert und danach wird kein ComboBox-Member mehr angefasst. Damit bleibt die
bestehende Lifetime-Regel für Callbacks erhalten.

Die erste Tastaturbelegung bleibt bewusst klein und benötigt keine Popup-Geometrie:

- unmodifiziertes F4 toggelt Open/Closed;
- exakt Alt+Down öffnet;
- exakt Alt+Up schließt;
- passendes Key-up wird ohne zweite Mutation konsumiert.

Das vorhandene unmodifizierte Up/Down/Home/End-Verhalten ändert weiterhin die committed Selection.
Preview-Selection bleibt undefiniert und wird in diesem Slice nicht durch provisorischen Zustand
simuliert.

Terminal und Rendered machen den neuen Zustand sichtbar, ohne ein bereits vorhandenes Popup
vorzutäuschen. Der reservierte rechte Marker ist geschlossen `v` und offen `^`. Im Terminal belegen
beide genau eine Zelle. Die Rendered-Messung reserviert die maximale Breite/Höhe beider Marker, sodass
auch bei proportionalen Fonts ein Open/Closed-Wechsel die intrinsische Größe nicht verändert.

### Bewusst vertagt

Dieser ADR definiert **nicht**:

- Popup-Item-Zeilen oder Popup-Platzierung;
- Highlight-/Preview-Selection;
- Enter-Commit oder Escape-Rollback;
- Pointer-Opening, Outside-Click oder Pointer-Capture;
- Popup-Focus-Scopes;
- Type-ahead-Suche;
- native ComboBox-Peers.

Diese Regeln sollen zusammen mit konkreten Popup-Verbrauchern und deterministischen Tests eingeführt
werden, statt sie aus einem booleschen Open-Flag abzuleiten.

### Konsequenzen

Positiv:

- spätere Terminal-/Rendered-/Native-Popup-Hosts teilen einen kleinen Core-Vertrag;
- Focus-Wechsel, Disable und Hide besitzen einen einheitlichen deterministischen Cleanup-Pfad;
- Open/Closed-Feedback ist sofort in beiden vorhandenen Presentation-Backends testbar;
- Measurement bleibt bei Open/Closed-Wechseln stabil;
- keine Backend-Geometrie gelangt in den Core.

Abwägungen:

- der `^`-Marker signalisiert bereits Open-Intent, bevor echte Popup-Zeilen existieren;
- Arrow-Navigation nutzt im offenen Zustand weiterhin committed Selection, bis der getrennte Preview-/
  Commit-Vertrag entworfen ist.
