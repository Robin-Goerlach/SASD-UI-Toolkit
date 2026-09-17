# Entwicklungsrichtlinien

## Sprachstandard

Der anfängliche Zielstandard ist **C++20**. Neue Sprachfeatures sollen verwendet werden, wenn sie Lesbarkeit, Sicherheit oder Wartbarkeit verbessern; Modernität allein ist kein Grund für zusätzliche Komplexität.

## Grundsätze

- Klarheit vor cleverem Code.
- Korrektheit vor Mikrooptimierung.
- Kleine, fokussierte Schnittstellen.
- RAII für Ressourcen und Lebenszeiten.
- Keine versteckte globale Ownership.
- Öffentliche API möglichst frei von Backend-Typen.
- Standardbibliothek bevorzugen, bevor eine eigene allgemeine Utility erfunden wird.
- Plattformcode bleibt in Backend-/Platform-Modulen.
- Implementierte Funktionalität und geplante Funktionalität in der Dokumentation klar unterscheiden.

## Namenskonventionen

Vorgesehene Richtung:

```cpp
namespace sasd::ui {
    class Component;
    class Widget;
    class Button;
    class Window;
}
```

- Typen: `PascalCase`
- Funktionen/Methoden: `camelCase`
- lokale Variablen: `camelCase`
- Konstanten: konsistente projektweite Konvention, vor Festlegung nicht unnötig standardisieren
- keine historischen Präfixe wie `TButton`, sofern kein technischer Grund besteht

## Header und Namespaces

Öffentliche Header sollen unter einem stabilen Include-Präfix liegen:

```cpp
#include <sasd/ui/button.hpp>
#include <sasd/ui/window.hpp>
```

Interne Backend-Header sind keine öffentliche API.

## Ownership und Lebensdauer

- Besitz soll im Typ-/Objektmodell sichtbar sein.
- Container besitzen standardmäßig ihre Kinder.
- Nicht-besitzende Zeiger/Referenzen sind zulässig, müssen aber eindeutig als nicht owning erkennbar sein.
- `shared_ptr` soll nicht als universelle Standardlösung dienen.
- Callback-/Event-Verbindungen brauchen ein Lebenszyklusmodell, das dangling callbacks verhindert.

## Fehlerbehandlung

Vor Implementierungsbeginn wird pro API-Familie entschieden, ob Fehler durch:

- Rückgabewerte/Result-Typen,
- Exceptions,
- oder nicht-fehlschlagende Preconditions

repräsentiert werden. Eine zufällige Mischung soll vermieden werden. Fehler in Anwendungsdaten sind von Programmierfehlern zu unterscheiden.

## Threading

Widgets gehören zunächst einem UI-Thread. Hintergrundthreads dürfen keine beliebigen Widget-Operationen ausführen. Geplant ist eine explizite Dispatch-/Post-API für Übergaben an die UI-Event-Queue.

Diese Einschränkung hält den Core verständlich und entspricht dem Modell vieler etablierter UI-Toolkits.

## Tests

Mindestens folgende Testebenen sind vorgesehen:

1. **Unit Tests** für Core, Events, Layout und Models.
2. **Contract Tests** gegen jedes Backend.
3. **Integration Tests** für Plattformadapter.
4. **Example/Smoke Tests**, die kleine vollständige Anwendungen bauen und starten können.

Tests sollen möglichst deterministisch sein. Core-Tests dürfen keine grafische Sitzung voraussetzen.

## Build-System

Vorgesehen ist CMake. Das Ziel ist, mindestens folgende Toolchains früh in CI zu prüfen:

- GCC auf Linux
- Clang auf Linux/macOS
- MSVC auf Windows

Die genaue Compiler-Mindestversion wird mit dem ersten Build festgelegt und dokumentiert.

## Abhängigkeiten

Neue Runtime-Abhängigkeiten benötigen eine Begründung. Vor der Aufnahme sind mindestens zu prüfen:

- Lizenz
- Plattformabdeckung
- Wartungszustand
- API-/ABI-Risiko
- Paketierbarkeit
- ob die Abhängigkeit öffentlich sichtbar wird

Optionales Backend-Zubehör soll den Core nicht unnötig belasten.

## Dokumentation

Öffentliche Klassen und Funktionen erhalten API-Kommentare im Code. Konzeptuelle Dokumentation bleibt unter `docs/`.

Bei architekturrelevanten Änderungen gilt:

1. Entscheidung erklären.
2. Auswirkungen auf Backends beschreiben.
3. deutsche und englische Dokumentation angleichen.
4. erst dann die neue Architektur als stabile Richtung behandeln.

## API-Kompatibilität

Bis `1.0` sind Breaking Changes möglich. Trotzdem sollen sie nicht leichtfertig erfolgen. Gute Namen, kleine Schnittstellen und frühe Beispiele sollen helfen, unnötige Umbauten zu vermeiden.

## Performance

Die ersten Releases optimieren auf saubere Architektur und korrektes Verhalten. Gleichzeitig müssen grundlegende Fehlentscheidungen vermieden werden, beispielsweise:

- ein UI-Objekt pro Tabellenzelle bei großen Datenmengen;
- unnötige Vollkopien kompletter Widget-Bäume;
- dauernde Plattformkonvertierungen ohne Caching;
- blockierende Arbeit im UI-Event-Loop.

Erst Messungen rechtfertigen gezielte Mikrooptimierungen.

## Sicherheit und Robustheit

UI-Code verarbeitet häufig externe Texte, Dateinamen, Clipboard-Daten und Terminal-Escape-Sequenzen. Backends müssen Eingaben robust behandeln. Insbesondere das Terminal-Backend darf untrusted Text nicht ungeprüft als Steuersequenz ausgeben.
