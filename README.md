# Chaotic Deck Randomizer

### General Idea
	I wanted to create an easy way to generate random decks to use in chaoticrecode.com for fun.
	Hopefully people like it. 

### Video Demonstration/Installation Guide
	https://youtu.be/y8HS56zQqSo

### Installation
	1) https://github.com/nandreik/ChaoticDeckRandomizer/releases/tag/1.3
	2) Download and save Chaotic Randomizer.exe and chaotic.json to the same place on your computer 

	   For example, save them to the same folder on your desktop.
	3) Run Chaotic Randomizer.exe and it will create a file on your desktop called "chaotic_random_deck.txt"
	4) Import this file into chaoticrecode.com as an un-tap.in deck and save it under the unrestricted ruleset

### Bugs/Problems
	If you encounter a bug with important a deck due to a card that is not added to recode, etc. 
	Please message me on discord @Sirilean#4526 and send me that txt file of the deck.
	
	If you encouter any other problem trying to use this, also message me and I will try to help.

### Number of cards currently added in Recode (# added/# total) 
	THIS MAY NOT BE 100% ACCURATE, missing.txt from Chaotic discord is not fully updated
	total added: 1086/1240
        creatures: 434/484
            UW: 100/111
            OW: 111/121
            D: 87/98
            Mip: 93/97
            M'ar: 39/53
            Gen: 4/4
        battlegear: 126/155
        attacks: 251/266
        mugic: 160/189
            UW: 26/32
            OW: 31/34
            D: 28/32
            Mip: 28/33
            M'ar: 10/13
            Gen: 37/45
        locations: 115/146

### Balancing
	1) The first creature that is selected has an equal chance to be selected from any tribe (19.4%) and a small chance to be generec (4%)
	2) Attacks are selected based on the most popular elements from selected creatures (at least 3 creatures must share an element)
	2.1) and the most popular stats from selected creatures (at least 3 creatures must have a minimum of 60 in a shared stat)
	2.2) Attacks that are typically useless are ignored; ex: Slashclaw, Mandiblor Might, 100 stat checks (Glacial Balls, Academy Strike, etc)
	2.3) If selected creatures don't have any common elements/stats only attacks without elements/stats are added
	2.4) Attacks that check loyalty are ignored unless selected creatures match it (ex: OW only for Force Balls)
	2.5) The first 10 attacks that are chosen must be 1 BP
	2.6) Do not add more than 2 attacks that are 3 BP or higher 
	3) Do not add mugic from tribes that are not from selected creatures
	4) If Aaune is added, equip it with Baton of Aaune, add 1 Rage of Aaune to attack deck, 1 Calling of Aaune to Mugic, and 1 Oligarch's Path to locations for at least a chance to pull off the flip

