# Dependency Policy and Inventory

## Policy

UtilityForge uses C++20, Qt 6 Widgets, and CMake. A dependency is acceptable only when the C++ standard library and the already-required Qt modules cannot reasonably provide the feature, its maintenance and license are understood, and the capability can be isolated behind a small interface.

There are three distinct categories below:

1. **Required build/runtime dependencies** are necessary to compile and launch the application.
2. **Feature runtime tools** are required only for a named operation; their absence must not prevent launch.
3. **Optional development tools** improve build or test workflows but do not ship as application requirements.

Dependencies must be discovered through CMake targets or absolute executable paths. The project will not download dependencies implicitly during a normal configure or build.

## Required build and application dependencies

| Dependency | Proposed minimum | Purpose | Rationale |
| --- | --- | --- | --- |
| C++ compiler with C++20 support | GCC 12+ or Clang 15+ | Compile the application | Approved Fedora-first baseline with mature C++20 support. Exact CI versions can be newer. |
| CMake | 3.24+ | Configure, build, test, and install | Modern target-based CMake without requiring the newest distribution release. |
| Qt 6 Core | 6.5+ | Event loop, object model, paths, settings, processes, threading | Core application infrastructure. |
| Qt 6 Gui | 6.5+ | Clipboard, drag/drop, images, desktop services | Required by Widgets and DropZone. |
| Qt 6 Widgets | 6.5+ | Entire user interface | The mandated native UI technology. |
| libqrencode | distribution-supported release | Encode QR module matrices behind `QrEncoder` | Approved small, reliable QR implementation. |

Qt Network, Qt SQL, Qt Concurrent, KDE Frameworks, and multimedia frameworks are not required for 0.1. `QThreadPool` from Qt Core is sufficient for in-process background work. SQLite is not used because version 0.1 has no structured history. Qt's PNG/JPEG handlers and a matching WebP image-format plugin are capability-checked; absent WebP support disables only WebP input/output rather than preventing application launch.

Qt 6.5 is the approved compatibility floor because it is an LTS line and provides the required Widgets APIs. Fedora KDE is the primary packaging and test target; support for other distributions depends on meeting the same Qt and compiler floor.

## QR encoding dependency

Qt does not provide a QR encoder. **libqrencode is approved** as one small, required linked dependency for version 0.1 and is isolated behind `QrEncoder`. This is preferable to invoking an extra process, implementing a non-trivial standard from scratch, or adopting a larger barcode suite. Its exact packaged version and license metadata must still be recorded during release preparation.

## Feature runtime tools and plugins

External executables are detected lazily with `QStandardPaths::findExecutable`, resolved to absolute paths, and capability-probed. Qt codec plugins are queried through Qt's supported-format APIs. A missing tool or plugin disables only its operations.

| Tool | Feature status | Used for | Detection/probe |
| --- | --- | --- | --- |
| `ffmpeg` | Required for MKV-to-MP4 operations | Stream-copy remux and opt-in H.264/AAC re-encode | Resolve executable, run a bounded version/encoder probe, cache for the session. |
| `ffprobe` | Required for MKV-to-MP4 operations | Machine-readable stream/container inspection | Resolve executable and validate JSON output from a bounded probe. |
| `bsdtar` | Required for ZIP and TAR.GZ compression | Create archives without linking a full archive library | Resolve executable and probe supported formats/options. |
| Qt WebP image-format plugin | Required for WebP operations | Decode and encode WebP through Qt's image API | Check `QImageReader`/`QImageWriter` supported formats at runtime. |

“Required for feature” does not mean required to start UtilityForge. The UI must show which operations are unavailable and a distribution-neutral explanation naming the missing executable. It must not attempt package installation or invoke a privileged package manager.

The approved archive tool is `bsdtar` because one executable can produce both ZIP and compressed tar archives. If Fedora packaging or capability testing makes this impractical, replacing it with separate `zip` and `tar` tools requires a new approval decision; supporting multiple archive backends in 0.1 is not planned.

FFmpeg builds vary. UtilityForge must probe for the specific encoders used by the standard re-encode preset instead of assuming that every `ffmpeg` binary includes them.

## Platform runtime integration

The Qt platform and image plugins installed with Qt provide desktop styling, file dialogs, and image codecs. UtilityForge uses `QDesktopServices` for opening files and output directories, allowing the desktop to select the handler. KDE Frameworks are intentionally not linked in 0.1 so that later GNOME/Xfce support remains practical.

No portal library is linked directly in 0.1. Qt may use desktop portals through its platform integration. A direct portal dependency should be proposed only when a later feature demonstrably needs behavior Qt cannot expose.

## Optional development dependencies

| Tool | Purpose | Shipping impact |
| --- | --- | --- |
| Ninja | Fast local/CI build generator | None; Makefiles remain possible. |
| clang-format | Consistent C++ formatting | Development only. |
| clang-tidy | Static analysis | Development/CI only. |
| GCC/Clang sanitizers | Address and undefined-behavior testing | Test builds only. |
| gcovr or llvm-cov | Coverage reporting | CI only; not required for ordinary builds. |
| CTest | Test orchestration | Included with CMake. |

The test framework is Qt Test, which is built as test-only targets. It is required only when `BUILD_TESTING=ON` and is never linked into the application.

## Explicitly excluded dependencies

- QML and Qt Quick
- Electron, React, Tauri, browser engines, HTML, CSS, and JavaScript runtimes
- KDE Frameworks in the 0.1 core
- Boost for functionality already covered by C++20 or Qt
- A logging framework before Qt and a small bounded file sink prove insufficient
- SQLite before a real structured-history requirement exists
- A media framework linked into the process when FFmpeg processes provide the narrow required behavior
- Automatic dependency fetchers in the default build

## Packaging and versioning rules

- CMake consumes imported targets such as `Qt6::Core`, never global include/link flags.
- System libraries are preferred for Fedora packaging. Any vendoring proposal requires an explicit maintenance and security rationale.
- Runtime tool versions and capabilities are shown in diagnostics, not treated as a guarantee based only on a version string.
- Optional feature availability is tested at runtime and covered by tests with fake executables.
- Dependency licenses and redistribution terms are recorded before release packaging.
