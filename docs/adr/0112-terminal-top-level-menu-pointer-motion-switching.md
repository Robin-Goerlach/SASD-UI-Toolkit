# ADR 0112 – Active terminal menus switch top-level root popups on pointer motion

**Status:** Accepted  
**Date:** 2026-10-05

## English

### Context

ADR 0110 defined semantic popup-row selection for `PointerAction::move`, while deliberately deferring top-level menu-bar switching. ADR 0111 then added opt-in all-motion terminal tracking (`DECSET 1003`), and the terminal form demo now enables that policy so ordinary pointer movement is available even when no button is held.

One desktop-style interaction gap remains: after a root popup is open, moving the pointer from one top-level title to another leaves the original root popup visible. A conventional menu bar instead follows the pointer across titles while menu mode is already active.

This behavior must not weaken the existing separation of responsibilities:

- `TerminalMenuHitTest` owns terminal-cell geometry and z-order;
- `TerminalMenuPointerInteraction` owns terminal pointer gesture policy;
- `MenuInteractionController` owns transient semantic menu state;
- `TerminalSession` owns whether passive/all-motion reports are requested at all.

### Decision

While menu interaction is active, `PointerAction::move` now uses the following order:

```text
PointerAction::move
        ↓
Topmost popup row hit?
        ├─ yes → selectPopupItem(...)
        └─ no
             ↓
Visible menu-bar title hit?
        ├─ same already-open title → consume, preserve state
        ├─ different title         → replace root popup
        └─ no title                → consume without semantic change
```

Popup hit testing remains first because popup layers are painted after the menu bar and therefore own overlapping cells.

### Root switching reuses the established controller transition

The terminal adapter does not mutate `MenuPath`, popup selections, or top-level indices directly.

For a different valid title it performs the same controller sequence already used by pointer press:

```text
controller.begin(bar, title)
        ↓
controller.handleKey(bar, Enter)
        ↓
selected title becomes active
root popup opens
root popup selection is empty
old descendants are discarded
```

The synthetic Enter is not terminal protocol state; it is the controller's existing canonical semantic transition for opening the selected root popup without forcing a row selection. No terminal coordinates or presentation objects enter Core.

A dedicated new Core API is intentionally not introduced in this slice because the controller already exposes the required transition and its behavior is already shared with keyboard semantics. If another backend later needs a direct top-level-open transaction, that can be promoted deliberately rather than expanding the public API speculatively.

### Moving over the current title is a no-op

If the pointer moves over the same top-level title whose root popup is already open, the adapter consumes the event but does not call `begin()` again.

This is important because `begin()` resets transient popup state. Reopening the current title on every hover would unnecessarily destroy valid descendants such as:

```text
File
 └─ Tools >
      └─ Advanced
```

Crossing the visible `File` title therefore preserves the complete existing popup path.

### Switching titles clears unrelated popup state

Moving from one title to another intentionally starts a fresh root-menu transaction:

```text
File popup + selection/children
        ↓ hover Edit
Edit root popup
selection = none
no File descendants survive
```

Popup-row indices are model-local values. Carrying a row index from `File` into `Edit` would silently reinterpret an unrelated semantic item.

### Armed popup clicks are retired across a root-menu switch

`TerminalMenuPointerInteraction::GestureState` stores only the armed popup value identity:

```text
{ level, item_index }
```

Those values are meaningful only inside the root menu model in which the press occurred.

A dangerous sequence would otherwise be possible:

```text
press File row 0       -> armed {0,0}
move to Edit title     -> root menu changes
move to Edit row 0     -> selection becomes {0,0}
release Edit row 0     -> numeric identity appears to match old press
```

Even though both rows use the same numbers, they are different semantic items.

Therefore a top-level title switch explicitly resets the host-owned menu `GestureState` before replacing the root popup. A later release cannot complete a click that began in another top-level menu.

Motion within the same root popup continues to preserve the armed identity as defined by ADR 0110; only the root-menu scope change retires it.

### Stale presentation indices fail closed

As with pointer press, a title hit from the presentation frame is checked against the current `MenuBarModel` before it is supplied to `begin()`.

This matters because `begin()` intentionally falls back to index zero when its preferred index is stale. Pointer geometry must never turn an unprovable stale title hit into a different valid menu through that fallback.

An out-of-range title hit is therefore consumed with no semantic mutation.

### No submenu hover opening yet

This ADR changes only top-level title motion.

Moving over a popup submenu row still selects that row but does not automatically open its child. Delayed submenu opening, close timers, and pointer-transfer heuristics across submenu gaps require timing/state policy that should remain a separate coherent slice.

### Tests

Regression tests cover:

- moving from an open `File` popup to the `Edit` title replaces the root popup and clears the old row selection;
- moving over the already-active title preserves an open descendant submenu path;
- switching top-level titles retires a stateful armed popup identity so the same numeric row index in the new root menu cannot be activated by the old press;
- existing popup-row motion selection, unavailable-row behavior, descendant truncation, and same-root armed-click semantics remain unchanged.

### Consequences

- active terminal menu bars now follow passive pointer motion across top-level titles;
- popup z-order remains authoritative over the menu bar;
- unrelated root-menu selection/descendant state is never carried across titles;
- a top-level scope switch cannot reinterpret an old armed click in the new menu;
- moving over the current title does not collapse valid descendants;
- Core remains free of terminal geometry and terminal tracking-mode details;
- submenu hover opening remains explicitly deferred.

### Deferred scope

This decision does not add:

- delayed submenu opening on pointer hover;
- submenu close timers;
- diagonal pointer-intent heuristics between parent and child popups;
- mnemonic/Alt menu activation;
- rendered/native menu pointer adapters;
- a new public direct `openTopLevelMenu()` Core transaction.

---

## Deutsch

### Kontext

ADR 0110 hat die semantische Auswahl von Popup-Zeilen für `PointerAction::move` definiert und den Wechsel von Top-Level-Menütiteln bewusst aufgeschoben. ADR 0111 ergänzte anschließend opt-in All-Motion-Tracking (`DECSET 1003`), und das Terminal-Form-Demo verwendet diesen Modus inzwischen ausdrücklich, sodass normale Pointer-Bewegung auch ohne gedrückte Taste verfügbar ist.

Eine typische Desktop-Menüfunktion fehlt noch: Ist ein Root-Popup geöffnet und der Pointer bewegt sich von einem Top-Level-Titel zu einem anderen, bleibt bisher das ursprüngliche Root-Popup sichtbar. Bei einer klassischen Menüleiste folgt das aktive Menü dagegen dem Pointer über die Titel hinweg.

Dabei müssen die bestehenden Zuständigkeitsgrenzen erhalten bleiben:

- `TerminalMenuHitTest` besitzt Terminal-Zellgeometrie und Z-Order;
- `TerminalMenuPointerInteraction` besitzt die Terminal-Pointer-Gestenpolicy;
- `MenuInteractionController` besitzt den transienten semantischen Menüzustand;
- `TerminalSession` entscheidet ausschließlich, ob passive/all-motion Reports überhaupt angefordert werden.

### Entscheidung

Bei aktivem Menü verarbeitet `PointerAction::move` jetzt in dieser Reihenfolge:

```text
PointerAction::move
        ↓
oberste Popup-Zeile getroffen?
        ├─ ja → selectPopupItem(...)
        └─ nein
             ↓
sichtbarer Menüleisten-Titel getroffen?
        ├─ derselbe bereits offene Titel → konsumieren, Zustand erhalten
        ├─ anderer Titel                 → Root-Popup ersetzen
        └─ kein Titel                    → konsumieren ohne semantische Änderung
```

Popup-Hit-Testing bleibt zuerst, weil Popup-Layer nach der Menüleiste gezeichnet werden und bei Überlappung deshalb die sichtbare Zelle besitzen.

### Root-Wechsel verwendet die vorhandene Controller-Transition

Der Terminal-Adapter verändert `MenuPath`, Popup-Selektionen oder Top-Level-Indizes nicht direkt.

Für einen anderen gültigen Titel verwendet er dieselbe Controller-Sequenz, die bereits beim Pointer-Press eingesetzt wird:

```text
controller.begin(bar, title)
        ↓
controller.handleKey(bar, Enter)
        ↓
Titel wird aktiv
Root-Popup öffnet sich
Root-Selection bleibt leer
alte Descendants werden entfernt
```

Das synthetische Enter ist kein Terminal-Protokollzustand, sondern die bereits vorhandene kanonische semantische Controller-Transition zum Öffnen eines selektierten Root-Popups ohne automatische Zeilenauswahl. Terminalkoordinaten und Presentation-Objekte gelangen weiterhin nicht in Core.

Eine neue öffentliche Core-API wird in diesem Slice bewusst nicht eingeführt, weil der Controller die benötigte Transition bereits anbietet und sie mit der Tastatursemantik teilt. Falls später ein weiteres Backend eine direkte Top-Level-Open-Transaktion benötigt, kann diese gezielt als eigene Core-Operation eingeführt werden statt vorsorglich das API zu verbreitern.

### Bewegung über den aktuellen Titel ist ein No-op

Bewegt sich der Pointer über denselben Top-Level-Titel, dessen Root-Popup bereits geöffnet ist, wird das Event konsumiert, aber `begin()` nicht erneut aufgerufen.

Das ist wichtig, weil `begin()` transienten Popup-Zustand zurücksetzt. Ein erneutes Öffnen bei jedem Hover würde gültige Descendants unnötig zerstören, etwa:

```text
File
 └─ Tools >
      └─ Advanced
```

Das Überqueren des sichtbaren `File`-Titels erhält daher den kompletten bestehenden Popup-Pfad.

### Titelwechsel löscht nicht verwandten Popup-Zustand

Beim Wechsel zu einem anderen Titel wird bewusst eine frische Root-Menü-Transaktion begonnen:

```text
File-Popup + Selection/Children
        ↓ Hover Edit
Edit-Root-Popup
Selection = leer
keine File-Descendants bleiben erhalten
```

Popup-Zeilenindizes sind modell-lokale Werte. Ein Index aus `File` darf nicht in `Edit` als vermeintlich gleiches semantisches Item weiterverwendet werden.

### Armed Popup-Klicks enden beim Root-Menüwechsel

`TerminalMenuPointerInteraction::GestureState` speichert ausschließlich die armed Popup-Value-Identität:

```text
{ level, item_index }
```

Diese Werte besitzen nur innerhalb des Root-Menümodells Bedeutung, in dem der Press stattgefunden hat.

Ohne zusätzlichen Schutz wäre folgende gefährliche Sequenz möglich:

```text
Press File Zeile 0      -> armed {0,0}
Motion zu Edit          -> Root-Menü wechselt
Motion Edit Zeile 0     -> Selection wird {0,0}
Release Edit Zeile 0    -> numerisch scheinbar identisch zum alten Press
```

Obwohl beide Zeilen dieselben Zahlen verwenden, sind es unterschiedliche semantische Items.

Deshalb setzt ein Top-Level-Titelwechsel den host-eigenen Menü-`GestureState` ausdrücklich zurück, bevor das Root-Popup ersetzt wird. Ein späteres Release kann keinen Klick abschließen, der in einem anderen Top-Level-Menü begonnen hat.

Motion innerhalb desselben Root-Popups behält die armed Identität weiterhin wie in ADR 0110 beschrieben; nur der Wechsel des Root-Menü-Scopes beendet sie.

### Veraltete Presentation-Indizes schlagen sicher fehl

Wie beim Pointer-Press wird ein Titelindex aus dem Presentation-Frame gegen das aktuelle `MenuBarModel` geprüft, bevor er an `begin()` übergeben wird.

Das ist wichtig, weil `begin()` bei einem veralteten bevorzugten Index bewusst auf Index null zurückfällt. Pointer-Geometrie darf aus einem nicht mehr beweisbaren Titel-Hit nicht über diesen Fallback ein anderes gültiges Menü erzeugen.

Ein außerhalb des aktuellen Modells liegender Titel-Hit wird deshalb konsumiert, ohne semantischen Zustand zu verändern.

### Submenu-Hover-Öffnung bleibt separat

Diese ADR verändert ausschließlich Top-Level-Titel-Motion.

Motion über einer Popup-Submenu-Zeile wählt diese Zeile weiterhin aus, öffnet das Child aber nicht automatisch. Verzögertes Submenu-Öffnen, Close-Timer und Pointer-Transfer-Heuristiken über Popup-Lücken benötigen eigene Timing-/State-Policy und bleiben daher ein späterer zusammenhängender Slice.

### Tests

Regressionstests prüfen:

- Motion von einem offenen `File`-Popup auf den `Edit`-Titel ersetzt das Root-Popup und entfernt die alte Zeilenauswahl;
- Motion über den bereits aktiven Titel erhält einen geöffneten Descendant-Submenu-Pfad;
- ein Top-Level-Titelwechsel entfernt eine stateful armed Popup-Identität, sodass derselbe numerische Zeilenindex im neuen Root-Menü nicht mit dem alten Press aktiviert werden kann;
- bestehende Popup-Zeilen-Motion, Verhalten nicht verfügbarer Zeilen, Descendant-Truncation und Same-Root-Armed-Click-Semantik bleiben unverändert.

### Konsequenzen

- aktive Terminal-Menüleisten folgen passiver Pointer-Bewegung jetzt über Top-Level-Titel;
- Popup-Z-Order bleibt gegenüber der Menüleiste maßgeblich;
- nicht verwandte Root-Menü-Selection-/Descendant-Zustände werden nie zwischen Titeln übernommen;
- ein Root-Scope-Wechsel kann einen alten armed Klick nicht im neuen Menü reinterpretieren;
- Motion über dem aktuellen Titel zerstört keine gültigen Descendants;
- Core bleibt frei von Terminalgeometrie und Terminal-Tracking-Modusdetails;
- Submenu-Hover-Öffnung bleibt ausdrücklich aufgeschoben.

### Aufgeschobener Umfang

Diese Entscheidung ergänzt noch nicht:

- verzögertes Öffnen von Submenus per Pointer-Hover;
- Submenu-Close-Timer;
- diagonale Pointer-Intent-Heuristiken zwischen Parent- und Child-Popup;
- Mnemonic-/Alt-Menüaktivierung;
- Rendered-/Native-Menü-Pointer-Adapter;
- eine neue öffentliche direkte Core-Transaktion `openTopLevelMenu()`.
