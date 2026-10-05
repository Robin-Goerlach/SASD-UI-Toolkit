# ADR 0106 – Terminal menu pointer interaction opens top-level menus and owns outside dismissal

**Status:** Accepted  
**Date:** 2026-10-05

## English

### Context

ADR 0105 introduced `TerminalMenuHitTest`, which maps terminal cells to the already-positioned `MenuFramePresentationSnapshot` used for menu rendering. That solved terminal menu geometry without duplicating popup placement, viewport fitting, Unicode-width rules, or submenu left/right flipping.

The next missing layer is semantic pointer interaction. The terminal demo already receives backend-neutral `PointerEvent` values, but an open terminal menu still behaves as a keyboard-only overlay. Simply forwarding menu pointer events to the Widget tree would be incorrect because transient menu chrome is not part of that tree and may visibly cover Buttons, TextFields, and other controls underneath it.

This first semantic slice must therefore establish two invariants before hover, popup selection, submenu opening, and command activation are added:

1. a primary press on a top-level menu title opens that title's root popup;
2. while a menu is active, pointer input owned by the menu surface must not click through to underlying application Widgets, including the press that dismisses the menu from outside.

### Decision

Add terminal-only `TerminalMenuPointerInteraction`.

The helper receives:

- the current `MenuBarModel`;
- the mutable backend-neutral `MenuInteractionController`;
- the already-built `MenuFramePresentationSnapshot` that represents visible terminal menu geometry;
- one backend-neutral `PointerEvent`;
- the terminal `AmbiguousWidthMode` used by the presentation frame.

It returns `std::optional<MenuInteractionResult>`.

The optional itself is the pointer-consumption boundary:

- `std::nullopt` means the menu layer did not consume the event and the host may continue normal application pointer routing;
- an engaged result means the event belongs to the transient menu scope, even when `action == MenuInteractionAction::none`;
- `state_changed` and `closed` reuse the controller's existing semantic action vocabulary and allow the host to decide whether menu presentation must be rebuilt.

No second terminal-specific interaction action enum is introduced.

### Top-level title press

An exact Primary `PointerAction::press` first checks active popup geometry, then menu-bar geometry.

Popup geometry has precedence because `MenuFramePresentationSnapshot` paints popup layers after the menu bar. Even if a synthetic or future placement overlaps the menu-bar row, the visually topmost popup row owns the pointer cell.

If no popup row owns the cell and `TerminalMenuHitTest::menuBarIndexAt()` identifies a valid top-level title, the interaction layer:

1. starts a fresh controller transaction with `MenuInteractionController::begin(bar, title_index)`;
2. invokes the controller's existing unmodified Enter transition;
3. returns the resulting `MenuInteractionResult` as consumed.

The Enter transition is intentionally reused because it is already the canonical semantic operation for "open the selected top-level menu without preselecting a popup item". This keeps keyboard and pointer entry behavior identical and avoids direct access to controller internals.

Changing top-level title through the pointer therefore clears any old submenu path and old popup selection exactly as a fresh controller transaction should. Indices from one top-level `MenuModel` are never carried into another one.

### Popup rows in this slice

A Primary press on an already-visible popup row is consumed but does not yet mutate semantic state.

This is deliberate staging rather than incomplete geometry. ADR 0105 already identifies the exact popup level and row. Selection, submenu opening, disabled/separator policy, command activation, and press/release gesture rules require their own semantic decisions and remain later slices.

Consuming the event now establishes the correct modal boundary and prevents click-through while those semantics are still deferred.

### Outside-click dismissal

If menu interaction is active and a Primary press hits neither a popup row nor a menu-bar title, `TerminalMenuPointerInteraction` resets the controller and returns an engaged `MenuInteractionResult` with `action == closed`.

The event remains consumed after dismissal.

This no-click-through rule is essential: one physical press must not both close a menu and activate whichever application control happens to occupy the same terminal cell underneath the transient overlay.

When menu interaction is inactive, an outside Primary press returns `std::nullopt` and remains available for normal application pointer routing.

### Other pointer transitions while active

This first slice does not yet interpret hover/motion or pointer release for menu semantics.

However, while the controller is active, non-Primary-press pointer events return an engaged no-op result. The menu therefore forms a transient modal pointer scope even before hover and release behavior are implemented.

This prevents a release following a title press, or incidental motion over an open popup, from reaching Widgets below the overlay merely because the current menu layer does not yet assign those transitions another meaning.

### Stale-frame handling

A `MenuFramePresentationSnapshot` is normally built immediately from the same `MenuBarModel` and controller state that receive the pointer event. Application code can nevertheless mutate semantic menu structure between frame construction and input handling.

`MenuInteractionController::begin()` intentionally treats an invalid preferred index as "select the first menu". That is useful for normal controller entry but unsafe for a stale pointer identity: an old title index must never silently become a click on menu zero.

Therefore the pointer interaction validates the hit title index against the current `MenuBarModel` before calling `begin()`. An unprovable stale title hit is consumed without semantic mutation rather than guessed.

As elsewhere in the current menu architecture, rebuilding the bar with different semantics at the same still-valid index cannot be distinguished without introducing stable menu identities; this slice does not invent them.

### Ownership boundaries

Responsibilities remain:

- `MenuBarModel`: semantic menu structure;
- `MenuInteractionController`: backend-neutral transient menu state and keyboard semantic transitions;
- `buildMenuPresentationFrame()`: owned terminal menu snapshot plus concrete popup placement;
- `TerminalMenuHitTest`: read-only terminal-cell to presentation-identity mapping;
- `TerminalMenuPointerInteraction`: terminal pointer consumption plus the small currently-defined semantic transitions;
- application host: decides when to invoke menu interaction versus ordinary Widget pointer routing;
- `renderMenuPresentationFrame()`: terminal-cell painting.

`TerminalMenuPointerInteraction` owns no persistent state, no `MenuModel*`, no `MenuItem*`, no Widget pointer, and no terminal session/device object.

### Consequences

- terminal menu titles can now be opened by Primary pointer press;
- switching titles by pointer starts a fresh root-popup transaction and discards stale nested state;
- visible popup rows already block pointer click-through even before row activation semantics are implemented;
- outside Primary press dismisses an active menu without also pressing an underlying application control;
- non-press pointer transitions stay inside the active menu's modal scope;
- keyboard and pointer top-level opening share one controller semantic transition;
- terminal geometry remains outside Core menu state.

### Deferred scope

This ADR does not yet add:

- wiring `TerminalMenuPointerInteraction` into the terminal form demo;
- popup-row hover selection;
- popup-row press/release selection;
- submenu opening by hover or click;
- command activation by pointer;
- disabled/separator semantic policy beyond consumption;
- pointer capture specifically for menus;
- delayed submenu auto-open timers;
- drag-to-select menu gestures;
- rendered/native desktop menu pointer adapters.

---

## Deutsch

### Kontext

ADR 0105 hat `TerminalMenuHitTest` eingeführt. Dieser ordnet Terminalzellen dem bereits positionierten `MenuFramePresentationSnapshot` zu, der auch für die Menüdarstellung verwendet wird. Damit ist die Terminal-Menügeometrie gelöst, ohne Popup-Platzierung, Viewport-Fitting, Unicode-Breitenregeln oder das Links-/Rechts-Umklappen von Untermenüs zu duplizieren.

Die nächste fehlende Schicht ist die semantische Pointer-Interaktion. Das Terminal-Demo erhält bereits backend-neutrale `PointerEvent`-Werte, ein geöffnetes Terminalmenü verhält sich aber weiterhin wie ein rein tastaturgesteuertes Overlay. Menü-Pointer-Events einfach an den Widget-Baum weiterzugeben wäre falsch, weil das transiente Menü-Chrome nicht Teil dieses Baums ist und sichtbar Buttons, TextFields und andere Controls darunter überdecken kann.

Dieser erste semantische Slice muss deshalb zunächst zwei Invarianten herstellen, bevor Hover, Popup-Selektion, Untermenüs und Command-Aktivierung ergänzt werden:

1. ein Primary-Press auf einen Top-Level-Menütitel öffnet dessen Root-Popup;
2. solange ein Menü aktiv ist, darf Pointer-Eingabe, die zur Menüoberfläche gehört, nicht zu darunterliegenden Application-Widgets durchklicken – einschließlich des Außenklicks, der das Menü schließt.

### Entscheidung

Wir ergänzen den terminal-spezifischen `TerminalMenuPointerInteraction`.

Der Helper erhält:

- das aktuelle `MenuBarModel`;
- den veränderbaren backend-neutralen `MenuInteractionController`;
- den bereits aufgebauten `MenuFramePresentationSnapshot`, der die sichtbare Terminal-Menügeometrie repräsentiert;
- ein backend-neutrales `PointerEvent`;
- den für den Presentation-Frame verwendeten Terminal-`AmbiguousWidthMode`.

Zurückgegeben wird `std::optional<MenuInteractionResult>`.

Das Optional selbst bildet die Pointer-Consumption-Grenze:

- `std::nullopt` bedeutet, dass die Menüschicht das Event nicht konsumiert hat und der Host mit normalem Application-Pointer-Routing fortfahren darf;
- ein gesetztes Ergebnis bedeutet, dass das Event zum transienten Menü-Scope gehört – auch bei `action == MenuInteractionAction::none`;
- `state_changed` und `closed` verwenden die bereits vorhandene semantische Aktionssprache des Controllers, sodass der Host entscheiden kann, ob die Menüpräsentation neu aufgebaut werden muss.

Es wird kein zweites terminal-spezifisches Interaction-Action-Enum eingeführt.

### Primary-Press auf einen Top-Level-Titel

Ein exakter Primary-`PointerAction::press` prüft zuerst aktive Popup-Geometrie und anschließend die Menüleisten-Geometrie.

Popup-Geometrie hat Vorrang, weil `MenuFramePresentationSnapshot` Popup-Ebenen nach der Menüleiste zeichnet. Selbst wenn ein synthetisches oder späteres Placement die Menüleistenzeile überlappt, besitzt daher die sichtbar oberste Popup-Zeile die Pointer-Zelle.

Wenn keine Popup-Zeile die Zelle besitzt und `TerminalMenuHitTest::menuBarIndexAt()` einen gültigen Top-Level-Titel erkennt, führt die Interaktionsschicht folgende Schritte aus:

1. Start einer frischen Controller-Transaktion mit `MenuInteractionController::begin(bar, title_index)`;
2. Aufruf der bereits vorhandenen unmodifizierten Enter-Transition des Controllers;
3. Rückgabe des resultierenden `MenuInteractionResult` als konsumiertes Event.

Die Enter-Transition wird bewusst wiederverwendet, weil sie bereits die kanonische semantische Operation für „ausgewähltes Top-Level-Menü öffnen, ohne eine Popup-Zeile vorzuselektieren“ ist. Tastatur- und Pointer-Einstieg bleiben dadurch identisch, ohne auf Controller-Interna zuzugreifen.

Ein Wechsel des Top-Level-Titels per Pointer löscht somit alte Untermenüpfade und alte Popup-Selektionen genau so, wie es eine frische Controller-Transaktion verlangt. Indizes eines Top-Level-`MenuModel` werden niemals in ein anderes Menü übertragen.

### Popup-Zeilen in diesem Slice

Ein Primary-Press auf eine bereits sichtbare Popup-Zeile wird konsumiert, verändert den semantischen Zustand aber noch nicht.

Das ist eine bewusste schrittweise Entwicklung und keine fehlende Geometrie. ADR 0105 identifiziert bereits exakt Popup-Level und Zeile. Selektion, Untermenüöffnung, Policy für deaktivierte Einträge/Separatoren, Command-Aktivierung sowie Press-/Release-Gestenregeln benötigen eigene semantische Entscheidungen und bleiben späteren Slices vorbehalten.

Das Event bereits jetzt zu konsumieren etabliert die korrekte modale Grenze und verhindert Click-Through, solange diese Semantik noch zurückgestellt ist.

### Schließen durch Außenklick

Ist die Menüinteraktion aktiv und trifft ein Primary-Press weder eine Popup-Zeile noch einen Menüleisten-Titel, setzt `TerminalMenuPointerInteraction` den Controller zurück und liefert ein gesetztes `MenuInteractionResult` mit `action == closed`.

Das Event bleibt auch nach dem Schließen konsumiert.

Diese No-Click-Through-Regel ist wesentlich: Ein physischer Press darf nicht gleichzeitig ein Menü schließen und das Application-Control aktivieren, das zufällig in derselben Terminalzelle unter dem transienten Overlay liegt.

Ist die Menüinteraktion inaktiv, liefert ein Außen-Primary-Press `std::nullopt` und bleibt für normales Application-Pointer-Routing verfügbar.

### Andere Pointer-Transitionen bei aktivem Menü

Dieser erste Slice interpretiert Hover/Motion und Pointer-Release noch nicht semantisch für Menüs.

Solange der Controller aktiv ist, liefern Nicht-Primary-Press-Pointer-Events dennoch ein gesetztes No-op-Ergebnis. Das Menü bildet dadurch bereits jetzt einen transienten modalen Pointer-Scope.

So kann weder das Release nach einem Titel-Press noch zufällige Bewegung über einem geöffneten Popup zu Widgets unter dem Overlay gelangen, nur weil die aktuelle Menüschicht diesen Transitionen noch keine weitere Bedeutung gibt.

### Umgang mit veralteten Frames

Ein `MenuFramePresentationSnapshot` wird normalerweise unmittelbar aus demselben `MenuBarModel` und Controller-Zustand aufgebaut, die anschließend das Pointer-Event erhalten. Anwendungscode kann die semantische Menüstruktur theoretisch dennoch zwischen Frame-Erzeugung und Input-Verarbeitung verändern.

`MenuInteractionController::begin()` interpretiert einen ungültigen bevorzugten Index bewusst als „erstes Menü auswählen“. Für normalen Controller-Einstieg ist das sinnvoll, für eine veraltete Pointer-Identität aber gefährlich: Ein alter Titelindex darf niemals stillschweigend zu einem Klick auf Menü null werden.

Darum prüft die Pointer-Interaktion den getroffenen Titelindex gegen das aktuelle `MenuBarModel`, bevor `begin()` aufgerufen wird. Ein nicht mehr beweisbarer veralteter Titel-Treffer wird konsumiert, ohne semantischen Zustand zu erraten.

Wie in der bestehenden Menüarchitektur kann ein Neuaufbau der Leiste mit anderer Semantik am selben weiterhin gültigen Index ohne stabile Menüidentitäten nicht unterschieden werden; dieser Slice führt solche Identitäten nicht ein.

### Ownership-Grenzen

Die Verantwortlichkeiten bleiben:

- `MenuBarModel`: semantische Menüstruktur;
- `MenuInteractionController`: backend-neutraler transienter Menüzustand und semantische Tastatur-Transitionen;
- `buildMenuPresentationFrame()`: eigener Terminal-Menü-Snapshot plus konkrete Popup-Platzierung;
- `TerminalMenuHitTest`: rein lesende Abbildung von Terminalzelle auf Präsentationsidentität;
- `TerminalMenuPointerInteraction`: Terminal-Pointer-Consumption plus die aktuell definierten kleinen semantischen Transitionen;
- Application-Host: entscheidet zwischen Menüinteraktion und normalem Widget-Pointer-Routing;
- `renderMenuPresentationFrame()`: Zeichnen der Terminalzellen.

`TerminalMenuPointerInteraction` besitzt keinen persistenten Zustand, keinen `MenuModel*`, keinen `MenuItem*`, keinen Widget-Zeiger und kein Terminal-Session-/Device-Objekt.

### Folgen

- Terminal-Menütitel können nun per Primary-Press geöffnet werden;
- Wechsel des Titels per Pointer startet eine frische Root-Popup-Transaktion und verwirft alten verschachtelten Zustand;
- sichtbare Popup-Zeilen blockieren bereits jetzt Pointer-Click-Through, obwohl ihre Aktivierungssemantik noch nicht implementiert ist;
- ein Primary-Außenklick schließt ein aktives Menü, ohne zusätzlich ein darunterliegendes Application-Control zu drücken;
- Nicht-Press-Pointer-Transitionen bleiben innerhalb des modalen aktiven Menü-Scope;
- Tastatur- und Pointer-Öffnung eines Top-Level-Menüs teilen dieselbe Controller-Semantik;
- Terminal-Geometrie bleibt außerhalb des Core-Menüzustands.

### Zurückgestellter Umfang

Diese ADR ergänzt noch nicht:

- Integration von `TerminalMenuPointerInteraction` in das Terminal-Form-Demo;
- Hover-Selektion von Popup-Zeilen;
- Popup-Zeilen-Selektion per Press/Release;
- Öffnen von Untermenüs per Hover oder Klick;
- Command-Aktivierung per Pointer;
- semantische Policy für deaktivierte Einträge/Separatoren über reines Consumption hinaus;
- eigenes Pointer-Capture für Menüs;
- verzögerte Auto-Open-Timer für Untermenüs;
- Drag-to-select-Menügesten;
- Pointer-Adapter für gerenderte/native Desktop-Menüs.
