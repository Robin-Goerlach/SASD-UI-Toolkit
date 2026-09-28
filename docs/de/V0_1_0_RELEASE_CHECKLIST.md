# v0.1.0-Release-Checkliste

Stand: 28.09.2026

Diese Checkliste trennt den **technisch releasefähigen Stand** von der späteren Veröffentlichung.
Ein Git-Tag oder GitHub-Release wird erst nach ausdrücklicher Freigabe erstellt.

## Bereits erfüllt

- [x] M2 Terminal Preview funktional abgeschlossen.
- [x] Linux/xterm-artiger manueller Smoke-Test bestanden.
- [x] Windows-Terminal-Smoke-Test bestanden, einschließlich `Robin AΩ界`.
- [x] TTY-/Codepage-Restoration für F10, Escape und Exit-Button bestanden.
- [x] Linux GCC, Linux Clang, macOS AppleClang und Windows MSVC in CI.
- [x] Linux Clang ASan + UBSan in CI.
- [x] CMake-Projektversion ist `0.1.0`.
- [x] Installierbares CMake-Paket vorhanden.
- [x] Externer Consumer kann `find_package(SASDUIToolkit 0.1 CONFIG REQUIRED)` verwenden.
- [x] Installierte Targets heißen wie im Build-Tree: `SASD::UI` und `SASD::UI::Terminal`.
- [x] Changelog für v0.1.0 angelegt.

## Vor Veröffentlichung noch prüfen

- [x] Finaler Release-Commit `dc34ebd4bf6db4a0f9777fc81320f47dae72b36b`; vollständige CI dieses SHAs grün.
- [x] Release-Build in CI aus einem frischen Checkout auf Linux, macOS und Windows erzeugt und getestet.
- [x] Installationsinhalt automatisiert geprüft: öffentliche Header, Core-/Terminal-Libraries, CMake-Config sowie Lizenz/README/Changelog.
- [x] Release Notes aus `CHANGELOG.md` zweisprachig final redigiert.
- [x] Repository-Hygiene in der Release-CI geprüft; keine generierten Build-/Linker-Artefakte versioniert.
- [x] Ausdrückliche Freigabe zur Veröffentlichung erteilt; CI-gekoppelter Publisher vorbereitet.
- [x] Annotierter Tag `v0.1.0` zeigt exakt auf `dc34ebd4bf6db4a0f9777fc81320f47dae72b36b`.
- [x] GitHub Release `SASD UI Toolkit v0.1.0` veröffentlicht.

## Release-Grenze

v0.1.0 ist ein **Pre-1.0-Release**. Es dokumentiert den ersten nutzbaren Terminal-Vertikalschnitt,
friert aber die langfristige Source-Kompatibilität noch nicht ein. Größere API-Korrekturen bleiben
vor 1.0 möglich, müssen nachvollziehbar dokumentiert werden.
