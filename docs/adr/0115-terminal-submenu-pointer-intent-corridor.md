# ADR 0115 – Terminal submenu pointer-intent corridor foundation

**Status:** Accepted  
**Date:** 2026-10-06

## English

### Context

ADR 0113 added delayed submenu opening and ADR 0114 added a deterministic close grace for short geometric gaps between an already-open parent popup and its child. Those two policies make basic pointer transfer usable, but one classic desktop-menu problem remains.

When a child submenu is open, a user may move diagonally from the owning parent row toward that child. The straight physical path can cross a neighbouring row in the parent popup. The current immediate pointer policy correctly treats that neighbouring row as a real selection and therefore closes descendants that the new row does not own. Semantically that behavior is consistent, but geometrically it can make diagonal transfer feel nervous.

Desktop menu systems often solve this with a "safe triangle" or "menu aim" heuristic: motion that is clearly heading from the owning parent row toward the open child may receive a short deferral before a sibling row replaces the parent selection.

Mixing geometry, timing, event suppression, and menu mutation into one first implementation would make the interaction difficult to reason about and test. This slice therefore introduces only the pointer-intent classifier that later policy can build on.

### Decision

Add `terminal::TerminalMenuPointerIntent`, a host-owned value-state classifier with the advisory result:

```cpp
enum class TerminalMenuPointerIntentKind {
    none,
    toward_open_submenu,
};
```

The class does not mutate `MenuInteractionController`, delay events, select rows, open/close popups, or own a timer. It only answers whether one motion sample is still inside the transfer corridor from the currently owning parent row toward the deepest open child popup.

### Pre-interaction observation is intentional

Unlike hover-open and gap-close timing, pointer intent must inspect state **before** ordinary pointer motion is allowed to select another ancestor row.

Future host ordering is therefore expected to begin like this:

```text
PointerAction::move
        ↓
TerminalMenuPointerIntent::observe(pre-interaction state/frame)
        ↓
none                       toward_open_submenu
  ↓                                ↓
normal immediate policy       future host may defer
TerminalMenuPointerInteraction    sibling replacement
```

This slice stops before that deferral policy. The existing demo and immediate pointer adapter are not changed yet.

If pointer interaction were allowed to run first, `selectPopupItem()` could already truncate the child path when the pointer crosses a sibling parent row. At that point there would be no open child left from which to derive intent.

### Anchor semantics

A transfer anchor is armed only while at least one child popup is open. The classifier targets the **deepest currently-open child** because that is the active transfer boundary in a nested chain.

For popup depth `N`:

```text
child level  = N - 1
parent level = N - 2
parent item  = popupPath.back()
```

Motion over that exact parent item refreshes the anchor to the latest cell in the row. Using the most recent parent-row cell rather than the first entry point gives the corridor the physical exit point from which the user actually started moving toward the child.

The anchor stores only:

```text
selected top-level index
complete MenuPath
parent {level,item_index}
child level
anchor Point
```

No menu/model/presentation pointers survive between calls.

### Safe-triangle geometry

The corridor is the inclusive triangle formed by:

```text
last owning-parent cell
        +
near top corner of child popup
        +
near bottom corner of child popup
```

The child rectangle is measured from the owned presentation snapshot. The classifier chooses whichever visible vertical child edge is horizontally closer to the anchor.

That choice is important because nested terminal popups may be viewport-fitted to either side:

```text
normal case                    fitted case

Parent → Child          Child ← Parent
        ^ near edge      near edge ^
```

No hard-coded "submenus open right" assumption is introduced.

The point-in-triangle test uses widened coordinates and `long double` cross products. `Coordinate` is signed 32-bit; widening before subtraction/product avoids signed integer overflow at extreme synthetic coordinates. A one-row child produces a degenerate target edge, which is treated as a finite line segment rather than an infinite collinear region.

### Structural scope is revalidated on every motion

A numeric `{level,item}` identity is not globally meaningful. Different top-level menus or sibling submenu paths can reuse the same numbers.

Before intent is classified, the current state must prove:

- menu interaction is active;
- a popup is open;
- popup depth is greater than one;
- `MenuPath` is engaged and non-empty;
- `popupPath.size() + 1 == popupDepth()`;
- the frame contains exactly the same number of popup levels;
- the owning parent item exists in the parent snapshot;
- the child popup is representable under the supplied ambiguous-width policy;
- top-level index, full `MenuPath`, parent identity, and child level still match the retained anchor.

Any failed proof clears the anchor and returns `none`.

### Reaching the child ends the transfer

If motion reaches any row of the target child popup, the transfer succeeded. The anchor is cleared and `none` is returned because there is no longer a parent-to-child crossing to protect.

Likewise, motion outside the safe triangle clears the anchor. A later point cannot revive an old trajectory without first returning to the owning parent row and establishing a fresh anchor.

### Sibling rows may be classified without being mutated

One important purpose of the classifier is to detect a physical point that is already on a sibling row of the parent popup while still lying inside the safe triangle.

At this stage the result remains advisory:

```text
pointer is physically over sibling row
        +
trajectory remains inside child corridor
        ↓
TerminalMenuPointerIntentKind::toward_open_submenu
        ↓
Core state remains unchanged
```

A later host-policy slice can decide whether and for how long to defer the sibling selection. Keeping mutation out of this ADR makes that future timing decision explicit rather than hidden inside geometry.

### Tests

Deterministic tests cover:

- a sibling parent row that lies inside a widened right-opening safe triangle is classified as movement toward the open child without changing Core state;
- the same classifier works when final presentation geometry places the child to the left;
- movement away from the corridor immediately retires the anchor and a later gap point cannot reuse it;
- switching to another top-level menu with the same numeric `MenuPath {0}` cannot reuse the old anchor.

Synthetic widened gaps are used only to make the triangle easy to probe in terminal-cell unit tests. Semantic menu state remains real and is built through `MenuInteractionController`.

### Consequences

- pointer intent becomes a separate, deterministic presentation-boundary concept;
- Core remains free of terminal coordinates and trajectory heuristics;
- left/right submenu placement is handled from final frame geometry;
- stale numeric identities fail closed through full structural scope checks;
- no timers, threads, callbacks, or automatic menu mutations are added;
- later safe-triangle timing can be implemented as a policy consumer of this classifier rather than by rewriting pointer hit testing.

### Deferred scope

This decision does not yet add:

- demo integration;
- suppression/deferment of sibling-row selection;
- a safe-triangle timeout;
- adaptive delay based on pointer speed or direction history;
- combining pointer intent with `TerminalMenuCloseInteraction` deadlines;
- rendered/native pointer-intent adapters.

---

## Deutsch

### Kontext

ADR 0113 hat das verzögerte Öffnen von Submenus ergänzt, ADR 0114 eine deterministische Close-Grace für kurze geometrische Lücken zwischen einem bereits geöffneten Parent-Popup und seinem Child. Damit funktioniert der grundlegende Pointer-Transfer, aber ein klassisches Desktop-Menüproblem bleibt bestehen.

Wenn ein Child-Submenu geöffnet ist, bewegt ein Benutzer den Pointer häufig diagonal von der owning Parent-Zeile zum Child. Der direkte physische Weg kann dabei eine benachbarte Zeile im Parent-Popup kreuzen. Unsere aktuelle Immediate-Pointer-Policy behandelt diese Nachbarzeile korrekt als echte Auswahl und schließt deshalb Descendants, die nicht zu dieser neuen Zeile gehören. Semantisch ist das konsistent, geometrisch kann sich ein diagonaler Übergang aber nervös anfühlen.

Desktop-Menüs lösen das häufig mit einer „Safe Triangle“- oder „Menu Aim“-Heuristik: Zeigt die Bewegung eindeutig von der owning Parent-Zeile in Richtung des offenen Childs, kann der Wechsel auf eine Sibling-Zeile kurz verzögert werden.

Geometrie, Timing, Event-Unterdrückung und Menümutation direkt in einer ersten Implementierung zu vermischen, würde die Interaktion unnötig schwer verständlich und testbar machen. Dieser Slice ergänzt deshalb ausschließlich den Pointer-Intent-Classifier, auf dem eine spätere Policy aufbauen kann.

### Entscheidung

Wir ergänzen `terminal::TerminalMenuPointerIntent`, einen host-eigenen Value-State-Classifier mit folgendem beratenden Ergebnis:

```cpp
enum class TerminalMenuPointerIntentKind {
    none,
    toward_open_submenu,
};
```

Die Klasse mutiert den `MenuInteractionController` nicht, verzögert keine Events, wählt keine Zeilen aus, öffnet oder schließt keine Popups und besitzt keinen Timer. Sie beantwortet nur, ob ein Motion-Sample weiterhin innerhalb des Transferkorridors von der aktuell owning Parent-Zeile zum tiefsten offenen Child-Popup liegt.

### Beobachtung vor der Immediate-Interaktion ist Absicht

Anders als Hover-Open- und Gap-Close-Timing muss Pointer Intent den Zustand **vor** der normalen Pointer-Selection betrachten.

Eine spätere Host-Reihenfolge soll deshalb so beginnen:

```text
PointerAction::move
        ↓
TerminalMenuPointerIntent::observe(Pre-Interaction-State/Frame)
        ↓
none                       toward_open_submenu
  ↓                                ↓
normale Immediate-Policy       spätere Host-Policy darf
TerminalMenuPointerInteraction Sibling-Wechsel verzögern
```

Dieser Slice endet vor dieser Delay-Policy. Demo und Immediate-Pointer-Adapter werden noch nicht verändert.

Würde zuerst die normale Pointer-Interaktion laufen, könnte `selectPopupItem()` beim Kreuzen einer Sibling-Parent-Zeile bereits den Child-Pfad truncaten. Danach gäbe es kein geöffnetes Child mehr, aus dessen Geometrie Intent abgeleitet werden könnte.

### Anchor-Semantik

Ein Transfer-Anchor entsteht nur, wenn mindestens ein Child-Popup geöffnet ist. Der Classifier betrachtet das **tiefste aktuell offene Child**, weil dort in einer verschachtelten Kette die aktive Transfergrenze liegt.

Bei Popup-Tiefe `N` gilt:

```text
Child-Level  = N - 1
Parent-Level = N - 2
Parent-Item  = popupPath.back()
```

Motion über genau diesem Parent-Item aktualisiert den Anchor auf die jüngste Zelle dieser Zeile. Der letzte Parent-Zellenpunkt ist sinnvoller als der erste Eintrittspunkt, weil der Korridor dort beginnen soll, wo der Pointer die Zeile tatsächlich in Richtung Child verlässt.

Der Anchor speichert ausschließlich:

```text
ausgewählten Top-Level-Index
vollständigen MenuPath
Parent {level,item_index}
Child-Level
Anchor-Point
```

Zwischen Aufrufen bleiben keine Menü-, Model- oder Presentation-Zeiger erhalten.

### Safe-Triangle-Geometrie

Der Korridor ist das inklusive Dreieck aus:

```text
letzter owning-Parent-Zelle
        +
naher oberer Ecke des Child-Popups
        +
naher unterer Ecke des Child-Popups
```

Das Child-Rechteck wird aus dem owned Presentation-Snapshot gemessen. Der Classifier wählt die sichtbare vertikale Child-Kante, die horizontal näher am Anchor liegt.

Das ist wichtig, weil verschachtelte Terminal-Popups durch Viewport-Fitting auf beiden Seiten erscheinen können:

```text
Normalfall                     Fitting-Fall

Parent → Child          Child ← Parent
        ^ nahe Kante     nahe Kante ^
```

Es wird keine Annahme „Submenus öffnen immer rechts“ eingebaut.

Der Point-in-Triangle-Test verwendet verbreiterte Koordinaten und `long double`-Kreuzprodukte. `Coordinate` ist signed 32 Bit; das Widening vor Subtraktion und Multiplikation verhindert Signed-Integer-Overflow bei extremen synthetischen Koordinaten. Ein Child mit nur einer Zeile erzeugt eine degenerierte Zielkante; dieser Fall wird als endliches Liniensegment behandelt und nicht als unendliche kollineare Region.

### Struktureller Scope wird bei jedem Motion erneut bewiesen

Eine numerische `{level,item}`-Identität ist nicht global eindeutig. Andere Top-Level-Menüs oder Sibling-Submenu-Pfade können dieselben Zahlen wiederverwenden.

Vor einer Intent-Klassifikation muss der aktuelle Zustand deshalb beweisen:

- Menüinteraktion ist aktiv;
- ein Popup ist geöffnet;
- Popup-Tiefe ist größer als eins;
- `MenuPath` ist gesetzt und nicht leer;
- `popupPath.size() + 1 == popupDepth()`;
- der Frame enthält exakt dieselbe Zahl von Popup-Ebenen;
- das owning Parent-Item existiert im Parent-Snapshot;
- das Child-Popup ist unter der gelieferten Ambiguous-Width-Policy darstellbar;
- Top-Level-Index, vollständiger `MenuPath`, Parent-Identität und Child-Level stimmen weiterhin mit dem gespeicherten Anchor überein.

Scheitert einer dieser Beweise, wird der Anchor gelöscht und `none` zurückgegeben.

### Erreichen des Childs beendet den Transfer

Erreicht Motion irgendeine Zeile des Ziel-Child-Popups, war der Transfer erfolgreich. Der Anchor wird gelöscht und `none` zurückgegeben, weil kein Parent-zu-Child-Übergang mehr geschützt werden muss.

Ebenso löscht eine Bewegung außerhalb des Safe Triangle den Anchor. Ein späterer Punkt kann eine alte Trajektorie nicht wiederbeleben, ohne vorher erneut die owning Parent-Zeile zu betreten und einen frischen Anchor anzulegen.

### Sibling-Zeilen können klassifiziert werden, ohne sie zu mutieren

Ein zentraler Zweck des Classifiers ist die Erkennung eines physischen Punkts, der bereits auf einer Sibling-Zeile des Parent-Popups liegt, geometrisch aber weiterhin innerhalb des Safe Triangle liegt.

In diesem Stadium bleibt das Ergebnis rein beratend:

```text
Pointer liegt physisch auf Sibling-Zeile
        +
Trajektorie bleibt im Child-Korridor
        ↓
TerminalMenuPointerIntentKind::toward_open_submenu
        ↓
Core-State bleibt unverändert
```

Ein späterer Host-Policy-Slice kann entscheiden, ob und wie lange die Sibling-Selection verzögert wird. Dadurch bleibt diese Timing-Entscheidung explizit und wird nicht versteckt in die Geometrieschicht eingebaut.

### Tests

Deterministische Tests prüfen:

- eine Sibling-Parent-Zeile innerhalb eines verbreiterten rechts öffnenden Safe Triangle wird als Bewegung zum offenen Child klassifiziert, ohne Core-State zu verändern;
- dieselbe Klassifikation funktioniert, wenn die finale Presentation-Geometrie das Child links platziert;
- Bewegung aus dem Korridor löscht den Anchor sofort und ein späterer Gap-Punkt kann ihn nicht wiederverwenden;
- der Wechsel in ein anderes Top-Level-Menü mit demselben numerischen `MenuPath {0}` kann den alten Anchor nicht wiederverwenden.

Synthetisch verbreiterte Gaps dienen ausschließlich dazu, das Dreieck in Terminalzellen-Unit-Tests gut prüfbar zu machen. Der semantische Menü-State bleibt real und wird über `MenuInteractionController` aufgebaut.

### Konsequenzen

- Pointer Intent wird ein eigener deterministischer Begriff an der Presentation-Grenze;
- Core bleibt frei von Terminalkoordinaten und Trajektorienheuristiken;
- Links-/Rechts-Platzierung von Submenus wird aus der finalen Frame-Geometrie abgeleitet;
- veraltete numerische Identitäten scheitern durch vollständige strukturelle Scope-Prüfung geschlossen;
- es kommen keine Timer, Threads, Callbacks oder automatischen Menümutationen hinzu;
- späteres Safe-Triangle-Timing kann als Policy-Verbraucher dieses Classifiers umgesetzt werden, ohne Pointer-Hit-Testing umzuschreiben.

### Aufgeschobener Umfang

Diese Entscheidung ergänzt noch nicht:

- Demo-Integration;
- Unterdrückung/Verzögerung der Sibling-Zeilen-Selection;
- einen Safe-Triangle-Timeout;
- adaptive Delays anhand von Pointer-Geschwindigkeit oder Bewegungsverlauf;
- die Verbindung von Pointer Intent mit Deadlines aus `TerminalMenuCloseInteraction`;
- Rendered-/Native-Pointer-Intent-Adapter.
