#!/usr/bin/env python3
"""Launch unattended Unreal tests inside Echoes' macOS save sandbox.

The launcher never inventories, hashes, opens, or otherwise probes a real
player-save directory.  Its access claim is limited to the `sandbox-exec`
deny rules emitted below and a synthetic-denial probe against caller-created
fixtures.  The Unreal bootstrap validates routing configuration; it cannot
attest that an arbitrary manually launched process has an OS sandbox.
"""

from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import shutil
import signal
import subprocess
import sys
import tempfile
from typing import Iterable


MANIFEST_FORMAT = "EchoesTestSandbox/v1"


def _resolved(path: Path) -> Path:
    """Resolve lexical aliases without following a non-existent leaf."""
    return path.expanduser().resolve(strict=False)


def _lexical_absolute(path: Path) -> Path:
    """Make an absolute policy path without probing the target directory."""
    return Path(os.path.abspath(os.fspath(path)))


def _is_descendant(candidate: Path, parent: Path) -> bool:
    try:
        candidate.relative_to(parent)
        return True
    except ValueError:
        return False


def _lisp_string(value: str) -> str:
    if "\x00" in value or "\n" in value or "\r" in value:
        raise ValueError("sandbox paths may not contain NUL or line breaks")
    return '"' + value.replace("\\", "\\\\").replace('"', '\\"') + '"'


def build_sandbox_profile(
    protected_roots: Iterable[Path], *, resolve_paths: bool = True
) -> str:
    """Build the targeted macOS policy used for real test launches."""
    lines = ["(version 1)", "(allow default)"]
    for protected_root in protected_roots:
        target = _resolved(protected_root) if resolve_paths else protected_root
        escaped = _lisp_string(str(target))
        lines.append(f"(deny file-read* (subpath {escaped}))")
        lines.append(f"(deny file-write* (subpath {escaped}))")
    return "\n".join(lines) + "\n"


def assert_targeted_deny_clauses(
    profile: str, protected_roots: Iterable[Path]
) -> None:
    """Check exact deny clauses without accessing any protected target."""
    for protected_root in protected_roots:
        escaped = _lisp_string(str(protected_root))
        for operation in ("file-read*", "file-write*"):
            clause = f"(deny {operation} (subpath {escaped}))"
            if clause not in profile:
                raise ValueError(f"missing targeted sandbox deny clause: {operation}")


def build_manifest(root: Path, save_dir: Path, user_dir: Path) -> dict[str, str]:
    root = _resolved(root)
    save_dir = _resolved(save_dir)
    user_dir = _resolved(user_dir)
    if not _is_descendant(save_dir, root) or not _is_descendant(user_dir, root):
        raise ValueError("test routes must remain inside the sandbox root")
    return {
        "format": MANIFEST_FORMAT,
        "root": str(root),
        "save_dir": str(save_dir),
        "user_dir": str(user_dir),
    }


def validate_manifest_config(
    manifest: dict[str, object], root: Path, save_dir: Path, user_dir: Path
) -> None:
    """Validate exactly the configuration later checked by Unreal bootstrap."""
    expected = build_manifest(root, save_dir, user_dir)
    if manifest != expected:
        raise ValueError("sandbox manifest does not exactly match its launch routes")


def run_synthetic_denial_probe(sandbox_exec: str, profile: Path, root: Path) -> None:
    """Prove the emitted policy denies one synthetic protected descendant."""
    protected = root / "synthetic player saves"
    writable = root / "writable"
    protected.mkdir()
    writable.mkdir()
    sentinel = protected / "sentinel.bin"
    sentinel.write_bytes(b"synthetic-only")
    code = (
        "from pathlib import Path; import sys; "
        "protected=Path(sys.argv[1]); writable=Path(sys.argv[2]); "
        "denied=False; "
        "\ntry:\n protected.read_bytes()\nexcept PermissionError:\n denied=True\n"
        "\nif not denied: raise SystemExit(31)\n"
        "denied=False\n"
        "try:\n protected.write_bytes(b'forbidden overwrite')\nexcept PermissionError:\n denied=True\n"
        "if not denied: raise SystemExit(32)\n"
        "denied=False\n"
        "try:\n (protected.parent / 'forbidden new file').write_bytes(b'forbidden create')\nexcept PermissionError:\n denied=True\n"
        "if not denied: raise SystemExit(33)\n"
        "(writable / 'probe.txt').write_text('ok', encoding='utf-8')"
    )
    completed = subprocess.run(
        [sandbox_exec, "-f", str(profile), sys.executable, "-c", code,
         str(sentinel), str(writable)],
        check=False,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    if completed.returncode != 0:
        raise RuntimeError(
            "sandbox-exec synthetic denial probe failed "
            f"({completed.returncode}): {completed.stderr.strip()}"
        )
    if (writable / "probe.txt").read_text(encoding="utf-8") != "ok":
        raise RuntimeError("sandbox-exec synthetic writable probe did not persist")


def build_editor_command(
    sandbox_exec: str,
    profile: Path,
    editor: Path,
    project: Path,
    manifest_path: Path,
    save_dir: Path,
    user_dir: Path,
    local_cache_dir: Path,
    editor_args: Iterable[str],
) -> list[str]:
    """Return argv without shell parsing so route values cannot inject flags."""
    editor_args = list(editor_args)
    protected_options = (
        "-echoestestsandbox",
        "-echoestestsandboxmanifest",
        "-echoessavegamedirectory",
        "-userdir",
        "-ddc",
        "-localdatacachepath",
        "-notraceserver",
        "-traceautostart",
        "-cvarsini",
    )
    for argument in editor_args:
        normalized = argument.casefold()
        if any(
            normalized == option or normalized.startswith(option + "=")
            for option in protected_options
        ):
            raise ValueError(
                "caller may not override sandbox routing or DDC arguments")
    return [
        sandbox_exec, "-f", str(profile), str(editor), str(project),
        "-EchoesTestSandbox",
        f"-EchoesTestSandboxManifest={manifest_path}",
        f"-EchoesSaveGameDirectory={save_dir}",
        f"-UserDir={user_dir}",
        "-DDC=(InstalledEnginePak,Local)",
        f"-LocalDataCachePath={local_cache_dir}",
        "-notraceserver",
        "-traceautostart=0",
        f"-cvarsini={manifest_path.parent / 'ConsoleVariables.ini'}",
        *editor_args,
    ]


def _write_result(report_dir: Path, result: dict[str, object]) -> None:
    destination = report_dir / "SaveIsolation"
    destination.mkdir(parents=True, exist_ok=True)
    (destination / "launcher-result.json").write_text(
        json.dumps(result, sort_keys=True, indent=2) + "\n", encoding="utf-8"
    )


def run_editor_with_timeout(
    command: list[str], report_dir: Path, timeout_seconds: int
) -> tuple[int, bool, Path]:
    """Run the editor in its own process group and retain its launch log."""
    isolation_dir = report_dir / "SaveIsolation"
    isolation_dir.mkdir(parents=True, exist_ok=True)
    log_path = isolation_dir / "editor-launch.log"
    with log_path.open("w", encoding="utf-8") as log:
        process = subprocess.Popen(
            command,
            stdout=log,
            stderr=subprocess.STDOUT,
            start_new_session=True,
        )
        try:
            return process.wait(timeout=timeout_seconds), False, log_path
        except subprocess.TimeoutExpired:
            os.killpg(process.pid, signal.SIGTERM)
            try:
                process.wait(timeout=10)
            except subprocess.TimeoutExpired:
                os.killpg(process.pid, signal.SIGKILL)
                process.wait()
            return 124, True, log_path


def launch(args: argparse.Namespace) -> int:
    if sys.platform != "darwin":
        raise RuntimeError("macOS save isolation requires sandbox-exec; no fallback is permitted")
    sandbox_exec = shutil.which("sandbox-exec")
    if sandbox_exec is None:
        raise RuntimeError("sandbox-exec is unavailable; refusing to launch unisolated tests")

    editor = _resolved(Path(args.editor))
    project = _resolved(Path(args.project))
    report_dir = _resolved(Path(args.report_dir))
    if not editor.is_file() or not os.access(editor, os.X_OK):
        raise RuntimeError(f"Unreal Editor is not executable: {editor}")
    if not project.is_file():
        raise RuntimeError(f"Unreal project is absent: {project}")

    # Do not let TMPDIR redirect test creation into a player-data location.
    # This is the OS-owned per-user temporary hierarchy also used by UE on macOS.
    temporary_root = subprocess.check_output(
        ["/usr/bin/getconf", "DARWIN_USER_TEMP_DIR"], text=True).strip()
    if not temporary_root.startswith("/var/folders/") and not temporary_root.startswith("/private/var/folders/"):
        raise RuntimeError("macOS user temporary directory is outside the supported hierarchy")
    sandbox_root = Path(tempfile.mkdtemp(
        prefix="EchoesAutomationSuite.", dir=temporary_root))
    save_dir = sandbox_root / "SaveGames"
    user_dir = sandbox_root / "UserDir"
    local_cache_dir = sandbox_root / "DerivedDataCache"
    save_dir.mkdir()
    user_dir.mkdir()
    local_cache_dir.mkdir()
    manifest_path = sandbox_root / "launch-manifest.json"
    # UE reads this Startup section before MainFrame creates the Home Screen.
    # Its recent-project browser otherwise opens an installation-registry dialog
    # under home denial, before ExecCmds or the automation worker can run.
    startup_cvars = "[Startup]\nHomeScreen.EnableHomeScreen=0\n"
    (sandbox_root / "ConsoleVariables.ini").write_text(startup_cvars, encoding="utf-8")
    manifest = build_manifest(sandbox_root, save_dir, user_dir)
    manifest_path.write_text(
        json.dumps(manifest, sort_keys=True, indent=2) + "\n", encoding="utf-8"
    )

    # This is a protected target, never an input to resolve/stat. The project
    # file itself was already checked above; its hypothetical Saved/SaveGames
    # descendant must remain entirely untouched by this launcher.
    project_save_games = _lexical_absolute(project.parent / "Saved" / "SaveGames")
    home_value = os.environ.get("HOME")
    if not home_value:
        raise RuntimeError("HOME is unavailable; refusing to infer a player-data deny path")
    # Keep the home deny target lexical: this launcher must not stat, enumerate,
    # hash, or otherwise access the real player's home directory to protect it.
    home_directory = _lexical_absolute(Path(home_value))
    profile_path = sandbox_root / "sandbox.sb"
    # This fixture is created only after the policy is written and exists only
    # to prove the actual launch profile denies a protected descendant.
    synthetic_protected = _resolved(sandbox_root) / "synthetic player saves"
    protected_roots = (project_save_games, home_directory, synthetic_protected)
    profile = build_sandbox_profile(protected_roots, resolve_paths=False)
    assert_targeted_deny_clauses(profile, protected_roots)
    profile_path.write_text(profile, encoding="utf-8")
    isolation_evidence = report_dir / "SaveIsolation"
    isolation_evidence.mkdir(parents=True, exist_ok=True)
    (isolation_evidence / "sandbox-policy.sb").write_text(profile, encoding="utf-8")
    (isolation_evidence / "launch-manifest.json").write_text(
        json.dumps(manifest, sort_keys=True, indent=2) + "\n", encoding="utf-8")
    (isolation_evidence / "ConsoleVariables.ini").write_text(startup_cvars, encoding="utf-8")

    result: dict[str, object] = {
        "format": MANIFEST_FORMAT,
        "os_boundary": "sandbox-exec targeted deny policy",
        "protected_paths_configured": [str(project_save_games), str(home_directory)],
        "protected_policy_clauses_verified": True,
        "user_dir_note": (
            "UE 5.8 -UserDir routes FPaths::ProjectUserDir; it does not "
            "redirect EngineUserDir, FPlatformProcess::UserDir, or macOS "
            "ApplicationSettingsDir. Whole-home denial remains fail-closed; "
            "editor startup feasibility is unverified until an isolated launch."
        ),
        "synthetic_denial_probe": False,
        "scoped_save_directory_empty_after_run": False,
        "cleanup_succeeded": False,
        "prelaunch_failure": False,
    }
    exit_status = 9
    try:
        run_synthetic_denial_probe(sandbox_exec, profile_path, sandbox_root)
        result["synthetic_denial_probe"] = True
        command = build_editor_command(
            sandbox_exec, profile_path, editor, project, manifest_path,
            save_dir, user_dir, local_cache_dir, args.editor_args)
        editor_status, timed_out, log_path = run_editor_with_timeout(
            command, report_dir, args.timeout_seconds)
        result["editor_exit_status"] = editor_status
        result["editor_timed_out"] = timed_out
        result["editor_log"] = str(log_path)
        result["scoped_save_directory_empty_after_run"] = not any(save_dir.iterdir())
        if not result["scoped_save_directory_empty_after_run"]:
            exit_status = 8
        else:
            exit_status = editor_status
    except (OSError, RuntimeError, ValueError) as error:
        result["prelaunch_failure"] = True
        result["launch_error"] = str(error)
        exit_status = 9
    finally:
        try:
            shutil.rmtree(sandbox_root)
            result["cleanup_succeeded"] = True
        except OSError as error:
            result["cleanup_error"] = str(error)
    _write_result(report_dir, result)
    return exit_status


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--editor", required=True)
    parser.add_argument("--project", required=True)
    parser.add_argument("--report-dir", required=True)
    parser.add_argument("--timeout-seconds", type=int, default=600)
    parser.add_argument("editor_args", nargs=argparse.REMAINDER)
    arguments = parser.parse_args()
    if arguments.editor_args[:1] == ["--"]:
        arguments.editor_args = arguments.editor_args[1:]
    if arguments.timeout_seconds <= 0:
        parser.error("--timeout-seconds must be positive")
    try:
        return launch(arguments)
    except (OSError, RuntimeError, ValueError) as error:
        print(f"[ECHOES_TEST_SANDBOX_LAUNCH_FAILED] {error}", file=sys.stderr)
        return 9


if __name__ == "__main__":
    raise SystemExit(main())
