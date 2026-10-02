# ADR 0064: Transactional Terminal Menu-Frame Composition

- Status: Accepted
- Date: 2026-10-02

## Context

The Terminal backend can now render the persistent top-level menu bar and individual popup snapshots into `ScreenBuffer`. Those renderers intentionally validate their own snapshots before mutating cells, but an application needs to present the menu surface as one frame: bar plus zero or more nested popup levels.

Calling the individual renderers directly in sequence creates a subtle transactional gap. A valid bar could already modify the buffer before a later popup discovers unsupported text and fails closed. That would leave a partially updated menu surface even though every individual renderer obeys its own contract.

The semantic menu model and `MenuInteractionController` must not acquire terminal geometry or backend ownership merely to solve this presentation problem.

## Decision

Add `MenuFramePresentationSnapshot` at the Terminal presentation boundary. It owns a `MenuBarPresentationSnapshot`, its terminal-cell origin, and an ordered vector of `PositionedMenuPopupPresentationSnapshot` values. Popup origins are presentation data and are therefore stored next to the owned popup snapshots rather than in `MenuModel`.

Add `renderMenuPresentationFrame()`. Before changing `ScreenBuffer`, it preflights the menu bar and every popup with the existing terminal measurement functions. If any layer is not representable, the function returns `false` and the buffer remains unchanged.

After successful preflight, the bar is painted first and popup layers are painted in vector order. Later popup layers therefore appear above earlier ones where rectangles overlap. This gives nested menus an explicit, deterministic paint-order rule without introducing a second terminal-specific Widget tree.

The individual bar and popup renderers retain their standalone validation. Repeated measurement is accepted for now because preserving simple independent contracts is more important than avoiding a small amount of work. Optimization can later reuse measurements inside a frame transaction without changing the public semantics.

The frame renderer still does not calculate popup placement, route input, mutate menu interaction, emit ANSI/VT, or own terminal-session state.

## Consequences

Terminal menus now have an owned transaction boundary above the two individual renderers. Fail-closed behavior applies to the complete menu surface rather than only to each layer in isolation.

The frame snapshot deliberately remains a presentation value rather than a semantic object. This keeps backend geometry out of Core menu types and leaves future rendered/native menu presentations free to choose different composition rules.

---

# ADR 0064: Transaktionale Zusammensetzung eines Terminal-Menüframes

- Status: Akzeptiert
- Datum: 2026-10-02

## Kontext

Das Terminal-Backend kann inzwischen die permanente Top-Level-Menüleiste und einzelne Popup-Snapshots in den `ScreenBuffer` rendern. Beide Renderer prüfen ihren jeweiligen Snapshot vor der ersten Zelländerung. Eine Anwendung muss jedoch die gesamte Menüoberfläche als einen Frame darstellen: Menüleiste plus null oder mehrere verschachtelte Popup-Ebenen.

Werden die Einzelrenderer einfach nacheinander aufgerufen, entsteht eine kleine Transaktionslücke. Eine gültige Menüleiste könnte den Buffer bereits verändert haben, bevor ein späteres Popup nicht unterstützten Text entdeckt und fail-closed abbricht. Trotz korrekter Einzelrenderer wäre dann ein teilweise aktualisierter Menüframe sichtbar.

Weder das semantische Menümodell noch der `MenuInteractionController` sollen dafür Terminal-Geometrie oder Backend-Verantwortung übernehmen.

## Entscheidung

An der Terminal-Presentation-Grenze wird `MenuFramePresentationSnapshot` eingeführt. Der Snapshot besitzt einen `MenuBarPresentationSnapshot` mit Zellursprung sowie einen geordneten Vektor von `PositionedMenuPopupPresentationSnapshot`-Werten. Popup-Ursprünge sind reine Presentation-Daten und werden deshalb neben dem jeweiligen besitzenden Popup-Snapshot gespeichert, nicht im `MenuModel`.

Zusätzlich wird `renderMenuPresentationFrame()` eingeführt. Vor jeder Änderung des `ScreenBuffer` werden Menüleiste und sämtliche Popups mit den bestehenden Terminal-Messfunktionen vorgeprüft. Ist auch nur eine Ebene nicht darstellbar, liefert die Funktion `false` und der Buffer bleibt unverändert.

Nach erfolgreicher Vorprüfung wird zuerst die Menüleiste und anschließend werden die Popup-Ebenen in Vektorreihenfolge gerendert. Spätere Popup-Ebenen liegen bei überlappenden Rechtecken somit sichtbar über früheren. Verschachtelte Menüs erhalten dadurch eine explizite und deterministische Zeichenreihenfolge, ohne einen zweiten terminalspezifischen Widget-Baum einzuführen.

Die Einzelrenderer behalten ihre eigene Validierung. Die dadurch zunächst doppelte Vermessung wird bewusst akzeptiert: klare unabhängige Verträge sind derzeit wichtiger als das Einsparen einer kleinen Menge Arbeit. Ein späterer Optimierungsschritt kann Messergebnisse innerhalb einer Frame-Transaktion wiederverwenden, ohne die öffentliche Semantik zu verändern.

Der Frame-Renderer berechnet weiterhin keine Popup-Platzierung, routet keine Eingaben, verändert keinen Menü-Interaktionszustand, erzeugt kein ANSI/VT und besitzt keinen Terminal-Session-Zustand.

## Konsequenzen

Terminal-Menüs besitzen nun oberhalb der beiden Einzelrenderer eine besitzende Transaktionsgrenze. Das Fail-closed-Verhalten gilt damit für die gesamte Menüoberfläche und nicht nur isoliert für jede Ebene.

Der Frame-Snapshot bleibt bewusst ein Presentation-Wert und kein semantisches Objekt. Dadurch gelangt keine Backend-Geometrie in die Core-Menütypen, und spätere gerenderte bzw. native Menüoberflächen können eigene Kompositionsregeln verwenden.
