# ADR 0063: Headless Terminal Menu-Bar Rendering

- Status: Accepted
- Date: 2026-10-02

## Context

The terminal backend already has owned menu-presentation snapshots, deterministic cell measurement, and a headless popup renderer. The remaining top-level menu bar should use the same architectural boundary instead of being rendered directly by ANSI/VT or device/session code.

A menu-bar renderer must preserve the terminal backend's existing Unicode-cell invariants, remain testable on all CI platforms, and avoid mutating semantic menu state. It must also fail closed when text cannot be represented by the current simple `Cell` model.

## Decision

Add a dedicated header-only terminal renderer, `renderMenuBarPresentation()`, that consumes `MenuBarPresentationSnapshot`, validates the complete snapshot through `measureMenuBarPresentation()`, and then paints into `ScreenBuffer`.

Each top-level title uses the measurement contract already established for the terminal menu bar: one leading padding cell, the measured title, and one trailing padding cell. A valid selected index is represented with inverse video across that entire padded span. The renderer does not infer or store additional selection state.

Rendering remains clipped to `ScreenBuffer`. Wide glyphs are written through the same scalar-writing helper used by popup presentation, so a partially clipped two-cell glyph is omitted rather than represented by an orphaned lead or continuation cell.

Preflight happens before any buffer mutation. Unsupported multiline, control, combining, zero-width, or saturated text therefore leaves the previous frame untouched.

The renderer intentionally stops at `ScreenBuffer`. ANSI encoding, terminal sessions, event routing, pointer interaction, popup placement, and native menu integration remain separate concerns.

## Consequences

Terminal top-level menu bars and popup menus now share the same semantic-to-snapshot-to-measurement-to-cell-rendering architecture. This makes later composition of a complete terminal menu surface possible without coupling menu semantics to device I/O.

The initial implementation favors clarity and invariant preservation over avoiding repeated measurement. A later optimization pass may cache per-title measurements inside one render transaction if profiling shows this matters.

---

# ADR 0063: Headless-Rendering der Terminal-Menüleiste

- Status: Akzeptiert
- Datum: 2026-10-02

## Kontext

Das Terminal-Backend besitzt bereits besitzende Menü-Presentation-Snapshots, deterministische Zellvermessung und einen headless Popup-Renderer. Die Top-Level-Menüleiste soll dieselbe Architekturschnittstelle verwenden und nicht direkt in ANSI/VT- oder Geräte-/Session-Code gerendert werden.

Ein Menüleisten-Renderer muss die bestehenden Unicode-Zellinvarianten erhalten, auf allen CI-Plattformen testbar bleiben und den semantischen Menüzustand nicht verändern. Nicht sauber mit dem aktuellen einfachen `Cell`-Modell darstellbarer Text muss fail-closed behandelt werden.

## Entscheidung

Es wird ein eigener header-only Terminal-Renderer `renderMenuBarPresentation()` ergänzt. Er konsumiert `MenuBarPresentationSnapshot`, validiert zuerst den vollständigen Snapshot über `measureMenuBarPresentation()` und rendert danach in den `ScreenBuffer`.

Jeder Top-Level-Titel verwendet exakt den bereits festgelegten Messvertrag: eine führende Padding-Zelle, den vermessenen Titel und eine abschließende Padding-Zelle. Ein gültiger Auswahlindex wird über den kompletten gepaddeten Bereich mit inverser Darstellung markiert. Zusätzlicher Auswahlzustand wird weder abgeleitet noch gespeichert.

Das Rendering wird am `ScreenBuffer` geclippt. Breite Unicode-Zeichen werden über denselben Scalar-Writer wie beim Popup gerendert. Wird ein zweizelliges Zeichen teilweise abgeschnitten, wird es vollständig ausgelassen, statt eine verwaiste Lead- oder Continuation-Zelle zu erzeugen.

Die vollständige Vorprüfung findet vor jeder Buffer-Änderung statt. Nicht unterstützte Mehrzeiligkeit, Steuerzeichen, Combining-/Zero-Width-Semantik oder saturierte Messungen lassen daher den vorherigen Frame unverändert.

Die Schicht endet bewusst beim `ScreenBuffer`. ANSI-Encoding, Terminal-Session, Event-Routing, Pointer-Interaktion, Popup-Platzierung und native Menüs bleiben getrennte Verantwortlichkeiten.

## Konsequenzen

Top-Level-Menüleiste und Popup-Menüs verwenden im Terminal nun dieselbe Architektur von Semantik über Snapshot und Vermessung bis zum Zell-Rendering. Damit kann später eine vollständige Terminal-Menüoberfläche zusammengesetzt werden, ohne Menüsemantik an Geräte-I/O zu koppeln.

Die erste Implementierung priorisiert Verständlichkeit und Invarianten vor der Vermeidung wiederholter Messungen. Falls Profiling später einen relevanten Aufwand zeigt, können Titelbreiten innerhalb einer Render-Transaktion gecacht werden.
