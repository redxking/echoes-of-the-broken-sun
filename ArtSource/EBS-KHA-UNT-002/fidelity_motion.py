"""Riftstalker authored motion source; presentation only. Author: Angelis Pseftis.

The clips deliberately contain no root translation. EchoesSimCore remains the source
of world movement, facing, attacks and adaptation. ``motion_contract`` is the bridge
for an Unreal animation graph: it describes the nominal speed/cadence and the
Carapace play-rate rather than trying to make a cosmetic root track authoritative.
"""
import math

TICKS_PER_SECOND = 20.0
BASE_SPEED_CM_S = 410.0
CARAPACE_SPEED_CM_S = 328.0
WEAPON_COOLDOWN_TICKS = 22
MOLT_TICKS = 80
# Nine import frames per gait loop. A 3-frame planted stance makes the local foot
# travel precisely -410 cm/s under the externally moving root; the remaining six
# frames lift and return it to the next contact. The endpoints are inside the
# restrictive rear-leg IK range at the traced standing rest pose.
LOCOMOTION_DURATION_S = 9.0 / 30.0
STANCE_DURATION_S = 3.0 / 30.0
SWING_DURATION_S = LOCOMOTION_DURATION_S - STANCE_DURATION_S
LOCAL_FOOT_REAR_CM = -1.0
LOCAL_FOOT_FORWARD_CM = 40.0
ANKLE_TARGET_Z_CM = 27.0  # One-centimetre clearance absorbs linear key interpolation at the toe.
LOCOMOTION_NOMINAL_DISTANCE_CM = BASE_SPEED_CM_S * LOCOMOTION_DURATION_S


def motion_contract():
    """Runtime-facing presentation facts; this function has no simulation side effects."""
    return {
        "root_motion": "EXTERNAL_SIMCORE_TRANSLATION",
        "base_speed_cm_s": BASE_SPEED_CM_S,
        "carapace_speed_cm_s": CARAPACE_SPEED_CM_S,
        "carapace_play_rate": CARAPACE_SPEED_CM_S / BASE_SPEED_CM_S,
        "locomotion_nominal_distance_cm": LOCOMOTION_NOMINAL_DISTANCE_CM,
        "locomotion_duration_s": LOCOMOTION_DURATION_S,
        "locomotion_nominal_speed_cm_s": LOCOMOTION_NOMINAL_DISTANCE_CM / LOCOMOTION_DURATION_S,
        "weapon_cooldown_ticks": WEAPON_COOLDOWN_TICKS,
        "weapon_cooldown_s": WEAPON_COOLDOWN_TICKS / TICKS_PER_SECOND,
        "molt_ticks": MOLT_TICKS,
        "molt_duration_s": MOLT_TICKS / TICKS_PER_SECOND,
        "ability": {"status": "NOT_APPLICABLE", "reason": "SPEC-UNIT-006 grants Slipfire, a movement-and-attack rule, not an active ability."},
        "reduced_motion": "Runtime may snap facing; it does not alter deterministic simulation movement or attacks.",
    }


def limb_angles(base, tag, ankle_x, ankle_z, body_z):
    """Solve one X/Z leg chain using ebs_skelkit's Unreal pitch convention."""
    bones = {n: h for n, p, h, why in base.BONES}
    h, k, a = (bones[tag + "_" + part] for part in ("upper", "lower", "foot"))
    l1 = math.hypot(k[0] - h[0], k[2] - h[2])
    l2 = math.hypot(a[0] - k[0], a[2] - k[2])
    dx, dz = ankle_x - h[0], ankle_z - body_z - h[2]
    distance = math.hypot(dx, dz)
    if not abs(l1 - l2) + 1e-5 < distance < l1 + l2 - 1e-5:
        raise ValueError(f"{tag}: unreachable ankle target {distance:.3f}, reach {abs(l1-l2):.3f}..{l1+l2:.3f}")
    rest1, rest2 = math.atan2(k[2] - h[2], k[0] - h[0]), math.atan2(a[2] - k[2], a[0] - k[0])
    rest_delta = math.atan2(math.sin(rest2 - rest1), math.cos(rest2 - rest1))
    delta = math.copysign(math.acos(max(-1, min(1, (distance * distance - l1 * l1 - l2 * l2) / (2 * l1 * l2)))), rest_delta)
    theta1 = math.atan2(dz, dx) - math.atan2(l2 * math.sin(delta), l1 + l2 * math.cos(delta))
    upper, lower = math.degrees(theta1 - rest1), math.degrees(delta - rest_delta)
    return upper, lower, -upper - lower


def _key_leg_targets(clip, base, t, phase, body_z=0.0):
    """Key a diagonal gait. During stance, local x is exactly -410 cm/s.

    With external root translation +410 cm/s, this makes a planted foot have zero
    world-space velocity. The swing phase is deliberately not asserted as planted.
    """
    for tag, _ax, _ay, foot_x, _foot_y in base.LEGS:
        q = (phase + (0.0 if tag in ("fl", "rr") else 0.5)) % 1.0
        stance_fraction = STANCE_DURATION_S / LOCOMOTION_DURATION_S
        if q <= stance_fraction:
            x, lift = foot_x + LOCAL_FOOT_FORWARD_CM - BASE_SPEED_CM_S * q * LOCOMOTION_DURATION_S, 0.0
        else:
            swing_u = (q - stance_fraction) / (1.0 - stance_fraction)
            x = foot_x + LOCAL_FOOT_REAR_CM + (LOCAL_FOOT_FORWARD_CM - LOCAL_FOOT_REAR_CM) * (1.0 - math.cos(math.pi * swing_u)) / 2.0
            lift = 9.0 * math.sin(math.pi * swing_u)
        upper, lower, foot = limb_angles(base, tag, x, ANKLE_TARGET_Z_CM + lift, body_z)
        clip.key(tag + "_upper", t, (upper, 0.0, 0.0))
        clip.key(tag + "_lower", t, (lower, 0.0, 0.0))
        clip.key(tag + "_foot", t, (foot, 0.0, 0.0))


def _key_neutral_legs(clip, base, t, body_z=0.0):
    for tag, _ax, _ay, foot_x, _foot_y in base.LEGS:
        upper, lower, foot = limb_angles(base, tag, foot_x, ANKLE_TARGET_Z_CM, body_z)
        clip.key(tag + "_upper", t, (upper, 0.0, 0.0))
        clip.key(tag + "_lower", t, (lower, 0.0, 0.0))
        clip.key(tag + "_foot", t, (foot, 0.0, 0.0))


def clips(base):
    """Complete applicable SPEC-ART-002 set plus retained compatibility clips."""
    result = []

    def new(name, duration, loop, purpose):
        clip = base.skel.AnimationClip(name, duration, loop=loop, purpose=purpose)
        result.append(clip)
        return clip

    def key_body(clip, t, z=0.0, pitch=0.0, yaw=0.0, roll=0.0):
        clip.key("body", t, (pitch, yaw, roll), translation_cm=(0.0, 0.0, z))

    idle = new("idle", 2.0, True, "Idle vigilance: breathing carapace and restrained caster scan.")
    for t, z, yaw in ((0.0, 0.0, 0.0), (0.5, 1.4, 2.0), (1.0, 0.0, 0.0), (1.5, 1.4, -2.0), (2.0, 0.0, 0.0)):
        key_body(idle, t, z); idle.key("caster_yaw", t, (0.0, yaw, 0.0)); _key_neutral_legs(idle, base, t, z)

    def locomotion(clip):
        # 60 Hz keys reduce world-space contact drift introduced by linear Euler
        # interpolation between analytic IK solutions; clip duration remains 30fps-safe.
        for i in range(19):
            u, t = i / 18.0, i / 18.0 * clip.duration_s
            key_body(clip, t); _key_leg_targets(clip, base, t, u)

    move = new("move", LOCOMOTION_DURATION_S, True, "In-place diagonal locomotion. SimCore translates at 410 cm/s; 123 cm nominal root travel per loop.")
    locomotion(move)
    fire = new("fire_on_the_move", LOCOMOTION_DURATION_S, True, "Slipfire: exact locomotion tracks plus a caster-only firing pulse; damage remains authoritative.")
    locomotion(fire)
    for t, yaw, pitch in ((0.0, 0.0, 0.0), (0.09, 3.0, -6.0), (0.13, 0.0, 2.0), (0.18, 0.0, 0.0), (LOCOMOTION_DURATION_S, 0.0, 0.0)):
        fire.key("caster_yaw", t, (0.0, yaw, 0.0)); fire.key("caster_pitch", t, (pitch, 0.0, 0.0))

    for name, direction in (("turn_left", 1.0), ("turn_right", -1.0)):
        turn = new(name, 13.0 / 30.0, False, f"Acquire a new facing with a {name.split('_')[1]}-biased carapace settle.")
        for t, yaw in ((0.0, 0.0), (0.20, 28.0 * direction), (13.0 / 30.0, 56.0 * direction)):
            key_body(turn, t, yaw=yaw); _key_neutral_legs(turn, base, t)

    acquire = new("acquire", 10.0 / 30.0, False, "Caster searches and settles on an acquired hostile; no simulation target is inferred.")
    for t, yaw, pitch in ((0.0, 0.0, 0.0), (0.16, 16.0, -4.0), (10.0 / 30.0, 20.0, -5.0)):
        key_body(acquire, t); acquire.key("caster_yaw", t, (0.0, yaw, 0.0)); acquire.key("caster_pitch", t, (pitch, 0.0, 0.0)); _key_neutral_legs(acquire, base, t)

    windup = new("windup", 7.0 / 30.0, False, "Caster retracts before an authoritative shard-spike attack.")
    attack = new("attack", 4.0 / 30.0, False, "Caster snap marks a scheduled damage/VFX event only when combat authorizes it.")
    recovery = new("recovery", 0.3, False, "Caster returns to its travel-safe resting line after a shard spike.")
    for clip, keys in ((windup, ((0.0, 0.0), (0.12, 8.0), (7.0 / 30.0, 12.0))), (attack, ((0.0, 12.0), (0.045, -11.0), (4.0 / 30.0, 2.0))), (recovery, ((0.0, 2.0), (0.14, -2.0), (0.3, 0.0)))):
        for t, pitch in keys:
            key_body(clip, t); clip.key("caster_pitch", t, (pitch, 0.0, 0.0)); _key_neutral_legs(clip, base, t)

    hit = new("hit", 0.30, False, "Brief carapace impact recoil that returns control to the existing simulation state.")
    for t, z, yaw in ((0.0, 0.0, 0.0), (0.10, 2.0, -3.0), (0.30, 0.0, 0.0)):
        key_body(hit, t, z, yaw=yaw); _key_neutral_legs(hit, base, t, z)

    molt = new("molt", MOLT_TICKS / TICKS_PER_SECOND, False, "80-tick public adaptation transition. It shows vulnerability; it grants no active combat ability.")
    for i in range(17):
        u, t = i / 16.0, i / 16.0 * molt.duration_s
        body_z = -28.0 * math.sin(math.pi * u) ** 2
        key_body(molt, t, body_z); _key_neutral_legs(molt, base, t, body_z)
        molt.key("caster_yaw", t, (0.0, 4.0 * math.sin(2.0 * math.pi * u), 0.0))

    death = new("death", 1.8, False, "Legs fold under the body into a non-colliding cosmetic remains pose.")
    for i in range(13):
        u, t = i / 12.0, i / 12.0 * death.duration_s
        body_z = -44.0 * (u * u * (3.0 - 2.0 * u))
        key_body(death, t, body_z); _key_neutral_legs(death, base, t, body_z)
        death.key("caster_pitch", t, (7.0 * u, 0.0, 0.0))

    acknowledge = new("selection_ack", 0.5, False, "Selection acknowledgment: a readable caster acknowledgement, no capability claim.")
    for t, yaw, pitch in ((0.0, 0.0, 0.0), (0.16, -10.0, -5.0), (0.30, 8.0, 3.0), (0.5, 0.0, 0.0)):
        key_body(acknowledge, t); acknowledge.key("caster_yaw", t, (0.0, yaw, 0.0)); acknowledge.key("caster_pitch", t, (pitch, 0.0, 0.0)); _key_neutral_legs(acknowledge, base, t)
    return result
