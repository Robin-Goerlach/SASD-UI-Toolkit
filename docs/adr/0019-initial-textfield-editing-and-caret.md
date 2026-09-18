# ADR 0019 – Initial TextField editing, cursor and terminal-caret semantics

**Status:** Accepted  
**Date:** 2026-09-18

## English

### Context

M2 now has a semantic event model that separates key events from text input, a logical focus manager,
backend-neutral measurement, automatic box layout, terminal Unicode width metrics and the first
interactive Button.

TextField is the first control that must combine all of those contracts with editable Unicode state.
It also exposes several areas where a terminal backend differs fundamentally from a graphical/native
backend:

- UTF-8 byte offsets are not user-visible cursor positions;
- terminal display width is not Unicode-scalar count;
- a hardware terminal caret is a cursor position, not a glyph that should overwrite text;
- constrained fields need backend-specific horizontal scrolling;
- Unicode grapheme clusters can contain several scalars;
- ANSI/VT input generally reports key presses and text, not a full desktop-style key-state stream.

A first implementation must be correct about these boundaries without prematurely implementing full
Unicode grapheme editing, selection, clipboard, IME or styling.

### Decision

M2 introduces `sasd::ui::TextField` as a single-line editable UTF-8 control.

#### Core text invariant

TextField stores **valid single-line UTF-8**.

A new platform-neutral `sasd::ui::utf8` helper provides deterministic scalar decoding and navigation:

- valid UTF-8 scalars decode normally;
- each malformed source byte becomes one U+FFFD replacement scalar;
- scalar decoding always consumes at least one byte and therefore always makes forward progress;
- C0/C1 controls and Unicode U+2028/U+2029 line/paragraph separators are removed by the single-line
  sanitizer;
- printable Unicode, combining marks and format/ZWJ characters are retained.

Terminal `TextMetrics::decodeOne()` delegates to the same core decoder, so editing and presentation
cannot disagree about UTF-8 scalar boundaries.

#### Cursor model

The public TextField cursor is a **Unicode-scalar index**, not a UTF-8 byte offset and not yet a
grapheme-cluster index.

- `cursorPosition()` returns the scalar index;
- `setCursorPosition()` clamps to the current scalar count;
- Left/Right move by one scalar;
- Home/End move to scalar index 0/end;
- Backspace removes the scalar before the cursor;
- `Key::delete_forward` removes the scalar at the cursor.

This is an explicitly limited M2 contract. Combining/ZWJ sequences are preserved in text, but cursor
movement can currently step through their constituent scalars. Full grapheme-cluster navigation is
deferred to a later Unicode text-editing milestone.

#### Input routing

Editing requires the TextField to own logical keyboard focus and be locally visible/enabled.

- `TextInputEvent` inserts sanitized text at the current scalar cursor;
- a focused TextField consumes TextInputEvent even when sanitization removes all supplied controls;
- unmodified Left/Right/Home/End/Backspace/Delete are editing keys;
- editing-key release events are consumed without repeating the edit;
- modified navigation/editing keys remain unhandled for future selection/word-navigation/shortcut
  semantics;
- Enter is not treated as text insertion in this single-line control.

Text mutations invalidate both measurement and presentation. Cursor-only movement invalidates
presentation but retains valid measurement.

#### Measurement

`MeasurementContext` gains `measureTextField(text)`. Its default delegates to `measureText()` so
existing/custom contexts remain source-compatible.

`TerminalMeasurementContext` measures a TextField as:

`text display width + 2 delimiter cells + 1 reserved caret cell`

The reserved caret cell is present in the natural size even while unfocused. This keeps focus changes
layout-stable and lets an intrinsically sized field show the complete text plus an end-of-text caret
without immediate scrolling.

#### Terminal presentation and caret

Terminal TextField chrome is initially:

- normal enabled: `[text ]`
- focused enabled: `>text <`
- disabled: `(text )`

The exact visible text may be a horizontally scrolled suffix/prefix when parent constraints make the
field narrower than its natural size.

Horizontal viewport state is **not stored in TextField**. The terminal presentation computes it from
the semantic scalar cursor, available cell width and terminal Unicode display widths. Viewport starts
always occur on scalar boundaries and never in the continuation cell of a wide glyph.

`TerminalPresentationSink` exposes an optional `caretPosition()`. The caret is separate from
`ScreenBuffer` glyph cells so a future ANSI/console writer can move the real terminal cursor without
overwriting text with a fake caret character.

The sink remembers the non-owning identity of the TextField that owns the current caret. An unfocused
field clears caret state only when it is that owner, making focus transfer independent from tree
traversal order. A conservative presentation-subtree refresh clears stale caret ownership and then
rebuilds it while descendants replay.

Text containing combining/zero-width sequences that the current simple Cell model cannot preserve is
deferred before old synchronized cells are modified.

### Rationale

Keeping UTF-8 syntax/navigation in a platform-neutral helper prevents the semantic editor and terminal
renderer from evolving different malformed-input or scalar-boundary rules.

A scalar-index cursor is more stable and meaningful than exposing byte offsets. It is still simpler
than a full grapheme-cluster editor, and its limitation can be stated precisely.

Keeping horizontal scrolling and terminal-cursor geometry inside the terminal presentation layer
prevents terminal-cell assumptions from leaking into the public TextField API. A future SDL/native
backend can implement a pixel/font/native viewport for the same semantic cursor.

A real terminal caret should remain a cursor service, not become a stored character in the field.
Separating it from the off-screen glyph buffer preserves underlying text and prepares for real
ANSI/console output.

### Alternatives considered

#### Store cursor as UTF-8 byte offset

Rejected as public semantics. Byte offsets expose encoding mechanics and can point into a continuation
sequence.

#### Claim grapheme-aware editing immediately

Rejected. Correct UAX #29 grapheme segmentation, selection, deletion and terminal rendering require a
larger Unicode text layer than M2 currently has. The toolkit preserves the data instead of pretending
scalar movement is grapheme movement.

#### Store terminal horizontal-scroll offset in TextField

Rejected. A pixel/native backend needs a different viewport representation. Scroll position is
presentation state derived from semantic cursor/content/geometry.

#### Draw a `|` character as the caret

Rejected. It would overwrite or shift real text and conflate terminal device cursor state with content.

#### Insert raw KeyEvent characters into TextField

Rejected. KeyEvent expresses physical/control intent; TextInputEvent is the Unicode/IME-oriented text
boundary.

### Consequences

- M2 has a functional single-line editing core and headless terminal presentation;
- malformed external/pasted input cannot corrupt TextField's UTF-8 invariant;
- terminal and semantic editing share one UTF-8 decoder;
- constrained terminal fields scroll horizontally while keeping the scalar cursor visible;
- a future terminal device writer can consume `caretPosition()` directly;
- selection, Shift/Ctrl navigation, clipboard operations, grapheme-aware editing, IME composition,
  password masking, validation and undo/redo remain future work;
- focus traversal and real terminal input/output are now the largest missing pieces before a small
  end-to-end interactive M2 sample can run in a real terminal.

---

## Deutsch

### Kontext

M2 besitzt inzwischen ein semantisches Eventmodell mit Trennung von KeyEvent und TextInput,
FocusManager, backendneutrales Measurement, automatische Box-Layouts, Terminal-Unicode-Breiten und den
ersten interaktiven Button.

TextField ist das erste Control, das diese Verträge mit editierbarem Unicode-Zustand verbinden muss.
Dabei treten mehrere grundlegende Unterschiede zwischen Terminal und grafischem/nativem Backend auf:

- UTF-8-Byteoffsets sind keine sinnvollen Benutzer-Cursorpositionen;
- Terminal-Displaybreite entspricht nicht der Anzahl Unicode-Scalars;
- ein echter Terminal-Caret ist eine Cursorposition und kein Glyph, der Text überschreiben sollte;
- constrained Felder benötigen backendabhängiges horizontales Scrolling;
- Unicode-Grapheme-Cluster können aus mehreren Scalars bestehen;
- ANSI/VT-Eingabe liefert üblicherweise Key-Presses und Text, aber keinen vollständigen
  Desktop-Key-State-Strom.

Die erste Implementierung muss diese Grenzen korrekt halten, ohne bereits Grapheme-Editing, Selection,
Clipboard, IME oder Styling halb zu implementieren.

### Entscheidung

M2 führt `sasd::ui::TextField` als editierbares einzeiliges UTF-8-Control ein.

#### Core-Textinvariante

TextField speichert **gültiges einzeiliges UTF-8**.

Eine neue plattformneutrale `sasd::ui::utf8`-Utility bietet deterministisches Scalar-Decoding und
Navigation:

- gültige UTF-8-Scalars werden normal dekodiert;
- jedes malformed Eingabebyte wird zu genau einem U+FFFD-Replacement-Scalar;
- Decoding verbraucht immer mindestens ein Byte und macht damit sicher Fortschritt;
- C0/C1-Controls sowie Unicode U+2028/U+2029 Line-/Paragraph-Separator werden vom Single-Line-Sanitizer
  entfernt;
- druckbares Unicode, Combining Marks sowie Format-/ZWJ-Zeichen bleiben erhalten.

Terminal-`TextMetrics::decodeOne()` delegiert an denselben Core-Decoder. Editing und Presentation
können damit nicht unterschiedliche UTF-8-Scalar-Grenzen verwenden.

#### Cursor-Modell

Der öffentliche TextField-Cursor ist ein **Unicode-Scalar-Index**, kein UTF-8-Byteoffset und noch kein
Grapheme-Cluster-Index.

- `cursorPosition()` liefert den Scalar-Index;
- `setCursorPosition()` begrenzt auf die aktuelle Scalar-Anzahl;
- Left/Right bewegen um einen Scalar;
- Home/End bewegen auf 0/Ende;
- Backspace entfernt den Scalar vor dem Cursor;
- `Key::delete_forward` entfernt den Scalar am Cursor.

Das ist ausdrücklich ein begrenzter M2-Vertrag. Combining-/ZWJ-Sequenzen bleiben im Text erhalten,
aber der Cursor kann momentan durch ihre einzelnen Scalars laufen. Vollständige
Grapheme-Cluster-Navigation bleibt einem späteren Unicode-Editing-Meilenstein vorbehalten.

#### Input-Routing

Editing verlangt logischen Keyboard-Fokus sowie lokalen visible/enabled-Zustand.

- `TextInputEvent` fügt sanitisierten Text am aktuellen Scalar-Cursor ein;
- ein fokussiertes TextField konsumiert TextInputEvent auch dann, wenn Sanitization alle gelieferten
  Controls entfernt;
- unmodifiziertes Left/Right/Home/End/Backspace/Delete sind Editing-Keys;
- Release-Events dieser Editing-Keys werden konsumiert, wiederholen aber die Operation nicht;
- modifizierte Navigation/Editing-Keys bleiben für spätere Selection-/Word-Navigation-/Shortcut-
  Semantik unhandled;
- Enter wird im einzeiligen Feld nicht als Texteingabe behandelt.

Textänderungen invalidieren Measurement und Presentation. Reine Cursorbewegung invalidiert nur
Presentation.

#### Measurement

`MeasurementContext` erhält `measureTextField(text)`. Die Default-Implementierung delegiert an
`measureText()`, damit bestehende/eigene Contexts source-kompatibel bleiben.

`TerminalMeasurementContext` misst:

`Text-Displaybreite + 2 Delimiter-Zellen + 1 reservierte Caret-Zelle`

Die Caret-Zelle gehört auch im unfokussierten Zustand zur natürlichen Größe. Dadurch bleibt ein
Fokuswechsel layoutstabil und ein intrinsisch großes Feld kann vollständigen Text plus End-Caret zeigen,
ohne sofort horizontal scrollen zu müssen.

#### Terminal-Presentation und Caret

Die erste Terminal-Chrome lautet:

- normal enabled: `[text ]`
- focused enabled: `>text <`
- disabled: `(text )`

Wenn Parent-Constraints das Feld kleiner als seine natürliche Größe machen, kann der sichtbare Text ein
horizontal gescrollter Ausschnitt sein.

Die horizontale Viewport-Position wird **nicht im TextField gespeichert**. Die Terminal-Presentation
berechnet sie aus semantischem Scalar-Cursor, verfügbarer Zellbreite und Terminal-Unicode-Displaybreiten.
Viewport-Starts liegen immer auf Scalar-Grenzen und niemals in der Continuation-Zelle einer Wide-Glyphe.

`TerminalPresentationSink` liefert eine optionale `caretPosition()`. Der Caret bleibt getrennt von
den `ScreenBuffer`-Glyph-Zellen, sodass ein späterer ANSI-/Console-Writer den echten Terminalcursor
bewegen kann, ohne Text durch ein künstliches Caret-Zeichen zu überschreiben.

Der Sink merkt sich die nicht-besitzende Identität des TextFields, dem der aktuelle Caret gehört. Ein
unfokussiertes Feld löscht den Caret nur, wenn es selbst dieser Owner war; Fokuswechsel bleiben dadurch
unabhängig von der Traversierungsreihenfolge. Ein konservativer Presentation-Subtree-Refresh verwirft
stale Caret-Ownership und baut sie beim Replay neu auf.

Text mit Combining-/Zero-Width-Sequenzen, den das heutige einfache Cell-Modell nicht verlustfrei
speichern kann, wird vor Änderung der zuletzt synchronisierten Zellen `deferred`.

### Begründung

Eine gemeinsame plattformneutrale UTF-8-Syntax-/Navigationsutility verhindert, dass semantischer Editor
und Terminalrenderer unterschiedliche Regeln für malformed Input oder Scalar-Grenzen entwickeln.

Ein Scalar-Index ist stabiler und sinnvoller als öffentlich sichtbare Byteoffsets. Gleichzeitig ist er
kleiner als ein vollständiger Grapheme-Cluster-Editor; seine Grenze kann exakt dokumentiert werden.

Horizontales Scrolling und Terminal-Cursor-Geometrie gehören in die Terminal-Presentation. Dadurch
gelangen keine Terminalzellen-Annahmen in die öffentliche TextField-API. Ein späteres SDL-/Native-
Backend kann für denselben semantischen Cursor eine Pixel-/Font-/Native-Viewport-Logik verwenden.

Ein echter Terminal-Caret ist ein Cursor-Service und kein Zeichen im Feld. Die Trennung vom Off-Screen-
Glyph-Buffer schützt den Text und bereitet reale ANSI-/Console-Ausgabe vor.

### Betrachtete Alternativen

#### Cursor als UTF-8-Byteoffset speichern

Als öffentliche Semantik verworfen. Byteoffsets leaken Encodingdetails und können mitten in einer
Continuation-Sequenz liegen.

#### Sofort Grapheme-aware Editing behaupten

Verworfen. Korrekte UAX-#29-Segmentierung, Selection, Delete und Terminaldarstellung benötigen eine
größere Unicode-Textschicht. Das Toolkit erhält die Daten und nennt Scalar-Navigation nicht
fälschlicherweise Grapheme-Navigation.

#### Terminal-Scrolloffset im TextField speichern

Verworfen. Pixel-/Native-Backends benötigen eine andere Viewportrepräsentation. Scrollposition ist
Presentation-State aus semantischem Cursor, Inhalt und Geometrie.

#### `|` als Caret-Zeichen zeichnen

Verworfen. Dadurch würde echter Text überschrieben/verschoben und Device-Cursor-State mit Inhalt
vermischt.

#### Raw KeyEvent-Zeichen in TextField einfügen

Verworfen. KeyEvent beschreibt physische/controlbezogene Absicht; TextInputEvent ist die Unicode-/IME-
orientierte Textgrenze.

### Konsequenzen

- M2 besitzt einen funktionalen Single-Line-Editing-Core und Headless-Terminaldarstellung;
- malformed externe/eingefügte Eingabe kann die UTF-8-Invariante des TextField nicht beschädigen;
- Terminal und semantisches Editing teilen sich einen UTF-8-Decoder;
- constrained Terminalfelder scrollen horizontal und halten den Scalar-Cursor sichtbar;
- ein späterer Terminal-Device-Writer kann `caretPosition()` direkt verwenden;
- Selection, Shift-/Ctrl-Navigation, Clipboard, Grapheme-aware Editing, IME-Komposition,
  Password-Masking, Validation und Undo/Redo bleiben zukünftige Arbeit;
- Fokusnavigation und reales Terminal-I/O sind nun die größten fehlenden Teile vor einer kleinen
  End-to-End-interaktiven M2-Beispielanwendung.
