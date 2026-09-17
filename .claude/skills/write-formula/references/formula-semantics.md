# Formula Semantics

These are design rules, not a versioned API table. Resolve every concrete name,
signature, and return behavior from the target LLAR revision.

## Module And Version Ownership

Keep a module under its upstream repository id. Preserve exact upstream tag
spelling, including prefixes and case. Enumerate the complete upstream tag set
visible to LLAR, then inspect every existing Formula threshold and comparator
before adding another one.

Set `fromVer` to the lowest upstream version for which the same dependency,
build, install, metadata, and consumer contract is verified. Work upward from
the oldest available candidate instead of starting at the requested version.
Verify all older tags are incompatible or intentionally unsupported so a lower
compatible version is not missed. Add a new threshold when a source-backed
incompatibility begins; do not split ranges merely because a dependency version
changed if the active Formula can discover it from the selected tag.

A Formula is a reusable build script for its selected range, not an
exact-version recipe. LLAR uses the requested version to check out source and
to pick `max(fromVer <= requested version)`. After that selection, Formula
code must not read, compare, or branch on the requested version string or VCS
ref. Do not use `target.version` even when an older LLAR revision exposed it.
If one Formula cannot build every version in its range without that string,
its `fromVer` range is wrong: add another threshold. A one-release exception
needs both the exceptional `fromVer` and the restored later `fromVer`; it
does not justify an exact-version conditional.

Add a comparator only when the store's active default comparator misorders the
real tag set. Before writing one, check every upstream version LLAR may select,
not only the requested version or Formula thresholds. Every tag must conform to
the comparator's input rules and the complete set must have the intended total
order. In particular, use a semantic-version comparator only when every tag is
valid semantic-version syntax.

In llarhub, start every `_llar.gox` filename stem with a lowercase ASCII letter.
For example, use `picobench_llar.gox`, not `Picobench_llar.gox`; do not present
this convention as an LLAR parser limit.

## Upstream Investigation

Inspect the exact source revision before editing. Read:

- the build-system entrypoint and install rules;
- dependency manifests, lock files, and vendored-subproject declarations;
- build options and their defaults;
- package metadata generated or installed upstream;
- source patches and exported files used by the reference package recipe;
- upstream or package-recipe consumer tests.

Translate package semantics, not another package manager's implementation.
Exclude its cache layout, generated toolchains, cleanup conventions, generic
compatibility code, and defensive flags unless the selected upstream revision
requires equivalent behavior.

## Dependencies

Declare direct dependencies only. Map each upstream dependency name to an LLAR
module id using verified repository evidence.

When dependency versions vary inside a Formula range and upstream records them
in a stable machine-readable source file, discover them from the requested
source tree through the active Formula dependency hook. Parse structured
formats with a structured parser. The extraction algorithm stays the same
across the range; only the data in each checkout may change. Do not use the
requested version string to choose among discovery procedures.

Resolve source ownership separately for every hook. Do not assume a project
filesystem exposed during dependency discovery refers to the same source as a
build context. During build or test, use the upstream source directory exposed
by the active context unless that LLAR revision proves another API owns it.

Use `versions.json` according to the selected LLAR loader's actual reconciliation
rules. Treat static dependency entries as verified conservative data, not as a
place to copy the newest known dependency versions. Never add fallback data for
a failure mode that has not been shown to occur.

## Matrix, Target, And Options

Expose a dimension only when it changes dependencies, commands, installed
output, metadata, tests, or supported platforms.

- Environment-owned requirements belong to the target requirements surface.
- Package-owned build choices belong to the target options surface.
- Defaults choose option values; they do not by themselves define every legal
  value.
- Reject a selection only when the selected upstream revision proves it is
  unsupported.

Keep options independent. Combine values that represent one indivisible choice
instead of creating invalid cartesian combinations.

Read the selected target from Formula `target`, not from the host process:

- `target.require["os"]` and `target.require["arch"]` are the selected
  platform. Use the selected value, typically `target.require["os"][0]`, or a
  membership test such as `slices.contains(target.require["os"], "linux")`.
- Do not import `"runtime"` or use `runtime.GOOS` / `runtime.GOARCH`. Those
  report the interpreter host, which is wrong under cross-compile and when
  CI injects `--os` / `--arch`.
- Do not write `osName := runtime.GOOS` and then overwrite it from
  `target.require["os"]`. Do not fall back to the host when require is
  empty; missing require is a matrix contract failure.
- `target.options["name"]` is the selected option values. Read the active
  choice as `target.options["name"][0]`. Do not treat
  `slices.contains(target.options["name"], "ON")` as the selected value when
  the slice can hold both a default and an override.

`filter` may inspect `target.require` and `target.options` to reject
unsupported matrix selections. It must not inspect the requested version.

## Build And Install

Use the LLAR helper matching upstream when it owns the required flow. Configure
only source-backed flags. Do not force a generator, build type, linkage mode,
toolchain, policy version, test toggle, or optional feature because another
Formula used it.

Choose the helper from the selected upstream revision:

- CMake when the source has `CMakeLists.txt`.
- Autotools when the source has `configure` and/or a Makefile, including
  Makefile-only trees. Autotools `build`/`install` run `make`/`make install`
  through execbroker.

Do not invent a `CMakeLists.txt` for a Makefile project. Do not call `cc`,
`c++`, `gcc`, `clang`, `ar`, `ld`, or equivalent compilers and archivers from
`onBuild`. Those helpers are the injection point for later LLAR cross-compile
toolchains and sysroots; a host compiler invoked from the Formula skips that
path.

Keep the Autotools helper for Makefile projects even when the default target
is a test program or there is no `install` rule. Do not invent a replacement
Makefile. Call `a.build` with the existing Makefile's compile targets and
make variables such as `CFLAGS=`; skip `a.configure` when there is no
configure script. If there is no `install` target, copy the files make
produced into the output directory instead of calling `a.install`.

`onTest` may compile a consumer with `cc!` or `c++!` against the installed
pkg-config flags. That is the only Formula hook where a naked compiler is
allowed.

Expose dependency install roots through the active helper or context contract.
Install the complete public result into the current module's output directory.
Do not depend on a build scratch path after the build callback returns.

For an unsupported build system that is not CMake or Make, use gsh commands
with separate arguments and explicit failure propagation. Still do not invoke
a C/C++ compiler from `onBuild` when a Makefile plus the Autotools helper can
own the compile. Call active CMake or Autotools helper methods according to
their real return contract; do not add `!` to a void helper that already
panics on failure. Add a shell only when the verified upstream step requires
shell grammar.

## Metadata

Metadata describes how an installed consumer uses the package. Derive it from
installed headers, libraries, tools, CMake package files, pkg-config files, or
another verified public interface.

Prefer valid installed package metadata. Otherwise construct only the flags
proved by a consumer compile/link/load/run check. Include dependency metadata
only when the package's public interface requires it, preserving verified link
ordering.

For C/C++ library metadata exposed through pkg-config, install a valid `.pc`
file under `lib/pkgconfig` in the output. Derive its name, version, libraries,
private dependencies, and flags from the selected upstream source and the
actual installed interface; do not invent missing fields. Make the file
relocatable, for example by deriving its prefix from `${pcfiledir}` instead of
embedding the build or installation path.

Prefer a valid `.pc` file installed by the upstream build. When a Formula must
create the file itself and the resolved LLAR revision provides
`pkgconfig.new`, use that writer. It supplies relocatable defaults for
`prefix`, `exec_prefix`, `libdir`, and `includedir`. The Formula provides
`name`, `description`, `version`, optional `url` / `requires` / `libs` /
`cflags`, and optional private or shared fragments, then writes the file and
runs the lookup:

```xgo
pcDir := filepath.join(installDir, "lib", "pkgconfig")
os.mkdirAll(pcDir, 0o755)!

pc := pkgconfig.new(
    name = "foo",
    description = "Foo library",
    version = "1.2.0",
    requires = ["zlib >= 1.2.0"],
    libs = ["-L$${libdir}", "-lfoo"],
    cflags = ["-I$${includedir}"],
)!
pc.libs.private ["-lm"]
pc.cflags.private ["-DFOO_STATIC"]

out := os.create(filepath.join(pcDir, "foo.pc"))!
pc.writeTo(out)!
out.close()!

pkgconfig.use installDir
ctx.setMetadata pkgconfig.lookup("foo")!
```

`pkgconfig.new` requires `name`, `description`, and `version`. `requires`
entries are joined with `, `. Public `Libs` and `Cflags` are always emitted,
including when empty. `pc.libs.private`, `pc.libs.shared`,
`pc.cflags.private`, and `pc.cflags.shared` replace those fragment lists and
are omitted from the file when empty. `${pc.libs}` and `${pc.cflags}` are the
public fragments. Variable references in Formula source use `$${libdir}` so
the `.pc` file contains `${libdir}`. The Formula still owns the output
directory, filename, writer lifetime, and the final `pkgconfig.lookup`. Do
not hand-write the default variables or the property layout when this writer
is available. Parsing, patching, or relocating an upstream-provided `.pc`
file remains a separate problem; keep a valid installed file when upstream
already writes one.

A Formula-authored `.pc` `Version` or `Requires` constraint is part of that
Formula's stable output. Use the Formula's `fromVer` floor, not the exact
requested tag. If that pkg-config content must change at a later release, add
another Formula threshold.

Resolve the pkg-config helper API from the target LLAR revision. When that
revision provides `pkgconfig.use` and `pkgconfig.lookup`, call
`pkgconfig.use installDir` before `pkgconfig.lookup(name)!`. The lookup must run
the equivalent of `pkg-config --cflags --libs` and its complete result must be
the Formula metadata. Do not replace it with a handwritten include, library,
or linker fragment, and do not use a libs-only helper. Apply the same full
query to a header-only library even though its libs portion is empty.

Do not copy a package manager's `package_info` declaration without comparing it
to the actual installed result.

## Conan Flag Audit

This comparison is done by the **author** (the AI writing the Formula) after
the Formula exists. It is not a test script, not CI string-matching, and not
something `onTest` encodes as `panic` on lookup tokens.

When a
[Conan Center](https://github.com/conan-io/conan-center-index) recipe exists
for the same package, read the live `recipes/<name>/all/conanfile.py` and
diff `package_info()` against the published `.pc` `Libs`/`Cflags`. Fix the
`.pc` when the review finds a real miss. Do not put Conan tokens into the
consumer (`panic` if lookup lacks `-lm`, require `-lglm` while shipping
header-only, and similar). Library coverage belongs in **Consumer Test**.

Baseline: linux amd64 static Release, default options, plus every retained
option that changes exported flags (`shared`, `nothreads`, components).

| Conan `package_info()` | LLAR `.pc` |
|---|---|
| `cpp_info.libs` / component `libs` | `Libs:` `-l…` |
| `system_libs` | extra `Libs:` `-l…` (linux typically `m`, `pthread`, `rt`, `anl`, `dl`) |
| `defines` | `Cflags:` `-D…` |
| extra `includedirs` beyond `include` | extra `Cflags:` `-I…` |
| `pkg_config_name` / components | `.pc` filename, `Name:`, `Requires:` |

Missing Conan exports are a Formula defect. Extra LLAR flags are not
automatically wrong, but record them. Confirm the installed archive and
headers exist.

`pkgconfig.lookup` does not pass `--static`. Put unix system libs the consumer
needs on public `Libs`, not only `Libs.private`.

## Consumer Test

Base `onTest` on upstream or reference-package consumer behavior. Build against
the installed output and declared dependencies, then run or load the produced
artifact when the platform supports it.

When the Formula publishes pkg-config metadata, require the installed `.pc`
file and compile/link the consumer with the complete flags returned by the
verified cflags-and-libs lookup. A test that reconstructs `-I`, `-L`, or `-l`
flags independently does not validate the published metadata.

`onTest` must **cover the installed library**, not smoke one constructor and
not assert Conan flag tokens. A green `new`/`free`/`version` consumer is how
missing system libs, extra include dirs, contrib archives, and unused public
headers escape CI. Align tests to the library, not to Conan.

Cover:

- Every published `.pc` / component / extra archive the Formula installs
  (`libhwy` and `libhwy-contrib`, not only the primary name).
- The include forms a real consumer would write from the installed tree
  (`<json.h>` when headers also live in `include/json-c`;
  `<libdxfrw/libdxfrw.h>` when headers live under `include/libdxfrw`).
- Enough of the public API that the test pulls the objects a user would:
  encode/decode, parse, evaluate, decode a frame, open a socket — not only
  allocate a handle. Prefer the upstream `test_package` or examples as the
  floor, then add the rest of the shipped interface they skip.
- Compile/link with **only** `pkgconfig.lookup` flags. Do not add `-lm`,
  `-lpthread`, `-lstdc++`, or extra `-I` on the test line.
- Do not `panic` on lookup text. If a flag is missing, a thorough consumer
  fails at compile or link. If it still would not, deepen the consumer.
  Conan `package_info()` mismatches that a full-library test cannot see
  belong in the author review, not in `onTest`.

Use a test build tree distinct from the build callback's scratch tree. A cached
artifact may skip the build callback while still running the consumer test.
Tests must not repair or mutate cached metadata.

## Validation Range

Test the exact requested tag, the lowest compatible `fromVer`, the immediately
preceding incompatible or unsupported version, every changed threshold,
representative tags where dependencies or build files differ, defaults, and
each retained output-changing option. When a comparator exists, validate it
against the complete upstream tag set. Use every required environment dimension
when the CLI does not merge host defaults into an explicit matrix.

Re-run a supported selection to exercise cache-hit consumer behavior. Inspect
the installed files and consumer result; parsing and build completion alone are
not sufficient.
