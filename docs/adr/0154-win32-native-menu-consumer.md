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
text and enabled state. `WM_COMMAND` validates the native identifier, copies the matching
lifetime-safe reference, and queues an application-level `CommandActivationEvent`; the native
callback then ends. Application/host code may observe the event and explicitly execute the same Core
`Command` later. The event is not bubbled through Widgets and is not implicitly executed by
`Application`. Command callbacks may release their command; no binding state is accessed after the
reference has been queued. Invalid text, expired commands and unknown identifiers fail closed.

Native labels escape every literal SASD ampersand as `&&`; this slice has no mnemonic model and must
not accidentally assign Win32 mnemonics. Structural menu replacement builds a separate complete
binding, installs it with direct `SetMenu` return-value checking, redraws with direct
`DrawMenuBar` return-value checking, and commits binding ownership only after success. Failure rolls
back to the old menu where Win32 permits; an installed HMENU is never destroyed. Structural
resynchronization remains explicit, while command text/enabled refresh remains owned by
`WM_INITMENU`/`WM_INITMENUPOPUP`.

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
Live-Command-Text und Enabled-State. `WM_COMMAND` validiert die native ID, kopiert die passende
lebenszeitsichere Reference und legt ein application-level `CommandActivationEvent` in die Queue;
danach endet der native Callback. Application/Host kann das Event beobachten und denselben
Core-`Command` später explizit ausführen. Das Event wird nicht durch Widgets gebubbelt und von
`Application` nicht implizit ausgeführt. Callback-Code darf seinen Command freigeben; nach dem
Einreihen wird kein Binding-Zustand mehr verwendet. Fehlerhafte Texte, abgelaufene Commands und
unbekannte IDs schlagen geschlossen fehl.

Literal-Ampersands aus SASD-Labels werden als `&&` escaped; dieses Slice besitzt kein Mnemonic-Modell
und darf keine Win32-Mnemonics versehentlich erzeugen. Die strukturelle Menüersetzung baut zunächst
ein vollständiges separates Binding, installiert es mit Prüfung des direkten `SetMenu`-Rückgabewerts,
zeichnet mit Prüfung des direkten `DrawMenuBar`-Rückgabewerts und committed Ownership erst danach.
Fehler rollen soweit der Win32-Vertrag es erlaubt auf das alte Menü zurück; ein installiertes HMENU
wird nie zerstört. Strukturelle Resynchronisierung bleibt explizit, Command-Text/Enabled-Refresh
bleibt bei `WM_INITMENU`/`WM_INITMENUPOPUP`.

Das Backend meldet `BackendCapabilities::native_menus`. AppKit-/GTK-Menüs und die übrigen M7-Themen
bleiben zurückgestellt.

### Folgen

Rendered-/Terminal-Menüs verwenden weiterhin den `MenuInteractionController`; native Win32-Menüs
verwenden das Interaktionsmodell von Windows. Beide Wege treffen sich erst bei der Aktivierung eines
`Command`. Command-Zustandsänderungen erfordern keinen Neuaufbau des HMENU-Baums; Strukturänderungen
erfordern einen erneuten Synchronisationsaufruf.
