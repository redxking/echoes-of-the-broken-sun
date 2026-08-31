#!/usr/bin/env python3

from __future__ import annotations

import argparse
import ctypes
import datetime
import errno
import hashlib
import importlib.util
import json
import os
import pathlib
import re
import signal
import stat
import sys
from typing import Callable, Iterable, Optional


SUMMARY_NAME = "packaged_sustained_soak_summary.json"
COMPLETION_NAME = "qualification_completion.json"
MANIFEST_NAME = "sustained_evidence.sha256"
ABORT_NAME = "qualification_abort.txt"
PREFLIGHT_BINDING_NAME = "preflight_binding.json"
PREFLIGHT_SNAPSHOT_NAME = "preflight_evidence_snapshot.zip"
PREFLIGHT_VERIFIER_NAME = "validate_sustained_preflight.used.py"
MANIFEST_RECORD = re.compile(r"^([0-9a-f]{64})  ([^/\x00]+)$")
SHA256 = re.compile(r"^[0-9a-f]{64}$")
PREFLIGHT_IDENTITY_FIELDS = (
    "artifact",
    "configuration",
    "platform",
    "package_version",
    "source_commit",
    "manifest_sha256",
    "normal_startup_smoke_sha256",
    "legacy_stress_startup_smoke_sha256",
    "runner_sha256",
    "validator_sha256",
    "packager_sha256",
    "package_verifier_sha256",
    "evidence_finalizer_sha256",
    "preflight_verifier_sha256",
)
RETAINED_TOOL_FILES = {
    "runner_sha256": "soak_packaged_sustained_macos.used.sh",
    "validator_sha256": "validate_sustained_soak_log.used.py",
    "packager_sha256": "package_macos.used.sh",
    "package_verifier_sha256": "verify_packaged_app.used.py",
    "evidence_finalizer_sha256": "finalize_sustained_evidence.used.py",
    "preflight_verifier_sha256": PREFLIGHT_VERIFIER_NAME,
}


class FinalizationError(RuntimeError):
    pass


def _digest(path: pathlib.Path) -> str:
    value = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            value.update(chunk)
    return value.hexdigest()


def _write_bytes_atomic(path: pathlib.Path, payload: bytes) -> None:
    temporary = path.with_name(f".{path.name}.tmp-{os.getpid()}")
    try:
        with temporary.open("xb") as handle:
            handle.write(payload)
            handle.flush()
            os.fsync(handle.fileno())
        os.replace(temporary, path)
    finally:
        try:
            temporary.unlink()
        except FileNotFoundError:
            pass


def _write_json_atomic(path: pathlib.Path, value: object) -> None:
    payload = (json.dumps(value, indent=2, sort_keys=True) + "\n").encode("utf-8")
    _write_bytes_atomic(path, payload)


def _write_abort_provenance(
    path: pathlib.Path,
    *,
    reason: str,
    detail: str,
    game_pid: int,
    game_pgid: int,
    elapsed_seconds: int,
    next_sample_boundary_seconds: int,
    wrapper_exit_code: int,
    cleanup_signal: str,
    termination_signal: str,
    wrapper_pid: int | None = None,
    wrapper_pgid: int | None = None,
) -> None:
    path = path.absolute()
    if path.name != ABORT_NAME or not path.parent.is_dir() or path.parent.is_symlink():
        raise FinalizationError("Abort provenance path is missing or unsafe")
    if wrapper_pid is None:
        wrapper_pid = os.getpid()
    if wrapper_pgid is None:
        wrapper_pgid = os.getpgid(0)
    aborted_utc = datetime.datetime.now(datetime.timezone.utc).strftime(
        "%Y%m%dT%H%M%SZ"
    )
    fields = {
        "aborted_utc": aborted_utc,
        "reason": reason,
        "detail": detail,
        "wrapper_pid": str(wrapper_pid),
        "wrapper_pgid": str(wrapper_pgid),
        "game_pid": str(game_pid),
        "game_pgid": str(game_pgid),
        "elapsed_seconds": str(elapsed_seconds),
        "next_sample_boundary_seconds": str(next_sample_boundary_seconds),
        "wrapper_status": "failed",
        "wrapper_exit_code": str(wrapper_exit_code),
        "cleanup_signal": cleanup_signal,
        "termination_signal": termination_signal,
    }
    for key, value in fields.items():
        if not value or "\n" in value or "\r" in value:
            raise FinalizationError(f"Invalid abort-provenance value: {key}")
    if re.fullmatch(r"[a-z0-9_]+", reason) is None:
        raise FinalizationError("Abort-provenance reason is invalid")
    if min(wrapper_pid, wrapper_pgid, game_pid, game_pgid) <= 0:
        raise FinalizationError("Abort-provenance process identity is invalid")
    if elapsed_seconds < -1 or next_sample_boundary_seconds < -1:
        raise FinalizationError("Abort-provenance boundary is invalid")
    if not 1 <= wrapper_exit_code <= 255:
        raise FinalizationError("Abort-provenance exit code is invalid")
    if cleanup_signal not in {"none", "TERM", "TERM+KILL"}:
        raise FinalizationError("Abort-provenance cleanup signal is invalid")
    if termination_signal not in {"none", "INT", "TERM", "HUP", "QUIT"}:
        raise FinalizationError("Abort-provenance termination signal is invalid")
    _write_bytes_atomic(
        path,
        "".join(f"{key}={value}\n" for key, value in fields.items()).encode(
            "utf-8"
        ),
    )


def _validate_evidence_names(names: Iterable[str]) -> list[str]:
    result = list(names)
    if not result or len(set(result)) != len(result):
        raise FinalizationError("Evidence file names must be present and unique")
    for name in result:
        candidate = pathlib.PurePath(name)
        if not name or candidate.name != name or name in {COMPLETION_NAME, MANIFEST_NAME}:
            raise FinalizationError(f"Unsafe evidence file name: {name!r}")
    if SUMMARY_NAME not in result:
        raise FinalizationError(f"Evidence set must include {SUMMARY_NAME}")
    return result


def verify_published_evidence(directory: pathlib.Path) -> dict[str, object]:
    directory = directory.absolute()
    if not directory.is_dir() or directory.is_symlink():
        raise FinalizationError("Published evidence directory is missing or unsafe")

    top_files: set[str] = set()
    top_directories: set[str] = set()
    for candidate in directory.iterdir():
        if candidate.is_symlink():
            raise FinalizationError(f"Published evidence contains a symlink: {candidate.name}")
        mode = candidate.stat().st_mode
        if stat.S_ISREG(mode):
            top_files.add(candidate.name)
        elif stat.S_ISDIR(mode):
            top_directories.add(candidate.name)
        else:
            raise FinalizationError(
                f"Published evidence contains a special entry: {candidate.name}"
            )
    if not top_directories.issubset({"runtime-state"}):
        raise FinalizationError("Published evidence contains an unexpected directory")
    runtime_state = directory / "runtime-state"
    if runtime_state.is_dir():
        for root, directory_names, file_names in os.walk(runtime_state, followlinks=False):
            for name in directory_names + file_names:
                candidate = pathlib.Path(root) / name
                if candidate.is_symlink():
                    raise FinalizationError("Runtime-state evidence contains a symlink")
                mode = candidate.stat().st_mode
                if not (stat.S_ISREG(mode) or stat.S_ISDIR(mode)):
                    raise FinalizationError("Runtime-state evidence contains a special entry")

    manifest_path = directory / MANIFEST_NAME
    if MANIFEST_NAME not in top_files:
        raise FinalizationError("Published evidence manifest is missing")
    try:
        manifest_lines = manifest_path.read_text(encoding="utf-8").splitlines()
    except (OSError, UnicodeError) as error:
        raise FinalizationError(f"Could not read evidence manifest: {error}") from error
    records: dict[str, str] = {}
    for line in manifest_lines:
        match = MANIFEST_RECORD.fullmatch(line)
        if match is None:
            raise FinalizationError("Published evidence manifest is malformed")
        digest, name = match.groups()
        if name in records or name in {".", "..", MANIFEST_NAME}:
            raise FinalizationError("Published evidence manifest has an unsafe duplicate")
        records[name] = digest
    if set(records) != top_files - {MANIFEST_NAME}:
        raise FinalizationError("Published evidence manifest file set is not exact")
    for name, expected in records.items():
        if _digest(directory / name) != expected:
            raise FinalizationError(f"Published evidence digest mismatch: {name}")

    try:
        summary = json.loads((directory / SUMMARY_NAME).read_text(encoding="utf-8"))
        completion = json.loads(
            (directory / COMPLETION_NAME).read_text(encoding="utf-8")
        )
    except (OSError, UnicodeError, json.JSONDecodeError) as error:
        raise FinalizationError(f"Published evidence metadata is unreadable: {error}") from error
    if not isinstance(summary, dict) or not isinstance(completion, dict):
        raise FinalizationError("Published evidence metadata is not object-shaped")
    if completion.get("status") != "complete":
        raise FinalizationError("Published evidence lacks a complete transaction record")
    for key in (
        "all_measured_budgets_pass",
        "preflight_evidence_directory",
        "preflight_evidence_manifest_sha256",
        "preflight_snapshot_sha256",
        "qualified_one_hour",
        "run_class",
        "runtime_contract_qualified_one_hour",
        "source_commit",
    ):
        if completion.get(key) != summary.get(key):
            raise FinalizationError(f"Summary and completion record disagree: {key}")
    if summary.get("qualified_one_hour") is True:
        _verify_static_one_hour_binding(directory, summary)
    return {
        "completion": completion,
        "directory_names": sorted(top_directories),
        "file_names": sorted(top_files),
        "manifest_sha256": _digest(manifest_path),
        "summary": summary,
    }


def _fsync_directory(path: pathlib.Path) -> None:
    descriptor = os.open(str(path), os.O_RDONLY)
    try:
        os.fsync(descriptor)
    finally:
        os.close(descriptor)


def _rename_directory_no_replace(
    staging_directory: pathlib.Path, final_directory: pathlib.Path
) -> None:
    if sys.platform != "darwin":
        raise FinalizationError(
            "Atomic no-replace evidence publication requires the supported macOS host"
        )
    renameatx_np = ctypes.CDLL(None, use_errno=True).renameatx_np
    renameatx_np.argtypes = (
        ctypes.c_int,
        ctypes.c_char_p,
        ctypes.c_int,
        ctypes.c_char_p,
        ctypes.c_uint,
    )
    renameatx_np.restype = ctypes.c_int
    at_fdcwd = -2
    rename_exclusive = 0x00000004
    if renameatx_np(
        at_fdcwd,
        os.fsencode(staging_directory),
        at_fdcwd,
        os.fsencode(final_directory),
        rename_exclusive,
    ) != 0:
        error_number = ctypes.get_errno()
        if error_number == errno.EEXIST:
            raise FinalizationError("Final evidence path appeared before publication")
        raise OSError(error_number, os.strerror(error_number), str(final_directory))


def _terminal_success_output(final_directory: pathlib.Path, run_class: object) -> bytes:
    if run_class == "one_hour_qualification":
        headline = "Bounded one-hour packaged sustained qualification passed."
    elif run_class == "ten_minute_preflight":
        headline = (
            "Ten-minute packaged sustained preflight passed; "
            "this is not one-hour qualification."
        )
    else:
        headline = "Packaged sustained diagnostic passed; this is not one-hour qualification."
    lines = (
        headline,
        f"Summary: {final_directory / SUMMARY_NAME}",
        f"Evidence SHA-256 manifest: {final_directory / MANIFEST_NAME}",
        f"Atomic completion record: {final_directory / COMPLETION_NAME}",
    )
    return ("\n".join(lines) + "\n").encode("utf-8")


def _load_trusted_preflight_verifier(
    directory: pathlib.Path,
    verifier_path: pathlib.Path,
    expected_digest: str,
) -> object:
    verifier_path = verifier_path.absolute()
    if not verifier_path.is_file() or verifier_path.is_symlink():
        raise FinalizationError("Trusted preflight verifier is missing or unsafe")
    try:
        verifier_path.resolve().relative_to(directory.resolve())
    except ValueError:
        pass
    else:
        raise FinalizationError("Trusted preflight verifier cannot come from evidence")
    if _digest(verifier_path) != expected_digest:
        raise FinalizationError("Trusted preflight verifier digest differs")
    specification = importlib.util.spec_from_file_location(
        f"_echoes_trusted_preflight_verifier_{os.getpid()}_{id(directory)}",
        verifier_path,
    )
    if specification is None or specification.loader is None:
        raise FinalizationError("Could not load the trusted preflight verifier")
    module = importlib.util.module_from_spec(specification)
    try:
        specification.loader.exec_module(module)
    except Exception as error:
        raise FinalizationError(
            f"Could not initialize the trusted preflight verifier: {error}"
        ) from error
    return module


def _verify_static_one_hour_binding(
    directory: pathlib.Path,
    summary: dict[str, object],
) -> tuple[
    dict[str, object],
    dict[str, object],
    pathlib.Path,
    str,
]:
    binding_path = directory / PREFLIGHT_BINDING_NAME
    snapshot_path = directory / PREFLIGHT_SNAPSHOT_NAME
    if not binding_path.is_file() or binding_path.is_symlink():
        raise FinalizationError("One-hour evidence lacks a safe preflight binding")
    if not snapshot_path.is_file() or snapshot_path.is_symlink():
        raise FinalizationError("One-hour evidence lacks an immutable preflight snapshot")
    try:
        binding = json.loads(binding_path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as error:
        raise FinalizationError(f"Preflight binding is unreadable: {error}") from error
    if not isinstance(binding, dict):
        raise FinalizationError("Preflight binding is not an object")

    expected: dict[str, object] = {}
    for field in PREFLIGHT_IDENTITY_FIELDS:
        value = summary.get(field)
        if not isinstance(value, str) or not value:
            raise FinalizationError(f"One-hour summary lacks preflight identity: {field}")
        if field.endswith("_sha256") and SHA256.fullmatch(value) is None:
            raise FinalizationError(f"One-hour summary has an invalid digest: {field}")
        expected[field] = value
    for field, name in RETAINED_TOOL_FILES.items():
        candidate = directory / name
        if not candidate.is_file() or candidate.is_symlink():
            raise FinalizationError(f"One-hour retained tool is missing or unsafe: {name}")
        if _digest(candidate) != expected[field]:
            raise FinalizationError(f"One-hour retained tool identity differs: {field}")

    recorded_directory = summary.get("preflight_evidence_directory")
    if (
        not isinstance(recorded_directory, str)
        or not pathlib.Path(recorded_directory).is_absolute()
    ):
        raise FinalizationError("One-hour summary lacks an absolute preflight directory")
    expected_manifest = summary.get("preflight_evidence_manifest_sha256")
    expected_snapshot = summary.get("preflight_snapshot_sha256")
    if (
        not isinstance(expected_manifest, str)
        or SHA256.fullmatch(expected_manifest) is None
        or not isinstance(expected_snapshot, str)
        or SHA256.fullmatch(expected_snapshot) is None
    ):
        raise FinalizationError("One-hour summary lacks valid preflight binding digests")
    if _digest(snapshot_path) != expected_snapshot:
        raise FinalizationError("Immutable preflight snapshot digest differs")

    expected_binding = {
        "accepted": True,
        "package_manifest_sha256": summary.get("manifest_sha256"),
        "preflight_evidence_directory": recorded_directory,
        "preflight_evidence_manifest_sha256": expected_manifest,
        "preflight_snapshot_sha256": expected_snapshot,
        "requested_active_seconds": 600,
        "run_class": "ten_minute_preflight",
        "source_commit": summary.get("source_commit"),
    }
    if binding != expected_binding:
        raise FinalizationError("Preflight binding differs from the one-hour summary")
    return binding, expected, snapshot_path, recorded_directory


def _validate_one_hour_preflight_binding(
    directory: pathlib.Path,
    summary: dict[str, object],
    trusted_preflight_verifier: pathlib.Path,
    evidence_names: list[str],
    expected_qualified: bool,
) -> dict[str, object]:
    binding, expected, snapshot_path, recorded_directory = (
        _verify_static_one_hour_binding(directory, summary)
    )

    verifier = _load_trusted_preflight_verifier(
        directory,
        trusted_preflight_verifier,
        str(expected["preflight_verifier_sha256"]),
    )
    try:
        verifier.verify_staged_one_hour(
            directory,
            expected,
            evidence_names,
            expected_qualified,
        )
        revalidated = verifier.verify_preflight_archive(
            snapshot_path,
            expected,
            recorded_directory,
        )
    except Exception as error:
        raise FinalizationError(f"Immutable preflight revalidation failed: {error}") from error
    if revalidated != binding:
        raise FinalizationError("Preflight binding differs from the verified immutable snapshot")
    expected_manifest = summary.get("preflight_evidence_manifest_sha256")
    expected_snapshot = summary.get("preflight_snapshot_sha256")
    if (
        revalidated.get("preflight_evidence_manifest_sha256") != expected_manifest
        or revalidated.get("preflight_snapshot_sha256") != expected_snapshot
        or revalidated.get("source_commit") != summary.get("source_commit")
    ):
        raise FinalizationError("Preflight binding identity differs from the one-hour summary")
    return revalidated


def finalize_evidence(
    staging_directory: pathlib.Path,
    final_directory: pathlib.Path,
    evidence_names: Iterable[str],
    failure_injector: Optional[Callable[[str], None]] = None,
    terminal_publish: bool = False,
    trusted_preflight_verifier: Optional[pathlib.Path] = None,
) -> pathlib.Path:
    staging_directory = staging_directory.absolute()
    final_directory = final_directory.absolute()
    if not staging_directory.is_dir() or staging_directory.is_symlink():
        raise FinalizationError("Staging evidence directory is missing or unsafe")

    summary_path = staging_directory / SUMMARY_NAME
    if not summary_path.is_file() or summary_path.is_symlink():
        raise FinalizationError("Sustained summary is missing or unsafe")
    try:
        summary = json.loads(summary_path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as error:
        raise FinalizationError(f"Could not read sustained summary: {error}") from error
    if not isinstance(summary, dict):
        raise FinalizationError("Sustained summary is not an object")

    # A staging directory can survive an interrupted or rejected attempt. Clear any
    # stale claim before every later gate that can reject publication.
    summary["qualified_one_hour"] = False
    _write_json_atomic(summary_path, summary)

    if final_directory.exists() or final_directory.is_symlink():
        raise FinalizationError("Final evidence path already exists")
    if staging_directory.parent.resolve() != final_directory.parent.resolve():
        raise FinalizationError("Staging and final evidence directories must share one parent")

    names = _validate_evidence_names(evidence_names)
    for name in names:
        candidate = staging_directory / name
        if not candidate.is_file() or candidate.is_symlink():
            raise FinalizationError(f"Evidence file is missing or unsafe: {name}")

    run_class = summary.get("run_class")
    runtime_qualified = summary.get("runtime_contract_qualified_one_hour") is True
    budgets_pass = summary.get("all_measured_budgets_pass") is True
    preflight_binding: Optional[dict[str, object]] = None
    if run_class == "one_hour_qualification":
        if (
            PREFLIGHT_BINDING_NAME not in names
            or PREFLIGHT_SNAPSHOT_NAME not in names
        ):
            raise FinalizationError(
                "One-hour evidence must retain its preflight binding and immutable snapshot"
            )
        if trusted_preflight_verifier is None:
            raise FinalizationError(
                "One-hour finalization requires an external trusted preflight verifier"
            )
        preflight_binding = _validate_one_hour_preflight_binding(
            staging_directory,
            summary,
            trusted_preflight_verifier,
            names,
            False,
        )
    qualified = bool(
        run_class == "one_hour_qualification"
        and runtime_qualified
        and budgets_pass
        and preflight_binding is not None
    )
    if run_class == "one_hour_qualification" and not qualified:
        raise FinalizationError("The final one-hour qualification contract is not satisfied")

    completion_path = staging_directory / COMPLETION_NAME
    manifest_path = staging_directory / MANIFEST_NAME
    try:
        summary["qualified_one_hour"] = qualified
        _write_json_atomic(summary_path, summary)
        if failure_injector is not None:
            failure_injector("after_summary")

        completion = {
            "all_measured_budgets_pass": budgets_pass,
            "preflight_evidence_directory": summary.get(
                "preflight_evidence_directory"
            ),
            "preflight_evidence_manifest_sha256": summary.get(
                "preflight_evidence_manifest_sha256"
            ),
            "preflight_snapshot_sha256": summary.get(
                "preflight_snapshot_sha256"
            ),
            "qualified_one_hour": qualified,
            "run_class": run_class,
            "runtime_contract_qualified_one_hour": runtime_qualified,
            "source_commit": summary.get("source_commit"),
            "status": "complete",
        }
        _write_json_atomic(completion_path, completion)
        published_names = names + [COMPLETION_NAME]
        if failure_injector is not None:
            failure_injector("after_completion")

        manifest_payload = "".join(
            f"{_digest(staging_directory / name)}  {name}\n"
            for name in published_names
        ).encode("utf-8")
        _write_bytes_atomic(manifest_path, manifest_payload)
        for name in published_names:
            expected = next(
                line.split("  ", 1)[0]
                for line in manifest_payload.decode("utf-8").splitlines()
                if line.endswith(f"  {name}")
            )
            if _digest(staging_directory / name) != expected:
                raise FinalizationError(f"Evidence digest verification failed: {name}")
        if failure_injector is not None:
            failure_injector("before_publish")

        if qualified:
            final_preflight_binding = _validate_one_hour_preflight_binding(
                staging_directory,
                summary,
                trusted_preflight_verifier,
                names,
                True,
            )
            if final_preflight_binding != preflight_binding:
                raise FinalizationError(
                    "Preflight binding changed before evidence publication"
                )

        verified_transaction = verify_published_evidence(staging_directory)
        if (
            verified_transaction.get("summary") != summary
            or verified_transaction.get("completion") != completion
        ):
            raise FinalizationError(
                "Final evidence transaction differs from its verified payload"
            )

        _fsync_directory(staging_directory)
        if terminal_publish:
            signal.pthread_sigmask(
                signal.SIG_BLOCK,
                {signal.SIGINT, signal.SIGTERM, signal.SIGHUP, signal.SIGQUIT},
            )
        _rename_directory_no_replace(staging_directory, final_directory)
        try:
            _fsync_directory(final_directory.parent)
        except OSError:
            # The atomic rename is the publication boundary. A parent-directory fsync is
            # best-effort on filesystems that do not expose directory synchronization.
            pass
        if terminal_publish:
            try:
                os.write(1, _terminal_success_output(final_directory, run_class))
            except OSError:
                pass
            os._exit(0)
        return final_directory
    except Exception:
        if staging_directory.is_dir():
            summary["qualified_one_hour"] = False
            try:
                _write_json_atomic(summary_path, summary)
            except OSError:
                pass
            for incomplete_path in (completion_path, manifest_path):
                try:
                    incomplete_path.unlink()
                except FileNotFoundError:
                    pass
                except OSError:
                    pass
        raise


def _parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Atomically publish a verified sustained-soak evidence directory."
    )
    parser.add_argument("--staging-dir", required=True, type=pathlib.Path)
    parser.add_argument("--final-dir", required=True, type=pathlib.Path)
    parser.add_argument("--evidence-file", action="append", default=[])
    parser.add_argument("--trusted-preflight-verifier", type=pathlib.Path)
    parser.add_argument("--abort-provenance", type=pathlib.Path)
    parser.add_argument("--abort-reason", default="evidence_finalization_failed")
    parser.add_argument("--game-pid", type=int)
    parser.add_argument("--game-pgid", type=int)
    parser.add_argument("--elapsed-seconds", type=int)
    parser.add_argument("--next-sample-boundary-seconds", type=int)
    parser.add_argument("--cleanup-signal", default="none")
    parser.add_argument("--wrapper-pid", type=int)
    parser.add_argument("--wrapper-pgid", type=int)
    parser.add_argument("--terminal", action="store_true")
    return parser


def main() -> int:
    arguments = _parser().parse_args()
    termination_signal = "none"
    handled_signals = {
        signal.SIGINT,
        signal.SIGTERM,
        signal.SIGHUP,
        signal.SIGQUIT,
    }
    original_signal_mask = signal.pthread_sigmask(signal.SIG_BLOCK, handled_signals)

    def interrupted(signum: int, _frame: object) -> None:
        nonlocal termination_signal
        signal.pthread_sigmask(signal.SIG_BLOCK, handled_signals)
        termination_signal = signal.Signals(signum).name.removeprefix("SIG")
        raise FinalizationError(
            f"Evidence publication interrupted by signal {signum} before commit"
        )

    for handled_signal in handled_signals:
        signal.signal(handled_signal, interrupted)
    signal.pthread_sigmask(signal.SIG_SETMASK, original_signal_mask)
    try:
        published = finalize_evidence(
            arguments.staging_dir,
            arguments.final_dir,
            arguments.evidence_file,
            terminal_publish=arguments.terminal,
            trusted_preflight_verifier=arguments.trusted_preflight_verifier,
        )
    except Exception as error:
        signal.pthread_sigmask(signal.SIG_BLOCK, handled_signals)
        print(f"Sustained evidence finalization failed: {error}", file=sys.stderr)
        exit_code = {
            "INT": 130,
            "TERM": 143,
            "HUP": 129,
            "QUIT": 131,
        }.get(termination_signal, 1)
        wrapper_identity = (arguments.wrapper_pid, arguments.wrapper_pgid)
        runtime_provenance = (
            arguments.game_pid,
            arguments.game_pgid,
            arguments.elapsed_seconds,
            arguments.next_sample_boundary_seconds,
        )
        if arguments.abort_provenance is not None:
            if any(value is None for value in runtime_provenance) or (
                (wrapper_identity[0] is None) != (wrapper_identity[1] is None)
            ):
                print(
                    "Sustained evidence abort provenance is incomplete.",
                    file=sys.stderr,
                )
            else:
                try:
                    _write_abort_provenance(
                        arguments.abort_provenance,
                        reason=arguments.abort_reason,
                        detail=str(error),
                        game_pid=arguments.game_pid,
                        game_pgid=arguments.game_pgid,
                        elapsed_seconds=arguments.elapsed_seconds,
                        next_sample_boundary_seconds=(
                            arguments.next_sample_boundary_seconds
                        ),
                        wrapper_exit_code=exit_code,
                        cleanup_signal=arguments.cleanup_signal,
                        termination_signal=termination_signal,
                        wrapper_pid=arguments.wrapper_pid,
                        wrapper_pgid=arguments.wrapper_pgid,
                    )
                except (FinalizationError, OSError) as provenance_error:
                    print(
                        f"Sustained evidence abort provenance failed: {provenance_error}",
                        file=sys.stderr,
                    )
        if arguments.terminal:
            try:
                sys.stdout.flush()
                sys.stderr.flush()
            finally:
                os._exit(exit_code)
        signal.pthread_sigmask(signal.SIG_SETMASK, original_signal_mask)
        return exit_code
    signal.pthread_sigmask(signal.SIG_SETMASK, original_signal_mask)
    print(published)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
