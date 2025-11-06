More of the same, just a lot of learning about how unreal handles pointers
Delegates in UE confuse the heck out of me so let's write this down:
### Example Case
- UCapsuleComponent's base class has an OnComponentBeginOverlap delegate
- WallRunAbility has a UCapsuleComponent
- WallRunAbility defines a function to use when that event is broadcast
- UCapsuleComponent's base class fires off the events

So does this work for my design?
### Thinking it through
- If UMovementAbility has an OnBegin and OnEnd event...
- And my movement components have UMovementAbilities...
- Then my movement component can define a function to use when that event is broadcast
- And UMovementAbility is responsible for broadcasting

Okay that worked after much trial and tribulation.

**Instanced keyword is imporant for sub-objects which need to maintain a state**

I think i might need another capsule for vault, on hit registration isn't working for base capsule

Worked: 10:30-4:45, 7:15-7:30