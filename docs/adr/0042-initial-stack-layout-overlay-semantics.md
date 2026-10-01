# ADR 0042 – Initial StackLayout overlay semantics / Initiale StackLayout-Overlay-Semantik

- **Status:** Accepted
- **Date:** 2026-10-01

## English

### Context

M4 now has one-dimensional `VBox`/`HBox`, a general intrinsic `GridLayout`, and a specialized two-column `FormLayout`. A remaining layout primitive is needed for layered content: status overlays, decorative layers, page content controlled by visibility, and later popup/composite building blocks.

The name `StackLayout` is potentially ambiguous. Some frameworks use "stack" for sequential placement or for an active-page container. SASD already uses VBox/HBox for sequential placement, so reusing that meaning would duplicate an existing concept. Making StackLayout own an active page would also mix layout with page-selection state before any real navigation/page model exists.

### Decision

Introduce public `sasd::ui::StackLayout : Container` with an overlay contract:

- all visible children occupy the same logical client rectangle;
- desired size is the component-wise maximum desired size of visible children, not a sum;
- hidden children collapse from measurement and arrangement;
- MeasurementContext is forwarded to visible children but never stored;
- child coordinates remain parent-relative;
- visual adoption order defines layering: earlier children are below later children;
- existing reverse-order HitTest therefore treats later children as topmost;
- StackLayout itself owns no active-page state, z-index, alignment, margins, padding or per-child offsets;
- applications may implement an initial page-switching pattern by changing child visibility without changing StackLayout semantics.

The implementation is header-only because the first contract has no persistent layout state. This is an implementation choice, not a promise that StackLayout must remain header-only.

### Consequences

StackLayout remains a small backend-neutral geometry primitive. It composes with the existing visual tree, presentation traversal and hit testing instead of introducing a second layering model.

The first slice defines Core semantics only. Terminal and Rendered presentation sinks must explicitly recognize StackLayout as a known structural container before StackLayout is used in end-to-end presentation trees; the project intentionally does not weaken the sinks into accepting arbitrary future Container subclasses.

If real applications later require explicit z-order changes without re-adoption, per-child alignment, clipping policies, or a first-class page selector, those become separate evidenced policies rather than accidental properties of this initial overlay primitive.

## Deutsch

### Kontext

M4 besitzt inzwischen eindimensionale `VBox`/`HBox`, ein allgemeines intrinsisches `GridLayout` und ein spezialisiertes zweispaltiges `FormLayout`. Als weiteres Layout-Primitiv wird geschichteter Inhalt benötigt: Status-Overlays, dekorative Ebenen, über Visibility gesteuerter Seiteninhalt sowie spätere Popup-/Composite-Bausteine.

Der Name `StackLayout` ist potenziell mehrdeutig. Einige Frameworks verwenden "Stack" für sequentielle Anordnung oder für einen Active-Page-Container. SASD besitzt für sequentielle Anordnung bereits VBox/HBox; dieselbe Bedeutung würde daher ein bestehendes Konzept duplizieren. Eine aktive Seite direkt in StackLayout würde außerdem Layout mit Page-Selection-State vermischen, bevor überhaupt ein reales Navigations-/Seitenmodell existiert.

### Entscheidung

Es wird ein öffentliches `sasd::ui::StackLayout : Container` mit Overlay-Vertrag eingeführt:

- alle sichtbaren Kinder belegen denselben logischen Client-Bereich;
- die Wunschgröße ist das komponentenweise Maximum der sichtbaren Child-Wunschgrößen und keine Summe;
- unsichtbare Kinder kollabieren aus Measurement und Arrangement;
- MeasurementContext wird an sichtbare Kinder weitergereicht, aber nicht gespeichert;
- Child-Koordinaten bleiben parent-relativ;
- die visuelle Adoptionsreihenfolge definiert die Ebenen: frühere Kinder liegen unter späteren;
- das bestehende reverse-order HitTest behandelt spätere Kinder damit als oberste Ebene;
- StackLayout besitzt selbst keinen Active-Page-State, Z-Index, Alignment, Margins, Padding oder per-Child-Offsets;
- Anwendungen können ein erstes Page-Switching-Muster durch Änderung der Child-Visibility realisieren, ohne die StackLayout-Semantik zu ändern.

Die Implementierung ist zunächst header-only, weil der erste Vertrag keinen persistenten Layoutzustand besitzt. Das ist eine Implementierungsentscheidung und kein Versprechen, dass StackLayout dauerhaft header-only bleiben muss.

### Konsequenzen

StackLayout bleibt ein kleines backendneutrales Geometrie-Primitiv. Es komponiert mit dem bestehenden Visual Tree, Presentation-Traversal und Hit Testing, statt ein zweites Layering-Modell einzuführen.

Der erste Slice definiert zunächst die Core-Semantik. Terminal- und Rendered-Presentation-Sinks müssen StackLayout explizit als bekannten strukturellen Container anerkennen, bevor StackLayout in End-to-End-Presentation-Bäumen verwendet wird; die Sinks werden bewusst nicht so aufgeweicht, dass sie beliebige zukünftige Container-Subklassen akzeptieren.

Wenn reale Anwendungen später explizite Z-Order-Änderungen ohne Re-Adoption, per-Child-Alignment, Clipping-Policies oder einen First-Class-Page-Selector benötigen, werden diese als eigene evidenzbasierte Policies ergänzt statt zufällige Eigenschaften dieses ersten Overlay-Primitivs zu werden.
