# ADR 0109 – Terminal popup submenu clicks use a backend-neutral opening transaction

**Status:** Accepted  
**Date:** 2026-10-05

## English

### Context

ADR 0107 introduced direct semantic popup-row selection from terminal pointer geometry, and ADR 0108 added stateful Primary press/release completion for Command items. The terminal menu adapter now retains only a painted `{level,item_index}` identity between press and release, re-hit-tests the current presentation frame on release, and asks the backend-neutral `MenuInteractionController` to prove current semantics before a Command may be activated.

Submenu rows were intentionally left one step behind. A pointer press could select `Tools >`, and the matching release was consumed, but the child popup did not open. Keyboard Right/Enter already knows how to enter a selected submenu, yet routing a terminal click by synthesizing a keyboard event would couple two distinct input policies and would be unsafe at the root popup where Right can also mean switching to the next top-level menu.

The menu interaction layer therefore needs an explicit semantic operation for "open this exact selected submenu" in the same way that it already has explicit operations for selecting a popup item and activating a popup Command.

### Decision

Add the backend-neutral transaction:

```cpp
MenuInteractionResult openPopupSubmenu(
    const MenuBarModel& bar,
    std::size_t level,
    std::size_t item_index);
```

The operation accepts value-state identity only. It receives no terminal coordinate, pixel position, native menu handle, pointer event, presentation frame, or retained semantic-object pointer.

The terminal stateful pointer adapter uses this operation when a Primary release lands on exactly the same topmost popup row that was armed by the preceding Primary press. A matching release now completes one of two semantic item operations:

```text
matching Primary release
        ↓
openPopupSubmenu(level,item)
        ↓
state_changed ? child opened / state repaired : no submenu transition
        ↓ if action == none
activatePopupItem(level,item)
        ↓
activate_command or none
```

The terminal adapter deliberately does not inspect `MenuItemKind` itself. It supplies only the identity proven by terminal presentation geometry; Core decides whether that identity currently denotes an enabled selected submenu, a live selected Command, or neither.

### Submenu-opening proof

`openPopupSubmenu()` succeeds only when all of the following remain true after normalization:

- menu interaction is active and a popup is open;
- `level` names a currently represented popup level;
- `item_index` is in range in the `MenuModel` currently resolved for that level;
- `item_index` is still the semantic selection of that level;
- the selected item is enabled;
- the selected item has `MenuItemKind::submenu`;
- the selected item still exposes a child `MenuModel`.

Commands, separators, disabled/unavailable rows, unrelated selections, stale indices, and absent child models are rejected without guessing.

### Normalization is commit-like

As with pointer Command activation, submenu opening treats a normalization repair as a transaction boundary. If `normalizeAgainst()` changes retained controller state, `openPopupSubmenu()` returns `state_changed` and does **not** also open a child during the same request.

This prevents a pointer identity captured from pre-repair presentation state from being reinterpreted immediately against a structurally repaired model. A later deliberate input gesture can act on the repaired state.

### One child level, no implicit row selection

A successful request opens exactly one child popup level. The new child selection is initialized to `std::nullopt`.

This matches existing keyboard submenu entry:

```text
selected Tools >
      ↓ Right / matching pointer click
Tools child popup opens
      ↓
no child row selected yet
```

The next explicit key or pointer gesture chooses a child item. Pointer opening does not silently pick the first row merely because a child became visible.

`handlePopupKey()` now delegates its `open_submenu` result to the same `openPopupSubmenu()` transaction. Keyboard and pointer therefore share one implementation for validating submenu ownership, constructing `MenuPath`, and initializing the new popup level.

### Existing children are stable

If the exact selected submenu already owns the next open popup level, another open request is a no-op. Existing descendants are preserved rather than closed and rebuilt.

This complements ADR 0107: `selectPopupItem()` already preserves descendants when the same ancestor submenu remains selected, while selecting a different ancestor item closes descendants that no longer have a valid owner.

When a new child really must be opened, `openPopupSubmenu()` truncates retained path/selection state after the requested parent level before appending the new submenu transition. This keeps `MenuPath` and `popup_selections_` structurally synchronized even when the public transaction is used independently of terminal pointer policy.

### Terminal release policy

The stateful terminal adapter continues to arm only value identity on Primary press:

```text
TerminalMenuPopupHit { level, item_index }
```

Motion does not retarget that identity. On Primary release:

1. the armed identity is cleared before semantic completion;
2. the current presentation frame is hit-tested again;
3. release must hit exactly the same `{level,item_index}`;
4. Core is asked to open a submenu;
5. only when no submenu transition or normalization repair occurred is Command activation attempted.

Releasing on a different row or outside still cancels completion while preserving the selection established by the press.

A submenu click therefore follows the same physical cancellation rule as a Command click without requiring terminal-specific semantic traversal.

### Presentation remains authoritative for geometry

Opening changes semantic menu state only. The child popup's concrete location is still calculated later by the normal terminal frame builder.

This is important because nested popup placement may depend on viewport fitting and may open to the left rather than the right. After a child opens, a host rebuilds `MenuFramePresentationSnapshot`; subsequent pointer input is hit-tested against that new final geometry. The interaction layer never predicts child rectangles itself.

The nested regression therefore explicitly rebuilds presentation after opening the first submenu before clicking a submenu inside that child.

### Consequences

- terminal popup submenus can now be opened with a deliberate Primary click/release;
- nested submenu levels can be opened recursively through the same value-state transaction;
- keyboard and pointer submenu entry share one backend-neutral Core operation;
- terminal input code does not inspect `MenuItemKind` or retain semantic pointers;
- newly opened child popups start unselected;
- already-open matching child routes remain stable;
- model repair fails closed instead of combining repair and opening in one transaction;
- final popup geometry remains solely a presentation concern;
- the stateless terminal pointer overload remains selection-only and source-compatible.

### Deferred scope

This decision does not add:

- hover-driven popup-row selection;
- delayed submenu opening on hover;
- pointer-motion transfer from a parent row into an already-open child;
- click-and-drag menu traversal;
- automatic submenu closing timers;
- rendered/native menu pointer adapters;
- stable persistent item identities across arbitrary structural rebuilds.

Those policies can build on `selectPopupItem()`, `openPopupSubmenu()`, and `activatePopupItem()` without changing terminal geometry contracts.

---

## Deutsch

### Kontext

ADR 0107 hat die direkte semantische Auswahl von Popup-Zeilen über Terminal-Pointer-Geometrie eingeführt; ADR 0108 ergänzte den zustandsbehafteten Abschluss von Primary-Press/-Release für Command-Einträge. Der Terminal-Menüadapter behält zwischen Press und Release nur noch die gezeichnete Identität `{level,item_index}`, führt beim Release erneut einen Hit-Test gegen den aktuellen Presentation-Frame aus und lässt den backend-neutralen `MenuInteractionController` die aktuelle Semantik beweisen, bevor ein Command aktiviert werden darf.

Submenu-Zeilen lagen bewusst noch einen Schritt zurück. Ein Pointer-Press konnte `Tools >` auswählen und das passende Release wurde konsumiert, aber das Child-Popup öffnete sich nicht. Die Tastaturbedienung kann über Rechts/Enter bereits in ein ausgewähltes Submenu wechseln. Einen Terminal-Klick dafür als künstliches Tastaturereignis zu behandeln, würde jedoch zwei unterschiedliche Eingabepolicies koppeln und wäre insbesondere im Root-Popup unsauber, weil Rechts dort auch zum nächsten Top-Level-Menü wechseln kann.

Die Menü-Interaktionsschicht benötigt deshalb eine explizite semantische Operation für „öffne exakt dieses ausgewählte Submenu“ – analog zu den bereits expliziten Operationen für Popup-Auswahl und Popup-Command-Aktivierung.

### Entscheidung

Wir ergänzen die backend-neutrale Transaktion:

```cpp
MenuInteractionResult openPopupSubmenu(
    const MenuBarModel& bar,
    std::size_t level,
    std::size_t item_index);
```

Die Operation akzeptiert ausschließlich Value-State-Identität. Terminalkoordinaten, Pixelpositionen, native Menühandles, Pointer-Events, Presentation-Frames oder gespeicherte Zeiger auf semantische Objekte gelangen nicht in Core.

Der zustandsbehaftete Terminal-Pointer-Adapter verwendet diese Operation, wenn ein Primary-Release exakt auf derselben obersten Popup-Zeile landet, die beim vorherigen Primary-Press armed wurde. Ein passendes Release schließt nun eine von zwei semantischen Item-Operationen ab:

```text
passendes Primary-Release
        ↓
openPopupSubmenu(level,item)
        ↓
state_changed ? Child geöffnet / Zustand repariert : kein Submenu-Übergang
        ↓ falls action == none
activatePopupItem(level,item)
        ↓
activate_command oder none
```

Der Terminal-Adapter prüft `MenuItemKind` bewusst nicht selbst. Er liefert nur die durch Terminal-Presentation-Geometrie belegte Identität; Core entscheidet, ob diese Identität aktuell ein aktiviertes ausgewähltes Submenu, einen lebenden ausgewählten Command oder keines von beidem bezeichnet.

### Beweis für das Öffnen eines Submenus

`openPopupSubmenu()` ist nur erfolgreich, wenn nach der Normalisierung weiterhin alle folgenden Bedingungen gelten:

- Menüinteraktion ist aktiv und ein Popup ist geöffnet;
- `level` bezeichnet eine aktuell repräsentierte Popup-Ebene;
- `item_index` liegt im für diese Ebene aktuell aufgelösten `MenuModel` im gültigen Bereich;
- `item_index` ist weiterhin die semantische Auswahl dieser Ebene;
- das ausgewählte Item ist aktiviert;
- das ausgewählte Item besitzt `MenuItemKind::submenu`;
- das ausgewählte Item besitzt weiterhin ein Child-`MenuModel`.

Commands, Separatoren, deaktivierte/nicht verfügbare Zeilen, fremde Selektionen, veraltete Indizes und fehlende Child-Modelle werden ohne Raten abgelehnt.

### Normalisierung ist Commit-artig

Wie bei der Pointer-Command-Aktivierung behandelt auch die Submenu-Öffnung eine Reparatur durch Normalisierung als Transaktionsgrenze. Ändert `normalizeAgainst()` den gespeicherten Controller-Zustand, liefert `openPopupSubmenu()` `state_changed` zurück und öffnet **nicht zusätzlich** im selben Request ein Child.

Damit kann eine Pointer-Identität aus einem alten Presentation-Zustand nicht unmittelbar gegen ein strukturell repariertes Modell neu interpretiert werden. Eine spätere bewusste Eingabegeste kann auf dem reparierten Zustand weiterarbeiten.

### Genau eine Child-Ebene, keine implizite Zeilenauswahl

Ein erfolgreicher Request öffnet exakt eine Child-Popup-Ebene. Die neue Child-Selection wird mit `std::nullopt` initialisiert.

Das entspricht dem bestehenden Tastaturverhalten:

```text
ausgewähltes Tools >
      ↓ Rechts / passender Pointer-Klick
Tools-Child-Popup öffnet
      ↓
noch keine Child-Zeile ausgewählt
```

Erst die nächste explizite Tastatur- oder Pointer-Geste wählt ein Item im Child aus. Pointer-Öffnung wählt nicht stillschweigend die erste Zeile, nur weil ein neues Popup sichtbar geworden ist.

`handlePopupKey()` delegiert sein `open_submenu`-Ergebnis nun ebenfalls an dieselbe `openPopupSubmenu()`-Transaktion. Tastatur und Pointer teilen damit eine Implementierung für die Prüfung der Submenu-Eigentümerschaft, den Aufbau von `MenuPath` und die Initialisierung der neuen Popup-Ebene.

### Bereits geöffnete Kinder bleiben stabil

Besitzt exakt das ausgewählte Submenu bereits die nächste offene Popup-Ebene, ist ein weiterer Open-Request ein No-op. Vorhandene Descendants bleiben erhalten, statt geschlossen und neu aufgebaut zu werden.

Das ergänzt ADR 0107: `selectPopupItem()` erhält Descendants bereits dann, wenn dasselbe Parent-Submenu ausgewählt bleibt; die Auswahl eines anderen Ancestor-Items schließt dagegen Descendants, die keinen gültigen Eigentümer mehr besitzen.

Muss tatsächlich ein neues Child geöffnet werden, kürzt `openPopupSubmenu()` gespeicherten Path-/Selection-State hinter der angeforderten Parent-Ebene, bevor der neue Submenu-Übergang angehängt wird. Dadurch bleiben `MenuPath` und `popup_selections_` strukturell synchron, auch wenn die öffentliche Transaktion unabhängig von Terminal-Pointer-Policy verwendet wird.

### Terminal-Release-Policy

Der zustandsbehaftete Terminal-Adapter armed beim Primary-Press weiterhin nur Value-Identität:

```text
TerminalMenuPopupHit { level, item_index }
```

Motion verändert diese Identität nicht. Beim Primary-Release gilt:

1. Die armed Identität wird vor dem semantischen Abschluss gelöscht.
2. Der aktuelle Presentation-Frame wird erneut hit-getestet.
3. Das Release muss exakt dieselbe `{level,item_index}` treffen.
4. Core wird zunächst um das Öffnen eines Submenus gebeten.
5. Nur wenn weder ein Submenu-Übergang noch eine Normalisierungsreparatur stattgefunden hat, wird Command-Aktivierung versucht.

Ein Release auf einer anderen Zeile oder außerhalb bricht den Abschluss weiterhin ab; die durch den Press etablierte Selection bleibt bestehen.

Ein Submenu-Klick besitzt damit dieselbe physische Abbruchregel wie ein Command-Klick, ohne dass terminalspezifischer Code den semantischen Menübaum traversieren muss.

### Presentation bleibt für Geometrie maßgeblich

Das Öffnen verändert ausschließlich semantischen Menüzustand. Die konkrete Position des Child-Popups berechnet weiterhin erst der normale Terminal-Frame-Builder.

Das ist wichtig, weil verschachtelte Popup-Platzierung vom verfügbaren Viewport abhängt und ein Submenu gegebenenfalls nach links statt nach rechts geöffnet wird. Nachdem ein Child geöffnet wurde, baut der Host einen neuen `MenuFramePresentationSnapshot`; folgende Pointer-Eingaben werden gegen diese neue endgültige Geometrie hit-getestet. Die Interaktionsschicht sagt Child-Rechtecke niemals voraus.

Der verschachtelte Regressionstest baut deshalb nach dem Öffnen des ersten Submenus ausdrücklich einen neuen Presentation-Frame, bevor ein Submenu innerhalb dieses Child-Popups angeklickt wird.

### Folgen

- Terminal-Popup-Submenus können jetzt über einen bewussten Primary-Click/-Release geöffnet werden.
- Verschachtelte Submenu-Ebenen lassen sich rekursiv über dieselbe Value-State-Transaktion öffnen.
- Tastatur und Pointer teilen eine backend-neutrale Core-Operation für Submenu-Eintritt.
- Terminal-Input-Code prüft weder `MenuItemKind` noch speichert er semantische Zeiger.
- Neu geöffnete Child-Popups starten ohne Selection.
- Bereits geöffnete passende Child-Routen bleiben stabil.
- Modellreparatur schlägt konservativ fehl, statt Reparatur und Öffnung in einer Transaktion zu kombinieren.
- Finale Popup-Geometrie bleibt ausschließlich Aufgabe der Presentation-Schicht.
- Der stateless Terminal-Pointer-Overload bleibt Selection-only und quellkompatibel.

### Vertagter Umfang

Diese Entscheidung ergänzt noch nicht:

- Hover-gesteuerte Popup-Zeilenauswahl;
- verzögertes Submenu-Öffnen bei Hover;
- Pointer-Motion-Übergang von einer Parent-Zeile in ein bereits offenes Child;
- Click-and-Drag-Menünavigation;
- automatische Timer zum Schließen von Submenus;
- Rendered-/Native-Menü-Pointer-Adapter;
- stabile persistente Item-Identitäten über beliebige strukturelle Rebuilds hinweg.

Diese Policies können später auf `selectPopupItem()`, `openPopupSubmenu()` und `activatePopupItem()` aufbauen, ohne die Terminal-Geometrieverträge zu verändern.
