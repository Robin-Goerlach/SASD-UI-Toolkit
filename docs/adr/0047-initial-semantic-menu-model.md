# ADR 0047 – Initial semantic menu model / Initiales semantisches Menümodell

- **Status:** Accepted
- **Date:** 2026-10-01

## English

### Context

M4 now has backend-neutral `Command`, lifetime-safe command references/observation, Button-to-Command binding, and `ShortcutMap`. The next menu work needs a semantic representation that can later feed terminal, rendered, or native presentation without making any of those backends the owner of application commands.

A complete desktop menu system would be premature. We have not yet established submenu ownership, menu-bar/window attachment, popup lifetime, mnemonic syntax, native-menu synchronization, checked/radio menu items, icons, dynamic population, or platform-specific conventions. Encoding those policies now would turn a small semantic seam into a speculative framework.

### Decision

Introduce public `MenuModel`, `MenuItem`, and `MenuItemKind` with a deliberately small first contract:

- `MenuModel` represents one ordered flat menu with UTF-8 title metadata;
- an item is either a command item or a separator;
- command items retain only `Command::Reference`, never ownership or a raw lifetime assumption;
- command text and enabled state are queried from the live `Command` instead of duplicated into menu-local state;
- an expired command remains a structural item but becomes unavailable: `command()` returns null, text is empty, enabled is false, and activation is rejected;
- command-item activation delegates to `Command::execute()` and tolerates synchronous command destruction;
- a command item may carry optional `Shortcut` display metadata;
- storing shortcut metadata does not register the gesture and does not mutate any `ShortcutMap`;
- `MenuModel` owns only its title and structural item order;
- the first model has no submenus, menu bar, popup host, mnemonics, checked/radio state, icons, native handles, or presentation behavior.

Structural mutation may invalidate references to items because the implementation uses an ordered vector. Presentation code should read the model synchronously while constructing or refreshing its own representation rather than retain item addresses across mutation.

### Consequences

The toolkit gains a backend-neutral semantic menu source that already composes with the Command and Shortcut foundations. Terminal, rendered, and future native menu presenters can share one application model without acquiring ownership of commands or accidentally changing shortcut routing merely by displaying a shortcut label.

Querying live Command state keeps the first model simple and avoids adding a second observation/cache layer. A future long-lived native menu peer may subscribe directly to live Commands when push-style synchronization is needed; that policy does not need to live inside the semantic model itself.

Hierarchical submenus should be added only with an explicit ownership/lifetime decision. The flat first contract is intentional: it provides useful File/Edit/Help-style menu content while leaving submenu trees, popup behavior, focus scopes, and platform menu-bar attachment open for evidence from real consumers.

## Deutsch

### Kontext

M4 besitzt inzwischen backend-neutrale `Command`-Objekte, lifetime-sichere Command-Referenzen/-Observation, Button-zu-Command-Binding und `ShortcutMap`. Für die nächste Menüarbeit benötigen wir eine semantische Repräsentation, die später Terminal-, Rendered- oder Native-Presentation versorgen kann, ohne dass eines dieser Backends Eigentümer der Anwendungs-Commands wird.

Ein vollständiges Desktop-Menüsystem wäre verfrüht. Ownership von Untermenüs, Anbindung einer Menüleiste an ein Window, Popup-Lebensdauer, Mnemonic-Syntax, Synchronisation nativer Menüs, Checked-/Radio-Menüeinträge, Icons, dynamischer Aufbau und plattformspezifische Konventionen sind noch nicht belastbar definiert. Diese Policies jetzt festzuschreiben würde aus einer kleinen semantischen Nahtstelle ein spekulatives Framework machen.

### Entscheidung

Es werden öffentliche `MenuModel`, `MenuItem` und `MenuItemKind` mit bewusst kleinem Erstvertrag eingeführt:

- `MenuModel` repräsentiert ein einzelnes geordnetes flaches Menü mit UTF-8-Titelmetadaten;
- ein Eintrag ist entweder Command-Eintrag oder Separator;
- Command-Einträge halten nur `Command::Reference`, niemals Ownership oder eine rohe Lifetime-Annahme;
- Command-Text und Enabled-State werden vom noch lebenden `Command` gelesen, statt in menülokalem Zustand dupliziert zu werden;
- ein zerstörter Command bleibt als struktureller Eintrag bestehen, wird aber unavailable: `command()` liefert null, Text ist leer, Enabled ist false und Aktivierung wird abgelehnt;
- Aktivierung eines Command-Eintrags delegiert an `Command::execute()` und toleriert synchrone Command-Zerstörung;
- ein Command-Eintrag darf optionale `Shortcut`-Anzeigemetadaten tragen;
- Shortcut-Metadaten zu speichern registriert die Geste nicht und verändert keine `ShortcutMap`;
- `MenuModel` besitzt nur Titel und strukturelle Reihenfolge seiner Einträge;
- das erste Modell besitzt keine Untermenüs, Menüleiste, Popup-Hosts, Mnemonics, Checked-/Radio-State, Icons, Native Handles oder Presentation-Verhalten.

Strukturelle Änderungen dürfen Referenzen auf Einträge invalidieren, weil die Implementierung einen geordneten Vektor verwendet. Presentation-Code soll das Modell synchron beim Aufbau oder Refresh seiner eigenen Darstellung lesen, statt Item-Adressen über Mutationen hinweg aufzubewahren.

### Konsequenzen

Das Toolkit erhält eine backend-neutrale semantische Menüquelle, die bereits sauber mit den Command- und Shortcut-Grundlagen zusammenspielt. Terminal-, Rendered- und spätere Native-Menüdarstellungen können dasselbe Anwendungsmodell verwenden, ohne Command-Ownership zu übernehmen oder Shortcut-Routing allein durch die Anzeige eines Shortcut-Labels zu verändern.

Das direkte Lesen des aktuellen Command-State hält das erste Modell klein und vermeidet eine zweite Observation-/Cache-Schicht. Ein später langlebiger nativer Menü-Peer kann bei Bedarf direkt auf lebende Commands subscriben, wenn Push-Synchronisation notwendig wird; diese Policy muss nicht im semantischen Modell selbst liegen.

Hierarchische Untermenüs sollen erst mit einer expliziten Ownership-/Lifetime-Entscheidung ergänzt werden. Der flache Erstvertrag ist bewusst gewählt: Er liefert bereits nützlichen File-/Edit-/Help-Menüinhalt und lässt Untermenübäume, Popup-Verhalten, Focus-Scopes und plattformspezifische Menüleisten-Anbindung für echte Consumer-Anforderungen offen.
