Yup, it's been two months. Life has been a bit hectic. Let's get back on this train for real.

Goals:
- Figure out grappling
	- **Pushing this for now, want to go back to magic**
- Fixed health bar statuses
	- Health bar iteration 1 is done for enemies
- Niagara systems from statuses aren't removing after expiry
	- I think it's because the lifetime of cue is dropping the reference
	- Make the effect track it instead and manually delete
	- **Fixed** issue was that GAS creates separate cues for instant (on hit) and ongoing (periodic) effects, particles now start off and are explicitly enabled
- Magic To-Dos
	- Actually Use AttackBaseGameplayAbility::HitEffectActors
		- Now in use! But needs configuration
		- Also needs to respect snap to ground
	- Explore "Magic Orbs / Energy Cores - VFX"
		- 8, 15 could be good for fire
		- 11, 13, 17, 18, 19 have void potential
		- 16 could be nature
		- 21 for lightning but needs color change
		- 25 for earth?
	- Stylized Sword Seals VFX
	- Stylized Sword Trails VFX
	- Whooshes & Impacts 2

Worked: 2:30-6