# ADR 0078: Integrate Semantic Menus into the Terminal Form Demo

- Status: Accepted
- Date: 2026-10-03

## Context

The terminal menu work is now available as a complete headless pipeline: semantic `Command` and menu models, `MenuInteractionController`, owned menu presentation snapshots, viewport-aware popup placement, transactional rendering, `TerminalPresentationFrame` capture/composition, and final `TerminalSession::present(frame)` transport.

Until now, the interactive terminal form demo still bypassed that pipeline. It presented the `TerminalPresentationSink` work buffer directly and used F10 as an application-exit key. This meant the individual menu contracts were well tested, but the normal sample application did not yet demonstrate how semantic commands, widget focus, menu keyboard ownership, frame composition, and transport are intended to fit together.

The demo is particularly useful as an architectural integration point because it already contains a focused `TextField`, Buttons, RadioButtons, a CheckBox, resize handling, and real terminal input/output. Adding menus there can validate the intended separation without adding menu policy to `Window`, `Application`, `TerminalBackend`, or the widget hierarchy.

## Decision

Integrate the existing semantic menu pipeline into `examples/terminal_form_demo.cpp`.

The demo now:

- reserves terminal row zero for a persistent menu bar while keeping that layout decision explicit in sample code;
- defines shared semantic `Command` objects for Greet, Exit, and Help;
- binds the Greet and Exit Buttons to the same Commands used by menu items;
- uses F10 only as application policy to enter or leave menu interaction;
- prevents focused Widgets from receiving keyboard/text input while the menu controller is active;
- lets `MenuInteractionController` handle menu navigation and return lifetime-safe command references;
- captures the current application presentation through `TerminalPresentationSink::captureFrame()`;
- composes the current menu interaction over that owned base frame;
- presents the resulting `TerminalPresentationFrame` through `TerminalSession::present(frame)`;
- delays menu-command execution until after the controller has closed and the closed menu state has been presented.

The demo does not move menu semantics into `Window` or `Application`. It also does not make F10 part of `MenuInteractionController`; global activation remains application policy. Likewise, no menu-specific state is retained by `TerminalPresentationSink` or `TerminalSession`.

While menu interaction is active, the event target selector deliberately returns no focused Widget for keyboard and text input. This prevents a focused `TextField` or Button from consuming navigation keys before the menu controller. The semantic focus itself is not changed; menu composition merely suppresses the hardware caret while the menu owns keyboard attention, as established by ADR 0073.

Command activation follows the two-phase `MenuInteractionController` contract. The controller first closes transient menu state and returns `Command::Reference`. The demo then presents that closed state before calling `Command::execute()`. This makes it safe for command callbacks to mutate widgets, rebuild semantic state, or request application exit without running while stale popup state is still visually active.

## Consequences

The interactive terminal demo now exercises the intended M4 menu architecture end to end:

`Widgets -> TerminalPresentationSink -> captureFrame() -> menu composition -> TerminalPresentationFrame -> TerminalSession`

and, independently:

`terminal KeyEvent -> application activation policy -> MenuInteractionController -> Command::Reference -> Command::execute()`

The same Greet and Exit semantics can be invoked from either Buttons or menu items without duplicating callbacks. This demonstrates the purpose of backend-neutral Commands and keeps presentation-specific menu chrome outside the widget tree.

The demo still uses correctness-first full-frame capture/composition. It does not introduce damage tracking, buffer reuse, native menus, pointer menu interaction, mnemonics, or localization. Those remain separate future concerns.

---

# ADR 0078: Semantische Menüs in das Terminal-Formular-Demo integrieren

- Status: Akzeptiert
- Datum: 2026-10-03

## Kontext

Die Terminal-Menüentwicklung steht inzwischen als vollständige headless Pipeline zur Verfügung: semantische `Command`- und Menümodelle, `MenuInteractionController`, eigene Menu-Presentation-Snapshots, viewport-abhängiges Popup-Placement, transaktionales Rendering, Capture/Composition über `TerminalPresentationFrame` und schließlich der Transport über `TerminalSession::present(frame)`.

Das interaktive Terminal-Formular-Demo umging diese Pipeline bisher noch. Es übergab den Arbeitsbuffer des `TerminalPresentationSink` direkt an die Session und verwendete F10 als Taste zum Beenden der Anwendung. Damit waren die einzelnen Menüverträge zwar gut getestet, die normale Beispielanwendung zeigte aber noch nicht, wie semantische Commands, Widget-Fokus, Keyboard-Ownership des Menüs, Frame-Komposition und Transport zusammenspielen sollen.

Das Demo eignet sich besonders gut als architektonischer Integrationspunkt, weil es bereits ein fokussiertes `TextField`, Buttons, RadioButtons, eine CheckBox, Resize-Behandlung und echtes Terminal-I/O enthält. Menüs können dort die beabsichtigte Trennung validieren, ohne Menü-Policy in `Window`, `Application`, `TerminalBackend` oder die Widget-Hierarchie einzubauen.

## Entscheidung

Die bestehende semantische Menü-Pipeline wird in `examples/terminal_form_demo.cpp` integriert.

Das Demo:

- reserviert Terminal-Zeile null für eine persistente Menüleiste, wobei diese Layout-Entscheidung ausdrücklich im Beispielcode bleibt;
- definiert gemeinsame semantische `Command`-Objekte für Greet, Exit und Help;
- bindet die Greet- und Exit-Buttons an dieselben Commands, die auch von Menüeinträgen verwendet werden;
- verwendet F10 ausschließlich als Anwendungspolicy zum Aktivieren oder Verlassen der Menüinteraktion;
- verhindert, dass fokussierte Widgets Keyboard-/Text-Input erhalten, solange der Menücontroller aktiv ist;
- lässt `MenuInteractionController` die Menünavigation verarbeiten und lifetime-sichere Command-Referenzen zurückgeben;
- erfasst die aktuelle Anwendungsdarstellung über `TerminalPresentationSink::captureFrame()`;
- komponiert die aktuelle Menüinteraktion über diesen eigenen Basisframe;
- übergibt den resultierenden `TerminalPresentationFrame` an `TerminalSession::present(frame)`;
- verzögert die Ausführung eines Menü-Commands, bis der Controller geschlossen und dieser geschlossene Menüzustand dargestellt wurde.

Die Menüsemantik wird nicht in `Window` oder `Application` verschoben. F10 wird auch nicht Bestandteil des `MenuInteractionController`; die globale Aktivierung bleibt Anwendungspolicy. Ebenso halten weder `TerminalPresentationSink` noch `TerminalSession` menüspezifischen Zustand.

Solange die Menüinteraktion aktiv ist, liefert die Event-Zielauswahl für Keyboard- und Texteingaben bewusst kein fokussiertes Widget. Dadurch kann ein fokussiertes `TextField` oder ein Button keine Navigations-Taste vor dem Menücontroller konsumieren. Der semantische Fokus selbst wird nicht verändert; die Menükomposition unterdrückt lediglich den Hardware-Caret, solange das Menü die Keyboard-Aufmerksamkeit besitzt, entsprechend ADR 0073.

Die Command-Aktivierung folgt dem zweiphasigen Vertrag von `MenuInteractionController`. Der Controller schließt zuerst den transienten Menüzustand und liefert anschließend eine `Command::Reference`. Das Demo stellt diesen geschlossenen Zustand dar, bevor `Command::execute()` aufgerufen wird. Damit können Command-Callbacks Widgets verändern, semantischen Zustand neu aufbauen oder das Anwendungsende anfordern, ohne während noch sichtbarer veralteter Popup-Zustände zu laufen.

## Konsequenzen

Das interaktive Terminal-Demo verwendet nun die beabsichtigte M4-Menüarchitektur durchgehend:

`Widgets -> TerminalPresentationSink -> captureFrame() -> Menükomposition -> TerminalPresentationFrame -> TerminalSession`

und unabhängig davon:

`Terminal-KeyEvent -> Aktivierungspolicy der Anwendung -> MenuInteractionController -> Command::Reference -> Command::execute()`

Dieselben Greet- und Exit-Semantiken können damit über Buttons oder Menüeinträge ausgelöst werden, ohne Callback-Logik zu duplizieren. Das demonstriert den Zweck backend-neutraler Commands und hält präsentationsspezifische Menü-Chrome außerhalb des Widget-Baums.

Das Demo verwendet weiterhin bewusst correctness-first Full-Frame-Capture und -Composition. Damage-Tracking, Buffer-Reuse, native Menüs, Pointer-Menüinteraktion, Mnemonics und Lokalisierung werden dadurch nicht vorweggenommen und bleiben getrennte spätere Aufgaben.
