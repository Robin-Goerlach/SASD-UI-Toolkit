# ADR 0110 – Terminal popup pointer motion selects rows without completing menu items

**Status:** Accepted  
**Date:** 2026-10-05

## English

### Context

ADR 0107 introduced direct popup-row selection from terminal pointer geometry. ADR 0108 added stateful press/release Command activation, and ADR 0109 added backend-neutral submenu opening on a matching click completion.

One interaction gap remains in the semantic adapter: `PointerAction::move` is currently consumed while a menu is active, but moving across visible popup rows does not update the current semantic selection.

It would be tempting to call the missing behavior “hover selection”, but the current terminal session deliberately enables xterm button-event tracking (`DECSET 1002`), not all-motion tracking (`DECSET 1003`). Mode 1002 reports motion while a button is held and is sufficient for click-and-drag interactions; it does not provide ordinary passive mouse movement while no button is pressed.

The architectural problem should therefore be split cleanly:

1. define what the menu interaction layer does when a backend-neutral `PointerAction::move` actually arrives;
2. keep the terminal protocol decision about whether passive/all-motion reports are requested in `TerminalSession` as a separate later decision.

This keeps semantic menu behavior independent of a particular xterm tracking mode.

### Decision

While `MenuInteractionController` is active, `TerminalMenuPointerInteraction` now treats pointer motion over the topmost visible popup row as a semantic row-selection request:

```text
PointerAction::move
        ↓
TerminalMenuHitTest::popupItemAt(...)
        ↓
{ level, item_index }
        ↓
MenuInteractionController::selectPopupItem(...)
```

No new Core API is introduced. The existing `selectPopupItem()` transaction already owns all semantic validation and path-coherence rules needed by both press and motion.

### Geometry remains terminal-specific

`TerminalMenuPointerInteraction` still owns only translation from terminal presentation geometry into value identity.

It does not pass any of the following into Core:

- terminal cell coordinates;
- popup rectangles;
- presentation snapshots;
- native terminal state;
- terminal protocol mode;
- retained `MenuModel*` or `MenuItem*` pointers.

The adapter asks `TerminalMenuHitTest` which painted popup row owns the current cell and supplies only `{level,item_index}` to the backend-neutral controller.

### Motion is selection only

Motion never completes an item.

In particular, moving over a row does not:

- activate a `Command`;
- open a submenu;
- execute application callbacks;
- synthesize Enter/Right;
- dismiss the menu;
- switch a top-level menu-bar title.

It can only request the same semantic row-selection transition that an ordinary popup press already uses.

This keeps future passive hover safe: enabling more motion reports later cannot by itself turn movement into activation.

### Unavailable rows remain modal but do not become selected

Popup geometry can identify separators and disabled/unavailable command rows because they are still painted menu surface.

`selectPopupItem()` already rejects such identities through the semantic `MenuItem::isEnabled()` contract. The terminal adapter still returns an engaged `MenuInteractionResult` when motion lands there.

Therefore:

- the modal menu surface consumes the physical motion;
- the previous valid selection remains unchanged;
- unavailable rows do not become semantic selections;
- the event cannot leak through to application Widgets underneath the popup.

### Existing popup-path invariants are reused

Selecting an ancestor row through motion follows exactly the same structural rule as selecting it through a press.

If the newly selected ancestor does not own an already-open descendant popup, `selectPopupItem()` truncates the stale descendant path. If the selected ancestor is still the exact submenu that owns the child, that child remains open.

This prevents a second terminal-specific implementation of `MenuPath` ownership rules.

### Armed click identity and motion selection are intentionally independent

The stateful terminal pointer adapter retains the physical press identity in `GestureState`:

```text
{ pressed level, pressed item_index }
```

Motion does **not** rewrite that armed identity.

It may, however, change the current semantic selection through `selectPopupItem()`.

On release, completion still requires both:

1. release geometry hits the same popup identity that was armed by press;
2. Core still proves that identity as the current selected semantic item.

Consequently, this sequence is safe:

```text
press Open        -> armed=Open, selection=Open
move to Save      -> armed=Open, selection=Save
release on Open   -> geometry matches armed Open
                    but Core selection is Save
                    => no activation
```

The release retires the armed physical identity and leaves the current semantic selection intact for a later deliberate gesture.

If motion later returns to the original row before release, the same selection transaction may select that row again. Completion can then succeed because both geometry and current Core state once again agree.

### Motion outside popup rows

While a menu is active, motion over menu-bar chrome, another non-popup cell, or outside the popup surface remains consumed but does not clear the current popup selection.

This is deliberately conservative. Clearing selection on every geometric gap would introduce a second policy decision that is not required for row selection and would make crossing narrow borders or submenu gaps unnecessarily destructive.

Top-level title switching on motion is also deferred. It has different semantics from popup-row selection because switching a title replaces the root popup and therefore deserves an explicit policy rather than being hidden inside generic motion handling.

### Current terminal reporting mode remains unchanged

This ADR does **not** change `TerminalSession` pointer-reporting bytes.

The current session still uses:

```text
DECSET 1002  -> button-event tracking
DECSET 1006  -> SGR coordinates
```

As a result, normal production sessions currently deliver pointer motion primarily while a button is held. The new menu behavior therefore immediately improves drag/motion interaction, while also establishing the semantic behavior that a future opt-in `DECSET 1003` all-motion mode can reuse unchanged.

This sequencing is intentional: protocol volume/lifetime policy belongs to `TerminalSession`; semantic row selection belongs to `TerminalMenuPointerInteraction` plus Core.

### Tests

Dedicated regressions cover:

- motion selecting an enabled popup row without executing its Command;
- separator and disabled rows being consumed without replacing the previous valid selection;
- motion to another ancestor row closing descendants that no longer belong to the selected parent;
- stateful press identity remaining armed across motion while Core selection changes, and release refusing to activate when those two identities no longer agree.

The motion tests live in a separate terminal test translation unit so the already large click/activation regression file remains focused on press/release completion semantics.

### Consequences

- terminal menu motion now participates in semantic popup-row selection;
- Core remains the single owner of selectability and popup-path coherence;
- motion cannot activate Commands or open submenus;
- unavailable popup rows remain modal no-ops;
- click arming is not silently retargeted by motion;
- current `DECSET 1002` session behavior is preserved;
- a future all-motion/hover protocol mode can reuse the same semantic adapter without redesigning menu selection.

### Deferred scope

This decision does not add:

- `DECSET 1003` passive/all-motion terminal reporting;
- top-level menu-bar switching on pointer motion;
- delayed submenu opening on hover;
- submenu-close timers;
- pointer-motion transfer heuristics across popup gaps;
- click-and-drag release activation on a different row;
- rendered/native menu pointer adapters.

---

## Deutsch

### Kontext

ADR 0107 hat die direkte Auswahl von Popup-Zeilen über Terminal-Pointer-Geometrie eingeführt. ADR 0108 ergänzte die zustandsbehaftete Command-Aktivierung über Press/Release, ADR 0109 anschließend das backend-neutrale Öffnen von Submenus bei einem passenden Klickabschluss.

In der semantischen Adapter-Schicht fehlt noch ein Baustein: `PointerAction::move` wird bei aktivem Menü zwar konsumiert, das Bewegen über sichtbare Popup-Zeilen aktualisiert die aktuelle semantische Auswahl jedoch noch nicht.

Man könnte das vorschnell „Hover-Auswahl“ nennen. Die aktuelle Terminal-Session aktiviert aber bewusst xterm Button-Event-Tracking (`DECSET 1002`) und **nicht** All-Motion-Tracking (`DECSET 1003`). Modus 1002 liefert Bewegung bei gedrückter Maustaste und reicht damit für Click-/Drag-Interaktion; normale passive Mausbewegung ohne gedrückte Taste wird dadurch nicht angefordert.

Das Architekturproblem wird deshalb sauber getrennt:

1. Wir definieren jetzt, was die Menüinteraktionsschicht tut, wenn tatsächlich ein backend-neutrales `PointerAction::move` eintrifft.
2. Die Protokollentscheidung, ob `TerminalSession` später passive/all-motion Reports anfordert, bleibt eine eigene Entscheidung.

Damit hängt die semantische Menübedienung nicht von einem bestimmten xterm-Tracking-Modus ab.

### Entscheidung

Solange `MenuInteractionController` aktiv ist, behandelt `TerminalMenuPointerInteraction` Pointer-Bewegung über einer sichtbaren obersten Popup-Zeile jetzt als semantischen Auswahl-Request:

```text
PointerAction::move
        ↓
TerminalMenuHitTest::popupItemAt(...)
        ↓
{ level, item_index }
        ↓
MenuInteractionController::selectPopupItem(...)
```

Es wird keine neue Core-API eingeführt. Die vorhandene Transaktion `selectPopupItem()` besitzt bereits sämtliche semantischen Prüfungen und Regeln für kohärente Popup-Pfade, die Press und Motion gemeinsam benötigen.

### Geometrie bleibt terminalspezifisch

`TerminalMenuPointerInteraction` übersetzt weiterhin ausschließlich Terminal-Presentation-Geometrie in Value-Identität.

Folgende Dinge gelangen nicht in Core:

- Terminal-Zellkoordinaten;
- Popup-Rechtecke;
- Presentation-Snapshots;
- nativer Terminal-Zustand;
- Terminal-Protokollmodus;
- gespeicherte `MenuModel*`- oder `MenuItem*`-Zeiger.

Der Adapter fragt `TerminalMenuHitTest`, welche gezeichnete Popup-Zeile die aktuelle Zelle besitzt, und übergibt ausschließlich `{level,item_index}` an den backend-neutralen Controller.

### Motion bedeutet ausschließlich Auswahl

Bewegung schließt niemals ein Item ab.

Insbesondere führt Motion nicht zu:

- Command-Aktivierung;
- Submenu-Öffnung;
- Ausführung von Anwendungscode;
- künstlichem Enter/Rechts;
- Schließen des Menüs;
- Wechsel eines Top-Level-Menütitels.

Motion darf nur dieselbe semantische Zeilenauswahl anfordern, die bereits für einen Popup-Press verwendet wird.

Damit bleibt auch eine spätere passive Hover-Funktion sicher: Mehr Bewegungsereignisse allein dürfen keine Aktivierung erzeugen.

### Nicht verfügbare Zeilen bleiben modal, werden aber nicht ausgewählt

Die Popup-Geometrie kann Separatoren sowie deaktivierte/nicht verfügbare Commands identifizieren, weil auch diese Zeilen gezeichnete Menüoberfläche sind.

`selectPopupItem()` lehnt solche Identitäten bereits über den semantischen Vertrag `MenuItem::isEnabled()` ab. Der Terminal-Adapter liefert beim Motion-Hit trotzdem ein gesetztes `MenuInteractionResult` zurück.

Damit gilt:

- die modale Menüoberfläche konsumiert die physische Bewegung;
- die vorherige gültige Auswahl bleibt erhalten;
- nicht verfügbare Zeilen werden nicht zu semantischen Selektionen;
- das Event kann nicht zu Widgets unter dem Popup durchfallen.

### Vorhandene Popup-Pfad-Invarianten werden wiederverwendet

Die Auswahl einer Ancestor-Zeile durch Motion folgt exakt derselben Strukturregel wie eine Auswahl per Press.

Besitzt der neu ausgewählte Ancestor ein bereits geöffnetes Child-Popup nicht mehr, kürzt `selectPopupItem()` den veralteten Descendant-Pfad. Ist der ausgewählte Ancestor weiterhin exakt das Submenu, dem das Child gehört, bleibt das Child geöffnet.

Damit entsteht keine zweite terminalspezifische Implementierung der `MenuPath`-Eigentumsregeln.

### Armed Click-Identität und Motion-Auswahl sind bewusst unabhängig

Der zustandsbehaftete Terminal-Pointer-Adapter speichert die physische Press-Identität im `GestureState`:

```text
{ pressed level, pressed item_index }
```

Motion schreibt diese armed Identität **nicht** um.

Motion darf jedoch die aktuelle semantische Auswahl über `selectPopupItem()` ändern.

Beim Release müssen weiterhin beide Bedingungen erfüllt sein:

1. Die Release-Geometrie trifft dieselbe Popup-Identität, die beim Press armed wurde.
2. Core kann dieselbe Identität weiterhin als aktuell ausgewähltes semantisches Item beweisen.

Damit ist beispielsweise folgende Sequenz sicher:

```text
Press auf Open     -> armed=Open, selection=Open
Motion zu Save     -> armed=Open, selection=Save
Release auf Open   -> Geometrie passt zu armed Open
                     Core-Selection ist aber Save
                     => keine Aktivierung
```

Das Release entfernt anschließend die physische armed Identität; die aktuelle semantische Selection bleibt für eine spätere bewusste Geste erhalten.

Bewegt sich der Pointer vor dem Release wieder auf die ursprüngliche Zeile zurück, kann dieselbe Selection-Transaktion diese Zeile erneut auswählen. Dann kann der Abschluss wieder erfolgreich sein, weil Geometrie und aktueller Core-Zustand erneut übereinstimmen.

### Bewegung außerhalb von Popup-Zeilen

Bei aktivem Menü wird Motion über Menüleisten-Chrome, anderen Nicht-Popup-Zellen oder außerhalb der Popup-Fläche weiterhin konsumiert, ohne die aktuelle Popup-Auswahl zu löschen.

Das ist bewusst konservativ. Eine Selection bei jeder geometrischen Lücke zu löschen wäre eine zusätzliche Policy-Entscheidung und würde das Überqueren schmaler Rahmen oder Submenu-Abstände unnötig destruktiv machen.

Auch der Wechsel von Top-Level-Titeln durch Motion bleibt aufgeschoben. Er besitzt eine andere Semantik als Popup-Zeilenauswahl, weil ein Titelwechsel das Root-Popup ersetzt und deshalb eine eigene explizite Policy verdient.

### Der aktuelle Terminal-Reporting-Modus bleibt unverändert

Diese ADR verändert die Pointer-Reporting-Bytes von `TerminalSession` **nicht**.

Die Session verwendet weiterhin:

```text
DECSET 1002  -> Button-Event-Tracking
DECSET 1006  -> SGR-Koordinaten
```

Normale Produktions-Sessions liefern Pointer-Motion damit derzeit hauptsächlich bei gedrückter Maustaste. Das neue Verhalten verbessert daher unmittelbar Drag-/Motion-Interaktion und definiert gleichzeitig schon die Semantik, die ein späterer opt-in `DECSET 1003` All-Motion-Modus unverändert wiederverwenden kann.

Diese Reihenfolge ist bewusst gewählt: Protokollvolumen und Protokoll-Lifetime gehören in `TerminalSession`; semantische Zeilenauswahl gehört in `TerminalMenuPointerInteraction` plus Core.

### Tests

Eigene Regressionstests prüfen:

- Motion wählt eine aktivierte Popup-Zeile aus, ohne deren Command auszuführen;
- Separatoren und deaktivierte Zeilen werden konsumiert, ohne die vorherige gültige Auswahl zu ersetzen;
- Motion zu einer anderen Ancestor-Zeile schließt Descendants, die nicht mehr zum ausgewählten Parent gehören;
- eine zustandsbehaftete Press-Identität bleibt über Motion hinweg armed, während sich Core-Selection ändert, und ein Release verweigert die Aktivierung, wenn beide Identitäten nicht mehr übereinstimmen.

Die Motion-Tests liegen in einer eigenen Terminal-Test-Translation-Unit, damit die bereits große Regressiondatei für Click/Aktivierung weiterhin auf Press-/Release-Abschluss konzentriert bleibt.

### Konsequenzen

- Terminal-Menü-Motion nimmt jetzt an der semantischen Popup-Zeilenauswahl teil;
- Core bleibt alleiniger Besitzer von Selectability- und Popup-Pfad-Regeln;
- Motion kann weder Commands aktivieren noch Submenus öffnen;
- nicht verfügbare Popup-Zeilen bleiben modale No-ops;
- Click-Arming wird durch Motion nicht stillschweigend retargeted;
- das aktuelle `DECSET 1002`-Session-Verhalten bleibt unverändert;
- ein späterer All-Motion-/Hover-Protokollmodus kann denselben semantischen Adapter ohne Redesign wiederverwenden.

### Aufgeschobener Umfang

Diese Entscheidung ergänzt noch nicht:

- `DECSET 1003` passive/all-motion Terminal-Reports;
- Wechsel von Top-Level-Menütiteln durch Pointer-Motion;
- zeitverzögertes Öffnen von Submenus bei Hover;
- Timer zum automatischen Schließen von Submenus;
- Motion-Transfer-Heuristiken über Popup-Lücken;
- Click-and-Drag-Release-Aktivierung auf einer anderen Zeile;
- Rendered-/Native-Menü-Pointer-Adapter.
