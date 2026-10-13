# ADR 0152 – M6 GTK validates the native rendered-host boundary

**Status:** Accepted
**Date:** 2026-10-13
**Predecessor:** [ADR 0151](0151-m6-native-host-peer-boundary.md)

## English

GTK4 is the optional second native host. `SASD::UI::Native::GTK` is controlled by
`SASD_UI_BUILD_GTK_ADAPTER` (default `OFF`) and discovers GTK4, Cairo and Pango with pkg-config
only inside the adapter. `GtkWindowBackend` directly implements `Backend`, `RenderDevice` and
`RenderedMeasurementContext`; no public native base class or GTK child-control peers are added.

The host pumps the default GLib main context from `pollEvent()`, retains a complete copied
`DisplayList`, and replays it from the GTK draw callback. Cairo executes the commands and Pango is
the shared font policy for measurement and drawing. Coordinates remain logical; GTK's Cairo context
owns device scaling and the adapter does not scale twice. Key, pointer, surface-leave, close and
resize observations use existing Core events. Printable committed text is supported, while IME
preedit remains deferred and `ime` is not advertised. Resources are UI-thread-affine and released
through the host lifecycle. The first slice is build-tree-only until a portable installed GTK
pkg-config consumer contract is proven. Xvfb is the CI integration environment.

## Deutsch

GTK4 ist der optionale zweite Native Host. `SASD::UI::Native::GTK` wird durch
`SASD_UI_BUILD_GTK_ADAPTER` (Standard `OFF`) aktiviert und findet GTK4, Cairo und Pango nur im
Adapter über pkg-config. `GtkWindowBackend` implementiert direkt `Backend`, `RenderDevice` und
`RenderedMeasurementContext`; eine öffentliche Native-Basisklasse und native GTK-Child-Peers werden
nicht eingeführt.

Der Host pumpt den Standard-GLib-MainContext in `pollEvent()`, hält eine vollständige kopierte
`DisplayList` und spielt sie im GTK-Draw-Callback erneut ab. Cairo führt die Befehle aus, Pango ist
die gemeinsame Font-Policy für Messung und Zeichnung. Koordinaten bleiben logisch; die GTK-Cairo-
Zeichenfläche verwaltet die Geräteskalierung, ohne doppelte Skalierung. Tastatur, Pointer,
Surface-Leave, Close und Resize verwenden bestehende Core-Events. Druckbarer Commit-Text wird
unterstützt, IME-Preedit bleibt offen und `ime` wird nicht gemeldet. Ressourcen sind UI-Thread-
gebunden und werden im Host-Lifecycle freigegeben. Der erste Schnitt bleibt build-tree-only, bis
ein portabler installierter GTK-pkg-config-Consumer-Vertrag bewiesen ist. Xvfb ist die CI-
Integrationsumgebung.
