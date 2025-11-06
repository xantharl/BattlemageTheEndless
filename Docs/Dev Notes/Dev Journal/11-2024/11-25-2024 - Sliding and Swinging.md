Got most of the way through a refactor of sliding to last until stopped (to support accel/decel)

Goals:
- Make sliding last when on a downslope
	- Implemented but needs testing
	- Completely reworked now lol
	- Accel/Decel work and take the slope into account
	- Momentum is conserved on exit until landing 
- Make sliding gain speed on a downslope
	- Done!
- Figure out how to determine target state of run vs crouch coming out of transition
- Figure out grappling
Random To-Do:
- Actually Use AttackBaseGameplayAbility::HitEffectActors
Worked: 4-4:45, 5:30-6:30