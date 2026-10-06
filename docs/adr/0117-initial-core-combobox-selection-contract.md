# ADR 0117 – Initial Core ComboBox selection contract

**Status:** Accepted  
**Date:** 2026-10-06

## English

### Context

M4 lists `ComboBox` as the next unfinished form control after `CheckBox` and `RadioButton`. The toolkit already has backend-neutral focus, semantic key events, measurement services and presentation invalidation, while M5 deliberately reserves `ListModel`/selection-model infrastructure for data-heavy widgets.

A ComboBox creates a design tension at this point. A complete desktop-style implementation combines several concerns:

- owned or model-backed item data;
- committed selection;
- collapsed-control measurement/presentation;
- drop-down open/close state;
- popup placement and clipping;
- pointer hit testing/capture outside the collapsed Widget bounds;
- keyboard navigation and commit/cancel rules;
- later native-peer behavior.

Implementing all of those in one slice would either move terminal/rendered popup geometry into Core or force the M5 model/view abstraction to be designed prematurely around a single control.

### Decision

Introduce a first backend-neutral, non-editable `ComboBox` with a deliberately small semantic contract:

1. The control owns a `std::vector<std::string>` of UTF-8 items.
2. Selection is `std::optional<std::size_t>`; no selection is a valid state.
3. The complete item collection may be replaced with `setItems()` and extended with `appendItem()`.
4. Replacing a different collection clears selection rather than reusing an old numeric index against unrelated data.
5. Appending preserves selection because existing indices are unchanged.
6. `setSelectedIndex()` validates indices and throws `std::out_of_range` before mutation for an invalid non-empty index.
7. Selection callbacks run only after collection/selection/invalidation state is internally coherent. The callback is copied before invocation and the control is not touched afterwards, matching the existing lifetime-safety rule used by Button/CheckBox/RadioButton.
8. The control is focusable and handles unmodified Up/Down/Home/End. Navigation is non-wrapping. With no current selection, Down/Home select the first item and Up/End select the last item.
9. Key-down changes selection; matching key-up is consumed without a second mutation so terminal and desktop input agree.
10. Pointer/drop-down opening, popup rows and commit/cancel preview state are intentionally deferred. The foundation does not invent a temporary click-to-cycle behavior that would later need to be removed.

### Measurement

Add a compatibility-default `MeasurementContext::measureComboBox(std::string_view)` method. Existing/custom contexts continue to compile because the default delegates to `measureText()`.

`ComboBox` measures the empty-text chrome baseline and every owned item through this semantic hook, then takes the component-wise maximum. Consequently:

- backend-specific border/padding/drop-indicator metrics remain outside Core;
- the control remains measurable even with zero items;
- changing selection does not invalidate measurement or cause layout jitter between short and long choices;
- changing the collection does invalidate measurement.

### Why owned items before ListModel

ADR 0008 still governs the long-term Model/View direction. This slice does not reject model-backed ComboBox data. It avoids designing `ListModel` early merely because one small form control needs a useful first implementation.

The owned-item API is sufficient for common form choices and gives real backend work a stable selection contract to validate. A later model-backed API can be added once M5 provides evidence about observation, identity, mutation and virtualization requirements.

### Consequences

Positive:

- M4 gains a real, testable Core ComboBox foundation without terminal/rendered/native types.
- Selection semantics are deterministic and fail closed across complete data replacement.
- Measurement is stable across selection changes and extensible per backend.
- Backend popup work can be developed as a later vertical slice instead of being hidden inside Core.

Deferred intentionally:

- editable ComboBox text;
- open/closed drop-down state;
- pointer opening and popup item hit testing;
- highlighted-but-not-yet-committed preview selection;
- Escape rollback semantics;
- type-ahead search;
- model-backed/virtualized item sources;
- Terminal and Rendered presentation/native peers.

These are not exclusions. They are separate contracts that should be added only with tests and concrete backend consumers.

---

## Deutsch

### Kontext

M4 führt `ComboBox` als nächstes noch offenes Formular-Control nach `CheckBox` und `RadioButton`. Das Toolkit besitzt bereits backend-unabhängigen Fokus, semantische Key-Events, Messdienste und Presentation-Invalidierung. Gleichzeitig ist die `ListModel`-/Selection-Model-Infrastruktur bewusst für M5 und datenintensive Widgets vorgesehen.

Eine vollständige Desktop-ComboBox vermischt zu diesem Zeitpunkt mehrere Themen:

- eigene oder modellgebundene Eintragsdaten;
- festgeschriebene Auswahl;
- Messung und Darstellung des geschlossenen Controls;
- Open/Close-Zustand der Drop-down-Liste;
- Popup-Platzierung und Clipping;
- Pointer-Hit-Testing außerhalb der Bounds des geschlossenen Widgets;
- Tastatur-Navigation sowie Commit/Cancel-Regeln;
- spätere Native-Peers.

Alles in einem Schritt zu implementieren würde entweder Terminal-/Rendered-Geometrie in den Core ziehen oder die M5-Model/View-Abstraktion zu früh um ein einzelnes Control herum festlegen.

### Entscheidung

Wir führen zunächst eine backend-unabhängige, nicht editierbare `ComboBox` mit bewusst kleinem semantischem Vertrag ein:

1. Das Control besitzt einen eigenen `std::vector<std::string>` mit UTF-8-Einträgen.
2. Die Auswahl ist `std::optional<std::size_t>`; keine Auswahl ist ein gültiger Zustand.
3. Die vollständige Liste kann mit `setItems()` ersetzt und mit `appendItem()` erweitert werden.
4. Beim Ersetzen durch eine andere Liste wird die Auswahl gelöscht, statt einen alten numerischen Index gegen inhaltlich andere Daten neu zu interpretieren.
5. Anhängen erhält die Auswahl, weil vorhandene Indizes unverändert bleiben.
6. `setSelectedIndex()` prüft den Index und wirft bei einem ungültigen nichtleeren Index vor jeder Mutation `std::out_of_range`.
7. Selection-Callbacks laufen erst, wenn Itemliste, Auswahl und Invalidierungszustand konsistent sind. Der Handler wird vor dem Aufruf kopiert und danach wird das Control nicht mehr angefasst. Das entspricht der bereits bei Button/CheckBox/RadioButton verwendeten Lifetime-Regel.
8. Das Control ist fokussierbar und verarbeitet unmodifiziertes Up/Down/Home/End ohne Wrap-around. Ohne bestehende Auswahl wählen Down/Home den ersten sowie Up/End den letzten Eintrag.
9. Key-down verändert die Auswahl; ein vorhandenes Key-up wird nur konsumiert, damit Terminal- und Desktop-Eingabe dieselbe Semantik besitzen.
10. Pointer-/Drop-down-Öffnung, Popup-Zeilen sowie Preview-/Commit-/Cancel-Zustand bleiben bewusst einem Folge-Slice vorbehalten. Es wird keine provisorische Click-to-cycle-Semantik eingeführt, die später wieder entfernt werden müsste.

### Messung

`MeasurementContext` erhält `measureComboBox(std::string_view)` mit kompatiblem Default auf `measureText()`. Bestehende und eigene MeasurementContexts bleiben dadurch source-kompatibel.

`ComboBox` misst zunächst die Empty-Text-Chrome-Basis und danach jeden eigenen Eintrag über diesen semantischen Hook. Aus allen Ergebnissen wird komponentenweise das Maximum gebildet. Dadurch gilt:

- Backend-spezifische Border-/Padding-/Drop-Indikator-Metrik bleibt außerhalb des Core;
- auch eine leere ComboBox besitzt eine messbare Control-Größe;
- reine Selection-Änderungen invalidieren die Messung nicht und erzeugen kein Layout-Springen zwischen kurzen und langen Einträgen;
- Änderungen der Itemliste invalidieren die Messung.

### Warum zunächst eigene Einträge statt ListModel

ADR 0008 bleibt für die langfristige Model/View-Richtung verbindlich. Dieser Slice lehnt modelgebundene ComboBox-Daten nicht ab. Er vermeidet lediglich, `ListModel` vorzeitig nur deshalb zu entwerfen, weil ein kleines Formular-Control eine erste brauchbare Implementierung benötigt.

Die Owned-Item-API reicht für typische Formularauswahlen und gibt den folgenden Backend-Slices einen stabilen Selection-Vertrag. Eine modellgebundene Variante kann ergänzt werden, sobald M5 belastbare Anforderungen an Observation, Identität, Mutation und Virtualisierung liefert.

### Konsequenzen

Positiv:

- M4 erhält eine echte, testbare Core-Grundlage für `ComboBox` ohne Terminal-, Rendered- oder Native-Typen.
- Die Selection-Semantik ist deterministisch und verhält sich beim vollständigen Datenaustausch fail-closed.
- Die Messung bleibt bei Selection-Wechseln stabil und pro Backend erweiterbar.
- Popup-Verhalten kann als eigener vertikaler Slice entwickelt werden, statt versteckt im Core zu landen.

Bewusst vertagt:

- editierbarer ComboBox-Text;
- Open/Close-State des Drop-downs;
- Pointer-Öffnung und Popup-Item-Hit-Testing;
- hervorgehobene, aber noch nicht committed Preview-Auswahl;
- Escape-Rollback-Semantik;
- Type-ahead-Suche;
- modellgebundene/virtualisierte Datenquellen;
- Terminal-/Rendered-Präsentation und Native-Peers.

Das sind keine dauerhaften Ausschlüsse. Es sind getrennte Verträge, die erst zusammen mit konkreten Backend-Verbrauchern und Tests ergänzt werden sollen.
