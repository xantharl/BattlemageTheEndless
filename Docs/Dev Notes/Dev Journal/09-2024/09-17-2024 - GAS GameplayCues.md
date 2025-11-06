Goals:
- Fix for finisher attack leaving character with roll
	- https://www.youtube.com/watch?v=f32fzAuJyZU
- Have a working magic combo
	- Needs collision for particles
		- **WIP for ice wall**
	- Keep gameplay ability in a state where I can cancel particle effects on next move
		- **WIP via GC refactor**
	- Extend GC Actor base class to support specific use case with niagara so that I can pull the instance from it
		- **Done**
		- Combo works complete with cancelation
	- Started on frozen blade, need to add enum for attach type (character or equipment)
		- Will require reference to bmagecharacter
		- use choices of character, primary, secondary

Worked: 4-5, 5:15-5:45