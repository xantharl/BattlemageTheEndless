Goals:
- Fix bug with projectile colliding with character
	- I think it's got to do with the actors attached to the character not ignoring the projectiles
	- **Attempted fix broke build**
	- Second try, **GetAttachedActors has a reset array param that defaults to true**
	- Lesson: Always check your assumptions (in this case, that the character is actually being ignored)
- Fix bug with getWorld in HandleSpawn
	- **Fixed** by calling GetWorld off of OwnerCharacter instead of in the projectile itself. There seems to be unreliable instantiation of something under the hood when GetWorld is called from a new actor
- Fix respawn losing items
	- Resist the urge to add on a new refactor for a proper inventory
	- Think the fix lies in moving weapon granting logic to OnPossessed instead of BeginPlay
	- **Fixed, had to override a different method for player restart**
- Work on spell ability expansion as detailed in [[Spell System]]
	- Implementing projectile system
	- OnHit works now, inverted relationship to have character hit by projectile process the effect
- Make spell hits do something
	- Base spells working, now to set up a more complex one

Worked: 3:30-5, 9:30-10