# ADR 0095 – Terminal SGR mouse input decoding

**Status:** Accepted  
**Date:** 2026-10-04

## English

### Context

The Terminal backend already decodes nonblocking VT/ANSI keyboard and UTF-8 input into backend-neutral `Event` values, while Core already has a backend-neutral `PointerEvent` model used by the Rendered desktop path.

The next terminal-selection work needs the same semantic pointer events without teaching Widgets about xterm escape bytes or terminal-specific coordinate conventions. Modern xterm-compatible terminals commonly report mouse input with SGR 1006 sequences:

`CSI < Cb ; Cx ; Cy M` for press/motion and `CSI < Cb ; Cx ; Cy m` for release.

Those reports are incremental byte-stream input just like keyboard CSI sequences, so `AnsiInputDecoder` is the correct transport-decoding boundary.

### Decision

`AnsiInputDecoder` recognizes complete SGR-1006 mouse reports and emits backend-neutral `PointerEvent` values.

Rules:

1. SGR terminal coordinates are one-based cells; they are converted once at the backend boundary to zero-based SASD UI logical coordinates.
2. Button codes 0, 1 and 2 map to `primary`, `middle` and `secondary`.
3. SGR modifier bits map to backend-neutral Shift, Alt and Control snapshots. xterm's Meta mouse bit maps to `KeyModifier::alt`, matching the terminal keyboard convention already used by the decoder.
4. Motion reports emit `PointerAction::move`, `PointerButton::none` and `click_count == 0`. The low SGR button bits describe held-button state, but the current `PointerEvent` contract deliberately does not encode held buttons on motion; `PointerRouter` capture remains responsible for active gesture ownership.
5. Press/release reports emit `click_count == 1`. SGR does not carry native multi-click counts; timing/position-based double-click synthesis is deferred to a later interaction layer rather than adding clock state to the byte decoder.
6. Wheel reports are consumed atomically but do not emit a semantic event yet because `PointerEvent` has no wheel delta.
7. Extended button encodings not representable by the current `PointerButton` model are likewise consumed atomically without inventing a false button identity.
8. Malformed or out-of-range complete SGR reports are consumed as one CSI sequence so their private marker/decimal parameter bytes never leak into `TextInputEvent`.
9. Incomplete reports remain buffered across nonblocking reads under the same incremental CSI contract as existing keyboard input.

### Why decoding is separate from enabling mouse reporting

Recognizing SGR reports and asking a terminal to send them are different responsibilities.

This ADR adds only the input-decoding capability. `TerminalSession` does not yet enable DEC/xterm mouse tracking modes. A later slice can add explicit, RAII-safe session policy for enabling/disabling the desired tracking mode and SGR encoding without coupling protocol lifetime to byte parsing.

That separation also keeps applications that do not want terminal mouse reporting unchanged.

### Consequences

- terminal input can now represent pointer press, release and motion using the same Core `PointerEvent` type as desktop backends;
- terminal mouse coordinates enter the existing logical UI coordinate system at one well-defined boundary;
- future terminal TextField drag selection can reuse `PointerRouter` instead of inventing a terminal-only gesture model;
- wheel input, extended buttons and multi-click synthesis remain explicit future extensions rather than lossy approximations;
- existing keyboard/UTF-8 decoding semantics remain unchanged.

### Deferred scope

This ADR does not yet add:

- terminal mouse-mode enable/disable sequences (`?1000`, `?1002`, `?1003`, `?1006`);
- routing terminal pointer events through a demo/application host;
- terminal TextField click/drag geometry policy;
- double-/triple-click synthesis for terminal input;
- wheel events;
- extended mouse buttons;
- touch or richer pointer devices.

---

## Deutsch

### Kontext

Das Terminal-Backend dekodiert bereits nichtblockierende VT-/ANSI-Tastatur- und UTF-8-Eingaben in backend-neutrale `Event`-Werte. Gleichzeitig besitzt Core bereits das backend-neutrale `PointerEvent`-Modell, das vom Rendered-Desktop-Pfad verwendet wird.

Für die nächste Terminal-Selection-Arbeit benötigen wir dieselben semantischen Pointer-Events, ohne Widgets mit xterm-Escape-Bytes oder terminalspezifischen Koordinatenkonventionen zu belasten. Moderne xterm-kompatible Terminals melden Mauseingaben häufig über SGR 1006:

`CSI < Cb ; Cx ; Cy M` für Press/Motion und `CSI < Cb ; Cx ; Cy m` für Release.

Diese Meldungen sind genauso inkrementelle Byte-Stream-Eingaben wie Tastatur-CSI-Sequenzen. `AnsiInputDecoder` ist deshalb die richtige Grenze für die Transportdekodierung.

### Entscheidung

`AnsiInputDecoder` erkennt vollständige SGR-1006-Mausmeldungen und erzeugt backend-neutrale `PointerEvent`-Werte.

Regeln:

1. SGR-Terminalkoordinaten sind einsbasierte Zellen; sie werden genau einmal an der Backend-Grenze in nullbasierte logische SASD-UI-Koordinaten umgerechnet.
2. Die Button-Codes 0, 1 und 2 werden auf `primary`, `middle` und `secondary` abgebildet.
3. SGR-Modifier-Bits werden auf backend-neutrale Shift-, Alt- und Control-Snapshots abgebildet. Das von xterm als Meta bezeichnete Maus-Bit wird auf `KeyModifier::alt` gemappt, passend zur bereits im Decoder verwendeten Terminal-Tastaturkonvention.
4. Motion-Meldungen erzeugen `PointerAction::move`, `PointerButton::none` und `click_count == 0`. Die unteren SGR-Button-Bits beschreiben den gehaltenen Button; der aktuelle `PointerEvent`-Vertrag kodiert gehaltene Buttons bei Bewegung bewusst nicht. `PointerRouter`-Capture bleibt für den Besitz einer laufenden Geste verantwortlich.
5. Press-/Release-Meldungen erzeugen `click_count == 1`. SGR transportiert keine native Multi-Click-Anzahl. Eine zeit-/positionsbasierte Double-Click-Synthese wird einer späteren Interaction-Schicht überlassen, statt dem Byte-Decoder Uhrzustand zu geben.
6. Wheel-Meldungen werden atomar konsumiert, erzeugen aber noch kein semantisches Event, weil `PointerEvent` aktuell keinen Wheel-Delta besitzt.
7. Erweiterte Button-Codes, die das aktuelle `PointerButton`-Modell nicht darstellen kann, werden ebenfalls atomar konsumiert, ohne eine falsche Button-Identität zu erfinden.
8. Fehlerhafte oder außerhalb des darstellbaren Bereichs liegende vollständige SGR-Meldungen werden als eine CSI-Sequenz konsumiert, damit weder privates Marker-Zeichen noch Dezimalparameter in `TextInputEvent` gelangen.
9. Unvollständige Meldungen bleiben über nichtblockierende Reads hinweg gepuffert und folgen demselben inkrementellen CSI-Vertrag wie die vorhandene Tastatureingabe.

### Warum Dekodierung und Aktivierung getrennt bleiben

Eine SGR-Meldung zu verstehen und ein Terminal anzuweisen, solche Meldungen zu senden, sind zwei verschiedene Verantwortlichkeiten.

Diese ADR ergänzt ausschließlich die Dekodierfähigkeit. `TerminalSession` aktiviert noch keine DEC-/xterm-Maus-Tracking-Modi. Ein späterer Slice kann eine explizite, RAII-sichere Session-Policy für Aktivierung/Deaktivierung des gewünschten Tracking-Modus und der SGR-Kodierung ergänzen, ohne Protokoll-Lifetime und Byte-Parsing zu vermischen.

Damit bleiben Anwendungen, die keine Terminal-Mausmeldungen wünschen, unverändert.

### Folgen

- Terminaleingabe kann Pointer-Press, -Release und -Motion nun über denselben Core-`PointerEvent`-Typ wie Desktop-Backends darstellen;
- Terminal-Mauskoordinaten gelangen an genau einer definierten Grenze in das bestehende logische UI-Koordinatensystem;
- spätere Terminal-TextField-Drag-Selektion kann `PointerRouter` wiederverwenden, statt ein terminalspezifisches Gestenmodell zu erfinden;
- Wheel-Eingaben, erweiterte Buttons und Multi-Click-Synthese bleiben explizite spätere Erweiterungen statt verlustbehafteter Näherungen;
- die bestehende Tastatur-/UTF-8-Dekodierung bleibt semantisch unverändert.

### Bewusst später

Diese ADR ergänzt noch nicht:

- Terminal-Mausmodus-Aktivierungs-/Deaktivierungssequenzen (`?1000`, `?1002`, `?1003`, `?1006`);
- Routing von Terminal-Pointer-Events durch Demo-/Application-Hosts;
- Terminal-TextField-Klick-/Drag-Geometrie;
- Double-/Triple-Click-Synthese für Terminaleingaben;
- Wheel-Events;
- erweiterte Maustasten;
- Touch oder reichere Pointer-Geräte.
