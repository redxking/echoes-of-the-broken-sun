#!/bin/zsh
set -euo pipefail

project_root="${0:A:h:h}"

/usr/bin/python3 "$project_root/Tests/Content/test_world_kits.py"
/usr/bin/python3 "$project_root/Tests/Content/test_evacuation_props.py"
/usr/bin/python3 "$project_root/Tests/Content/test_meridian_facing.py"
/usr/bin/python3 "$project_root/Content/World/Tools/compile_mission_landmarks.py" --root "$project_root" --check
/usr/bin/python3 "$project_root/Tests/World/test_mission_landmarks.py"
/usr/bin/python3 "$project_root/Content/World/Tools/compile_campaign_map_pack.py" --root "$project_root" --check
/usr/bin/python3 "$project_root/Tests/World/test_campaign_map_pack.py"
/usr/bin/python3 "$project_root/Tests/Content/test_content_compiler.py"
/usr/bin/python3 "$project_root/Tests/Content/test_build_identity.py"
/usr/bin/python3 "$project_root/Tests/Content/test_package_manifest_verifier.py"
/usr/bin/python3 "$project_root/Tests/Content/test_sustained_soak_validator.py"
/usr/bin/python3 "$project_root/Tests/Content/test_sustained_evidence_finalizer.py"
/usr/bin/python3 "$project_root/Tests/Content/test_sustained_preflight.py"
/usr/bin/python3 "$project_root/Tests/Content/test_sustained_soak_wrapper.py"
/usr/bin/python3 "$project_root/Scripts/compile_content.py"
