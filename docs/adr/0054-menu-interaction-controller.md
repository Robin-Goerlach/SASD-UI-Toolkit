# ADR 0054 – Backend-neutral menu interaction controller / Backend-neutraler Menü-Interaktionscontroller

- **Status:** Accepted
- **Date:** 2026-10-01

## English

### Context

ADR 0047–0049 established semantic menu structure and recursive submenu ownership. ADR 0050–0051 added stateless popup navigation/key interpretation, ADR 0052 introduced mutation-tolerant indexed submenu paths, and ADR 0053 added equivalent top-level menu-bar navigation. These pieces deliberately avoided owning transient interaction state.

The next layer needs to compose them into one coherent keyboard session without moving state into `MenuModel` or duplicating policy independently in Terminal and Rendered backends. It must also remain safe when application code rebuilds menu structure while a menu is open, and Command execution must not enter arbitrary client callbacks while the controller still contains half-updated popup state.

### Decision

Introduce header-only `MenuInteractionController` as a backend-neutral owner of transient keyboard interaction state. It stores only value state:

- the selected top-level menu index;
- an optional `MenuPath` for the deepest open popup (`nullopt` means no popup, an empty path means the root popup);
- one optional item-selection index for each open popup level.

The controller stores no `MenuModel*`, `MenuItem*`, backend/native handle, focus object, or presentation object across calls. Callers explicitly enter menu interaction with `begin()`. An inactive controller ignores keys rather than implicitly claiming ordinary application input.

Before every active key transaction, retained indices are normalized against the current `MenuBarModel`. A vanished top-level menu closes the entire interaction. A stale submenu path is truncated using `sanitizeMenuPath()`. Item selections that are out of range or no longer enabled are cleared, not redirected to another item. Recovery is therefore conservative and deterministic.

The controller delegates actual key meaning to `interpretMenuBarKey()` and `interpretMenuPopupKey()` instead of reimplementing their mappings. Opening a submenu extends the validated `MenuPath` and adds a fresh empty selection for that level. Closing a nested popup removes exactly one level; closing the root popup returns to menu-bar interaction; a subsequent menu-bar Escape closes the complete interaction.

Command execution remains two-phase. On `activate_command`, the controller copies a lifetime-safe `Command::Reference`, resets all menu interaction state, and returns `MenuInteractionAction::activate_command`. It does **not** execute the Command. The caller can first dismiss/repaint/fix focus and then execute the returned reference if it is still live. No borrowed menu pointer escapes `handleKey()`.

This first controller intentionally does not implement pointer hover/click menus, mnemonics, Alt activation, automatic lateral switching between top-level menus while a root popup is open, focus restoration, popup geometry, or native-menu integration. Those concerns can consume the controller state or extend the interaction layer later without contaminating the semantic menu model.

### Consequences

Terminal, Rendered, and later native presentations can share one deterministic keyboard-session state machine while still owning their concrete surfaces and focus transactions. Menu rebuilds no longer require a presenter to retain potentially dangling submenu pointers, and Command callbacks can run only after controller state has already reached a stable closed state.

The current implementation performs path revalidation and selection checks on each key event. Menu depth and popup item counts are expected to be small, so clarity and lifetime correctness are preferred over cached pointers or indexes. Profiling may justify optimization later without changing the public state contract.

## Deutsch

### Kontext

ADR 0047–0049 etablierten die semantische Menüstruktur und rekursives Ownership von Untermenüs. ADR 0050–0051 ergänzten zustandslose Popup-Navigation und Tastaturinterpretation, ADR 0052 führte mutationsrobuste indexbasierte Untermenüpfade ein, und ADR 0053 ergänzte die entsprechende Navigation für die Top-Level-Menüleiste. Diese Bausteine vermieden bewusst den Besitz transienten Interaktionszustands.

Die nächste Schicht muss diese Teile zu einer konsistenten Tastatursitzung zusammensetzen, ohne den Zustand in `MenuModel` zu verschieben oder die Policy getrennt in Terminal- und Rendered-Backends zu duplizieren. Sie muss außerdem sicher bleiben, wenn Anwendungscode die Menüstruktur bei geöffnetem Menü neu aufbaut. Command-Ausführung darf keinen beliebigen Client-Callback betreten, solange der Controller noch halb aktualisierten Popup-Zustand enthält.

### Entscheidung

Der header-only `MenuInteractionController` wird als backend-neutraler Besitzer des transienten Tastatur-Interaktionszustands eingeführt. Er speichert ausschließlich Wertzustand:

- den Index des ausgewählten Top-Level-Menüs;
- einen optionalen `MenuPath` für das tiefste geöffnete Popup (`nullopt` bedeutet kein Popup, ein leerer Pfad das Root-Popup);
- einen optionalen Item-Selection-Index für jede geöffnete Popup-Ebene.

Der Controller hält über Aufrufe hinweg weder `MenuModel*` noch `MenuItem*`, Backend-/Native-Handles, Fokusobjekte oder Presentation-Objekte. Caller starten Menüinteraktion explizit mit `begin()`. Ein inaktiver Controller ignoriert Tasten, statt normale Anwendungseingaben implizit an sich zu ziehen.

Vor jeder aktiven Tastentransaktion werden gespeicherte Indizes gegen das aktuelle `MenuBarModel` normalisiert. Ein verschwundenes Top-Level-Menü schließt die gesamte Interaktion. Ein veralteter Untermenüpfad wird über `sanitizeMenuPath()` gekürzt. Item-Selektionen, die außerhalb des gültigen Bereichs liegen oder nicht mehr enabled sind, werden geleert und nicht auf einen anderen Eintrag umgebogen. Die Wiederherstellung bleibt damit konservativ und deterministisch.

Der Controller delegiert die eigentliche Tastenbedeutung an `interpretMenuBarKey()` und `interpretMenuPopupKey()`, statt deren Mappings erneut zu implementieren. Das Öffnen eines Untermenüs erweitert den validierten `MenuPath` und ergänzt für diese Ebene eine neue leere Auswahl. Das Schließen eines verschachtelten Popups entfernt genau eine Ebene; das Schließen des Root-Popups kehrt zur Menüleisteninteraktion zurück; ein anschließendes Escape auf der Menüleiste beendet die komplette Interaktion.

Command-Ausführung bleibt zweiphasig. Bei `activate_command` kopiert der Controller eine lifetime-sichere `Command::Reference`, setzt seinen gesamten Menüinteraktionszustand zurück und liefert `MenuInteractionAction::activate_command`. Er führt den Command **nicht** aus. Der Caller kann zuerst Popup/Presentation/Fokus stabilisieren und danach die zurückgegebene Referenz ausführen, sofern sie noch lebt. Kein ausgeliehener Menü-Pointer verlässt `handleKey()`.

Dieser erste Controller implementiert bewusst noch keine Pointer-Hover-/Click-Menüs, Mnemonics, Alt-Aktivierung, automatisches laterales Umschalten zwischen Top-Level-Menüs bei geöffnetem Root-Popup, Fokuswiederherstellung, Popup-Geometrie oder Native-Menu-Integration. Diese Themen können den Controller-Zustand später konsumieren oder die Interaktionsschicht erweitern, ohne das semantische Menümodell zu verunreinigen.

### Konsequenzen

Terminal-, Rendered- und spätere native Presentations können eine gemeinsame deterministische Tastatur-Zustandsmaschine verwenden und trotzdem ihre konkreten Oberflächen und Fokus-Transaktionen selbst besitzen. Menü-Rebuilds zwingen Presenter nicht mehr dazu, potenziell dangling Submenu-Pointer zu behalten, und Command-Callbacks können erst laufen, nachdem der Controller bereits einen stabilen geschlossenen Zustand erreicht hat.

Die aktuelle Implementierung validiert Pfad und Selektionen bei jedem Tastenevent erneut. Menütiefe und Popup-Item-Anzahlen dürften klein bleiben; Klarheit und Lifetime-Korrektheit haben daher Vorrang vor gecachten Pointern oder Indizes. Profiling kann später Optimierungen rechtfertigen, ohne den öffentlichen Zustandsvertrag zu ändern.
