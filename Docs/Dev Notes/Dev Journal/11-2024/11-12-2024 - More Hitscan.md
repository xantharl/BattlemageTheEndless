Let's use the effect to make some chain lightning, no, I'm not super sure on the particulars yet either.

Goals:
- Implement chain effects
	- Need to make start and end locations tied to an actor each **Implemented**
		- Make an actor which has the following:
			- Reference to originator
			- Reference to target
			- Niagara Effect instance
		- Make the actor update start and end position of the system on tick
		- Make the actor dispose of itself after the niagara system's lifetime
	- Needs to happen on hit **Implemented**
		- Add spawn of the new actor class to the hit scan handling
	- Need to support subsequent chain effects **Free since I handled the former in ApplyEffects**
		- Handle the subsequent chains by including the actor creation in the logic which schedules the hits
Random To-Do:
- Actually Use AttackBaseGameplayAbility::HitEffectActors
Worked: 6:30-9:30