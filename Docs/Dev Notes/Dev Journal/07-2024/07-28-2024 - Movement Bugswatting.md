It's been a fairly long haul on movement, but I think we're almost to a good enough state

- Fix wall climb bug where climbing happens from way too far below
	- **Unable to reproduce reliably**
- Identify any other movement fine tuning
- Still running into slowdown on walls
	- **Think I fixed it** issue was inconsistent calculation in some quadrants plus bad handling of deciding which side the wall was on
- Walking off a wall away from it can trigger a wallrun
	- **Fixed** by adding a max yaw diff to start wallrun
- Think I want jump ascend and descend to have different gravity
	- **Done**
- Make vaulting only possible while falling (in the air)
	- **Done**
- Allow sliding out of a dodge
	- **Done**
- Subsequently; allow launch jump out of such slides
	- **Done**
- Make sure wall runs are only allowed on walls (currently possible on characters)
	- **Done** applied to vault as well

Start: 2:30
Finish: 4:30

Thoughts: Productive couple of hours, think I finally got wallrun direction solidified and sorted out a litany of other small changes