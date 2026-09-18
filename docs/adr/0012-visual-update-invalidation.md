# ADR 0012 – Separate visual update invalidation from layout invalidation

**Status:** Accepted  
**Date:** 2026-09-18

## English

### Context

A widget can change visually without changing how much space it needs. Examples include keyboard focus, enabled/disabled presentation, pressed/hovered state, caret visibility, selection highlighting or a theme-related appearance change.

If every visual change invalidated measurement, terminal and desktop backends would repeatedly perform unnecessary layout work. Conversely, layout invalidation alone is not sufficient to tell a presentation backend that a widget whose size stayed unchanged still needs to be redrawn or synchronized.

The toolkit also supports more than one presentation strategy. A visual update may mean repainting terminal cells, redrawing into a rendered surface, or synchronizing state to a native operating-system control. The core therefore needs semantics that do not assume a particular renderer.

### Decision

The M1 core keeps **layout invalidation** and **visual update invalidation** as separate widget state.

Layout remains represented by:

- `invalidateMeasure()`;
- `isMeasureValid()`;
- `measure()` and `desiredSize()`.

Visual presentation uses a separate backend-neutral state:

- newly created widgets start with a visual update pending because they have never been synchronized to a presentation backend;
- `invalidateVisual()` marks the widget as requiring presentation synchronization;
- visual invalidation propagates through the **visual parent** path;
- `isVisualUpdatePending()` reports the state;
- `acknowledgeVisualUpdate()` clears the state only for the widget that was actually synchronized.

Visual invalidation does **not** automatically invalidate measurement.

Current M1 state changes use the following rules:

- visibility changes invalidate both measurement and visual presentation;
- enabled-state changes invalidate visual presentation but not measurement;
- focus transitions invalidate visual presentation but not measurement;
- a changed arranged rectangle invalidates visual presentation but does not invalidate the already computed desired size;
- adding or removing a visual child invalidates both the container's measurement and visual presentation;
- changing only `focusable` does not imply a visual change by itself.

Acknowledging an ancestor does not acknowledge descendants. A renderer or backend coordinator must acknowledge each widget whose current state it actually synchronized.

Visual invalidation is intentionally propagated even when the originating widget is already dirty. This preserves correctness if an ancestor was acknowledged independently while a descendant remained pending.

M1 does **not** yet define:

- dirty rectangles or region merging;
- frame scheduling;
- double buffering;
- a renderer object hierarchy;
- paint commands or a display list;
- terminal cell-diff algorithms;
- whether a native peer can satisfy an update immediately;
- clipping and occlusion policy.

Those belong to later backend/rendering work.

### Rationale

Layout work and presentation work have different costs and triggers. Separating them allows a focused button to redraw its focus indication without re-measuring the entire widget tree.

Using the visual parent relation for propagation matches the presentation hierarchy rather than the ownership hierarchy. A non-visual owned Component should not make a container visually dirty merely because it shares lifetime ownership.

Keeping acknowledgement local is conservative and observable. A parent update cannot falsely claim that descendants were synchronized unless the presentation coordinator actually processed them.

### Alternatives considered

#### One generic dirty flag

Rejected because it cannot distinguish expensive size/layout recomputation from a presentation-only update.

#### Always invalidate layout for visual changes

Rejected because common interaction changes such as focus or enabled state usually do not alter intrinsic size.

#### Push redraw requests directly into Backend from Widget

Deferred/rejected for the M1 core. Widgets should not depend on one selected backend or renderer, and doing so would blur the semantic tree with platform execution. A later coordinator can observe pending state and decide how a particular backend consumes it.

#### Clear an entire subtree when an ancestor is acknowledged

Rejected as the core default because an ancestor can be synchronized without every descendant necessarily succeeding or being visited.

### Consequences

- concrete widgets should call `invalidateMeasure()` when intrinsic size can change;
- concrete widgets should call `invalidateVisual()` when presentation can change;
- a content change may legitimately call both;
- terminal/rendered/native backends can share the same pending-update semantics while consuming them differently;
- repeated visual changes can be coalesced until the presentation layer performs an update.

---

## Deutsch

### Kontext

Ein Widget kann sich optisch ändern, ohne dass sich sein Platzbedarf ändert. Beispiele sind Tastaturfokus, Enabled-/Disabled-Darstellung, Pressed-/Hover-Zustand, Cursor/Caret, Selektionshervorhebung oder eine rein visuelle Theme-Änderung.

Würde jede optische Änderung automatisch die Messung ungültig machen, müssten Terminal- und Desktop-Backends unnötig häufig Layout neu berechnen. Umgekehrt reicht Layout-Invalidierung allein nicht aus, um einem Darstellungs-Backend mitzuteilen, dass ein Widget trotz unveränderter Größe neu gezeichnet oder synchronisiert werden muss.

Außerdem unterstützt das Toolkit unterschiedliche Darstellungsstrategien. Ein visuelles Update kann das Neuzeichnen von Terminal-Zellen, das Rendern auf eine Surface oder das Synchronisieren eines nativen Betriebssystem-Controls bedeuten. Der Core darf deshalb keinen konkreten Renderer voraussetzen.

### Entscheidung

Der M1-Core behandelt **Layout-Invalidierung** und **visuelle Update-Invalidierung** als getrennte Widget-Zustände.

Layout bleibt über folgende Mechanismen abgebildet:

- `invalidateMeasure()`;
- `isMeasureValid()`;
- `measure()` und `desiredSize()`.

Für die Darstellung existiert ein separater backendneutraler Zustand:

- neu erzeugte Widgets starten mit ausstehendem visuellen Update, weil sie noch nie mit einem Darstellungs-Backend synchronisiert wurden;
- `invalidateVisual()` markiert ein Widget als darstellungsseitig veraltet;
- visuelle Invalidierung propagiert über den **visuellen Parent-Pfad**;
- `isVisualUpdatePending()` meldet diesen Zustand;
- `acknowledgeVisualUpdate()` bestätigt ausschließlich das Widget, das tatsächlich synchronisiert wurde.

Visuelle Invalidierung macht die Messung **nicht automatisch** ungültig.

Für den aktuellen M1-Core gelten folgende Regeln:

- Visibility-Änderungen invalidieren Messung und Darstellung;
- Enabled-/Disabled-Änderungen invalidieren die Darstellung, aber nicht die Messung;
- Fokuswechsel invalidieren die Darstellung, aber nicht die Messung;
- ein geändertes final angeordnetes Rechteck invalidiert die Darstellung, aber nicht die bereits berechnete Wunschgröße;
- Hinzufügen oder Entfernen eines visuellen Kindes invalidiert beim Container sowohl Messung als auch Darstellung;
- eine reine Änderung von `focusable` gilt nicht automatisch als visuelle Änderung.

Das Bestätigen eines Parents bestätigt seine Kinder nicht. Ein Renderer bzw. Backend-Koordinator muss jedes Widget bestätigen, dessen aktueller Zustand tatsächlich synchronisiert wurde.

Visuelle Invalidierung propagiert bewusst auch dann erneut nach oben, wenn das Ausgangs-Widget bereits dirty ist. Dadurch bleibt der Zustand korrekt, wenn ein Parent zwischenzeitlich bestätigt wurde, während ein Kind weiterhin ausstehend war.

M1 legt bewusst **noch nicht** fest:

- Dirty Rectangles oder Region-Merging;
- Frame Scheduling;
- Double Buffering;
- eine Renderer-Klassenhierarchie;
- Paint Commands oder Display Lists;
- Terminal-Zell-Diff-Algorithmen;
- ob ein Native Peer ein Update unmittelbar bestätigen kann;
- Clipping- und Occlusion-Regeln.

Diese Punkte gehören in die späteren Backend-/Rendering-Schritte.

### Begründung

Layout-Arbeit und Darstellungs-Arbeit besitzen unterschiedliche Auslöser und Kosten. Durch die Trennung kann beispielsweise ein fokussierter Button seine Fokusdarstellung ändern, ohne den gesamten Widget-Baum neu vermessen zu müssen.

Die Propagation über den visuellen Parent entspricht dem Darstellungsbaum und nicht dem Ownership-Baum. Eine nichtvisuelle besessene Component darf einen Container nicht allein aufgrund gemeinsamer Lebensdauer visuell dirty machen.

Die lokale Bestätigung ist bewusst konservativ: Ein Parent-Update behauptet nicht automatisch, dass alle Nachfahren erfolgreich synchronisiert wurden.

### Betrachtete Alternativen

#### Ein einziges allgemeines Dirty-Flag

Verworfen, weil damit teure Größen-/Layout-Neuberechnung nicht von einem reinen Darstellungsupdate unterschieden werden kann.

#### Bei jeder optischen Änderung Layout invalidieren

Verworfen, weil häufige Interaktionszustände wie Fokus oder Enabled normalerweise die intrinsische Größe nicht ändern.

#### Widget ruft Backend direkt für Redraw auf

Für M1 verworfen bzw. verschoben. Widgets sollen weder von einem ausgewählten Backend noch von einem konkreten Renderer abhängen. Ein späterer Koordinator kann den Pending-Zustand auswerten und backendabhängig konsumieren.

#### Beim Bestätigen eines Parents den kompletten Subtree bestätigen

Als Core-Standard verworfen, weil ein Parent synchronisiert sein kann, ohne dass jeder Nachfahre tatsächlich erfolgreich verarbeitet wurde.

### Konsequenzen

- konkrete Widgets rufen `invalidateMeasure()`, wenn sich ihre intrinsische Größe ändern kann;
- konkrete Widgets rufen `invalidateVisual()`, wenn sich nur oder zusätzlich ihre Darstellung ändert;
- eine Inhaltsänderung darf beide Invalidierungen auslösen;
- Terminal-, Rendered- und Native-Backends können dieselbe Pending-Semantik unterschiedlich konsumieren;
- mehrere visuelle Änderungen können bis zum nächsten Darstellungsupdate zusammengefasst werden.
