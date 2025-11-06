I want to bang out a few magic features with lower levels of effort. But first let's fix up the anim issues on exit of slide.

Goals:
- Figure out how to determine target state of run vs crouch coming out of transition
	- Just check whether we're crouching? Might be forceable crouched still, use bShouldUncrouch
	- Fixed up with some transition updates and slight crouch rework
- Figure out grappling
	- **Pushing this for now, want to go back to magic**
- Magic To-Dos
	- Figure out control scheme
		- **Done** see [[Control Scheme]]
		- Implement control scheme changes
			- Done as much as can be without more features
	- Impl "Aim" mode
		- Zoom in camera by reducing FOV
Random To-Do:
- Actually Use AttackBaseGameplayAbility::HitEffectActors

Worked: 1-3:45