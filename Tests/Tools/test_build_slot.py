#!/usr/bin/env python3
"""Build-slot refusal and holder detection. Author: Angelis Pseftis."""
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

SCRIPT = Path(__file__).resolve().parents[2] / 'Scripts/acquire_build_slot.sh'


class BuildSlotTests(unittest.TestCase):
    def inspect(self, processes='1 /sbin/launchd\n', ps_exit=0, editor='', pgrep_exit=1):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for name, body in {
                'ps': 'printf "%s" "$MOCK_PROCESSES"; exit "$MOCK_PS_EXIT"',
                'pgrep': 'printf "%s" "$MOCK_EDITOR"; exit "$MOCK_PGREP_EXIT"',
            }.items():
                executable = root / name
                executable.write_text('#!/bin/sh\n' + body + '\n')
                executable.chmod(0o755)
            env = dict(os.environ, PATH=f'{root}:/usr/bin:/bin',
                       MOCK_PROCESSES=processes, MOCK_PS_EXIT=str(ps_exit),
                       MOCK_EDITOR=editor, MOCK_PGREP_EXIT=str(pgrep_exit))
            return subprocess.run(['/bin/zsh', str(SCRIPT)], env=env,
                                  capture_output=True, text=True)

    def test_denied_process_inspection_refuses_launch(self):
        for processes in ('', '1 /sbin/launchd\n'):
            with self.subTest(processes=processes):
                result = self.inspect(processes=processes, ps_exit=1)
                self.assertEqual(result.returncode, 2)
                self.assertNotIn('build slot free', result.stdout)

    def test_empty_successful_inventory_refuses_launch(self):
        self.assertEqual(self.inspect(processes=' \n').returncode, 2)

    def test_failed_editor_inspection_refuses_launch(self):
        self.assertEqual(self.inspect(pgrep_exit=2).returncode, 2)

    def test_no_holders(self):
        self.assertEqual(self.inspect().returncode, 0)

    def test_each_real_holder_blocks_launch(self):
        for process in ('/usr/bin/dotnet /engine/UnrealBuildTool.dll',
                        '/usr/bin/dotnet /engine/AutomationTool.dll',
                        '/usr/bin/clang @/project/Intermediate/Build/args'):
            with self.subTest(process=process):
                self.assertEqual(self.inspect(processes=f'12345 {process}\n').returncode, 1)
        self.assertEqual(self.inspect(editor='12345', pgrep_exit=0).returncode, 1)

    def test_inspection_commands_are_not_builds(self):
        result = self.inspect(processes='12345 /usr/bin/grep UnrealBuildTool.dll\n'
                             '12346 /bin/zsh -c inspect RunUAT\n')
        self.assertEqual(result.returncode, 0)


if __name__ == '__main__':
    unittest.main()
