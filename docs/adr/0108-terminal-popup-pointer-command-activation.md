# ADR 0108 – Terminal popup Commands activate on matching pointer release

**Status:** Accepted  
**Date:** 2026-10-05

## English

### Context

ADR 0107 added direct popup-row selection from terminal pointer geometry. A Primary press can now identify a painted popup row through `TerminalMenuHitTest` and ask the backend-neutral `MenuInteractionController` to select that exact semantic item. Separators and unavailable Commands remain consumed but unselected, and changing an ancestor selection keeps nested popup state structurally coherent.

The next missing behavior is Command activation. Activating on press would be easy, but it would collapse two distinct interaction stages into one and would differ from the toolkit's existing pointer controls: a press establishes intent/state, while a matching release completes activation. It would also make it impossible to cancel by releasing on a different row.

The menu controller already has an important two-phase Command contract for keyboard activation: it captures a lifetime-safe `Command::Reference`, closes transient menu state, returns `activate_command`, and leaves actual callback execution to the host after the closed menu has been presented. Pointer activation should preserve exactly that lifecycle.

At the same time, the terminal interaction layer must not retain `MenuModel*`, `MenuItem*`, `Command*`, or a presentation-frame pointer between press and release. Menu structure can be rebuilt, Commands can become disabled or disappear, and every backend-facing identity must therefore be revalidated against current semantic state.

### Decision

Add two cooperating pieces:

1. backend-neutral `MenuInteractionController::activatePopupItem(bar, level, item_index)`;
2. opt-in host-owned `TerminalMenuPointerInteraction::GestureState` plus a stateful `handle()` overload.

The existing stateless `TerminalMenuPointerInteraction::handle()` remains source- and behavior-compatible: popup press may select a row, but a later release cannot activate a Command because no press identity is retained.

### Backend-neutral activation transaction

`activatePopupItem()` accepts only semantic value indices:

```cpp
MenuInteractionResult activatePopupItem(
    const MenuBarModel& bar,
    std::size_t level,
    std::size_t item_index);
```

No terminal coordinate, pixel position, native handle, or presentation object enters Core.

Before activation the controller normalizes its retained state against the supplied current `MenuBarModel`. Activation is intentionally commit-like: if normalization repairs anything, the transaction returns `state_changed` and does not activate in the same call. A new deliberate input gesture must operate against the repaired state.

Without such a repair, activation succeeds only when all of the following remain true:

- menu interaction and a popup are still active;
- `level` is the deepest currently open popup level;
- `item_index` is in range in that level's currently resolved `MenuModel`;
- the same item is still the retained semantic selection at that level;
- the item is enabled and of `MenuItemKind::command`;
- the referenced `Command` is still alive.

Submenus and separators cannot be activated by this transaction.

On success the controller copies `Command::Reference`, calls `reset()` to close all transient menu state, and returns `MenuInteractionAction::activate_command`. It never executes the Command itself.

The controller also gains one private `popupMenuAtLevel()` resolver used by both `selectPopupItem()` and `activatePopupItem()`. It re-resolves the requested popup level from current value-state and retains no model pointer across transactions.

### Stateful terminal press/release gesture

`TerminalMenuPointerInteraction::GestureState` stores only:

```text
TerminalMenuPopupHit { level, item_index }
```

for the popup row hit by a Primary press.

It stores no semantic-object pointer and no frame pointer. The state is explicitly host-owned, mirroring other toolkit interaction seams where multi-event policy belongs to the host while semantic ownership stays in Core.

A fresh Primary press resets any older armed identity. If an active popup row is hit, the normal `selectPopupItem()` transaction runs first; the painted level/row identity is then retained for possible release completion.

Motion does not retarget the armed identity. This permits the common interaction where the pointer leaves a pressed row and later returns before release.

A Primary release completes activation only when `TerminalMenuHitTest::popupItemAt()` on the current presentation frame returns exactly the same `{level,item_index}` as the press. Releasing on another row or outside cancels activation while preserving the press-established menu selection.

After a geometric match, `activatePopupItem()` performs the independent semantic proof described above. Therefore matching coordinates alone can never activate a separator, disabled/expired Command, submenu, stale selection, or repaired menu state.

The armed identity is retired before an activation result is returned, so no press state can survive into host-side callback execution.

### Command execution remains delayed

The stateful pointer layer returns the same `MenuInteractionResult` contract as keyboard interaction.

On successful release:

```text
Primary release on same command row
        ↓
activatePopupItem()
        ↓
copy lifetime-safe Command::Reference
        ↓
close MenuInteractionController state
        ↓
return activate_command
        ↓
host presents closed menu
        ↓
host executes returned Command (if still alive)
```

This keeps arbitrary application callbacks outside the transient menu transaction and allows callbacks to rebuild menus, mutate Widgets, or request application exit safely after the popup has disappeared.

### Submenus remain separate

A matching release on a selected submenu is consumed but does not open its child popup. Submenu opening needs its own deliberate gesture policy (click, hover, delay, movement between parent and child, and fitted popup geometry) and is not conflated with Command activation.

### Modal and cancellation rules

- release on a different popup row cancels Command activation;
- release outside the armed row cancels activation;
- stateless callers never activate on release;
- non-primary press interrupts an armed Primary gesture;
- if menu interaction was closed by keyboard/application policy before the next pointer event, `GestureState` is reset;
- while a menu remains active, unsupported pointer transitions continue to be consumed so they cannot click through to underlying Widgets.

### Consequences

- terminal popup Commands now have a clean press/select → matching-release/activate library path;
- Core activation semantics are backend-neutral and reusable by rendered/native pointer adapters later;
- Command callbacks are still delayed until after menu state closes;
- cancellation by releasing elsewhere is deterministic;
- model mutation between press and release fails closed;
- no borrowed semantic pointer or presentation frame survives between events;
- existing stateless hosts retain their previous selection-only behavior;
- submenu interaction remains independently evolvable.

### Deferred scope

This ADR does not yet add:

- wiring the new stateful menu `GestureState` into `terminal_form_demo`;
- opening submenus by pointer click or hover;
- popup hover selection;
- delayed submenu auto-open timers;
- drag-to-select menu rows;
- native/rendered menu pointer adapters;
- stable persistent menu-item identities across arbitrary structural rebuilds.

---

## Deutsch

### Kontext

ADR 0107 hat die direkte Auswahl von Popup-Zeilen über Terminal-Pointer-Geometrie eingeführt. Ein Primary-Press kann nun über `TerminalMenuHitTest` eine gezeichnete Popup-Zeile bestimmen und den backend-neutralen `MenuInteractionController` bitten, exakt dieses semantische Item auszuwählen. Separatoren und nicht verfügbare Commands bleiben konsumiert, aber nicht ausgewählt; bei Änderungen einer Parent-Auswahl bleibt verschachtelter Popup-Zustand strukturell konsistent.

Als nächstes fehlt die Command-Aktivierung. Eine Aktivierung bereits beim Press wäre einfach, würde aber zwei unterschiedliche Interaktionsphasen vermischen und von den vorhandenen Pointer-Controls des Toolkits abweichen: Ein Press etabliert Absicht/Zustand, ein passendes Release schließt die Aktivierung ab. Außerdem könnte der Benutzer eine Aktivierung dann nicht mehr durch Release auf einer anderen Zeile abbrechen.

Der Menü-Controller besitzt für Tastaturaktivierung bereits einen wichtigen zweiphasigen Command-Vertrag: Er kopiert eine lebenszeitsichere `Command::Reference`, schließt den transienten Menüzustand, liefert `activate_command` zurück und überlässt die tatsächliche Callback-Ausführung dem Host, nachdem das geschlossene Menü dargestellt wurde. Pointer-Aktivierung soll exakt diesen Lebenszyklus beibehalten.

Gleichzeitig darf die Terminal-Interaktionsschicht zwischen Press und Release keinen `MenuModel*`, `MenuItem*`, `Command*` und keinen Zeiger auf einen Presentation-Frame behalten. Menüstruktur kann neu aufgebaut werden, Commands können deaktiviert werden oder verschwinden; jede backend-seitige Identität muss deshalb gegen den aktuellen semantischen Zustand erneut validiert werden.

### Entscheidung

Wir ergänzen zwei zusammenarbeitende Bausteine:

1. backend-neutral `MenuInteractionController::activatePopupItem(bar, level, item_index)`;
2. einen opt-in, host-eigenen `TerminalMenuPointerInteraction::GestureState` sowie einen stateful `handle()`-Overload.

Der vorhandene stateless `TerminalMenuPointerInteraction::handle()` bleibt quell- und verhaltenskompatibel: Ein Popup-Press kann eine Zeile auswählen, ein späteres Release kann aber keinen Command aktivieren, weil keine Press-Identität zwischen Events gespeichert wird.

### Backend-neutrale Aktivierungstransaktion

`activatePopupItem()` akzeptiert ausschließlich semantische Wertindizes:

```cpp
MenuInteractionResult activatePopupItem(
    const MenuBarModel& bar,
    std::size_t level,
    std::size_t item_index);
```

Keine Terminalkoordinate, Pixelposition, kein natives Handle und kein Presentation-Objekt gelangen in Core.

Vor der Aktivierung normalisiert der Controller seinen gespeicherten Zustand gegen das übergebene aktuelle `MenuBarModel`. Aktivierung wird bewusst wie ein Commit behandelt: Falls die Normalisierung etwas repariert, liefert die Transaktion `state_changed` zurück und aktiviert im selben Aufruf nichts. Eine neue bewusste Eingabegeste muss anschließend gegen den reparierten Zustand arbeiten.

Ohne solche Reparatur gelingt die Aktivierung nur, wenn alle folgenden Bedingungen weiterhin gelten:

- Menüinteraktion und ein Popup sind noch aktiv;
- `level` ist die tiefste aktuell offene Popup-Ebene;
- `item_index` liegt im aktuell aufgelösten `MenuModel` dieser Ebene im gültigen Bereich;
- dasselbe Item ist weiterhin die gespeicherte semantische Auswahl dieser Ebene;
- das Item ist aktiviert und vom Typ `MenuItemKind::command`;
- der referenzierte `Command` lebt noch.

Submenus und Separatoren können über diese Transaktion nicht aktiviert werden.

Bei Erfolg kopiert der Controller `Command::Reference`, ruft `reset()` zum Schließen des vollständigen transienten Menüzustands auf und liefert `MenuInteractionAction::activate_command`. Der Command selbst wird niemals im Controller ausgeführt.

Zusätzlich erhält der Controller einen privaten Resolver `popupMenuAtLevel()`, den sowohl `selectPopupItem()` als auch `activatePopupItem()` verwenden. Er löst die gewünschte Popup-Ebene jeweils neu aus aktuellem Value-State auf und behält keinen Modellzeiger über eine Transaktion hinaus.

### Stateful Terminal-Press-/Release-Geste

`TerminalMenuPointerInteraction::GestureState` speichert ausschließlich:

```text
TerminalMenuPopupHit { level, item_index }
```

für die Popup-Zeile, die beim Primary-Press getroffen wurde.

Er speichert keinen Zeiger auf semantische Objekte und keinen Frame-Zeiger. Der Zustand gehört ausdrücklich dem Host. Das entspricht anderen Toolkit-Interaktionsschichten: Mehr-Event-Policy gehört in den passenden Host-Scope, semantischer Besitz bleibt in Core.

Jeder neue Primary-Press löscht eine ältere armed Identität. Wird eine Zeile eines aktiven Popups getroffen, läuft zuerst die normale `selectPopupItem()`-Transaktion; anschließend wird lediglich die gezeichnete Level-/Zeilenidentität für ein mögliches Release gespeichert.

Motion verändert diese armed Identität nicht. Dadurch ist die übliche Bedienung möglich, bei der der Pointer die gedrückte Zeile verlässt und vor dem Release wieder dorthin zurückkehrt.

Ein Primary-Release schließt eine Aktivierung nur ab, wenn `TerminalMenuHitTest::popupItemAt()` auf dem aktuellen Presentation-Frame exakt dasselbe `{level,item_index}` wie beim Press liefert. Ein Release auf einer anderen Zeile oder außerhalb bricht die Aktivierung ab; die durch den Press etablierte Menüauswahl bleibt bestehen.

Nach einem geometrischen Treffer führt `activatePopupItem()` die davon unabhängige semantische Prüfung durch. Gleiche Koordinaten allein können daher niemals einen Separator, deaktivierten/abgelaufenen Command, ein Submenu, eine veraltete Auswahl oder einen reparierten Menüzustand aktivieren.

Die armed Identität wird entfernt, bevor ein Aktivierungsergebnis zurückgegeben wird. Dadurch kann kein Press-Zustand in die spätere Callback-Ausführung des Hosts hineinleben.

### Command-Ausführung bleibt verzögert

Die stateful Pointer-Schicht liefert denselben `MenuInteractionResult`-Vertrag wie die Tastaturinteraktion.

Bei erfolgreichem Release:

```text
Primary-Release auf derselben Command-Zeile
        ↓
activatePopupItem()
        ↓
lebenszeitsichere Command::Reference kopieren
        ↓
MenuInteractionController-Zustand schließen
        ↓
activate_command zurückgeben
        ↓
Host stellt geschlossenes Menü dar
        ↓
Host führt zurückgegebenen Command aus (falls noch lebend)
```

Damit bleiben beliebige Application-Callbacks außerhalb der transienten Menütransaktion. Callbacks können nach dem Verschwinden des Popups Menüs neu aufbauen, Widgets verändern oder das Beenden der Anwendung anfordern.

### Submenus bleiben separat

Ein passendes Release auf einem ausgewählten Submenu wird konsumiert, öffnet aber noch kein Child-Popup. Das Öffnen von Untermenüs benötigt eine eigene bewusst definierte Gesten-Policy (Klick, Hover, Verzögerung, Bewegung zwischen Parent und Child sowie angepasste Popup-Geometrie) und wird nicht mit Command-Aktivierung vermischt.

### Modalitäts- und Abbruchregeln

- Release auf einer anderen Popup-Zeile bricht Command-Aktivierung ab;
- Release außerhalb der armed Zeile bricht Aktivierung ab;
- stateless Aufrufer aktivieren niemals beim Release;
- ein Nicht-Primary-Press unterbricht eine armed Primary-Geste;
- wurde die Menüinteraktion zwischenzeitlich per Tastatur/Application-Policy geschlossen, wird `GestureState` beim nächsten Pointer-Event zurückgesetzt;
- solange ein Menü aktiv bleibt, werden noch nicht unterstützte Pointer-Transitionen weiterhin konsumiert und können nicht zu darunterliegenden Widgets durchklicken.

### Folgen

- Terminal-Popup-Commands besitzen nun einen sauberen Bibliothekspfad Press/Auswahl → passendes Release/Aktivierung;
- die Core-Aktivierungssemantik ist backend-neutral und später auch für Rendered-/Native-Pointer-Adapter nutzbar;
- Command-Callbacks bleiben bis nach dem Schließen des Menüzustands verzögert;
- Abbruch durch Release an anderer Stelle ist deterministisch;
- Modellmutation zwischen Press und Release schlägt konservativ fehl;
- zwischen Events bleibt kein geliehener semantischer Zeiger und kein Presentation-Frame erhalten;
- vorhandene stateless Hosts behalten ihr bisheriges reines Auswahlverhalten;
- Submenu-Interaktion kann unabhängig weiterentwickelt werden.

### Zurückgestellter Umfang

Diese ADR ergänzt noch nicht:

- Integration des neuen stateful Menü-`GestureState` in `terminal_form_demo`;
- Öffnen von Untermenüs per Pointer-Klick oder Hover;
- Popup-Hover-Selektion;
- verzögerte automatische Untermenüöffnung;
- Drag-to-select über Menüzeilen;
- Native-/Rendered-Menü-Pointer-Adapter;
- stabile persistente Menü-Item-Identitäten über beliebige strukturelle Neuaufbauten hinweg.
