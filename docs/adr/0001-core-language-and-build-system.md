# ADR 0001 – C++20 core and CMake build system

**Status:** Accepted  
**Date:** 2026-09-17

## English

### Context

SASD UI Toolkit is intended to be usable on Windows, Linux and macOS and eventually across graphical and terminal environments. The public toolkit must not require users to adopt a vendor-specific language runtime or a single IDE.

### Decision

- The core implementation and public API use **modern C++**, initially targeting **C++20**.
- **CMake** is the primary build-system generator.
- The core must build with the major desktop C++ toolchains: MSVC, GCC and Clang.
- Platform and backend dependencies are isolated behind backend modules; the core must not require Qt, wxWidgets, GTK, SDL or another GUI framework.
- C++20 is a starting baseline, not permission to use complexity gratuitously. Prefer simple standard-library facilities and explicit ownership.
- No stable binary ABI is promised during the pre-1.0 phase. Source compatibility is more important than freezing a premature ABI.

### Rationale

C++ provides native access to operating-system APIs, deterministic lifetime management, mature toolchains and broad portability without requiring a managed runtime. CMake gives the project one build description that can generate builds for the major target platforms and CI environments.

### Consequences

- Public headers must remain portable and avoid leaking backend-specific types.
- New dependencies require architectural justification.
- Compiler-specific extensions should be avoided in the public API.
- CI should exercise multiple compilers and operating systems as early as practical.

## Deutsch

### Kontext

Das SASD UI Toolkit soll unter Windows, Linux und macOS sowie langfristig in grafischen und terminalbasierten Umgebungen einsetzbar sein. Anwender sollen weder an eine herstellerspezifische Sprachlaufzeit noch an eine einzelne IDE gebunden werden.

### Entscheidung

- Kernimplementierung und öffentliche API verwenden **modernes C++**, zunächst mit **C++20** als Mindeststandard.
- **CMake** ist das primäre Build-System.
- Der Core soll mit MSVC, GCC und Clang gebaut werden können.
- Plattform- und Backend-Abhängigkeiten werden in Backend-Modulen gekapselt; der Core darf Qt, wxWidgets, GTK, SDL oder ein anderes GUI-Framework nicht voraussetzen.
- C++20 ist eine technische Basis, kein Selbstzweck. Einfache Standardbibliothekslösungen und klares Ownership sind komplexen Konstruktionen vorzuziehen.
- Vor Version 1.0 wird keine stabile Binär-ABI zugesagt. Saubere Quellcode-APIs sind wichtiger als eine zu früh eingefrorene ABI.

### Konsequenzen

Öffentliche Header müssen portabel bleiben, Backend-Typen dürfen nicht in die öffentliche API durchsickern und neue Abhängigkeiten benötigen eine nachvollziehbare Begründung.