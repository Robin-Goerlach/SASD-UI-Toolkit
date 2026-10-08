# ADR 0134: Rendered menu frame and SDL3 overlay interaction policy

Status: Accepted
Date: 2026-10-08
Predecessors: ADR 0133 (Rendered menu popup snapshots)

## English

The Rendered menu system composes one owned `RenderedMenuFramePresentationSnapshot` containing the
persistent menu bar and every open popup layer in paint order. Core `MenuBarModel` and
`MenuInteractionController` are observed only while building the frame. No `MenuModel`, `MenuItem`,
`Command`, measurement-context or SDL pointer crosses that boundary. Root and nested popup placement
is resolved once against the supplied logical viewport: a child prefers the right side of its displayed
parent row and falls back to the left side. Rendering and hit testing consume the exact final frame;
stale or malformed frames fail closed and composition does not partially mutate the base `DisplayList`.

Rendered pointer input is a translation layer, not a second menu state machine. It stores only value
identities (`top-level index`, `popup level`, `row index`) for press/release matching and delegates
selection, submenu opening, closing and command resolution to `MenuInteractionController`. An engaged
menu hit consumes the physical event, including outside dismissal, so covered Widgets cannot receive
click-through input. Command activation remains two-phase: Core closes transient state and returns a
`Command::Reference`; the host stabilizes presentation and focus, then executes the reference without
performing further member access across application callback code.

The SDL3 demo treats the rendered menu as the highest-priority transient overlay. F10 is host policy
for entering or leaving menu mode. Opening a menu cancels an open ComboBox preview, and a menu owns
pointer and relevant keyboard events before ComboBox or ordinary Widget routing. The persistent bar
occupies its final logical row; the form is arranged below that row without putting menu geometry into
Core layout types. Resize rebuilds the frame against the new viewport and conservatively closes the
transient menu if the new geometry cannot be represented.

This decision intentionally does not introduce a general overlay manager, hover-delay policy or a
second action framework. Those remain deferred until multiple concrete consumers require them.

## Deutsch

Das Rendered-Menüsystem komponiert einen besitzenden `RenderedMenuFramePresentationSnapshot`, der die
persistent sichtbare Menüleiste und alle geöffneten Popup-Ebenen in Paint-Reihenfolge enthält. Das Core-
`MenuBarModel` und der `MenuInteractionController` werden nur während des Frame-Aufbaus beobachtet. Kein
`MenuModel`, `MenuItem`, `Command`, Measurement-Kontext oder SDL-Pointer überschreitet diese Grenze.
Root- und verschachteltes Popup-Placement wird genau einmal gegen den logischen Viewport bestimmt: Ein
Child-Popup wird bevorzugt rechts neben seiner tatsächlich dargestellten Parent-Row platziert und fällt
bei Platzmangel nach links zurück. Rendering und Hit-Testing verwenden exakt diesen finalen Frame;
veraltete oder fehlerhafte Frames schlagen geschlossen fehl, und die Basis-`DisplayList` wird bei einem
Kompositionsfehler nicht teilweise verändert.

Rendered-Pointerinput ist nur eine Übersetzungsschicht und keine zweite Menü-Zustandsmaschine. Für das
Press/Release-Matching werden ausschließlich Value-Identitäten (`top-level index`, `popup level`,
`row index`) gespeichert; Selection, Submenu-Öffnung, Schließen und Command-Auflösung delegiert die
Schicht an den `MenuInteractionController`. Ein vom Menü angenommener Pointer-Event wird immer
konsumiert, auch beim Outside-Dismissal, damit kein Click-through zu überdeckten Widgets entsteht. Die
Command-Aktivierung bleibt zweiphasig: Der Core schließt den transienten Zustand und liefert eine
`Command::Reference`; der Host stabilisiert zunächst Presentation und Focus und führt erst danach die
Referenz aus, ohne während oder nach fremdem Callback-Code unzulässige Memberzugriffe auszuführen.

Die SDL3-Demo behandelt das Rendered-Menü als Overlay mit höchster transienter Priorität. F10 ist eine
Host-Policy zum Betreten und Verlassen des Menümodus. Beim Öffnen eines Menüs wird ein offener ComboBox-
Preview abgebrochen; Menü-Pointer und relevante Keyboard-Events werden vor ComboBox- und normalem
Widget-Routing verarbeitet. Die persistente Menüleiste belegt ihre finale logische Zeile; das Formular
wird darunter angeordnet, ohne Menügeometrie in Core-Layouttypen einzubauen. Bei Resize wird der Frame
gegen den neuen Viewport neu erstellt; kann die neue Geometrie nicht dargestellt werden, wird das
transiente Menü konservativ geschlossen.

Bewusst werden weder ein allgemeiner Overlay-Manager noch Hover-Delay-Policy oder ein zweites Action-
Framework eingeführt. Diese Themen bleiben zurückgestellt, bis mehrere konkrete Verbraucher sie
rechtfertigen.
