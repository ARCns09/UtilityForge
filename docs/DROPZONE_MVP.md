# DropZone Version 0.1 Specification

## Version statement

Version 0.1 is a native Qt 6 Widgets application shell containing one functional module: DropZone. It is not a preview of four empty modules. The sidebar exposes DropZone plus Settings/About access only; ColorDrop, Folder Preview, and GitSnap are neither implemented nor represented by placeholder pages or directories.

The implementation language and build system are strictly C++20 and CMake. No QML or web technology is used.

## Included user interface

Version 0.1 includes:

- one compact main window following the system Qt/KDE palette and widget style;
- a compact sidebar with DropZone selected;
- a DropZone toolbar for adding files, adding a folder, creating a QR code, starting queued work, cancelling selected work, and opening settings;
- a central queue table with input name, operation, state, progress/phase, output, and error indicator;
- a drop empty-state that accepts supported local file URLs and folders;
- native file/folder dialogs, native menus/context menus, keyboard shortcuts, and standard focus behavior;
- a details area for preflight warnings, bounded task logs, tool capability, and output location;
- a status bar showing active/queued task counts and aggregate state without periodic polling;
- an in-window notification surface for success, warning, failure, and actionable missing-capability messages.

Primary actions must be usable without a mouse. The design uses lists/tables and standard controls; it does not copy Prism Launcher layouts, assets, branding, or icons.

## Accepted inputs

Drop and picker input is limited to accessible local files and folders. Remote URLs and non-file MIME payloads are rejected with a reason. A directory is not enumerated merely to add it to the queue.

Supported operation inputs are:

- still PNG, JPEG/JPG, and WebP images that Qt can decode;
- Matroska (`.mkv`) files confirmed by FFprobe;
- one regular file or one directory per compression queue item;
- text entered directly in the QR dialog.

MIME/format inspection is authoritative where practical; extension alone is not enough to execute an operation. Symlinks are identified and handled by the operation-specific rules. Special files such as sockets, devices, and FIFOs are rejected.

## Queue and planning behavior

Each dropped top-level input becomes a stable queue row. Multiple selections create multiple independent tasks, so each can succeed, fail, or be cancelled without changing the others. The user chooses or confirms an applicable operation and output location before starting.

Potentially slow metadata reads, image probes, and media probes occur asynchronously. Rows can show “Inspecting” without freezing the rest of the interface. Starting is disabled until the row has a valid immutable plan.

Supported row states are Pending, Inspecting, Ready, Queued, Running, Cancelling, Succeeded, Failed, and Cancelled. A retry creates a new execution attempt while retaining the prior diagnostic result until replaced.

## Image conversion

Included behavior:

- Convert a still PNG, JPEG, or WebP image to one of the other listed formats, or re-encode to the same format when explicitly selected.
- Use Qt `QImageReader`/`QImageWriter` and the installed matching image plugins; no external image command is invoked.
- Apply reader auto-transform so EXIF orientation is reflected in output pixels.
- Preserve pixel dimensions; resizing, cropping, and rotation controls are not included.
- Offer JPEG quality and lossy WebP quality with conservative defaults; PNG uses its normal lossless path. A WebP lossless toggle is included only if the active Qt plugin reports support reliably.
- Warn before converting transparency to JPEG and composite against a user-visible solid background choice; default is white.
- Treat animated inputs as unsupported rather than silently writing only an arbitrary frame.
- Enforce a configurable-in-code maximum decoded pixel count. The initial limit is 100 megapixels and is reported clearly when exceeded.
- Run decode and encode on the bounded worker pool, check cancellation between phases, and publish through a staged file/`QSaveFile`-style commit.

Version 0.1 promises rendered-pixel conversion, orientation application, and standard color output. It does not promise preservation of EXIF/IPTC/XMP metadata, embedded thumbnails, animation, or every ICC/color-profile detail; the confirmation text states this when metadata is detected.

## MKV to MP4

Included behavior is deliberately narrow and deterministic:

- Accept a Matroska input containing exactly one video stream, zero or one audio stream, and no subtitle, attachment, or data streams.
- Use FFprobe JSON to inspect the container, codec names, stream counts, and duration before proposing an operation.
- Offer lossless remux when video is H.264/AVC or H.265/HEVC and audio is absent, AAC, or MP3. FFmpeg copies streams with no media re-encoding.
- If that narrow stream layout is valid but a codec is not remux-compatible, offer—not automatically start—a standard re-encode to H.264 using available `libx264` and AAC for audio. The dialog clearly labels it lossy, slower, and potentially larger/smaller.
- If the required encoders are unavailable, report the missing capability and do not offer a nonfunctional preset.
- Reject other stream layouts in 0.1 with a precise message. UtilityForge does not silently discard subtitles, attachments, extra audio, data, or extra video streams.
- Produce an `.mp4` staged output and verify successful process exit plus a minimal FFprobe inspection before publication.
- Use FFmpeg machine progress relative to the probed duration when available; otherwise show a phase/indeterminate progress.

The application neither offers nor runs re-encoding when the file qualifies for remux. Re-encoding appears only as the explicit fallback for a codec-incompatible file that otherwise meets the narrow stream-layout contract. Source MKV files are never changed.

## Compression

Included behavior:

- Create either ZIP (`.zip`) or gzip-compressed tar (`.tar.gz`) using a capability-probed `bsdtar` process.
- Each selected regular file or directory produces its own archive task and archive output. Batch selection therefore produces multiple archives, not one combined archive.
- Store the selected top-level item's basename and its descendants, never an absolute path.
- Include hidden descendants.
- Store symlinks as symlinks and do not follow them.
- Reject top-level or encountered device nodes, FIFOs, and sockets rather than archiving special objects ambiguously. An inaccessible descendant fails the task; 0.1 does not publish a silently incomplete archive.
- Validate the resulting archive by listing it with the same probed tool before publication.
- Show indeterminate progress if the tool cannot supply trustworthy structured progress; cancellation remains available.

Password-protected/encrypted archives, extraction, split archives, custom compression levels, ignore patterns, multi-root archives, and alternate archive backends are postponed.

## QR generation

Included behavior, contingent on approval of the isolated libqrencode dependency:

- Open a native dialog accepting plain text or a URL as data; UtilityForge does not fetch or open it.
- Accept UTF-8 input up to 2,048 bytes for a predictable UI/security bound, subject to the encoder's stricter capacity for the chosen data.
- Generate a QR code at error-correction level M with the required quiet zone.
- Preview at integer scaling and save a crisp PNG at 512 × 512 pixels by default.
- Allow copying the generated image and the original text separately.
- Generate/render off the GUI thread when work may be perceptible and publish saved output safely.

QR decoding, styled/color/gradient QR codes, logos, SVG/PDF output, bulk QR jobs, automatic URL shortening, and network access are postponed.

## Output location and collision rules

The default is “beside source” for file/folder operations and the last explicitly selected output directory for QR images. The user can choose a different writable local directory before execution.

Outputs never overwrite inputs or existing files in version 0.1. Name collisions are resolved with a numeric suffix such as `name (2).png`. The chosen final name is visible before work starts.

Each operation writes to an unpredictable task-owned staging path in the destination directory. Only a validated success is renamed/committed to the final name. Cancellation or failure removes only the recorded staging artifact. If cleanup fails, the leftover path is reported. A destination that cannot support safe same-filesystem publication fails preflight or the task; silent cross-filesystem fallback is not included.

## Progress, cancellation, and concurrency

Every operation is submitted to the shared `TaskManager`. CPU/image/QR work uses a bounded `QThreadPool`; FFmpeg, FFprobe, and `bsdtar` use signal-driven `QProcess`. No blocking wait, full directory traversal, decode, encode, archive, media probe, or process pipe read is performed on the Qt GUI thread.

The initial concurrency policy is at most two heavyweight running tasks and at most `max(1, idealThreadCount - 1)` worker threads, capped at four. This is a safe default exposed as a bounded preference only if testing shows the setting is understandable. Queued tasks do not consume threads.

Cancellation is available for queued and running work. Cooperative tasks check a token between meaningful phases; external processes receive terminate and then kill after an asynchronous grace period. The UI distinguishes “Cancelling” from “Cancelled.” A cancellation racing with output completion is resolved before publication, so cancelled work cannot appear as success.

## Logs, errors, and notifications

Each task retains a bounded diagnostic log with phase changes, the resolved tool/version, exit status, and a truncated plain-text output excerpt. Logs do not include input contents, clipboard data, a full environment, or shell-ready commands. Paths can appear in local diagnostics and are identified as potentially private before export/copy.

Errors appear inline on the affected row and through the shared in-window notification surface. Missing tools/plugins explain which feature is disabled. Success is non-modal and offers open output/open containing directory/copy actions. Repeated failures do not create multiple modal dialogs.

## Clipboard actions

Version 0.1 supports explicit user actions to:

- copy a source or completed-output path as plain text;
- copy a source or completed output as local file URL MIME data;
- copy a generated QR image;
- copy the original QR text.

Generated outputs are copyable only after successful publication. Clipboard actions occur on the GUI thread and are never automatic.

## Settings retained in 0.1

QSettings stores window geometry/state, the last explicit output directory, output-location preference, image quality defaults, JPEG transparency background, default archive format, and approved concurrency setting. Settings are XDG-compliant as defined in `ARCHITECTURE.md`.

Inputs, QR text, clipboard contents, task history, and logs are not persisted as preferences. There is no SQLite database.

## External capability behavior

Tool checks are lazy and asynchronous. Application startup does not run FFmpeg, FFprobe, or `bsdtar`. Selecting a dependent operation triggers a cached capability check. The UI represents Checking, Available, Missing, and Unsupported states.

- Missing FFmpeg/FFprobe disables MKV actions only.
- Missing required FFmpeg encoders disables re-encoding but can leave remux available.
- Missing `bsdtar` disables compression only.
- Missing Qt WebP read/write support disables only the affected WebP source/target path.
- Missing the approved QR dependency is a build/configuration failure because QR is part of the approved 0.1 build, not a silently absent runtime feature.

UtilityForge does not install any dependency or request administrator access.

## Version 0.1 acceptance checklist

- The application builds as C++20 with CMake and Qt 6 Widgets and contains no QML/web runtime or assets.
- Fedora KDE Wayland is the primary manual acceptance environment and both system light/dark palettes remain readable.
- Startup does not probe external processes, and idle operation has no recurring worker/polling activity.
- Dropping supported and hostile-path inputs never blocks or injects a shell command.
- Still-image conversions pass format, orientation, alpha-warning, corrupt-input, oversize, cancellation, and collision tests.
- Compatible MKV input remuxes without re-encoding; codec-incompatible narrow-layout input requires explicit re-encode confirmation; unsupported streams are not silently dropped.
- File and folder ZIP/TAR.GZ tasks obey basename, hidden-file, symlink, special-file, validation, cancellation, and cleanup rules.
- QR Unicode/bounds, preview, save, and clipboard behavior passes with the approved dependency.
- All slow work leaves the main window interactive and all tasks have observable phase/progress plus cancellation.
- Source files and existing outputs are never overwritten.
- No source directory, placeholder page, navigation item, or empty model exists for ColorDrop, Folder Preview, or GitSnap.
- No Prism Launcher source, branding, icon, asset, or layout has been copied.

## Explicitly postponed beyond 0.1

The following are not included:

- all ColorDrop functionality;
- all Folder Preview functionality, including global folder hovering;
- all GitSnap functionality and network access;
- animated image conversion, resize/crop/edit tools, broad metadata preservation, RAW/AVIF/GIF/TIFF targets, and color-management guarantees;
- general-purpose media conversion, arbitrary FFmpeg arguments/presets, subtitle handling, multiple audio/video streams, attachments, hardware encoding, video trimming, and non-MKV inputs;
- archive extraction, encryption/passwords, split archives, custom levels/exclusions, combined multi-root archives, and alternate tool backends;
- QR decoding, styling, logos, vector output, bulk generation, and URL shortening;
- task/input history, recent files, SQLite, telemetry, analytics, and an update checker;
- desktop notifications, system tray/background mode, daemon/service, file watching, and automatic folder monitoring;
- direct portal APIs, KDE Frameworks-specific enhancements, Flatpak/AppImage packaging, and guaranteed non-KDE support;
- custom themes, excessive animation, QML, Electron, React, Tauri, HTML, CSS, JavaScript, or any browser technology;
- overwriting existing outputs, in-place conversion, and arbitrary external command execution.
