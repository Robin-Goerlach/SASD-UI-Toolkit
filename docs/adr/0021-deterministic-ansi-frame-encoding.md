# ADR 0021 – Deterministic ANSI/VT frame encoding before terminal device I/O

**Status:** Accepted  
**Date:** 2026-09-18

## English

### Context

M2 can now build a semantic widget tree, measure/layout it, render it into a terminal `ScreenBuffer`,
track an optional TextField hardware-caret request and navigate focus with Tab/Shift+Tab.

The next visible step is producing actual ANSI/VT bytes. It would be tempting to combine this
immediately with stdout handles, raw terminal mode, alternate-screen setup, resize detection and input
parsing. Doing so would make rendering tests depend on a live console and would mix portable ANSI
serialization with operating-system session management.

### Decision

M2 introduces `terminal::AnsiFrameEncoder` as a pure, deterministic **full-frame encoder**.

Its input is:

- a completed `ScreenBuffer`;
- an optional 0-based caret `Point`.

Its output is one `std::string` containing UTF-8 and ANSI/VT CSI sequences.

The initial encoding contract is:

- hide the hardware cursor before repainting;
- clear the complete display;
- address every screen row explicitly using 1-based ANSI cursor coordinates;
- encode each normal/wide-lead Unicode scalar as UTF-8;
- never emit `CellRole::wide_continuation` as an independent blank because the corresponding
  wide-lead glyph already consumes both physical columns;
- after drawing, move to and show the hardware cursor only when the requested caret lies inside the
  ScreenBuffer;
- when no valid caret exists, leave the hardware cursor hidden;
- an empty ScreenBuffer still produces deterministic hide-cursor + clear-display output.

The shared core UTF-8 helper gains `utf8::appendScalar()`. Invalid Unicode scalar values encode as
U+FFFD, matching the toolkit's deterministic replacement policy.

`AnsiFrameEncoder` performs **no I/O** and manages no terminal session state. In particular, it does
not:

- open/write stdout or Windows console handles;
- switch POSIX termios modes;
- enable Windows Virtual Terminal processing;
- enter/leave alternate screen;
- query terminal size;
- read or parse keyboard input;
- install signal/console handlers.

Those responsibilities belong to a following terminal device/session layer.

### Rationale

A pure encoder can be unit-tested identically on Linux, macOS and Windows and keeps the rendering path
independent from whichever system API eventually transports the bytes.

Explicit cursor addressing avoids newline translation differences and keeps row placement deterministic.
It also makes the encoder suitable for Windows Terminal/ConPTY and POSIX VT-style terminals using the
same byte representation.

The first implementation deliberately encodes a complete frame rather than introducing a diff
algorithm. Correct terminal output is the priority. A later frame-diff/run encoder can optimize output
without changing Widget, PresentationSink or ScreenBuffer semantics.

Keeping the hardware caret separate from glyph cells preserves TextField content and lets the actual
device layer move the terminal cursor after the frame is drawn.

### Alternatives considered

#### Write directly to stdout from TerminalPresentationSink

Rejected. PresentationSink should synchronize semantic widget state into the backend presentation
model; transport errors and terminal session ownership are a different responsibility.

#### Combine ANSI serialization and OS terminal session management now

Rejected. This would make deterministic CI testing harder and create unnecessary POSIX/Windows
conditional code in the serialization path.

#### Emit CR/LF between rows

Rejected for the first contract. Explicit CSI row/column addressing avoids platform newline
translation and terminal line-wrap assumptions.

#### Implement incremental frame diffs immediately

Deferred. Full-frame output is simpler and establishes a correctness baseline. Diffing is a later
performance optimization.

### Consequences

- the toolkit now has a deterministic path from semantic widgets to real ANSI/VT bytes;
- Unicode wide-cell occupancy is preserved through output serialization;
- TextField caret requests become real ANSI cursor coordinates without fake caret glyphs;
- the same encoder is usable by POSIX terminals and modern Windows Terminal/ConPTY;
- output is currently full-frame and intentionally not bandwidth-optimal;
- alternate screen, raw mode, terminal-size detection, byte transport and input parsing remain the next
  terminal device/session work.

---

## Deutsch

### Kontext

M2 kann inzwischen einen semantischen Widget-Baum aufbauen, messen/anordnen, in einen terminalen
`ScreenBuffer` rendern, einen optionalen TextField-Hardware-Caret führen und den Fokus mit
Tab/Shift+Tab navigieren.

Der nächste sichtbare Schritt sind echte ANSI-/VT-Bytes. Es wäre naheliegend, dies sofort mit
stdout-Handles, Raw Mode, Alternate Screen, Resize-Erkennung und Input-Parsing zu vermischen. Dadurch
würden Renderingtests jedoch ein reales Terminal benötigen und portable ANSI-Serialisierung würde mit
betriebssystemspezifischem Session-Management gekoppelt.

### Entscheidung

M2 führt `terminal::AnsiFrameEncoder` als reinen deterministischen **Full-Frame-Encoder** ein.

Eingabe:

- ein fertig gerenderter `ScreenBuffer`;
- ein optionaler 0-basierter Caret-`Point`.

Ausgabe:

- ein `std::string` aus UTF-8 und ANSI-/VT-CSI-Sequenzen.

Der erste Vertrag:

- Hardware-Cursor vor dem Repaint verstecken;
- komplettes Display löschen;
- jede Screen-Zeile explizit mit 1-basierten ANSI-Cursor-Koordinaten adressieren;
- normale und Wide-Lead-Unicode-Scalars als UTF-8 ausgeben;
- `CellRole::wide_continuation` niemals als zusätzliches Leerzeichen ausgeben, weil die zugehörige
  Wide-Glyphe bereits zwei physische Spalten verbraucht;
- nach dem Zeichnen den Hardware-Cursor nur dann positionieren und anzeigen, wenn der angeforderte
  Caret innerhalb des ScreenBuffers liegt;
- ohne gültigen Caret bleibt der Cursor versteckt;
- auch ein leerer ScreenBuffer erzeugt deterministische Hide-Cursor-/Clear-Display-Ausgabe.

Die gemeinsame Core-UTF-8-Utility erhält `utf8::appendScalar()`. Ungültige Unicode-Scalarwerte werden
als U+FFFD kodiert und folgen damit derselben deterministischen Replacement-Policy wie das Decoding.

`AnsiFrameEncoder` führt **kein I/O** durch und verwaltet keinen Terminal-Session-State. Insbesondere
öffnet/schreibt er keine Handles, schaltet keinen POSIX-termios- oder Windows-VT-Modus, betritt keinen
Alternate Screen, fragt keine Größe ab und liest/parst keine Eingabe.

Diese Aufgaben gehören in eine folgende Terminal-Device-/Session-Schicht.

### Begründung

Ein reiner Encoder kann auf Linux, macOS und Windows identisch getestet werden und hält den
Renderingpfad unabhängig davon, welche System-API die Bytes später transportiert.

Explizite Cursoradressierung vermeidet Unterschiede durch Newline-Übersetzung und macht die
Zeilenposition deterministisch. Derselbe Byte-Frame kann damit von POSIX-VT-Terminals und modernem
Windows Terminal/ConPTY verwendet werden.

Die erste Implementierung kodiert bewusst komplette Frames statt bereits einen Diff-Algorithmus
einzuführen. Korrekte Ausgabe hat Vorrang; ein späterer Frame-Diff-/Run-Encoder kann die Datenmenge
optimieren, ohne Widget-, PresentationSink- oder ScreenBuffer-Semantik zu ändern.

Der vom Glyph-Buffer getrennte Hardware-Caret schützt TextField-Inhalte und kann nach dem Frame vom
Device-Layer als echter Terminalcursor gesetzt werden.

### Betrachtete Alternativen

#### Direkt aus TerminalPresentationSink nach stdout schreiben

Verworfen. PresentationSink synchronisiert semantischen Widget-Zustand in das Backend-
Präsentationsmodell; Transportfehler und Terminal-Session-Ownership sind eine andere Verantwortung.

#### ANSI-Serialisierung und OS-Terminal-Session sofort zusammenbauen

Verworfen. Das würde deterministische CI-Tests erschweren und unnötigen POSIX-/Windows-
Conditional-Code in die Serialisierung bringen.

#### Zeilen mit CR/LF ausgeben

Für den ersten Vertrag verworfen. Explizite CSI-Zeilen-/Spaltenadressierung vermeidet
plattformabhängige Newline-Übersetzung und Line-Wrap-Annahmen.

#### Sofort inkrementelle Frame-Diffs implementieren

Verschoben. Full-Frame-Ausgabe ist einfacher und schafft eine Korrektheitsbasis; Diffing ist spätere
Performance-Optimierung.

### Konsequenzen

- das Toolkit besitzt nun einen deterministischen Pfad von semantischen Widgets bis zu echten
  ANSI-/VT-Bytes;
- Unicode-Wide-Cell-Occupancy bleibt bei der Ausgabe erhalten;
- TextField-Caret-Requests werden echte ANSI-Cursor-Koordinaten ohne Fake-Caret-Glyph;
- derselbe Encoder ist für POSIX-Terminals und modernes Windows Terminal/ConPTY nutzbar;
- die Ausgabe ist zunächst Full-Frame und bewusst nicht bandbreitenoptimal;
- Alternate Screen, Raw Mode, Terminalgrößen-Erkennung, Byte-Transport und Input-Parsing sind die
  nächsten Aufgaben der Terminal-Device-/Session-Schicht.
