# ADR 0128 – Backend-neutral collapsed ComboBox Primary pointer activation

**Status:** Accepted  
**Date:** 2026-10-07

## English

### Context

The ComboBox now has complete Terminal popup-row pointer semantics: hover preview, matching
press/release commit and Primary outside-press dismissal. The remaining asymmetry is the collapsed
control itself. Keyboard can open it through F4/Alt+Down, but a Primary click on the collapsed
ComboBox still falls through because Core `ComboBox::onEvent()` does not interpret PointerEvent.

Opening the collapsed surface is not Terminal geometry. It is a backend-neutral control gesture and
should therefore live in Core, just like Button, CheckBox and RadioButton Primary activation. Popup
rows remain presentation overlays and keep their existing Terminal-specific adapter.

The control's drop-down is focus-scoped: `setDropDownOpen(true)` is legal only while ComboBox owns
logical focus. `PointerRouter` deliberately does not choose keyboard focus. Both current form demos
already implement host focus-on-Primary-press before routing, so Core should preserve that separation
instead of acquiring a FocusManager dependency.

### Decision

`ComboBox` adopts the existing private `primary_pointer_gesture.hpp` mechanics used by the simple
clickable controls.

The collapsed gesture is:

1. a fresh Primary press is accepted only when the ComboBox already has logical focus;
2. press inside visible/enabled clipped geometry arms the gesture;
3. PointerRouter capture owns subsequent motion/release delivery;
4. captured motion updates only transient inside/pressed feedback;
5. matching Primary release retires gesture state first;
6. release inside toggles `setDropDownOpen(!isDropDownOpen())`;
7. release outside retires the gesture without changing drop-down state;
8. capture loss clears the transient gesture without changing selection/open state.

The "already focused on press" rule is deliberate host/Core separation. A host that wants normal
focus-on-click behavior must assign focus before routing the press. If it does not, ComboBox leaves the
fresh press unhandled rather than capturing a gesture whose open completion Core would reject.

`ComboBox::isPressed()` exposes only presentation state:

`pointer_armed && pointer_inside && visible && enabled`.

It is independent from committed selection, preview selection and open/closed state.

#### Callback lifetime

The shared gesture helper retires armed state before reporting a completed inside release. ComboBox
computes the next open value before calling `setDropDownOpen()`. That method may synchronously invoke
application callbacks that release/destroy the control; no ComboBox member is accessed after the call
begins.

#### Presentation

Terminal and Rendered collapsed presentation consume the new state without changing geometry.

Terminal:

- hover underlines the existing style;
- pressed uses stable-width `* ... *` delimiters;
- focus inverse styling remains intact;
- release-open changes the existing `v` marker to `^`.

Rendered:

- hover underlines;
- pressed toggles the resolved inverse style so it remains distinguishable even when focus already made
  the control inverse;
- border, selected-text clip and indicator lane remain unchanged.

No pressed/hover state participates in measurement.

### Consequences

Positive:

- collapsed ComboBox pointer opening is shared by Terminal, Rendered and future native hosts;
- Core reuses proven PointerRouter/capture mechanics instead of a Terminal-only click seam;
- focus ownership remains a host policy rather than a ComboBox dependency;
- pressed/capture-loss state is deterministic and presentation-visible;
- popup-row interaction remains cleanly separated from collapsed-control interaction;
- click opening changes no committed selection.

Trade-offs:

- opening occurs on matching release, not initial press; this matches the toolkit's existing armed
  control gesture and guarantees PointerRouter can retire capture before the popup handles later events;
- a host that routes pointer presses without first applying its desired focus policy will not open an
  unfocused ComboBox.

### Deliberately deferred

- PointerRouter/native capture strengthening for popup-row gestures;
- popup scrolling/maximum visible rows;
- Rendered popup-row presentation and interaction;
- native ComboBox peers.

---

## Deutsch

### Kontext

Die ComboBox besitzt inzwischen vollständige Terminal-Popup-Row-Pointer-Semantik: Hover-Preview,
Matching-Press-/Release-Commit und Primary-Outside-Press-Dismissal. Die verbleibende Asymmetrie ist die
collapsed Control selbst. Per F4/Alt+Down kann sie geöffnet werden, ein Primary-Klick auf die
geschlossene ComboBox fällt bisher jedoch durch, weil Core-`ComboBox::onEvent()` keinen
`PointerEvent` interpretiert.

Das Öffnen der collapsed Oberfläche ist keine Terminal-Geometrie, sondern eine backend-neutrale
Control-Geste. Sie gehört deshalb wie die Primary-Aktivierung von Button, CheckBox und RadioButton in
den Core. Popup-Rows bleiben Presentation-Overlays und behalten ihren vorhandenen Terminal-spezifischen
Adapter.

Das Drop-down ist focus-scoped: `setDropDownOpen(true)` ist nur mit logischem ComboBox-Fokus legal.
`PointerRouter` wählt bewusst keinen Keyboard-Fokus. Beide aktuellen Form-Demos setzen bereits
Host-Focus-on-Primary-Press vor dem Routing um; Core soll diese Trennung erhalten statt eine
FocusManager-Abhängigkeit aufzunehmen.

### Entscheidung

`ComboBox` übernimmt die vorhandenen privaten Mechaniken aus `primary_pointer_gesture.hpp`, die auch
die einfachen klickbaren Controls verwenden.

Die collapsed Geste lautet:

1. ein frischer Primary Press wird nur akzeptiert, wenn ComboBox bereits logischen Fokus besitzt;
2. Press innerhalb sichtbarer/aktivierter geclippter Geometrie armed die Geste;
3. `PointerRouter`-Capture besitzt nachfolgende Motion-/Release-Zustellung;
4. captured Motion verändert nur transient Inside-/Pressed-Feedback;
5. Matching Primary Release löscht zuerst den Gesture-State;
6. Release innerhalb toggelt `setDropDownOpen(!isDropDownOpen())`;
7. Release außerhalb löscht die Geste ohne Drop-down-Änderung;
8. Capture-Loss löscht die transiente Geste ohne Selection-/Open-Änderung.

Die Regel "beim Press bereits fokussiert" ist bewusste Host/Core-Trennung. Ein Host mit normalem
Focus-on-Click muss den Fokus vor dem Routing des Press setzen. Tut er das nicht, lässt ComboBox den
frischen Press unhandled, statt eine Geste zu capturen, deren Open-Completion Core später ablehnen
müsste.

`ComboBox::isPressed()` exponiert nur Presentation-Zustand:

`pointer_armed && pointer_inside && visible && enabled`.

Er ist unabhängig von committed Selection, Preview Selection und Open/Closed-State.

#### Callback-Lifetime

Der gemeinsame Gesture-Helfer löscht Armed-State, bevor er ein completed-inside Release meldet.
ComboBox berechnet den nächsten Open-Wert vor `setDropDownOpen()`. Diese Methode darf synchron
Application-Callbacks ausführen, die das Control freigeben/zerstören; nach Beginn des Calls wird kein
ComboBox-Member mehr angefasst.

#### Presentation

Terminal- und Rendered-Collapsed-Presentation konsumieren den neuen Zustand ohne Geometrieänderung.

Terminal:

- Hover unterstreicht den vorhandenen Style;
- Pressed verwendet stabile `* ... *`-Delimiter;
- Focus-Inverse bleibt erhalten;
- Release/Open ändert den vorhandenen Marker von `v` nach `^`.

Rendered:

- Hover unterstreicht;
- Pressed toggelt den bereits aufgelösten Inverse-Style, sodass der Zustand auch bei bereits inverser
  Focus-Darstellung sichtbar bleibt;
- Border, Selected-Text-Clip und Indicator-Lane bleiben unverändert.

Pressed/Hover nehmen nicht am Measurement teil.

### Konsequenzen

Positiv:

- Pointer-Öffnen der collapsed ComboBox ist gemeinsam für Terminal, Rendered und spätere native Hosts;
- Core verwendet bewährte PointerRouter-/Capture-Mechanik statt eines Terminal-only Click-Seams;
- Focus-Ownership bleibt Host-Policy statt ComboBox-Abhängigkeit;
- Pressed-/Capture-Loss-Zustand ist deterministisch und presentation-sichtbar;
- Popup-Row-Interaktion bleibt sauber von collapsed Control-Interaktion getrennt;
- Click-Open verändert keine committed Selection.

Abwägungen:

- geöffnet wird erst beim passenden Release statt schon beim Press; das entspricht der bestehenden
  Armed-Control-Geste des Toolkits und stellt sicher, dass PointerRouter-Capture beendet werden kann,
  bevor spätere Events zum Popup gehen;
- ein Host, der Pointer-Press ohne vorherige Focus-Policy routet, öffnet eine unfokussierte ComboBox
  nicht.

### Bewusst vertagt

- stärkere PointerRouter-/Native-Capture-Policy für Popup-Row-Gesten;
- Popup-Scrolling/maximal sichtbare Rows;
- Rendered-Popup-Row-Presentation und -Interaktion;
- native ComboBox-Peers.
