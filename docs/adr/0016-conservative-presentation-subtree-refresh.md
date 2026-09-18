# ADR 0016 – Conservative subtree refresh for geometry and structural presentation changes

**Status:** Accepted  
**Date:** 2026-09-18

## English

### Context

Incremental visual invalidation is sufficient when a Widget changes presentation without changing
where its old representation lives. Text replacement inside unchanged bounds, focus state and enabled
state can normally repaint only that Widget.

Geometry and structural changes are different:

- moving a Widget can leave its old pixels/cells behind;
- resizing can expose content that was previously covered;
- moving a Container changes the absolute position of descendants even when their local bounds remain
  unchanged;
- removing a visual child means that child will not participate in the next presentation traversal and
  therefore cannot erase its own old representation;
- repainting only the changed Widget can erase/overlap clean siblings in the wrong order.

A fully optimized dirty-rectangle/occlusion system would solve these problems, but introducing region
tracking, z-order intersection, clipping and merge policy before the first terminal preview would be a
large premature subsystem.

### Decision

M2 introduces a conservative **presentation subtree refresh** contract in addition to normal visual
invalidation.

The rules are:

- ordinary visual state changes continue to use `invalidateVisual()`;
- geometry changes through `Widget::arrange()` call `invalidatePresentationSubtree()`;
- removing a visual child requests `invalidatePresentationSubtree()` on the old container before the
  next presentation pass;
- a subtree-refresh request also marks normal visual state pending and propagates through visual
  parents to the presentation root;
- `Widget::isSubtreeRefreshPending()` exposes the stronger invalidation to PresentationSink;
- successful `acknowledgeVisualUpdate()` clears both local visual-pending and subtree-refresh state;
- PresentationCoordinator captures the refresh request before acknowledgement and then **forces every
  descendant to be offered to the sink**, even descendants that were otherwise clean;
- forced clean descendants that return `deferred` are converted into ordinary pending updates so a
  later pass cannot forget them;
- if a Widget that owns a pending subtree refresh itself returns `deferred`, its descendants are not
  replayed in that pass because the subtree root was not successfully prepared/synchronized;
- forced traversal follows the same deterministic parent-before-child/adoption order as normal
  presentation;
- `PresentationPassResult::forced` reports how many otherwise-clean Widgets were replayed.

For the current terminal backend, `Window` is the presentation root. A Window whose subtree-refresh
flag is pending clears the off-screen ScreenBuffer; the coordinator then replays every remaining
descendant. This intentionally trades rendering efficiency for correctness during M2.

Normal state updates that do not request subtree refresh remain incremental and do not clear the
Window buffer.

### Rationale

The contract fixes stale old geometry without storing backend-specific previous bounds or renderer
objects in semantic Widgets. It also handles Container movement and child removal, which cannot be
solved reliably by teaching only the moved leaf Widget about its previous rectangle.

A conservative rebuild is easy to reason about and test across terminal, rendered and future native
presentation strategies. It creates a correctness baseline before introducing optimized damage
regions.

Separating ordinary invalidation from subtree refresh preserves the benefits of incremental updates:
typing text, focus changes and most control state changes need not repaint the full Widget tree.

### Alternatives considered

#### Store each Widget's previous rendered absolute rectangle

Deferred/rejected as the primary M2 mechanism. Reparenting, Container movement, overlapping siblings
and multiple presentation roots make a single previous rectangle insufficient without a larger damage
system.

#### Clear only the moved Widget's old rectangle

Rejected. The old area may need underlying siblings to be repainted, and a removed Widget no longer
exists in traversal.

#### Clear the whole terminal Window on every visual update

Rejected. It would make ordinary text/focus changes full-frame repaints and previously caused clean
siblings to disappear when they were not replayed.

#### Implement dirty rectangles, occlusion and region merging now

Deferred. Those are performance optimizations that should be added after the first correct terminal
layout/control path demonstrates where the real costs are.

### Consequences

- geometry changes and visual removal are correct before VBox/HBox begin moving Widgets automatically;
- terminal presentation can clear old geometry without retaining terminal-specific history in Widget;
- some geometry operations repaint the entire presentation subtree and are intentionally not optimal;
- later dirty-region optimization may replace the conservative clearing strategy while keeping the
  stronger semantic distinction between ordinary visual invalidation and geometry/structural damage;
- a presentation sink must be prepared to receive clean Widgets during forced subtree replay.

---

## Deutsch

### Kontext

Normale visuelle Invalidierung reicht aus, wenn sich die Darstellung eines Widgets ändert, ohne dass
seine bisherige Darstellung an einer anderen Stelle liegt. Textänderungen innerhalb gleicher Bounds,
Fokus oder Enabled-State können typischerweise nur dieses Widget neu zeichnen.

Geometrie- und Strukturänderungen sind anders:

- beim Verschieben können alte Pixel/Zellen stehenbleiben;
- beim Verkleinern können bisher verdeckte Inhalte wieder sichtbar werden;
- beim Verschieben eines Containers ändern sich die absoluten Positionen aller Nachfahren, obwohl deren
  lokale Bounds unverändert bleiben;
- ein entferntes visuelles Kind wird im nächsten Presentation-Pass nicht mehr traversiert und kann
  seine alte Darstellung daher nicht selbst löschen;
- nur das geänderte Widget neu zu zeichnen kann überlappende cleane Geschwister in falscher Reihenfolge
  beschädigen.

Ein vollständiges Dirty-Rectangle-/Occlusion-System könnte diese Fälle optimieren. Region-Tracking,
Z-Order-Schnittmengen, Clipping und Merge-Policy bereits vor dem ersten Terminal-Preview einzuführen
wäre jedoch ein großer voreiliger Subsystem-Schritt.

### Entscheidung

M2 führt zusätzlich zur normalen visuellen Invalidierung einen konservativen
**Presentation-Subtree-Refresh** ein.

Es gelten folgende Regeln:

- normale visuelle Zustandsänderungen verwenden weiterhin `invalidateVisual()`;
- Geometrieänderungen über `Widget::arrange()` rufen `invalidatePresentationSubtree()` auf;
- das Entfernen eines visuellen Kindes fordert am alten Container vor dem nächsten Presentation-Pass
  einen `invalidatePresentationSubtree()` an;
- ein Subtree-Refresh setzt zugleich normalen Visual-Pending-State und propagiert über visuelle Parents
  bis zum Presentation-Root;
- `Widget::isSubtreeRefreshPending()` macht die stärkere Invalidierung für PresentationSink sichtbar;
- erfolgreiches `acknowledgeVisualUpdate()` löscht lokalen Visual-Pending- und Subtree-Refresh-State;
- PresentationCoordinator merkt sich den Refresh vor dem Acknowledge und bietet danach **jeden
  Descendant erneut an**, auch sonst cleane Widgets;
- erzwungen angebotene cleane Descendants werden bei `deferred` in normale Pending-Updates
  umgewandelt, damit ein späterer Pass sie nicht vergisst;
- liefert das Widget, das selbst einen Subtree-Refresh besitzt, `deferred`, werden seine Descendants
  in diesem Pass nicht wiederholt, weil der Subtree-Root nicht erfolgreich vorbereitet/synchronisiert
  wurde;
- Forced Traversal verwendet dieselbe deterministische Parent-vor-Child-/Adoptionsreihenfolge wie
  normale Presentation;
- `PresentationPassResult::forced` zählt sonst cleane, erzwungen wiederholte Widgets.

Für das aktuelle Terminal-Backend ist `Window` der Presentation-Root. Besitzt Window einen pending
Subtree-Refresh, wird der Off-Screen-`ScreenBuffer` geleert; danach spielt der Coordinator alle
verbleibenden Descendants erneut ein. Das tauscht während M2 bewusst Performance gegen Korrektheit.

Normale Zustandsupdates ohne Subtree-Refresh bleiben inkrementell und löschen den Window-Buffer nicht.

### Begründung

Der Vertrag beseitigt alte Geometrie, ohne backendabhängige Previous-Bounds oder Renderer-Objekte in
semantischen Widgets zu speichern. Er löst außerdem Container-Movement und Child-Removal, die mit
einem Previous-Rectangle nur am verschobenen Leaf nicht zuverlässig lösbar sind.

Ein konservativer Rebuild ist leicht zu verstehen und über Terminal, Rendered und spätere Native
Presentation hinweg testbar. Er schafft zuerst eine Korrektheitsbasis; optimierte Damage-Regionen
können später darauf aufbauen.

Die Trennung von normaler Invalidierung und Subtree-Refresh erhält trotzdem inkrementelle Vorteile:
Textänderungen, Fokus und die meisten Control-State-Änderungen müssen nicht den gesamten Widget-Baum
neu rendern.

### Betrachtete Alternativen

#### Pro Widget das zuletzt gerenderte absolute Rechteck speichern

Als primärer M2-Mechanismus verschoben/verworfen. Reparenting, Container-Movement, überlappende
Geschwister und mehrere Presentation-Roots machen ein einzelnes Previous-Rectangle ohne größeres
Damage-System unzureichend.

#### Nur das alte Rechteck des verschobenen Widgets löschen

Verworfen. Unter dem alten Bereich können Geschwister neu sichtbar werden; ein entferntes Widget
existiert außerdem im nächsten Traversal nicht mehr.

#### Bei jedem visuellen Update das komplette Terminal-Window löschen

Verworfen. Normale Text-/Fokusänderungen würden unnötig Full-Frame-Repaints auslösen; außerdem führte
diese Strategie bereits dazu, dass cleane Geschwister verschwanden, wenn sie nicht wiederholt wurden.

#### Dirty Rectangles, Occlusion und Region-Merging sofort implementieren

Verschoben. Das sind Performance-Optimierungen, die erst nach einem korrekten Terminal-Layout-/Control-
Pfad anhand realer Kosten entwickelt werden sollen.

### Konsequenzen

- Geometrieänderungen und visuelle Entfernung sind korrekt, bevor VBox/HBox Widgets automatisch
  verschieben;
- Terminal-Presentation kann alte Geometrie entfernen, ohne terminalspezifische History in Widget zu
  speichern;
- manche Geometrieoperationen rendern bewusst den gesamten Presentation-Subtree neu und sind noch
  nicht optimal;
- spätere Dirty-Region-Optimierungen dürfen die konservative Clearing-Strategie ersetzen, während die
  semantische Trennung zwischen normaler Visual-Invalidierung und Geometrie-/Strukturschaden erhalten
  bleibt;
- ein PresentationSink muss während Forced-Subtree-Replay auch cleane Widgets verarbeiten können.
