# ADR 0156 – M7 GTK4 validates window-local GMenu/GAction native-menu projection / M7 GTK4 validiert fensterlokale GMenu/GAction-Native-Menüprojektion

**Status:** Accepted
**Date:** 2026-10-14
**Predecessor:** [0155](0155-m7-appkit-native-menu-consumer.md)

## English

### Context

GTK4 no longer provides the deprecated GTK3 `GtkMenuBar`/`GtkMenu`/`GtkAction` model. Its modern
native menu path is `GMenu`/`GMenuModel`, `GSimpleActionGroup`/`GSimpleAction` and
`GtkPopoverMenuBar`. Unlike AppKit's process-global main menu, this projection belongs to one GTK
window and participates in the window's widget layout.

### Decision

`GtkWindowBackend::syncMenuBar()` builds a complete private projection before changing the window:
an owned recursive `GMenu` tree, a `GSimpleActionGroup`, numeric-derived private action names and a
`GtkPopoverMenuBar`. The widget is installed above the existing drawing area in a private vertical
`GtkBox`. The drawing area remains the Rendered surface, and its actual GTK allocation remains the
source for window size, resize events and pointer coordinates. `clearMenuBar()` removes only the
projection and leaves the drawing area, event controllers and retained frame alive.

The binding stores no `MenuBarModel*`, `MenuModel*` or `MenuItem*`. Structural changes require an
explicit resynchronization. Live command text and enabled state are refreshed deterministically from
the GTK host event pump. GAction callbacks validate the private action identity and live
`Command::Reference`, then queue `CommandActivationEvent`; they never call `Command::execute()` or
application code. Binding callback state is invalidated before old GTK objects are released, so stale
action groups fail closed after a resync or shutdown.

Labels are strict UTF-8 without embedded NUL, preserve literal ampersands and underscores, and do not
invent a Core mnemonic policy. Menu shortcut metadata is not registered as a GTK accelerator because
`ShortcutMap` remains the authoritative SASD input route. Empty `syncMenuBar()` removes the optional
menu projection.

### Consequences

GTK becomes a third real native-menu consumer without requiring `GtkApplication` or changing the
existing host lifecycle. Its window-local ownership and layout effects remain visible in the adapter
instead of being hidden behind an artificial cross-platform native-menu lifecycle class.

## Deutsch

### Kontext

GTK4 besitzt nicht mehr das veraltete GTK3-Modell aus `GtkMenuBar`/`GtkMenu`/`GtkAction`. Der moderne
native Menüpfad verwendet `GMenu`/`GMenuModel`, `GSimpleActionGroup`/`GSimpleAction` und
`GtkPopoverMenuBar`. Anders als AppKits prozessweites Main Menu gehört diese Projektion zu genau
einem GTK-Fenster und nimmt Platz im Widget-Layout ein.

### Entscheidung

`GtkWindowBackend::syncMenuBar()` baut vor jeder Änderung am Fenster eine vollständige private
Projektion: einen besitzenden rekursiven `GMenu`-Baum, eine `GSimpleActionGroup`, aus numerischen IDs
abgeleitete private Action-Namen und eine `GtkPopoverMenuBar`. Das Widget wird oberhalb der bestehenden
DrawingArea in einer privaten vertikalen `GtkBox` installiert. Die DrawingArea bleibt die Rendered-
Fläche; ihre tatsächliche GTK-Allokation bleibt Quelle für Fenstergröße, Resize-Events und Pointer-
Koordinaten. `clearMenuBar()` entfernt nur die Projektion und erhält DrawingArea, Event-Controller und
den gespeicherten Frame.

Das Binding behält weder `MenuBarModel*`, `MenuModel*` noch `MenuItem*`. Strukturänderungen benötigen
expliziten Resync. Live-Command-Text und Enabled-State werden deterministisch aus dem GTK-Eventpump
aktualisiert. GAction-Callbacks validieren private Action-Identität und live `Command::Reference`,
legen danach ein `CommandActivationEvent` in die Queue und rufen weder `Command::execute()` noch
Application-Code auf. Der Callback-Zustand wird vor dem Freigeben alter GTK-Objekte invalidiert;
stale Action-Groups schlagen nach Resync oder Shutdown geschlossen fehl.

Labels sind striktes UTF-8 ohne eingebettetes NUL, bewahren literale Ampersands und Unterstriche und
erfinden keine Core-Mnemonic-Policy. Shortcut-Metadaten werden nicht als GTK-Accelerator registriert,
weil `ShortcutMap` die autoritative SASD-Eingaberoute bleibt. Ein leeres `syncMenuBar()` entfernt die
optionale Menüprojektion.

### Folgen

GTK ist damit ein dritter realer Native-Menü-Consumer, ohne `GtkApplication` oder den bestehenden
Host-Lifecycle umzubauen. Fensterlokales Ownership und Layout-Auswirkungen bleiben im Adapter sichtbar,
statt hinter einer künstlichen plattformübergreifenden Native-Menü-Lifecycle-Klasse verborgen zu werden.
