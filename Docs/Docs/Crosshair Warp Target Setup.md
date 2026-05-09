# Crosshair Warp Target Setup

`UAttackBaseGameplayAbility` can push a Motion Warping target derived from where the player's crosshair is pointing. This is used to compensate for the third-person camera's lateral offset (attacks would otherwise swing slightly off-screen) and optionally to lunge the character toward the target.

The feature is opt-in per ability and requires three pieces of setup: the character, the ability, and the montage. All three must agree or the warp will silently no-op or destroy the animation's root motion.

## 1. Character setup

The owning character must have a `UMotionWarpingComponent`. `ABattlemageTheEndlessCharacter` does not add one in C++ — add it on the BP:

1. Open `BP_BattlemageCharacter` (or whichever BP your character uses).
2. **Add Component** → **Motion Warping**.
3. No properties need to change from defaults.

If the component is missing, `UpdateCrosshairWarpTarget` logs a warning and returns without pushing a target.

## 2. Ability setup

On any BP child of `UAttackBaseGameplayAbility`, set the **MotionWarping** category properties:

| Property | Purpose |
|---|---|
| `bUseCrosshairWarpTarget` | Master toggle. Off = legacy behavior, no warp target pushed. |
| `bWarpTranslation` | `false` = rotation-only (character pivots in place to face the crosshair). `true` = lunge toward the crosshair, clamped to `MaxRange`. |
| `WarpTargetName` | Identifier the montage notify uses to look up the target. Must match the modifier config (see below). Default: `AttackTarget`. |

`MaxRange` (on the parent class, `Projectile` category) caps how far the character can lunge when `bWarpTranslation = true`. It does **not** affect the camera trace distance, only the target location.

## 3. Montage setup

On the `FireAnimation` montage:

1. Add a **Motion Warping** notify state over the window where the warp should occur. Typically this is the wind-up + swing region — start it before the swing's directional commit, end it once the strike has played out. Anywhere outside this window, root motion plays unchanged.
2. In the notify's details, set **Root Motion Modifier** to a config asset (or inline instance). Use **`RootMotionModifier_SkewWarp_Config`** for almost all cases — it deforms existing root motion to hit the target rather than replacing it.
3. On the modifier config:
    - **Warp Target Name** — must equal the ability's `WarpTargetName`.
    - **`bWarpTranslation`** — must equal the ability's `bWarpTranslation`. **This is the most common misconfiguration.** If the modifier warps translation but the ability is rotation-only, the warp target is the character's current location and the animation's forward root motion is collapsed to zero — i.e., the lunge disappears.
    - **`bWarpRotation`** — leave on (default).
    - **`bIgnoreZAxis`** — leave on (default), so vertical motion isn't flattened.

Avoid `RootMotionModifier_SimpleWarp_Config` — it linearly interpolates from start to target and discards the source root motion entirely.

## Quick reference: pairing the flags

| Ability `bWarpTranslation` | Modifier `bWarpTranslation` | Result |
|---|---|---|
| `false` | `false` | Yaw is corrected toward crosshair, original root motion plays. ✅ |
| `false` | `true` | Forward root motion collapses to zero (target is character's current location). ❌ |
| `true` | `false` | Yaw corrected, no lunge — translation passes through unchanged. Probably not what you want. |
| `true` | `true` | Yaw corrected and character lunges up to `MaxRange` toward crosshair. ✅ |

## Debugging

`UpdateCrosshairWarpTarget` logs a warning to `LogTemp` for each invalid-config early exit:

- `CurrentActorInfo is null` — internal, shouldn't happen during normal activation.
- `OwnerActor is not an ABattlemageTheEndlessCharacter` — ability was activated on a non-player actor.
- `character ... has no UMotionWarpingComponent` — see step 1.
- `no active camera on character ...` — `GetActiveCamera()` returned null; check the character's `FirstPersonCamera` / `ThirdPersonCamera`.

If no warning fires but the warp does nothing, the issue is almost certainly in the montage notify or the modifier config (step 3).
