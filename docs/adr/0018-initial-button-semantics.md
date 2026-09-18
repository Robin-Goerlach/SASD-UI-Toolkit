# ADR 0018 – Initial Button activation, measurement and terminal presentation semantics

**Status:** Accepted  
**Date:** 2026-09-18

## English

### Context

M2 has a focus manager, normalized key events, target-to-parent event routing, backend-neutral
measurement services, automatic VBox/HBox layout and terminal presentation. The first interactive
control should connect these existing contracts rather than introduce a separate interaction stack.

A Button also exposes several policy questions that differ between desktop and terminal environments:
key-up events are common on desktop but generally unavailable in ANSI/VT input; visual chrome affects
intrinsic measurement; focus/disabled state should be visible; and an application needs a small way to
observe activation before a general command/action or signal system exists.

### Decision

M2 introduces `sasd::ui::Button` with the following initial contract:

- Button is focusable by default;
- caption text is UTF-8 and changing it invalidates measurement and presentation;
- `setOnActivated(std::function<void()>)` installs one optional synchronous activation callback;
- `activate()` performs programmatic activation when the Button is enabled;
- programmatic activation does not require focus or visibility;
- keyboard activation requires the Button to own logical focus and be visible/enabled;
- unmodified Enter and Space are activation keys;
- activation happens on the **pressed** `KeyEvent`, not on key release;
- focused Enter/Space release events are consumed but do not activate again;
- modified Enter/Space remains unhandled so parent/application shortcut logic may use it;
- Button does not consume `TextInputEvent`; editable text belongs to TextField;
- no pressed/armed state is introduced yet because the terminal input model cannot reliably pair
  key-down/key-up events;
- no pointer/mouse behavior is defined until pointer events and hit testing exist;
- the activation callback is a narrow first-control API and does not establish a general toolkit
  signal/slot/event-listener architecture.

`MeasurementContext` gains a virtual `measureButton(caption)` hook. Its default implementation
delegates to `measureText(caption)`, preserving source compatibility for existing custom measurement
contexts. Presentation-specific contexts may override it to include control chrome.

`TerminalMeasurementContext` overrides `measureButton()` to include the exact four-cell ASCII
chrome used by the terminal renderer. Terminal Button presentation is:

- normal enabled: `[ caption ]`
- focused enabled: `> caption <`
- disabled: `( caption )`

All states have identical width, so focus/enabled transitions require presentation invalidation but no
new measurement.

The terminal backend supports Unicode-width-aware single-line captions. Wide characters retain
`wide_lead` / `wide_continuation` occupancy. Captions requiring combining/grapheme storage or
multi-line Button chrome are deferred before the previous synchronized buffer content is modified.

### Rationale

Activating on key press is required for the terminal-first strategy. Many terminals report key presses
but no matching release event; release-based activation would create a Button that works in desktop
tests but cannot be activated reliably in the first real backend.

Reusing `FocusManager`, `EventDispatcher`, `MeasurementContext` and
`PresentationCoordinator` validates that those abstractions compose into an actual interactive
control.

A Button-specific measurement hook is the smallest demonstrated extension of MeasurementContext.
Desktop backends can later include font padding/native-control metrics without contaminating the
semantic Button with pixels or terminal columns.

The ASCII chrome is intentionally simple and deterministic. Styling, color, themes and native control
appearance remain backend responsibilities.

### Alternatives considered

#### Activate on key release

Rejected for the initial contract because ANSI/VT input usually cannot provide a reliable release
event.

#### Add a generic signal/slot subsystem before Button

Deferred. One activation callback is enough to validate control semantics. A generic observer/command
model should be driven by multiple real controls and use cases.

#### Put terminal padding directly in Button::onMeasure()

Rejected. Four terminal cells are not a backend-neutral measurement and would be meaningless for
proportional desktop fonts/native peers.

#### Treat Button caption as TextInputEvent

Rejected. Enter/Space activation is control intent, while TextInputEvent represents textual input and
IME/Unicode text composition.

#### Add a persistent pressed state immediately

Deferred. Without reliable terminal key-up or pointer press/release events, such state would have
inconsistent lifetime across backends.

### Consequences

- M2 has its first interactive focusable control;
- existing focus, routing, measurement, layout and presentation abstractions are validated together;
- terminal applications can measure and render Button alongside Label in VBox/HBox;
- Button activation callbacks may mutate/reparent/release controls synchronously, subject to the
  existing EventDispatcher lifetime rules;
- pointer interaction, richer command/action binding, accessibility semantics, default/cancel buttons,
  mnemonics and styling remain future work;
- TextField is the next control that will exercise TextInputEvent, editing state and caret behavior.

---

## Deutsch

### Kontext

M2 besitzt bereits FocusManager, normalisierte KeyEvents, Target-zu-Parent-Eventrouting,
backendneutrale Measurement-Services, automatische VBox/HBox-Layouts und Terminal-Presentation. Das
erste interaktive Control soll diese vorhandenen Verträge verbinden, statt einen separaten
Interaktions-Stack einzuführen.

Ein Button wirft außerdem Policy-Fragen auf, die sich zwischen Desktop und Terminal unterscheiden:
Key-Up-Ereignisse sind auf Desktops üblich, in ANSI/VT-Eingabe aber normalerweise nicht zuverlässig
vorhanden; sichtbare Chrome beeinflusst die intrinsische Größe; Fokus/Disabled-State sollen sichtbar
sein; und Anwendungen brauchen eine kleine Aktivierungsbenachrichtigung, bevor ein allgemeines
Command-/Action-/Signal-System existiert.

### Entscheidung

M2 führt `sasd::ui::Button` mit folgendem ersten Vertrag ein:

- Button ist standardmäßig fokusfähig;
- Caption-Text ist UTF-8; Änderungen invalidieren Measurement und Presentation;
- `setOnActivated(std::function<void()>)` installiert einen optionalen synchronen
  Aktivierungs-Callback;
- `activate()` führt programmatische Aktivierung aus, solange der Button enabled ist;
- programmatische Aktivierung erfordert weder Fokus noch Sichtbarkeit;
- Tastaturaktivierung verlangt logischen Fokus sowie visible/enabled;
- unmodifiziertes Enter und Space sind Aktivierungstasten;
- Aktivierung erfolgt beim **pressed**-`KeyEvent`, nicht beim Key-Release;
- Release-Events von Enter/Space werden bei fokussiertem Button konsumiert, aktivieren aber nicht
  erneut;
- modifiziertes Enter/Space bleibt unhandled, damit Parent-/Application-Shortcuts es verwenden können;
- `TextInputEvent` wird nicht konsumiert; editierbarer Text gehört zu TextField;
- ein dauerhafter Pressed-/Armed-State wird noch nicht eingeführt, da Terminalinput Key-Down/Key-Up
  nicht zuverlässig paaren kann;
- Pointer-/Mausverhalten wird erst definiert, wenn Pointer-Events und Hit Testing existieren;
- der Aktivierungs-Callback ist eine schmale erste Control-API und legt noch keine allgemeine
  Signal/Slot-/Listener-Architektur fest.

`MeasurementContext` erhält den virtuellen Hook `measureButton(caption)`. Die Default-Implementierung
delegiert an `measureText(caption)`, sodass bestehende eigene MeasurementContexts source-kompatibel
bleiben. Presentation-spezifische Contexts dürfen Control-Chrome einrechnen.

`TerminalMeasurementContext` überschreibt `measureButton()` und rechnet die exakt vier Zellen breite
ASCII-Chrome ein, die der Terminal-Renderer verwendet. Darstellung:

- normal enabled: `[ caption ]`
- focused enabled: `> caption <`
- disabled: `( caption )`

Alle Zustände besitzen dieselbe Breite; Fokus-/Enabled-Wechsel benötigen daher nur
Presentation-Invalidierung, kein Re-Measure.

Das Terminal-Backend unterstützt Unicode-width-aware einzeilige Captions. Wide Characters behalten
`wide_lead` / `wide_continuation`. Captions, die Combining-/Grapheme-Speicher oder mehrzeilige
Button-Chrome benötigen, werden vor jeder Änderung der zuletzt synchronisierten Bufferdarstellung
`deferred`.

### Begründung

Aktivierung auf Key-Press ist für die Terminal-first-Strategie erforderlich. Viele Terminals melden
Tastendrücke, aber kein korrespondierendes Release; Release-basierte Aktivierung würde einen Button
erzeugen, der in Desktop-Tests funktioniert, im ersten realen Backend aber nicht zuverlässig
aktivierbar ist.

Die Wiederverwendung von `FocusManager`, `EventDispatcher`, `MeasurementContext` und
`PresentationCoordinator` validiert, dass diese Abstraktionen zusammen ein tatsächliches
interaktives Control tragen.

Ein Button-spezifischer Measurement-Hook ist die kleinste praktisch belegte Erweiterung des
MeasurementContext. Desktop-Backends können später Font-Padding/native Control-Metriken einbringen,
ohne Pixel oder Terminalspalten in den semantischen Button zu leaken.

Die ASCII-Chrome bleibt bewusst einfach und deterministisch. Styling, Farbe, Themes und native
Control-Darstellung bleiben Backendaufgabe.

### Betrachtete Alternativen

#### Aktivierung auf Key-Release

Für den ersten Vertrag verworfen, weil ANSI/VT-Eingabe normalerweise kein zuverlässiges Release-Event
liefert.

#### Vor Button ein generisches Signal/Slot-System bauen

Verschoben. Ein Aktivierungs-Callback reicht zur Validierung der Control-Semantik. Ein allgemeines
Observer-/Command-Modell soll aus mehreren realen Controls und Anwendungsfällen entstehen.

#### Terminal-Padding direkt in Button::onMeasure()

Verworfen. Vier Terminalzellen sind keine backendneutrale Metrik und für proportionale Desktop-Fonts
oder native Peers bedeutungslos.

#### Button-Caption über TextInputEvent behandeln

Verworfen. Enter/Space-Aktivierung beschreibt Control-Intent; TextInputEvent steht für Texteingabe und
IME-/Unicode-Komposition.

#### Sofort dauerhaften Pressed-State einführen

Verschoben. Ohne zuverlässige Terminal-Key-Up- oder Pointer-Press-/Release-Events hätte dieser Zustand
je Backend unterschiedliche Lebensdauer.

### Konsequenzen

- M2 besitzt das erste interaktive fokusfähige Control;
- vorhandene Fokus-, Routing-, Measurement-, Layout- und Presentation-Abstraktionen werden gemeinsam
  praktisch validiert;
- Terminalanwendungen können Button zusammen mit Label in VBox/HBox messen und darstellen;
- Button-Aktivierungs-Callbacks dürfen Controls synchron mutieren/reparenten/releasen, unter den
  bestehenden Lifetime-Regeln des EventDispatcher;
- Pointer-Interaktion, reichere Command-/Action-Bindung, Accessibility, Default-/Cancel-Buttons,
  Mnemonics und Styling bleiben zukünftige Arbeit;
- TextField ist das nächste Control, das TextInputEvent, Editierzustand und Caret-Verhalten praktisch
  validieren wird.
