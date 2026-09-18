# ADR 0023 – Non-blocking terminal byte input and incremental ANSI decoding

**Status:** Accepted  
**Date:** 2026-09-18

## English

### Context

M2 now has a real terminal session/device boundary and deterministic ANSI frame output. The remaining
input path must turn native terminal bytes into the existing backend-neutral `KeyEvent` and
`TextInputEvent` types.

Terminal input is stream-oriented:

- reads may split UTF-8 scalars;
- reads may split CSI/SS3 escape sequences;
- a lone ESC is ambiguous with the first byte of a later escape sequence;
- CR/LF translation differs between environments;
- terminals commonly encode modified cursor keys as CSI parameters.

Blocking reads must not be hidden inside the toolkit event pump.

### Decision

#### Non-blocking device read

`TerminalDevice` gains:

`std::string readAvailable()`

The method returns every byte currently available without waiting for future input. Empty means
"nothing ready now", not end-of-stream.

- POSIX uses zero-timeout `poll()` before `read()`;
- Windows checks the console input handle without waiting and reads available VT bytes;
- `TerminalSession::pollInputBytes()` delegates transport only and performs no decoding.

#### Incremental decoder

`terminal::AnsiInputDecoder` owns pending partial bytes across polls.

`feed(bytes)` emits every unambiguous semantic Event available now. `flushPending()` explicitly
resolves remaining ambiguity when the event loop's Escape timeout expires.

The initial mapping includes:

- printable UTF-8 -> `TextInputEvent`;
- Enter/CR/LF -> `KeyEvent{enter}`;
- Tab -> `KeyEvent{tab}`;
- Backspace/DEL -> `KeyEvent{backspace}`;
- Space -> `KeyEvent{space}` plus `TextInputEvent{" "}`;
- CSI/SS3 arrows -> directional KeyEvents;
- Home/End;
- Delete;
- PageUp/PageDown;
- Shift+Tab;
- xterm modifier parameters for Shift/Alt/Control combinations on supported cursor/navigation keys.

Unknown complete CSI sequences are consumed atomically rather than leaking their parameter bytes as
text.

UTF-8 decoding uses the shared core decoder. Incomplete scalars remain pending; an explicit flush
applies the existing U+FFFD malformed-byte policy.

CRLF is normalized to one Enter even when CR and LF arrive in separate device reads.

### ESC ambiguity

A byte containing only ESC is not emitted immediately. It may still become `ESC [ A` (Up), another
CSI sequence or SS3.

The decoder therefore keeps a lone/incomplete recognized escape prefix pending until either:

- later bytes complete the sequence; or
- the event loop explicitly calls `flushPending()` after its Escape timeout.

This keeps timing policy out of the decoder and avoids hard-coding sleep/delay behavior into tests or
device adapters.

### Rationale

Separating transport from decoding keeps platform code small:

```text
POSIX/Windows bytes
       |
TerminalDevice::readAvailable()
       |
TerminalSession::pollInputBytes()
       |
AnsiInputDecoder
       |
KeyEvent / TextInputEvent
       |
Application / EventDispatcher
```

The decoder is fully deterministic and testable without a live terminal, while native adapters remain
responsible only for retrieving bytes.

Emitting both key intent and text input for Space mirrors desktop input stacks and lets the same
physical terminal key activate Button or insert a space into TextField depending on focused control.

### Deferred behavior

The first decoder deliberately does not yet define:

- Alt+letter/digit identity;
- function keys F1..Fn;
- mouse reporting;
- bracketed paste protocol;
- kitty keyboard protocol;
- full modifyOtherKeys/CSI-u support;
- IME composition;
- terminal-generated key release events.

Those require either a richer `Key` identity model or additional semantic event types.

### Consequences

- native terminal input can now be polled without blocking;
- byte chunks can be split arbitrarily without corrupting UTF-8 or navigation sequences;
- terminal bytes already drive the existing Button/TextField/focus semantics;
- the remaining M2 gap is event-loop timing/resize integration and a runnable native terminal sample.

---

## Deutsch

### Kontext

M2 besitzt inzwischen eine reale Terminal-Device-/Session-Grenze und deterministische ANSI-Ausgabe.
Für den Eingabepfad müssen native Terminalbytes nun in die vorhandenen backendneutralen
`KeyEvent`- und `TextInputEvent`-Typen übersetzt werden.

Terminalinput ist ein Stream:

- Reads können UTF-8-Scalars teilen;
- Reads können CSI-/SS3-Sequenzen teilen;
- ein einzelnes ESC ist mehrdeutig;
- CR/LF unterscheiden sich je Umgebung;
- modifizierte Cursor-Tasten werden häufig über CSI-Parameter kodiert.

Blockierende Reads dürfen nicht versteckt im Event-Pump landen.

### Entscheidung

#### Nichtblockierendes Device-Read

`TerminalDevice` erhält:

`std::string readAvailable()`

Die Methode liefert alle aktuell verfügbaren Bytes, ohne auf zukünftige Eingabe zu warten. Ein leerer
String bedeutet "jetzt nichts verfügbar", nicht Session-Ende.

- POSIX verwendet `poll()` mit Timeout 0 vor `read()`;
- Windows prüft den Console-Input-Handle ohne Wartezeit und liest VT-Bytes;
- `TerminalSession::pollInputBytes()` delegiert ausschließlich den Transport.

#### Inkrementeller Decoder

`terminal::AnsiInputDecoder` hält unvollständige Bytes über mehrere Polls hinweg.

`feed(bytes)` gibt alle bereits eindeutigen Events zurück. `flushPending()` löst verbleibende
Mehrdeutigkeit explizit auf, wenn der Eventloop sein Escape-Timeout erreicht.

Der erste Mapping-Vertrag umfasst:

- druckbares UTF-8 -> `TextInputEvent`;
- Enter/CR/LF -> `KeyEvent{enter}`;
- Tab;
- Backspace/DEL;
- Space -> `KeyEvent{space}` plus `TextInputEvent{" "}`;
- CSI-/SS3-Pfeile;
- Home/End;
- Delete;
- PageUp/PageDown;
- Shift+Tab;
- xterm-Modifier für unterstützte Navigationskeys.

Unbekannte vollständige CSI-Sequenzen werden atomar konsumiert und nicht als Textbestandteile
weitergereicht.

Unvollständiges UTF-8 bleibt gepuffert; ein explizites Flush nutzt die gemeinsame U+FFFD-Policy.
CRLF wird auch über getrennte Reads zu genau einem Enter normalisiert.

### ESC-Mehrdeutigkeit

Ein einzelnes ESC wird nicht sofort als Escape-Taste emittiert. Es kann noch Beginn einer CSI-/SS3-
Sequenz sein.

Der Decoder hält einen solchen Prefix daher, bis:

- weitere Bytes die Sequenz vervollständigen; oder
- der Eventloop nach seinem Escape-Timeout `flushPending()` aufruft.

Damit bleibt Timing-Policy außerhalb von Decoder und Device.

### Begründung

Die Trennung hält Plattformcode klein:

```text
POSIX-/Windows-Bytes
       |
TerminalDevice::readAvailable()
       |
TerminalSession::pollInputBytes()
       |
AnsiInputDecoder
       |
KeyEvent / TextInputEvent
       |
Application / EventDispatcher
```

Space erzeugt sowohl Key-Intent als auch TextInput, damit dieselbe physische Taste je nach Fokus einen
Button aktivieren oder ein Leerzeichen in TextField einfügen kann.

### Bewusst später

Noch nicht festgelegt sind unter anderem:

- Alt+Letter/Digit;
- F1..Fn;
- Mausprotokolle;
- Bracketed Paste;
- Kitty Keyboard Protocol;
- vollständiges CSI-u/modifyOtherKeys;
- IME;
- terminalseitige Key-Release-Events.

### Konsequenzen

- Terminalinput kann jetzt ohne Blockieren gepollt werden;
- beliebig gesplittete Reads beschädigen weder UTF-8 noch Navigation;
- Terminalbytes können bereits die vorhandene Button-/TextField-/Focus-Semantik treiben;
- vor einer ausführbaren M2-Terminalanwendung fehlen hauptsächlich Eventloop-Timing/Resize-Integration
  und das native Beispiel.
