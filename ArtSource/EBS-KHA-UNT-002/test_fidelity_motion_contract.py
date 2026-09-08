"""Animation-contract checks for the Riftstalker fidelity pilot.

These are source/rig checks. They deliberately do not claim rendered, gameplay, or
owner acceptance. Author: Angelis Pseftis.
"""
import math
import unittest

import build_fidelity_pilot as pilot


class FidelityMotionContractChecks(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.base = pilot.base
        cls.motion = pilot.fidelity_motion
        cls.by_name = {clip.name: clip for clip in cls.motion.clips(cls.base)}

    def test_complete_applicable_spec_art_002_inventory(self):
        required = {
            "idle", "move", "turn_left", "turn_right", "acquire", "windup",
            "attack", "recovery", "hit", "molt", "death", "selection_ack",
            "fire_on_the_move",
        }
        self.assertEqual(required, set(self.by_name))
        self.assertEqual(self.motion.motion_contract()["ability"]["status"], "NOT_APPLICABLE")
        self.assertIn("Slipfire", self.motion.motion_contract()["ability"]["reason"])

    def test_canon_timing_and_external_runtime_policy(self):
        contract = self.motion.motion_contract()
        self.assertEqual(contract["base_speed_cm_s"], 410.0)
        self.assertEqual(contract["carapace_speed_cm_s"], 328.0)
        self.assertEqual(contract["carapace_play_rate"], 0.8)
        self.assertEqual(contract["root_motion"], "EXTERNAL_SIMCORE_TRANSLATION")
        self.assertAlmostEqual(contract["locomotion_nominal_speed_cm_s"], 410.0)
        self.assertEqual(self.by_name["molt"].duration_s, 4.0)
        self.assertEqual(contract["weapon_cooldown_ticks"], 22)
        self.assertEqual(contract["weapon_cooldown_s"], 1.1)

    def test_loop_endpoints_and_slipfire_gait_are_exact(self):
        for name in ("idle", "move", "fire_on_the_move"):
            clip = self.by_name[name]
            self.assertTrue(clip.loop)
            for bone, keys in clip.tracks.items():
                self.assertEqual(keys[0].rotation_deg, keys[-1].rotation_deg, (name, bone))
                self.assertEqual(keys[0].translation_cm, keys[-1].translation_cm, (name, bone))
        for bone in self.by_name["move"].tracks:
            self.assertEqual(self.by_name["move"].tracks[bone], self.by_name["fire_on_the_move"].tracks[bone])

    def test_action_endpoints_are_meaningful(self):
        def span(name, bone, axis=0):
            return max(key.rotation_deg[axis] for key in self.by_name[name].tracks[bone]) - min(key.rotation_deg[axis] for key in self.by_name[name].tracks[bone])
        # Runtime owns facing. A fixed terminal yaw would double-apply its turn.
        for name in ("turn_left","turn_right"):
            self.assertEqual(self.by_name[name].tracks["body"][-1].rotation_deg[1],0.0)
            self.assertGreater(span(name,"body",axis=1),0.0)
        self.assertGreater(self.by_name["turn_left"].tracks["body"][1].rotation_deg[1],0.0)
        self.assertLess(self.by_name["turn_right"].tracks["body"][1].rotation_deg[1],0.0)
        for name in ("acquire", "windup", "attack", "recovery", "selection_ack"):
            self.assertGreater(span(name, "caster_pitch"), 0.0, name)
        self.assertGreater(span("hit", "body", axis=1), 0.0)
        self.assertLess(self.by_name["death"].tracks["body"][-1].translation_cm[2], -40.0)

    def test_leg_ik_is_grounded_at_authored_keys_with_actual_pitch_math(self):
        bones = {name: head for name, _parent, head, _purpose in self.base.BONES}
        for clip in self.by_name.values():
            for tag, _ax, _ay, _fx, _fy in self.base.LEGS:
                for upper, lower, foot in zip(
                    clip.tracks[tag + "_upper"], clip.tracks[tag + "_lower"], clip.tracks[tag + "_foot"]
                ):
                    self.assertEqual(upper.time_s, lower.time_s)
                    self.assertEqual(upper.time_s, foot.time_s)
                    h, k, a = (bones[tag + "_" + part] for part in ("upper", "lower", "foot"))
                    q = self.base.kit.v_add(self.base.kit.rot_y(self.base.kit.v_sub(a, k), lower.rotation_deg[0]), k)
                    q = self.base.kit.v_add(self.base.kit.rot_y(self.base.kit.v_sub(q, h), upper.rotation_deg[0]), h)
                    body = next(key for key in clip.tracks["body"] if key.time_s == upper.time_s)
                    q = self.base.kit.v_add(q, body.translation_cm)
                    self.assertGreaterEqual(q[2], self.motion.ANKLE_TARGET_Z_CM - 1e-6, (clip.name, tag, upper.time_s, q))
                    self.assertAlmostEqual(upper.rotation_deg[0] + lower.rotation_deg[0] + foot.rotation_deg[0], 0.0, places=7)

    def test_planted_stance_has_near_zero_world_space_velocity_at_410_cm_s(self):
        """The leg is local-space in-place, but a planted foot must not skate in world space."""
        clip = self.by_name["move"]
        bones = {name: head for name, _parent, head, _purpose in self.base.BONES}

        def world_ankle_x(tag, time_s):
            pose = self.base.sample_pose(clip, time_s / clip.duration_s)
            h, k, a = (bones[tag + "_" + part] for part in ("upper", "lower", "foot"))
            q = self.base.kit.v_add(self.base.kit.rot_y(self.base.kit.v_sub(a, k), pose[tag + "_lower"][0]), k)
            q = self.base.kit.v_add(self.base.kit.rot_y(self.base.kit.v_sub(q, h), pose[tag + "_upper"][0]), h)
            return q[0] + self.motion.BASE_SPEED_CM_S * time_s

        stance = self.motion.STANCE_DURATION_S
        for tag in ("fl", "rr"):
            anchor = world_ankle_x(tag, 0.0)
            for fraction in (1.0 / 6.0, 2.0 / 6.0, 3.0 / 6.0, 4.0 / 6.0, 5.0 / 6.0, 1.0):
                self.assertAlmostEqual(world_ankle_x(tag, stance * fraction), anchor, delta=0.12)
        # The opposite diagonal begins its stance half a cycle later.
        start = clip.duration_s / 2.0
        for tag in ("fr", "rl"):
            anchor = world_ankle_x(tag, start)
            for fraction in (1.0 / 6.0, 2.0 / 6.0, 3.0 / 6.0, 4.0 / 6.0, 5.0 / 6.0, 1.0):
                self.assertAlmostEqual(world_ankle_x(tag, start + stance * fraction), anchor, delta=0.12)


if __name__ == "__main__":
    unittest.main()
