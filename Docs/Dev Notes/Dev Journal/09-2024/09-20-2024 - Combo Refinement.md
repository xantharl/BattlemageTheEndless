Goals:

- Have a working magic combo
	- Need to tune up weapon attacks, they're wonky right now
		- Implement action queueing instead of canceling for combos
			- Need to use an ability task PlayMontageAndWait, see [docs](https://github.com/tranek/GASDocumentation?tab=readme-ov-file#473-using-ability-tasks)
		- Fix for finisher attack leaving character with roll
			- https://www.youtube.com/watch?v=f32fzAuJyZU
		- **Made changes to keep abilities alive, need to figure out how to auto-remove duration effects**
	- Add GAS supported health 
	- Make spell hits do something
		- Implement effects on the spells
		- Status effects and/or extra damage

Worked: 1:30-2:45, 3-3:45