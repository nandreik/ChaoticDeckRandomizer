# Chaotic Deck Randomizer

### General Idea
	I wanted to create an easy way to generate random decks to use in chaoticrecode.com for fun.
	Hopefully people like it. 

### Video Demonstration

### Installation
1) Go to https://github.com/nandreik/ChaoticDeckRandomizer/tree/executable
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
	total added: 1083/1225
	creatures: 427/475
		UW: 100/110
		OW: 111/120
		D: 81/92
		Mip: 92/96
		M'ar: 39/53
		Gen: 4/4
	battlegear: 126/155
	attacks: 252/260
	mugic: 161/189
		UW: 26/32
		OW: 31/34
		D: 29/32
		Mip: 28/33
		M'ar: 10/13
		Gen: 37/45
	locations: 115/146


### General idea of randomization 
       1) pick random creatures
           1.1) if creature is loyal, pick rest of creatures from same tribe 
           1.2) if creature is loyal and m'arrillian, pick rest of creatures from same tribe or minions
           1.3) if creature is legendary, do not allow for more legendary creatures
           1.4) if card is unique, do not pick a duplicate 
                (may need hard coding for multiple versions of same card where one is unique and another is not, ex: Headmaster Ankhyja, Headmaster Ankhyja SOTA)

      2) pick random attacks
           2.1) if card is unique, do not pick a duplicate 
           2.2) keep track of Build Points (20), keep build point count after adding each attack
                if BP is 20 and less than 20 attacks are added, only add attacks with 0 BP
                same if BP is 18, only add atacks with 2 BP or less; etc

      3) pick random locations 
           3.1) if card is unique, do not pick a duplicate 

      4) pick random battlegear (must be chosen after selecting creatures)
           4.1) if card is unique, do not pick a duplicate 
           4.2) if card if legendary, do not add unless chosen creatures match tribe 

      5) pick random mugic
           5.1) if card is unique, do not pick a duplicate
           method one (must be chosen after selecting creatures): 
               5.2a) only add mugic for which there exists a matching tribe from the chosen creatures (and generic) 
           method two: **CURRENTLY IN USE**
               5.2b) add any mugic with disregard for already chosen creatures tribe (would lead to higher chance to not be able cast mugic) 


### Balancing
       1) There may be balance problems when Loyal creatures are selected early in the process.
          This not only adds a (usually) very powerful creature to your random deck (ex: Chaor, Takinom, Ankhyja),
          but also ensures that the rest of the creatures to be randomly added are all the same tribe. 
          This can make a "random" deck not very random at all compared to the more common mixed tribe results. 
          -possible solutions: 
	          1) ban loyal cards 
	          2) ???
       2) Mugic are currently totally random which may cause the inability to play all or any mugic. 
          -possible solutions: 
	          1) only add mugic that match the already picked tribes + generics 
       3) Attacks may be TOO random. Most decks result in mixed armies that don't share elements/etc. Even
          single tribe armies can have no common characteristics. This can make many of the random attacks
          useless or deal very little damage. 
          -possible solutions: 
		1) track creature elements and/or disciplines and give a weight towards attacks that match them (this would be annoying to implement)
		2) try to create a more balanced attack deck by setting a limit to high bp attacks or giving higher weights to lower build point attacks
		3) do nothing and embrace chaos


