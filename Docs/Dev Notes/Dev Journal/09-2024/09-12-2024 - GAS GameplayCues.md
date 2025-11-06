Today was figuring out why my gameplay cues weren't working repeatedly. Turns out they don't automatically recycle when the effect applying them ends and you need to manually override WhileActive and OnRemoved to manage it

Goals:
- Fix for finisher attack leaving character with roll
	- https://www.youtube.com/watch?v=f32fzAuJyZU
- Have a working magic combo
	- Needs collision for particles
		- **WIP for ice wall**
	- Keep gameplay ability in a state where I can cancel particle effects on next move
		- **WIP via GC refactor**
	- Extend GC Actor base class to support specific use case with niagara so that I can pull the instance from it

Worked: 3-5:15