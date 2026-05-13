# Refactor: Collapse the BP `GA_*` wrapper on `UMovementAbility`

## Status
Proposed. Not started. A point-fix to the listen-server bug it caused was applied
in `BMageCharacterMovementComponent::TryStartAbilityFromEvent` — see "Why this
matters" below.

## Current architecture

Two parallel objects represent each movement ability (Dodge, Sprint, Slide,
Launch, Vault, WallRun):

1. **C++ instance** — e.g. `UDodgeAbility : UMovementAbility : UGameplayAbility`.
   Lives as a default subobject on `UBMageCharacterMovementComponent` inside the
   `MovementAbilities` TMap (`BMageCharacterMovementComponent.h:44`). It is
   **never** granted to the ASC — it's only ever reached through the movement
   component's TMap.
2. **BP shim** — e.g. `GA_Dodge`. Inherits directly from `UGameplayAbility`
   (not from `UMovementAbility`). This is the one `GiveAbility`'d to the ASC
   (`BattlemageTheEndlessCharacter.cpp:412`) and bound to player input.

`GA_Dodge::K2_ActivateAbilityFromEvent` walks the character → movement
component → `TryStartAbilityFromEvent` → finds the C++ instance in the TMap and
calls `Begin` on it, then binds `OnMovementAbilityEnd` back to `K2_EndAbility`
so the BP-side ability ends when the C++ one finishes. The BP also calls
`K2_CommitAbility` and `BMageAbilitySystemComponent::ResetActiveEffectPeriod`
on the dodge-charge regen GE.

## The smell

`UMovementAbility` has two activation-state concepts for the same logical
ability (`MovementAbility.cpp`):

- `IsGAActive()` — queries the ASC by tag to find a *different* ability spec
  (the BP one) and checks its active state.
- `ISMAActive()` — returns a local bool on the C++ instance.

That's a tell that activation lives in two places because the
ASC-registered instance and the gameplay-runs-here instance are different
objects.

## Why this matters (the bug it caused)

The BP-side activation graph calls `TryStartAbilityFromEvent` and depends on
its return value: an `IsValid` check gates `K2_CommitAbility`,
`ResetActiveEffectPeriod`, the `MovementAbility` var-set, and the
`OnMovementAbilityEnd` delegate bind.

`TryStartAbilityFromEvent` previously early-returned `nullptr` on the server
for any character that wasn't locally controlled. For a remote client on a
listen server, the server-side character is *not* locally controlled, so the
server skipped commit / cooldown / cost / delegate bind entirely. Only the
host (locally controlled on the server because it's the same machine) worked.

Point fix applied: removed the `!IsLocallyControlled()` early-return from
`TryStartAbilityFromEvent`. The function now mirrors `TryStartAbility` above
it, which never had the check.

### History of the guard

- **`486e3656` (Jan 7 2026, "working on replication of dodge")** introduced
  `TryStartAbilityFromEvent` with the *opposite* guard: `!HasAuthority()`
  early-returned on the **client**, log message "called on client." The
  intent was for event-triggered activation to run on the server, paired
  with the new `Server_RequestStartMovementAbility` RPC added in the same
  commit (`BMageCharacterMovementComponent.cpp:111`). That RPC still exists
  but ended up wired only for WallRun.
- **`3d239b10` (Jan 11 2026, "more dodge replication work and wiring up a
  struct for movement ability activation")** flipped the guard to
  `!CharacterOwner->IsLocallyControlled()` with the comment *"only the
  client should be running this code, the server uses an RPC to handle
  activation."* The log message string wasn't updated and still said
  "called on client"; the "called on server" wording in the working tree
  before this refactor was a later uncommitted local correction.

The design intent in 3d239b10 was: run the BP activation logic on the
client only, and let an RPC handle server-side bookkeeping. The fatal gap
is that no such RPC was actually built for Dodge — only WallRun uses
`Server_RequestStartMovementAbility`. Combined with the BP-side `IsValid`
check gating `K2_CommitAbility` and `ResetActiveEffectPeriod`, the server
never committed the ability for any remote client. The listen-server host
hid the bug because the host's character is locally controlled on the
server.

## Likely historical reason

`UMovementAbility` predates GAS integration. It was originally a plain
`Begin/Tick/End` component-held thing. When GAS was layered on, the easiest
path was a thin BP wrapper that owned the GAS-side concerns (input, cost,
cooldown, prediction key) and delegated to the existing C++ instance, instead
of moving all those movement instances into the ASC.

The TMap-on-the-movement-component stuck around because cross-ability
interactions in `OnMovementAbilityBegin` (`BMageCharacterMovementComponent.cpp:353`)
— slide cancels on dodge, vault cancels everything else, sprint force-ends
slide, etc. — are easier to write with direct pointers to siblings than by
querying the ASC.

## Proposed end state

Make `UDodgeAbility` (and each sibling) the granted GAS ability itself.
Override `ActivateAbility` / `EndAbility` on the C++ class to do what the BP
currently does. Drop `GA_Dodge` and its siblings, drop the TMap of
subobjects, or repopulate it on activate/end via the ASC if the
cross-ability logic still wants direct pointers.

Wins:

- One instance per net role; the listen-server-only bug stops being possible
  by construction.
- One activation-state concept; `IsGAActive` and `ISMAActive` collapse into
  one.
- One place to look when wiring cost / cooldown / charges (see
  [[project_ability_charges]]).

Costs:

- The cross-ability logic in `OnMovementAbilityBegin` / `OnMovementAbilityEnd`
  needs to either keep a lazy cache of active siblings or look them up via
  the ASC each transition.
- Each BP's serialized configuration (curves, transition durations, etc.)
  needs to migrate onto the C++ class CDO or onto a data asset the C++ class
  references.
- Touches every movement ability, not just Dodge. Scope is "the whole
  movement ability system," not a one-asset change.

## Related memory

- [[project_ability_charges]] — dodge-charges design that this refactor
  needs to preserve.
- [[feedback_gas_react_ability_netpolicy]] — net policy gotcha that informed
  this analysis.
