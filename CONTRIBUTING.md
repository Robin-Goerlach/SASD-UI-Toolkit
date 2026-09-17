# Contributing to SASD UI Toolkit

SASD UI Toolkit is currently in its architecture/bootstrap phase. Contributions are welcome, but early changes should protect the core design rather than maximize feature count.

## English

### Before implementing a feature

Please check whether the change belongs to:

1. the platform-neutral core;
2. a backend;
3. a reusable model/layout/event abstraction;
4. an example or documentation only.

Platform-specific types should not leak into the normal public API unless the feature is explicitly documented as a backend extension.

### Preferred contribution shape

Small, reviewable changes are preferred:

- one architectural concern at a time;
- tests together with behavior changes;
- documentation updates together with public API changes;
- no large dependency addition without a written rationale;
- avoid speculative abstraction that is not needed by at least one concrete use case.

### Architecture changes

Before introducing a new fundamental concept, explain:

- the problem being solved;
- whether it applies to terminal, rendered desktop and native desktop backends;
- why the existing abstraction is insufficient;
- lifetime/ownership implications;
- event/threading implications;
- expected impact on the public API.

Important decisions should eventually receive an Architecture Decision Record (ADR).

### Code direction

The project targets modern C++ with C++20 as the initial baseline.

Core expectations:

- use RAII;
- make ownership explicit;
- prefer readable code over clever code;
- keep backend implementation details out of public headers;
- do not introduce raw owning pointers;
- avoid mandatory macros/code generation for ordinary application code;
- keep UI-thread assumptions explicit;
- treat Unicode and text input separately from physical key input.

### Documentation

Important project documentation is maintained in German and English. A change does not need literal word-for-word translations, but architectural meaning should remain aligned.

Start at [docs/README.md](docs/README.md).

### Commit messages

Use short, descriptive commit messages. Conventional prefixes are encouraged but not required, for example:

```text
core: add component ownership prototype
backend: introduce terminal capability detection
docs: clarify native peer strategy
test: add layout contract cases
```

---

## Deutsch

Das SASD UI Toolkit befindet sich derzeit in der Architektur- und Bootstrap-Phase. Beiträge sind willkommen; in dieser frühen Phase ist eine saubere Kernarchitektur wichtiger als eine möglichst lange Featureliste.

### Vor der Implementierung

Bitte zuerst prüfen, ob eine Änderung in:

1. den plattformneutralen Core,
2. ein Backend,
3. eine wiederverwendbare Model-/Layout-/Event-Abstraktion,
4. oder ausschließlich Beispiele/Dokumentation

gehört.

Plattformspezifische Typen sollen nicht in die normale öffentliche API gelangen, außer eine Funktion ist ausdrücklich als Backend-Erweiterung dokumentiert.

### Bevorzugte Beiträge

Kleine, gut prüfbare Änderungen sind erwünscht:

- jeweils ein architektonisches Thema;
- Tests gemeinsam mit Verhaltensänderungen;
- Dokumentationsanpassungen gemeinsam mit Änderungen an öffentlicher API;
- keine große neue Abhängigkeit ohne Begründung;
- keine spekulative Abstraktion ohne mindestens einen konkreten Anwendungsfall.

### Architekturänderungen

Bei neuen Grundkonzepten bitte erklären:

- welches Problem gelöst wird;
- ob Terminal-, gerenderte Desktop- und native Desktop-Backends betroffen sind;
- warum die bestehende Abstraktion nicht genügt;
- Auswirkungen auf Ownership/Lebensdauer;
- Auswirkungen auf Events/Threading;
- Auswirkungen auf die öffentliche API.

Wichtige Entscheidungen sollen später durch Architecture Decision Records (ADR) nachvollziehbar gemacht werden.

### Code-Richtung

C++20 ist als anfängliche Sprachbasis vorgesehen.

Wichtige Regeln:

- RAII verwenden;
- Ownership sichtbar machen;
- verständlichen Code cleverem Code vorziehen;
- Backend-Details aus öffentlichen Headern fernhalten;
- keine rohen owning pointer;
- keine zwingende Makro-/Codegenerator-Infrastruktur für normalen Anwendungscode;
- UI-Thread-Annahmen klar halten;
- Unicode/Textinput nicht mit physischen Tastendrücken verwechseln.

### Dokumentation

Wichtige Projektdokumente werden auf Deutsch und Englisch gepflegt. Die Texte müssen keine wörtlichen Übersetzungen sein, sollen aber dieselben Architekturentscheidungen wiedergeben.

Einstieg: [docs/README.md](docs/README.md).
