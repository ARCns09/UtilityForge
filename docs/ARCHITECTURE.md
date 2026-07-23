# UtilityForge Architecture

## Architectural position

UtilityForge is one C++20 executable built with CMake and Qt 6 Widgets. The GUI process owns a small application shell, shared services, and the currently implemented feature module. There is no daemon, browser runtime, QML engine, or periodic background worker.

Version 0.1 contains DropZone only. ColorDrop, Folder Preview, and GitSnap are future modules, but their source directories and registrations must not be created until their implementation phase begins. The architecture supplies clean seams without speculative placeholder code.

## Recommended repository structure

This is the structure to create during implementation, not during the current planning phase:

```text
UtilityForge/
├── CMakeLists.txt
├── cmake/
│   └── UtilityForgeOptions.cmake
├── docs/                         # Approved planning documents
├── packaging/
│   └── linux/                    # desktop file, AppStream metadata, icons
├── resources/
│   ├── utilityforge.qrc
│   └── icons/                    # original or properly licensed assets only
├── src/
│   ├── app/
│   │   ├── main.cpp
│   │   ├── ApplicationBootstrap.{h,cpp}
│   │   ├── MainWindow.{h,cpp}
│   │   └── SettingsDialog.{h,cpp}
│   ├── core/
│   │   ├── errors/
│   │   │   ├── AppError.{h,cpp}
│   │   │   └── ErrorFormatter.{h,cpp}
│   │   ├── notifications/
│   │   │   └── NotificationCenter.{h,cpp}
│   │   ├── process/
│   │   │   ├── ExternalToolRegistry.{h,cpp}
│   │   │   ├── ProcessRequest.h
│   │   │   └── ProcessRunner.{h,cpp}
│   │   ├── settings/
│   │   │   ├── AppPaths.{h,cpp}
│   │   │   └── SettingsService.{h,cpp}
│   │   └── tasks/
│   │       ├── CancellationToken.{h,cpp}
│   │       ├── TaskManager.{h,cpp}
│   │       ├── TaskRecord.h
│   │       └── TaskTypes.h
│   └── modules/
│       └── dropzone/
│           ├── DropZonePage.{h,cpp}
│           ├── DropQueueModel.{h,cpp}
│           ├── DropZoneController.{h,cpp}
│           ├── DropTypes.h
│           ├── planning/
│           │   ├── OperationPlanner.{h,cpp}
│           │   └── OutputPlanner.{h,cpp}
│           ├── services/
│           │   ├── ClipboardService.{h,cpp}
│           │   ├── ImageConverter.{h,cpp}
│           │   ├── MediaInspector.{h,cpp}
│           │   ├── QrEncoder.{h,cpp}
│           │   └── ToolCommandBuilder.{h,cpp}
│           └── tasks/
│               ├── ArchiveTask.{h,cpp}
│               ├── ImageConvertTask.{h,cpp}
│               ├── QrGenerateTask.{h,cpp}
│               └── VideoConvertTask.{h,cpp}
├── tests/
│   ├── CMakeLists.txt
│   ├── core/
│   ├── dropzone/
│   ├── fixtures/
│   └── fake-tools/
└── LICENSE
```

Headers are kept near their implementation rather than exposed through a premature public SDK. Directories for later modules are deliberately absent from this tree. When a future module is implemented, it receives its own directory under `src/modules` and depends inward on stable core contracts.

## Build boundaries

CMake should define small internal static/object library targets and one executable:

- `utilityforge_core`: task, process, settings, error, and notification primitives; no dependency on feature UI.
- `utilityforge_dropzone`: DropZone model, controller, page, planners, services, and tasks; depends on core.
- `utilityforge_app`: bootstrap and main window; composes core and DropZone.
- matching test targets when `BUILD_TESTING=ON`.

The dependency direction is `app -> module -> core`. Core must not include module headers, and DropZone logic must not reach through `MainWindow`. Widgets stay in the app/module presentation layers; command building, validation, and operation planning remain testable without showing windows.

Future module interfaces should be extracted only after a second real module proves the common shape. Version 0.1 does not need a generic plugin system, dynamic loading, or ABI boundary.

## Principal classes and responsibilities

### Application shell

`ApplicationBootstrap` sets organization/application identity, initializes `AppPaths`, settings, and shared services, then constructs the window. It performs no slow tool probing on the startup path.

`MainWindow` owns the compact sidebar, toolbar host, central page stack, and status/task surface. It binds window-level shortcuts and handles close-with-active-tasks. It does not implement conversion logic.

`SettingsDialog` exposes only approved typed preferences and tool diagnostics/refresh. It reads and writes through `SettingsService`; it does not probe tools synchronously or own feature logic.

### Shared core

`AppPaths` is the only authority for configuration, cache, data, state/log, and temporary locations. It uses XDG-aware Qt paths and validates directory creation.

`SettingsService` wraps `QSettings` with typed keys, defaults, migration/version handling, and explicit sync-error reporting. Feature code does not scatter string keys.

`TaskManager` owns active `TaskRecord`s, assigns IDs, enforces concurrency policy, aggregates progress, routes completion, and exposes cancel requests. It also supplies the status bar and shutdown dialog with one consistent view of work.

`CancellationSource`/`CancellationToken` provide cooperative, thread-safe cancellation. A source is owned by the task manager/task; workers receive tokens. Cancellation is idempotent.

`ExternalToolRegistry` lazily locates and probes allowlisted executables, records feature capabilities, and invalidates cached state only when explicitly refreshed or when settings change.

`ProcessRunner` executes a validated `ProcessRequest` asynchronously with `QProcess`, bounded logs, timeouts, structured progress parsing hooks, and terminate/kill cancellation. It is the only general external-process entry point.

`AppError` is a value type containing stable category/code, severity, a safe user message, optional technical detail, task ID, and recoverability/actions. `ErrorFormatter` maps internal failures and process exits to user-facing text.

`NotificationCenter` publishes in-window transient notices, persistent task failures, and details actions. It does not require a tray icon or desktop notification daemon in 0.1.

### DropZone

`DropZonePage` is the widget composition: drop target/empty state, queue table, operation controls, toolbar actions, details/log panel, and task buttons. It emits user intent and renders model/controller state.

`DropQueueModel` is a `QAbstractTableModel` holding display-safe rows keyed by stable item IDs. It stores input metadata, planned operation, state, progress, result reference, and error reference. It does not execute work.

`DropZoneController` validates dropped/local URLs, starts asynchronous inspection/planning, updates the model, submits tasks, handles cancellation, and publishes results. It mediates between widgets and services.

`OperationPlanner` maps inspected inputs plus selected options to an immutable operation plan. It returns either a valid plan, a set of warnings requiring confirmation, or a structured error.

`OutputPlanner` creates safe destination names, collision-free staging paths, and publish plans. Only it may decide task output paths.

`ImageConverter` provides Qt image preflight and conversion helpers. Heavy decode/encode runs in `ImageConvertTask`, never synchronously from the page.

`MediaInspector` invokes FFprobe through `ProcessRunner`, parses bounded JSON, and returns typed stream/container facts.

`ToolCommandBuilder` converts validated archive/video plans to immutable program/argument requests. It contains no UI and never accepts a raw command string.

`QrEncoder` isolates the approved QR library and returns an in-memory monochrome module matrix or image-ready result; rendering/saving occurs in the QR task.

`ClipboardService` performs explicit GUI-thread clipboard operations for text, local file URLs, and images after the user requests them.

The four task classes implement the operation lifecycle: preflight, run, progress, staged validation, publish, cleanup, and result. They share task primitives without sharing format-specific logic.

## Data and control flow

1. Qt delivers a drag/drop or native-picker selection to `DropZonePage`.
2. The page emits local URLs to `DropZoneController`; it does not enumerate or decode them.
3. The controller performs quick syntactic checks, creates pending model rows, and schedules any potentially slow inspection.
4. `OperationPlanner` produces an immutable plan and warnings. The UI confirms lossy or stream-dropping choices.
5. The controller submits a task to `TaskManager`.
6. The task performs in-process worker work or delegates external execution to `ProcessRunner`.
7. Progress and state changes return through queued Qt signals, and the model updates on the GUI thread.
8. On success, the staged output is validated and published. The result becomes available to open/copy/reveal.
9. On failure or cancellation, only recorded task-owned staging files are cleaned, and a structured notification is raised.

All cross-thread values are immutable snapshots or safely synchronized primitives. QObjects with GUI affinity stay on the GUI thread.

## Keeping the Qt main thread responsive

Work is divided by behavior:

- **CPU-heavy or blocking library work**—image decode/encode, QR generation, filesystem checks that may stall—is submitted to a bounded `QThreadPool`. Worker lambdas/tasks receive value inputs and a cancellation token. Results return via queued signals or queued method invocation.
- **External tools** run through asynchronous `QProcess`. Creating and observing a process can remain on the Qt event-loop thread because the OS process performs the work and pipes are consumed through signals. No `waitFor...` APIs are allowed on the GUI thread.
- **Filesystem enumeration** is never done from paint, model-data, drop, or selection handlers. Even metadata access moves to a worker when it can involve many entries, network mounts, FUSE, or slow media.
- **GUI-only work**—model mutation, clipboard access, dialogs, widgets, and notifications—runs on the GUI thread in short event-handler slices.

`TaskManager` uses a conservative pool limit derived from `QThread::idealThreadCount`, with an upper cap, and separately limits heavyweight external tasks to avoid saturating disk/CPU. Queueing replaces unbounded thread creation. There are no periodic progress polls; workers/processes emit changes, and UI progress updates may be coalesced to avoid event flooding.

## Task state and cancellation

A task follows this state machine:

```text
Queued -> Preparing -> Running -> Publishing -> Succeeded
   |          |           |           |
   +----------+-----------+-----------+-> Cancelling -> Cancelled
                          +---------------------------> Failed
```

Cancellation is a request, not an assumption that work stopped immediately:

- In-process loops check an atomic token at defined checkpoints and before publish.
- `ProcessRunner` sends terminate once, starts a non-blocking grace timer, then kills the exact child if still running.
- A cancelled task can never transition to success, even if completion races with the request; the task state transition is serialized by `TaskManager`.
- The publish step checks cancellation again and is designed to be a short atomic rename/commit.
- A task owns an explicit list of staging artifacts, allowing narrow cleanup.
- Application shutdown requests cancellation, waits through the event loop while showing state, and offers a force-exit option only after explaining possible staging leftovers.

Progress is either indeterminate or normalized with a textual phase. FFmpeg progress is derived from machine-readable timestamps versus probed duration. Archive progress may remain indeterminate unless the selected tool exposes reliable structured progress; fabricated percentages are forbidden.

## Error and notification handling

Errors cross boundaries as `AppError`, not already-formatted dialog strings. Categories include validation, unsupported input, missing capability, filesystem, process start, process failure, cancellation cleanup, and internal error.

Expected user-correctable errors appear inline on the affected queue row and through a concise in-window notification. Details show the operation, safe arguments summary, tool/version, exit code, and a bounded stderr excerpt. Repeated errors are grouped by task rather than spawning dialog storms.

Warnings that change output semantics—lossy image conversion, alpha removal, metadata loss, re-encoding, or excluding unsupported media streams—require an explicit confirmation before task submission. For 0.1 MKV operations, the planner includes every compatible audio stream and explicitly lists subtitles, attachments, data, and incompatible audio streams that will be excluded. Inputs without exactly one video stream are rejected. Success uses the status surface rather than modal dialogs. Programmer invariants are logged and tested; the release UI reports an internal error without crashing where recovery is safe.

## QProcess safety contract

Only `ProcessRunner` starts supported external tools. Its contract requires:

- an executable ID resolved by `ExternalToolRegistry`, never an arbitrary shell string;
- an absolute executable path and a separately constructed `QStringList`;
- validated, absolute input and staging paths;
- an explicit working directory and process-channel policy;
- maximum captured-output sizes, probe/start timeout, and cancellation policy;
- an expected output contract and a parser selected by code, not user input.

No command is passed to `/bin/sh`, `bash`, `sh -c`, or an equivalent. User-editable argument templates are excluded. Environment variables capable of tool/plugin injection are reviewed and removed where doing so does not break normal codecs. Process output is untrusted plain data. Argument arrays can be displayed for diagnostics with unambiguous quoting, but the displayed representation is never executed.

FFprobe uses JSON. FFmpeg uses `-progress pipe:1` and non-interactive operation. Tools are prohibited from reading stdin. Cancellation targets only the child process started for the task; process-group handling must be tested before relying on it.

## Settings and XDG storage

The application identity is fixed before any path/settings lookup. Proposed lowercase storage ID: `utilityforge`.

`AppPaths` uses `QStandardPaths` and honors XDG environment overrides. Proposed logical locations are:

| Data | Default Linux location | Contents |
| --- | --- | --- |
| Configuration | `$XDG_CONFIG_HOME/utilityforge/` or `~/.config/utilityforge/` | `settings.ini` through `QSettings`, settings schema version |
| Persistent data | `$XDG_DATA_HOME/utilityforge/` or `~/.local/share/utilityforge/` | future user-owned history/favorites; normally empty in 0.1 |
| Cache | `$XDG_CACHE_HOME/utilityforge/` or `~/.cache/utilityforge/` | reproducible capability probe cache and disposable thumbnails if later needed |
| State/logs | `$XDG_STATE_HOME/utilityforge/` or `~/.local/state/utilityforge/` | bounded diagnostic logs and last-session nonessential state |
| Runtime temporary | system/Qt temporary location | per-task temporary data when same-directory staging is unavailable |

Because older supported Qt versions do not uniformly expose every XDG state helper, `AppPaths` may resolve `XDG_STATE_HOME` explicitly with a standards-compliant fallback. All other locations prefer `QStandardPaths`. Directory permissions rely on the user's umask and are checked on creation.

Version 0.1 settings include window geometry/state, last chosen output directory or “beside source” preference, image quality defaults, archive format default, and bounded concurrency. Recent inputs, clipboard values, task history, and secrets are not persisted. Settings migrations are explicit and forward-only. A failed `QSettings::sync()` is reported without crashing.

Desktop integration files installed by CMake follow XDG locations under the selected install prefix. Runtime code never writes into system application directories.

## External tool detection

Tool detection is lazy so startup is not tied to subprocesses. When the user opens or selects an operation requiring a tool:

1. Look for the executable in an approved configured path (if that capability supports configuration), then with `QStandardPaths::findExecutable`.
2. Resolve and store its absolute path.
3. Run a short asynchronous identity/capability probe.
4. Parse bounded structured output or conservative version output.
5. Cache the result for the session and expose `Available`, `Unavailable`, `Unsupported`, or `Checking`.

FFmpeg and FFprobe are checked as a compatible pair. Required encoders and muxers are probed, not inferred from a package name. `bsdtar` is probed for both ZIP and gzip-compressed tar creation. A Settings diagnostics page offers manual refresh; there is no background polling.

## Lazy loading

Version 0.1 constructs the DropZone page because it is the only feature module. Expensive services—tool probes, codecs, file inspection, and logs—initialize on first use. Future modules should be instantiated when selected, not during startup. This is source-level modularity in a single executable; dynamic plugin loading is intentionally out of scope.

## Testing strategy

Tests use Qt Test and CTest, divided by risk:

- **Unit tests:** output naming/collisions, MIME/extension planning, media compatibility decisions, command argument arrays, error mapping, task state transitions, settings keys/migrations, log bounds, and QR render sizing.
- **Model/controller tests:** queued updates, cancellation races, warning confirmation, row/result association, and no UI-thread blocking assumptions using signals and event-loop-aware spies.
- **Process integration tests:** small fake executables emit controlled stdout/stderr, delay, fail, ignore terminate, and create or omit outputs. Tests verify arguments arrive literally and no shell expansion occurs.
- **Format integration tests:** tiny generated/redistributable image and media fixtures exercise PNG/JPEG/WebP, compatible remux with multiple audio streams, confirmed stream exclusions, fallback re-encode, corrupt input, and missing codecs. External-tool tests skip with an explicit reason when capabilities are absent.
- **Filesystem tests:** temporary directories cover Unicode/control characters, symlinks, permissions, collisions, cancellation cleanup, and atomic publication.
- **Widget smoke tests:** keyboard actions, drag/drop acceptance, focus order, system palette response, and close-with-running-work.
- **Performance tests:** instrument startup, idle wakeups, large queue insertion, and responsiveness during CPU/process tasks on Fedora KDE. Performance targets are tracked rather than made timing-fragile unit tests.
- **Manual platform matrix:** Fedora KDE Wayland is the release gate; Fedora KDE X11 (while available) and at least one non-KDE desktop are compatibility checks later.

CI should build Debug and Release configurations, enable strict warnings, run unit/integration tests, and include AddressSanitizer/UndefinedBehaviorSanitizer jobs. Tests must not require network access or user data.

## Architectural exclusions for 0.1

- No placeholder classes/directories for ColorDrop, Folder Preview, or GitSnap
- No plugin loader or stable module ABI
- No QML/Qt Quick or web UI/resources
- No SQL database
- No daemon, tray-resident mode, file watcher, or scheduled work
- No arbitrary command execution or editable process templates
- No copied Prism Launcher code, branding, icons, assets, or layout
