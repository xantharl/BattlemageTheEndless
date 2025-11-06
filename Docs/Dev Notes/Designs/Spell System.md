All spell Gameplay Abilities have an identifying tag of format: Spells.Class.Name (e.g. Spells.Ice.FrozenLance). Spells with Combos must add a suffix of .PhaseNumber (e.g. Spells.Ice.IceWall.1).

Spell Types (a spell can be of one or more types):
- Innate: Do not need to be prepared
- Quick: Usable as part of a combo involving melee, cannot start a spell combo
- Passive: Has sub types
	- Timed: Has a passive effect for a specified duration
	- Charges: Has a passive effect until used a specified amount of times
- Charge: Hold to charge, release to fire
- Hold to place: For spells with persistent effects
- Multiple Chargers: Buff spells with multiple charges can be expended all at once by holding the input
Spell Types are tracked as Gameplay Ability Tags
## Spell Types Design
- Add an input action for heavy abilities (Heavy weapon attacks and charge spells)
- Add a projectile configuration struct which contains all settings detailed in Hit Detection
- In ActivateAbility handle the projectile creation
- Spell have an FHitType of Single, Area, or Chain