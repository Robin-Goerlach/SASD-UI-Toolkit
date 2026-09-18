# Architektur

## Überblick

Das SASD UI Toolkit soll eine **semantische UI-Schicht** von ihrer konkreten Darstellung trennen. Anwendungscode arbeitet mit `Window`, `Button`, `Label`, Layouts, Models und Events. Ein Backend entscheidet, wie diese Konzepte auf einer Zielumgebung umgesetzt werden.

```text
                    SASD-Anwendung
                         │
                  Öffentliche UI-API
                         │
        ┌────────────────┼────────────────┐
        │                │                │
   Komponenten         Layouts          Models
        │                │                │
        └────────────────┼────────────────┘
                         │
                    Events/Commands
                         │
                      Toolkit
                         │
                 Backend-/Peer-API
                         │
       ┌─────────────────┼──────────────────┐
       │                 │                  │
 Native Desktop     Rendered Desktop      Terminal
 Win32/GTK/AppKit       z. B. SDL3       ANSI/VT/Console
```

## Architektonische Leitidee

Die öffentliche UI-API darf nicht von einer konkreten Plattform abhängig sein. Gleichzeitig soll das Projekt nicht versuchen, jede Plattform künstlich identisch erscheinen zu lassen.

Daraus folgen drei Regeln:

1. **Semantik wird vereinheitlicht.** Ein Button ist ein Button, unabhängig vom Backend.
2. **Darstellung darf unterschiedlich sein.** Ein nativer Windows-Button und ein Terminal-Button müssen nicht pixel- oder zeichengenau gleich aussehen.
3. **Capabilities werden explizit gemacht.** Plattformunterschiede werden abgefragt statt versteckt.

## Komponentenmodell

Inspiriert von VCL und AWT wird zwischen allgemeinen Komponenten und sichtbaren Widgets unterschieden.

```text
Component
├── Command
├── Action
├── Timer
├── DataSource
└── Widget
    ├── Label
    ├── Button
    ├── CheckBox
    ├── TextField
    └── Container
        ├── Panel
        ├── ScrollView
        └── Window
            └── Dialog
```

`Component` ist nicht zwangsläufig sichtbar. Dadurch können später auch Actions, Commands, Timer oder Datenquellen in dasselbe Ownership- und Lebenszyklusmodell integriert werden.

### Ownership

Für den Core gilt als Startpunkt:

- Ein `Container` besitzt seine Kindkomponenten.
- Ownership soll vorzugsweise durch RAII und eindeutige Besitzverhältnisse dargestellt werden.
- Nicht-besitzende Referenzen dürfen Lebenszeiten nicht verlängern.
- Callback-Verbindungen müssen sicher getrennt werden können, wenn Sender oder Empfänger zerstört werden.

Die konkrete C++-API wird erst mit dem ersten Core-Prototyp festgezurrt; rohe owning pointer sind jedoch kein Ziel.

## Toolkit und Backend

Das `Toolkit` ist die Brücke zwischen semantischem UI-Baum und Plattformimplementierung. Es ist für globale Dienste zuständig, beispielsweise:

- Erzeugen und Zerstören von Top-Level-Fenstern
- Event-Pump / Event-Loop
- Ermitteln von Backend-Capabilities
- Clipboard- und Cursor-Dienste
- Theme-/Metric-Zugriff
- Plattformintegration

Ein Backend kann Widgets auf zwei Arten umsetzen:

### Native Peers

Das Backend ordnet einem SASD-Widget ein natives Betriebssystem-Control zu.

Beispiele:

- `Button` → Win32 Button
- `Button` → GTK Button
- `Button` → AppKit Button

### Rendered Peers

Das Backend zeichnet das Widget selbst auf eine Zeichenfläche.

Das ist insbesondere wichtig für:

- Terminal-Backends
- SDL-basierte Backends
- Widgets ohne natives Gegenstück
- konsistente Spezialkomponenten

Ein Backend darf beide Strategien kombinieren.

## BackendCapabilities

Plattformunterschiede sollen nicht durch Annahmen oder implizites Verhalten kaschiert werden. Ein Backend meldet Fähigkeiten, zum Beispiel:

```cpp
struct BackendCapabilities {
    bool mouse;
    bool clipboard;
    bool dragAndDrop;
    bool nativeMenus;
    bool multipleWindows;
    bool trueColor;
    bool accessibility;
};
```

Diese Struktur ist nur ein Konzeptentwurf. Die spätere API kann granularer werden.

## Eventmodell

Das Eventsystem soll backendunabhängig sein. Rohereignisse werden vom Backend in semantische Events übersetzt.

Geplante Eventfamilien:

- Tastatur
- Textinput
- Pointer/Maus
- Fokus
- Aktivierung/Click
- Fenster/Lifecycle
- Resize
- Command/Action

Wichtig ist die Trennung von **Key Events** und **Text Input**. Ein Zeichen ist nicht dasselbe wie eine physische Taste; dies ist für internationale Tastaturen, IMEs und Accessibility relevant.

Für UI-Events ist zunächst ein einzelner UI-Thread mit Event-Queue vorgesehen. Hintergrundarbeit soll über explizite Übergaben zurück in den UI-Thread integriert werden, statt Widgets beliebig thread-safe zu machen.

### Routing und Fokus im aktuellen M1-Core

Der implementierte M1-Core trennt Event-Pump, Zielauswahl, Routing und Fokus bewusst voneinander:

```text
Backend
   │
   ▼
Application
   │
   ├── QuitEvent → Application-Lifecycle
   │
   └── semantisches Eingabe-Event
             │
             ▼
      EventTargetResolver
             │
             ▼
        Ziel-Widget
             │
             ▼
      EventDispatcher
             │
             └── Target → Parent → ... → Root
```

`EventDispatcher` führt synchrones Bubbling über den **visuellen Parent-Pfad** aus. Ein Widget kann ein Event mit `EventResult::handled` konsumieren; bei `ignored` wird dasselbe Event dem visuellen Parent angeboten. Ownership-Beziehungen zu nicht-visuellen Komponenten gehören bewusst nicht zu diesem Routing-Pfad.

Der `FocusManager` ist eine davon getrennte Zielauswahl-Komponente für logischen Tastaturfokus. Er besitzt Widgets nicht, sondern hält genau eine nicht-besitzende Referenz auf das aktuell fokussierte Widget. Widget und FocusManager lösen diese Referenz beim jeweiligen Zerstören gegenseitig, damit kein dangling Pointer bestehen bleibt.

Widgets sind standardmäßig **nicht fokussierbar**. Für den M1-Core besteht die lokale Fokus-Eignung aus `focusable && visible && enabled`. Vererbte Zustände von späteren `Window`-/Container-Fokus-Scope-Regeln werden erst festgelegt, wenn reale Widgets und Backends diese Semantik validieren können.

Ein vom `FocusManager` erzeugtes `FocusEvent` ist eine direkte Zustandsbenachrichtigung an genau das betroffene Widget. Der Fokuszustand ist bereits geändert, wenn der Handler aufgerufen wird. Diese Benachrichtigung ist nicht veto-fähig und wird nicht als normales Eingabe-Bubbling interpretiert. Gewöhnliche Tastatur- und Texteingabe verwendet weiterhin den `EventDispatcher`.

## Layout

Layouts dürfen nicht direkt in Pixeln denken. Ein Terminal arbeitet mit Zellen, Desktop-UIs mit logischen Geräteeinheiten und Schriftmetriken.

### Measure/Arrange-Vertrag im M1-Core

Der implementierte Core verwendet einen zweiphasigen, backendneutralen Layout-Lebenszyklus:

```text
Parent
  │
  ├── measure(MeasureConstraints)
  │        │
  │        ▼
  │   onMeasure(...)
  │        │
  │        ▼
  │   desiredSize
  │
  └── arrange(final Rect)
           │
           ▼
      bounds werden gesetzt
           │
           ▼
      onArrange(...)
```

`SizeConstraints` gehören zum Widget selbst und beschreiben Minimum, Preferred und Maximum. `MeasureConstraints` kommen vom Parent und beschreiben ausschließlich den tatsächlich angebotenen Minimum-/Maximum-Bereich. Die intrinsische Größe aus `onMeasure()` wird zuerst durch die eigenen Widget-Hinweise und danach durch die Parent-Constraints begrenzt; damit kann ein Kind keinen Platz erzwingen, den der Parent nicht besitzt.

`measure()` cached `desiredSize()` für identische Parent-Constraints. Größenrelevante Zustandsänderungen rufen `invalidateMeasure()` auf. Diese Invalidierung propagiert über den visuellen Parent-Pfad, sodass beispielsweise eine spätere Textänderung in einem `Label` auch `VBox` und `Window` als neu zu vermessen markieren kann. Auch das Hinzufügen oder Entfernen visueller Kinder invalidiert den Container.

`arrange()` weist das endgültige logische Rechteck zu. Die Bounds werden vor `onArrange()` gespeichert, damit ein Container beim Anordnen seiner Kinder seine eigene finale Geometrie verwenden kann. Nullgrößen sind erlaubt; negative Ausdehnungen werden abgelehnt. Direktes `setBounds()` läuft bewusst über denselben Arrange-Pfad.

Noch offen bleiben konkrete Regeln für `VBox`/`HBox`, Margin/Padding/Spacing, den Layout-Effekt unsichtbarer Widgets und backendabhängige Textmessung. Diese Punkte sollen erst mit den ersten realen M2-Controls festgelegt werden.

Deshalb soll der Layoutprozess auf folgenden Konzepten beruhen:

- Minimum Size
- Preferred Size
- Maximum Size
- verfügbare Constraints
- Padding/Margins/Spacing
- backendabhängige Text- und Widget-Messung

Erste geplante Layouts:


- `HBox`
- `VBox`
- `GridLayout`
- `FormLayout`
- `StackLayout`
- optional `AbsoluteLayout` für Sonderfälle

Absolute Positionierung soll möglich, aber nicht das Standardmodell sein.

## Model/View für datenreiche Widgets

Listen, Tabellen und Bäume sollen langfristig Daten nicht zwangsläufig kopieren. Dafür wird eine von Qt und anderen UI-Frameworks inspirierte Model/View-Trennung vorgesehen.

```text
Anwendungsdaten
     │
   Model
     │
    View
     │
 Delegate/Renderer/Editor
```

Ein `TableView` soll beispielsweise große Datenmengen virtualisiert darstellen können, ohne Millionen UI-Objekte erzeugen zu müssen.

## Commands und Actions

Ein Command beschreibt eine auslösbare Aktion unabhängig vom auslösenden Widget. Ein Command kann später gleichzeitig mit Menüeintrag, Toolbar-Button, Tastenkürzel oder Command-Palette verbunden sein.

Geplante Eigenschaften:

- Text/Label
- enabled/disabled
- checked/unchecked, wo sinnvoll
- Shortcut
- auszuführender Callback

Das vermeidet duplizierte Logik zwischen Menü, Button und Tastaturbedienung.

## Rendering

Der Core soll keinen bestimmten Renderer vorschreiben. Rendered Backends erhalten eine abstrakte Zeichen-/Surface-Schicht oder kapseln ihre Renderer vollständig hinter dem Backend.

Ein optionales SDL3-Backend ist als früher grafischer Proof-of-Concept attraktiv, weil damit eine Render-/Input-Basis für mehrere Desktopplattformen verfügbar ist, ohne SDL zur öffentlichen SASD-API zu machen.

## Text und Unicode

Die öffentliche API soll Unicode von Anfang an berücksichtigen. Als Startpunkt wird UTF-8 an API-Grenzen bevorzugt. Plattformkonvertierungen, beispielsweise zu UTF-16 unter Windows, gehören in das jeweilige Backend.

Terminal-Rendering muss Zeichenbreiten, Combining Characters und später Grapheme Cluster berücksichtigen. Für `v0.1.0` darf der Umfang begrenzt sein, die Architektur darf aber nicht dauerhaft von „ein Byte = ein Zeichen = eine Zelle“ ausgehen.

## Accessibility

Accessibility wird als Architekturthema behandelt, nicht als spätes Zusatzfeature. Native Backends können Plattformdienste nutzen; gerenderte Backends benötigen langfristig eine semantische Accessibility-Bridge. Der Core soll daher semantische Rollen, Namen, Zustände und Aktionen nicht ausschließlich aus visuellen Details ableiten.

## Öffentliche API versus Backend-API

Diese Trennung ist strikt:

```text
include/sasd/ui/...              öffentliche API
src/core/...                     plattformneutraler Kern
src/backends/terminal/...        Terminal
src/backends/sdl/...             gerenderter Desktop
src/backends/win32/...           Windows native
src/backends/gtk/...             Linux native
src/backends/appkit/...          macOS native
```

Die genaue Verzeichnisstruktur wird beim ersten Code-Meilenstein angelegt, sobald die minimalen Schnittstellen feststehen.

## Stabilitätsregel

Während `0.x` darf sich die API ändern. Änderungen sollen aber begründet, dokumentiert und nach Möglichkeit migrationsfreundlich erfolgen. Ab einer späteren stabilen Version wird eine explizite API-/ABI-Policy notwendig.
