Yesterday was frustrating. After trying everything I can think of I still can't get OnHit to fire when hitting the dummy. I have a few thoughts this morning to explore though.

Today's goals:
- Make the sword do damage
	- Change event subscription to happen on pickup **No difference**
	- Check if handler is being hit even for self damage
	- Does the handler need to be on the subscribing class?
	- **Turns out the sphere collision works, but doesn't seem to move with the weapon**
	- Is this an issue with how my animations are set up?
	- **Turns out anim montages don't give free collision, found a great youtube video on setup. My inheritance hierarchy made it a bit less straight forward but it's working**
- Make the sword do damage only once per swing to a given actor **Done**
- Add some sort of hit indicator
- Figure out how to blend sword anim into main state better
- Start on Spells

Started at: 9:30
Break From: 11:30-5:00
End At: 8:30

Closing Thoughts: After a bit more pain, I found a youtube savior who showed me how to sphere trace. We have working hits now!