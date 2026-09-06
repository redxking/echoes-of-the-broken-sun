#!/usr/bin/env python3
"""Compile Training-only lesson coordinates. Author: Angelis Pseftis."""
import argparse
import hashlib
import json
from pathlib import Path

SOURCE = Path('Content/World/Source/Authoring/training_readiness_staging_v1.json')
HEADER = Path('Content/World/Generated/Training/EchoesTrainingStaging.h')


def exact_keys(value, keys, label):
    if not isinstance(value, dict) or set(value) != set(keys):
        raise ValueError(f'{label}: unexpected or missing fields')


def render(source):
    data = json.loads(source)
    exact_keys(data, ('source_format', 'source_version', 'author', 'operation',
                     'map_id', 'link', 'foundry', 'scope'), 'staging')
    if (data['source_format'] != 'echoes_training_staging' or
            type(data['source_version']) is not int or data['source_version'] != 1 or
            data['author'] != 'Angelis Pseftis' or
            data['operation'] != 'training-readiness' or
            data['map_id'] != 'campaign-map-01' or
            not isinstance(data['scope'], str) or not data['scope'].strip()):
        raise ValueError('invalid staging identity')
    link, foundry = data['link'], data['foundry']
    exact_keys(link, ('build_site', 'rejected_site', 'repair_target_site',
                     'repair_initial_hp', 'repair_max_hp', 'completion_radius_sim_cm'), 'link')
    exact_keys(foundry, ('producer_site', 'rally_site'), 'foundry')
    points = {'LinkBuildSite': link['build_site'], 'LinkRejectedSite': link['rejected_site'],
              'LinkRepairTargetSite': link['repair_target_site'],
              'FoundryProducerSite': foundry['producer_site'], 'FoundryRallySite': foundry['rally_site']}
    for name, value in points.items():
        if (not isinstance(value, list) or len(value) != 2 or
                any(type(v) is not int or not 0 <= v < 64 for v in value)):
            raise ValueError(f'{name}: expected integer M01 grid coordinates')
    if len({tuple(v) for v in points.values()}) != len(points):
        raise ValueError('staging sites must remain distinct')
    if (any(type(link[k]) is not int for k in
            ('repair_initial_hp', 'repair_max_hp', 'completion_radius_sim_cm')) or
            not 0 < link['repair_initial_hp'] < link['repair_max_hp'] or
            link['repair_max_hp'] != 450 or link['completion_radius_sim_cm'] != 200):
        raise ValueError('invalid damaged target or completion radius')
    rows = ['// GENERATED FILE - edit the registered Training staging source instead.',
            '// Author and owner: Angelis Pseftis', '#pragma once', '#include <cstdint>',
            '#include <string_view>', 'namespace echoes::world::training_staging {',
            'struct Tile final { std::int32_t x; std::int32_t y; };',
            f'inline constexpr std::string_view kSourceSha256 = "{hashlib.sha256(source).hexdigest()}";',
            'inline constexpr std::string_view kOperation = "training-readiness";']
    rows += [f'inline constexpr Tile k{name}{{{v[0]}, {v[1]}}};' for name, v in points.items()]
    rows += [f"inline constexpr std::int32_t kLinkRepairInitialHp = {link['repair_initial_hp']};",
             f"inline constexpr std::int32_t kLinkRepairMaxHp = {link['repair_max_hp']};",
             f"inline constexpr std::int32_t kLinkCompletionRadiusSimCm = {link['completion_radius_sim_cm']};", '}', '']
    return '\n'.join(rows).encode()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[3])
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument('--check', action='store_true')
    mode.add_argument('--write', action='store_true')
    args = parser.parse_args()
    output = render((args.root / SOURCE).read_bytes())
    path = args.root / HEADER
    if args.write:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(output)
    elif not path.is_file() or path.read_bytes() != output:
        raise SystemExit('Training staging header is missing or stale')
    print('Training staging source/header current; runtime geometry admission is a separate check')


if __name__ == '__main__':
    main()
