**Note**: *The BT behavior is currently running every tick, Tick Interval doesn't seem to work. I'm filing this away under "I'll worry about it when it's a problem"*

The setup to get this guy to actually aim at things is pretty complicated.

First of all, there's a few Blackboard Keys that come into play here:
1. `TargetActor`:
	- Stores an Actor to shoot at
	- Set by the AI Controller in function `HandleSightSense`, which is called when the Skeleton detects a target
	- Must be set to initiate this whole attack flow
2. `ActiveAbilityInstance`: 
	- Stores a pointer to an activate instance of the ability in use
	- Set by `BTTask_ActivateChargedAbility`
	- Used by `BTTask_PickAimAtLocation` to retrieve projectile information and calculate ideal launch angle for the projectile
3. `AttackAngle`: 
	- Stores an `FRotator` with the ideal attack direction
	- Set by `BTTask_PickAimAtLocation`, uses projectile information from `ActiveAbilityInstance` along with `TargetActor` for aim point
	- Used to set a `TargetRotation` var on the ABP, which in turn sets the `spine_01` bone's rotation (roll only) to angle the character's aim up and down
4. `LookAtLocation`: 
	- Stores an `FVector` representing a point at which the head should look
	- Used to set the head's look at position in the ABP's anim graph
	- Set by `BTTask_PickAimAtLocation` by projecting a 100 length vector in the direction of `AttackAngle`
5. `ChargeDuration`
	- Stores the duration to full charge for the target ability
	- Set by `BTTask_ActivateChargedAbility`
	- Used to determine appropriate time for the BT to wait before firing the bow

Secondly, make sure your SkeletalMesh has a socket called `AimSocket`

The ability flow is fairly straightforward, the BT performs the following actions:
1. Activate the ability to start the charge (draw)
	- This requires manual specification of the target ability when calling `BTTask_ActivateChargedAbility`
2. Until `ChargeDuration` seconds later, keep Aim on the target while waiting
3. Fire the bow by completing the ability

The Animation Blueprint flow is decidedly less intuitive, here's how it works:
1. On `Event Blueprint Update Animation` the event graph sets BB Key `AttackAngle` to a variable called `TargetRotation`
	- This is done because access the BB in the anim graph is not threadsafe
2. In the AnimGraph, after all other operations are performed, we have added two new ones:
	1. A `Transform (Modify) Bone` on `spine_01` uses `TargetRotation` to set the spine's roll, thus adjusting the bow's angle
	2. A `Look At` on bone `head` precisely(ish) points the head at the Skeleton's target

After all this is done, we have to apply a bit of fudging on the projectile, since the animation with the extra applied translations does not leave the aim point *quite* dead on (he likes to shoot left of target without this)
This is done in `UProjectileManager::GetOwnerLookAtTransform()` where we use the BB Key `AttackAngle` if the `OwnerCharacter` is an AI.

Note that this requires the `AimSocket` to function properly since that is what we use to get the base transform to spawn the projectiles at.