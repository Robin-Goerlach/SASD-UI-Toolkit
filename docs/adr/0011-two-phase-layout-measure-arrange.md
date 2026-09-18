# ADR 0011 – Two-phase layout measurement and arrangement

**Status:** Accepted  
**Date:** 2026-09-18

## English

### Context

SASD UI Toolkit must use one semantic layout foundation across terminal, rendered desktop and future native backends. These environments do not share one physical measurement model: a terminal commonly works in character cells, while desktop backends may work in logical device-independent units and font metrics.

A child also needs to express how much space it would like before a parent can decide where that child will finally be placed. A one-step `setBounds()` model is insufficient for containers such as `VBox`, `HBox`, forms or grids because the parent would have to know child-specific sizing rules itself.

### Decision

The platform-neutral `Widget` contract uses two explicit layout phases:

1. **Measure** – `measure(MeasureConstraints)` asks a widget for its desired size inside a parent-provided minimum/maximum interval.
2. **Arrange** – `arrange(Rect)` assigns the final logical rectangle after the parent has made its layout decision.

The following rules apply:

- `SizeConstraints` describe the widget's own minimum, preferred and maximum size hints.
- `MeasureConstraints` describe only the space interval offered by the parent; they deliberately have no preferred value.
- A widget's `onMeasure()` produces an intrinsic/content size.
- The intrinsic size is constrained first by the widget's own size hints and then by the parent's measure constraints. The parent therefore has final authority over actually available space.
- The resulting size is exposed as `desiredSize()`.
- Measurement is cached for identical parent constraints until `invalidateMeasure()` marks it stale.
- Measurement invalidation propagates through the **visual parent** chain, because a descendant size change may change every ancestor layout.
- Adding or removing a visual child invalidates its container's cached measurement.
- `arrange()` accepts zero-sized rectangles but rejects negative extents.
- Final bounds are stored before `onArrange()` runs so a container can query its own final geometry while arranging children.
- All units remain logical/backend-neutral; the contract does not assume pixels.
- Direct/manual `setBounds()` follows the same arrangement path rather than bypassing layout hooks.

M1 does **not** yet define:

- `VBox`, `HBox`, grid or form allocation algorithms;
- margins, padding or spacing APIs;
- whether hidden widgets reserve layout space;
- inherited visibility effects on layout;
- backend-specific text measurement services;
- a public layout-manager class hierarchy.

Those decisions are deferred until real controls and the terminal backend can validate them.

### Rationale

A two-phase contract separates two different questions:

- **How much space would this widget like under these limits?**
- **Where and at what final size did the parent place it?**

This supports intrinsic text/control measurement without forcing parents to understand individual widget implementations. It also works for both terminal cells and desktop logical units.

Keeping parent measurement constraints separate from widget size hints prevents an important semantic ambiguity. A `TextField` may prefer 30 columns, while its parent may only have 20 available; preference cannot manufacture space that the parent does not have.

### Alternatives considered

#### One-step bounds assignment only

Rejected as the common layout foundation. It is useful for absolute/manual positioning but does not let a parent query child requirements before allocation.

#### Pixel-based measurement

Rejected because terminal is a first-class backend and native/rendered desktop backends may use different logical metrics.

#### Backend-owned layout

Rejected for normal toolkit layout. `VBox`, `HBox` and similar semantic layout behavior must be shared rather than separately reimplemented by every backend.

#### Full layout-manager hierarchy in M1

Deferred. The project first needs a minimal measure/arrange contract that can be tested headlessly. Concrete layout containers/managers should be added only when M2 widgets exercise real cases.

### Consequences

- Concrete widgets implement content sizing through `onMeasure()`.
- Layout-aware containers can implement child placement through `onArrange()`.
- State changes that alter intrinsic size must call `invalidateMeasure()`.
- Tests can validate measurement, arrangement and invalidation without a display server.
- Future backends provide metrics where required, but do not redefine the core layout lifecycle.

---

## Deutsch

### Kontext

Das SASD UI Toolkit benötigt eine gemeinsame semantische Layout-Grundlage für Terminal, gerenderten Desktop und spätere native Backends. Diese Umgebungen besitzen jedoch kein gemeinsames physisches Messmodell: Ein Terminal arbeitet typischerweise mit Zeichenzellen, Desktop-Backends dagegen beispielsweise mit logischen geräteunabhängigen Einheiten und Schriftmetriken.

Außerdem muss ein Kind zunächst ausdrücken können, wie viel Platz es benötigt, bevor ein Parent entscheiden kann, wo und wie groß dieses Kind endgültig angeordnet wird. Ein reines einstufiges `setBounds()`-Modell reicht für Container wie `VBox`, `HBox`, Formulare oder Grids nicht aus.

### Entscheidung

Der plattformneutrale `Widget`-Vertrag verwendet zwei explizite Layoutphasen:

1. **Measure** – `measure(MeasureConstraints)` ermittelt die gewünschte Größe innerhalb eines vom Parent angebotenen Minimum-/Maximum-Bereichs.
2. **Arrange** – `arrange(Rect)` weist nach der Layoutentscheidung des Parents das endgültige logische Rechteck zu.

Dabei gelten folgende Regeln:

- `SizeConstraints` beschreiben die eigenen Minimum-, Preferred- und Maximum-Größenhinweise eines Widgets.
- `MeasureConstraints` beschreiben ausschließlich den vom Parent angebotenen Größenbereich und besitzen bewusst keinen Preferred-Wert.
- `onMeasure()` liefert die intrinsische bzw. inhaltsabhängige Wunschgröße.
- Diese Größe wird zuerst durch die eigenen Größenhinweise des Widgets und danach durch die Parent-Constraints begrenzt. Der Parent hat damit die letzte Autorität über tatsächlich verfügbaren Platz.
- Das Ergebnis steht als `desiredSize()` zur Verfügung.
- Messungen werden für identische Parent-Constraints gecacht, bis `invalidateMeasure()` sie ungültig macht.
- Eine Messinvalidierung propagiert über den **visuellen Parent-Pfad**, weil eine Größenänderung eines Nachfahren das Layout aller Vorfahren beeinflussen kann.
- Das Hinzufügen oder Entfernen eines visuellen Kindes invalidiert die Messung des Containers.
- `arrange()` erlaubt Rechtecke mit Größe null, lehnt aber negative Ausdehnungen ab.
- Die finalen Bounds werden vor `onArrange()` gespeichert, damit ein Container beim Anordnen seiner Kinder seine eigene endgültige Geometrie abfragen kann.
- Alle Maße bleiben logisch und backendneutral; Pixel werden nicht vorausgesetzt.
- Direktes/manuelles `setBounds()` verwendet denselben Arrange-Pfad und umgeht die Layout-Hooks nicht.

M1 legt bewusst **noch nicht** fest:

- die Platzverteilungsalgorithmen von `VBox`, `HBox`, Grid oder Form;
- APIs für Margin, Padding oder Spacing;
- ob unsichtbare Widgets Layoutplatz reservieren;
- vererbte Sichtbarkeitsregeln für Layout;
- backendabhängige Textmessdienste;
- eine öffentliche Hierarchie von Layout-Managern.

Diese Punkte werden verschoben, bis reale Controls und das Terminal-Backend die Semantik praktisch validieren können.

### Begründung

Der zweiphasige Vertrag trennt zwei unterschiedliche Fragen:

- **Wie viel Platz möchte dieses Widget unter den aktuellen Grenzen?**
- **Wo und in welcher endgültigen Größe hat der Parent es angeordnet?**

Dadurch können Widgets Text oder andere Inhalte intrinsisch messen, ohne dass ein Parent die interne Größenlogik jedes Controls kennen muss. Derselbe Vertrag funktioniert für Terminal-Zellen und logische Desktop-Einheiten.

Die Trennung von Parent-`MeasureConstraints` und eigenen `SizeConstraints` verhindert außerdem eine wichtige Mehrdeutigkeit. Ein `TextField` kann beispielsweise 30 Spalten bevorzugen, obwohl sein Parent nur 20 zur Verfügung hat; ein Preferred-Wert darf keinen nicht vorhandenen Platz erzeugen.

### Betrachtete Alternativen

#### Nur einstufige Bounds-Zuweisung

Als allgemeine Layout-Grundlage verworfen. Für absolute/manuelle Positionierung ist sie nützlich, aber ein Parent kann damit die Anforderungen seiner Kinder nicht vor der Platzverteilung abfragen.

#### Pixelbasierte Messung

Verworfen, weil das Terminal ein First-Class-Backend ist und auch Desktop-Backends unterschiedliche logische Metriken verwenden können.

#### Layout vollständig im Backend

Für normales Toolkit-Layout verworfen. Semantische Layouts wie `VBox` und `HBox` sollen gemeinsam implementiert und nicht in jedem Backend erneut erfunden werden.

#### Vollständige Layout-Manager-Hierarchie bereits in M1

Verschoben. Zuerst wird der kleine Measure/Arrange-Vertrag headless validiert. Konkrete Layout-Container oder -Manager kommen hinzu, sobald M2-Widgets reale Anwendungsfälle liefern.

### Konsequenzen

- Konkrete Widgets implementieren inhaltsabhängige Größenberechnung über `onMeasure()`.
- Layoutfähige Container können Kinder über `onArrange()` positionieren.
- Zustandsänderungen, die die intrinsische Größe beeinflussen, müssen `invalidateMeasure()` auslösen.
- Measurement, Arrangement und Invalidierung sind ohne Display Server testbar.
- Spätere Backends liefern bei Bedarf Metriken, definieren aber nicht den Core-Layout-Lebenszyklus neu.
