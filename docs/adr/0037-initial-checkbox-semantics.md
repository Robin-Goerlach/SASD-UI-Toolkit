# ADR 0037 – Initial CheckBox semantics / Initiale CheckBox-Semantik

- **Status:** Accepted
- **Date:** 2026-09-30

## English

### Context

M4 introduces the first form controls beyond Button and TextField. CheckBox is a useful first slice because it exercises persistent control state, keyboard input, pointer capture and presentation without requiring popup windows, item models or selection lists.

The toolkit already separates semantic Widget state from backend presentation. Button established an armed primary-pointer gesture and terminal-compatible key-down activation, while PointerRouter owns capture independently from the control. CheckBox should reuse those interaction invariants without making terminal markers or rendered indicator geometry part of Core.

### Decision

The initial public `sasd::ui::CheckBox` is a two-state semantic Widget with UTF-8 caption and `TextStyle`, persistent boolean checked state, focusability by default, unmodified Space as the keyboard toggle gesture, primary-pointer press/capture/release semantics matching Button, transient pressed state independent from checked state, `setChecked(bool)`, `toggle()`, a synchronous `setOnCheckedChanged()` callback, and a `MeasurementContext::measureCheckBox()` hook whose default falls back to text measurement.

Programmatic state changes remain valid while the control is disabled or hidden. Enabled/visible state gates user input, not application model assignment. Enter is deliberately not consumed by CheckBox so later form/default-action semantics can use it.

The first contract is intentionally not tri-state. Indeterminate state can be introduced later when a real application requirement justifies the additional public-state and presentation complexity.

### Consequences

- Core remains free of terminal marker, SDL or native-control geometry.
- Terminal, rendered and later native backends can choose appropriate indicator presentation while sharing one semantic state and input contract.
- Checked-state changes invalidate presentation but not intrinsic measurement.
- Backends can account for indicator chrome through `measureCheckBox()` without forcing existing custom MeasurementContext implementations to add an override immediately.
- Button and CheckBox deliberately duplicate a small amount of pointer-gesture code. Extracting an interaction base class before another control demonstrates a stable common contract would be premature.

## Deutsch

### Kontext

M4 führt die ersten Formular-Steuerelemente über Button und TextField hinaus ein. CheckBox eignet sich als erster vertikaler Schnitt, weil damit persistenter Control-Zustand, Tastatur, Pointer-Capture und Darstellung geprüft werden können, ohne bereits Popup-Fenster, Item-Modelle oder Auswahllisten zu benötigen.

Das Toolkit trennt semantischen Widget-Zustand bereits von der Backend-Darstellung. Button hat eine bewaffnete Primary-Pointer-Geste und terminaltaugliche Aktivierung auf Key-Down etabliert; PointerRouter besitzt Capture unabhängig vom Control. CheckBox soll diese Invarianten wiederverwenden, ohne Terminal-Markierungen oder gerenderte Indikatorgeometrie in den Core zu ziehen.

### Entscheidung

Die erste öffentliche `sasd::ui::CheckBox` ist ein semantisches Zwei-Zustands-Widget mit UTF-8-Beschriftung und `TextStyle`, persistentem booleschem Checked-Zustand, standardmäßiger Fokussierbarkeit, unmodifiziertem Space als Umschaltgeste, Primary-Pointer-Press/Capture/Release entsprechend Button, transientem Pressed-Zustand unabhängig von Checked, `setChecked(bool)`, `toggle()`, synchronem `setOnCheckedChanged()`-Callback und einem `MeasurementContext::measureCheckBox()`-Hook mit Textmessung als kompatiblem Default.

Programmatische Zustandsänderungen bleiben auch bei deaktiviertem oder unsichtbarem Control zulässig. Enabled/Visible begrenzen Benutzereingaben, nicht Änderungen durch das Anwendungsmodell. Enter wird bewusst nicht von CheckBox konsumiert, damit spätere Default-Action-/Form-Semantik es verwenden kann.

Der erste Vertrag ist bewusst nicht tri-state. Ein Indeterminate-Zustand kann später ergänzt werden, wenn ein realer Anwendungsfall die zusätzliche öffentliche Zustands- und Darstellungs-Komplexität rechtfertigt.

### Konsequenzen

- Der Core bleibt frei von Terminal-Markern, SDL und nativer Control-Geometrie.
- Terminal-, Rendered- und spätere Native-Backends können passende Indikatoren darstellen und dennoch denselben semantischen Zustand und Eingabevertrag verwenden.
- Checked-Änderungen invalidieren Darstellung, nicht intrinsische Messung.
- Backends können Indikator-Chrome über `measureCheckBox()` berücksichtigen, ohne bestehende benutzerdefinierte MeasurementContext-Implementierungen sofort zu einem Override zu zwingen.
- Button und CheckBox duplizieren bewusst einen kleinen Teil der Pointer-Gestenlogik; eine gemeinsame Interaktions-Basisklasse wäre vor weiterer praktischer Evidenz verfrüht.
