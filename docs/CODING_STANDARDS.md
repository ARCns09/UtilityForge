# UtilityForge Coding Standards

## Purpose and authority

These standards apply to all UtilityForge application, test, build-support, and packaging
changes. They implement the approved C++20, Qt 6 Widgets, and CMake architecture. New code
must follow them unless a documented technical constraint requires a narrowly reviewed
exception.

The priorities, in order, are correctness and safety, GUI responsiveness, clarity,
testability, and compactness. Cleverness, speculative abstraction, and style imitation of
another project are not goals.

## Language and framework rules

- Compile project C++ as C++20; do not rely on compiler extensions where a standard solution
  exists.
- Build the interface with Qt 6 Widgets. Do not introduce QML, Qt Quick, web views, HTML,
  CSS, JavaScript, Electron, React, or Tauri.
- Prefer the C++ standard library and already-required Qt modules before adding a dependency.
- Use Qt types at Qt API boundaries: `QString` for Qt-facing text and paths, `QByteArray` for
  byte-oriented Qt APIs, and Qt containers where a Qt model/signal API benefits from them.
- Prefer standard types for framework-independent domain logic when they avoid conversions:
  `std::optional`, `std::variant`, `std::span`, `std::chrono`, and standard algorithms are
  encouraged.
- Make conversions between Qt and standard types explicit and keep them at boundaries.
- Use `enum class` rather than unscoped enums.
- Use `nullptr`, not `0` or `NULL`.
- Use `QStringLiteral` for non-translated fixed Qt strings and `tr()` in an appropriate
  QObject/widget context for visible user text. Protocol tokens, tool arguments, settings
  keys, and log category names are not translated.
- Add `Q_OBJECT` only when a class needs signals, slots, properties, translation context, or
  other meta-object behavior.

## Naming conventions

| Element | Convention | Example |
| --- | --- | --- |
| Namespace | lowercase | `utilityforge::core`, `utilityforge::dropzone` |
| Class or value type | PascalCase | `TaskManager`, `OperationPlan` |
| Function or method | lowerCamelCase | `buildOutputPath()` |
| Local variable or parameter | lowerCamelCase | `outputPath` |
| Private data member | lowerCamelCase with trailing underscore | `taskManager_` |
| Compile-time constant | `k` plus PascalCase | `kMaximumImagePixels` |
| Enum type and enumerator | PascalCase | `TaskState::Running` |
| Qt signal | lowerCamelCase event/change phrase | `taskFinished()`, `progressChanged()` |
| Slot/handler method | lowerCamelCase verb phrase | `handleCancelRequested()` |
| Preprocessor macro | UPPER_SNAKE_CASE | `UTILITYFORGE_VERSION` |
| Primary class files | match class name | `TaskManager.h`, `TaskManager.cpp` |
| Test file | `tst_` plus unit name | `tst_OutputPlanner.cpp` |

Additional naming rules:

- Names describe domain meaning, not implementation mechanics. Prefer `outputPlan` to
  `data2`.
- Boolean names begin with `is`, `has`, `can`, `should`, or another unambiguous predicate.
- Accessors normally use the noun (`state()`), following Qt style; do not add `get` without
  a meaningful distinction.
- Avoid abbreviations except established domain terms such as `qr`, `ui`, `id`, and `xdg`.
  Capitalize them naturally inside identifiers, for example `qrEncoder` and `taskId`.
- Do not use Qt Designer auto-connect names such as `on_startButton_clicked`. Connections are
  explicit, and handlers describe intent rather than a particular widget.
- Names beginning with an underscore, double underscores, and reserved implementation names
  are forbidden.

## Formatting rules

A repository `.clang-format` file will be the mechanical authority once implementation
begins. Until then, use these rules:

- Four spaces per indentation level; never use tabs.
- UTF-8 source text and Unix line endings.
- Aim for 100 columns. Exceed it only when splitting would reduce readability, such as an
  indivisible diagnostic string or URL.
- Put a function's opening brace on the next line. Put control-statement and lambda opening
  braces on the same line.
- Write pointer and reference symbols with the type in declarations following Qt style:
  `QObject *parent` and `const QString &path`.
- Use one declaration per statement.
- Use braces for every `if`, loop, and branch, including one-line bodies.
- Prefer early returns when they reduce nesting and preserve a clear success path.
- Keep functions focused. A function that mixes validation, command construction,
  execution, UI mutation, and error formatting must be split across the appropriate
  boundary.
- Order includes as: matching header, project headers, Qt headers, standard headers. Separate
  groups with one blank line and sort within a group.
- Headers include what they use and do not depend on transitive includes. Use forward
  declarations only where they remain correct and improve build isolation.
- Use `#pragma once` consistently for project headers.
- Avoid `using namespace` in headers and source files. Narrow type aliases are allowed inside
  the smallest useful scope.
- Use `auto` when the initializer makes the type obvious, for iterators, or when it avoids
  repeating a long type. Do not use it when the concrete type communicates units, ownership,
  signedness, or an important Qt conversion.
- Mark values and methods `const` where appropriate. Use `const` local values by default when
  they are not reassigned.
- Use `[[nodiscard]]` for result values whose omission can lose an error, plan, or output
  state.
- Comments explain intent, constraints, ownership, or a non-obvious safety decision. They do
  not restate the code. TODO comments include an issue/reference or a concrete follow-up
  condition.

Formatting changes should be mechanical and separate from unrelated behavior changes where
practical. Do not reformat untouched user code merely because a tool can do so.

## Ownership and memory management

- Prefer automatic storage and value semantics.
- Every allocated object has one obvious owner. Ownership is visible from the type or Qt
  parent relationship.
- Use `std::unique_ptr<T>` for exclusive ownership of non-QObject resources that cannot live
  directly by value. Construct with `std::make_unique`.
- Use `std::shared_ptr<T>` only when lifetime is genuinely shared and cannot be represented
  by a clearer owner/observer relationship. Document why shared ownership is necessary.
- Use `std::weak_ptr<T>` to observe a shared object without extending its lifetime.
- A raw pointer or reference is non-owning unless the API explicitly documents otherwise.
  Prefer references for required, non-null borrowed objects and pointers for optional
  borrowed objects.
- Do not use owning raw pointers, manual `new`/`delete` pairs, or ownership hidden inside an
  untyped container.
- Qt implicit sharing is value semantics, not an excuse to retain large images or buffers
  indefinitely. Release task-heavy values when no longer needed.
- Do not mix a QObject parent owner with `std::unique_ptr` or `std::shared_ptr` ownership of
  the same object.
- RAII wrappers must own files, locks, staging cleanup, and other non-QObject resources so
  early returns cannot leak them.

## QObject parent ownership

- A QObject created for a QObject/widget hierarchy receives its final parent at construction
  whenever possible.
- Parent-owned QObjects are stored as non-owning pointers. For observations that can outlive
  or race deletion, use `QPointer<T>`.
- Top-level objects without a parent must have an explicit stack or RAII owner.
- Never manually delete a parent-owned child during ordinary teardown. If early asynchronous
  destruction is required, use `deleteLater()` on the object's affinity thread and clear
  observers.
- A QObject and its parent must have compatible thread affinity. Do not parent an object to
  an object in another thread or move a QObject that still has a parent.
- GUI objects always remain on the GUI thread.
- Do not retain pointers to temporary dialogs, model indexes, or signal payload data beyond
  their documented lifetime.

## Signals and slots

- Use typed function-pointer/lambda `connect` syntax. String-based `SIGNAL`/`SLOT` syntax and
  automatic `connectSlotsByName` conventions are not used.
- A signal reports an event or state change; it does not command another object. Commands
  are normal methods on the responsible controller/service.
- Keep signal payloads small, immutable by value, and meaningful outside the sender. For
  large results, send a stable task/result ID or an implicitly shared immutable value.
- A connection from a lambda always supplies a context QObject when the lambda captures
  QObject state, so Qt disconnects it when that context is destroyed.
- Avoid capturing borrowed raw pointers in asynchronous lambdas. Use a context-bound
  connection, `QPointer`, a stable ID, or an owned value.
- Cross-thread delivery uses `Qt::QueuedConnection` explicitly when thread affinity is not
  self-evident. Direct cross-thread UI calls are forbidden.
- Do not use `Qt::BlockingQueuedConnection` in application code.
- Do not emit signals while holding a mutex.
- A slot validates externally supplied or stale identifiers before mutating state.
- Use `Qt::UniqueConnection` only when duplicate connections would be a bug; it is not a
  substitute for clear connection ownership.

## Threading and responsiveness

- The Qt main thread handles widgets, models, dialogs, clipboard access, short state updates,
  and event-driven `QProcess` observation only.
- Filesystem traversal, blocking metadata access, image decode/encode, QR generation, and
  other CPU-heavy or blocking work run through the shared bounded `QThreadPool`.
- Do not call blocking `waitFor...` APIs, sleep, perform unbounded loops, or wait on futures
  from the GUI thread.
- Prefer `QThreadPool` tasks for finite work. Use a dedicated `QThread` only when an object
  genuinely needs a long-lived event loop or serialized resource affinity. Do not subclass
  `QThread` merely to place a `run()` function somewhere.
- Worker inputs are immutable value snapshots. Worker results return through queued signals
  or queued invocations and are applied to models only on the GUI thread.
- Shared mutable state is the exception. If unavoidable, protect it with a narrowly scoped
  lock and never call user code, emit a signal, block, or touch UI while holding the lock.
- Every cancellable worker receives a `CancellationToken`, checks it at meaningful bounded
  intervals and immediately before publication, and treats cancellation as a terminal
  result.
- Task state transitions are serialized by `TaskManager`; workers do not mutate UI task
  records directly.
- Do not create one thread per queue row or use unbounded global thread-pool concurrency.
- Progress is event-driven and may be coalesced while a task is active. Do not add permanent
  polling timers.

## QProcess rules

All supported external execution goes through `ExternalToolRegistry`, `ProcessRequest`, and
`ProcessRunner`.

- Resolve an allowlisted executable to an absolute path and capability-probe it before use.
- Set the program and `QStringList` arguments separately. Never construct a shell command and
  never invoke `/bin/sh`, `bash`, `sh -c`, or an equivalent.
- Do not expose arbitrary arguments, executable paths from dropped content, or command
  templates to version 0.1 users.
- Validate and normalize input, output, and working-directory paths before start. Use a
  task-owned staging output, never the source or final output directly.
- Set an explicit working directory when it affects archive member names or tool behavior.
- Construct a reviewed environment and remove behavior-injection variables where safe.
  Never log the full environment.
- Use asynchronous `started`, `readyReadStandardOutput`, `readyReadStandardError`, `errorOccurred`,
  and `finished` handling. `waitForStarted`, `waitForReadyRead`, `waitForFinished`, and
  blocking pipe reads are forbidden on the GUI thread.
- Parse documented machine-readable formats: FFprobe JSON and FFmpeg progress records.
  Treat all output as untrusted and enforce byte/time limits.
- Prevent tools from waiting for interactive stdin.
- Cancellation is idempotent: request `terminate()`, start a non-blocking grace timer, then
  `kill()` the exact child if necessary.
- Success requires the expected normal exit plus operation-specific output validation.
  Exit code zero alone is insufficient.
- Bound retained stdout/stderr, escape control characters for display, and never execute a
  diagnostic representation of arguments.
- Unit-test literal argument transport using fake executables and hostile filenames.

## Error handling

- Expected failures use typed result values carrying `AppError`; they are not represented by
  exceptions, magic booleans, or preformatted modal-dialog text.
- `AppError` contains a stable category/code, severity, safe user message, optional technical
  details, task ID where relevant, and recovery actions.
- Validate at system boundaries: user input, filesystem state, settings, codec data, process
  output, and external-tool capabilities.
- Do not ignore return values from file operations, `QSettings::sync()`, parsing, process
  start, staging publication, or cleanup.
- Recoverable exceptions from an approved dependency must be caught at the worker boundary
  and converted to a structured internal error before crossing a Qt signal/event boundary.
  Do not treat `std::bad_alloc` or another process-integrity failure as an ordinary
  recoverable task error. Never allow exceptions to escape Qt callbacks.
- Assertions express programmer invariants, not recoverable input errors. Release behavior
  must still fail safely if an external value violates an assumption.
- Cancellation is a distinct terminal state, not a generic error.
- Show user-correctable errors inline and use the shared notification system. Avoid modal
  dialog storms.
- Preserve useful technical context without disclosing file contents, clipboard data,
  credentials, or full environments.
- Never silently continue after output validation, cleanup, or stream-exclusion behavior
  differs from the approved plan.

## Logging

- Define focused `QLoggingCategory` categories such as `utilityforge.core.tasks` and
  `utilityforge.dropzone.media`; do not use ad hoc unconditional `qDebug()` output.
- Use debug for development detail, info for meaningful lifecycle events, warning for
  recovered or user-actionable problems, and critical for lost invariants or unsafe
  completion.
- Each log entry names the task/component and event. Avoid prose that cannot be searched.
- Tool output is untrusted plain text. Normalize or escape control characters before
  display, and retain only bounded excerpts.
- Never log file contents, QR/clipboard text, secrets, credentials, full process
  environments, or shell-ready command strings.
- Paths may be logged only when needed for local diagnosis and are treated as private when
  logs are copied or exported.
- Persistent logs are bounded/rotated or session-scoped and live under the approved XDG
  state location. Logging must not create periodic idle work.
- Tests may capture expected categories/messages, but should prefer stable error codes over
  brittle full-text comparisons.

## Testing standards

- Use Qt Test with CTest orchestration. Tests are separate targets and never linked into the
  production executable.
- Mirror source boundaries under `tests/core` and `tests/dropzone`.
- Name tests after behavior and outcome, not implementation sequence.
- Unit-test pure planners, validators, state transitions, command argument arrays, output
  naming, settings migration, and error mapping.
- Use `QSignalSpy` and the Qt event loop for asynchronous behavior. Do not make tests pass by
  adding arbitrary sleeps.
- Use temporary directories and small owned/redistributable fixtures. Tests never read or
  modify real user configuration, cache, data, clipboard, or home-directory files.
- Fake tool executables cover success, malformed output, missing output, non-zero exit,
  delayed completion, terminate/kill behavior, and literal hostile arguments.
- Tests are deterministic, offline, locale-aware where parsing is involved, and independent
  of execution order.
- Integration tests that require FFmpeg, FFprobe, `bsdtar`, or a codec plugin must probe the
  capability and skip with a precise reason when absent. Core safety tests use fakes and
  never skip.
- Every bug fix includes a regression test where the behavior can be reproduced reliably.
- Cancellation tests cover queued, startup, running, pre-publication, and cleanup races.
- Media tests cover one video with zero, one, and multiple compatible audio streams; visible
  exclusion of subtitle, attachment, data, and incompatible audio streams; and rejection of
  zero/multiple video streams.
- CI builds Debug and Release configurations with strict project warnings. Sanitizer jobs
  run AddressSanitizer and UndefinedBehaviorSanitizer tests.
- Performance measurements cover startup, idle wakeups, large queues, and UI responsiveness.
  Avoid fragile wall-clock assertions in normal unit tests.

## Commit message conventions

Use an imperative, scoped subject:

```text
area: concise imperative summary
```

Examples of areas are `core`, `dropzone`, `ui`, `build`, `tests`, `docs`, and `packaging`.

- Keep the subject at or below 72 characters, without a trailing period.
- Describe what the commit does, not what was done in the past.
- Use a body when the reason, safety tradeoff, migration, or user-visible consequence is not
  obvious. Wrap body text consistently.
- Reference an issue or decision when applicable, but do not make an external link the only
  explanation.
- Keep commits cohesive and reviewable. Separate broad formatting, mechanical renames,
  dependency changes, and behavioral changes.
- Include relevant tests and documentation with the behavior they validate.
- Do not commit build output, local settings, logs, credentials, downloaded binaries, or
  unrelated user changes.
- Do not claim tests passed unless they were actually run in the described environment.

## Dependency policy

- Prefer standard C++ and the already-approved Qt Core, Gui, and Widgets APIs.
- Required build dependencies and feature runtime tools remain classified separately in
  `DEPENDENCIES.md`.
- A new dependency requires a documented capability gap, alternatives considered, Fedora
  availability, license, maintenance/security status, binary/startup impact, and test plan.
- Keep a dependency behind a narrow project-owned adapter so domain/UI code does not depend
  on its API. `QrEncoder` is the model for libqrencode.
- Use system packages for normal Fedora builds. Do not download code during a normal CMake
  configure/build and do not add a package manager without approval.
- Optional tools must fail closed at their feature boundary and must not prevent application
  startup.
- Do not add a broad framework to solve a narrow problem already covered by Qt, the standard
  library, or an approved external tool.
- Pin or constrain versions only with a compatibility/security reason and test the declared
  minimums.
- Any dependency addition or replacement updates architecture, dependency, security,
  packaging, and license documentation before merge.

## Classes versus free functions

Use a class when at least one of these is true:

- it owns a resource or maintains an invariant across operations;
- it has meaningful state or lifecycle;
- it is a QObject participating in signals, thread affinity, parenting, or Qt models;
- callers need substitutable behavior at a tested boundary;
- a cohesive domain concept benefits from encapsulated private representation.

Use a free function when:

- the operation is stateless and naturally expressed entirely by its inputs and return value;
- it is a pure transformation, parser, validator, comparison, or small algorithm;
- no identity, ownership, lifecycle, virtual dispatch, or QObject behavior is needed.

Additional design rules:

- Put related free functions in a domain namespace; do not create static “utility” or
  “manager” classes solely to group functions.
- Use a `struct` for passive value aggregates with public data and maintained-by-construction
  validity. Use a `class` when invariants require controlled mutation.
- Do not make a class a QObject merely to obtain callbacks; a normal function, result value,
  or injected callable is often clearer.
- Introduce abstract interfaces only when there are real independent implementations, a
  required fake boundary, or a resource boundary that must be substituted. Avoid one-method
  interfaces created speculatively.
- Prefer composition to inheritance. Widget inheritance is appropriate for actual widgets;
  domain behavior should not be hidden in deep widget subclasses.
- A class named `Manager`, `Service`, or `Helper` must have a precise responsibility stated
  in its documentation. Split it when it becomes a miscellaneous dependency hub.
