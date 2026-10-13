# ADR 0154 – First M7 consumer: Win32 native menus / Erster M7-Consumer: native Win32-Menüs

**Status:** Accepted
**Date:** 2026-10-13
**Predecessor:** [0151](0151-m6-native-host-peer-boundary.md)

## English

### Context

M7 needs a real desktop-integration consumer. Core already owns `MenuBarModel`, recursive
`MenuModel`, `MenuItem` and `Command`, while `MenuInteractionController` owns transient state for
SASD-rendered Terminal/Rendered menus. Win32 menus already provide popup navigation, highlighting,
keyboard handling and dismissal; mirroring that state in Core would create competing state machines.

### Decision

The first native-menu slice is private to `SASD::UI::Native::Win32`.
`Win32WindowBackend::syncMenuBar()` builds a complete owned HMENU tree from the current
`MenuBarModel`; `clearMenuBar()` removes it. Structural model changes require explicit
synchronization. The binding retains only private lifetime-safe `Command::Reference` values and native
command identifiers. It is not a second semantic menu model and no public cross-platform NativeMenu
abstraction is introduced.

Windows owns transient native menu interaction. `WM_INITMENU`/`WM_INITMENUPOPUP` refresh live command
text and enabled state, while `WM_COMMAND` copies the matching reference and executes the same Core
`Command`. Command callbacks may release their command; no binding state is accessed after execution
begins. Invalid text, expired commands and unknown identifiers fail closed.

The backend advertises `BackendCapabilities::native_menus`. AppKit/GTK menus and the other M7 areas
remain deferred.

### Consequences

Rendered/Terminal menus continue to use `MenuInteractionController`; native Win32 menus use Windows'
interaction model. Both paths converge only at `Command` activation. Command state changes do not
require rebuilding the HMENU tree, but structural menu changes do require another synchronization call.

## Deutsch

### Kontext

M7 benötigt einen realen Consumer für Desktop-Integration. Core besitzt bereits `MenuBarModel`,
rekursives `MenuModel`, `MenuItem` und `Command`; `MenuInteractionController` besitzt den transienten
Zustand der selbst dargestellten Terminal-/Rendered-Menüs. Win32-Menüs bringen Popup-Navigation,
Highlighting, Tastaturbedienung und Dismissal selbst mit. Eine Spiegelung dieses Zustands im Core
würde konkurrierende Zustandsmaschinen erzeugen.

### Entscheidung

Der erste Native-Menü-Slice bleibt privat in `SASD::UI::Native::Win32`.
`Win32WindowBackend::syncMenuBar()` baut aus dem aktuellen `MenuBarModel` einen vollständigen,
besitzenden HMENU-Baum; `clearMenuBar()` entfernt ihn. Strukturelle Modelländerungen benötigen eine
explizite erneute Synchronisation. Das Binding hält nur private lebenszeitsichere `Command::Reference`
und native Command-IDs. Es ist kein zweites semantisches Menümodell, und es entsteht keine öffentliche
plattformübergreifende NativeMenu-Abstraktion.

Windows besitzt den transienten nativen Menüzustand. `WM_INITMENU`/`WM_INITMENUPOPUP` aktualisieren
Live-Command-Text und Enabled-State; `WM_COMMAND` kopiert die passende Reference und führt denselben
Core-`Command` aus. Callback-Code darf seinen Command freigeben; nach Beginn der Ausführung wird kein
Binding-Zustand mehr verwendet. Fehlerhafte Texte, abgelaufene Commands und unbekannte IDs schlagen
geschlossen fehl.

Das Backend meldet `BackendCapabilities::native_menus`. AppKit-/GTK-Menüs und die übrigen M7-Themen
bleiben zurückgestellt.

### Folgen

Rendered-/Terminal-Menüs verwenden weiterhin den `MenuInteractionController`; native Win32-Menüs
verwenden das Interaktionsmodell von Windows. Beide Wege treffen sich erst bei der Aktivierung eines
`Command`. Command-Zustandsänderungen erfordern keinen Neuaufbau des HMENU-Baums; Strukturänderungen
erfordern einen erneuten Synchronisationsaufruf.
