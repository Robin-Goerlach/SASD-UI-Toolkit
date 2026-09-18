# ADR 0022 – Native terminal device/session boundary with transactional restoration

**Status:** Accepted  
**Date:** 2026-09-18

## English

### Context

M2 can already render semantic widgets into a `ScreenBuffer` and serialize complete frames as UTF-8
plus ANSI/VT sequences. The remaining gap between deterministic rendering and a real interactive
terminal is operating-system session management:

- detecting whether stdin/stdout are interactive;
- querying visible terminal dimensions;
- enabling raw/immediate keyboard input;
- enabling Windows Virtual Terminal processing;
- entering/leaving alternate-screen mode;
- transporting frame bytes;
- restoring every changed terminal property reliably.

These operations are inherently platform-specific and stateful. They should not leak into widgets,
`PresentationSink`, `AnsiFrameEncoder` or `Application`.

### Decision

M2 introduces three separate responsibilities.

#### TerminalDevice

`terminal::TerminalDevice` is the small byte-oriented operating-system boundary. It exposes:

- diagnostic implementation name;
- `isInteractive()`;
- current visible `size()`;
- transactional `beginSession(options)`;
- noexcept/idempotent `endSession()`;
- all-or-throw `write(bytes)`.

`TerminalSessionOptions` currently describes two portable intentions:

- `alternate_screen`;
- `raw_input`.

The public interface contains no POSIX file descriptors, `termios`, Win32 `HANDLE`, console-mode
flags or other platform types.

#### TerminalSession

`terminal::TerminalSession` owns one active TerminalDevice session through RAII.

Its constructor:

1. rejects non-interactive devices before mutation;
2. calls `beginSession()`;
3. marks itself active only after successful completion.

Its destructor and explicit `close()` call idempotent/noexcept `endSession()`.

`present(ScreenBuffer, caret)` composes the already separate
`AnsiFrameEncoder::encode()` with `TerminalDevice::write()`. A transport failure does not silently
close the session; callers may retry or close explicitly.

#### Native implementations

`createNativeTerminalDevice()` returns the current platform implementation without exposing its
native types.

On POSIX targets (Linux/macOS), the first implementation:

- uses stdin/stdout TTY detection;
- queries size with `TIOCGWINSZ`;
- saves/restores input `termios`;
- applies explicit raw-style flags rather than relying on non-POSIX `cfmakeraw()`;
- writes using retry-safe `write()`;
- optionally enters/leaves the alternate screen.

On Windows, the first implementation:

- validates standard console handles with `GetConsoleMode()`;
- queries visible console-window dimensions;
- saves/restores input/output console modes and code pages;
- enables Virtual Terminal output;
- enables Virtual Terminal input when raw input is requested;
- switches console input/output code pages to UTF-8 for the active session;
- writes frame bytes with `WriteFile()`;
- optionally enters/leaves the alternate screen.

### Transactional requirement

`TerminalDevice::beginSession()` has a binding transactional contract.

If any step throws, the implementation must restore every state change it already made before
propagating the exception. This includes raw mode, console modes, code pages and alternate-screen
state.

This rule is essential because a failed UI startup must not leave the user's shell in raw mode, with a
hidden cursor or with modified console configuration.

`endSession()` is noexcept and best-effort. Restoration failures during teardown are not allowed to
escape destructors.

### Testing strategy

Native adapters are compiled by the real CI operating-system jobs:

- POSIX implementation on Linux and macOS;
- Windows implementation on MSVC/Windows.

CI workers are not assumed to provide an interactive TTY. Runtime session semantics are therefore
covered through a deterministic `testing::MockTerminalDevice` that records:

- begin/end counts;
- session options;
- exact byte writes;
- size delegation;
- injected begin/size/write failures.

This keeps native code compile-validated while the portable RAII/transport contract remains fully
unit-testable.

### Rationale

The terminal session is the first place where failure can damage the developer's surrounding shell
experience. Making restoration transactional and RAII-owned is therefore more important than adding
input parsing quickly.

Separating device state from frame encoding preserves the architecture established by ADR 0021:

```text
Widgets
  -> Presentation
  -> ScreenBuffer
  -> AnsiFrameEncoder
  -> TerminalSession
  -> TerminalDevice
  -> OS/TTY
```

The same semantic/rendering code can therefore use POSIX terminals and modern Windows consoles.

### Alternatives considered

#### Write directly with std::cout

Rejected. It provides no ownership of raw mode, console modes, alternate screen, short writes or
restoration.

#### Put termios/Win32 logic into AnsiFrameEncoder

Rejected. Encoding is deterministic serialization; terminal session mutation is an OS responsibility.

#### Put native terminal state into Application

Rejected. Application should remain backend/runtime coordination and not acquire platform console
types or restoration rules.

#### Runtime-test native TTY manipulation in ordinary CI

Rejected as the primary contract test. Hosted CI is generally non-interactive and such tests would be
fragile. Native implementation is compile-tested while deterministic session behavior uses a mock
device.

### Consequences

- the toolkit now has a real OS boundary for terminal output and session lifetime;
- POSIX and Windows terminal configuration no longer needs to leak into UI/presentation code;
- failed session startup has an explicit rollback contract;
- complete rendered frames can now be transported through a real device abstraction;
- actual byte input/read APIs, escape-sequence parsing, resize event production and a runnable terminal
  sample remain the next M2 work.

---

## Deutsch

### Kontext

M2 kann semantische Widgets bereits in einen `ScreenBuffer` rendern und vollständige Frames als
UTF-8 plus ANSI-/VT-Sequenzen serialisieren. Zwischen deterministischem Rendering und einem real
interaktiven Terminal fehlt damit vor allem das betriebssystemspezifische Session-Management:

- Erkennen eines interaktiven stdin/stdout;
- Abfrage der sichtbaren Terminalgröße;
- Raw-/Immediate-Keyboard-Input;
- Windows Virtual Terminal Processing;
- Alternate Screen;
- Byte-Transport;
- zuverlässige Wiederherstellung aller veränderten Terminaleigenschaften.

Diese Aufgaben sind zustandsbehaftet und plattformspezifisch und sollen nicht in Widgets,
`PresentationSink`, `AnsiFrameEncoder` oder `Application` gelangen.

### Entscheidung

M2 trennt drei Verantwortlichkeiten.

#### TerminalDevice

`terminal::TerminalDevice` ist die kleine byteorientierte Betriebssystemgrenze. Sie bietet:

- diagnostischen Implementierungsnamen;
- `isInteractive()`;
- sichtbare `size()`;
- transaktionales `beginSession(options)`;
- noexcept/idempotentes `endSession()`;
- vollständiges oder fehlschlagendes `write(bytes)`.

`TerminalSessionOptions` beschreibt zunächst zwei portable Absichten:

- `alternate_screen`;
- `raw_input`.

Öffentliche Header enthalten keine POSIX-FDs, `termios`, Win32-`HANDLE` oder Console-Mode-Typen.

#### TerminalSession

`terminal::TerminalSession` besitzt eine aktive TerminalDevice-Session per RAII.

Der Konstruktor:

1. lehnt nicht interaktive Devices vor jeder Mutation ab;
2. ruft `beginSession()` auf;
3. setzt den eigenen Active-State erst nach vollständigem Erfolg.

Destruktor und `close()` rufen idempotentes/noexcept `endSession()` auf.

`present(ScreenBuffer, caret)` verbindet den separaten
`AnsiFrameEncoder::encode()` mit `TerminalDevice::write()`. Ein Transportfehler schließt die
Session nicht heimlich; der Aufrufer kann erneut versuchen oder explizit schließen.

#### Native Implementierungen

`createNativeTerminalDevice()` liefert die Plattformimplementierung, ohne native Typen nach außen zu
tragen.

POSIX (Linux/macOS):

- stdin/stdout-TTY-Erkennung;
- Größenabfrage über `TIOCGWINSZ`;
- Speichern/Wiederherstellen von Input-`termios`;
- explizite Raw-Flags statt Abhängigkeit von nicht-basispOSIX `cfmakeraw()`;
- retry-sicheres `write()`;
- optional Alternate Screen.

Windows:

- Standard-Console-Handles über `GetConsoleMode()`;
- sichtbare Console-Window-Größe;
- Speichern/Wiederherstellen von Input-/Output-Modi und Codepages;
- Virtual Terminal Output;
- Virtual Terminal Input bei gewünschtem Raw Input;
- UTF-8-Codepages während der Session;
- Byte-Ausgabe per `WriteFile()`;
- optional Alternate Screen.

### Transaktionale Anforderung

`TerminalDevice::beginSession()` besitzt einen verbindlichen transaktionalen Vertrag.

Schlägt irgendein Schritt fehl, muss die Implementierung alle bis dahin vorgenommenen Änderungen
wiederherstellen, bevor die Exception weitergereicht wird. Das umfasst Raw Mode, Console Modes,
Codepages und Alternate Screen.

Ein fehlgeschlagener UI-Start darf die Shell des Benutzers nicht in Raw Mode, mit verstecktem Cursor
oder veränderter Console-Konfiguration zurücklassen.

`endSession()` ist noexcept und Best-Effort. Fehler beim Teardown dürfen nicht aus Destruktoren
entweichen.

### Teststrategie

Die nativen Adapter werden auf den realen CI-Betriebssystemen kompiliert:

- POSIX auf Linux und macOS;
- Windows unter MSVC/Windows.

CI-Worker werden nicht als interaktive TTYs vorausgesetzt. Runtime-Session-Semantik wird daher durch
ein deterministisches `testing::MockTerminalDevice` getestet, das Begin/End, Optionen, Bytes, Größe
und injizierte Fehler aufzeichnet.

### Begründung

Die Terminal-Session ist die erste Schicht, bei der ein Fehler die umgebende Shell des Entwicklers
beschädigen kann. Transaktionale Wiederherstellung und RAII sind daher wichtiger als möglichst schnell
Input-Parsing hinzuzufügen.

Die Trennung erhält die Architektur aus ADR 0021:

```text
Widgets
  -> Presentation
  -> ScreenBuffer
  -> AnsiFrameEncoder
  -> TerminalSession
  -> TerminalDevice
  -> OS/TTY
```

Damit kann derselbe semantische/rendernde Code POSIX-Terminals und moderne Windows-Konsolen verwenden.

### Betrachtete Alternativen

#### Direkt über std::cout schreiben

Verworfen. Damit gibt es kein Ownership für Raw Mode, Console Modes, Alternate Screen, Short Writes
oder Restoration.

#### termios/Win32-Logik in AnsiFrameEncoder

Verworfen. Encoding ist deterministische Serialisierung; Terminalzustand ist OS-Verantwortung.

#### Nativen Terminalzustand in Application speichern

Verworfen. Application soll keine Plattform-Console-Typen oder Restoration-Regeln übernehmen.

#### Native TTY-Manipulation regulär in Hosted-CI ausführen

Als primärer Vertragstest verworfen. Hosted CI ist meist nicht interaktiv. Native Implementierungen
werden kompiliert, während Session-Semantik deterministisch gegen MockTerminalDevice getestet wird.

### Konsequenzen

- das Toolkit besitzt nun eine reale OS-Grenze für Terminalausgabe und Session-Lifetime;
- POSIX-/Windows-Konfiguration bleibt außerhalb von UI-/Presentation-Code;
- fehlgeschlagener Session-Start besitzt einen expliziten Rollback-Vertrag;
- komplette gerenderte Frames können über eine reale Device-Abstraktion transportiert werden;
- Byte-Input/Read-API, Escape-Sequenz-Parsing, ResizeEvent-Erzeugung und ein ausführbares Terminal-
  Beispiel bleiben die nächsten M2-Schritte.
