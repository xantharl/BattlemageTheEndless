Goals:

- Need to tune up weapon attacks, they're wonky right now
	- Implement action queueing instead of canceling for combos
		- Need to use an ability task PlayMontageAndWait, see [docs](https://github.com/tranek/GASDocumentation?tab=readme-ov-file#473-using-ability-tasks)
		- **Done but janky**
		- Need to:
			- Check if ability is active before subscribing
			- Unsubscribe after
			- **That works for well timed actions, but clicking early doesn't wait**
	- Fix for finisher attack leaving character with roll
		- https://www.youtube.com/watch?v=f32fzAuJyZU
	- Added attack transition animations
- Add GAS supported health 
- Make spell hits do something
	- Implement effects on the spells
	- Status effects and/or extra damage

Worked: 4:30-6:30, 10:15-10:30