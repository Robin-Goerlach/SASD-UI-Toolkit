# ADR 0075: TerminalSession Accepts Complete Terminal Presentation Frames

- Status: Accepted
- Date: 2026-10-03

## Context

ADR 0074 introduced the backend-wide `TerminalPresentationFrame` value containing the complete terminal presentation payload currently needed at the transport boundary: an owned `ScreenBuffer` and an optional hardware-caret position.

`TerminalSession` already exposes `present(const ScreenBuffer&, std::optional<Point>)` and therefore consumes exactly the two pieces of information carried by `TerminalPresentationFrame`. Without a frame overload, every caller that has already composed a complete frame must unpack that value manually before transport. This is small boilerplate, but it weakens the purpose of the new frame type and encourages call sites to treat buffer and caret as unrelated values again.

At the same time, transport behavior must not split into two independent implementations. Active-session validation, ANSI encoding, write-failure semantics, retry behavior, and later transport fixes should remain identical regardless of whether a caller starts with separate arguments or a complete frame.

## Decision

Add `TerminalSession::present(const TerminalPresentationFrame&)`.

The new overload is a thin transport adapter only. It delegates directly to the established primitive overload:

```cpp
present(frame.buffer, frame.caret);
```

It does not perform menu composition, focus handling, widget synchronization, event-loop work, or additional validation of its own.

The existing `present(const ScreenBuffer&, std::optional<Point>)` overload remains public and is the single implementation of active-session checks, ANSI encoding, and device writes. Simple callers therefore remain supported, while higher presentation layers can cross the transport boundary using the complete frame value without unpacking it.

`terminal_session.hpp` forward-declares `TerminalPresentationFrame` so the public session header does not need to include the complete frame definition merely to declare a const-reference overload. The implementation includes `presentation_frame.hpp` where member access is required.

## Consequences

The presentation pipeline now has a direct value-preserving path from composition to transport:

`TerminalPresentationFrame -> TerminalSession::present(frame) -> existing buffer/caret transport primitive`.

There is still exactly one byte-emission path. A closed session, an encoding failure, or a device write failure therefore behaves the same for both overloads.

The overload is convenience at an architectural boundary, not a new abstraction layer. It intentionally adds no ownership or retained-frame state to `TerminalSession`.

---

# ADR 0075: TerminalSession akzeptiert vollständige Terminal-Presentation-Frames

- Status: Akzeptiert
- Datum: 2026-10-03

## Kontext

ADR 0074 führte den backendweiten Wert `TerminalPresentationFrame` ein. Er enthält die vollständigen Terminal-Presentation-Daten, die an der derzeitigen Transportgrenze benötigt werden: einen eigenen `ScreenBuffer` und eine optionale Hardware-Caret-Position.

`TerminalSession` stellt bereits `present(const ScreenBuffer&, std::optional<Point>)` bereit und konsumiert damit exakt die beiden Informationen, die `TerminalPresentationFrame` gemeinsam trägt. Ohne einen Frame-Overload müsste jeder Aufrufer, der bereits einen vollständigen Frame zusammengesetzt hat, diesen Wert vor dem Transport wieder manuell zerlegen. Das ist zwar nur wenig Boilerplate, schwächt aber den Zweck des neuen Frame-Typs und lädt dazu ein, Buffer und Caret erneut als voneinander unabhängige Werte zu behandeln.

Gleichzeitig darf sich das Transportverhalten nicht in zwei unabhängige Implementierungen aufteilen. Prüfung des aktiven Sessionszustands, ANSI-Encoding, Verhalten bei Schreibfehlern, Retry-Semantik und spätere Transportkorrekturen sollen unabhängig davon identisch bleiben, ob ein Aufrufer mit getrennten Argumenten oder mit einem vollständigen Frame startet.

## Entscheidung

`TerminalSession::present(const TerminalPresentationFrame&)` wird ergänzt.

Der neue Overload ist ausschließlich ein dünner Transportadapter. Er delegiert direkt an den bestehenden primitiven Overload:

```cpp
present(frame.buffer, frame.caret);
```

Er führt weder Menü-Komposition noch Fokusbehandlung, Widget-Synchronisierung, Event-Loop-Arbeit oder zusätzliche eigene Validierung aus.

Der vorhandene Overload `present(const ScreenBuffer&, std::optional<Point>)` bleibt öffentlich und ist weiterhin die einzige Implementierung für Active-Session-Prüfung, ANSI-Encoding und Device-Schreibvorgang. Einfache Aufrufer bleiben damit unterstützt, während höhere Presentation-Schichten den vollständigen Frame-Wert ohne erneutes Zerlegen über die Transportgrenze geben können.

`terminal_session.hpp` deklariert `TerminalPresentationFrame` vorwärts, sodass der öffentliche Session-Header die vollständige Frame-Definition nicht nur für einen const-Reference-Overload einbinden muss. Die Implementierung bindet `presentation_frame.hpp` dort ein, wo tatsächlich auf die Member zugegriffen wird.

## Konsequenzen

Die Presentation-Pipeline besitzt jetzt einen direkten werterhaltenden Weg von der Komposition bis zum Transport:

`TerminalPresentationFrame -> TerminalSession::present(frame) -> bestehendes Buffer/Caret-Transportprimitiv`.

Es gibt weiterhin genau einen Pfad für die Byte-Ausgabe. Eine geschlossene Session, ein Encoding-Fehler oder ein Device-Schreibfehler verhalten sich deshalb bei beiden Overloads identisch.

Der Overload ist eine Komfortfunktion an einer Architekturgrenze und keine neue Abstraktionsschicht. `TerminalSession` erhält dadurch bewusst weder Ownership über vergangene Frames noch retained Frame-State.
