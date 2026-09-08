"""Executable pilot invariants, not art acceptance. Author: Angelis Pseftis."""
import unittest, math, json, pathlib, tempfile, struct, hashlib
import build_fidelity_pilot as p
b=p.base

class FidelityChecks(unittest.TestCase):
    def test_unreachable_foot_fails_explicitly(self):
        with self.assertRaisesRegex(ValueError,'unreachable'):
            p.fidelity_motion.limb_angles(b,'fl',10000,26,0)

    def test_ik_reconstructs_ankle_in_actual_meshkit_convention(self):
        bones={n:h for n,pa,h,why in b.BONES}
        for tag,ax,ay,fx,fy in b.LEGS:
            h,k,a=[bones[tag+'_'+name] for name in ('upper','lower','foot')]
            for tx,tz,dz in [(fx,26,0),(fx+8,32,-12),(fx,26,-36)]:
                up,lo,foot=p.fidelity_motion.limb_angles(b,tag,tx,tz,dz)
                q=b.kit.v_add(b.kit.rot_y(b.kit.v_sub(a,k),lo),k)
                q=b.kit.v_add(b.kit.rot_y(b.kit.v_sub(q,h),up),h)
                q=b.kit.v_add(q,(0,0,dz))
                self.assertAlmostEqual(q[0],tx,places=7);self.assertAlmostEqual(q[2],tz,places=7)
                self.assertAlmostEqual(up+lo+foot,0,places=7)

    def test_components_bind_without_fallback(self):
        for state in b.STATES:
            for lod in (0,1):
                m=p.fidelity_geometry.build(b.kit,b,lod,state);b.bind(m)
                self.assertTrue(all(getattr(poly,'bone',None) not in (None,'root') for poly in m.polygons))
                self.assertLessEqual(m.triangle_count(),7500 if lod==0 else 3200)
                self.assertGreaterEqual(m.bounds()[0][2],0.)
                colors=b.vertex_colors_for(m.components())
                self.assertEqual(set(colors),set(m.components()))
                self.assertEqual(any(c[1] for c in colors.values()),state=='carapace_molt')
                self.assertEqual(any(c[2] for c in colors.values()),state=='striker_molt')

    def test_generated_glb_has_rig_sockets_and_channel_boundaries(self):
        with tempfile.TemporaryDirectory() as td:
            p.generate(pathlib.Path(td))
            for f in pathlib.Path(td).glob('*.glb'):
                data=f.read_bytes();n=struct.unpack_from('<I',data,12)[0];g=json.loads(data[20:20+n])
                self.assertEqual(len(g['skins'][0]['joints']),22)
                names={node.get('name') for node in g['nodes']}
                self.assertTrue({'SOCKET_'+n for n in b.SOCKETS}<=names)
                self.assertEqual({a['name'] for a in g['animations']},{'idle','move','fire_on_the_move','molt','death'})
                for prim in g['meshes'][0]['primitives']:
                    self.assertIn('COLOR_0',prim['attributes']);self.assertIn('JOINTS_0',prim['attributes'])
                self.assertFalse(any(str(name).startswith(('UBX_','UCX_','USP_','UCP_')) for name in names))

    def test_firing_preserves_limb_motion(self):
        clips={c.name:c for c in p.fidelity_motion.clips(b)}
        for bone in clips['move'].tracks:
            self.assertEqual(clips['move'].tracks[bone],clips['fire_on_the_move'].tracks[bone])

    def test_repeated_generation_is_byte_identical(self):
        with tempfile.TemporaryDirectory() as a,tempfile.TemporaryDirectory() as c:
            p.generate(pathlib.Path(a));p.generate(pathlib.Path(c))
            for f in pathlib.Path(a).glob('*'):
                self.assertEqual(f.read_bytes(),(pathlib.Path(c)/f.name).read_bytes(),f.name)

    def test_contact_across_states_lods_and_interpolated_poses(self):
        skeleton=b.build_skeleton()
        for state in b.STATES:
            for lod in (0,1):
                m=p.fidelity_geometry.build(b.kit,b,lod,state);b.bind(m)
                for clip in p.fidelity_motion.clips(b):
                    for i in range(97):
                        posed=b.skel.pose_mesh(m,skeleton,b.sample_pose(clip,i/96))
                        self.assertGreaterEqual(posed.bounds()[0][2],-.01,(state,lod,clip.name,i))

if __name__=='__main__':unittest.main()
