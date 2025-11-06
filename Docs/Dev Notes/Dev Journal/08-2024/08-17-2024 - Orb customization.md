I want the spell orb to change with selected element, probably different textures and adjust the particles as well. Stretch goal today, make different spells cast based on the orb changes

- Fix idle animation's jerk at the end
	- **Done**
- Make spell anim only play on cd
	- **Done**
- Fix projectile hitting self
	- **Done**
- Add input actions for different spell class selections
	- **Done**
- Add casting mode which holds the orb up for viewing
	- **Done**
- Fix crouching while not moving
	- **Done**
- Wire them up to change the material assigned to the orb
- Wire them up to change the particle color
- Figure out how to handle multiple spell types

Worked: 3:30 - 4:30, 5:45 - 9:15

### Design for Spell Classes
- New spell class base class
	- New class per spell class
	- Probably an enum for ease of tracking similar to movement ability
- Character tracks which spell classes it has
	- Input actions need to be on default context then