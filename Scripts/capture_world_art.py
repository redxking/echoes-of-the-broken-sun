"""Capture bounded world-art authoring previews. Author: Angelis Pseftis.

Runs one Unreal process at a time, uses isolated saves, and retains command/source
identity. These are editor-hosted runtime stills, not packaged or interaction proof.
"""
import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
EDITOR = Path('/Users/Shared/Epic Games/UE_5.8/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor')
CASES = {
    'glass-scar': (None, 0),
    'm01-play': ('Prologue', 0),
    'm01-archive': ('Prologue', 0),
    'm01-well': ('Prologue', 0),
    'shivergrass': ('SevenAccounts', 1),
    'cavern': ('UnburiedRoad', 3),
    'civic': ('NamesWithoutBirths', 5),
    'choir': ('ShapeBesideUs', 7),
    'solar': ('TheBrokenSun', 14),
}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--output', type=Path, required=True)
    views = parser.add_mutually_exclusive_group()
    views.add_argument('--unlit', action='store_true', help='Lighting diagnosis only; never visual qualification')
    views.add_argument('--buffer', choices=('WorldNormal', 'BaseColor'), help='Material diagnosis only; never visual qualification')
    parser.add_argument('cases', nargs='*', choices=list(CASES))
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    base = subprocess.check_output(['getconf','DARWIN_USER_TEMP_DIR'],text=True).strip()
    source = subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip()
    diff = subprocess.check_output(['git','diff','--binary'],cwd=ROOT)
    status = subprocess.check_output(['git','status','--porcelain'],cwd=ROOT,text=True)
    untracked = subprocess.check_output(['git','ls-files','--others','--exclude-standard','-z'],cwd=ROOT).decode().split('\0')
    source_files = {name: hashlib.sha256((ROOT/name).read_bytes()).hexdigest()
        for name in untracked if name and name.startswith(('Source/','Scripts/','Tests/','Content/','Config/')) and (ROOT/name).is_file()}
    runtime_binaries = {str(path.relative_to(ROOT)): hashlib.sha256(path.read_bytes()).hexdigest()
        for path in (ROOT/'Binaries/Mac').glob('*EchoesOfTheBrokenSun*.dylib') if path.is_file()}
    (output/'tracked-source.patch').write_bytes(diff)
    for name in args.cases or CASES:
        mode, through = CASES[name]
        center, delay = {'m01-archive': ('-2000,-2800', 45),
                         'm01-well': ('0,0', 90)}.get(name, ('-4400,-4400', 12))
        target = output / f'{name}.png'
        log = output / f'{name}.log'
        if target.exists() or log.exists():
            raise RuntimeError(f'Refusing to overwrite retained evidence: {target}')
        with tempfile.TemporaryDirectory(prefix='EchoesWorldCapture.',dir=base) as saves:
            command = [str(EDITOR),str(ROOT/'EchoesOfTheBrokenSun.uproject'),'/Engine/Maps/Entry',
                '-game','-ResX=1920','-ResY=1080','-Windowed','-nosplash','-nosound',
                '-benchmark','-fps=20',f'-benchmarkseconds={delay+13}','-EchoesAutoStart',
                '-EchoesArtReviewHideUI',f'-EchoesSaveGameDirectory={saves}',
                f'-EchoesArtReviewOutput={target}',f'-abslog={log}']
            if args.unlit:
                command += ['-ExecCmds=viewmode unlit']
            elif args.buffer:
                command += [f'-ExecCmds=viewmode VisualizeBuffer,r.BufferVisualizationTarget {args.buffer}']
            if mode:
                if through:
                    fixture = output / f'{name}-fixture.bin'
                    subprocess.run([sys.executable,str(ROOT/'Scripts/create_campaign_fixture.py'),str(fixture),
                        '--choice','Harvest' if through == 14 else 'Preserve',
                        '--through-mission',str(through)],cwd=ROOT,check=True)
                    command += [f'-EchoesCampaignProgressPath={fixture}']
                command += [f'-EchoesCampaign{mode}',
                    '-EchoesArtReview',f'-EchoesArtReviewCenter={center}',
                    '-EchoesArtReviewZoom=3800',f'-EchoesArtReviewDelay={delay}']
                if name in ('m01-archive', 'm01-well'):
                    command += [f'-EchoesArtReviewScout={center}', '-EchoesArtReviewScoutOnly']
            else:
                command += ['-EchoesGlassScarReview=VerticalSlice']
            started = datetime.now(timezone.utc).isoformat()
            print(f'CAPTURE_START {name}',flush=True)
            with (output/f'{name}-console.log').open('w') as console:
                result = subprocess.run(command,cwd=ROOT,stdout=console,stderr=subprocess.STDOUT,timeout=240)
            text = log.read_text(errors='replace') if log.exists() else ''
            refused = any(marker in text for marker in (
                'Failed to compile Material','[ECHOES_BOOT_INCOMPLETE]', 'Fatal error:', 'Assertion failed:'))
            if args.buffer and 'Set new viewmode: VisualizeBuffer' not in text:
                refused = True
            valid = result.returncode == 0 and target.exists() and '[ECHOES_ART_REVIEW_CAPTURE]' in text and not refused
            (output/f'{name}-identity.json').write_text(json.dumps({
                'author':'Angelis Pseftis','kind':'editor-hosted authoring preview',
                'started_utc':started,'finished_utc':datetime.now(timezone.utc).isoformat(),
                'untracked_source_and_assets_sha256':source_files,'project_runtime_binaries_sha256':runtime_binaries,
                'source_commit':source,'tracked_diff_sha256':hashlib.sha256(diff).hexdigest(),
                'dirty_status':status,'command':command,'exit_code':result.returncode,
                'capture_created':valid,'resolution':[1920,1080],
                'image_sha256':hashlib.sha256(target.read_bytes()).hexdigest() if target.exists() else None,
                'visual_quality':f'{args.buffer} material diagnostic only' if args.buffer else ('unlit diagnostic only' if args.unlit else 'not assessed by this capture script'),
                'audio':'disabled; no audio claim','interaction':'no physical-input claim',
            },indent=2)+'\n')
            if not valid: raise RuntimeError(f'Capture failed: {name}; inspect {log}')
            print(f'CAPTURE_CREATED {name}',flush=True)


if __name__ == '__main__':
    main()
