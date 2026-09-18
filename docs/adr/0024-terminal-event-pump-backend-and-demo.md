# ADR 0024 – Terminal event pump, Backend integration and runnable M2 loop

**Status:** Accepted  
**Date:** 2026-09-18

## English

### Context

M2 already has native terminal session ownership, non-blocking byte transport, incremental ANSI input
decoding, semantic focus/event routing, terminal presentation and ANSI frame output.

The remaining integration problem is not another widget or parser. The toolkit needs a small runtime
bridge that:

- turns terminal size changes into `ResizeEvent`;
- resolves incomplete input sequences after a bounded timeout;
- feeds those events through the existing `Backend` / `Application` contract;
- remains non-blocking and testable without an interactive terminal;
- does not hide rendering, sleeping or application policy inside the core event source.

A first runnable sample is also needed to prove that the previously independent M2 pieces compose into
one application path.

### Decision

#### TerminalEventPump

M2 introduces `terminal::TerminalEventPump`.

It owns no terminal session and performs no routing, sleeping or rendering. One `poll()` call is one
non-blocking observation step.

The initial order is:

1. query the current terminal size;
2. emit `ResizeEvent` only when it differs from the last observed size;
3. poll bytes already available from `TerminalSession`;
4. feed those bytes to `AnsiInputDecoder`;
5. if an incomplete byte sequence remains and no further progress occurs before the configured timeout,
   call `flushPending()`.

The constructor captures the initial terminal size and does **not** emit a synthetic initial
`ResizeEvent`. The application already needs that size for its first layout.

Resize is emitted before input collected in the same tick. This establishes a deterministic precedent:
downstream code can update geometry before processing keyboard/text input from that observation step.

The default incomplete-sequence timeout is 30 ms. Supplying time explicitly to `poll(time_point)`
makes timeout behavior deterministic in tests.

Whenever additional bytes extend an incomplete sequence, the timeout window starts again. This allows
ESC, `[` and the final CSI byte to arrive in separate non-blocking reads without prematurely turning
ESC into the Escape key.

The timeout applies to every incomplete decoder prefix, not only ESC. This prevents a truncated UTF-8
scalar from remaining buffered forever. Expiry uses the decoder's documented U+FFFD replacement policy.

#### TerminalBackend

M2 also introduces `terminal::TerminalBackend : Backend`.

Its responsibilities remain deliberately narrow:

- own a `TerminalDevice`;
- initialize/shutdown a `TerminalSession`;
- own a `TerminalEventPump`;
- expose semantic events through the existing non-blocking `Backend::pollEvent()` contract.

Initialization is transactional. Session and pump are first built in local RAII objects and only
published to the backend after both succeed.

Shutdown is idempotent and destroys decoder/timing state before closing the terminal session.

Presentation is **not** moved into the generic `Backend` interface. Application code continues to use:

`TerminalPresentationSink -> ScreenBuffer -> TerminalSession::present()`

This preserves the existing separation between event/lifecycle backend responsibilities and
presentation synchronization.

#### Runnable terminal form example

The repository builds `sasd_ui_terminal_demo` when `SASD_UI_BUILD_EXAMPLES=ON` (default).

The sample uses the ordinary toolkit path:

```text
TerminalBackend
      |
 Application
      |
Key/Text Events ------> focused Widget
      |                    |
      +--> FocusTraversal <-+
      |
 ResizeEvent
      |
 measure / arrange
      |
PresentationCoordinator
      |
TerminalPresentationSink
      |
 ScreenBuffer
      |
TerminalSession / ANSI
```

The sample contains:

- a Label;
- an editable TextField;
- a Greet Button;
- a status Label;
- an Exit Button;
- Tab/Shift+Tab focus traversal;
- Escape-to-exit;
- resize-triggered ScreenBuffer/layout updates.

The sample uses an 8 ms sleep only as a simple idle-CPU policy for the first demonstration. That delay
is **example policy**, not part of `TerminalEventPump`, `TerminalBackend` or `Application`.
A later wait/wakeup abstraction may replace polling without changing event semantics.

### Testing and release validation

`TerminalEventPump` is deterministically unit-tested with `MockTerminalDevice`, including:

- no synthetic initial resize;
- changed-size-only resize production;
- resize-before-input ordering;
- lone ESC timeout;
- timeout extension while a CSI sequence makes progress;
- timeout flushing of truncated UTF-8;
- invalid negative timeout rejection.

`TerminalBackend` is tested for session ownership, byte-to-semantic-event conversion, resize
production, presentation access and duplicate initialization.

The demo is compiled by the Linux/GCC, Linux/Clang, sanitizer, macOS/AppleClang and Windows/MSVC CI
jobs.

Hosted CI compilation is **not** considered proof that a real interactive terminal behaves correctly.
The M2 exit criterion still requires manual smoke validation in at least:

- a Linux/xterm-like environment;
- a modern Windows console/Windows Terminal environment.

### Alternatives considered

#### Add a blocking Application::run() now

Deferred. Waiting, wakeup sources, timers and multiple future backend types are not yet mature enough
to define one universal blocking run-loop contract.

#### Sleep inside TerminalEventPump

Rejected. The event source must stay non-blocking and deterministic. Idle scheduling belongs to the
caller/runtime loop.

#### Let TerminalBackend render automatically

Rejected. It would merge event/lifecycle and presentation concerns and enlarge the generic Backend
interface before a second visible backend validates that design.

#### Emit ResizeEvent immediately on construction

Rejected. The initial size is already required for initial layout and is exposed as terminalSize().
ResizeEvent represents a change, not initial discovery.

### Consequences

- the terminal backend now participates in the same `Application` lifecycle/event API as the mock
  backend;
- terminal resize is represented by the existing backend-neutral `ResizeEvent`;
- ESC/incomplete-sequence timing is deterministic and outside the decoder;
- the repository contains its first buildable real-terminal M2 application;
- manual Linux/Windows interactive smoke validation remains required before claiming the M2 exit
  criterion;
- simple styling and richer terminal protocols remain later M2 work.

---

## Deutsch

### Kontext

M2 besitzt bereits native Terminal-Session-Verwaltung, nichtblockierenden Byte-Transport,
inkrementelles ANSI-Input-Decoding, semantisches Fokus-/Eventrouting, Terminal-Presentation und
ANSI-Frame-Ausgabe.

Die verbleibende Integrationsaufgabe ist damit kein weiterer Parser oder Widget-Typ, sondern eine kleine
Runtime-Brücke, die:

- Größenänderungen als `ResizeEvent` erzeugt;
- unvollständige Inputsequenzen nach einem begrenzten Timeout auflöst;
- Events über den bestehenden `Backend`-/`Application`-Vertrag liefert;
- nichtblockierend und ohne echtes Terminal testbar bleibt;
- Rendering, Sleep oder Anwendungspolitik nicht im Core-Eventsource versteckt.

Zusätzlich wird ein ausführbares Beispiel benötigt, das den vollständigen M2-Pfad zusammensetzt.

### Entscheidung

#### TerminalEventPump

M2 führt `terminal::TerminalEventPump` ein.

Er besitzt keine TerminalSession und routet, schläft oder rendert nicht. Ein `poll()` ist genau ein
nichtblockierender Beobachtungsschritt:

1. aktuelle Terminalgröße abfragen;
2. `ResizeEvent` nur bei Änderung erzeugen;
3. aktuell verfügbare Bytes aus `TerminalSession` lesen;
4. Bytes an `AnsiInputDecoder` geben;
5. bei ausbleibendem Fortschritt einer unvollständigen Sequenz nach dem konfigurierten Timeout
   `flushPending()` aufrufen.

Der Konstruktor merkt sich die Startgröße, erzeugt aber kein künstliches initiales `ResizeEvent`.
Diese Größe wird ohnehin für das erste Layout benötigt.

Resize kommt innerhalb desselben Ticks vor Inputevents. So kann Anwendungscode zuerst Geometrie
aktualisieren.

Das Standardtimeout beträgt 30 ms. `poll(time_point)` erlaubt vollständig deterministische Tests.

Treffen weitere Bytes einer unvollständigen Sequenz ein, beginnt das Timeoutfenster neu. Damit dürfen
ESC, `[` und CSI-Finalbyte in getrennten Reads kommen.

Das Timeout gilt für alle unvollständigen Decoderpräfixe. Dadurch bleibt auch ein abgebrochenes UTF-8-
Scalar nicht unbegrenzt gepuffert; beim Ablauf greift die dokumentierte U+FFFD-Policy.

#### TerminalBackend

M2 führt außerdem `terminal::TerminalBackend : Backend` ein.

Der Backend-Verantwortungsbereich bleibt klein:

- `TerminalDevice` besitzen;
- `TerminalSession` initialisieren/beenden;
- `TerminalEventPump` besitzen;
- semantische Events über das vorhandene nichtblockierende `Backend::pollEvent()` liefern.

Initialisierung ist transaktional: Session und Pump werden zunächst in lokalen RAII-Objekten aufgebaut
und erst nach vollständigem Erfolg veröffentlicht.

Shutdown ist idempotent und beendet Decoder-/Timing-State vor der nativen TerminalSession.

Presentation wird **nicht** in das generische `Backend`-Interface gezogen. Sie bleibt:

`TerminalPresentationSink -> ScreenBuffer -> TerminalSession::present()`

#### Ausführbares Terminalformular

Bei `SASD_UI_BUILD_EXAMPLES=ON` (Default) wird `sasd_ui_terminal_demo` gebaut.

Das Beispiel nutzt den normalen Toolkitpfad mit `Application`, `TerminalBackend`, Fokus, Resize,
Layout und Presentation.

Enthalten sind:

- Label;
- editierbares TextField;
- Greet-Button;
- Status-Label;
- Exit-Button;
- Tab/Shift+Tab;
- Escape zum Beenden;
- Resize-Reaktion mit neuem ScreenBuffer/Layout.

Das Beispiel schläft im Idlepfad 8 ms. Das ist **nur Beispielpolitik** und kein Bestandteil von
`TerminalEventPump`, `TerminalBackend` oder `Application`. Ein späteres Wait/Wakeup-Modell kann das
ersetzen.

### Test- und Releasevalidierung

`TerminalEventPump` wird deterministisch mit `MockTerminalDevice` getestet, unter anderem für
Resize-Reihenfolge, ESC-Timeout, fortschreitende CSI-Sequenzen und abgebrochenes UTF-8.

`TerminalBackend` wird für Lifecycle, Byte-zu-Event-Übersetzung, Resize, Presentationzugriff und
doppelte Initialisierung getestet.

Die Demo wird in CI mit GCC, Clang, Sanitizern, AppleClang und MSVC kompiliert.

Hosted-CI-Kompilierung beweist jedoch **nicht** das Verhalten eines realen interaktiven Terminals.
Das M2-Exit-Kriterium benötigt weiterhin manuelle Smoke-Tests mindestens unter:

- Linux/xterm-artiger Umgebung;
- moderner Windows-Konsole bzw. Windows Terminal.

### Betrachtete Alternativen

#### Jetzt bereits blockierendes Application::run()

Verschoben. Wait/Wakeup, Timer und mehrere zukünftige Backendtypen sind noch nicht stabil genug für
einen universellen blockierenden Runloop-Vertrag.

#### Sleep in TerminalEventPump

Verworfen. Der Eventsource bleibt nichtblockierend und deterministisch; Idle-Scheduling ist
Aufruferpolitik.

#### TerminalBackend rendert automatisch

Verworfen. Das würde Lifecycle/Event und Presentation vermischen und das generische Backend-Interface
vor einem zweiten sichtbaren Backend vergrößern.

#### ResizeEvent direkt beim Start

Verworfen. Die Startgröße wird ohnehin für das initiale Layout benötigt; ResizeEvent beschreibt eine
Änderung.

### Konsequenzen

- das Terminalbackend nimmt jetzt am selben `Application`-Lifecycle-/Event-API wie das MockBackend
  teil;
- Terminalresize nutzt den bestehenden backendneutralen `ResizeEvent`;
- ESC-/Incomplete-Sequence-Timing ist deterministisch und liegt außerhalb des Decoders;
- das Repository enthält die erste buildbare reale M2-Terminalanwendung;
- vor Erfüllung des M2-Exit-Kriteriums bleiben manuelle Linux-/Windows-Smoke-Tests notwendig;
- einfache Styles und reichhaltigere Terminalprotokolle bleiben spätere M2-Arbeit.
