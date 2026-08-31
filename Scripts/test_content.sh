#!/bin/zsh
set -euo pipefail

project_root="${0:A:h:h}"

/usr/bin/python3 "$project_root/Tests/Content/test_content_compiler.py"
/usr/bin/python3 "$project_root/Tests/Content/test_build_identity.py"
/usr/bin/python3 "$project_root/Tests/Content/test_package_manifest_verifier.py"
/usr/bin/python3 "$project_root/Tests/Content/test_sustained_soak_validator.py"
/usr/bin/python3 "$project_root/Tests/Content/test_sustained_evidence_finalizer.py"
/usr/bin/python3 "$project_root/Tests/Content/test_sustained_preflight.py"
/usr/bin/python3 "$project_root/Tests/Content/test_sustained_soak_wrapper.py"
/usr/bin/python3 "$project_root/Scripts/compile_content.py"
