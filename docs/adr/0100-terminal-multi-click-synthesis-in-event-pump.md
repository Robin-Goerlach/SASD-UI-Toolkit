# ADR 0100 – Terminal multi-click synthesis in the event pump

**Status:** Accepted  
**Date:** 2026-10-05

## English

### Context

ADR 0095 deliberately keeps `AnsiInputDecoder` clock-free. xterm SGR-1006 reports identify button transitions, modifiers and terminal-cell coordinates, but they do not carry a native single/double/triple-click count. The decoder therefore emits press/release transitions with `click_count == 1` and leaves multi-click classification for a later interaction layer.

The terminal path now has real pointer reporting, Core routing, TextField cell-to-caret mapping and demo integration. Rendered TextField interaction already has useful backend-neutral semantics for `PointerEvent::click_count == 2` and `== 3`. The terminal path needs a deterministic way to produce the same semantic information without putting clocks into the byte parser or widgets.

`TerminalEventPump` already owns the monotonic observation time used for incomplete escape-sequence handling. It is therefore the narrowest terminal layer that has both decoded `PointerEvent` values and a testable clock boundary.

### Decision

`TerminalEventPump` enriches decoded terminal pointer transitions with synthesized multi-click counts.

`TerminalEventPumpOptions` gains:

```cpp
std::chrono::milliseconds multi_click_interval{500};
```

The interval must be non-negative.

A press continues the previous click chain only when all of the following are true:

1. the previous gesture completed with a matching release;
2. that gesture never moved to another terminal cell while pressed;
3. the button identity is the same;
4. the new press is in the exact same terminal cell;
5. the new press is observed no later than `multi_click_interval` after the previous release;
6. supplied monotonic time did not move backwards.

The first completed click has count 1, the second 2, and the third 3. Counts saturate at 3 for further rapid clicks because the current toolkit has explicit useful semantics for single, double and triple click, while exposing larger values would not yet have a defined interaction contract.

The matching release receives the same count as its press. Motion always has count 0.

### Exact-cell policy

Desktop systems commonly use a small pixel rectangle when classifying double clicks. Terminal pointer reports provide only integer cells. The toolkit therefore requires the exact same cell instead of inventing sub-cell geometry or an arbitrary multi-cell tolerance.

This policy can be made configurable later if real terminal usability evidence justifies it, but the first contract stays deterministic and geometrically honest.

### Dragging breaks the click chain

If an active press receives motion into a different terminal cell, that gesture becomes a drag for multi-click purposes. Returning to the original cell before release does not restore click eligibility.

The release still carries the press's current click count so one press/release pair remains internally consistent, but the gesture does not seed a later double click. This prevents TextField selection drags from accidentally becoming the first half of a multi-click gesture.

### Why the event pump owns this state

The ownership boundaries are:

- `AnsiInputDecoder`: incremental VT/ANSI/SGR byte parsing, no clock;
- `TerminalEventPump`: terminal input observation timing and terminal-only click-count synthesis;
- `PointerEvent`: backend-neutral semantic result;
- `PointerRouter`: target routing and capture lifetime;
- TextField interaction helpers: meaning of single/double/triple clicks.

The event pump does not select widgets or mutate UI state. It only enriches a decoded backend event with information that the terminal protocol itself omits.

This is analogous to a desktop backend translating native click-count information into the same `PointerEvent` field, except the terminal backend must derive that information from successive observations.

### Observation-time limitation

SGR reports do not contain timestamps. All transitions decoded from one device read therefore share the `poll(now)` observation time. This is an unavoidable property of the protocol boundary, not an attempt to reconstruct unavailable physical timestamps.

The explicit `poll(TimePoint)` API makes the classification deterministic in tests and keeps production based on `steady_clock`.

### Reset semantics

`resetInput()` now resets both incremental decoder state and remembered terminal click state. An explicit input reset is a semantic discontinuity; a click before it must not combine with a click after it.

Malformed overlapping presses or unmatched releases also clear remembered synthesis state rather than guessing across an ambiguous stream.

### Consequences

- terminal `PointerEvent` values can now carry deterministic single/double/triple counts;
- `AnsiInputDecoder` remains clock-free and protocol-focused;
- the existing terminal event-pump clock seam is reused instead of creating a second timing abstraction;
- drag gestures cannot accidentally seed double clicks;
- press/release counts remain paired;
- Core and Widgets remain unaware of terminal timing policy;
- future terminal TextField word/triple-click selection can consume the same backend-neutral click-count contract already used by rendered interaction.

### Deferred scope

This ADR does not yet add:

- terminal TextField word selection on double click;
- terminal TextField select-all behavior on triple click;
- word-granular captured dragging after a double click;
- configurable spatial tolerance beyond exact terminal-cell equality;
- system-native double-click timing discovery;
- terminal menu pointer interaction;
- wheel or extended-button semantics.

---

## Deutsch

### Kontext

ADR 0095 hält den `AnsiInputDecoder` bewusst frei von Uhrzustand. xterm-SGR-1006-Meldungen enthalten Button-Übergänge, Modifier und Terminalzell-Koordinaten, aber keine native Anzahl für Single-, Double- oder Triple-Click. Der Decoder erzeugt Press-/Release-Übergänge deshalb mit `click_count == 1` und überlässt die Multi-Click-Klassifikation einer späteren Interaktionsschicht.

Der Terminalpfad besitzt inzwischen echtes Pointer-Reporting, Core-Routing, Zell-zu-Caret-Abbildung für TextFields und Demo-Integration. Die Rendered-TextField-Interaktion besitzt bereits sinnvolle backend-neutrale Semantik für `PointerEvent::click_count == 2` und `== 3`. Der Terminalpfad benötigt dieselbe semantische Information, ohne Uhren in den Byte-Parser oder Widgets einzubauen.

`TerminalEventPump` besitzt bereits den monotonen Beobachtungszeitpunkt, der für unvollständige Escape-Sequenzen verwendet wird. Damit ist er die schmalste Terminalschicht, die sowohl dekodierte `PointerEvent`-Werte als auch eine testbare Zeitgrenze besitzt.

### Entscheidung

`TerminalEventPump` ergänzt dekodierte Terminal-Pointer-Übergänge um synthetisierte Multi-Click-Anzahlen.

`TerminalEventPumpOptions` erhält:

```cpp
std::chrono::milliseconds multi_click_interval{500};
```

Das Intervall darf nicht negativ sein.

Ein Press setzt die vorherige Klickkette nur fort, wenn alle folgenden Bedingungen gelten:

1. die vorherige Geste wurde mit passendem Release vollständig abgeschlossen;
2. die vorherige Geste wurde während des gedrückten Zustands nie in eine andere Terminalzelle bewegt;
3. die Button-Identität ist gleich;
4. der neue Press liegt exakt in derselben Terminalzelle;
5. der neue Press wird spätestens `multi_click_interval` nach dem vorherigen Release beobachtet;
6. der übergebene monotone Zeitpunkt ist nicht rückwärts gelaufen.

Der erste vollständige Klick erhält Count 1, der zweite 2 und der dritte 3. Weitere schnelle Klicks sättigen bei 3, weil das aktuelle Toolkit explizite sinnvolle Semantik für Single-, Double- und Triple-Click besitzt, größere Werte jedoch noch keinen definierten Interaktionsvertrag haben.

Das passende Release erhält denselben Count wie sein Press. Motion besitzt immer Count 0.

### Exakte Zell-Policy

Desktop-Systeme verwenden für Double-Click-Klassifikation häufig ein kleines Pixelrechteck. Terminal-Pointer-Meldungen liefern nur ganzzahlige Zellen. Das Toolkit verlangt deshalb exakt dieselbe Zelle, statt Sub-Zell-Geometrie oder eine willkürliche Mehrzellen-Toleranz zu erfinden.

Diese Policy kann später konfigurierbar werden, falls praktische Terminal-Erfahrungen das rechtfertigen. Der erste Vertrag bleibt jedoch deterministisch und geometrisch ehrlich.

### Dragging unterbricht die Klickkette

Erhält ein aktiver Press Motion in eine andere Terminalzelle, gilt die Geste für Multi-Click-Zwecke als Drag. Auch eine Rückkehr in die ursprüngliche Zelle vor dem Release stellt die Klickfähigkeit nicht wieder her.

Das Release trägt weiterhin den aktuellen Click-Count seines Press, damit ein Press-/Release-Paar intern konsistent bleibt. Die Geste bildet jedoch nicht die Grundlage eines späteren Double-Clicks. So kann eine TextField-Selection per Drag nicht versehentlich die erste Hälfte einer Multi-Click-Geste werden.

### Warum der Event Pump diesen Zustand besitzt

Die Verantwortungsgrenzen sind:

- `AnsiInputDecoder`: inkrementelles VT-/ANSI-/SGR-Byte-Parsing ohne Uhr;
- `TerminalEventPump`: zeitliche Beobachtung der Terminaleingabe und terminalspezifische Click-Count-Synthese;
- `PointerEvent`: backend-neutrales semantisches Ergebnis;
- `PointerRouter`: Ziel-Routing und Capture-Lebensdauer;
- TextField-Interaktionshilfen: Bedeutung von Single-/Double-/Triple-Click.

Der Event Pump wählt keine Widgets aus und verändert keinen UI-Zustand. Er ergänzt ausschließlich ein dekodiertes Backend-Event um Information, die das Terminalprotokoll selbst nicht transportiert.

Das entspricht konzeptionell einem Desktop-Backend, das native Click-Count-Information in dasselbe `PointerEvent`-Feld überführt; beim Terminal muss diese Information lediglich aus aufeinanderfolgenden Beobachtungen abgeleitet werden.

### Einschränkung des Beobachtungszeitpunkts

SGR-Meldungen enthalten keine Zeitstempel. Alle Übergänge, die aus einem Device-Read dekodiert werden, teilen deshalb denselben `poll(now)`-Beobachtungszeitpunkt. Das ist eine unvermeidbare Eigenschaft der Protokollgrenze und kein Versuch, nicht vorhandene physische Zeitstempel zu rekonstruieren.

Die explizite API `poll(TimePoint)` macht die Klassifikation in Tests deterministisch; im Produktivpfad bleibt `steady_clock` die Zeitquelle.

### Reset-Semantik

`resetInput()` setzt nun sowohl den inkrementellen Decoderzustand als auch den gemerkten Terminal-Klickzustand zurück. Ein expliziter Input-Reset ist eine semantische Unterbrechung; ein Klick davor darf nicht mit einem Klick danach kombiniert werden.

Fehlerhafte überlappende Presses oder nicht zuordenbare Releases löschen den gemerkten Synthesezustand ebenfalls, statt über einen mehrdeutigen Stream hinweg zu raten.

### Folgen

- Terminal-`PointerEvent`s können jetzt deterministische Single-/Double-/Triple-Click-Counts tragen;
- `AnsiInputDecoder` bleibt uhrfrei und auf das Protokoll fokussiert;
- die vorhandene Zeitgrenze des Terminal-Event-Pumps wird wiederverwendet, statt eine zweite Timing-Abstraktion einzuführen;
- Drag-Gesten können nicht versehentlich Double-Clicks vorbereiten;
- Press-/Release-Counts bleiben paarweise konsistent;
- Core und Widgets bleiben unabhängig von Terminal-Timing-Policy;
- spätere Terminal-TextField-Wort-/Triple-Click-Selection kann denselben backend-neutralen Click-Count-Vertrag verwenden, den die Rendered-Interaktion bereits nutzt.

### Bewusst später

Diese ADR ergänzt noch nicht:

- Terminal-TextField-Wortauswahl per Double-Click;
- Select-All-Verhalten im Terminal-TextField per Triple-Click;
- wortgranulares Capture-Dragging nach Double-Click;
- konfigurierbare räumliche Toleranz zusätzlich zur exakten Terminalzelle;
- Ermittlung des systemeigenen Double-Click-Zeitintervalls;
- Pointer-Bedienung der Terminal-Menüs;
- Wheel- oder Extended-Button-Semantik.
