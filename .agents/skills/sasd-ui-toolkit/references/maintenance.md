# Version drift, maintenance and acceptance criteria

The skill is intentionally **small and version-aware**. It must not become another copy of the full human manual or of `AGENTS.md`.

## Source of truth

At every invocation, inspect the **actual selected** tag/commit and public API. The text here gives a search/navigation method; it does not guarantee that a method added on `main` is present in an earlier 0.x release.

Status taxonomy:
- **Implemented:** header, implementation and behavior tests exist in the targeted version.
- **Experimental:** code exists, but availability, packaging, platform support or interactions remain limited.
- **Planned:** roadmap only; do not produce calling code.
- **Unknown:** verify before use or report the missing evidence.

## Update triggers

A new milestone by itself **does not** require rewriting the skill. Update only when a real consumer contract changes:

1. Exported CMake target, package version requirement or compiler baseline changes.
2. Public ownership, callback, Model/View, event or host entry-point semantics change.
3. A backend moves from planned to experimental or from experimental to supported.
4. The reference terminal demo moves or its entry-point/lifecycle changes.
5. A compile-tested consumer example fails or becomes misleading.

For these changes, update the smallest matching reference and example. Keep `SKILL.md` stable unless the overall workflow changes. **Never blindly duplicate every class from public headers.**

## Automated and human validation

The repository CI validates the skill manifest/path references and builds/runs the headless sample **against the freshly installed current toolkit package** on multiple supported compiler/platform configurations.

- Validate metadata: `python .agents/skills/sasd-ui-toolkit/scripts/validate.py`
- Compile/test the independent example using [quickstart](quickstart.md)
- If the change affects terminal event routing, run terminal tests and inspect the interactive demo manually on a capable terminal.
- For experimental SDL3/native paths, use platform-specific CI/tests and accurately report untested combinations.

A green example verifies only the exercised **Core consumer contract**; it does not certify every API or renderer. Runtime introspection/feature discovery remains a separate future enhancement.

## Using the skill from a different project

The canonical location is `.agents/skills/sasd-ui-toolkit/`, recognized by agent clients supporting repository skills. In an application repository, install/copy that directory under the consumer repository's supported skills location; keep the copy synchronized with the version of the toolkit you use. Some clients require a new session or skill reload to discover new files. If no automatic skill discovery exists, explicitly provide the path to `SKILL.md`.

Do not assume the consumer app's repository includes the toolkit source tree. Locate its installed headers or vendored checkout before dereferencing paths mentioned in this skill.
