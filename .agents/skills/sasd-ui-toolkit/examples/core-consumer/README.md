# Core consumer smoke example

A minimal, runnable **headless C++20 application** linked against an **installed** SASD UI Toolkit package.

It tests real public calls for component ownership, Command -> Button binding, selection and virtualized TableView snapshots. It deliberately opens **no terminal and no desktop window**; it is not a substitute for `examples/terminal_form_demo.cpp`.

Build instructions: [consumer quickstart](../../references/quickstart.md). From the toolkit checkout:

~~~sh
cmake -S .agents/skills/sasd-ui-toolkit/examples/core-consumer \
  -B build-sasd/skill-consumer \
  -DCMAKE_PREFIX_PATH="$(pwd)/build-sasd/install"
cmake --build build-sasd/skill-consumer --parallel
ctest --test-dir build-sasd/skill-consumer --output-on-failure
~~~

This example participates in the main repository CI. It intentionally requests the 0.2.x CMake package feature line; revise this version when a new incompatible feature line is made the consumer baseline.
