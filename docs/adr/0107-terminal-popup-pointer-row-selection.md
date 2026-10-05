# ADR 0107 – Terminal popup pointer presses select semantic menu rows

**Status:** Accepted  
**Date:** 2026-10-05

## English

### Context

ADR 0105 introduced `TerminalMenuHitTest`, which maps terminal cells to the already-positioned menu presentation frame. ADR 0106 then introduced `TerminalMenuPointerInteraction`, establishing top-level title opening, modal pointer ownership, and outside-click dismissal. The terminal form demo now wires that interaction into the real event loop.

The next missing behavior is direct popup-row selection. A visible popup row already has a stable identity inside one presentation transaction: `(popup level, item index)`. However, terminal geometry must not mutate `MenuInteractionController` internals directly, and a pointer adapter must not retain `MenuModel*`/`MenuItem*` values across application-side menu mutation.

Selection also has structural consequences when nested submenus are open. If a pointer selects a different item in an ancestor popup, any descendant popup that belonged to the old parent item can no longer remain open. Conversely, re-selecting the exact submenu item that already owns the next popup should not destroy valid descendants merely because the same row was pressed again.

Finally, separators and disabled/expired command rows are still painted popup surface. They must consume the physical press to preserve modal no-click-through behavior, but they must not become semantic selections.

### Decision

Add a backend-neutral direct selection transaction to `MenuInteractionController`:

```cpp
MenuInteractionResult selectPopupItem(
    const MenuBarModel& bar,
    std::size_t level,
    std::size_t item_index);
```

`level == 0` names the root popup of the selected top-level menu. Higher levels follow the currently open `MenuPath`.

The method accepts only semantic value indices. It receives no pointer coordinates, terminal cell widths, presentation rectangles, backend handles, or borrowed menu pointers. This keeps terminal geometry outside Core while allowing pointer, accessibility, and later native adapters to request an exact semantic selection through one common state owner.

### Validation and fail-closed behavior

`selectPopupItem()` first normalizes the retained controller state against the supplied `MenuBarModel`, using the same conservative mutation-recovery policy as keyboard transactions.

The selection request is rejected without guessing when:

- menu interaction is inactive;
- no popup is open;
- the requested popup level is no longer open;
- the retained structural path cannot be proven;
- the item index is out of range;
- the item is a separator;
- the command is disabled or expired.

`MenuItem::isEnabled()` remains the single semantic selectability rule. The terminal pointer layer does not duplicate command/submenu/separator availability logic.

If normalization repairs stale controller state, `state_changed` may still be returned even when the requested identity itself is rejected. Otherwise a rejected selection returns `action == none`.

### Ancestor selection and nested popup coherence

When an enabled row is selected in an ancestor popup, the controller updates that level's selection.

If the selected row is not the submenu item that currently owns the next open popup, every deeper popup level is closed. The `MenuPath` is truncated to exactly the prefix needed to keep levels `[0, level]` open and the deeper selection values are discarded.

If the selected row is exactly the submenu item already stored in `MenuPath[level]`, existing child/descendant popups remain valid and are preserved. Re-selecting that row is therefore a semantic no-op when no other state changed.

This rule keeps the invariant that every open child popup is structurally owned by the selected submenu row above it.

### Terminal pointer behavior

`TerminalMenuPointerInteraction` now handles a Primary press on a visible popup row by:

1. using `TerminalMenuHitTest::popupItemAt()` to obtain presentation identity `(level, item_index)`;
2. passing that identity to `MenuInteractionController::selectPopupItem()`;
3. returning the controller result as an engaged `std::optional<MenuInteractionResult>`.

An engaged result continues to mean that the menu layer consumed the physical pointer event even when `action == none`.

Therefore separators, disabled rows, and stale presentation identities remain non-click-through menu surface while producing no illegal semantic selection.

### Selection is not activation

This slice deliberately defines Primary **press** as row selection only.

It does not:

- activate a command;
- open a newly selected submenu;
- close the complete menu after selecting a command row;
- synthesize press/release ownership;
- introduce hover-to-select behavior.

Keeping selection separate from activation preserves a clean place to design command activation on release and submenu opening as later transactions instead of making one pointer press perform several unrelated semantic actions.

### Ownership boundaries

Responsibilities are now:

- `MenuBarModel`: semantic menu structure and item availability;
- `MenuInteractionController`: all retained backend-neutral menu selection/path state, including direct popup selection;
- `buildMenuPresentationFrame()`: owned terminal presentation snapshot and popup placement;
- `TerminalMenuHitTest`: read-only terminal-cell to presentation identity mapping;
- `TerminalMenuPointerInteraction`: terminal pointer consumption and translation of a popup hit into the controller transaction;
- application host: chooses menu pointer interaction versus ordinary Widget routing;
- `renderMenuPresentationFrame()`: terminal painting.

No terminal-specific selection state is added to Core, and no menu-model pointer is retained by the terminal adapter.

### Consequences

- enabled popup rows can now be selected directly by Primary pointer press;
- command callbacks are not executed by selection;
- separator and disabled rows consume presses but do not become selected;
- selecting a different ancestor row closes structurally unrelated child popups;
- re-selecting the submenu row that owns an existing child preserves that valid child state;
- stale popup frame identities fail closed rather than selecting a guessed replacement;
- keyboard and pointer input continue to share one `MenuInteractionController` state model.

### Deferred scope

This ADR does not yet add:

- hover selection;
- automatic submenu opening after selecting a submenu row;
- command activation by pointer release/click;
- menu-specific pointer capture;
- drag-to-select menu gestures;
- delayed submenu timers;
- rendered/native desktop pointer adapters.

---

## Deutsch

### Kontext

ADR 0105 hat `TerminalMenuHitTest` eingeführt, das Terminalzellen auf den bereits positionierten Menü-Presentation-Frame abbildet. ADR 0106 hat darauf `TerminalMenuPointerInteraction` aufgebaut und damit das Öffnen von Top-Level-Titeln, modalen Pointer-Besitz sowie das Schließen per Außenklick festgelegt. Das Terminal-Form-Demo verwendet diese Interaktion inzwischen im realen Eventloop.

Als nächster Schritt fehlt die direkte Selektion einer Popup-Zeile. Eine sichtbare Popup-Zeile besitzt innerhalb einer Presentation-Transaktion bereits eine eindeutige Identität aus `(Popup-Level, Item-Index)`. Terminal-Geometrie darf aber nicht direkt interne Felder des `MenuInteractionController` verändern, und ein Pointer-Adapter darf keine `MenuModel*`-/`MenuItem*`-Zeiger über mögliche Änderungen der Anwendung hinweg behalten.

Bei geöffneten Untermenüs hat eine Auswahl außerdem strukturelle Folgen. Wird in einem übergeordneten Popup ein anderes Item gewählt, darf ein Kind-Popup, das zum vorherigen Parent-Item gehörte, nicht offen bleiben. Wird dagegen genau das Submenu-Item erneut selektiert, das bereits das nächste Popup besitzt, sollen gültige Kind-Popups nicht nur wegen desselben erneuten Press zerstört werden.

Separatoren sowie deaktivierte oder abgelaufene Command-Zeilen bleiben dennoch gezeichnete Popup-Fläche. Der physische Press muss dort konsumiert werden, um Click-Through zu verhindern, die Zeile darf aber nicht zur semantischen Auswahl werden.

### Entscheidung

Der backend-neutrale `MenuInteractionController` erhält eine direkte Selektions-Transaktion:

```cpp
MenuInteractionResult selectPopupItem(
    const MenuBarModel& bar,
    std::size_t level,
    std::size_t item_index);
```

`level == 0` bezeichnet das Root-Popup des ausgewählten Top-Level-Menüs. Höhere Ebenen folgen dem aktuell geöffneten `MenuPath`.

Die Methode erhält ausschließlich semantische Wert-Indizes. Pointer-Koordinaten, Terminalzellenbreiten, Presentation-Rechtecke, Backend-Handles oder geliehene Menüzeiger gelangen nicht in den Core. Terminal-Geometrie bleibt damit außerhalb des Controllers, während Pointer-, Accessibility- und spätere native Adapter dieselbe zentrale Zustandsinstanz für eine exakte semantische Selektion verwenden können.

### Validierung und Fail-Closed-Verhalten

`selectPopupItem()` normalisiert zunächst den gespeicherten Controller-Zustand gegen das übergebene `MenuBarModel` und verwendet damit dieselbe konservative Recovery-Policy wie Tastatur-Transaktionen.

Die Auswahl wird ohne Raten verworfen, wenn:

- die Menüinteraktion inaktiv ist;
- kein Popup geöffnet ist;
- das angeforderte Popup-Level nicht mehr offen ist;
- der gespeicherte Strukturpfad nicht mehr beweisbar ist;
- der Item-Index außerhalb des aktuellen Menüs liegt;
- das Item ein Separator ist;
- das Command deaktiviert oder nicht mehr vorhanden ist.

`MenuItem::isEnabled()` bleibt die einzige semantische Selectability-Regel. Die Terminal-Pointer-Schicht dupliziert keine Availability-Logik für Commands, Submenus oder Separatoren.

Repariert die Normalisierung veralteten Controller-Zustand, kann auch bei anschließend verworfener angeforderter Identität `state_changed` zurückgegeben werden. Ohne solche Reparatur ergibt eine ungültige Auswahl `action == none`.

### Auswahl in einem Parent-Popup und Konsistenz verschachtelter Popups

Wird eine aktivierte Zeile in einem übergeordneten Popup selektiert, aktualisiert der Controller die Auswahl dieser Ebene.

Ist die ausgewählte Zeile **nicht** das Submenu-Item, das aktuell das nächste geöffnete Popup besitzt, werden alle tieferen Popup-Ebenen geschlossen. Der `MenuPath` wird genau auf den Prefix gekürzt, der die Ebenen `[0, level]` offen hält; tiefere Selection-Werte werden verworfen.

Ist die ausgewählte Zeile dagegen genau das bereits in `MenuPath[level]` gespeicherte Submenu-Item, bleiben vorhandene gültige Kind-/Enkel-Popups erhalten. Eine erneute Auswahl derselben Parent-Zeile ist daher ein semantischer No-op, sofern sich sonst nichts geändert hat.

Damit bleibt die Invariante erhalten, dass jedes geöffnete Kind-Popup strukturell vom ausgewählten Submenu-Item der Ebene darüber getragen wird.

### Verhalten der Terminal-Pointer-Schicht

`TerminalMenuPointerInteraction` behandelt einen Primary-Press auf einer sichtbaren Popup-Zeile nun folgendermaßen:

1. `TerminalMenuHitTest::popupItemAt()` bestimmt die Presentation-Identität `(level, item_index)`;
2. diese Identität wird an `MenuInteractionController::selectPopupItem()` übergeben;
3. das Controller-Ergebnis wird als gesetztes `std::optional<MenuInteractionResult>` zurückgegeben.

Ein gesetztes Optional bedeutet weiterhin, dass die Menüschicht das physische Pointer-Event konsumiert hat – auch bei `action == none`.

Damit bleiben Separatoren, deaktivierte Zeilen und veraltete Presentation-Identitäten undurchlässige Menüoberfläche, ohne eine unzulässige semantische Auswahl zu erzeugen.

### Selektion ist noch keine Aktivierung

Dieser Slice definiert einen Primary-**Press** bewusst ausschließlich als Zeilenselektion.

Er führt noch nicht aus:

- Command-Aktivierung;
- automatisches Öffnen eines neu selektierten Submenus;
- Schließen des gesamten Menüs nach Auswahl eines Command-Items;
- eigene Press-/Release-Besitzlogik;
- Hover-Selektion.

Die Trennung hält einen sauberen Ort für spätere Command-Aktivierung beim Release und für Submenu-Öffnung offen, statt einem einzelnen Pointer-Press mehrere unabhängige semantische Bedeutungen zu geben.

### Ownership-Grenzen

Die Verantwortlichkeiten sind nun:

- `MenuBarModel`: semantische Menüstruktur und Availability der Items;
- `MenuInteractionController`: gesamter gespeicherter backend-neutraler Menüauswahl-/Pfadzustand einschließlich direkter Popup-Selektion;
- `buildMenuPresentationFrame()`: eigener Terminal-Presentation-Snapshot und Popup-Platzierung;
- `TerminalMenuHitTest`: rein lesende Abbildung von Terminalzellen auf Presentation-Identität;
- `TerminalMenuPointerInteraction`: Terminal-Pointer-Consumption und Übersetzung eines Popup-Treffers in die Controller-Transaktion;
- Application-Host: Entscheidung zwischen Menü-Pointer-Interaktion und normalem Widget-Routing;
- `renderMenuPresentationFrame()`: Terminaldarstellung.

Es wird kein terminal-spezifischer Selection-Zustand in Core ergänzt, und der Terminal-Adapter behält keine Menümodell-Zeiger.

### Folgen

- aktivierte Popup-Zeilen können per Primary-Press direkt selektiert werden;
- die Auswahl führt noch keinen Command-Callback aus;
- Separatoren und deaktivierte Zeilen konsumieren den Press, werden aber nicht ausgewählt;
- die Auswahl einer anderen Parent-Zeile schließt strukturell nicht mehr passende Kind-Popups;
- die erneute Auswahl des Submenu-Items, das ein vorhandenes Kind besitzt, erhält diesen gültigen Kindzustand;
- veraltete Popup-Frame-Identitäten schlagen kontrolliert fehl, statt einen Ersatz zu erraten;
- Tastatur und Pointer verwenden weiterhin dasselbe Zustandsmodell im `MenuInteractionController`.

### Zurückgestellter Umfang

Diese ADR ergänzt noch nicht:

- Hover-Selektion;
- automatisches Öffnen eines Submenus nach Auswahl seiner Zeile;
- Command-Aktivierung per Pointer-Release/Klick;
- menüspezifisches Pointer-Capture;
- Drag-to-select für Menüs;
- verzögerte Submenu-Timer;
- Pointer-Adapter für Rendered/Native Desktop.
