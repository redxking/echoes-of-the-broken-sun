"""Build an isolated editor calibration map. Author: Angelis Pseftis.

Ordinary Python: --check prints the dimension plan without importing Unreal.
Unreal: -ExecutePythonScript=<this file> builds only the owned Developer map.
Set ECHOES_SPATIAL_EVIDENCE to retain the engine readback receipt.
"""
from __future__ import annotations

import argparse
import gc
import hashlib
import json
import math
import os
from pathlib import Path
import time
import traceback

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'Content/World/Source/Authoring/sc2_spatial_calibration_v1.json'
ASSET_ROOT = '/Game/Developers/AngelisPseftis/SpatialCalibration'
MAP_PATH = ASSET_ROOT + '/SC2SpatialCalibration'
AUTHOR = 'Angelis Pseftis'


def positive(value, name):
    if isinstance(value, bool) or not isinstance(value, (int, float)) or not math.isfinite(value) or value <= 0:
        raise ValueError(f'{name} must be a finite positive number')
    return float(value)


def load_source(path=SOURCE):
    data = json.loads(Path(path).read_text())
    if (data.get('source_format'), data.get('source_version'), data.get('runtime_binding')) != (
            'echoes_spatial_calibration', 1, 'none'):
        raise ValueError('Only version-1 calibration data with no runtime binding is admitted')
    if data.get('author') != AUTHOR:
        raise ValueError('Missing authoritative authorship')
    for key in ('cell_cm', 'cliff_step_cells', 'ramp_run_cells', 'clearance_agent_radius_cells'):
        positive(data[key], key)
    for key in ('ramp_width_cells', 'clearance_width_cells'):
        if len(data[key]) != 3:
            raise ValueError(f'{key} needs three comparison stations')
        for value in data[key]:
            positive(value, key)
    if len(data['footprints']) != 4 or len(data['units']) != 6:
        raise ValueError('Expected four footprint and six unit references')
    for group, field in (('footprints', 'cells'), ('units', 'radius_cells')):
        ids = [row['id'] for row in data[group]]
        if len(ids) != len(set(ids)):
            raise ValueError(f'Duplicate {group} identity')
        for row in data[group]:
            positive(row[field], field)
    for row in data['units']:
        if row['inner_radius_cells'] is not None:
            positive(row['inner_radius_cells'], 'inner_radius_cells')
    camera = data['camera']
    if camera.get('fov_axis_verified') is not False:
        raise ValueError('The unresolved SC2 FOV axis cannot be silently promoted')
    for key in ('sc2_catalog_fov_degrees', 'unreal_horizontal_fov_hypothesis'):
        if not 1 <= positive(camera[key], key) < 170:
            raise ValueError('Perspective FOV must be in [1,170)')
    positive(camera['aspect_ratio'], 'aspect_ratio')
    derived = math.degrees(2*math.atan(math.tan(math.radians(camera['sc2_catalog_fov_degrees']/2))*camera['aspect_ratio']))
    if abs(derived-camera['unreal_horizontal_fov_hypothesis']) > .001:
        raise ValueError('Horizontal FOV does not match the documented vertical-axis hypothesis')
    if not math.isfinite(camera['yaw_degrees']):
        raise ValueError('Invalid camera yaw')
    if len(camera['zoom_stops']) != 5:
        raise ValueError('Expected five source zoom stops')
    for stop in camera['zoom_stops']:
        positive(stop['distance_cells'], 'distance_cells')
        if not -89 <= stop['pitch_degrees'] <= -1:
            raise ValueError('Camera pitch must look downward')
    return data


def build_plan(data):
    """Pure geometry shared by engine generation and dimensional verification."""
    cell = data['cell_cm']
    actors = []

    def shape(name, kind, center, size, color='block', pitch=0):
        actors.append(dict(name=name, kind=kind, center=list(center), size=list(size), color=color, pitch=pitch))

    def label(name, text, center, size=75):
        actors.append(dict(name=name, kind='label', text=text, center=list(center), font_size=size))

    # Fixture-only geometry: neutral measuring stations with no mission/lore binding.
    shape('Floor', 'cube', (0, 0, -20), (52*cell, 36*cell, 40), 'floor')
    for i in range(-26, 27):
        shape(f'GridX_{i}', 'cube', (i*cell, 0, 1), (2, 36*cell, 2), 'grid')
    for i in range(-18, 19):
        shape(f'GridY_{i}', 'cube', (0, i*cell, 1), (52*cell, 2, 2), 'grid')
    label('Title', f'SPATIAL CALIBRATION | 1 cell = {cell:g} cm', (0, 3200, 80), 110)
    label('Boundary', 'EDITOR REFERENCE | FOV hypothesis | no gameplay validation', (0, 2950, 70), 70)
    for row, x in zip(data['footprints'], (-3900, -2600, -1000, 900)):
        width = row['cells']*cell
        shape('Footprint_'+row['id'], 'cube', (x, 2100, 100), (width, width, 200))
        label('FootprintLabel_'+row['id'], f"{row['label']} / {width:g} cm", (x, 1550, 260), 70)
    label('FootprintBoundary', 'Placement squares; building pathing contours differ', (-1500, 1250, 240), 65)
    for i, row in enumerate(data['units']):
        x = -3900+i*780
        radius = row['radius_cells']*cell
        shape('Radius_'+row['id'], 'cylinder', (x, 800, 12), (2*radius, 2*radius, 20), 'radius')
        label('UnitLabel_'+row['id'], f"{row['label']} r={radius:g}", (x, 440, 60), 60)
        inner = row['inner_radius_cells']
        if inner is not None:
            shape('Inner_'+row['id'], 'cylinder', (x, 800, 28), (2*inner*cell, 2*inner*cell, 8), 'inner')
        else:
            label('Unknown_'+row['id'], 'inner unresolved', (x, 240, 50), 45)
    label('RadiusKey', 'Amber: Radius | pale disc: terrain / structure InnerRadius', (-1500, 40, 60), 65)

    ramp_info = []
    run, rise = data['ramp_run_cells']*cell, data['cliff_step_cells']*cell
    angle = math.atan2(rise, run)
    for i, width_cells in enumerate(data['ramp_width_cells']):
        x, y = -3700+i*3300, -1500
        width = width_cells*cell
        length = math.hypot(run, rise)
        # Cube top surface endpoints exactly meet z=0 and z=rise.
        center = (x+10*math.sin(angle), y, rise/2-10*math.cos(angle))
        shape(f'Ramp_{width_cells}', 'cube', center, (length, width, 20), 'ramp', math.degrees(angle))
        shape(f'Plateau_{width_cells}', 'cube', (x+run/2+cell, y, rise/2), (2*cell, width, rise), 'ramp')
        label(f'RampLabel_{width_cells}', f'{width_cells:g}-cell ramp | rise {rise:g} / run {run:g}', (x, -2700, 70), 65)
        ramp_info.append(dict(name=f'Ramp_{width_cells}', width_cm=width, run_cm=run, rise_cm=rise,
                              slope_degrees=math.degrees(angle), top_start=[x-run/2, y, 0], top_end=[x+run/2, y, rise]))

    # Clearance station beyond the footprint station, viewed with its own camera.
    clearance_info = []
    for i, cells in enumerate(data['clearance_width_cells']):
        x, y, width = 6200+i*2000, 1600, cells*cell
        wall = cell
        for side in (-1, 1):
            shape(f'Gap_{cells}_Wall_{side}', 'cube', (x+side*(width+wall)/2, y, 150), (wall, 6*cell, 300))
        r = data['clearance_agent_radius_cells']*cell
        shape(f'Gap_{cells}_Agent', 'cylinder', (x, y, 40), (2*r, 2*r, 80), 'radius')
        label(f'Gap_{cells}_Label', f'{cells:g} cells / {width:g} cm' + (' / TANGENT' if width == 2*r else ''), (x, 600, 80), 65)
        clearance_info.append(dict(name=f'Gap_{cells}', clear_width_cm=width, agent_radius_cm=r,
                                   side_margin_cm=(width-2*r)/2))
    shape('ClearanceFloor', 'cube', (8200, 1400, -20), (34*cell, 14*cell, 40), 'floor')
    label('ClearanceTitle', 'CLEARANCE | geometry comparison, not pathing proof', (8200, 2850, 80), 85)

    camera = data['camera']
    cameras = []
    def camera_record(name, focus, distance, pitch, fov):
        yaw = camera['yaw_degrees']
        pr, yr = math.radians(pitch), math.radians(yaw)
        forward = [math.cos(pr)*math.cos(yr), math.cos(pr)*math.sin(yr), math.sin(pr)]
        cameras.append(dict(name=name, focus=list(focus), distance_cm=distance, pitch=pitch, yaw=yaw,
                            location=[focus[i]-distance*forward[i] for i in range(3)],
                            horizontal_fov=fov, aspect_ratio=camera['aspect_ratio']))
    hfov = camera['unreal_horizontal_fov_hypothesis']
    for i, stop in enumerate(camera['zoom_stops']):
        camera_record(f'Reference_Zoom_{i}', (-1500, 1600, 0), stop['distance_cells']*cell, stop['pitch_degrees'], hfov)
    camera_record('Reference_AlternateHorizontalFOV', (-1500, 1600, 0), 34*cell, -56, camera['sc2_catalog_fov_degrees'])
    camera_record('Reference_Ramps', (0, -1400, 200), 34*cell, -56, hfov)
    camera_record('Reference_Clearance', (8200, 1600, 0), 34*cell, -56, hfov)
    camera_record('Overview_NotGameplayZoom', (3100, 100, 0), 110*cell, -65, hfov)
    return dict(actors=actors, cameras=cameras, ramps=ramp_info, clearance=clearance_info,
                cell_cm=cell, cliff_step_cm=rise, author=AUTHOR, runtime_binding='none')


def generate(data, plan):
    import unreal
    # This generator runs in its own clean editor process, never in live M01 PIE.
    if unreal.EditorLevelLibrary.get_game_world() is not None:
        raise RuntimeError('Refusing generation while a game/PIE world exists')
    level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    unreal.EditorAssetLibrary.make_directory(ASSET_ROOT)
    if unreal.EditorAssetLibrary.does_asset_exist(MAP_PATH):
        existing = unreal.load_asset(MAP_PATH)
        owner = unreal.EditorAssetLibrary.get_metadata_tag(existing, 'Author')
        if owner not in ('', AUTHOR):
            raise RuntimeError('Existing calibration map has another owner')
        # Map reload collects the old package; Python must release its strong reference.
        del existing
        gc.collect()
        if not level.load_level(MAP_PATH):
            raise RuntimeError('Cannot reload existing calibration level')
        for actor in actors.get_all_level_actors():
            if unreal.Name('Echoes.SpatialCalibration') in actor.get_editor_property('tags'):
                if not actors.destroy_actor(actor):
                    raise RuntimeError('Cannot replace owned fixture actor')
            elif not owner and actor.get_class().get_name() not in ('WorldSettings','Brush','DefaultPhysicsVolume'):
                raise RuntimeError('Unowned existing map is not an empty partial generation')
    elif not level.new_level(MAP_PATH):
        raise RuntimeError(f'Cannot create owned calibration level {MAP_PATH}')
    assets = unreal.AssetToolsHelpers.get_asset_tools()
    palette = {'floor':(.06,.07,.08), 'grid':(.16,.18,.20), 'block':(.55,.60,.63),
               'radius':(.70,.34,.06), 'inner':(.85,.86,.78), 'ramp':(.22,.42,.48)}
    materials = {}
    for name, color in palette.items():
        path = ASSET_ROOT+'/M_Calibration_'+name
        material = unreal.load_asset(path)
        if material is not None and unreal.EditorAssetLibrary.get_metadata_tag(material, 'Author') != AUTHOR:
            raise RuntimeError('Existing material does not belong to calibration generator')
        if material is None:
            material = assets.create_asset('M_Calibration_'+name, ASSET_ROOT, unreal.Material, unreal.MaterialFactoryNew())
        unreal.MaterialEditingLibrary.delete_all_material_expressions(material)
        rgb = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionConstant3Vector)
        rgb.set_editor_property('constant', unreal.LinearColor(*color, 1.0))
        unreal.MaterialEditingLibrary.connect_material_property(rgb, '', unreal.MaterialProperty.MP_BASE_COLOR)
        rough = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionConstant)
        rough.set_editor_property('r', .9)
        unreal.MaterialEditingLibrary.connect_material_property(rough, '', unreal.MaterialProperty.MP_ROUGHNESS)
        unreal.MaterialEditingLibrary.recompile_material(material)
        unreal.EditorAssetLibrary.set_metadata_tag(material, 'Author', AUTHOR)
        unreal.EditorAssetLibrary.set_metadata_tag(material, 'Creator', AUTHOR)
        if not unreal.EditorAssetLibrary.save_loaded_asset(material):
            raise RuntimeError('Calibration material save failed')
        materials[name] = material
    meshes = {kind:unreal.load_asset('/Engine/BasicShapes/'+name) for kind,name in [('cube','Cube'),('cylinder','Cylinder')]}
    if not all(meshes.values()):
        raise RuntimeError('Missing engine primitive mesh')
    observed = []
    for item in plan['actors']:
        actor_class = unreal.TextRenderActor if item['kind']=='label' else unreal.StaticMeshActor
        actor = actors.spawn_actor_from_class(actor_class, unreal.Vector(*item['center']))
        actor.set_actor_label(item['name'])
        actor.set_folder_path('SpatialCalibration')
        actor.set_editor_property('tags', [unreal.Name('Echoes.SpatialCalibration')])
        actor.set_editor_property('is_editor_only_actor', True)
        if item['kind']=='label':
            text = actor.get_component_by_class(unreal.TextRenderComponent)
            text.set_text(item['text'])
            text.set_world_size(item['font_size'])
            text.set_horizontal_alignment(unreal.HorizTextAligment.EHTA_CENTER)
            text.set_text_render_color(unreal.Color(225,235,235,255))
            actor.set_actor_rotation(unreal.Rotator(pitch=56, yaw=90, roll=0), False)
        else:
            component = actor.static_mesh_component
            component.set_static_mesh(meshes[item['kind']])
            component.set_material(0, materials[item['color']])
            component.set_collision_profile_name('NoCollision')
            component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
            component.set_editor_property('generate_overlap_events', False)
            actor.set_actor_scale3d(unreal.Vector(*(v/100 for v in item['size'])))
            actor.set_actor_rotation(unreal.Rotator(pitch=item['pitch'],yaw=0,roll=0), False)
            origin, extent = actor.get_actor_bounds(False)
            angle = math.radians(item['pitch'])
            expected_extent = [(abs(math.cos(angle))*item['size'][0]+abs(math.sin(angle))*item['size'][2])/2,
                               item['size'][1]/2,
                               (abs(math.sin(angle))*item['size'][0]+abs(math.cos(angle))*item['size'][2])/2]
            if any(abs(actual-expected) > .1 for actual,expected in zip((extent.x,extent.y,extent.z),expected_extent)):
                raise RuntimeError(f'Engine primitive bounds mismatch for {item["name"]}: {extent} vs {expected_extent}')
            observed.append(dict(name=item['name'], center=[origin.x,origin.y,origin.z],
                                 extent=[extent.x,extent.y,extent.z], collision=str(component.get_collision_enabled())))
    cameras = {}
    for item in plan['cameras']:
        camera = actors.spawn_actor_from_class(unreal.CameraActor, unreal.Vector(*item['location']))
        camera.set_actor_label(item['name'])
        camera.set_actor_rotation(unreal.Rotator(pitch=item['pitch'],yaw=item['yaw'],roll=0),False)
        camera.set_editor_property('is_editor_only_actor', True)
        camera.set_editor_property('tags', [unreal.Name('Echoes.SpatialCalibration')])
        camera.set_folder_path('SpatialCalibration/Cameras')
        component = camera.get_component_by_class(unreal.CameraComponent)
        component.set_field_of_view(item['horizontal_fov'])
        component.set_aspect_ratio(item['aspect_ratio'])
        component.set_editor_property('constrain_aspect_ratio', True)
        cameras[item['name']] = camera
    sun = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0,0,5000))
    sun.set_actor_rotation(unreal.Rotator(pitch=-55,yaw=-35,roll=0),False)
    sun.light_component.set_intensity(3.0)
    sun.set_editor_property('is_editor_only_actor',True)
    sun.set_editor_property('tags', [unreal.Name('Echoes.SpatialCalibration')])
    sky = actors.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0,0,4000))
    sky.light_component.set_intensity(1.0)
    sky.set_editor_property('is_editor_only_actor',True)
    sky.set_editor_property('tags', [unreal.Name('Echoes.SpatialCalibration')])
    world = unreal.EditorLevelLibrary.get_editor_world()
    world.get_world_settings().set_editor_property('default_game_mode', unreal.GameModeBase)
    overview = next(c for c in plan['cameras'] if c['name']=='Overview_NotGameplayZoom')
    unreal.EditorLevelLibrary.set_level_viewport_camera_info(unreal.Vector(*overview['location']), unreal.Rotator(pitch=overview['pitch'],yaw=overview['yaw'],roll=0))
    if not level.save_current_level():
        raise RuntimeError('Calibration map save failed')
    # UWorld metadata is persisted into the same package, keeping one asset authority.
    asset = unreal.load_asset(MAP_PATH)
    for key, value in {'Author':AUTHOR, 'Creator':AUTHOR, 'Echoes.RuntimeBinding':'none',
                       'Echoes.SourceSHA256':hashlib.sha256(SOURCE.read_bytes()).hexdigest(),
                       'Echoes.GeneratorSHA256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest()}.items():
        unreal.EditorAssetLibrary.set_metadata_tag(asset, key, value)
    if not level.save_current_level():
        raise RuntimeError('Calibration metadata save failed')
    receipt = dict(author=AUTHOR, map_path=MAP_PATH, source_sha256=hashlib.sha256(SOURCE.read_bytes()).hexdigest(),
                   actors_observed=observed, camera_count=len(cameras), dimensions=plan,
                   evidence_class='editor generation and property readback; no gameplay or rendered qualification')
    output = os.environ.get('ECHOES_SPATIAL_EVIDENCE')
    if output:
        folder = Path(output); folder.mkdir(parents=True,exist_ok=True)
        with (folder/'generation-receipt.json').open('x') as stream:
            stream.write(json.dumps(receipt,indent=2)+'\n')
    unreal.log(f'[ECHOES_SPATIAL_CALIBRATION_READY] map={MAP_PATH} actors={len(observed)} cameras={len(cameras)} cell_cm={data["cell_cm"]} runtime_binding=none')


def capture():
    """Capture only the saved calibration world in a separate editor process."""
    import unreal
    folder = Path(os.environ['ECHOES_SPATIAL_EVIDENCE'])
    folder.mkdir(parents=True,exist_ok=True)
    level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not level.load_level(MAP_PATH):
        raise RuntimeError('Calibration map must be generated before capture')
    asset = unreal.load_asset(MAP_PATH)
    identity = {'Author':AUTHOR,'Echoes.RuntimeBinding':'none',
                'Echoes.SourceSHA256':hashlib.sha256(SOURCE.read_bytes()).hexdigest(),
                'Echoes.GeneratorSHA256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest()}
    for key,value in identity.items():
        if unreal.EditorAssetLibrary.get_metadata_tag(asset,key) != value:
            raise RuntimeError('Saved calibration identity mismatch: '+key)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    camera_actors = [a for a in actors.get_all_level_actors() if isinstance(a,unreal.CameraActor)
                    and unreal.Name('Echoes.SpatialCalibration') in a.get_editor_property('tags')]
    cameras = {a.get_actor_label():a for a in camera_actors}
    if len(cameras)!=len(camera_actors):
        raise RuntimeError('Duplicate calibration camera labels')
    observed_cameras=[]
    for expected in build_plan(load_source())['cameras']:
        camera = cameras.get(expected['name'])
        if camera is None:
            raise RuntimeError('Missing calibration camera')
        loc,rot = camera.get_actor_location(),camera.get_actor_rotation()
        component = camera.get_component_by_class(unreal.CameraComponent)
        actual=[loc.x,loc.y,loc.z,rot.pitch,rot.yaw,rot.roll,component.field_of_view,component.aspect_ratio]
        target=expected['location']+[expected['pitch'],expected['yaw'],0,expected['horizontal_fov'],expected['aspect_ratio']]
        if any(abs(a-b)>.01 for a,b in zip(actual,target)):
            raise RuntimeError('Saved camera differs from source: '+expected['name'])
        observed_cameras.append({'name':expected['name'],'location_rotation_fov_aspect':actual})
    names = ['Overview_NotGameplayZoom','Reference_Zoom_0','Reference_Zoom_4',
             'Reference_AlternateHorizontalFOV','Reference_Ramps','Reference_Clearance']
    if any(name not in cameras for name in names):
        raise RuntimeError('Saved calibration cameras are incomplete')
    unreal.AutomationLibrary.finish_loading_before_screenshot()
    unreal.EditorPythonScripting.set_keep_python_script_alive(True)
    state = {'index':0,'task':None,'started':time.monotonic(),'next':time.monotonic()+3,'images':[]}
    def tick(delta):
        try:
            if time.monotonic()-state['started'] > 180:
                raise RuntimeError('Calibration screenshot timeout')
            if time.monotonic() < state['next']:
                return
            if state['task'] is not None:
                if not state['task'].is_task_done():
                    return
                path = folder/(names[state['index']]+'.png')
                if not path.exists() or path.stat().st_size < 1000:
                    raise RuntimeError(f'Missing screenshot {path}')
                state['images'].append({'file':str(path),'sha256':hashlib.sha256(path.read_bytes()).hexdigest()})
                state['task']=None; state['index']+=1; state['next']=time.monotonic()+1
            if state['index']==len(names):
                with (folder/'capture-receipt.json').open('x') as stream:
                    stream.write(json.dumps({'author':AUTHOR,'images':state['images'],'identity':identity,
                        'map_path':MAP_PATH,'observed_cameras':observed_cameras,
                        'evidence_class':'editor camera stills; no runtime movement or human acceptance'},indent=2)+'\n')
                unreal.log('[ECHOES_SPATIAL_CAPTURE_READY] images='+str(len(names)))
                unreal.unregister_slate_post_tick_callback(handle)
                unreal.EditorPythonScripting.set_keep_python_script_alive(False)
                return
            if state['task'] is None:
                name = names[state['index']]
                path = folder/(name+'.png')
                if path.exists():
                    raise RuntimeError('Refusing to overwrite prior screenshot evidence')
                # Fixture actors are editor-only; game view intentionally excludes them.
                state['task']=unreal.AutomationLibrary.take_high_res_screenshot(1920,1080,str(path),cameras[name],delay=1.0,force_game_view=False)
                if not state['task'].is_valid_task():
                    raise RuntimeError('Unreal refused screenshot task')
        except Exception:
            (folder/'capture-error.txt').write_text(traceback.format_exc())
            unreal.log_error(traceback.format_exc())
            unreal.unregister_slate_post_tick_callback(handle)
            unreal.EditorPythonScripting.set_keep_python_script_alive(False)
    handle = unreal.register_slate_post_tick_callback(tick)


if __name__ == '__main__':
    config = load_source()
    plan = build_plan(config)
    try:
        import unreal
    except ImportError:
        parser = argparse.ArgumentParser(description=__doc__)
        parser.add_argument('--check',action='store_true',required=True)
        parser.parse_args()
        print(json.dumps({key:plan[key] for key in ('cell_cm','cliff_step_cm','ramps','clearance','cameras')},indent=2))
    else:
        if os.environ.get('ECHOES_SPATIAL_CAPTURE')=='1':
            capture()
        else:
            generate(config,plan)
