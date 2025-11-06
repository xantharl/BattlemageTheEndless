I'm officially deciding to simplify these entries for my own sanity
Goals:
- Get wallrun animation mirroring properly with legs moving
	- **Issue seems to be the control rig mapping portion, i frankly have no clue why it's here**
- Mirroring use cases:
	- Animation was made with wall to the right
	- If not left handed and not wall to left, no mirroring, false, false = false
	- if not left handed and wall to left, mirror, false, true = true
	- if left handed and not wall to left, mirror, true, false = true
	- if left handed and wall to the left, no mirroring, true, true = false
- It was simple after all, just had to mirror if wall wasn't to left
- Did a bunch more refactoring to movement

Worked: 3:15-7, 8:45-10