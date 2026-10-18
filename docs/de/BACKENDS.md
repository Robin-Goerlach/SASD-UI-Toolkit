# Backend- und Plattformstrategie

## Ziel

Das SASD UI Toolkit soll eine gemeinsame semantische API anbieten, ohne die Eigenheiten der Zielplattformen zu leugnen. Ein Backend übersetzt Widgets, Events, Layout-Messungen und Plattformdienste in die jeweilige Umgebung.

## Backend-Typen

### 1. Terminal-Backend

Das Terminal-Backend ist das erste geplante vollständige Backend und dient zugleich als Architekturtest.

Ziele:

- ANSI-/VT-Sequenzen, wo verfügbar
- Tastatureingabe und Fokusnavigation
- Resize-Erkennung
- 16/256/True-Color abhängig von Capabilities
- Mausunterstützung optional, sofern das Terminal sie anbietet
- Unicode mit schrittweise wachsender Graphem-/Breitenunterstützung
- kein Zwang zu ncurses in der öffentlichen API

Eine spätere Implementierung darf intern systemnahe Hilfen oder optionale Bibliotheken verwenden, solange sie hinter dem Backend bleiben.

### Aktueller M2-Implementierungsstand

Der erste Terminal-Baustein ist als separat linkbares `SASD::UI::Terminal`-Target angelegt. Er enthält eine vollständig headless testbare Off-Screen-`ScreenBuffer`-Abstraktion für Terminal-Zellen. Dadurch können Rendering-, Clipping- und Resize-Regeln auf allen CI-Plattformen geprüft werden, ohne bereits eine reale Konsole zu benötigen.

Die Terminal-Schicht besitzt inzwischen eine eigene versionierte `TextMetrics`-Abstraktion. UTF-8 wird zentral dekodiert; Narrow/Wide/Fullwidth werden als 1/2 Zellen gemessen, East-Asian-Ambiguous ist explizit als Narrow- oder Wide-Policy wählbar. `CellRole::wide_lead` und `wide_continuation` halten Zwei-Spalten-Belegung auch im Off-Screen-Buffer sichtbar. Die aktuell generierten Tabellen sind bewusst auf Unicode 17.0.0 gepinnt und über die API als solche identifizierbar.

`Window` und `Label` sind inzwischen als erste semantische M2-Widgets vorhanden. Ein `TerminalPresentationSink` konsumiert deren normale Visual-Invalidierung über den `PresentationCoordinator` und rendert deterministisch in den `ScreenBuffer`. Dabei werden UTF-8-Sequenzen sicher dekodiert, Parent-Offsets aufgelöst, Widget-/Screen-Grenzen geclippt und malformed UTF-8 durch U+FFFD ersetzt. Unbekannte konkrete Widget-Typen werden bewusst als `deferred` behandelt, statt ihre Updates still zu verlieren.

Zero-Width-/Combining-/Format-Sequenzen und gewöhnliche Terminal-Control-Zeichen werden vom heutigen einfachen Cell-Modell noch nicht verlustfrei repräsentiert. Solche Label-Updates werden deshalb vor jeder Buffer-Änderung `deferred`; die zuletzt erfolgreich synchronisierte Darstellung bleibt erhalten. Der plattformneutrale `Label`-Core berechnet weiterhin keine terminalspezifische intrinsische Textbreite. Vollständige Grapheme-Segmentierung und ein Upgrade der gepinnten Unicode-Daten bleiben eigene M2-Schritte.

Geometrieänderungen und Child-Removal verwenden inzwischen einen konservativen Subtree-Refresh: das Terminal-`Window` leert seinen Off-Screen-Buffer nur bei solchem Geometrie-/Strukturschaden, anschließend werden auch cleane Descendants deterministisch erneut gerendert. Normale Text-/Fokusänderungen bleiben inkrementell. Dirty Rectangles und Region-Merging sind bewusst spätere Optimierungen.

`Button` ist inzwischen als erstes interaktives Control integriert. Terminal-Measurement und -Presentation verwenden dieselbe feste vierzellige Chrome: normal `[ caption ]`, fokussiert `> caption <`, disabled `( caption )`. Enter/Space aktiviert den fokussierten Button auf Key-Press; das ist absichtlich terminaltauglich, weil ANSI/VT normalerweise keine zuverlässigen Key-Up-Ereignisse liefert.

`TextField` ist inzwischen als einzeiliger Editor integriert. Terminal-Measurement reserviert zwei Delimiter plus eine Caret-Zelle; bei zu kleinen Bounds scrollt die Presentation horizontal auf Unicode-Scalar-Grenzen. Der Hardware-Caret ist als separate optionale Position im `TerminalPresentationSink` modelliert und überschreibt keine Glyph-Zelle. Combining-/ZWJ-Inhalte bleiben im Core erhalten, werden vom aktuellen einfachen Cell-Modell aber weiterhin `deferred`.

`FocusTraversal` liefert inzwischen deterministische Tab-/Shift+Tab-Navigation über sichtbare/enabled Controls. Zusätzlich serialisiert `AnsiFrameEncoder` den vollständigen `ScreenBuffer` samt optionalem Hardware-Caret in deterministische UTF-8-/ANSI-VT-Frames. Der Encoder ist absichtlich I/O-frei und behandelt weder Raw Mode noch Alternate Screen oder OS-Handles.

`TerminalDevice`/`TerminalSession` bilden nun die reale Device-/Session-Grenze. POSIX verwendet TTY-Erkennung, `termios`, `TIOCGWINSZ` und retry-sicheres `write`; Windows speichert/wiederherstellt Console Modes und Codepages, aktiviert Virtual Terminal Processing und schreibt über `WriteFile`. Alternate Screen und Raw Input werden RAII-sicher mit transaktionalem Rollback verwaltet. Hosted-CI testet die Session-Semantik deterministisch über `MockTerminalDevice`, während die nativen Adapter auf ihren jeweiligen Plattformen kompiliert werden.

`TerminalDevice::readAvailable()` liefert nun nichtblockierend verfügbare Inputbytes. `AnsiInputDecoder` verarbeitet gesplittete UTF-8-/CSI-/SS3-Sequenzen inkrementell und übersetzt Enter, Tab, Backspace, Space, Pfeile, Home/End, Delete, PageUp/PageDown, Shift+Tab, logische Ctrl-Buchstaben aus eindeutigen C0-Bytes sowie F1–F12 einschließlich ausgewählter xterm-Modifier in die bestehenden semantischen Events. Ein einzelnes ESC bleibt bis zum expliziten Eventloop-Flush gepuffert. Das Terminal meldet weiterhin keine Clipboard-Capability; Select All bleibt als semantisches Command nutzbar, ohne OSC52 oder ein System-Clipboard vorzutäuschen.

`TerminalEventPump` verbindet Größenabfrage, nichtblockierenden Input und Incomplete-Sequence-Timeout deterministisch zu semantischen Events. Größenänderungen werden als vorhandenes `ResizeEvent` erzeugt; der Startzustand erzeugt kein künstliches Resize. `TerminalBackend : Backend` integriert Session und EventPump in den normalen `Application`-Lifecycle. Die neue ausführbare `sasd_ui_terminal_demo` nutzt diesen Pfad mit TextField, Buttons, Tab-/Shift+Tab-Fokus, Resize und realer ANSI-Ausgabe. CI kompiliert die Demo auf Linux, macOS und Windows; ein manueller interaktiver Smoke-Test bleibt für das M2-Exit-Kriterium erforderlich.

`Color`/`TextStyle` ergänzen nun einen bewusst kleinen backendneutralen Stylevertrag für `Label`, `Button` und `TextField`. Unterstützt werden 16 benannte Vordergrundfarben sowie bold/dim/underline/inverse. Terminalzellen speichern den aufgelösten Style; der ANSI-Encoder erzeugt daraus SGR-Sequenzen. Fokus und Disabled werden nur als Terminal-Presentation-Overlay ergänzt. Hintergrundfarben, Cascade/Themes, RGB/Alpha und Fontmodelle bleiben bewusst später.

F1–F12 sind nun als backendneutrale `Key`-Identitäten implementiert und werden aus verbreiteten SS3-/CSI-/xterm-Sequenzen dekodiert. Noch offen sind insbesondere erweiterte Keyboard-Protokolle (F13+, Kitty/CSI-u/modifyOtherKeys), vollständige Grapheme-/ZWJ-Darstellung sowie Pointer-/Hit-Test-Interaktion.

### 2. Rendered Desktop / experimenteller SDL3-Adapter

M3 besitzt nun sowohl den headless SDL3-Proof als auch ein reales fenstergebundenes Backend unterhalb
der generischen Rendered-Schicht. `SASD::UI::Rendered::SDL3` verwendet für Software-Surface-Tests und
Desktopfenster dieselbe private SDL3-Render-/Metrik-Policy.

`Sdl3WindowBackend` implementiert `Backend`, `RenderDevice` und
`RenderedMeasurementContext`. Window/Renderer/Font werden transaktional erzeugt, vollständige
DisplayList-Frames präsentiert, Close/Resize/Key/Committed-Text-Events übersetzt und logische
Layoutgröße von physischer Pixelgröße/Display Scale getrennt gehalten. SDL-Typen bleiben aus Core und
generischer Rendered-API heraus.

`PresentationCoordinator::replay()` rekonstruiert eine Presentation-Surface aus einem cleanen
semantischen Widgetbaum, ohne geschützte Widget-Invalidierung offenzulegen.

SDL3-Keyevents normalisieren logische A–Z-Keycodes auf die backendneutrale `Key`-Identität, während
SDL-Texteingabe weiterhin als getrennte `TextInputEvent`-Sequenz läuft. Das initialisierte
Fensterbackend besitzt den konkreten UTF-8-SDL-Clipboard-Dienst und gibt ihn nur als nichtbesitzende
`Backend::clipboard()`-Referenz heraus; nach Shutdown wird die Capability wieder false.

Die dedizierte CI baut SDL3-Adapter und Desktop-Demo, verwendet ein reales verstecktes SDL-Fenster
über den Offscreen-Videotreiber und prüft den End-to-End-Pfad Semantic Widget ->
RenderedPresentationSink -> DisplayList -> SDL3-Window.

Der Adapter bleibt opt-in und nur im Build-Tree verfügbar. Pointer/Hit-Testing, reichhaltigere
IME-Composition und breitere Desktopvalidierung folgen in weiteren M3-Schnitten.

### 3. Native Windows-Backend

Der erste M6-Schnitt ist das separat linkbare Target `SASD::UI::Native::Win32`. Sein
`Win32WindowBackend` besitzt ein Top-Level-HWND und übersetzt Win32-Nachrichten in die bestehende
Core-`Backend`-/`Event`-Pipeline. Es ist bewusst ein nativer Host und kein zweites Button-/TextField-
Semantiksystem: `EventDispatcher`, `FocusManager`, `PointerRouter`, Commands und Model/View bleiben
maßgeblich. HWND-Ownership ist RAII- und UI-Thread-gebunden; `WM_NCCREATE`/`GWLP_USERDATA` bleiben
innerhalb des Adapters. Der Host kann später die bestehende Rendered-Pipeline oder einen konkreten
Native-Peer-Sink speisen. `WM_MOUSELEAVE` wird in den bestehenden `PointerSurfaceEvent`-Vertrag
übersetzt, sodass Core Hover/Capture deterministisch beenden kann.

Bewusst zurückgestellt sind IME-Komposition, native Child-Controls, Accessibility, Drag & Drop,
weitergehende DPI-Policy und GUI-Automation. Nicht unterstützte Nachrichten werden fail-closed
ignoriert.

#### Nativer Win32-Menü-Consumer (M7 Slice 1)

`Win32WindowBackend::syncMenuBar()` projiziert den Core-`MenuBarModel` explizit in einen privaten,
besitzenden HMENU-Baum; `clearMenuBar()` entfernt ihn. Die Projektion umfasst rekursive Submenus,
Separatoren, Command-Texte, Shortcut-Anzeige und Enabled-State. Strukturänderungen benötigen einen
erneuten expliziten Synchronisationsaufruf; `WM_INITMENU`/`WM_INITMENUPOPUP` aktualisieren Live-Command-
Zustand.

Windows besitzt Popup-Navigation, Highlighting, Tastaturnavigation, Submenu-Öffnung und Escape-
Verhalten. Der native Pfad verwendet deshalb nicht den `MenuInteractionController`; dieser bleibt
Besitzer der transienten Interaktion für selbst dargestellte Terminal-/Rendered-Menüs. `WM_COMMAND`
löst eine private lebenszeitsichere `Command::Reference` auf und legt ein application-level
`CommandActivationEvent` in die Queue; danach endet der native Callback. `Application` reicht das
Event nur an den Application-Handler weiter und führt den Command nicht implizit aus. So kann der
Host zuerst den Präsentationszustand stabilisieren und den Command anschließend explizit ausführen.
Literal-Ampersands aus SASD-Text werden wegen des fehlenden Mnemonic-Modells als `&&` in nativen
Labels gespeichert. Menüersetzung baut ein separates Binding, prüft die direkten Rückgabewerte von
`SetMenu`/`DrawMenuBar`, rollt soweit möglich zurück und committed Ownership erst nach erfolgreicher
Installation. Strukturelle Resynchronisierung bleibt explizit; Command-Text/Enabled-Refresh bleibt
bei `WM_INITMENU*`. Es gibt kein Win32-spezifisches semantisches Menümodell und keine öffentliche
plattformübergreifende Native-Menü-Basisklasse.

#### Win32-Open-File-Dialog-Consumer (M7-File-Dialog-Slice 1)

`Backend::fileDialogs()` ist ein optionaler Host-Service-Accessor. Der portable Service nimmt einen
`OpenFileDialogRequest` an und liefert eine nicht-null `FileDialogRequestId`; der Abschluss wird immer
als application-level `FileDialogResultEvent` in die Queue gelegt, niemals als Pfad zurückgegeben und
niemals per Callback zugestellt. Ein Win32-Request darf synchron in die native modale Schleife gehen,
aber innerhalb davon läuft kein Application-Code. Version 1 erlaubt höchstens einen Pending-Request.

Der Win32-Service verwendet `IFileOpenDialog` mit `FOS_FORCEFILESYSTEM`, `FOS_FILEMUSTEXIST`,
`FOS_PATHMUSTEXIST` und `FOS_NOCHANGEDIR` und besitzt als Owner das vorhandene HWND des
`Win32WindowBackend`. Titel werden strikt von UTF-8 nach UTF-16 konvertiert. Akzeptierte Ergebnisse
bewahren den nativen Pfad als `std::filesystem::path` ohne Canonicalization oder verlustbehaftete ACP-
Konvertierung; Cancel ist ein pfadloses `cancelled`-Result, technische Fehler sind pfadlose
`failed`-Results. COM wird nur für die Dialogoperation auf dem aufrufenden UI-Thread initialisiert und
ausgeglichen. Filter, Save, Ordner und Mehrfachauswahl gehören bewusst noch nicht zu diesem ersten
Vertrag. Siehe [ADR 0158](../adr/0158-m7-file-dialog-request-result-and-win32-open-file.md).

#### AppKit-Open-File-Consumer (M7 File-Dialog-Slice 2)

`AppKitWindowBackend::fileDialogs()` stellt denselben optionalen Core-Service wie Win32 bereit. AppKit
startet `NSOpenPanel` mit aktivierter Dateiauswahl, deaktivierter Ordnerauswahl und deaktivierter
Mehrfachauswahl über `beginSheetModalForWindow:completionHandler:` am privaten `NSWindow` des
Backends. Die Request-ID wird vor dem Abschluss zurückgegeben; später wird genau ein application-level
Result in die bestehende Queue gelegt. Pending endet mit dem Queueing, nicht erst mit dem Konsum durch
die Anwendung, sodass danach sofort ein neuer Request beginnen kann.

Die private Invoker-Seam hält Tests headless. Ein geteilter, invalidierbarer Completion-State enthält
nur Queue und Request-Identität; native Blocks halten den State schwach und damit weder Backend,
Service noch Queue fest. Generation- und Exactly-once-Prüfungen verwerfen verspätete und doppelte
Completions; beim Shutdown wird der State vor dem Abbruch des Panels invalidiert. Titel werden strikt
als UTF-8 konvertiert; akzeptierte Ergebnisse verwenden die native Dateisystemrepräsentation der
`NSURL`, ohne Canonicalization, Normalisierung oder nachträgliche Existenzprüfung. Die native
Completion legt ausschließlich `FileDialogResultEvent` ab; Application-Callbacks laufen später über
den normalen Event-Polling-Pfad.

Manueller macOS-Smoke-Test: `native_appkit_form_demo` starten, `File -> Open File` wählen, Sheet-
Zugehörigkeit zum SASD-Fenster, reine Dateiauswahl und Einzelauswahl prüfen, dann Cancel und erneut
öffnen. Eine Unicode-Datei wählen und prüfen, dass die Ausgabe erst nach dem Completion-Event erscheint
und das Fenster nutzbar bleibt. Dieser sichtbare Test wird unter Linux nicht ausgeführt.

#### GTK4-Open-File-Dialog-Consumer (M7 File-Dialog-Slice 3)

`GtkWindowBackend::fileDialogs()` stellt denselben optionalen `FileDialogService` bereit, solange das
vorhandene `GtkWindow` initialisiert ist. Der Adapter verwendet bei GTK-Headern ab 4.10 `GtkFileDialog`,
ansonsten die kompatible asynchrone `response`-API `GtkFileChooserNative`. Damit wird das bestehende,
nicht auf eine GTK4-Minorversion angehobene Minimum erhalten; es gibt keine zweite `GtkApplication`,
Main Loop, Window-Instanz oder EventQueue.

Der Dialog gehört zum vorhandenen `GtkWindow`, erlaubt genau eine existierende Datei und liefert weder
Verzeichnisse noch Mehrfachauswahl. Die Completion wird als `FileDialogResultEvent` über dieselbe
EventQueue gelegt, die `pollEvent()` durch Pumpen des GLib-Default-Kontexts leert. Generation-/Exactly-
once-Prüfungen verwerfen alte und doppelte Callbacks; Shutdown invalidiert den State vor Abbruch und
Freigabe der GTK-Objekte. Native Callbacks führen weder Commands noch Application-Callbacks aus.

Akzeptierte `GFile`-Ergebnisse werden mit `g_file_get_path()` sofort in `std::filesystem::path` kopiert;
nicht-lokale URIs, leere Pfade und technische Fehler werden `failed`. Canonicalization, Normalisierung,
Existenzprüfung, Save, Ordner, Mehrfachauswahl, Filter und eine öffentliche GTK-Dialogklasse bleiben
außen vor. Tests verwenden eine private Invoker-Seam und zeigen niemals einen Chooser. Siehe [ADR
0160](../adr/0160-m7-gtk4-open-file-dialog.md) und den Drei-Consumer-Review in [ADR
0161](../adr/0161-m7-file-dialog-three-consumer-contract-review.md).

Manueller GTK-Smoke-Test: `native_gtk_form_demo` starten, `File -> Open File` wählen, Einzelauswahl
und die Nichtverwendbarkeit eines Verzeichnisses als Ergebnis prüfen, abbrechen und erneut öffnen.
Eine Datei mit Leerzeichen oder Nicht-ASCII-Zeichen auswählen und prüfen, dass die Ausgabe erst nach
dem gequeueten Result-Event erscheint und das Hauptfenster nutzbar bleibt. Dieser sichtbare Test wird
durch Linux/Xvfb-CI nicht als ausgeführt behauptet.

#### M7 Save File über die nativen Consumer (Slice 4)

Der gleiche `FileDialogService` unterstützt nun zusätzlich `SaveFileDialogRequest` über
`requestSaveFile()`. Open und Save teilen Request-IDs, die gemeinsame Grenze von genau einer Pending-
Operation und das gequeuete `FileDialogResultEvent`. Ein akzeptiertes Ergebnis ist ein lokales
Dateisystemziel, das bereits existieren darf oder neu sein kann. Das Toolkit erzeugt, öffnet, truncatet,
schreibt, canonicalisiert oder prüft die Datei nicht; die native Oberfläche übernimmt die
Überschreibbestätigung. Filter und vorgeschlagene Dateinamen bleiben außen vor.

Win32 verwendet am bestehenden `HWND` synchron `IFileSaveDialog` mit `FOS_FORCEFILESYSTEM`,
`FOS_PATHMUSTEXIST`, `FOS_NOCHANGEDIR` und `FOS_OVERWRITEPROMPT`, ausdrücklich ohne
`FOS_FILEMUSTEXIST`. AppKit verwendet ein asynchrones `NSSavePanel`-Sheet am vorhandenen `NSWindow`.
GTK verwendet ab 4.10 `gtk_file_dialog_save()`/`save_finish()` und bei älteren unterstützten GTK4-
Headern den asynchronen `GtkFileChooserNative`-Fallback mit `ACTION_SAVE`. Die drei Adapter kopieren
die native Dateisystemrepräsentation einschließlich Unicode und Leerzeichen in `std::filesystem::path`
und legen die Completion über die bestehende EventQueue ab. Beim manuellen Smoke-Test neue und bereits
existierende Ziele wählen, die native Überschreibbestätigung beobachten, Cancel prüfen und sicherstellen,
dass die Demo keine Datei erzeugt; dieser sichtbare Test wird durch CI nicht als ausgeführt behauptet.

#### M7 Ordnerauswahl über die nativen Consumer (Slice 5)

`FileDialogService::requestSelectFolder(SelectFolderDialogRequest)` wählt genau ein bestehendes
lokales Dateisystemverzeichnis und verwendet `FileDialogResultEvent` weiter. Open, Save und Folder
teilen Request-IDs, die gemeinsame Pending-Grenze, die EventQueue-Grenze sowie Lifecycle-, Generation-
und Exactly-once-Schutz. Die Operation erzeugt kein Verzeichnis, schreibt keinen Pfad und ändert das
Working Directory nicht. Filter, Startverzeichnis und Mehrfachauswahl bleiben zurückgestellt.

Win32 verwendet am bestehenden `HWND` `IFileOpenDialog` mit `FOS_PICKFOLDERS`,
`FOS_FORCEFILESYSTEM`, `FOS_PATHMUSTEXIST` und `FOS_NOCHANGEDIR` und kopiert das UTF-16-
`SIGDN_FILESYSPATH` direkt nach `std::filesystem::path`. AppKit verwendet ein asynchrones
`NSOpenPanel`-Sheet am vorhandenen `NSWindow`, konfiguriert nur für Verzeichnisse und Einzelauswahl.
GTK ab 4.10 verwendet `gtk_file_dialog_select_folder()`/`select_folder_finish()`; ältere
unterstützte GTK4-Versionen verwenden den asynchronen `GtkFileChooserNative`-Fallback mit
`ACTION_SELECT_FOLDER`. GTK akzeptiert nur ein lokales, nichtleeres Ergebnis von `g_file_get_path()`.
Die native Auswahl garantiert die Verzeichnis-Semantik; eine zweite Existenzprüfung oder
Canonicalization findet nicht statt.

Manueller Ordner-Smoke-Test: Eine native Demo starten, `File -> Select Folder` wählen, die Bindung
an das Hauptfenster und die Unmöglichkeit einer Datei als Ergebnis prüfen, ein bestehendes
Verzeichnis auswählen und anschließend Cancel sowie eine Wiederholung mit Leerzeichen/Nicht-ASCII
testen. Prüfen, dass das gequeuete Ergebnis den Verzeichnispfad zeigt, das Fenster nutzbar bleibt,
Open und Save weiter funktionieren und sich das Working Directory nicht ändert. Dieser sichtbare
Smoke-Test wird durch CI nicht als ausgeführt behauptet.

#### M7 Open-File-Extension-Filter (Slice 6)

`OpenFileDialogRequest` kann einen geordneten Vektor literaler UTF-8-Extension-Tokens tragen, etwa
`{"txt", "md", "cpp"}`. Leer bedeutet ungefiltert. Tokens müssen nichtleer und gültiges UTF-8 sein
und dürfen keinen Punkt, Wildcards, Pfadtrenner, NUL oder Semikolon enthalten; `tar.gz` und Glob-
Syntax sind in V1 nicht definiert. Byte-identische Duplikate werden vor der Request-ID abgelehnt.
Benannte Gruppen, MIME-Typen, öffentliche UTType-Werte und Result-Änderungen gibt es nicht.

Win32 projiziert die Menge in genau eine sprachneutrale `COMDLG_FILTERSPEC`, etwa
`*.txt;*.md;*.cpp`; die UTF-16-Backings leben bis nach `SetFileTypes` und `Show`. AppKit verwendet
aus Extensions erzeugte `UTType`-Werte und ab macOS 11 `allowedContentTypes`, mit einer schwach
gelinkten, lokal begrenzten `allowedFileTypes`-Kompatibilität für ältere Laufzeiten. GTK erzeugt
einen `GtkFileFilter` mit `*.Token`-Regeln; ab GTK 4.10 wird er über ein ein-elementiges `GListModel`
gesetzt, ältere unterstützte GTK4-Versionen behalten den `GtkFileChooserNative`-Pfad. Kein
automatischer „All Files“-Filter wird ergänzt.

Der Filter ist Auswahlhilfe, keine Sicherheits- oder Inhaltsvalidierung. Ein nativer akzeptierter
Pfad wird unverändert weitergereicht, auch bei anderer Endung; die Anwendung validiert den Inhalt.
Folder bleibt unverändert. Der sichtbare native Smoke-Test ist dokumentiert, wurde unter WSL aber
nicht ausgeführt.

#### M7 Save-File-Extension-Filter und native Default-Extension-Regel (Slice 7)

`SaveFileDialogRequest` akzeptiert nun denselben geordneten Vektor literaler UTF-8-`extensions` wie
Open File. Leer bleibt ungefiltert. Ein nichtleerer Vektor ist genau ein nativer Filter und keine
Auswahl benannter Alternativen; das erste Token ist die bevorzugte native Extension, sofern die
jeweilige Save-Oberfläche diese Semantik besitzt. Das ist keine portable Result-Postcondition: Der
native akzeptierte Pfad bleibt autoritativ, auch bei einer anderen Endung. Danach wird kein Pfad
umgeschrieben oder abgelehnt, und es gibt weder eine zweite Existenzprüfung noch eine eigene
Überschreibbestätigung.

Win32 setzt einen `COMDLG_FILTERSPEC` über `IFileSaveDialog::SetFileTypes()` und ruft vor `Show()`
`SetDefaultExtension()` mit dem ersten Token auf. Die Backing-Strings bleiben während des nativen
Aufrufs gültig; die Shell behält ihre native Überschreibbestätigung. AppKit projiziert die geordneten
Tokens auf `allowedContentTypes` (oder die bestehende lokal begrenzte `allowedFileTypes`-Kompatibilität);
der erste Content-Type ist die native Präferenz und die Panel-URL wird unverändert zurückgegeben.
Die bestehende `allowsOtherFileTypes`-Semantik wird nicht überschrieben; dieser Slice erfindet keine
neue globale AppKit-Policy. GTK installiert einen `GtkFileFilter` über
`gtk_file_dialog_set_filters()`/`set_default_filter()` ab
GTK 4.10 oder über den bestehenden `GtkFileChooserNative`-Kompatibilitätspfad. GTK besitzt keine
gleichwertige separate Default-Extension-Operation; daher wird keine Extension erfunden oder angehängt.

Der gemeinsame private Validator hält den Open-/Save-Token-Vertrag identisch. Result-Events,
Capabilities, IDs, Pending-/Lifecycle-Schutz und Überschreibsemantik bleiben unverändert. Der Filter
ist Auswahl-/Benennungshilfe, keine Sicherheitsgrenze oder Inhaltsvalidierung. Vorgeschlagener
Dateiname, Startverzeichnis, Mehrfachauswahl, MIME-/UTType-APIs und benannte Filtergruppen bleiben
zurückgestellt.

Manueller nativer Smoke-Test: Jede native Demo starten, Save File öffnen, den nativen Filter prüfen,
einen Namen ohne Endung, mit passender und mit anderer Endung testen sowie ein vorhandenes Ziel und
die native Überschreiboberfläche prüfen. Cancel ausführen, Open und Folder wiederholen und das
Ergebnis exakt so ausgeben, wie der native Picker es liefert. Plattformunterschiede beim automatischen
Anhängen einer Endung sind innerhalb dieses Vertrags erwartbar und kein Fehler. Dieser sichtbare Test
wurde unter WSL nicht ausgeführt.

#### Nativer AppKit-Menü-Consumer (M7 Slice 2)

`AppKitWindowBackend::syncMenuBar()` baut aus demselben Core-`MenuBarModel` einen vollständigen,
besitzenden `NSMenu`-Baum und installiert ihn als prozessweites
`NSApplication.sharedApplication.mainMenu`. Es gibt genau einen SASD-AppKit-Menübesitzer; eine
konkurrierende Backend-Instanz schlägt geschlossen fehl. Ein bereits vorhandenes Prozessmenü wird
als Restore-Ziel gespeichert und transactional ersetzt. `clearMenuBar()` stellt es nur wieder her,
solange das SASD-Menü noch aktuell ist; das vorherige Menü bleibt beim ursprünglichen
Prozessbesitzer.

AppKit-Labels sind striktes UTF-8 einschließlich CJK und Supplementary Unicode; fehlerhafte Eingaben
brechen den vollständigen Austausch vor einer Änderung des alten Menüs ab. Strukturänderungen werden
explizit resynchronisiert. `menuNeedsUpdate`/`validateMenuItem:` aktualisieren kopierte Command-Texte
und den Enabled-State. Native Actions kopieren nur `Command::Reference` und legen ein
`CommandActivationEvent` in die Queue; im Objective-C-Action-Callback läuft kein Command-Callback.
`keyEquivalent` bleibt leer, weil die ShortcutMap autoritativ ist; AppKit-Mnemonics werden nicht
nachgebildet. Eine allgemeine NativeMenu-Abstraktion wird noch nicht eingeführt. GTK bleibt der
nächste Validierungs-Consumer.

Langfristiges Ziel: geeignete Widgets nur dann auf Win32-/Windows-Controls und Plattformdienste
abbilden, wenn ein realer Plattformvorteil einen eigenen Peer-Vertrag rechtfertigt.

Schwerpunkte:

- native Fenster und Controls
- Windows-Event-/Message-Integration
- Clipboard
- Accessibility
- High-DPI
- IME/Textinput
- Drag & Drop

### 4. Native Linux-Backend

GTK4 ist nun ein optionaler zweiter Native Host. `SASD::UI::Native::GTK` bleibt build-tree-only und
verwendet `GtkWindowBackend`, Cairo als `RenderDevice` und Pango als gemeinsame Mess-/Zeichenpolicy.
GTK wird nur bei `SASD_UI_BUILD_GTK_ADAPTER=ON` über pkg-config gefunden; normale Builds bleiben
GTK-frei. GLib wird in `pollEvent()` gepumpt und vollständige DisplayLists werden für Repaints
behalten. Druckbarer Commit-Text wird unterstützt, IME-Preedit bleibt offen.

Wichtig ist, dass GTK nicht Teil der öffentlichen Core-API wird.

#### Nativer GTK4-Menü-Consumer (M7 Slice 3)

`GtkWindowBackend::syncMenuBar()` projiziert dasselbe Core-`MenuBarModel` in einen privaten,
fensterlokalen `GMenu`-/`GMenuModel`-Baum, eine `GSimpleActionGroup` und eine
`GtkPopoverMenuBar`. Der private GTK-Root ist eine vertikale `GtkBox`; die bestehende
`GtkDrawingArea` bleibt darunter die Rendered-Fläche. Daher beziehen sich `windowSize()`, Resize-Events
und Pointer-Koordinaten weiterhin auf die tatsächliche DrawingArea-Allokation. Menüinstallation und
Entfernung erzeugen weder DrawingArea noch `DisplayList` neu.

Die Projektion besitzt ihren strukturellen Snapshot: Modelländerungen werden erst nach explizitem
Resync sichtbar; live `Command::text()` und `Command::isEnabled()` werden aus `pollEvent()` aktualisiert.
GTK Actions verwenden intern aus numerischen IDs abgeleitete Namen und halten nur
`Command::Reference`. Die Activation validiert die Reference und legt ein `CommandActivationEvent` in
die Queue; `Command::execute()` läuft nicht im GLib-Callback. Abgelaufene, deaktivierte und fehlerhafte
dynamische Texte schlagen geschlossen fehl. GTK-Labels sind striktes UTF-8 und erhalten literales `&`
und `_`; Win32-Mnemonics werden nicht nachgebildet. Shortcut-Metadaten werden nicht als GTK-
Accelerators registriert; `ShortcutMap` bleibt autoritativ. Siehe [ADR 0156](../adr/0156-m7-gtk4-native-menu-consumer.md)
und [ADR 0157](../adr/0157-native-menu-portability-boundary.md).

### 5. Native macOS-Backend

AppKit ist der optionale dritte M6-Native-Host und wird auf macOS bei
`SASD_UI_BUILD_APPKIT_ADAPTER=ON` als `SASD::UI::Native::AppKit` im Build-Tree verfügbar. Der
`AppKitWindowBackend` implementiert direkt `Backend`, `RenderDevice` und `RenderedMeasurementContext`.
Eine private geflippte `NSView` spielt die vollständige besitzende `DisplayList` über
CoreGraphics/CoreText ab; es werden keine NSButton- oder NSTextField-Peers erzeugt. AppKit-View-Points
sind dieselben logischen Einheiten wie Rendered; ein Backing-Scale-Wechsel fordert Repaint an, verdoppelt
aber niemals Layout- oder Pointer-Koordinaten.

Der Adapter besitzt eigenes `NSWindow`, View, Delegate, Queue und den letzten Frame, verwendet aber
`NSApplication` als prozessweiten Service und beendet ihn nicht. Lifecycle und Callbacks sind an den
Main Thread gebunden. `pollEvent()` pumpt begrenzt und nichtblockierend AppKit-Events.
`NSTextInputClient` liefert committed UTF-8 getrennt von Core-KeyEvents; markierter/preedit IME bleibt
zurückgestellt. Für Caret-Advances gibt es eine explizite UTF-8-Scalar-zu-UTF-16-Abbildung. Das Target
bleibt build-tree-only, bis ein installierter Framework-Consumer validiert ist. Siehe
[ADR 0153](../adr/0153-m6-appkit-native-rendered-host.md) und
`tools/m6_smoke_appkit_macos.sh`; der sichtbare Test bleibt das offene M3-macOS-/Cocoa-Gate.

## Auswahlstrategie

Backends sollen zur Build-Zeit und, soweit sinnvoll, zur Laufzeit auswählbar sein.

Beispielhafte Zielvorstellung:

```text
sasd-ui-core
├── sasd-ui-terminal
├── sasd-ui-sdl
├── sasd-ui-win32
├── sasd-ui-gtk
└── sasd-ui-appkit
```

Eine Anwendung soll nur die Backends linken müssen, die sie tatsächlich verwendet.

## Fallback-Modell

Nicht jedes SASD-Widget benötigt auf jeder Plattform ein natives Gegenstück.

Die Priorität lautet:

1. natives Control, wenn es semantisch passt und einen echten Plattformvorteil bietet;
2. eigener gerenderter Peer, wenn kein passendes natives Control existiert;
3. explizit gemeldete Nicht-Unterstützung, wenn eine Funktion im Backend technisch oder sinnvoll nicht angeboten werden kann.

Stilles Weglassen kritischer Funktionalität ist kein Ziel.

## Capability Discovery

Backends sollen Fähigkeiten explizit melden. Dazu gehören unter anderem:

- Pointer-/Mausunterstützung
- Clipboard
- Drag & Drop
- mehrere Top-Level-Fenster
- native Menüs
- System-Tray
- Farbtiefe
- Accessibility
- Touch/Pen
- IME
- Dateidialoge

Anwendungscode kann dann entweder eine portable Teilmenge verwenden oder gezielt optionale Plattformfunktionen aktivieren.

## Portabilitätsstufen

Für Dokumentation und Tests sind drei Stufen sinnvoll:

### Portable Core

Funktionen, die jedes unterstützte Backend anbieten muss.

### Portable Extended

Funktionen, die auf den Haupt-Desktop-Backends verfügbar sein sollen, aber in eingeschränkten Terminals fehlen dürfen.

### Backend Extension

Bewusst plattformspezifische Erweiterungen. Sie müssen klar gekennzeichnet und außerhalb des portablen API-Kerns gehalten werden.

## Warum nicht Qt oder wxWidgets als Pflichtbasis?

Beide Projekte sind wertvolle Architekturvorbilder. Das SASD UI Toolkit soll jedoch eine eigenständige Abstraktion bleiben. Eine zwingende Abhängigkeit von einem vollständigen GUI-Framework würde:

- die Kontrolle über Backend-Design und Terminal-Integration reduzieren;
- die Größe und Abhängigkeiten des Projekts erhöhen;
- den Nutzen einer eigenen öffentlichen API teilweise auf einen Wrapper reduzieren.

Das schließt optionale Integration oder das Lernen aus deren Implementierungen nicht aus.

## Teststrategie pro Backend

Jedes Backend soll denselben Satz semantischer Contract-Tests erfüllen, soweit seine Capability-Stufe dies verlangt.

Beispiele:

- Button-Aktivierung erzeugt genau ein Activation-Event
- Fokusreihenfolge folgt derselben Semantik
- `VBox` hält Mindestgrößen ein
- disabled Widgets lösen keine Benutzeraktion aus
- Textinput und Key Events werden getrennt transportiert

Zusätzlich erhält jedes Backend plattformspezifische Integrations- und Smoke-Tests.

Für das aktuelle Terminal-Backend existieren nun echte Prozess-Smoke-Tests statt reiner Mock-/Compilerabdeckung: Linux und macOS starten den nativen POSIX-Adapter in einem Kernel-PTY, Windows startet den nativen Adapter in einer ConPTY-Pseudokonsole. Geprüft werden dabei unter anderem Größenabfrage, Raw-/VT-Input, Frame-Output, Alternate-Screen-/Cursor-Sequenzen sowie die Wiederherstellung von `termios` beziehungsweise Windows Console Modes und Codepages. Diese automatisierten Pseudoterminaltests ergänzen, ersetzen aber nicht den manuellen Emulator-Smoke-Test unter Linux/xterm und Windows Terminal.
