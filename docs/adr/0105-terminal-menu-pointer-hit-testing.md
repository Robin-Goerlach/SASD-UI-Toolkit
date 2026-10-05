# ADR 0105 – Terminal menu pointer hit testing uses presentation-frame geometry

**Status:** Accepted  
**Date:** 2026-10-05

## English

### Context

The terminal demo now receives real pointer events, and TextField pointer interaction is mature enough that menu pointer interaction is the next major terminal usability gap.

Menu presentation already has a strong separation of responsibilities:

- `MenuBarModel` and `MenuInteractionController` contain backend-neutral semantic menu state;
- `buildMenuPresentationFrame()` resolves the current interaction into an owned `MenuFramePresentationSnapshot`;
- terminal placement then decides the concrete cell origin of every popup, including viewport fitting and left/right submenu flipping;
- `renderMenuPresentationFrame()` paints exactly that owned frame in paint order.

Pointer interaction needs to answer two geometric questions before any semantic menu action can be designed:

1. which top-level menu title owns a terminal cell;
2. which popup level and row is visually under a terminal cell.

Recomputing those rectangles from `MenuBarModel` or `MenuInteractionController` would create a second placement implementation. That would be especially dangerous for fitted or left-flipped submenus because semantic state does not itself contain final terminal origins.

### Decision

Add terminal-only `TerminalMenuHitTest` over an already-built `MenuFramePresentationSnapshot`.

The helper exposes two deliberately narrow queries:

- `menuBarIndexAt(frame, point, ambiguous_width)` returns the top-level title index whose complete padded title span contains the point;
- `popupItemAt(frame, point, ambiguous_width)` returns `TerminalMenuPopupHit{level, item_index}` for the topmost popup row containing the point.

The hit-test layer is read-only. It does not mutate `MenuInteractionController`, open or close menus, activate commands, change focus, retain model pointers, or interpret pointer buttons.

### Menu-bar geometry

Terminal menu-bar rendering assigns every title one leading and one trailing padding cell. Selection highlighting covers that entire padded span.

Pointer hit testing therefore treats the same padded span as the title's geometric surface. A click on title padding is a hit on that title rather than on empty menu-bar background.

Unicode width is measured with the same terminal measurement primitive and `AmbiguousWidthMode` used by menu presentation. Wide scalars therefore contribute their actual terminal-cell width instead of being treated as byte or scalar counts.

### Popup geometry and paint order

Every positioned popup already carries its final terminal-cell origin in `MenuFramePresentationSnapshot`.

`popupItemAt()` measures each popup with the existing presentation contract and searches popup layers in reverse frame order. This mirrors rendering, where later popup layers are painted after earlier layers and are therefore visually on top if rectangles overlap.

Each popup item occupies exactly one measured row. The returned `item_index` is the row index inside that popup snapshot.

### Separators and disabled items

Separators and disabled commands still produce geometric row hits.

This is intentional. The hit-test layer answers only which painted menu row owns a cell. Whether that row may become selected, open a submenu, or activate a command is semantic interaction policy and belongs in the later menu-pointer interaction layer.

Keeping availability policy out of geometry avoids turning terminal hit testing into a second `MenuInteractionController`.

### Fail-closed behavior

Menu rendering is transactional: if an owned frame contains text that cannot be represented by the current simple terminal `Cell` model, the frame is rejected rather than partially painted.

Hit testing follows the same principle. Menu-bar queries preflight the complete bar snapshot. Popup queries preflight the complete bar plus all popup snapshots before returning a hit.

A synthetic or stale frame that cannot be truthfully rendered therefore cannot produce a pointer identity for geometry that should not be considered visible.

### Ownership boundaries

Responsibilities remain:

- `MenuBarModel`: semantic menu structure;
- `MenuInteractionController`: backend-neutral transient menu state;
- `buildMenuPresentationFrame()`: snapshotting and concrete terminal popup placement;
- `TerminalMenuHitTest`: read-only cell-to-presentation identity mapping;
- future menu pointer interaction: translating hits plus pointer actions into semantic controller operations;
- `renderMenuPresentationFrame()`: terminal-cell painting.

No terminal cell coordinate enters the semantic menu model or controller.

### Consequences

- terminal menu pointer work can reuse the exact geometry already chosen for painting;
- viewport-fitted and left-flipped submenus do not need a second rectangle algorithm;
- topmost popup identity follows frame paint order deterministically;
- padded menu-title cells are clickable in exactly the same area that is visibly highlighted;
- disabled/separator policy remains separate from geometry;
- the next slice can focus purely on semantic hover/press/release behavior rather than re-solving terminal placement.

### Deferred scope

This ADR does not yet add:

- pointer-driven menu activation or closing;
- hover selection;
- click-to-open top-level menus;
- submenu hover/open policy;
- command activation on pointer release;
- outside-click dismissal;
- pointer capture for menus;
- menu auto-open delays or timers.

---

## Deutsch

### Kontext

Das Terminal-Demo erhält inzwischen echte Pointer-Events, und die Pointer-Interaktion des TextFields ist weit genug ausgebaut, dass die Menübedienung per Pointer nun die nächste große Lücke der Terminal-Nutzbarkeit darstellt.

Die Menüpräsentation besitzt bereits eine klare Aufgabentrennung:

- `MenuBarModel` und `MenuInteractionController` enthalten backend-neutralen semantischen Menüzustand;
- `buildMenuPresentationFrame()` löst die aktuelle Interaktion in einen eigenen `MenuFramePresentationSnapshot` auf;
- die Terminal-Platzierung bestimmt anschließend den konkreten Zellursprung jedes Popups einschließlich Viewport-Fitting und Links-/Rechts-Umklappen von Untermenüs;
- `renderMenuPresentationFrame()` zeichnet exakt diesen eigenen Frame in Paint-Reihenfolge.

Für Pointer-Interaktion müssen zunächst zwei geometrische Fragen beantwortet werden:

1. welcher Top-Level-Menütitel besitzt eine Terminalzelle;
2. welche Popup-Ebene und welche Zeile liegt sichtbar unter einer Terminalzelle.

Würden diese Rechtecke erneut aus `MenuBarModel` oder `MenuInteractionController` berechnet, entstünde eine zweite Platzierungsimplementierung. Gerade bei angepassten oder nach links umgeklappten Untermenüs wäre das problematisch, weil der semantische Zustand die endgültigen Terminal-Ursprünge selbst nicht enthält.

### Entscheidung

Wir ergänzen einen terminal-spezifischen `TerminalMenuHitTest`, der auf einem bereits aufgebauten `MenuFramePresentationSnapshot` arbeitet.

Der Helper bietet bewusst nur zwei schmale Abfragen:

- `menuBarIndexAt(frame, point, ambiguous_width)` liefert den Index des Top-Level-Titels, dessen vollständige gepolsterte Titelspanne den Punkt enthält;
- `popupItemAt(frame, point, ambiguous_width)` liefert `TerminalMenuPopupHit{level, item_index}` für die oberste Popup-Zeile unter dem Punkt.

Die Hit-Test-Schicht ist rein lesend. Sie verändert den `MenuInteractionController` nicht, öffnet oder schließt keine Menüs, aktiviert keine Commands, verändert keinen Fokus, hält keine Modellzeiger und interpretiert keine Pointer-Buttons.

### Geometrie der Menüleiste

Die Terminaldarstellung der Menüleiste gibt jedem Titel eine führende und eine nachlaufende Padding-Zelle. Die Selection-Hervorhebung umfasst diese vollständige gepolsterte Spanne.

Der Pointer-Hit-Test behandelt deshalb dieselbe gepolsterte Spanne als geometrische Oberfläche des Titels. Ein Klick auf das Titel-Padding trifft also diesen Titel und nicht einen vermeintlich leeren Bereich der Menüleiste.

Die Unicode-Breite wird mit demselben Terminal-Messprimitive und demselben `AmbiguousWidthMode` wie bei der Menüpräsentation bestimmt. Breite Scalars tragen damit ihre tatsächliche Terminalzellbreite bei und werden nicht als Byte- oder Scalar-Anzahl behandelt.

### Popup-Geometrie und Paint-Reihenfolge

Jedes positionierte Popup trägt seinen endgültigen Terminalzell-Ursprung bereits im `MenuFramePresentationSnapshot`.

`popupItemAt()` misst jedes Popup mit dem bestehenden Präsentationsvertrag und durchsucht die Popup-Ebenen in umgekehrter Frame-Reihenfolge. Das entspricht dem Rendering: Spätere Popup-Ebenen werden nach früheren gezeichnet und liegen bei überlappenden Rechtecken somit sichtbar oben.

Jedes Popup-Item belegt exakt eine gemessene Zeile. Der zurückgegebene `item_index` ist der Zeilenindex innerhalb dieses Popup-Snapshots.

### Separatoren und deaktivierte Einträge

Separatoren und deaktivierte Commands erzeugen weiterhin geometrische Zeilentreffer.

Das ist beabsichtigt. Die Hit-Test-Schicht beantwortet ausschließlich, welche gezeichnete Menüzeile eine Zelle besitzt. Ob diese Zeile selektiert werden darf, ein Untermenü öffnet oder ein Command aktiviert, ist semantische Interaktions-Policy und gehört in die spätere Menü-Pointer-Interaktionsschicht.

Dadurch wird der Terminal-Hit-Test nicht zu einem zweiten `MenuInteractionController`.

### Fail-closed-Verhalten

Das Menü-Rendering ist transaktional: Enthält ein eigener Frame Text, den das aktuelle einfache Terminal-`Cell`-Modell nicht darstellen kann, wird der Frame abgelehnt statt teilweise gezeichnet.

Der Hit-Test folgt demselben Prinzip. Menüleistenabfragen prüfen zuerst den vollständigen Bar-Snapshot. Popup-Abfragen prüfen zuerst Menüleiste und sämtliche Popup-Snapshots, bevor ein Treffer geliefert wird.

Ein synthetischer oder veralteter Frame, der nicht wahrheitsgetreu dargestellt werden kann, darf daher auch keine Pointer-Identität für Geometrie liefern, die gar nicht als sichtbar betrachtet werden sollte.

### Ownership-Grenzen

Die Verantwortlichkeiten bleiben:

- `MenuBarModel`: semantische Menüstruktur;
- `MenuInteractionController`: backend-neutraler transienter Menüzustand;
- `buildMenuPresentationFrame()`: Snapshot-Erzeugung und konkrete Terminal-Popup-Platzierung;
- `TerminalMenuHitTest`: rein lesende Abbildung von Zellen auf Präsentationsidentität;
- zukünftige Menü-Pointer-Interaktion: Übersetzung aus Treffer plus Pointer-Aktion in semantische Controller-Operationen;
- `renderMenuPresentationFrame()`: Zeichnen der Terminalzellen.

Keine Terminalzell-Koordinate gelangt in das semantische Menümodell oder den Controller.

### Folgen

- Terminal-Menü-Pointer-Interaktion kann exakt die Geometrie wiederverwenden, die bereits für die Darstellung gewählt wurde;
- Viewport-angepasste und nach links umgeklappte Untermenüs benötigen keinen zweiten Rechteckalgorithmus;
- die Identität des obersten Popups folgt deterministisch der Paint-Reihenfolge des Frames;
- gepolsterte Menütitel sind exakt in dem Bereich anklickbar, der auch sichtbar hervorgehoben wird;
- Policy für deaktivierte Einträge und Separatoren bleibt von Geometrie getrennt;
- der nächste Slice kann sich ausschließlich auf semantisches Hover-/Press-/Release-Verhalten konzentrieren, statt Terminal-Platzierung erneut zu lösen.

### Zurückgestellter Umfang

Diese ADR ergänzt noch nicht:

- pointergetriebene Menüaktivierung oder Schließung;
- Hover-Selektion;
- Öffnen von Top-Level-Menüs per Klick;
- Hover-/Open-Policy für Untermenüs;
- Command-Aktivierung bei Pointer-Release;
- Schließen per Außenklick;
- Pointer-Capture für Menüs;
- Auto-Open-Verzögerungen oder Timer.
