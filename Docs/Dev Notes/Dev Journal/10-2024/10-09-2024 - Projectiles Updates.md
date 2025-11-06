Goals:
- Work on spell ability expansion as detailed in [[Spell System]]
	- Implementing projectile system
	- Implement Shapes
		- **Done**
		- Inward ring isn't killing projectiles properly since they ignore each other, make an exception for this case
			- **Done** *but not tested*
	- Implement example spells for each shape
		- **Done**
	- Add "Aim" spawn location
		- **Change of heart, don't see a good use case for this as a projectile, seems like hitscan behavior**
	- Implement other properties of Projectile Configuration
		- Make Persistent Area Effect a GA member rather than projectile config?
	- Refactor all the hard coded socket names into global constants
- Make spell hits do something
	- Base spells working, now to set up a more complex one
- Spent a while writing up spell class ideas

Worked: 4-5:45, 6-6:30, 9-10