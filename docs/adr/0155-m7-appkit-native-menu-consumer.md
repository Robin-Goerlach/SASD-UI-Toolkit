# ADR 0155 – M7 AppKit NSMenu validates application-global native-menu ownership / M7 AppKit NSMenu validiert prozessweites Native-Menü-Ownership

**Status:** Accepted
**Date:** 2026-10-14
**Predecessor:** [0154](0154-win32-native-menu-consumer.md)

## English

### Context

The Core `MenuBarModel`, recursive `MenuModel`/`MenuItem` structure and `Command::Reference` already
serve the Win32 native-menu consumer. AppKit is a useful second consumer because `NSApplication.mainMenu`
is process-global rather than bound to one window. Treating it as a per-window HMENU would conceal an
important ownership difference.

### Decision

`AppKitWindowBackend::syncMenuBar()` builds a complete owned `NSMenu` projection from the Core model
and installs it as `NSApplication.sharedApplication.mainMenu`. It retains no `MenuModel*` or
`MenuItem*`; labels and metadata are copied and command entries retain only `Command::Reference`.
Structural changes require explicit resynchronization. The old projection remains untouched until the
replacement is complete and installable.

Only one SASD AppKit menu owner is supported. A second backend fails closed, while an unrelated main
menu already present before the first sync is saved as the restoration target. The adapter does not own
`NSApplication`; the prior main menu remains owned by its previous process owner and is restored only
if it is still safe to replace the SASD projection during `clearMenuBar()`/shutdown. AppKit owns
transient popup interaction and validation; it is not mirrored in Core.

An Objective-C++ target/delegate copies a `Command::Reference` and queues `CommandActivationEvent`.
No application callback and no `Command::execute()` occur inside the Objective-C action callback.
Expired or disabled references are disabled and cannot queue activation. `menuNeedsUpdate:` and
`validateMenuItem:` refresh live text/enabled state. UTF-8 labels are strict, including supplementary
Unicode; malformed text rejects a complete sync. `keyEquivalent` remains empty because `ShortcutMap`
is authoritative, and AppKit does not emulate Win32 ampersand mnemonics.

No universal NativeMenu abstraction is introduced. The proven common boundary is semantic snapshot/
resync, owned native projection, `Command::Reference`, queued activation and native-owned transient
interaction. Win32 remains per-window HMENU/WM_COMMAND/WM_INITMENU*, while AppKit remains global
NSMenu/target-action/validation. GTK is the next real validation consumer.

## Deutsch

### Kontext

Der Core-Vertrag aus `MenuBarModel`, rekursivem `MenuModel`/`MenuItem` und `Command::Reference` dient
bereits dem nativen Win32-Menü. AppKit ist ein zweiter sinnvoller Consumer, weil
`NSApplication.mainMenu` prozessweit statt fenstergebunden ist. Eine künstliche Abbildung als
fenstergebundenes HMENU würde diesen Ownership-Unterschied verdecken.

### Entscheidung

`AppKitWindowBackend::syncMenuBar()` baut aus dem Core-Modell eine vollständige besitzende `NSMenu`-
Projektion und installiert sie als `NSApplication.sharedApplication.mainMenu`. Es behält weder
`MenuModel*` noch `MenuItem*`; Labels und Metadaten werden kopiert, Commands nur als
`Command::Reference` gehalten. Strukturänderungen benötigen eine explizite Resynchronisierung. Die
alte Projektion bleibt erhalten, bis die neue vollständig aufgebaut und installierbar ist.

Es gibt nur einen SASD-AppKit-Menübesitzer. Ein konkurrierendes Backend schlägt geschlossen fehl;
ein vor dem ersten Sync vorhandenes fremdes Main Menu wird als Restore-Ziel gespeichert. Der Adapter besitzt nicht `NSApplication`; das
vorherige Main Menu bleibt beim ursprünglichen Prozessbesitzer und wird bei `clearMenuBar()`/Shutdown
nur wiederhergestellt, wenn der SASD-Baum noch sicher ersetzt werden kann. AppKit besitzt Popup-
Interaktion und Validation; sie wird nicht im Core gespiegelt.

Ein privates Objective-C++ Target/Delegate kopiert eine `Command::Reference` und legt ein
`CommandActivationEvent` in die Queue. Im Objective-C-Action-Callback laufen weder Application-
Callbacks noch `Command::execute()`. Abgelaufene oder deaktivierte References werden disabled und
queue’n keine Aktivierung. `menuNeedsUpdate:` und `validateMenuItem:` aktualisieren Text/Enabled-State.
UTF-8-Labels sind strikt und unterstützen auch Supplementary Unicode; fehlerhafter Text verwirft eine
vollständige Synchronisierung. `keyEquivalent` bleibt leer, weil die `ShortcutMap` autoritativ ist;
Win32-Ampersand-Mnemonics werden in AppKit nicht nachgebildet.

Eine allgemeine NativeMenu-Abstraktion wird nicht eingeführt. Bewiesen gemeinsam sind semantischer
Snapshot/Resync, besitzende native Projektion, `Command::Reference`, queued Activation und native
Interaktion. Win32 bleibt fenstergebundenes HMENU mit WM_COMMAND/WM_INITMENU*, AppKit bleibt
prozessweites NSMenu mit Target/Action/Validation. GTK ist der nächste reale Validierungs-Consumer.
