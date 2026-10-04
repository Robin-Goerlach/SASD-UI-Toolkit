# ADR 0096 – RAII lifetime for terminal pointer reporting

**Status:** Accepted  
**Date:** 2026-10-04

## English

### Context

ADR 0095 added decoding of xterm-compatible SGR-1006 mouse reports to `AnsiInputDecoder`, but deliberately did not ask terminals to emit those reports. Decoding and protocol lifetime are separate responsibilities.

The next terminal-interaction work needs click and drag input in a way that is safe for normal shells and terminal emulators. Enabling mouse reporting changes terminal-global state. If an application exits normally, closes a session explicitly, throws during construction, or encounters an output error while shutting down, the toolkit must make a best-effort attempt to restore that state before releasing the native terminal session.

The toolkit also does not need continuous hover traffic yet. Terminal TextField selection requires presses, releases, and motion while a button is held.

### Decision

`TerminalSessionOptions` gains an opt-in `pointer_input` flag, defaulting to `false`.

When `pointer_input` is enabled, `TerminalSession` owns the following xterm-compatible protocol for exactly its RAII lifetime:

- `DECSET 1002` (`CSI ? 1002 h`) for button-event tracking: press/release plus motion while a button is held;
- `DECSET 1006` (`CSI ? 1006 h`) for SGR mouse coordinates and button encoding.

The two enable sequences are emitted only after `TerminalDevice::beginSession()` succeeds. This matters because native adapters may first need to configure raw input or Virtual Terminal output.

On normal close/destruction, `TerminalSession` emits the inverse sequence in reverse order:

- `DECRST 1006` (`CSI ? 1006 l`);
- `DECRST 1002` (`CSI ? 1002 l`);

and only then calls `TerminalDevice::endSession()`.

### Failure semantics

Construction has a strong lifetime guarantee across both native and ANSI session layers.

If pointer-protocol activation throws after native `beginSession()` succeeded, `TerminalSession`:

1. makes a best-effort attempt to emit the full disable sequence, because a real byte transport may have failed after partially reaching the terminal;
2. calls `endSession()`;
3. rethrows the original activation failure.

`close()` remains `noexcept`. If the pointer-disable write fails during close/destruction, that transport failure is suppressed only at this teardown boundary and native `endSession()` still runs. Normal frame writes continue to surface errors to callers.

### Why 1002 instead of 1003

Mode 1003 reports all pointer motion, including hover movement with no button held. The current terminal interaction goal needs drag tracking, not continuous hover telemetry. Mode 1002 is therefore the narrower protocol and avoids unnecessary input traffic.

### Why this belongs to TerminalSession

`AnsiInputDecoder` is responsible for interpreting bytes that arrive; it should not own terminal-global state or write control sequences.

`PointerRouter` is responsible for semantic capture after a press; it should not know how xterm tracking modes are enabled.

`TerminalDevice` remains the narrow native OS boundary. It establishes platform-specific console/TTY capability and transports bytes. `TerminalSession` already owns the cross-cutting RAII lifetime that can safely pair protocol enable/disable operations around that device state.

### Consequences

- existing applications are unchanged because pointer reporting is opt-in;
- terminal pointer reporting now has deterministic RAII ownership and restoration;
- drag-capable reports use the same SGR format already supported by ADR 0095;
- constructor failure cannot leave the toolkit owning a native terminal session;
- destructor/close paths still restore native state even if protocol teardown cannot be written;
- continuous hover reporting remains disabled;
- the next slice can route decoded terminal `PointerEvent`s without first solving protocol lifetime.

### Deferred scope

This ADR does not yet add:

- routing terminal pointer events through `PointerRouter` in the demo/application host;
- terminal-specific TextField hit geometry and drag selection policy;
- terminal double-/triple-click synthesis;
- wheel events or extended buttons;
- capability negotiation for terminals that do not support 1002/1006;
- runtime toggling of pointer reporting after session construction.

---

## Deutsch

### Kontext

ADR 0095 hat die Dekodierung xterm-kompatibler SGR-1006-Mausmeldungen im `AnsiInputDecoder` ergänzt, aber bewusst noch nicht das Terminal angewiesen, solche Meldungen zu senden. Dekodierung und Protokoll-Lebensdauer sind getrennte Verantwortlichkeiten.

Für die nächsten Terminal-Interaktionen benötigen wir Klick- und Drag-Eingaben, ohne normale Shells oder Terminalemulatoren nach Programmende in verändertem Zustand zurückzulassen. Maus-Reporting verändert terminalweiten Zustand. Deshalb muss das Toolkit bei normalem Ende, explizitem `close()`, einem Konstruktorfehler oder einem Ausgabefehler beim Herunterfahren bestmöglich restaurieren, bevor die native Terminal-Session freigegeben wird.

Kontinuierliche Hover-Bewegungen benötigen wir aktuell nicht. Für eine TextField-Selektion reichen Press, Release und Bewegung bei gedrückter Taste.

### Entscheidung

`TerminalSessionOptions` erhält das opt-in Flag `pointer_input`, standardmäßig `false`.

Ist `pointer_input` aktiviert, besitzt `TerminalSession` für exakt seine RAII-Lebensdauer folgendes xterm-kompatibles Protokoll:

- `DECSET 1002` (`CSI ? 1002 h`) für Button-Event-Tracking: Press/Release plus Bewegung, solange eine Taste gehalten wird;
- `DECSET 1006` (`CSI ? 1006 h`) für SGR-Mauskoordinaten und Button-Kodierung.

Die Aktivierungssequenzen werden erst geschrieben, nachdem `TerminalDevice::beginSession()` erfolgreich war. Das ist wichtig, weil native Adapter zuvor beispielsweise Raw Input oder Virtual-Terminal-Ausgabe konfigurieren müssen.

Bei normalem `close()` bzw. im Destruktor sendet `TerminalSession` die inversen Sequenzen in umgekehrter Reihenfolge:

- `DECRST 1006` (`CSI ? 1006 l`);
- `DECRST 1002` (`CSI ? 1002 l`);

und ruft erst danach `TerminalDevice::endSession()` auf.

### Fehlersemantik

Die Konstruktion besitzt eine starke Lifetime-Garantie über native und ANSI-Session-Schicht hinweg.

Schlägt die Aktivierung des Pointer-Protokolls fehl, nachdem `beginSession()` bereits erfolgreich war, führt `TerminalSession` folgende Schritte aus:

1. bestmöglicher Versuch, die vollständige Deaktivierungssequenz zu senden, weil ein reales Byte-Transportproblem erst nach teilweise übertragenen Bytes auftreten kann;
2. Aufruf von `endSession()`;
3. erneutes Werfen des ursprünglichen Aktivierungsfehlers.

`close()` bleibt `noexcept`. Schlägt der Disable-Write beim Schließen bzw. im Destruktor fehl, wird dieser Transportfehler ausschließlich an dieser Teardown-Grenze unterdrückt; `endSession()` läuft trotzdem. Normale Frame-Ausgaben melden Fehler weiterhin an den Aufrufer.

### Warum 1002 statt 1003

Modus 1003 meldet jede Pointer-Bewegung, also auch Hover ohne gedrückte Taste. Unser aktuelles Terminalziel benötigt Drag-Tracking, keine permanente Hover-Telemetrie. Modus 1002 ist deshalb die engere und sparsamere Wahl.

### Warum diese Verantwortung in TerminalSession liegt

`AnsiInputDecoder` interpretiert ankommende Bytes; er soll weder terminalweiten Zustand besitzen noch Control-Sequenzen schreiben.

`PointerRouter` besitzt nach einem Press die semantische Capture-Geste; er soll nicht wissen, wie xterm-Tracking aktiviert wird.

`TerminalDevice` bleibt die schmale native Betriebssystemgrenze. Es stellt plattformspezifische TTY-/Console-Fähigkeit her und transportiert Bytes. `TerminalSession` besitzt bereits die übergreifende RAII-Lebensdauer und kann deshalb Protokoll-Aktivierung und -Deaktivierung sauber um den nativen Gerätezustand legen.

### Folgen

- bestehende Anwendungen bleiben unverändert, weil Pointer-Reporting opt-in ist;
- Terminal-Pointer-Reporting besitzt nun deterministischen RAII-Besitz und Restaurierung;
- Drag-fähige Meldungen verwenden dasselbe SGR-Format, das ADR 0095 bereits dekodiert;
- ein Konstruktorfehler lässt keine vom Toolkit besessene native Terminal-Session zurück;
- Destruktor-/`close()`-Pfade restaurieren nativen Zustand auch dann, wenn das Protokoll-Teardown nicht geschrieben werden kann;
- kontinuierliches Hover-Reporting bleibt deaktiviert;
- der nächste Slice kann dekodierte Terminal-`PointerEvent`s routen, ohne zuerst die Protokoll-Lebensdauer lösen zu müssen.

### Bewusst später

Diese ADR ergänzt noch nicht:

- Routing von Terminal-Pointer-Events durch `PointerRouter` im Demo-/Application-Host;
- terminalspezifische TextField-Hit-Geometrie und Drag-Selection-Policy;
- Double-/Triple-Click-Synthese im Terminal;
- Wheel-Events oder erweiterte Buttons;
- Capability Negotiation für Terminals ohne 1002/1006-Unterstützung;
- Laufzeit-Umschalten des Pointer-Reportings nach Session-Konstruktion.
