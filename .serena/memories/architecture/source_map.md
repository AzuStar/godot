# Architecture Source Map

Top-level dirs:
- `core/`: base runtime types, ObjectDB/ClassDB, Variant, math, strings, IO/resources, OS abstractions, templates, extension interfaces. Builds first via `core/SCsub` and emits version/hash/license/certs/disabled-class generated headers.
- `servers/`: engine service APIs and backends: audio, rendering, text, display, physics/navigation 2D/3D, XR, movie writer. Builds before scene.
- `scene/`: node tree, GUI controls, 2D/3D nodes, resources, animation/audio/theme/debugger. `scene/SCsub` chains `main`, `gui`, optional `3d`, `2d`, `animation`, `audio`, `resources`, `debugger`, `theme`.
- `editor/`: editor-only UI, plugins, inspectors, import/export, debugger, project manager, settings. Included only for editor builds.
- `drivers/`: low-level platform/service drivers and rendering/audio/input backend pieces.
- `platform/`: platform glue. Root `platform/SCsub` registers platform-exclusive APIs and export icons; selected platform SCsub builds last.
- `modules/`: optional built-in/custom modules. Each detected module is its own library plus final `modules` registration library.
- `main/`: executable entry, startup, timer sync, performance, generated splash/app icon headers; linked near end.
- `tests/`: doctest-based C++ tests; compiled only when `tests=yes`.
- `doc/classes/`: class reference XML synced with script-exposed API.
- `misc/scripts/`: formatting, validation, generation helper scripts.

Build/link order in root `SConstruct`:
`core -> servers -> scene -> editor? -> drivers -> platform -> modules -> tests? -> main -> platform/<selected>`.

Resource/UI example:
- `StyleBox` lives in `scene/resources/style_box.h/.cpp` and docs in `doc/classes/StyleBox.xml`; concrete styleboxes are sibling resources (`style_box_flat`, `style_box_texture`, `style_box_line`).