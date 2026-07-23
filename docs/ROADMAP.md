# UtilityForge Development Roadmap

## Delivery rules

- Each phase begins only after the preceding acceptance gate is met.
- Version 0.1 implements the application shell and DropZone only.
- A future module's directory, navigation entry, model, or placeholder is created only when that module's implementation phase starts.
- The technology remains C++20, Qt 6 Widgets, and CMake throughout. QML and web technologies are not fallback options.
- Fedora KDE Wayland is the primary release environment; broader Linux checks follow once the native baseline is sound.
- Every feature phase includes tests, accessibility/keyboard checks, cancellation behavior, and performance verification rather than deferring them to a final cleanup phase.

## Phase 0 — Plan and approval

Deliver the seven planning documents, reconcile their scope, and obtain approval for architecture, version 0.1 boundaries, dependency choices, and minimum versions.

Status: the architecture and listed technical decisions are approved, and documentation alignment is complete. Final implementation approval remains pending.

Exit gate:

- product and DropZone acceptance criteria are approved;
- libqrencode and `bsdtar` decisions are approved or the affected scope is revised;
- Qt/CMake/compiler floors are approved;
- output collision, image metadata, archive symlink, and media stream rules are approved.

No source code, dependency installation, or GitHub publication occurs in this phase.

## Phase 1 — Native shell and core foundations

After approval, create the minimal CMake project, application bootstrap, main window, DropZone navigation, settings/path service, structured errors, notification surface, task manager, process runner, and external tool registry. Create only directories required by these actual components.

Build the shell with the system Qt style, keyboard navigation, toolbar host, queue/status surfaces, and an honest empty state. Add unit tests for task transitions, cancellation races, paths/settings, and safe process argument transport using fake tools.

Exit gate:

- the application starts quickly and idles without periodic work;
- the UI stays responsive during fake CPU/process tasks;
- cancellation and close-with-active-work behave deterministically;
- missing external tools are represented without blocking startup;
- no placeholder future module code or navigation exists.

## Phase 2 — DropZone input, planning, and output safety

Implement local drag/drop, native pickers, the queue model/controller, background preflight, operation/output planning, staging, atomic publication, collision naming, logs, and clipboard actions. Exercise hostile filenames, permissions, symlinks, cancellation cleanup, and removable/slow filesystem behavior.

Exit gate:

- large or slow input selection does not synchronously enumerate on the GUI thread;
- every queued item has an explicit operation and destination before execution;
- inputs are never overwritten;
- cancellation/failure never publishes a staging file as a successful result.

## Phase 3 — DropZone image and QR operations

Implement still-image conversion through Qt's image APIs on a bounded thread pool, including format capability detection, alpha warnings, orientation behavior, pixel limits, quality settings, progress phases, and safe output. Implement QR text input, encoding through the approved isolated dependency, preview, PNG saving, and copying.

Exit gate:

- PNG, JPEG, and WebP acceptance fixtures pass on Fedora KDE;
- corrupt/oversized images fail safely and the interface remains responsive;
- QR boundary, Unicode, cancellation, render-size, and clipboard tests pass;
- unavailable WebP support produces a capability error rather than a crash or false success.

## Phase 4 — DropZone archive and video operations

Implement probed `bsdtar`, FFprobe, and FFmpeg workflows through the shared asynchronous process runner. Add archive staging/validation, narrow MKV stream inspection, compatible remux, explicit re-encode fallback, structured progress where trustworthy, cancellation, and bounded logs.

Exit gate:

- ZIP and TAR.GZ creation passes the documented file/folder/symlink contract;
- compatible MKV fixtures, including fixtures with multiple AAC/MP3 audio streams, remux without re-encoding;
- incompatible codecs offer re-encoding only after preflight and explicit confirmation;
- subtitles, attachments, data, and incompatible audio streams are enumerated before confirmed exclusion, while inputs without exactly one video stream are rejected;
- termination/kill, partial-output cleanup, and output validation tests pass.

## Phase 5 — Version 0.1 hardening and Fedora KDE release

Profile startup, memory, idle wakeups, task concurrency, and large-queue behavior. Complete keyboard/focus/accessibility checks, theme checks, original icons/metadata, installation rules, license inventory, diagnostics, user documentation, and Fedora KDE Wayland acceptance testing.

Exit gate:

- all `DROPZONE_MVP.md` acceptance criteria pass;
- Release and sanitizer CI are green;
- dependency licenses and runtime capability messages are reviewed;
- no QML/web artifact, future-module placeholder, copied Prism asset, background service, or hidden network behavior exists;
- a release candidate can be built reproducibly from documented system dependencies.

## Post-0.1 sequencing

The order below is provisional and requires a fresh specification for each module.

### Folder Preview prototype and implementation

Validate cancellable traversal, filesystem boundaries, media thumbnail behavior, and memory limits. Then add the real module and its navigation entry. Global folder hovering remains excluded.

### ColorDrop Wayland prototype and implementation

First validate KDE Wayland's supported screen-pick/portal experience. Only after a viable permission and interaction model exists should the real module directory be created. Add conversions and palettes before persistent favorites; SQLite is unnecessary for a small favorites set unless evidence shows otherwise.

### GitSnap threat model and implementation

Perform a separate security design for repository URLs, credentials, network failures, clone cancellation, ZIP downloads, and editor launch configuration. Implement generic Git first, add GitHub-specific archive behavior behind clear URL validation, and persist only minimal recent metadata.

## Later candidates, not commitments

- desktop notifications after cross-desktop behavior is evaluated;
- broader Linux desktop test/packaging matrix;
- Flatpak or AppImage packaging after codec/tool and portal strategy is approved;
- richer task history, with SQLite only if structured querying and retention justify it;
- additional DropZone formats and presets based on user evidence;
- translations after interface strings and workflows stabilize.

None of these candidates should introduce a daemon, idle polling, web UI, or mandatory KDE Framework dependency without a separately approved architectural change.
