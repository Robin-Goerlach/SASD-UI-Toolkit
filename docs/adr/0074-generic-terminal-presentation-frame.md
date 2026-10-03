# ADR 0074: Generalize the Owned Terminal Presentation Frame

- Status: Accepted
- Date: 2026-10-03

## Context

ADR 0073 introduced `MenuComposedPresentationFrame`, an owned pair of `ScreenBuffer` plus optional hardware-caret metadata returned by terminal menu composition. The payload solved an important presentation problem, but its type name accidentally tied a generic terminal-frame concept to one specific overlay producer.

The buffer/caret pair is not inherently a menu concept. `TerminalSession::present()` already consumes exactly those two pieces of presentation information, and future terminal layers such as dialogs, additional overlays, status surfaces, or other composition stages may need the same owned value without depending on menu-specific vocabulary.

Leaving the type under a menu-specific name would force unrelated terminal presentation code either to import menu composition terminology or to introduce another structurally identical frame wrapper later.

## Decision

Add `terminal/presentation_frame.hpp` with a backend-level `TerminalPresentationFrame` containing:

- an owned `ScreenBuffer`;
- an optional hardware-caret `Point`.

The type remains a passive value object. It retains no Widget, MenuModel, TerminalSession, native terminal handle, ANSI byte stream, or prior-frame history.

Change `composeMenuInteractionFrame()` to return `std::optional<TerminalPresentationFrame>` directly. Menu composition still owns the policy that decides whether the base caret is propagated or suppressed; only the returned value type becomes terminal-generic.

Retain `MenuComposedPresentationFrame` as a source-compatible type alias to `TerminalPresentationFrame`. The alias avoids needless churn for code written against ADR 0073 while making the generic name the canonical type for new code.

No transport behavior is added in this step. `TerminalSession` remains separate, and no overload is introduced merely because the new value happens to match its current `present(buffer, caret)` arguments. That bridge can be added independently if it later removes meaningful duplication.

## Consequences

The terminal presentation architecture now has one reusable owned frame contract instead of a menu-specific frame wrapper. Menu composition becomes one producer of `TerminalPresentationFrame` rather than the owner of the concept itself.

The change improves dependency direction: later terminal presentation or transport code can depend on `presentation_frame.hpp` without including menu semantics. Existing menu-composition callers using `MenuComposedPresentationFrame` continue to compile through the alias.

The frame still owns a full `ScreenBuffer`; no optimization, damage tracking, or retained frame history is introduced. The correctness-first composition policy from ADR 0072 and the caret suppression policy from ADR 0073 remain unchanged.

---

# ADR 0074: Eigenen Terminal-Presentation-Frame verallgemeinern

- Status: Akzeptiert
- Datum: 2026-10-03

## Kontext

ADR 0073 führte `MenuComposedPresentationFrame` ein, ein eigenes Paar aus `ScreenBuffer` und optionalen Hardware-Caret-Metadaten, das von der Terminal-Menükomposition zurückgegeben wird. Der Payload löste ein wichtiges Presentation-Problem, doch der Typname band ein allgemeines Terminal-Frame-Konzept versehentlich an einen bestimmten Overlay-Erzeuger.

Das Paar aus Buffer und Caret ist kein menüspezifisches Konzept. `TerminalSession::present()` konsumiert bereits genau diese beiden Presentation-Informationen, und spätere Terminal-Schichten wie Dialoge, weitere Overlays, Statusflächen oder andere Kompositionsstufen können denselben eigenen Wert benötigen, ohne von Menübegriffen abhängig zu sein.

Bliebe der Typ menüspezifisch benannt, müsste anderer Terminal-Presentation-Code entweder Menükompositionsbegriffe importieren oder später einen zweiten strukturell identischen Frame-Wrapper einführen.

## Entscheidung

`terminal/presentation_frame.hpp` wird mit einem backendweiten `TerminalPresentationFrame` eingeführt. Er enthält:

- einen eigenen `ScreenBuffer`;
- einen optionalen Hardware-Caret-`Point`.

Der Typ bleibt ein passives Value Object. Er hält weder Widget, MenuModel, TerminalSession, natives Terminal-Handle, ANSI-Bytestrom noch Historie früherer Frames fest.

`composeMenuInteractionFrame()` liefert künftig direkt `std::optional<TerminalPresentationFrame>`. Die Menükomposition besitzt weiterhin die Policy, ob der Basis-Caret weitergegeben oder unterdrückt wird; nur der zurückgegebene Werttyp wird terminal-allgemein.

`MenuComposedPresentationFrame` bleibt als source-kompatibler Typalias auf `TerminalPresentationFrame` erhalten. Dadurch entsteht für Code auf Basis von ADR 0073 kein unnötiger Anpassungszwang, während der allgemeine Name für neuen Code kanonisch wird.

In diesem Schritt wird bewusst kein Transportverhalten hinzugefügt. `TerminalSession` bleibt getrennt, und es wird nicht allein deshalb ein Overload eingeführt, weil der neue Wert zufällig den heutigen Argumenten von `present(buffer, caret)` entspricht. Eine solche Brücke kann später unabhängig ergänzt werden, wenn sie tatsächlich relevante Duplizierung beseitigt.

## Konsequenzen

Die Terminal-Presentation besitzt nun einen wiederverwendbaren eigenen Frame-Vertrag statt eines menüspezifischen Frame-Wrappers. Menükomposition wird zu einem Erzeuger von `TerminalPresentationFrame`, statt Eigentümer des Konzepts zu sein.

Die Abhängigkeitsrichtung wird sauberer: Spätere Terminal-Presentation- oder Transport-Schichten können von `presentation_frame.hpp` abhängen, ohne Menüsemantik einzubeziehen. Bestehende Menükompositions-Aufrufer mit `MenuComposedPresentationFrame` kompilieren über den Alias weiter.

Der Frame besitzt weiterhin einen vollständigen `ScreenBuffer`; Optimierung, Damage-Tracking oder retained Frame-Historie werden nicht eingeführt. Die korrektheitsorientierte Kompositions-Policy aus ADR 0072 und die Caret-Unterdrückungs-Policy aus ADR 0073 bleiben unverändert.
