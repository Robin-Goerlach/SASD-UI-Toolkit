# ADR 0039 – Explicit RadioGroup arrow-key navigation policy / Explizite RadioGroup-Pfeiltasten-Navigation

- **Status:** Accepted
- **Date:** 2026-09-30

## English

### Context

ADR 0038 introduced explicit RadioGroup membership but deliberately deferred arrow-key navigation. Implementing arrows directly inside RadioButton would require the control to reach into FocusManager or to hide focus-scope policy inside one Widget. Folding radio behavior into FocusTraversal would create the opposite problem: generic Tab traversal would suddenly own control-specific group semantics.

RadioGroup is intentionally independent from visual parenting and can remain intact across reparenting. That makes an additional boundary important: a semantic group may theoretically span more than one top-level visual tree, but one arrow-key gesture must not move keyboard focus into another window.

### Decision

Introduce public `RadioGroupNavigation` as an explicit policy utility, parallel in spirit to `FocusTraversal` but specific to radio groups.

- Left/Up navigate to the previous eligible group member.
- Right/Down navigate to the next eligible group member.
- Navigation uses stable RadioGroup attachment order and wraps.
- Disabled/hidden/non-focusable members and members below hidden/disabled ancestors are skipped.
- Navigation never crosses the source RadioButton's top-level visual root.
- Unmodified arrows only are claimed; modified arrows remain application territory.
- A single/effectively isolated RadioButton does not consume arrow navigation.
- Focus moves before selection. If FocusManager callbacks redirect or reject the focus transition, selection remains unchanged.
- After successful focus movement, the target is selected. RadioButton/RadioGroup retain ownership of exclusivity and `onSelected` semantics.
- RadioButton itself continues to handle Space selection and pointer gestures. It does not store or expose a FocusManager dependency.
- Applications/focus scopes opt in by calling `RadioGroupNavigation::handleEvent()` from scope-level event fallback, before or alongside generic `FocusTraversal`.

### Consequences

The control, selection group, focus manager and navigation policy remain separate responsibilities. RadioGroup can stay semantic and layout-independent, while keyboard navigation still respects the current visual surface. The explicit utility also avoids adding a FocusManager pointer/accessor to Widget solely for RadioButton.

The first policy follows attachment order rather than geometric direction. This is deterministic for Terminal and rendered backends and remains valid when a group is split across nested containers. A future spatial-navigation subsystem may offer geometry-driven movement without changing this contract.

## Deutsch

### Kontext

ADR 0038 führte die explizite RadioGroup-Mitgliedschaft ein, verschob aber die Navigation mit Pfeiltasten bewusst. Eine direkte Implementierung in RadioButton würde entweder Zugriff auf FocusManager erfordern oder Fokus-Scope-Politik in einem einzelnen Widget verstecken. Umgekehrt würde eine Integration in FocusTraversal dazu führen, dass die generische Tab-Navigation plötzlich controlspezifische Gruppensemantik besitzt.

RadioGroup ist bewusst unabhängig von visueller Elternschaft und kann Reparenting überstehen. Deshalb ist eine weitere Grenze wichtig: Eine semantische Gruppe kann theoretisch mehrere Top-Level-Bäume überspannen, aber eine Pfeiltaste darf den Tastaturfokus nicht in ein anderes Fenster verschieben.

### Entscheidung

Es wird die öffentliche Policy-Hilfsklasse `RadioGroupNavigation` eingeführt, konzeptionell parallel zu `FocusTraversal`, aber spezifisch für Radio-Gruppen.

- Links/Hoch navigiert zum vorherigen geeigneten Gruppenmitglied.
- Rechts/Runter navigiert zum nächsten geeigneten Gruppenmitglied.
- Die Reihenfolge ist die stabile RadioGroup-Attach-Reihenfolge und wird zyklisch durchlaufen.
- Deaktivierte, unsichtbare, nicht fokussierbare Mitglieder sowie Mitglieder unter unsichtbaren oder deaktivierten Vorfahren werden übersprungen.
- Navigation überschreitet niemals den Top-Level-Visual-Root des Ausgangs-RadioButtons.
- Nur unmodifizierte Pfeiltasten werden beansprucht; modifizierte Pfeile bleiben Anwendungssache.
- Ein einzelner bzw. effektiv isolierter RadioButton konsumiert keine Gruppennavigation.
- Zuerst wird der Fokus verschoben. Wird die FocusManager-Transition durch Callbacks umgeleitet oder abgelehnt, bleibt die Auswahl unverändert.
- Nach erfolgreicher Fokusbewegung wird das Ziel ausgewählt. RadioButton/RadioGroup bleiben für Exklusivität und `onSelected` verantwortlich.
- RadioButton selbst behandelt weiterhin Space-Auswahl und Pointer-Gesten und erhält keine FocusManager-Abhängigkeit.
- Anwendungen/Fokus-Scopes aktivieren die Policy explizit über `RadioGroupNavigation::handleEvent()` im Scope-Fallback, zusammen mit der generischen `FocusTraversal`.

### Konsequenzen

Control, Auswahlgruppe, Fokusmanager und Navigationspolicy bleiben getrennte Verantwortlichkeiten. RadioGroup bleibt semantisch und layoutunabhängig, während Tastaturnavigation trotzdem die aktuelle visuelle Oberfläche respektiert. Die explizite Utility vermeidet außerdem einen FocusManager-Zugriff in Widget nur für RadioButton.

Die erste Policy folgt der Attach-Reihenfolge statt geometrischer Richtung. Das ist für Terminal- und Rendered-Backends deterministisch und funktioniert auch bei Gruppen über verschachtelte Container. Ein späteres Spatial-Navigation-Subsystem kann geometriebasierte Bewegung ergänzen, ohne diesen Vertrag zu ändern.
