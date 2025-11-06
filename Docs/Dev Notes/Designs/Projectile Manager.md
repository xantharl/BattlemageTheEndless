*Note: This class does not interact with Hit Scan or Melee attacks*
*Note: The various spawn shapes probably differ enough in required to warrant being classes of their own, but let's get something working first*
# What does it do?
## Spawn Projectiles
The entry point will be one of two `SpawnProjectiles` functions, one based on an actor and another based on previous ability's Transform.
The spawning of projectiles will support a number of options based on struct `FProjectileConfiguration`.
### Projectile Configuration
- See Code for details on each field
### Projectile Spawn Shapes
- None: Spawns at the requested location with no transformation beyond the projectile's offset. If `Amount` is more than 1, spawns an NGon of projectiles (or a line for 2)
- Cone: Requires 7 or more projectiles (one for center and each edge of a quadrilateral plane projected outward, spawns projectiles every Amount / Spread degrees, working from center out
- Fan: Similar to cone but only on one axis, requires 3 or more projectiles
- Line: Spawns projectiles in a line perpendicular to the spawn location, the line's length is derived from the hypotenuse of the triangle resulting from Spread angle and offset distance
- InwardRing: Spawns Projectiles in a ring around the spawn location and faces them inward
- OutwardRing: the same but outward
- Rain: Spawns projectiles in a radius of `SpawnRadius` at a fixed height above the ground shooting downward 
- Erruption: Spawns projectiles in a radius of `SpawnRadius` on the ground

*Note: We will need to store the transform of the last activated ability, probably on the player*
### On Hit Effects
#### Instantaneous Effects
For non-persistent HitTypes we will subscribe to the projectile's on hit event and apply any effects from the spawning `GameplayAbility` to the Hit Actor if it is a BMageCharacter
#### Persistent Effects
Persistent effects need to spawn an Actor of type `UPersistentAreaEffect` with a Collision volume set to Overlap All. 
- After spawn the OnBeginOverlapActor event will be subscribed to and any actor entering the area will have the effects of the GameplayAbility applied to it
- While in the area, if the effect(s) would wear off, their durations are refreshed
- Upon exiting the area, the effects wear off after their Duration
- *We need a persistent effect type (only in area vs lingering)*
# Where does it live?
Players create projectiles by using abilities, so let's track from the player level. This also gives the benefit of easy access to the AbilitySystemComponent when invoking methods on the projectile manager from the player.
Storing at the player level implies a single instance per player, so we need a data structure which easily supports tracking many projectiles from multiple ability activations.
# What does it store?
## Projectile Information
The manager needs to know about all projectiles created by the owning actor, along with details of the GameplayActivity that spawned them. 
Specifically, it needs to know about the EffectsToApply field in UAttackBaseGameplayAbility so it can properly apply on hit effects.
## Affected Actors
For the purposes of abilities with multiple projectiles, the Projectile Manager needs to know which actors have been hit by this instance of the ability. This means we need some sort of GUID to track each activation of an ability (as we're instancing per actor in most cases)
### Data Structure Design
Defines Struct FUniqueAbilityInstanceProjectiles
- GUID for the ability instance
- Ability Spec (to avoid needing an AbilitySystemComponent reference)
- TArray of projectiles
- TArray of Pawns who have been hit by this instance, let the on hit handle the check if the Pawn is a BMage character

TArray of FUniqueAbilityInstanceProjectiles