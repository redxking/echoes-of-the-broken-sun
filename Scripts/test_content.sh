#!/bin/zsh
set -euo pipefail

project_root="${0:A:h:h}"

python3 "$project_root/Tests/Content/test_content_compiler.py"
python3 "$project_root/Scripts/compile_content.py"
