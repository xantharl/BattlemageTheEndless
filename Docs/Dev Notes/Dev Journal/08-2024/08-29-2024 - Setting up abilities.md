I took some time to diagram out how i want to set up abilities, now to actually implement it. This is going to completely break melee combos as i currently have them implemented, but I think the result will be a better system. This likely won't be a one day thing.

Tasks:
- Unwind current combo system so we can compile again
	- **Done**
- Invert relationship between character and weapon so that the weapon doesn't need to know about the impl of bmage
	- **In progress**
- Implement ability system per this diagram https://excalidraw.com/#json=GBQtfjoIFaZY7IPTainGq,QkapbUzzgZyrhTDN700yyg
- Make combos work with the new system
	- Handle ability transitions in character or maybe controller
	- Cut montage into pieces for each attack

Worked: 2:45 - 5