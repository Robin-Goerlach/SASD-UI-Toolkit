# ADR 0060 – Deterministic shortcut display text / Deterministische Shortcut-Anzeigetexte

- **Status:** Accepted
- **Date:** 2026-10-02

## English

### Context

ADR 0046 introduced backend-neutral `Shortcut` identity and deliberately separated key gestures from text input. ADR 0047 and ADR 0059 later made optional Shortcut values part of menu presentation metadata and owned presentation snapshots. Terminal and Rendered menu presentation now need a human-readable label for the same semantic gesture.

If each backend formats shortcuts independently, presentation can drift even though routing semantics are shared. One backend might render `Ctrl+Shift+F5`, another `Shift-Ctrl-F5`, and a later native layer might invent a third convention. Formatting is not command routing, but inconsistent labels would make the toolkit look semantically inconsistent and would duplicate policy.

### Decision

Add backend-neutral presentation helpers in `shortcut_display.hpp`:

- `shortcutKeyDisplayText(Key)` maps every currently supported navigation/control/function-key identity to a compact deterministic ASCII token;
- `shortcutDisplayText(const Shortcut&)` combines known modifier bits and the key token using `+` separators;
- modifier display order is fixed as `Ctrl`, `Alt`, `Shift`, `Meta`;
- `Key::unknown` produces no display text because it is not an invokable Shortcut identity.

The helpers are presentation metadata only. They do not affect `Shortcut::matches()`, `ShortcutMap`, event routing, command execution, or semantic identity. ASCII labels are intentional for the first slice because they are immediately usable by Terminal and Rendered backends and avoid introducing a localization subsystem before the menu presentation path is complete.

The formatter handles only modifier bits currently defined by `KeyModifier`. Future key/modifier additions must be mapped explicitly instead of becoming visible accidentally through numeric formatting. This keeps unsupported input identity fail-closed at the presentation boundary as well.

### Consequences

Terminal and Rendered menu implementations can share one label convention without depending on each other. Owned menu snapshots may continue to carry semantic `Shortcut` values and defer string formatting until a presenter actually needs text, avoiding another duplicated stored representation in the snapshot model.

The initial labels are not a localization or platform-native accelerator convention. A future localization/native-menu policy may wrap or replace the display-token layer while preserving `Shortcut` identity and routing. Because formatting is separated from matching, such a presentation change does not alter application behavior.

## Deutsch

### Kontext

ADR 0046 führte die backend-neutrale `Shortcut`-Identität ein und trennte Tastengesten bewusst von Texteingabe. ADR 0047 und ADR 0059 machten optionale Shortcut-Werte später zu Menü-Presentation-Metadaten und Bestandteilen besitzender Presentation-Snapshots. Terminal- und Rendered-Menüdarstellung benötigen nun für dieselbe semantische Geste einen menschenlesbaren Anzeigetext.

Würde jedes Backend Shortcuts selbst formatieren, könnte die Darstellung trotz gemeinsamer Routing-Semantik auseinanderlaufen. Ein Backend könnte `Ctrl+Shift+F5`, ein anderes `Shift-Ctrl-F5` und eine spätere native Schicht eine dritte Konvention anzeigen. Formatierung ist zwar kein Command-Routing, aber unterschiedliche Labels würden eine semantisch inkonsistente Oberfläche erzeugen und Policy duplizieren.

### Entscheidung

In `shortcut_display.hpp` werden backend-neutrale Presentation-Helper eingeführt:

- `shortcutKeyDisplayText(Key)` ordnet jeder aktuell unterstützten Navigation-/Control-/Function-Key-Identität ein kompaktes deterministisches ASCII-Token zu;
- `shortcutDisplayText(const Shortcut&)` verbindet bekannte Modifier-Bits und das Key-Token mit `+`;
- die Modifier-Anzeigereihenfolge ist fest `Ctrl`, `Alt`, `Shift`, `Meta`;
- `Key::unknown` erzeugt keinen Anzeigetext, weil es keine ausführbare Shortcut-Identität darstellt.

Die Helper sind ausschließlich Presentation-Metadaten. Sie verändern weder `Shortcut::matches()` noch `ShortcutMap`, Event-Routing, Command-Ausführung oder semantische Identität. ASCII-Labels sind für diesen ersten Schritt bewusst gewählt, weil sie unmittelbar für Terminal und Rendered nutzbar sind und kein Lokalisierungssystem vor Fertigstellung des Menü-Presentation-Pfads erzwingen.

Der Formatter berücksichtigt nur aktuell definierte `KeyModifier`-Bits. Künftige Key-/Modifier-Erweiterungen müssen ausdrücklich abgebildet werden, statt zufällig über numerische Formatierung sichtbar zu werden. Damit bleibt auch die Presentation-Grenze bei unbekannter Eingabeidentität fail-closed.

### Konsequenzen

Terminal- und Rendered-Menüimplementierungen können eine gemeinsame Label-Konvention verwenden, ohne voneinander abhängig zu werden. Besitzende Menü-Snapshots können weiterhin semantische `Shortcut`-Werte tragen und die String-Formatierung erst dann durchführen, wenn ein Presenter tatsächlich Text benötigt. Dadurch entsteht keine zusätzliche dauerhaft duplizierte String-Repräsentation im Snapshot-Modell.

Die anfänglichen Labels sind weder ein Lokalisierungsvertrag noch eine plattformspezifische native Accelerator-Konvention. Eine spätere Lokalisierungs-/Native-Menu-Policy kann diese Darstellungsschicht umschließen oder ersetzen, während `Shortcut`-Identität und Routing erhalten bleiben. Da Formatierung und Matching getrennt sind, verändert eine solche Presentation-Anpassung nicht das Anwendungsverhalten.
