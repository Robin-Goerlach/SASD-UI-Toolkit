# ADR 0132 – Logical Latin-letter key identity / Logische Tastaturidentität für lateinische Buchstaben

- **Status:** Accepted
- **Date:** 2026-10-07

## English

### Context

`KeyEvent` represents key/shortcut intent while `TextInputEvent` represents committed Unicode or IME
text. ADR 0046 therefore correctly refused to infer Ctrl+letter shortcuts from text input, but the
existing `Key` enum only identifies control, navigation and function keys. The implemented Command,
ShortcutMap and TextField clipboard operations consequently cannot be composed into conventional
Ctrl+A/C/X/V interaction across Terminal and SDL3.

A complete physical-scancode, USB-HID or keyboard-layout abstraction is not justified by this use case.
It would also be the wrong source for application shortcuts: desktop APIs already expose a logical key
symbol after layout interpretation, while committed text remains a separate event stream.

### Decision

Extend the backend-neutral `Key` enum with `a` through `z` as logical Latin-letter identities.

- Letter identity is case-independent; Shift remains an explicit `KeyModifier`.
- A letter `KeyEvent` never substitutes for committed text and does not cause text insertion by itself.
- Desktop adapters map their native logical keycodes to these identities and continue to map native
  text-input events only to `TextInputEvent`.
- Terminal adapters may map only byte sequences whose established raw-terminal meaning is unambiguous.
  In particular, Ctrl+A/C/X/V C0 bytes become Control-modified letter key presses and never text.
- Key releases retain the same identity but Shortcut matching continues to accept presses only.
- `Key::unknown` remains invalid for Shortcut registration and matching.
- Shortcut display uses stable uppercase ASCII tokens (`A` through `Z`) without introducing localization.

This slice deliberately does not model digits, punctuation, dead keys, physical key positions, chords,
Alt-prefixed printable terminal input or a general keyboard-layout engine. Those require concrete,
portable consumers and ambiguity rules of their own.

### Consequences

Application policy can bind editing Commands to Ctrl+A/C/X/V without coupling Core to SDL or terminal
bytes and without confusing shortcut intent with Unicode input. Existing navigation and F-key values
remain unchanged semantically, and exact modifier matching remains deterministic.

The enum is intentionally repetitive rather than encoding a Unicode scalar in `KeyEvent`. That keeps
the supported identity set closed and reviewable, prevents arbitrary committed text from becoming a
shortcut accidentally, and lets each backend fail closed for unsupported native keys.

## Deutsch

### Kontext

`KeyEvent` beschreibt Tasten-/Shortcut-Absicht, während `TextInputEvent` eingegebenen Unicode- oder
IME-Text beschreibt. ADR 0046 hat deshalb korrekt ausgeschlossen, Ctrl+Buchstabe aus Texteingabe zu
erraten. Das bestehende `Key`-Enum kennt jedoch nur Steuer-, Navigations- und Funktionstasten. Die
vorhandenen Commands, ShortcutMap und TextField-Clipboard-Operationen können daher noch nicht zu einer
üblichen Ctrl+A/C/X/V-Interaktion über Terminal und SDL3 zusammengesetzt werden.

Eine vollständige Scancode-, USB-HID- oder Keyboard-Layout-Abstraktion ist für diesen Anwendungsfall
nicht gerechtfertigt. Sie wäre außerdem die falsche Quelle für Anwendungs-Shortcuts: Desktop-APIs
liefern bereits ein nach Layout interpretiertes logisches Tastensymbol, während eingegebener Text über
einen getrennten Eventstrom kommt.

### Entscheidung

Das backend-neutrale `Key`-Enum wird um `a` bis `z` als logische lateinische Buchstabenidentitäten
erweitert.

- Die Buchstabenidentität ist unabhängig von Groß-/Kleinschreibung; Shift bleibt ein expliziter
  `KeyModifier`.
- Ein Buchstaben-`KeyEvent` ersetzt niemals eingegebenen Text und fügt für sich keinen Text ein.
- Desktop-Adapter bilden ihre nativen logischen Keycodes auf diese Identitäten ab und übersetzen native
  Texteingabeereignisse weiterhin ausschließlich nach `TextInputEvent`.
- Terminal-Adapter dürfen nur Bytefolgen abbilden, deren etablierte Raw-Terminal-Bedeutung eindeutig ist.
  Insbesondere werden die C0-Bytes für Ctrl+A/C/X/V zu Control-modifizierten Buchstaben-Key-Presses und
  niemals zu Text.
- Key-Releases behalten dieselbe Identität; Shortcut-Matching akzeptiert weiterhin nur Presses.
- `Key::unknown` bleibt für Shortcut-Registrierung und Matching ungültig.
- Die Shortcut-Anzeige verwendet stabile ASCII-Großbuchstaben (`A` bis `Z`) ohne Lokalisierungssystem.

Dieser Slice modelliert bewusst keine Ziffern, Satzzeichen, Dead Keys, physischen Tastenpositionen,
Chords, Alt-präfixierte druckbare Terminaleingabe oder eine allgemeine Keyboard-Layout-Engine. Dafür
sind eigene konkrete portable Consumer und Ambiguitätsregeln erforderlich.

### Folgen

Anwendungs-Policy kann Editing-Commands an Ctrl+A/C/X/V binden, ohne den Core an SDL oder Terminalbytes
zu koppeln und ohne Shortcut-Absicht mit Unicode-Eingabe zu vermischen. Bestehende Navigation und
F-Tasten bleiben semantisch unverändert; exaktes Modifier-Matching bleibt deterministisch.

Das Enum ist bewusst explizit, statt einen Unicode-Scalar in `KeyEvent` zu tragen. Dadurch bleibt die
unterstützte Identitätsmenge geschlossen und reviewbar, beliebiger eingegebener Text wird nicht
versehentlich zum Shortcut, und jedes Backend kann unbekannte native Tasten fail-closed behandeln.
