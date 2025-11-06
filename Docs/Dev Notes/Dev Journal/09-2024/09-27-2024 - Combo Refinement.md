Goals:

- Need to tune up weapon attacks, they're wonky right now
	- Implement action queueing instead of canceling for combos
		- Need to:
			- Check if ability is active before subscribing
			- Unsubscribe after
			- **That works for well timed actions, but clicking early doesn't wait**
			- **Fixed, order of operations issue**
	- Fix for finisher attack leaving character with roll
		- https://www.youtube.com/watch?v=f32fzAuJyZU
		- **Decided to ditch the offending animation for now**
- Add GAS supported health 
- Make spell hits do something
	- Implement effects on the spells
	- Status effects and/or extra damage

Worked: 4-4:30