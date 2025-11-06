Goals:

- Have a working magic combo
	- Needs collision for particles
		- Collision works from a local standpoint, need custom impl of projectiles for 
	- Keep gameplay ability in a state where I can cancel particle effects on next move
		- Working now woohoo
	- Started on frozen blade, need to add enum for attach type (character or equipment)
		- Will require reference to bmagecharacter
		- use choices of character, primary, secondary
		- **Done**
	- Added ability to switch spells with scroll
	- Need to tune up weapon attacks, they're wonky right now
		- Implement action queueing instead of canceling for combos
			- **WIP**
			- Need to use an ability task PlayMontageAndWait, see [docs](https://github.com/tranek/GASDocumentation?tab=readme-ov-file#473-using-ability-tasks)
		- Fix for finisher attack leaving character with roll
			- https://www.youtube.com/watch?v=f32fzAuJyZU

Worked: 3:15-5:15, 7:45-9:15