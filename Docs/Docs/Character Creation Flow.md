1. Main Menu -> New Game
2. Go through the steps of selection
3. When Finish is clicked the following happens:
	1. The choices are finalized in the character creation widget, which then notifies the parent (main menu)
	2. Main menu widget stores selections to GameInstance
	3. BP_BattlemageTheEndlessCharacter uses the stored into in GameInstance to adjust DefaultEquipment and which spells are granted
		- Note: This happens in the **construction script** so that the choices are locked in before BeginPlay, which grants equipment and abilities