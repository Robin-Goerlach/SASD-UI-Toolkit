#!/usr/bin/env bash
set -euo pipefail

if [[ "$(uname -s)" != "Darwin" ]]; then
    echo "This smoke test must be run on macOS; no GUI automation is attempted." >&2
    exit 2
fi

build_dir="${1:-build-m6-appkit}"
cmake -S . -B "$build_dir" \
    -DCMAKE_BUILD_TYPE=Debug \
    -DSASD_UI_BUILD_TESTS=ON \
    -DSASD_UI_BUILD_EXAMPLES=ON \
    -DSASD_UI_WARNINGS_AS_ERRORS=ON \
    -DSASD_UI_BUILD_APPKIT_ADAPTER=ON
cmake --build "$build_dir" --target sasd_ui_appkit_demo sasd_ui_appkit_tests --parallel
ctest --test-dir "$build_dir" -R sasd_ui_appkit_tests --output-on-failure

echo "Starting the visible demo. Manually verify the AppKit window, Label, Button and TextField."
echo "Resize the window, click the Button, type committed text, move the pointer, and close it."
echo "On Retina hardware also verify that logical geometry does not double when scale changes."
"$build_dir/examples/sasd_ui_appkit_demo"
