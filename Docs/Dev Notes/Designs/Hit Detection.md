- Melee is solved for now, uses sphere trace
- Need to define amount of ticks, should be adjustable
	- Aiming for 60 tick rate
	- Seems UE has a max tick rate option for servers [post](https://forums.unrealengine.com/t/dedicated-server-tick-rate/320347)
- For magic with projectiles there are a few different use cases:
	- Single projectile; this is simple, just use the built-in projectile class
	- Multiple projectiles;
		- Probably best to use multiple of the main projectile class and place them procedurally
		- Eventual todo for Niagara effects is to match the hit boxes to the visuals but that's going to be down the road
		- Define options for placement of projectiles
			- Amount
			- Specified Projectile can define direction, speed, gravity
			- Shape: Cone, Fan, Line, Rectangle, Ring
			- Lifetime: Default of 0 = until hit or some default max
- For persistent area effects, we should spawn an actor with a collision volume and use oncomponentoverlapbegin
	- For volumes which deal ticking effects, we apply the gameplay effect to any players entering the area and remove it in the corresponding end event
	- Use GameplayEffect with duration and rate to set up tick
- For HitScan attacks we need separate handling
- Modifiers:
	- SpawnLocation: Needs to have options
		- Spell focus: fire from the spell focus' position in direction of camera
		- Player: simple case, spawn at player and ignore them
		- PreviousAbility: Spawn at location of previous ability
			- Use case is Ice Wall, shards should start from location of the wall
	- SpawnOffset for multi-projectile scenarios

## How to Add Hitscan?
UAttackBaseGameplayAbility has HitType already, so it it's HitType::HitScan we use that conditional logic.
- In ABattlemageTheEndlessCharacter::ProcessInputAndBindAbilityCancelled use a new HandleHitScan function
- 