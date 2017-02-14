/*
	File:		i.r

	Contains:	Resources for Max external object i.

	Written by:	Peter Castine

	Copyright:	 © 2000-2002 Peter Castine. All rights reserved.

	Change History (most recent first):

         <9>    6–3–2005    pc      String numbering still problematic.
         <8>      6–3–05    pc      Correct problem with index numbering on Windows. Strange that RC
                                    didn't pick this up earlier.
         <7>     21–1–04    pc      Go to final status.
         <6>      8–1–04    pc      Modify for Rez/RC compatibility.
         <5>    6–7–2003    pc      Add STR# resource for Object List categories
         <4>    7–3–2003    pc      Bump version to final candidate.
         <3>  30–12–2002    pc      Use 'STR#' resource instead of faux 'Vers' resource for storing
                                    version information used at run-time. Also let version info
                                    indicate Classic/Carbon-savviness. Bump minor revision numbers. 
         <2>  29–11–2002    pc      Tidy up initial check in.
         <1>  29–11–2002    pc      Initial check in.
*/


	//
	// Configuration values for this object
	//
	// These must be unique within Litter Package. The Names must match constant values used
	// in the C/C++ source code (we try never to access resources by ID).
#define LPobjID			17515
#define LPobjName		"lp.i"

	// -----------------------------------------
	// 'vers' stuff we need to maintain manually
//#define LPobjStarter		1							// Comment out for Pro Bundles
#define LPobjMajorRev		1
#define LPobjMinorRev		2
#define LPobjBugFix			0
#define LPobjStage			finalStage
#define LPobjStageBuild		1
#define LPobjRegion			0							// US
#define LPobjVersStr		"1.2fc1"
#define LPobjCRYears		"2001-08"

#define	LPobjLitterCategory	"Litter Utilities"
#define LPobjMax3Category	"Messages"					// Category for Max 2.2 - 3.6x
#define LPobjMax4Category	"Messages"					// Category starting at Max 4


	// Description string (for Windows Properties box, taken from documentation)
#define LPobjDescription	"Text of I Ching oracles"

	// The following sets up the 'mAxL', 'vers', and 'Vers' resources
	// It relies on the values above for resource IDs and names, as
	// well as concrete values for the 'vers'(1) resource.
#include "Litter Globals.r"
	//
	// -----------------------------------------


	//
	// Other resource definitions 
	//
	// Assistance strings
#define LPAssistIn1			"Int (Main Hexagram)"
#define LPAssistIn2			"Int (Future Hexagram)"
#define LPAssistOut1		"Symbols (Wisdom of the I Ching)"

	// Description string (for Windows Properties box, taken from documentation)
#define LPobjDescription	"Text of I Ching oracles"
#ifdef RC_INVOKED					// Must be Windows RC Compiler

	#define newlineSymbol		"\015"
	#define apostropheSymbol	"\325"
	#define uumlautSymbol		"\237"

	STRINGTABLE DISCARDABLE
		BEGIN
			/////////////////////////////////////////////////////////////////
			//
			// Assistance strings
			//
		#define lpStrIndexAssist	(lpStrIndexLastStandard + 1)				// 7
			
			lpStrIndexAssist,			LPAssistIn1
			lpStrIndexAssist + 1,		LPAssistIn2
			lpStrIndexAssist + 2,		LPAssistOut1

			/////////////////////////////////////////////////////////////////
			//
			// Common strings
			//
		#define lpStrIndexCommon	(lpStrIndexAssist + 3)						// 10
			
			lpStrIndexCommon,			"above"
			lpStrIndexCommon+1,			"below"
			lpStrIndexCommon+2,			"upper middle"
			lpStrIndexCommon+3,			"lower middle"
			lpStrIndexCommon+4,			"Governing Rulers:"
			lpStrIndexCommon+5,			"Constituting Rulers:"
			lpStrIndexCommon+6,			"The Judgement"
			lpStrIndexCommon+7,			"The Image"
			lpStrIndexCommon+8,			"The Lines"
			lpStrIndexCommon+9,			"  "
			lpStrIndexCommon+10,		"Six"
			lpStrIndexCommon+11,		"Nine"
			lpStrIndexCommon+12,		"at the beginning"
			lpStrIndexCommon+13,		"in the second place"
			lpStrIndexCommon+14,		"in the third place"
			lpStrIndexCommon+15,		"in the fourth place"
			lpStrIndexCommon+16,		"in the fifth place"
			lpStrIndexCommon+17,		"at the top"
			lpStrIndexCommon+18,		"means:"
			lpStrIndexCommon+19,		"The Future"				

			/////////////////////////////////////////////////////////////////
			//
			// Trigram names
			//
		#define lpStrIndexTrigrams	(lpStrIndexCommon + 20)						// 30

			lpStrIndexTrigrams,			"Ch" apostropheSymbol "ien"
			lpStrIndexTrigrams+1,		"The Creative Principle; The Heaven"
			lpStrIndexTrigrams+2,		"Kun"
			lpStrIndexTrigrams+3,		"The Passive Principle; The Earth"
			lpStrIndexTrigrams+4,		"Chen"
			lpStrIndexTrigrams+5,		"Movement; The Thunderclap"
			lpStrIndexTrigrams+6,		"K" apostropheSymbol "an"
			lpStrIndexTrigrams+7,		"The Abyss; Water, Danger"
			lpStrIndexTrigrams+8,		"Ken"
			lpStrIndexTrigrams+9,		"Inaction; The Mountain"
			lpStrIndexTrigrams+10,		"Sun"
			lpStrIndexTrigrams+11,		"The Gentle One; Wind, Wood"
			lpStrIndexTrigrams+12,		"Li"
			lpStrIndexTrigrams+13,		"Clinging; Fire"
			lpStrIndexTrigrams+14,		"Tui"
			lpStrIndexTrigrams+15,		"Joy; The Lake"				

			/////////////////////////////////////////////////////////////////
			//
			// The house of Ch'ien
			//
		#define lpStrIndexChien		(lpStrIndexTrigrams + 16)					// 46
		#define	lpStrIndexKou		(lpStrIndexChien + 10)
		#define lpStrIndexTun		(lpStrIndexChien + 20) 
		#define lpStrIndexPi		(lpStrIndexChien + 30)
		#define lpStrIndexKuan		(lpStrIndexChien + 40)
		#define lpStrIndexPo		(lpStrIndexChien + 50)
		#define lpStrIndexChin		(lpStrIndexChien + 60)
		#define lpStrIndexTaYu		(lpStrIndexChien + 70)

			lpStrIndexChien,			"Ch" apostropheSymbol "ien"
			lpStrIndexChien+1,		"The Creative"
			lpStrIndexChien+2,		"THE CREATIVE works sublime success," newlineSymbol "Furthering through perseverance."
			lpStrIndexChien+3,		"The movement of heaven is full of power." newlineSymbol "Thus the superior man makes himself strong and untiring."
			lpStrIndexChien+4,		"Hidden dragon. Do not act."
			lpStrIndexChien+5,		"Dragon appearing in the field." newlineSymbol "It furthers one to see the great man."
			lpStrIndexChien+6,		"All day long the superior man is creatively active." newlineSymbol "At nightfall his mind is still beset with cares." newlineSymbol "Danger. No blame."
			lpStrIndexChien+7,		"Wavering flight over the depths." newlineSymbol "No blame."
			lpStrIndexChien+8,		"Flying dragon in the heavens." newlineSymbol "It furthers one to see the great man."
			lpStrIndexChien+9,		"Arrogant dragon will have cause to repent." newlineSymbol "When all the lines are nines, it means:" newlineSymbol "There appears a flight of dragons without heads." newlineSymbol "Good fortune."

			lpStrIndexKou,				"Kou"
			lpStrIndexKou+1,		"Coming to Meet"
			lpStrIndexKou+2,		"COMING TO MEET. The maiden is powerful." newlineSymbol "One should not marry such a maiden."
			lpStrIndexKou+3,		"Under heaven, wind:" newlineSymbol "The image of COMING TO MEET." newlineSymbol "Thus does the prince act when disseminating his commands" newlineSymbol "And proclaiming them to the four quarters of heaven."
			lpStrIndexKou+4,		"It must be checked with a brake of bronze." newlineSymbol "Perseverance brings good fortune." newlineSymbol "If one lets it take its course, one experiences misfortune." newlineSymbol "Even a lean pig has it in him to rage around."
			lpStrIndexKou+5,		"There is a fish in the tank.  No blame." newlineSymbol "Does not further guests."
			lpStrIndexKou+6,		"There is no skin on his thighs," newlineSymbol "And walking comes hard." newlineSymbol "If one is mindful of the danger," newlineSymbol "No great mistake is made." newlineSymbol ""
			lpStrIndexKou+7,		"No fish in the tank." newlineSymbol "This leads to misfortune."
			lpStrIndexKou+8,		"A melon covered with willow leaves." newlineSymbol "Hidden lines." newlineSymbol "Then it drops down to one from heaven."
			lpStrIndexKou+9,		"He comes to meet with his horns." newlineSymbol "Humiliation. No blame."

			lpStrIndexTun,				"Tun"
			lpStrIndexTun+1,		"Retreat"
			lpStrIndexTun+2,		"RETREAT. Success." newlineSymbol "In what is small, perseverance furthers."
			lpStrIndexTun+3,		"Mountain under heaven: the image of RETREAT." newlineSymbol "Thus the superior man keeps the inferior man at a distance," newlineSymbol "Not angrily but with reserve."
			lpStrIndexTun+4,		"At the tail in retreat. This is dangerous." newlineSymbol "One must not wish to undertake anything."
			lpStrIndexTun+5,		"He holds him fast with yellow oxhide." newlineSymbol "No one can tear him loose."
			lpStrIndexTun+6,		"A halted retreat" newlineSymbol "Is nerve-wracking and dangerous." newlineSymbol "To retain people as men- and maidservants" newlineSymbol "Brings good fortune."
			lpStrIndexTun+7,		"Voluntary retreat brings good fortune to the superior man" newlineSymbol "And downfall to the inferior man."
			lpStrIndexTun+8,		"Friendly retreat. Perseverance brings good fortune."
			lpStrIndexTun+9,		"Cheerful retreat. Everything serves to further."

			lpStrIndexPi,				"P" apostropheSymbol "i"
			lpStrIndexPi+1,			"Standstill (Stagnation)"
			lpStrIndexPi+2,			"STANDSTILL. Evil people do not further" newlineSymbol "The perseverance of the superior man." newlineSymbol "The great departs; the small approaches."
			lpStrIndexPi+3,			"Heaven and earth do not unite:" newlineSymbol "The image of STANDSTILL." newlineSymbol "Thus the superior man falls back upon his inner worth" newlineSymbol "In order to escape the difficulties." newlineSymbol "He does not permit himself to be honored with revenue."
			lpStrIndexPi+4,			"When ribbon grass is pulled up, the sod comes with it." newlineSymbol "Each according to his kind." newlineSymbol "Perseverance brings good fortune and success."
			lpStrIndexPi+5,			"They bear and endure;" newlineSymbol "This means good fortune for inferior people." newlineSymbol "The standstill serves to help the great man to attain success."
			lpStrIndexPi+6,			"They bear shame."
			lpStrIndexPi+7,			"He who acts at the command of the highest" newlineSymbol "Remains without blame." newlineSymbol "Those of like mind partake of the blessing."
			lpStrIndexPi+8,			"Standstill is giving way." newlineSymbol "Good fortune for the great man." newlineSymbol """What if it should fail, what if it should fail?"",In this way, he ties it to a cluster of mulberry roots."
			lpStrIndexPi+9,			"The standstill comes to an end." newlineSymbol "First standstill, then good fortune."
			
			lpStrIndexKuan,				"Kuan"
			lpStrIndexKuan+1,		"Contemplation (View)"
			lpStrIndexKuan+2,		"CONTEMPLATION. The ablution has been made," newlineSymbol "But not yet the offering." newlineSymbol "Full of trust, they look up to him."
			lpStrIndexKuan+3,		"The wind blows over the earth:" newlineSymbol "The image of CONTEMPLATION." newlineSymbol "Thus the kings of old visited the regions of the world," newlineSymbol "Contemplated the people," newlineSymbol "And gave them instruction."
			lpStrIndexKuan+4,		"Boy-like contemplation." newlineSymbol "For an inferior man, no blame." newlineSymbol "For a superior man, humiliation."
			lpStrIndexKuan+5,		"Contemplation through the crack of the door." newlineSymbol "Furthering for the perseverance of a woman."
			lpStrIndexKuan+6,		"Contemplation of my life" newlineSymbol "Decides the choice" newlineSymbol "Between advance and retreat."
			lpStrIndexKuan+7,		"Contemplation of the light of the kingdom." newlineSymbol "It furthers one to exert influence as the guest of a king."
			lpStrIndexKuan+8,		"Contemplation of my life." newlineSymbol "The superior man is without blame."
			lpStrIndexKuan+9,		"Contemplation of his life." newlineSymbol "The superior man is without blame."
			
			lpStrIndexPo,				"Po"
			lpStrIndexPo+1,			"Splitting Apart"
			lpStrIndexPo+2,			"SPLITTING APART. It does not further one" newlineSymbol "To go anywhere."
			lpStrIndexPo+3,			"The mountain rests on the earth:" newlineSymbol "The image of SPLITTING APART." newlineSymbol "Thus those above can ensure their position" newlineSymbol "Only by giving generously to those below."
			lpStrIndexPo+4,			"The leg of the bed is split." newlineSymbol "Those who persevere are destroyed."
			lpStrIndexPo+5,			"The bed is split at the edge." newlineSymbol "Those who persevere are destroyed." newlineSymbol "Misfortune."
			lpStrIndexPo+6,			"He splits with them. No blame."
			lpStrIndexPo+7,			"The bed is split up to the skin." newlineSymbol "Misfortune."
			lpStrIndexPo+8,			"A shoal of fishes. Favor comes through the court ladies." newlineSymbol "Everything acts to further."
			lpStrIndexPo+9,			"There is a large fruit still un-eaten." newlineSymbol "The superior man receives a carriage." newlineSymbol "The house of the inferior man is split apart."
			
			lpStrIndexChin,				"Chin"
			lpStrIndexChin+1,		"Progress"
			lpStrIndexChin+2,		"PROGRESS. The powerful prince" newlineSymbol "Is honored with horses in large numbers." newlineSymbol "In a single day he is granted audience three times."
			lpStrIndexChin+3,		"The sun rises over the earth:" newlineSymbol "The image of PROGRESS." newlineSymbol "Thus the superior man himself" newlineSymbol "Brightens his bright virtue."
			lpStrIndexChin+4,		"Progressing, but turned back." newlineSymbol "Perseverance brings good fortune." newlineSymbol "If one meets with no confidence, one should remain calm." newlineSymbol "No mistake."
			lpStrIndexChin+5,		"Progressing, but in sorrow." newlineSymbol "Perseverance brings good fortune." newlineSymbol "Then one obtains great happiness from one" apostropheSymbol "s ancestress."
			lpStrIndexChin+6,		"All are in accord. Remorse disappears."
			lpStrIndexChin+7,		"Progress like a hamster." newlineSymbol "Perseverance brings danger."
			lpStrIndexChin+8,		"Remorse disappears." newlineSymbol "Take not gain and loss to heart." newlineSymbol "Undertakings bring good fortune." newlineSymbol "Everything serves to further."
			lpStrIndexChin+9,		"Making progress with the horns is permissible" newlineSymbol "One for the purpose of punishing one" apostropheSymbol "s own city." newlineSymbol "To be conscious of danger brings good fortune." newlineSymbol "No blame." newlineSymbol "Perseverance brings humiliation."
			
			lpStrIndexTaYu,				"Ta Yu"
			lpStrIndexTaYu+1,		"Possession in Great Measure"
			lpStrIndexTaYu+2,		"POSSESSION IN GREAT MEASURE." newlineSymbol "Supreme success."
			lpStrIndexTaYu+3,		"Fire in heaven above:" newlineSymbol "The image of POSSESSION IN GREAT MEASURE." newlineSymbol "Thus the superior man curbs evil and furthers good," newlineSymbol "And thereby orders the benevolent will of heaven."
			lpStrIndexTaYu+4,		"No relationship with what is harmful;" newlineSymbol "There is no blame in this." newlineSymbol "If one remains conscious of difficulty," newlineSymbol "One remains without blame."
			lpStrIndexTaYu+5,		"A big wagon for loading." newlineSymbol "One may undertake something." newlineSymbol "No blame."
			lpStrIndexTaYu+6,		"A prince offers it to the Son of Heaven." newlineSymbol "A petty man cannot do this."
			lpStrIndexTaYu+7,		"He makes a difference" newlineSymbol "Between himself and his neighbor." newlineSymbol "No blame."
			lpStrIndexTaYu+8,		"He whose truth is accessible, yet dignified," newlineSymbol "Has good fortune."
			lpStrIndexTaYu+9,		"He is blessed by heaven." newlineSymbol "Good fortune." newlineSymbol "Nothing that does not further."				

			/////////////////////////////////////////////////////////////////
			//
			// The house of K'an
			//
		#define lpStrIndexKan		(lpStrIndexChien + 80)						// 126
		#define	lpStrIndexChieh		(lpStrIndexKan + 10)
		#define lpStrIndexChun		(lpStrIndexKan + 20)
		#define lpStrIndexChiChi	(lpStrIndexKan + 30)
		#define lpStrIndexKo		(lpStrIndexKan + 40)
		#define lpStrIndexFeng		(lpStrIndexKan + 50)
		#define lpStrIndexMingI		(lpStrIndexKan + 60)
		#define lpStrIndexShih		(lpStrIndexKan + 70)

			lpStrIndexKan,				"K" apostropheSymbol "an"
			lpStrIndexKan+1,		"The Abysmal (Water)"
			lpStrIndexKan+2,		"The Abysmal repeated." newlineSymbol "If you are sincere, you have success in your heart," newlineSymbol "And whatever you do succeeds."
			lpStrIndexKan+3,		"Water flows on uninterruptedly and reaches its goal:" newlineSymbol "The image of the Abysmal repeated." newlineSymbol "Thus the superior man walks in lasting virtue" newlineSymbol "And carries on the business of teaching."
			lpStrIndexKan+4,		"Repetition of the Abysmal." newlineSymbol "In the abyss one falls into a pit." newlineSymbol "Misfortune."
			lpStrIndexKan+5,		"The abyss is dangerous." newlineSymbol "One should strive to attain small things only." newlineSymbol ""
			lpStrIndexKan+6,		"Forward and backward, abyss on abyss." newlineSymbol "In danger like this, pause at first and wait," newlineSymbol "Otherwise you will fall into a pit in the abyss." newlineSymbol "Do not act in this way."
			lpStrIndexKan+7,		"A jug of wine, a bowl of rice with it;" newlineSymbol "Earthen vessels" newlineSymbol "Simply handed in through the window." newlineSymbol "There is certainly no blame in this."
			lpStrIndexKan+8,		"The abyss is not filled to overflowing," newlineSymbol "It is filled only to the rim." newlineSymbol "No blame."
			lpStrIndexKan+9,		"Bound with cords and ropes," newlineSymbol "Shut in between thorn-edged prison walls:" newlineSymbol "For three years one does not find the way." newlineSymbol "Misfortune."
			
			lpStrIndexChieh,			"Chieh"
			lpStrIndexChieh+1,		"Limitation"
			lpStrIndexChieh+2,		"LIMITATION. Success." newlineSymbol "Galling limitation must not be persevered in."
			lpStrIndexChieh+3,		"Water over lake: the image of LIMITATION." newlineSymbol "Thus the superior man" newlineSymbol "Creates number and measure," newlineSymbol "And examines the nature of virtue and correct conduct."
			lpStrIndexChieh+4,		"Not going out of the door and the courtyard" newlineSymbol "Is without blame."
			lpStrIndexChieh+5,		"Not going out of the gate and the courtyard " newlineSymbol "Brings misfortune."
			lpStrIndexChieh+6,		"He who knows no limitation" newlineSymbol "Will have cause to lament." newlineSymbol "No blame."
			lpStrIndexChieh+7,		"Contented limitation. Success."
			lpStrIndexChieh+8,		"Sweet limitation brings good fortune." newlineSymbol "Going brings esteem."
			lpStrIndexChieh+9,		"Galling limitation." newlineSymbol "Perseverance brings misfortune." newlineSymbol "Remorse disappears."
			
			lpStrIndexChun,			"Chun"
			lpStrIndexChun+1,		"Difficulty at the Beginning"
			lpStrIndexChun+2,		"DIFFICULTY AT THE BEGINNING works supreme success," newlineSymbol "Furthering through perseverance." newlineSymbol "Nothing should be undertaken." newlineSymbol "It furthers one to appoint helpers." newlineSymbol ""
			lpStrIndexChun+3,		"Clouds and thunder:" newlineSymbol "The image of DIFFICULTY AT THE BEGINNING." newlineSymbol "Thus the superior man brings order out of confusion."
			lpStrIndexChun+4,		"Hesitation and hindrance." newlineSymbol "It furthers one to remain persevering." newlineSymbol "It furthers one to appoint helpers."
			lpStrIndexChun+5,		"Difficulties pile up." newlineSymbol "Horse and wagon part." newlineSymbol "He is not a robber;" newlineSymbol "He wants to woo when the time comes." newlineSymbol "The maiden is chaste," newlineSymbol "She does not pledge herself." newlineSymbol "Ten years -- then she pledges herself."
			lpStrIndexChun+6,		"Whoever hunts deer without the forester" newlineSymbol "Only loses his way in the forest." newlineSymbol "The superior man understands the signs of the time" newlineSymbol "And prefers to desist." newlineSymbol "To go on brings humiliation." newlineSymbol ""
			lpStrIndexChun+7,		"Horse and wagon part." newlineSymbol "Strive for union." newlineSymbol "To go brings good fortune." newlineSymbol "Everything acts to further."
			lpStrIndexChun+8,		"Difficulties in blessing." newlineSymbol "A little perseverance brings good fortune." newlineSymbol "Great perseverance brings misfortune."
			lpStrIndexChun+9,		"Horse and wagon part." newlineSymbol "Bloody tears flow."
			
			lpStrIndexChiChi,			"Chi Chi"
			lpStrIndexChiChi+1,		"After Completion"
			lpStrIndexChiChi+2,		"AFTER COMPLETION. Success in small matters." newlineSymbol "Perseverance furthers." newlineSymbol "At the beginning good fortune," newlineSymbol "At the end disorder."
			lpStrIndexChiChi+3,		"Water over fire: the image of the condition" newlineSymbol "In AFTER COMPLETION." newlineSymbol "Thus the superior man" newlineSymbol "Takes thought of misfortune" newlineSymbol "And arms himself against it in advance."
			lpStrIndexChiChi+4,		"He brakes his wheels." newlineSymbol "He gets his tail in the water." newlineSymbol "No blame."
			lpStrIndexChiChi+5,		"The woman loses the curtain of her carriage." newlineSymbol "Do not run after it;" newlineSymbol "One the seventh day you will get it."
			lpStrIndexChiChi+6,		"The Illustrious Ancestor" newlineSymbol "Disciplines the Devil" apostropheSymbol "s Country." newlineSymbol "After three years he conquers it." newlineSymbol "Inferior people must not be employed."
			lpStrIndexChiChi+7,		"The finest clothes turn to rags." newlineSymbol "Be careful all day long."
			lpStrIndexChiChi+8,		"The neighbor in the east who slaughters an ox" newlineSymbol "Does not attain as much real happiness" newlineSymbol "As the neighbor in the west" newlineSymbol "With his small offering."
			lpStrIndexChiChi+9,		"He gets his head in the water. Danger."
			
			lpStrIndexKo,				"Ko"
			lpStrIndexKo+1,			"Revolution (Molting)"
			lpStrIndexKo+2,			"REVOLUTION. On your own day" newlineSymbol "You are believed." newlineSymbol "Supreme success," newlineSymbol "Furthering through perseverance." newlineSymbol "Remorse disappears."
			lpStrIndexKo+3,			"Fire in the lake: the image of REVOLUTION." newlineSymbol "Thus the superior man" newlineSymbol "Sets the calendar in order" newlineSymbol "And makes the seasons clear."
			lpStrIndexKo+4,			"Wrapped in the hide of a yellow cow."
			lpStrIndexKo+5,			"When one" apostropheSymbol "s own day comes, one may create revolution." newlineSymbol "Starting brings good fortune. No blame."
			lpStrIndexKo+6,			"Starting brings misfortune." newlineSymbol "Perseverance brings danger." newlineSymbol "When talk of revolution has gone the rounds three times," newlineSymbol "One may commit himself," newlineSymbol "And men will believe him."
			lpStrIndexKo+7,			"Remorse disappears. Men believe him." newlineSymbol "Changing the form of government brings good fortune."
			lpStrIndexKo+8,			"The great man changes like a tiger." newlineSymbol "Even before he questions the oracle" newlineSymbol "He is believed."
			lpStrIndexKo+9,			"The superior man changes like a panther." newlineSymbol "The inferior man molts in the face." newlineSymbol "Starting brings misfortune." newlineSymbol "To remain persevering brings good fortune."
			
			lpStrIndexFeng,				"Feng"
			lpStrIndexFeng+1,		"Abundance (Fullness)"
			lpStrIndexFeng+2,		"ABUNDANCE has success." newlineSymbol "The king attains abundance." newlineSymbol "Be not sad." newlineSymbol "Be like the sun at midday."
			lpStrIndexFeng+3,		"Both thunder and lightning come:" newlineSymbol "The image of ABUNDANCE." newlineSymbol "Thus the superior man decides lawsuits" newlineSymbol "And carries out punishments."
			lpStrIndexFeng+4,		"When a man meets his destined ruler," newlineSymbol "The can be together ten days," newlineSymbol "And it is not a mistake." newlineSymbol "Going meets with recognition."
			lpStrIndexFeng+5,		"The curtain is of such fullness" newlineSymbol "That the polestars can be seen at noon." newlineSymbol "Through going one meets with mistrust and hate." newlineSymbol "If one rouses him through truth," newlineSymbol "Good fortune comes."
			lpStrIndexFeng+6,		"The underbrush is of such abundance" newlineSymbol "That the small stars can be seen at noon." newlineSymbol "He breaks his right arm. No blame."
			lpStrIndexFeng+7,		"The curtain is of such fullness" newlineSymbol "That the polestars can be seen at noon." newlineSymbol "He meets his ruler, who is of like mind." newlineSymbol "Good fortune."
			lpStrIndexFeng+8,		"Lines are coming." newlineSymbol "Blessing and fame draw near." newlineSymbol "Good fortune."
			lpStrIndexFeng+9,		"His house is in a state of abundance." newlineSymbol "He screens off his family." newlineSymbol "He peers through the gate" newlineSymbol "And no longer perceives anyone." newlineSymbol "For three years he sees nothing." newlineSymbol "Misfortune."
			
			lpStrIndexMingI,			"Ming I"
			lpStrIndexMingI+1,		"Darkening of the Light"
			lpStrIndexMingI+2,		"DARKENING OF THE LIGHT. In adversity" newlineSymbol "It furthers one to be persevering."
			lpStrIndexMingI+3,		"The light has sunk into the earth:" newlineSymbol "The image of DARKENING OF THE LIGHT." newlineSymbol "Thus does the superior man live with the great mass:" newlineSymbol "He veils his light, yet still shines."
			lpStrIndexMingI+4,		"Darkening of the light during flight." newlineSymbol "He lowers his wings." newlineSymbol "The superior man does not eat for three days" newlineSymbol "On his wanderings." newlineSymbol "Be he has somewhere to go." newlineSymbol "The host has occasion to gossip about him."
			lpStrIndexMingI+5,		"Darkening of the light injures him in the left thigh." newlineSymbol "He gives aid with the strength of a horse." newlineSymbol "Good fortune."
			lpStrIndexMingI+6,		"Darkening of the light during the hunt in the south." newlineSymbol "Their great leader is captured." newlineSymbol "One must not expect perseverance too soon."
			lpStrIndexMingI+7,		"He penetrates the left side of the belly." newlineSymbol "One gets at the very heart of the darkening of the light," newlineSymbol "And leaves gate and courtyard."
			lpStrIndexMingI+8,		"Darkening of the light as with Prince Chi." newlineSymbol "Perseverance furthers."
			lpStrIndexMingI+9,		"Not light but darkness." newlineSymbol "First he climbed up to heaven," newlineSymbol "The he plunged into the depths of the earth."
			
			lpStrIndexShih,				"Shih"
			lpStrIndexShih+1,		"The Army"
			lpStrIndexShih+2,		"THE ARMY. The army needs perseverance" newlineSymbol "And a strong man." newlineSymbol "Good fortune without blame."
			lpStrIndexShih+3,		"In the middle of the earth is water:" newlineSymbol "The image of THE ARMY." newlineSymbol "Thus the superior man increases his masses" newlineSymbol "By generosity toward the people."
			lpStrIndexShih+4,		"An army must set forth in proper order." newlineSymbol "If the order is not good, misfortune threatens."
			lpStrIndexShih+5,		"In the midst of the army." newlineSymbol "Good fortune. No blame." newlineSymbol "The king bestows a triple decoration." newlineSymbol ""
			lpStrIndexShih+6,		"Perchance the army carries corpses in the wagon." newlineSymbol "Misfortune." newlineSymbol ""
			lpStrIndexShih+7,		"The army retreats.  No blame."
			lpStrIndexShih+8,		"There is game in the field." newlineSymbol "It furthers one to catch it." newlineSymbol "Without blame." newlineSymbol "Let the eldest lead the army." newlineSymbol "The younger transport corpses;" newlineSymbol "Then perseverance brings misfortune."
			lpStrIndexShih+9,		"The great prince issues commands," newlineSymbol "Founds estates, vests families with fiefs." newlineSymbol "Inferior people should not be employed."				


			/////////////////////////////////////////////////////////////////
			//
			// The house of Ken
			//
		#define lpStrIndexKen		(lpStrIndexKan + 80)						// 206
		#define	lpStrIndexPi2		(lpStrIndexKen + 10)
		#define lpStrIndexTaChu		(lpStrIndexKen + 20)
		#define lpStrIndexSun2		(lpStrIndexKen + 30)
		#define lpStrIndexKuei		(lpStrIndexKen + 40)
		#define lpStrIndexLue		(lpStrIndexKen + 50)
		#define lpStrIndexChungFu	(lpStrIndexKen + 60)
		#define lpStrIndexChien2	(lpStrIndexKen + 70)

			lpStrIndexKen,				"Ken"
			lpStrIndexKen+1,		"Keeping Still, Mountain"
			lpStrIndexKen+2,		"KEEPING STILL. Keeping his back still" newlineSymbol "So that he no longer feels his body." newlineSymbol "He goes into his courtyard" newlineSymbol "And does not see his people." newlineSymbol "No blame."
			lpStrIndexKen+3,		"Mountains standing close together:" newlineSymbol "The image of KEEPING STILL." newlineSymbol "Thus the superior man" newlineSymbol "Does not permit his thoughts" newlineSymbol "To go beyond his situation."
			lpStrIndexKen+4,		"Keeping his toes still." newlineSymbol "No blame." newlineSymbol "Continued perseverance furthers."
			lpStrIndexKen+5,		"Keeping his calves still." newlineSymbol "He cannot rescue him whom he follows." newlineSymbol "His heart is not glad."
			lpStrIndexKen+6,		"Keeping his hips still." newlineSymbol "Making his sacrum stiff." newlineSymbol "Dangerous.  The heart suffocates."
			lpStrIndexKen+7,		"Keeping his trunk still." newlineSymbol "No blame."
			lpStrIndexKen+8,		"Keeping his jaws still." newlineSymbol "The words have order." newlineSymbol "Remorse disappears."
			lpStrIndexKen+9,		"Noble-hearted keeping still." newlineSymbol "Good fortune."
			
			lpStrIndexPi2,				"Pi"
			lpStrIndexPi2+1,		"Graces"
			lpStrIndexPi2+2,		"GRACE has success." newlineSymbol "In small matters" newlineSymbol "It is favorable to undertake something."
			lpStrIndexPi2+3,		"Fire at the foot of the mountain:" newlineSymbol "The image of GRACE." newlineSymbol "Thus does the superior man proceed" newlineSymbol "When clearing up current affairs." newlineSymbol "But he dare not decide controversial issues in this way."
			lpStrIndexPi2+4,		"He lends grace to his toes, leaves the carriage" newlineSymbol " and walks."
			lpStrIndexPi2+5,		"Lends grace to the beard on his chin."
			lpStrIndexPi2+6,		"Graceful and moist." newlineSymbol "Constant perseverance brings good fortune."
			lpStrIndexPi2+7,		"Grace or simplicity?" newlineSymbol "A white horse comes as if on wings." newlineSymbol "He is not a robber," newlineSymbol "He will woo at the right time."
			lpStrIndexPi2+8,		"Grace in hills and gardens." newlineSymbol "The roll of silk is meager and small." newlineSymbol "Humiliation, but in the end good fortune."
			lpStrIndexPi2+9,		"Simple grace. No blame."
			
			lpStrIndexTaChu,			"Ta Ch" apostropheSymbol "u"
			lpStrIndexTaChu+1,		"The Taming Power of the Great"
			lpStrIndexTaChu+2,		"THE TAMING POWER OF THE GREAT." newlineSymbol "Perseverance furthers." newlineSymbol "Not eating at home brings good fortune." newlineSymbol "It furthers one to cross the great water."
			lpStrIndexTaChu+3,		"Heaven within the mountain:" newlineSymbol "The image of THE TAMING POWER OF THE GREAT." newlineSymbol "Thus the superior man acquaints himself with many sayings of antiquity" newlineSymbol "And many deeds of the past," newlineSymbol "In order to strengthen his character thereby."
			lpStrIndexTaChu+4,		"Danger is at hand. It furthers one to desist."
			lpStrIndexTaChu+5,		"The axletrees are taken from the wagon."
			lpStrIndexTaChu+6,		"A good horse that follows others." newlineSymbol "Awareness of danger," newlineSymbol "With perseverance, furthers." newlineSymbol "Practice chariot driving and armed defense daily." newlineSymbol "It furthers one to have somewhere to go."
			lpStrIndexTaChu+7,		"The headboard of a young bull." newlineSymbol "Great good fortune."
			lpStrIndexTaChu+8,		"The tusk of a gelded boar." newlineSymbol "Good fortune."
			lpStrIndexTaChu+9,		"One attains the way of heaven. Success."
			
			lpStrIndexSun2,				"Sun"
			lpStrIndexSun2+1,		"Decrease"
			lpStrIndexSun2+2,		"DECREASE combined with sincerity" newlineSymbol "Brings about supreme good fortune" newlineSymbol "Without blame." newlineSymbol "One may be persevering in this." newlineSymbol "It furthers one to undertake something." newlineSymbol "How is this to be carried out?" newlineSymbol "One may use two small bowls for the sacrifice."
			lpStrIndexSun2+3,		"At the foot of the mountain, the lake:" newlineSymbol "The image of DECREASE." newlineSymbol "Thus the superior man controls his anger" newlineSymbol "And restricts his instincts."
			lpStrIndexSun2+4,		"Going quickly when one" apostropheSymbol "s tasks are finished" newlineSymbol "Is without blame." newlineSymbol "But one must reflect on how much one may decrease others."
			lpStrIndexSun2+5,		"Perseverance furthers." newlineSymbol "To undertake something brings misfortune." newlineSymbol "Without decreasing oneself," newlineSymbol "One is able to bring increase to others."
			lpStrIndexSun2+6,		"When three people journey together," newlineSymbol "Their number decreases by one." newlineSymbol "When one man journeys alone," newlineSymbol "He finds a companion."
			lpStrIndexSun2+7,		"If a man decreases his faults," newlineSymbol "It makes the other hasten to come and rejoice." newlineSymbol "No blame."
			lpStrIndexSun2+8,		"Someone does indeed increase him." newlineSymbol "Ten pairs of tortoises cannot oppose it." newlineSymbol "Supreme good fortune."
			lpStrIndexSun2+9,		"If one is increased without depriving others," newlineSymbol "There is no blame." newlineSymbol "Perseverance brings good fortune." newlineSymbol "It furthers one to undertake something." newlineSymbol "One obtains servants" newlineSymbol "But no longer has a separate home."
			
			lpStrIndexKuei,				"K" apostropheSymbol "uei"
			lpStrIndexKuei+1,		"Opposition"
			lpStrIndexKuei+2,		"OPPOSITION. In small matters, good fortune."
			lpStrIndexKuei+3,		"Above, fire; below, the lake:" newlineSymbol "The image of OPPOSITION." newlineSymbol "Thus amid all fellowship" newlineSymbol "The superior man retains his individuality."
			lpStrIndexKuei+4,		"Remorse disappears." newlineSymbol "If you lose your horse, do not run after it;" newlineSymbol "It will come back of its own accord." newlineSymbol "When you see evil people," newlineSymbol "Guard yourself against mistakes."
			lpStrIndexKuei+5,		"One meets his lord in a narrow street." newlineSymbol "No blame."
			lpStrIndexKuei+6,		"One sees the wagon dragged back," newlineSymbol "The oxen halted," newlineSymbol "A man" apostropheSymbol "s hair and nose cut off."	newlineSymbol "Not a good beginning, but a good end."
			lpStrIndexKuei+7,		"Isolated through opposition," newlineSymbol "One meets a like-minded man" newlineSymbol "With whom one can associate in good faith." newlineSymbol "Despite the danger, no blame."
			lpStrIndexKuei+8,		"Remorse disappears." newlineSymbol "The companion bites his way through the wrappings." newlineSymbol "If one goes to him," newlineSymbol "How could it be a mistake."
			lpStrIndexKuei+9,		"Isolated through opposition," newlineSymbol "One sees one" apostropheSymbol "s companion as a pig covered with dirt," newlineSymbol "As a wagon full of devils." newlineSymbol "First one draws a bow against him," newlineSymbol "The one lays the bow aside." newlineSymbol "He is not a robber; he will woo at the right time." newlineSymbol "As one goes, rain falls; then good fortune comes."
			
			lpStrIndexLue,				"L" uumlautSymbol
			lpStrIndexLue+1,		"Treading (Conduct)"
			lpStrIndexLue+2,		"TREADING. Treading upon the tail of the tiger." newlineSymbol "It does not bite the man. Success."
			lpStrIndexLue+3,		"Heaven above, the lake below:" newlineSymbol "The image of TREADING." newlineSymbol "Thus the superior man discriminates between high and low," newlineSymbol "And thereby fortifies the thinking of the people."
			lpStrIndexLue+4,		"Simple conduct. Progress without blame."
			lpStrIndexLue+5,		"Treading a smooth, level course." newlineSymbol "The perseverance of a dark man" newlineSymbol "Brings good fortune."
			lpStrIndexLue+6,		"A one-eyed man is able to see," newlineSymbol "A lame man is able to tread." newlineSymbol "He treads on the tail of the tiger." newlineSymbol "The tiger bites the man." newlineSymbol "Misfortune." newlineSymbol "Thus does a warrior act on behalf of his great prince."
			lpStrIndexLue+7,		"He treads on the tail of the tiger." newlineSymbol "Caution and circumspection" newlineSymbol "Lead ultimately to good fortune."
			lpStrIndexLue+8,		"Resolute conduct." newlineSymbol "Perseverance with awareness of danger."
			lpStrIndexLue+9,		"Look to your conduct and weigh the favorable signs." newlineSymbol "When everything is fulfilled, supreme good fortune comes."
			
			lpStrIndexChungFu,			"Chung Fu"
			lpStrIndexChungFu+1,	"Inner Truth"
			lpStrIndexChungFu+2,	"INNER TRUTH. Pigs and fishes." newlineSymbol "Good fortune." newlineSymbol "It furthers one to cross the great water." newlineSymbol "Perseverance furthers."
			lpStrIndexChungFu+3,	"Wind over lake: the image of INNER TRUTH." newlineSymbol "Thus the superior man discusses criminal cases" newlineSymbol "In order to delay executions."
			lpStrIndexChungFu+4,	"Being prepared brings good fortune." newlineSymbol "If there are secret designs, it is disquieting."
			lpStrIndexChungFu+5,	"A crane calling in the shade." newlineSymbol "Its young answers it." newlineSymbol "I have a good goblet." newlineSymbol "I will share it with you."
			lpStrIndexChungFu+6,	"He finds a comrade." newlineSymbol "Now he beats the drum, now he stops." newlineSymbol "Now he sobs, now he sings."
			lpStrIndexChungFu+7,	"The moon nearly at the full." newlineSymbol "The team horse goes astray." newlineSymbol "No blame."
			lpStrIndexChungFu+8,	"He possess truth, which links together." newlineSymbol "No blame."
			lpStrIndexChungFu+9,	"Cockcrow penetrating to heaven." newlineSymbol "Perseverance brings misfortune."
			
			lpStrIndexChien2,			"Chien"
			lpStrIndexChien2+1,		"Development (Gradual Progress)"
			lpStrIndexChien2+2,		"DEVELOPMENT. The maiden" newlineSymbol "Is given in marriage." newlineSymbol "Good fortune." newlineSymbol "Perseverance furthers."
			lpStrIndexChien2+3,		"On the mountain, a tree:" newlineSymbol "The image of DEVELOPMENT." newlineSymbol "Thus the superior man abides in dignity and virtue," newlineSymbol "In order to improve the mores."
			lpStrIndexChien2+4,		"The wild goose gradually draws near the shore." newlineSymbol "The young son is in danger." newlineSymbol "There is talk. No blame."
			lpStrIndexChien2+5,		"The wild goose draws gradually near the cliff." newlineSymbol "Eating and drinking in peace and accord." newlineSymbol "Good fortune."
			lpStrIndexChien2+6,		"The wild goose gradually draws near the plateau." newlineSymbol "The man goes forth and does not return." newlineSymbol "The woman carries a child but does not bring it forth." newlineSymbol "Misfortune." newlineSymbol "It furthers one to fight off robbers."
			lpStrIndexChien2+7,		"The wild goose gradually draws near the tree." newlineSymbol "Perhaps it will find a flat branch. No blame."
			lpStrIndexChien2+8,		"The wild goose draws near the summit." newlineSymbol "For three years the woman has no child." newlineSymbol "In the end nothing can hinder her." newlineSymbol "Good fortune."
			lpStrIndexChien2+9,		"The wild goose gradually draws near the cloud heights." newlineSymbol "Its feathers can be used for the sacred dance." newlineSymbol "Good fortune."				


			/////////////////////////////////////////////////////////////////
			//
			// The house of Chen
			//
		#define lpStrIndexChen		(lpStrIndexKen + 80)						// 286
		#define	lpStrIndexYue		(lpStrIndexChen + 10)
		#define lpStrIndexHsieh		(lpStrIndexChen + 20)
		#define lpStrIndexHeng		(lpStrIndexChen + 30)
		#define lpStrIndexSheng		(lpStrIndexChen + 40)
		#define lpStrIndexChing		(lpStrIndexChen + 50)
		#define lpStrIndexTaKuo		(lpStrIndexChen + 60)
		#define lpStrIndexSui		(lpStrIndexChen + 70)

			lpStrIndexChen,				"Chen"
			lpStrIndexChen+1,		"The Arousing (Shock, Thunder)"
			lpStrIndexChen+2,		"SHOCK brings success." newlineSymbol "Shock comes -- oh, oh!" newlineSymbol "Laughing words -- ha, ha!" newlineSymbol "The shock terrifies for a hundred miles," newlineSymbol "And he does not let fall the sacrificial spoon and chalice."
			lpStrIndexChen+3,		"Thunder repeated: the image of SHOCK. " newlineSymbol "Thus in fear and trembling" newlineSymbol "The superior man sets his life in order" newlineSymbol "And examines himself."
			lpStrIndexChen+4,		"Shock comes -- oh, oh!" newlineSymbol "Then follow laughing words -- ha, ha!" newlineSymbol "Good fortune."
			lpStrIndexChen+5,		"Shock comes from bringing danger." newlineSymbol "A hundred thousand times" newlineSymbol "You lose your treasures" newlineSymbol "And must climb the nine hills."
			lpStrIndexChen+6,		"Shock comes and makes one distraught." newlineSymbol "If shock spurs to action" newlineSymbol "One remains free of misfortune."
			lpStrIndexChen+7,		"Shock is mired."
			lpStrIndexChen+8,		"Shock goes hither and thither." newlineSymbol "Danger." newlineSymbol "However, nothing at all is lost." newlineSymbol "Yet there are things to be done."
			lpStrIndexChen+9,		"Shock brings ruin and terrified gazing around." newlineSymbol "Going ahead brings misfortune." newlineSymbol "If it has not yet touched one" apostropheSymbol "s own body" newlineSymbol "But has reached one" apostropheSymbol "s neighbor first," newlineSymbol "There is no blame." newlineSymbol "One" apostropheSymbol "s comrades have something to talk about."
			
			lpStrIndexYue,				"Y" uumlautSymbol
			lpStrIndexYue+1,		"Enthusiasm"
			lpStrIndexYue+2,		"ENTHUSIASM. It furthers one to install helpers" newlineSymbol "And to set armies marching."
			lpStrIndexYue+3,		"Thunder comes resounding out of the earth:" newlineSymbol "The image of ENTHUSIASM." newlineSymbol "Thus the ancient kings made music" newlineSymbol "In order to honor merit," newlineSymbol "And offered it with splendor" newlineSymbol "To the Supreme Deity," newlineSymbol "Inviting their ancestors to be present."
			lpStrIndexYue+4,		"Enthusiasm that expresses itself" newlineSymbol "Brings misfortune."
			lpStrIndexYue+5,		"Firm as a rock. Not a whole day." newlineSymbol "Perseverance brings good fortune."
			lpStrIndexYue+6,		"Enthusiasm that looks upward creates remorse." newlineSymbol "Hesitation brings remorse."
			lpStrIndexYue+7,		"The source of enthusiasm." newlineSymbol "He achieves great things." newlineSymbol "Doubt not." newlineSymbol "You gather friends around you" newlineSymbol "As a hair clasp gathers the hair."
			lpStrIndexYue+8,		"Persistently ill, and still does not die."
			lpStrIndexYue+9,		"Deluded enthusiasm." newlineSymbol "But if after completion one changes," newlineSymbol "There is no blame."
			
			lpStrIndexHsieh,			"Hsieh"
			lpStrIndexHsieh+1,		"Deliverance"
			lpStrIndexHsieh+2,		"DELIVERANCE. The southwest furthers." newlineSymbol "If there is no longer anything where one has to go," newlineSymbol "Return brings good fortune." newlineSymbol "If there is still something where one has to go," newlineSymbol "Hastening brings good fortune."
			lpStrIndexHsieh+3,		"Thunder and rain set in:" newlineSymbol "The image of DELIVERANCE." newlineSymbol "Thus the superior man pardons mistakes" newlineSymbol "And forgives misdeeds."
			lpStrIndexHsieh+4,		"Without blame."
			lpStrIndexHsieh+5,		"One kills three foxes in the field" newlineSymbol "And receives a yellow arrow." newlineSymbol "Perseverance brings good fortune."
			lpStrIndexHsieh+6,		"If a man carries a burden on his back" newlineSymbol "And nonetheless rides in a carriage," newlineSymbol "He thereby encourages robbers to draw near." newlineSymbol "Perseverance leads to humiliation."
			lpStrIndexHsieh+7,		"Deliver yourself from your great toe." newlineSymbol "Then the companion comes," newlineSymbol "And him you can trust."
			lpStrIndexHsieh+8,		"If only the superior man can deliver himself," newlineSymbol "It brings good fortune." newlineSymbol "Thus he proves to inferior men that he is in earnest."
			lpStrIndexHsieh+9,		"The prince shoots at a hawk on a high wall." newlineSymbol "He kills it. Everything serves to further."
			
			lpStrIndexHeng,				"Heng"
			lpStrIndexHeng+1,		"Duration"
			lpStrIndexHeng+2,		"DURATION. Success. No blame." newlineSymbol "Perseverance furthers." newlineSymbol "It furthers one to have somewhere to go."
			lpStrIndexHeng+3,		"Thunder and wind: the image of DURATION." newlineSymbol "Thus the superior man stands firm" newlineSymbol "And does not change his direction."
			lpStrIndexHeng+4,		"Seeking duration too hastily brings misfortune persistently." newlineSymbol "Nothing that would further."
			lpStrIndexHeng+5,		"Remorse disappears."
			lpStrIndexHeng+6,		"He who does not give duration to his character" newlineSymbol "Meets with disgrace." newlineSymbol "Persistent humiliation."
			lpStrIndexHeng+7,		"No game in the field."
			lpStrIndexHeng+8,		"Giving duration to one" apostropheSymbol "s character through perseverance." newlineSymbol "This is good fortune for a woman, misfortune for a man."
			lpStrIndexHeng+9,		"Restlessness as an enduring condition brings misfortune."
			
			lpStrIndexSheng,			"Sheng"
			lpStrIndexSheng+1,		"Pushing Upward"
			lpStrIndexSheng+2,		"PUSHING UPWARD has supreme success." newlineSymbol "One must see the great man." newlineSymbol "Fear not." newlineSymbol "Departure toward the south" newlineSymbol "Brings good fortune."
			lpStrIndexSheng+3,		"Within the earth, wood grows:" newlineSymbol "The image of PUSHING UPWARD." newlineSymbol "Thus the superior man of devoted character" newlineSymbol "Heaps up small things" newlineSymbol "In order to achieve something high and great."
			lpStrIndexSheng+4,		"Pushing upward that meets with confidence."
			lpStrIndexSheng+5,		"If one is sincere," newlineSymbol "It furthers one to bring even a small offering." newlineSymbol "No blame."
			lpStrIndexSheng+6,		"One pushes upward into an empty city."
			lpStrIndexSheng+7,		"The king offers him Mount Ch" apostropheSymbol "i." newlineSymbol "Good fortune. No blame."
			lpStrIndexSheng+8,		"Perseverance brings good fortune." newlineSymbol "One pushes upward by steps."
			lpStrIndexSheng+9,		"Pushing upward in darkness." newlineSymbol "It furthers one" newlineSymbol "To be unremittingly persevering."
			
			lpStrIndexChing,			"Ching"
			lpStrIndexChing+1,		"The Well"
			lpStrIndexChing+2,		"THE WELL. The town may be changed," newlineSymbol "But the well cannot be changed." newlineSymbol "It neither decreases nor increases." newlineSymbol "They come and go and draw from the well."
			lpStrIndexChing+3,		"Water over wood: the image of THE WELL." newlineSymbol "Thus the superior man encourages the people at their work," newlineSymbol "And exhorts them to help one another." newlineSymbol "If one gets down almost to the water" newlineSymbol "And the rope does not go all the way," newlineSymbol "Or the jug breaks, it brings misfortune."
			lpStrIndexChing+4,		"One does not drink the mud of the well." newlineSymbol "No animals come to an old well."
			lpStrIndexChing+5,		"At the wellhole one shoot fishes." newlineSymbol "The jug is broken and leaks."
			lpStrIndexChing+6,		"The well is cleaned, but no one drinks from it." newlineSymbol "This is my heart" apostropheSymbol "s sorrow." newlineSymbol "For one might draw from it." newlineSymbol "If the king were clear-minded," newlineSymbol "Good fortune might be enjoyed in common."
			lpStrIndexChing+7,		"The well is being lined. No blame."
			lpStrIndexChing+8,		"In the well there is a clear, cold spring" newlineSymbol "From which one can drink."
			lpStrIndexChing+9,		"One draws from the well" newlineSymbol "Without hindrance." newlineSymbol "It is dependable." newlineSymbol "Supreme good fortune."
			
			lpStrIndexTaKuo,			"Ta Kuo"
			lpStrIndexTaKuo+1,		"Preponderance of the Great"
			lpStrIndexTaKuo+2,		"PREPONDERANCE OF THE GREAT." newlineSymbol "The ridgepole sags at the breaking point." newlineSymbol "It furthers one to have somewhere to go." newlineSymbol "Success."
			lpStrIndexTaKuo+3,		"The lake rises above the trees:" newlineSymbol "The image of PREPONDERANCE OF THE GREAT." newlineSymbol "Thus the superior man, when he stands alone," newlineSymbol "Is unconcerned," newlineSymbol "And if he has to renounce the world," newlineSymbol "He is undaunted."
			lpStrIndexTaKuo+4,		"To spread white rushes underneath." newlineSymbol "No blame."
			lpStrIndexTaKuo+5,		"A dry poplar sprouts at the root." newlineSymbol "An older man takes a young wife." newlineSymbol "Everything furthers."
			lpStrIndexTaKuo+6,		"The ridgepole sags to the breaking point." newlineSymbol "Misfortune."
			lpStrIndexTaKuo+7,		"The ridgepole is braced. Good fortune." newlineSymbol "If there are ulterior motives, it is humiliating."
			lpStrIndexTaKuo+8,		"A withered poplar puts forth flowers." newlineSymbol "An older woman takes a husband." newlineSymbol "No blame.  No praise"
			lpStrIndexTaKuo+9,		"One must go through the water." newlineSymbol "It goes over one" apostropheSymbol "s head." newlineSymbol "Misfortune. No blame."
			
			lpStrIndexSui,				"Sui"
			lpStrIndexSui+1,		"Following"
			lpStrIndexSui+2,		"FOLLOWING has supreme success." newlineSymbol "Perseverance furthers. No blame."
			lpStrIndexSui+3,		"Thunder in the middle of the lake:" newlineSymbol "The image of FOLLOWING." newlineSymbol "Thus the superior man at nightfall" newlineSymbol "Goes indoors for rest and recuperation."
			lpStrIndexSui+4,		"The standard is changing." newlineSymbol "Perseverance brings good fortune." newlineSymbol "To go out of the door in company" newlineSymbol "Produces deeds."
			lpStrIndexSui+5,		"If one clings to the little boy," newlineSymbol "One loses the strong man." newlineSymbol ""
			lpStrIndexSui+6,		"If one clings to the strong man," newlineSymbol "One loses the little boy." newlineSymbol "Though following one finds what one seeks." newlineSymbol "It furthers one to remain persevering."
			lpStrIndexSui+7,		"Following creates success." newlineSymbol "Perseverance brings good fortune." newlineSymbol "To go one" apostropheSymbol "s way with sincerity brings clarity." newlineSymbol "How could there be blame in this?"
			lpStrIndexSui+8,		"Sincere in the good. Good fortune."
			lpStrIndexSui+9,		"He meets with firm allegiance" newlineSymbol "And is still further bound." newlineSymbol "The king introduces him" newlineSymbol "To the Western Mountain."				


			/////////////////////////////////////////////////////////////////
			//
			// The house of Sun
			//
		#define lpStrIndexSun		(lpStrIndexChen + 80)						// 366
		#define	lpStrIndexHsiaoChu	(lpStrIndexSun + 10)
		#define lpStrIndexChiaJen	(lpStrIndexSun + 20)
		#define lpStrIndexI			(lpStrIndexSun + 30)
		#define lpStrIndexWuWang	(lpStrIndexSun + 40)
		#define lpStrIndexShihHo	(lpStrIndexSun + 50)
		#define lpStrIndexI2		(lpStrIndexSun + 60)
		#define lpStrIndexKu		(lpStrIndexSun + 70)

			lpStrIndexSun,				"Sun"
			lpStrIndexSun+1,		"The Gentle (The Penetrating, Wind)"
			lpStrIndexSun+2,		"THE GENTLE. Success through what is small." newlineSymbol "It furthers one to have somewhere to go." newlineSymbol "It furthers one to see the great man."
			lpStrIndexSun+3,		"Winds following one upon the other:" newlineSymbol "The image of THE GENTLY PENETRATING." newlineSymbol "Thus the superior man" newlineSymbol "Spreads his commands abroad" newlineSymbol "And carries out his undertakings."
			lpStrIndexSun+4,		"In advancing and in retreating," newlineSymbol "The perseverance of a warrior furthers."
			lpStrIndexSun+5,		"Penetration under the bed." newlineSymbol "Priests and magicians are used in great number." newlineSymbol "Good fortune. No blame."
			lpStrIndexSun+6,		"Repeated penetration. Humiliation."
			lpStrIndexSun+7,		"Remorse vanishes." newlineSymbol "During the hunt" newlineSymbol "Three kinds of game are caught."
			lpStrIndexSun+8,		"Perseverance brings good fortune." newlineSymbol "Remorse vanishes." newlineSymbol "Nothing that does not further." newlineSymbol "No beginning, but an end." newlineSymbol "Before the change, three days." newlineSymbol "After the change, three days." newlineSymbol "Good fortune."
			lpStrIndexSun+9,		"Penetration under the bed." newlineSymbol "He loses his property and his ax." newlineSymbol "Perseverance brings misfortune."
			
			lpStrIndexHsiaoChu,			"Hsiao Ch" apostropheSymbol "u"
			lpStrIndexHsiaoChu+1,	"The Taming Power of the Small"
			lpStrIndexHsiaoChu+2,	"THE TAMING POWER OF THE SMALL" newlineSymbol "Has success." newlineSymbol "Dense clouds, no rain from our western region."
			lpStrIndexHsiaoChu+3,	"The wind drives across heaven:" newlineSymbol "The image of THE TAMING POWER OF THE SMALL." newlineSymbol "Thus the superior man" newlineSymbol "Refines the outward aspect of his nature."
			lpStrIndexHsiaoChu+4,	"Return to the way." newlineSymbol "How could there be blame in this?" newlineSymbol "Good fortune."
			lpStrIndexHsiaoChu+5,	"He allows himself to be drawn into returning." newlineSymbol "Good fortune."
			lpStrIndexHsiaoChu+6,	"The spokes burst out of the wagon wheels." newlineSymbol "Man and wife roll their eyes."
			lpStrIndexHsiaoChu+7,	"If you are sincere, blood vanishes and fear gives way." newlineSymbol "No blame."
			lpStrIndexHsiaoChu+8,	"If you are sincere and loyally attached," newlineSymbol "You are rich in your neighbor."
			lpStrIndexHsiaoChu+9,	"The rain comes, there is rest." newlineSymbol "This is due to the lasting effect of the character." newlineSymbol "Perseverance brings the woman into danger." newlineSymbol "The moon is nearly full." newlineSymbol "If the superior man persists," newlineSymbol "Misfortune comes."
			
			lpStrIndexChiaJen,			"Chia Jen"
			lpStrIndexChiaJen+1,	"The Family (The Clan)"
			lpStrIndexChiaJen+2,	"THE FAMILY. The perseverance of the woman furthers."
			lpStrIndexChiaJen+3,	"Wind comes forth from fire:" newlineSymbol "The image of THE FAMILY." newlineSymbol "Thus the superior man has substance in his words" newlineSymbol "And duration in his way of life."
			lpStrIndexChiaJen+4,	"Firm seclusion within the family." newlineSymbol "Remorse disappears."
			lpStrIndexChiaJen+5,	"She could not follow her whims." newlineSymbol "She must attend within to the food." newlineSymbol "Perseverance brings good fortune."
			lpStrIndexChiaJen+6,	"When tempers flare up in the family," newlineSymbol "Too great severity brings remorse." newlineSymbol "Good fortune nonetheless." newlineSymbol "When woman and child dally and laugh," newlineSymbol "It leads in the end to humiliation."
			lpStrIndexChiaJen+7,	"She is the treasure of the house." newlineSymbol "Good fortune."
			lpStrIndexChiaJen+8,	"As a king he approaches his family." newlineSymbol "Fear not." newlineSymbol "Good fortune."
			lpStrIndexChiaJen+9,	"His work commands respect." newlineSymbol "In the end good fortune comes."
			
			lpStrIndexI,				"I"
			lpStrIndexI+1,			"Increase"
			lpStrIndexI+2,			"INCREASE. It furthers one to undertake something." newlineSymbol "It furthers one to cross the great water."
			lpStrIndexI+3,			"Wind and thunder: the image of INCREASE." newlineSymbol "Thus the superior man:" newlineSymbol "If he sees good, he imitates it;" newlineSymbol "If he has faults, he rids himself of them."
			lpStrIndexI+4,			"It furthers one to accomplish great deeds." newlineSymbol "Supreme good fortune. No blame."
			lpStrIndexI+5,			"Someone does indeed increase him;" newlineSymbol "Ten pairs of tortoises cannot oppose it." newlineSymbol "Constant perseverance brings good fortune." newlineSymbol "The king presents him before God. Good fortune!"
			lpStrIndexI+6,			"One is enriched through unfortunate events." newlineSymbol "No blame, if you are sincere" newlineSymbol "And walk in the middle," newlineSymbol "And report with a seal to the prince."
			lpStrIndexI+7,			"If you walk in the middle " newlineSymbol "And report to the prince," newlineSymbol "He will follow." newlineSymbol "It furthers one to be used" newlineSymbol "In the removal of the capital."
			lpStrIndexI+8,			"If in truth you have a kind heart, ask not." newlineSymbol "Supreme good fortune." newlineSymbol "Truly, kindness will be recognized as your virtue."
			lpStrIndexI+9,			"He brings increase to no one." newlineSymbol "Indeed, someone even strikes him." newlineSymbol "He does not keep his heart constantly steady." newlineSymbol "Misfortune."
			
			lpStrIndexWuWang,			"Wu Wang"
			lpStrIndexWuWang+1,		"Innocence (The Unexpected)"
			lpStrIndexWuWang+2,		"INNOCENCE. Supreme success." newlineSymbol "Perseverance furthers." newlineSymbol "If someone is not as he should be," newlineSymbol "He has misfortune," newlineSymbol "And it does not further him" newlineSymbol "To undertake anything."
			lpStrIndexWuWang+3,		"Under heaven thunder rolls:" newlineSymbol "All things attain the natural state of innocence." newlineSymbol "Thus the kings of old," newlineSymbol "Rich in virtue, and in harmony with the time," newlineSymbol "Fostered and nourished all beings."
			lpStrIndexWuWang+4,		"Innocent behavior brings good fortune."
			lpStrIndexWuWang+5,		"If one does not count on the harvest while plowing," newlineSymbol "Nor on the use of the ground while clearing it," newlineSymbol "It furthers one to undertake something."
			lpStrIndexWuWang+6,		"Undeserved misfortune." newlineSymbol "The cow that was tethered by someone" newlineSymbol "Is the wanderer" apostropheSymbol "s gain, the citizen" apostropheSymbol "s loss."
			lpStrIndexWuWang+7,		"He who can be persevering " newlineSymbol "Remains without blame."
			lpStrIndexWuWang+8,		"Use no medicine in an illness" newlineSymbol "Incurred through no fault of your own." newlineSymbol "It will pass of itself."
			lpStrIndexWuWang+9,		"Innocent action brings misfortune." newlineSymbol "Nothing furthers."
			
			lpStrIndexShihHo,			"Shih Ho"
			lpStrIndexShihHo+1,		"Biting Through"
			lpStrIndexShihHo+2,		"BITING THROUGH has success." newlineSymbol "It is favorable to let justice be administered."
			lpStrIndexShihHo+3,		"Thunder and lightning:" newlineSymbol "The image of BITING THROUGH." newlineSymbol "Thus the kings of former times made firm the laws" newlineSymbol "Through clearly defined penalties."
			lpStrIndexShihHo+4,		"His feet are fastened in stocks," newlineSymbol "So that his toes disappear." newlineSymbol "No blame."
			lpStrIndexShihHo+5,		"Bites through tender meat," newlineSymbol "So that his nose disappears." newlineSymbol "No blame."
			lpStrIndexShihHo+6,		"Bites on old dried meat" newlineSymbol "And strikes on something poisonous." newlineSymbol "Slight humiliation.  No blame."
			lpStrIndexShihHo+7,		"Bites on dried gristly meat." newlineSymbol "Receives metal arrows." newlineSymbol "It furthers one to be mindful of difficulties" newlineSymbol "And to be persevering." newlineSymbol "Good fortune."
			lpStrIndexShihHo+8,		"Bites on dried lean meat." newlineSymbol "Receives yellow gold." newlineSymbol "Perseveringly aware of danger." newlineSymbol "No blame."
			lpStrIndexShihHo+9,		"His neck is fastened in the wooden cangue," newlineSymbol "So that his ears disappear." newlineSymbol "Misfortune."
			
			lpStrIndexI2,				"I"
			lpStrIndexI2+1,			"The Corners of the Mouth (Providing Nourishment)"
			lpStrIndexI2+2,			"THE CORNERS OF THE MOUTH." newlineSymbol "Perseverance brings good fortune." newlineSymbol "Pay heed to the providing of nourishment" newlineSymbol "And to what a man seeks" newlineSymbol "To fill his own mouth with."
			lpStrIndexI2+3,			"At the foot of the mountain, thunder:" newlineSymbol "The image of PROVIDING NOURISHMENT." newlineSymbol "Thus the superior man is careful of his own words" newlineSymbol "And temperate in eating and drinking."
			lpStrIndexI2+4,			"You let your magic tortoise go," newlineSymbol "And look at me with the corners of your mouth drooping." newlineSymbol "Misfortune."
			lpStrIndexI2+5,			"Turning to the summit for nourishment," newlineSymbol "Deviating from the path" newlineSymbol "To seek nourishment from the hill." newlineSymbol "Continuing to do so brings misfortune."
			lpStrIndexI2+6,			"Turning away from nourishment." newlineSymbol "Perseverance brings misfortune." newlineSymbol "Do not act thus for ten years." newlineSymbol "Nothing serves to further."
			lpStrIndexI2+7,			"Turning to the summit" newlineSymbol "For provision of nourishment" newlineSymbol "Brings good fortune." newlineSymbol "Spying about with sharp eyes" newlineSymbol "Like a tiger with insatiable craving." newlineSymbol "No blame."
			lpStrIndexI2+8,			"Turning away from the path." newlineSymbol "To remain persevering brings good fortune." newlineSymbol "One should not cross the great water."
			lpStrIndexI2+9,			"The source of nourishment." newlineSymbol "Awareness of danger brings good fortune." newlineSymbol "It furthers one to cross the great water."
			
			lpStrIndexKu,				"Ku"
			lpStrIndexKu+1,			"Work on what has been Spoiled (Decay)"
			lpStrIndexKu+2,			"WORK ON WHAT HAS BEEN SPOILED" newlineSymbol "Has supreme success." newlineSymbol "It furthers one to cross the great water." newlineSymbol "Before the starting point, three days." newlineSymbol "After the starting point, three days."
			lpStrIndexKu+3,			"The wind blows low on the mountain:" newlineSymbol "The image of DECAY." newlineSymbol "Thus the superior man stirs up the people" newlineSymbol "And strengthens their spirit."
			lpStrIndexKu+4,			"Setting right what has been spoiled by the father." newlineSymbol "If there is a son," newlineSymbol "No blame rests on the departed father." newlineSymbol "Danger. In the end good fortune."
			lpStrIndexKu+5,			"Setting right what has been spoiled by the mother." newlineSymbol "One must not be too persevering."
			lpStrIndexKu+6,			"Setting right what has been spoiled by the father." newlineSymbol "There will be a little remorse. No great blame."
			lpStrIndexKu+7,			"Tolerating what has been spoiled by the father." newlineSymbol "In continuing one sees humiliation."
			lpStrIndexKu+8,			"Setting right what has been spoiled by the father." newlineSymbol "One meets with praise."
			lpStrIndexKu+9,			"He does not serve kings and princes," newlineSymbol "Set himself higher goals."				

			/////////////////////////////////////////////////////////////////
			//
			// The house of Li
			//
		#define lpStrIndexLi		(lpStrIndexSun + 80)						// 446
		#define	lpStrIndexLue2		(lpStrIndexLi + 10)
		#define lpStrIndexTing		(lpStrIndexLi + 20)
		#define lpStrIndexWeiChi	(lpStrIndexLi + 30)
		#define lpStrIndexMeng		(lpStrIndexLi + 40)
		#define lpStrIndexHuan		(lpStrIndexLi + 50)
		#define lpStrIndexSung		(lpStrIndexLi + 60)
		#define lpStrIndexTungJen	(lpStrIndexLi + 70)

			lpStrIndexLi,				"Li"
			lpStrIndexLi+1,			"The Clinging, Fire"
			lpStrIndexLi+2,			"THE CLINGING. Perseverance furthers." newlineSymbol "It brings success." newlineSymbol "Care of the cow brings good fortune."
			lpStrIndexLi+3,			"That which is bright rises twice:" newlineSymbol "The image of FIRE." newlineSymbol "Thus the great man, by perpetuating this brightness," newlineSymbol "Illuminates the four quarters of the world."
			lpStrIndexLi+4,			"The footprints run crisscross." newlineSymbol "If one is seriously intent, no blame."
			lpStrIndexLi+5,			"Yellow light. Supreme good fortune."
			lpStrIndexLi+6,			"In the light of the setting sun," newlineSymbol "Men either beat the pot and sing" newlineSymbol "Or loudly bewail the approach of old age." newlineSymbol "Misfortune."
			lpStrIndexLi+7,			"Its coming is sudden." newlineSymbol "It flames up, dies down" newlineSymbol " is thrown away."
			lpStrIndexLi+8,			"Tears in floods, sighing and lamenting." newlineSymbol "Good fortune."
			lpStrIndexLi+9,			"The king uses him to march forth and chastise." newlineSymbol "Then it is best to kill the leaders" newlineSymbol "And take captive the followers. No blame."
			
			lpStrIndexLue2,				"L" uumlautSymbol
			lpStrIndexLue2+1,		"The Wanderer"
			lpStrIndexLue2+2,		"THE WANDERER. Success through smallness." newlineSymbol "Perseverance brings good fortune" newlineSymbol "To the wanderer."
			lpStrIndexLue2+3,		"Fire on the mountain:" newlineSymbol "The image of THE WANDERER." newlineSymbol "Thus the superior man" newlineSymbol "Is clear-minded and cautious" newlineSymbol "In imposing penalties," newlineSymbol "And protracts no lawsuits."
			lpStrIndexLue2+4,		"If the wanderer busies himself with trivial things," newlineSymbol "He draws down misfortune upon himself."
			lpStrIndexLue2+5,		"The wanderer comes to an inn." newlineSymbol "He has his property with him." newlineSymbol "He wins the steadfastness of a young servant."
			lpStrIndexLue2+6,		"The wanderer" apostropheSymbol "s in burns down." newlineSymbol "He loses the steadfastness of his young servant." newlineSymbol "Danger."
			lpStrIndexLue2+7,		"The wanderer rests in a shelter." newlineSymbol "He obtains his property and an ax." newlineSymbol "My heart is not glad."
			lpStrIndexLue2+8,		"He shoots a pheasant." newlineSymbol "It drops with the first arrow." newlineSymbol "In the end this brings both praise and office."
			lpStrIndexLue2+9,		"The bird" apostropheSymbol "s nest burns up." newlineSymbol "The wanderer laughs at first," newlineSymbol "Then must needs lament and weep." newlineSymbol "Through carelessness he loses his cow." newlineSymbol "Misfortune."
			
			lpStrIndexTing,				"Ting"
			lpStrIndexTing+1,		"The Caldron"
			lpStrIndexTing+2,		"THE CALDRON. Supreme good fortune." newlineSymbol "Success."
			lpStrIndexTing+3,		"Fire over wood:" newlineSymbol "The image of THE CALDRON." newlineSymbol "Thus the superior man consolidates his fate" newlineSymbol "By making his position correct."
			lpStrIndexTing+4,		"A ting with legs upturned." newlineSymbol "Furthers removal of stagnating stuff." newlineSymbol "One takes a concubine for the sake of her son." newlineSymbol "No blame."
			lpStrIndexTing+5,		"There is food in the ting." newlineSymbol "My comrades are envious," newlineSymbol "But they cannot harm me." newlineSymbol "Good fortune."
			lpStrIndexTing+6,		"The handle of the ting is altered." newlineSymbol "One is impeded in his way of life." newlineSymbol "The fat of the pheasant is not eaten." newlineSymbol "Once rain falls, remorse is spent." newlineSymbol "Good fortune comes in the end."
			lpStrIndexTing+7,		"The legs of the ting are broken." newlineSymbol "The prince" apostropheSymbol "s meal is spilled" newlineSymbol "And his person soiled." newlineSymbol "Misfortune."
			lpStrIndexTing+8,		"The ting has yellow handles, golden carrying rings." newlineSymbol "Perseverance furthers."
			lpStrIndexTing+9,		"The ting has rings of jade." newlineSymbol "Great good fortune." newlineSymbol "Nothing that would not act to further."
			
			lpStrIndexWeiChi,			"Wei Chi"
			lpStrIndexWeiChi+1,		"Before Completion"
			lpStrIndexWeiChi+2,		"BEFORE COMPLETION. Success." newlineSymbol "But if the little fox, after nearly completing the crossing," newlineSymbol "Gets his tail in the water," newlineSymbol "There is nothing that would further."
			lpStrIndexWeiChi+3,		"Fire over water:" newlineSymbol "The image of the condition before transition." newlineSymbol "Thus the superior man is careful" newlineSymbol "In the differentiation of things," newlineSymbol "So that each finds its place."
			lpStrIndexWeiChi+4,		"He gets his tail in the water." newlineSymbol "Humiliating."
			lpStrIndexWeiChi+5,		"He brakes his wheels." newlineSymbol "Perseverance brings good fortune."
			lpStrIndexWeiChi+6,		"Before completion, attack brings misfortune." newlineSymbol "It furthers one to cross the great water."
			lpStrIndexWeiChi+7,		"Perseverance brings good fortune." newlineSymbol "Remorse disappears." newlineSymbol "Shock, thus to discipline the Devil" apostropheSymbol "s Country." newlineSymbol "For three years, great realms are awarded."
			lpStrIndexWeiChi+8,		"Perseverance brings good fortune." newlineSymbol "No remorse." newlineSymbol "The light of the superior man is true." newlineSymbol "Good fortune."
			lpStrIndexWeiChi+9,		"There is drinking of wine" newlineSymbol "In genuine confidence. No blame." newlineSymbol "But if one wets his head," newlineSymbol "He loses it, in truth."
			
			lpStrIndexMeng,				"Meng"
			lpStrIndexMeng+1,		"Youthful Folly"
			lpStrIndexMeng+2,		"YOUTHFUL FOLLY has success;" newlineSymbol "It is not I who seek the young fool;" newlineSymbol "The young fool seeks me." newlineSymbol "At the first oracle I inform him." newlineSymbol "If he asks two or three times, it is importunity." newlineSymbol "If he importunes, I give him no information." newlineSymbol "Perseverance furthers." newlineSymbol ""
			lpStrIndexMeng+3,		"A spring wells up at the foot of the mountain:" newlineSymbol "The image of YOUTH." newlineSymbol "Thus the superior man fosters his character" newlineSymbol "By thoroughness in all that he does."
			lpStrIndexMeng+4,		"To make a fool develop" newlineSymbol "It furthers one to apply discipline." newlineSymbol "The fetters should be removed." newlineSymbol "To go on in this way brings humiliation."
			lpStrIndexMeng+5,		"To bear with fools in kindliness brings good fortune." newlineSymbol "To know how to take women brings good fortune." newlineSymbol "The son is capable of taking charge of the household."
			lpStrIndexMeng+6,		"Take not a maiden who, when she sees a man of bronze," newlineSymbol "Loses possession of herself." newlineSymbol "Nothing furthers."
			lpStrIndexMeng+7,		"Entangled folly brings humiliation."
			lpStrIndexMeng+8,		"Childlike folly brings good fortune."
			lpStrIndexMeng+9,		"In punishing folly," newlineSymbol "It does not further one" newlineSymbol "To commit transgressions." newlineSymbol "The only thing that furthers" newlineSymbol "Is to prevent transgressions."
			
			lpStrIndexHuan,				"Huan"
			lpStrIndexHuan+1,		"Dispersion (Dissolution)"
			lpStrIndexHuan+2,		"DISPERSION. Success." newlineSymbol "The king approaches his temple." newlineSymbol "It furthers one to cross the great water." newlineSymbol "Perseverance furthers."
			lpStrIndexHuan+3,		"The wind drives over the water:" newlineSymbol "The image of DISPERSION." newlineSymbol "Thus the kings of old sacrificed to the Lord" newlineSymbol "And built temples."
			lpStrIndexHuan+4,		"He brings help with the strength of a horse." newlineSymbol "Good fortune."
			lpStrIndexHuan+5,		"At the dissolution" newlineSymbol "He hurries to that which supports him." newlineSymbol "Remorse disappears."
			lpStrIndexHuan+6,		"He dissolves his self. No remorse."
			lpStrIndexHuan+7,		"He dissolves his bond with his group." newlineSymbol "Supreme good fortune." newlineSymbol "Dispersion leads in turn to accumulation." newlineSymbol "This is something that ordinary men do not think of."
			lpStrIndexHuan+8,		"His loud cries are dissolving as sweat." newlineSymbol "Dissolution! A king abides without blame."
			lpStrIndexHuan+9,		"He dissolves his blood." newlineSymbol "Departing, keeping at a distance, going out," newlineSymbol "Is without blame."
			
			lpStrIndexSung,				"Sung"
			lpStrIndexSung+1,		"Conflict"
			lpStrIndexSung+2,		"CONFLICT. You are sincere" newlineSymbol "And are being obstructed." newlineSymbol "A cautious halt halfway brings good fortune." newlineSymbol "Going through to the end brings misfortune." newlineSymbol "It furthers one to see the great man." newlineSymbol "It does not further one to cross the great water." newlineSymbol ""
			lpStrIndexSung+3,		"Heaven and water go their opposite ways:" newlineSymbol "The image of CONFLICT." newlineSymbol "Thus in all his transactions the superior man" newlineSymbol "Carefully considers the beginning."
			lpStrIndexSung+4,		"If one does not perpetuate the affair," newlineSymbol "There is a little gossip." newlineSymbol "In the end, good fortune comes."
			lpStrIndexSung+5,		"One cannot engage in conflict;" newlineSymbol "One returns home, gives way." newlineSymbol "The people of his town," newlineSymbol "Three hundred households," newlineSymbol "Remain free of guilt."
			lpStrIndexSung+6,		"To nourish oneself on ancient virtue induces perseverance." newlineSymbol "Danger. In the end, good fortune comes." newlineSymbol "If by chance you are in the service of a king," newlineSymbol "Seek not works."
			lpStrIndexSung+7,		"One cannot engage in conflict." newlineSymbol "One turns back and submits to fate," newlineSymbol "Changes one" apostropheSymbol "s attitude," newlineSymbol "And finds peace in perseverance." newlineSymbol "Good fortune."
			lpStrIndexSung+8,		"To contend before him" newlineSymbol "Brings supreme good fortune."
			lpStrIndexSung+9,		"Even if by chance a leather belt is bestowed on one," newlineSymbol "By the end of a morning" newlineSymbol "It will have been snatched away three times."
			
			lpStrIndexTungJen,			"T" apostropheSymbol "ung Jen"
			lpStrIndexTungJen+1,	"Fellowship with Men"
			lpStrIndexTungJen+2,	"FELLOWSHIP WITH MEN in the open." newlineSymbol "Success." newlineSymbol "It furthers one to cross the great water." newlineSymbol "The perseverance of the superior man furthers."
			lpStrIndexTungJen+3,	"Heaven together with fire:" newlineSymbol "The image of FELLOWSHIP WITH MEN." newlineSymbol "Thus the superior man organizes the clans" newlineSymbol "And makes distinctions between things."
			lpStrIndexTungJen+4,	"Fellowship with men at the gate." newlineSymbol "No blame."
			lpStrIndexTungJen+5,	"Fellowship with men in the clan." newlineSymbol "Humiliation."
			lpStrIndexTungJen+6,	"He hides weapons in the thicket;" newlineSymbol "He climbs the high hill in front of it." newlineSymbol "For three years he does not rise up."
			lpStrIndexTungJen+7,	"He climbs up on his wall; he cannot attack." newlineSymbol "Good fortune."
			lpStrIndexTungJen+8,	"Men bound in fellowship first weep and lament," newlineSymbol "But afterward they laugh." newlineSymbol "After great struggles they succeed in meeting."
			lpStrIndexTungJen+9,	"Fellowship with men in the meadow." newlineSymbol "No remorse."				

			/////////////////////////////////////////////////////////////////
			//
			// The house of K'un
			//
		#define lpStrIndexKun		(lpStrIndexLi + 80)							// 526
		#define	lpStrIndexFu		(lpStrIndexKun + 10)
		#define lpStrIndexLin		(lpStrIndexKun + 20)
		#define lpStrIndexTai		(lpStrIndexKun + 30)
		#define lpStrIndexTaChuang	(lpStrIndexKun + 40)
		#define lpStrIndexKuai		(lpStrIndexKun + 50)
		#define lpStrIndexHsue		(lpStrIndexKun + 60)
		#define lpStrIndexPi3		(lpStrIndexKun + 70)

			lpStrIndexKun,				"K" apostropheSymbol "un"
			lpStrIndexKun+1,		"The Receptive"
			lpStrIndexKun+2,		"THE RECEPTIVE brings about sublime success," newlineSymbol "Furthering through the perseverance of a mare." newlineSymbol "If the superior man undertakes something and tries to lead," newlineSymbol "He goes astray;" newlineSymbol "But if he follows, he finds guidance." newlineSymbol "It is favorable to find friends in the west and south," newlineSymbol "To forego friends in the east and north." newlineSymbol "Quiet perseverance brings good fortune."
			lpStrIndexKun+3,		"The earth" apostropheSymbol "s condition is receptive devotion." newlineSymbol "Thus the superior man who has breadth of characte" newlineSymbol "Carries the outer world." newlineSymbol ""
			lpStrIndexKun+4,		"When there is hoarfrost underfoot," newlineSymbol "Solid ice is not far off." newlineSymbol ""
			lpStrIndexKun+5,		"Straight, square, great." newlineSymbol "Without purpose," newlineSymbol "Yet nothing remains unfurthered."
			lpStrIndexKun+6,		"Hidden lines." newlineSymbol "One is able to remain persevering." newlineSymbol "If by chance you are in the service of a king," newlineSymbol "Seek not works, but bring to completion."
			lpStrIndexKun+7,		"A tied-up sack. No blame, no praise."
			lpStrIndexKun+8,		"A yellow lower garment brings supreme good fortune."
			lpStrIndexKun+9,		"Dragons fight in the meadow." newlineSymbol "Their blood is black and yellow." newlineSymbol "When all the lines are sixes, it means:" newlineSymbol "Lasting perseverance furthers."
			
			lpStrIndexFu,				"Fu"
			lpStrIndexFu+1,			"Return (The Turning Point)"
			lpStrIndexFu+2,			"RETURN. Success." newlineSymbol "Going out and coming in without error." newlineSymbol "Friends come without blame." newlineSymbol "To and fro goes the way." newlineSymbol "On the seventh day comes return." newlineSymbol "It furthers one to have somewhere to go."
			lpStrIndexFu+3,			"Thunder within the earth:" newlineSymbol "The image of THE TURNING POINT." newlineSymbol "Thus the kings of antiquity closed the passes" newlineSymbol "At the time of the solstice." newlineSymbol "Merchants and strangers did not go about," newlineSymbol "And the ruler" newlineSymbol "Did not travel through the provinces."
			lpStrIndexFu+4,			"Return from a short distance." newlineSymbol "No need for remorse."
			lpStrIndexFu+5,			"Quiet return. Good fortune."
			lpStrIndexFu+6,			"Repeated return. Danger. No blame."
			lpStrIndexFu+7,			"Walking in the midst of others," newlineSymbol "One returns alone."
			lpStrIndexFu+8,			"Noble-hearted return. No remorse."
			lpStrIndexFu+9,			"Missing the return. Misfortune." newlineSymbol "Misfortune from within and without." newlineSymbol "If armies are set to marching in this way," newlineSymbol "One will in the end suffer a great defeat," newlineSymbol "Disastrous for the ruler of the country." newlineSymbol "For ten years" newlineSymbol "It will not be possible to attack again"
			
			lpStrIndexLin,				"Lin"
			lpStrIndexLin+1,		"Approach"
			lpStrIndexLin+2,		"APPROACH has supreme success." newlineSymbol "Perseverance furthers." newlineSymbol "When the eighth month comes," newlineSymbol "There will be misfortune."
			lpStrIndexLin+3,		"The earth above the lake:" newlineSymbol "The image of APPROACH." newlineSymbol "Thus the superior man is inexhaustible" newlineSymbol "In his will to teach," newlineSymbol "And without limits" newlineSymbol "In his tolerance and protection of the people."
			lpStrIndexLin+4,		"Joint approach." newlineSymbol "Perseverance brings good fortune."
			lpStrIndexLin+5,		"Joint approach." newlineSymbol "Good fortune." newlineSymbol "Everything furthers."
			lpStrIndexLin+6,		"Comfortable approach." newlineSymbol "Nothing that would further." newlineSymbol "If one is induced to grieve over it," newlineSymbol "One becomes free of blame."
			lpStrIndexLin+7,		"Complete approach." newlineSymbol "No blame."
			lpStrIndexLin+8,		"Wise approach." newlineSymbol "This is right for a great prince." newlineSymbol "Good fortune."
			lpStrIndexLin+9,		"Great-hearted approach." newlineSymbol "Good fortune. No blame."
			
			lpStrIndexTai,				"T" apostropheSymbol "ai"
			lpStrIndexTai+1,		"Peace"
			lpStrIndexTai+2,		"PEACE. The small departs," newlineSymbol "The great approaches." newlineSymbol "Good fortune. Success."
			lpStrIndexTai+3,		"Heaven and earth unite: the image of PEACE." newlineSymbol "Thus the ruler" newlineSymbol "Divides and completes the course of heaven and earth;" newlineSymbol "He furthers and regulates the gifts of heaven and earth," newlineSymbol "And so aids the people."
			lpStrIndexTai+4,		"When ribbon grass is pulled up, the sod comes with it." newlineSymbol "Each according to his kind." newlineSymbol "Undertakings bring good fortune."
			lpStrIndexTai+5,		"Bearing with the uncultured in gentleness," newlineSymbol "Fording the river with resolution," newlineSymbol "Not neglecting what is distant," newlineSymbol "Not regarding one" apostropheSymbol "s companions:" newlineSymbol "Thus one may manage to walk in the middle." newlineSymbol ""
			lpStrIndexTai+6,		"No plain not followed by a slope." newlineSymbol "No going not followed by a return." newlineSymbol "He who remains persevering in danger" newlineSymbol "Is without blame." newlineSymbol "Do not complain about this truth;" newlineSymbol "Enjoy the good fortune you still possess."
			lpStrIndexTai+7,		"He flutters down, not boasting of his wealth," newlineSymbol "Together with his neighbor," newlineSymbol "Guileless and sincere."
			lpStrIndexTai+8,		"The sovereign I" newlineSymbol "Gives his daughter in marriage." newlineSymbol "This brings blessing" newlineSymbol "And supreme good fortune."
			lpStrIndexTai+9,		"The wall falls back into the moat." newlineSymbol "Use no army now." newlineSymbol "Make your commands known within your own town." newlineSymbol "Perseverance brings humiliation."
			
			lpStrIndexTaChuang,			"Ta Chuang"
			lpStrIndexTaChuang+1,	"The Power of the Great"
			lpStrIndexTaChuang+2,	"THE POWER OF THE GREAT. Perseverance furthers."
			lpStrIndexTaChuang+3,	"Thunder in heaven above:" newlineSymbol "The image of THE POWER OF THE GREAT." newlineSymbol "Thus the superior man does not tread upon paths" newlineSymbol "That do not accord with established order."
			lpStrIndexTaChuang+4,	"Power in the toes." newlineSymbol "Continuing brings misfortune." newlineSymbol "This is certainly true."
			lpStrIndexTaChuang+5,	"Perseverance brings good fortune."
			lpStrIndexTaChuang+6,	"The inferior man works through power." newlineSymbol "The superior man does not act thus." newlineSymbol "To continue is dangerous." newlineSymbol "A goat butts against a hedge" newlineSymbol "And gets its horns entangled."
			lpStrIndexTaChuang+7,	"Perseverance brings good fortune." newlineSymbol "Remorse disappears." newlineSymbol "The hedge opens; there is no entanglement." newlineSymbol "Power depends upon the axle of a big cart."
			lpStrIndexTaChuang+8,	"Loses the goat with ease." newlineSymbol "No remorse."
			lpStrIndexTaChuang+9,	"A goat butts against a hedge." newlineSymbol "It cannot go backward, it cannot go forward." newlineSymbol "Nothing serves to further." newlineSymbol "If one notes the difficulty, this brings good fortune."
			
			lpStrIndexKuai,				"Kuai"
			lpStrIndexKuai+1,		"Break-through (Resoluteness)"
			lpStrIndexKuai+2,		"BREAK-THROUGH. One must resolutely make the matter known" newlineSymbol "At the court of the king." newlineSymbol "It must be announced truthfully. Danger." newlineSymbol "It is necessary to notify one" apostropheSymbol "s own city." newlineSymbol "It does not further one to resort to arms." newlineSymbol "It furthers one to undertake something."
			lpStrIndexKuai+3,		"The lake has risen up to heaven:" newlineSymbol "The image of BREAK-THROUGH." newlineSymbol "Thus the superior man" newlineSymbol "Dispenses riches downward" newlineSymbol "And refrains from resting on his virtue."
			lpStrIndexKuai+4,		"Mighty in the forward-striding toes." newlineSymbol "When one goes and is not equal to the task," newlineSymbol "One makes a mistake."
			lpStrIndexKuai+5,		"A cry of alarm. Arms at evening and at night." newlineSymbol "Fear nothing."
			lpStrIndexKuai+6,		"To be powerful in the cheekbones" newlineSymbol "Brings misfortune." newlineSymbol "The superior man is firmly resolved." newlineSymbol "He walks alone and is caught in the rain." newlineSymbol "He is bespattered," newlineSymbol "And people murmur against him." newlineSymbol "No blame."
			lpStrIndexKuai+7,		"There is no skin on his thighs," newlineSymbol "And walking comes hard." newlineSymbol "If a man were to let himself be led like a sheep," newlineSymbol "Remorse would disappear." newlineSymbol "But if these words are heard," newlineSymbol "They will not be believed."
			lpStrIndexKuai+8,		"In dealing with weeds," newlineSymbol "Firm resolution is necessary." newlineSymbol "Walking in the middle" newlineSymbol "Remains free of blame."
			lpStrIndexKuai+9,		"No cry." newlineSymbol "In the end misfortune comes."
			
			lpStrIndexHsue,				"Hs" uumlautSymbol
			lpStrIndexHsue+1,		"Waiting (Nourishment)"
			lpStrIndexHsue+2,		"WAITING. If you are sincere," newlineSymbol "You have light and success." newlineSymbol "Perseverance brings good fortune." newlineSymbol "It furthers one to cross the great water."
			lpStrIndexHsue+3,		"Clouds rise up to heaven:" newlineSymbol "The image of WAITING." newlineSymbol "Thus the superior man eats and drinks," newlineSymbol "Is joyous and of good cheer."
			lpStrIndexHsue+4,		"Waiting in the meadow." newlineSymbol "It furthers one to abide in what endures."
			lpStrIndexHsue+5,		"Waiting on the sand." newlineSymbol "There is some gossip." newlineSymbol "The end brings good fortune."
			lpStrIndexHsue+6,		"Waiting in the mud" newlineSymbol "Brings about the arrival of the enemy."
			lpStrIndexHsue+7,		"Waiting in blood." newlineSymbol "Get out of the pit."
			lpStrIndexHsue+8,		"Waiting at meat and drink." newlineSymbol "Perseverance brings good fortune."
			lpStrIndexHsue+9,		"One falls into the pit." newlineSymbol "Three uninvited guests arrive." newlineSymbol "Honor them, and in the end there will be good fortune."
			
			lpStrIndexPi3,				"Pi"
			lpStrIndexPi3+1,		"Holding Together (Union)"
			lpStrIndexPi3+2,		"HOLDING TOGETHER brings good fortune." newlineSymbol "Inquire of the oracle once again" newlineSymbol "Whether you possess sublimity, constancy, and perseverance;" newlineSymbol "Then there is no blame." newlineSymbol "Those who are uncertain gradually join." newlineSymbol "Whoever comes too late" newlineSymbol "Meets with misfortune."
			lpStrIndexPi3+3,		"On the earth is water:" newlineSymbol "The image of HOLDING TOGETHER." newlineSymbol "Thus the kings of antiquity" newlineSymbol "Bestowed the different states as fiefs" newlineSymbol "And cultivated friendly relations" newlineSymbol "With the feudal lords."
			lpStrIndexPi3+4,		"Hold to him in truth and loyalty;" newlineSymbol "This is without blame." newlineSymbol "Truth, like a full earthen bowl:" newlineSymbol "Thus in the end" newlineSymbol "Good fortune comes from without."
			lpStrIndexPi3+5,		"Hold to him inwardly." newlineSymbol "Perseverance brings good fortune."
			lpStrIndexPi3+6,		"You hold together with the wrong people."
			lpStrIndexPi3+7,		"Hold to him outwardly also." newlineSymbol "Perseverance brings good fortune."
			lpStrIndexPi3+8,		"Manifestation of holding together." newlineSymbol "In the hunt the king uses beaters on three sides only" newlineSymbol "And foregoes the game that runs off in front." newlineSymbol "The citizens need no warning." newlineSymbol "Good fortune."
			lpStrIndexPi3+9,		"He finds no head for holding together." newlineSymbol "Misfortune."				

			/////////////////////////////////////////////////////////////////
			//
			// The house of Tui
			//
		#define lpStrIndexTui		(lpStrIndexKun + 80)						// 606
		#define	lpStrIndexKun2		(lpStrIndexTui + 10)
		#define lpStrIndexTsui		(lpStrIndexTui + 20)
		#define lpStrIndexHsien		(lpStrIndexTui + 30)
		#define lpStrIndexChien3	(lpStrIndexTui + 40)
		#define lpStrIndexChien4	(lpStrIndexTui + 50)
		#define lpStrIndexHsiaoKo	(lpStrIndexTui + 60)
		#define lpStrIndexKueiMei	(lpStrIndexTui + 70)

			lpStrIndexTui,				"Tui"
			lpStrIndexTui+1,		"The Joyous, Lake"
			lpStrIndexTui+2,		"THE JOYOUS. Success." newlineSymbol "Perseverance is favorable."
			lpStrIndexTui+3,		"Lakes resting one on the other:" newlineSymbol "The image of THE JOYOUS." newlineSymbol "Thus the superior man joins with his friends" newlineSymbol "For discussion and practice."
			lpStrIndexTui+4,		"Contented joyousness. Good fortune."
			lpStrIndexTui+5,		"Sincere joyousness. Good fortune." newlineSymbol "Remorse disappears."
			lpStrIndexTui+6,		"Coming joyousness. Misfortune."
			lpStrIndexTui+7,		"Joyousness that is weighed is not at peace." newlineSymbol "After ridding himself of mistakes a man has joy."
			lpStrIndexTui+8,		"Sincerity toward disintegrating influences is dangerous."
			lpStrIndexTui+9,		"Seductive joyousness."

			lpStrIndexKun2,				"K" apostropheSymbol "un"
			lpStrIndexKun2+1,		"Oppression (Exhaustion)"
			lpStrIndexKun2+2,		"OPPRESSION. Success. Perseverance." newlineSymbol "The great man brings about good fortune." newlineSymbol "No blame." newlineSymbol "When one has something to say," newlineSymbol "It is not believed."
			lpStrIndexKun2+3,		"There is no water in the lake:" newlineSymbol "The image of EXHAUSTION." newlineSymbol "Thus the superior man stakes his life" newlineSymbol "On following his will."
			lpStrIndexKun2+4,		"One sits oppressed under a bare tree" newlineSymbol "And strays into a gloomy valley." newlineSymbol "For three years one sees nothing."
			lpStrIndexKun2+5,		"One is oppressed while at meat and drink." newlineSymbol "The man with the scarlet knee bands is just coming." newlineSymbol "It furthers one to offer sacrifice." newlineSymbol "To set forth brings misfortune." newlineSymbol "No blame."
			lpStrIndexKun2+6,		"A man permits himself to be oppressed by stone," newlineSymbol "And leans on thorns and thistles." newlineSymbol "He enters his house and does not see his wife." newlineSymbol "Misfortune."
			lpStrIndexKun2+7,		"He comes very quietly, oppressed in a golden carriage." newlineSymbol "Humiliation, but the end is reached."
			lpStrIndexKun2+8,		"His nose and feet are cut off." newlineSymbol "Oppression at the hands of the man with the purple knee bands." newlineSymbol "Joy comes softly." newlineSymbol "It furthers one to make offerings and libations."
			lpStrIndexKun2+9,		"He is oppressed by creeping vines." newlineSymbol "He moves uncertainly and says, ""Movement brings remorse."",If one feels remorse over this and makes a start," newlineSymbol "Good fortune comes."

			lpStrIndexTsui,				"Ts" apostropheSymbol "ui"
			lpStrIndexTsui+1,		"Gathering Together (Massing)"
			lpStrIndexTsui+2,		"GATHERING TOGETHER. Success." newlineSymbol "The king approaches his temple." newlineSymbol "It furthers one to see the great man." newlineSymbol "This brings success. Perseverance furthers." newlineSymbol "To bring great offerings creates good fortune." newlineSymbol "It furthers one to undertake something."
			lpStrIndexTsui+3,		"Over the earth, the lake:" newlineSymbol "The image of GATHERING TOGETHER." newlineSymbol "Thus the superior man renews his weapons" newlineSymbol "In order to meet the unforeseen."
			lpStrIndexTsui+4,		"If you are sincere, but not to the end," newlineSymbol "There will sometimes be confusion, sometimes gathering together." newlineSymbol "If you call out," newlineSymbol "Then after one grasp of the hand you can laugh again." newlineSymbol "Regret not. Going is without blame."
			lpStrIndexTsui+5,		"Letting oneself be drawn" newlineSymbol "Brings good fortune and remains blameless." newlineSymbol "If one is sincere," newlineSymbol "It furthers one to bring even a small offering."
			lpStrIndexTsui+6,		"Gathering together amid sighs." newlineSymbol "Nothing that would further." newlineSymbol "Going is without blame." newlineSymbol "Slight humiliation."
			lpStrIndexTsui+7,		"Great good fortune. No blame."
			lpStrIndexTsui+8,		"If in gathering together one has position," newlineSymbol "This brings no blame." newlineSymbol "If there are some who are not yet sincerely in the work," newlineSymbol "Sublime and enduring perseverance is needed." newlineSymbol "The remorse disappears."
			lpStrIndexTsui+9,		"Lamenting and sighing, floods of tears." newlineSymbol "No blame."

			lpStrIndexHsien,			"Hsien"
			lpStrIndexHsien+1,		"Influence (Wooing)"
			lpStrIndexHsien+2,		"Influence. Success." newlineSymbol "Perseverance furthers." newlineSymbol "To take a maiden to wife brings good fortune."
			lpStrIndexHsien+3,		"A lake on the mountain." newlineSymbol "The image of influence." newlineSymbol "Thus the superior man encourages people to approach him" newlineSymbol "By his readiness to receive them."
			lpStrIndexHsien+4,		"The influence shows itself in the big toe."
			lpStrIndexHsien+5,		"The influence shows itself in the calves of the legs." newlineSymbol "Misfortune." newlineSymbol "Tarrying brings good fortune."
			lpStrIndexHsien+6,		"The influence shows itself in the thighs." newlineSymbol "Holds to that which follows it." newlineSymbol "To continue is humiliating."
			lpStrIndexHsien+7,		"Perseverance brings good fortune." newlineSymbol "Remorse disappears." newlineSymbol "If a man is agitated in mind," newlineSymbol "And his thoughts go hither and thither," newlineSymbol "Only those friends " newlineSymbol "On whom he fixes his conscious thoughts" newlineSymbol "Will follow."
			lpStrIndexHsien+8,		"The influence shows itself in the back of the neck." newlineSymbol "No remorse."
			lpStrIndexHsien+9,		"The influence shows itself in the jaws, cheeks" newlineSymbol " and tongue."

			lpStrIndexChien3,			"Chien"
			lpStrIndexChien3+1,		"Obstruction"
			lpStrIndexChien3+2,		"OBSTRUCTION. The southwest furthers." newlineSymbol "The northeast does not further." newlineSymbol "It furthers one to see the great man." newlineSymbol "Perseverance brings good fortune."
			lpStrIndexChien3+3,		"Water on the mountain:" newlineSymbol "The image of OBSTRUCTION." newlineSymbol "Thus the superior man turns his attention to himself" newlineSymbol "And molds his character."
			lpStrIndexChien3+4,		"Going leads to obstructions," newlineSymbol "Coming meets with praise."
			lpStrIndexChien3+5,		"The king" apostropheSymbol "s servant is beset by obstruction upon obstruction," newlineSymbol "But it is not his own fault."
			lpStrIndexChien3+6,		"Going leads to obstructions;" newlineSymbol "Hence he comes back."
			lpStrIndexChien3+7,		"Going leads to obstructions," newlineSymbol "Coming leads to union."
			lpStrIndexChien3+8,		"In the midst of the greatest obstructions," newlineSymbol "Friends come."
			lpStrIndexChien3+9,		"Going leads to obstructions," newlineSymbol "Coming leads to great good fortune." newlineSymbol "It furthers one to see the great man."

			lpStrIndexChien4,			"Ch" apostropheSymbol "ien"
			lpStrIndexChien4+1,		"Modesty"
			lpStrIndexChien4+2,		"MODESTY creates success." newlineSymbol "The superior man carries things through."
			lpStrIndexChien4+3,		"Within the earth, a mountain:" newlineSymbol "The image of MODESTY." newlineSymbol "Thus the superior man reduces that which is too much," newlineSymbol "And augments that which is too little." newlineSymbol "He weighs things and makes them equal."
			lpStrIndexChien4+4,		"A superior man modest about his modesty" newlineSymbol "May cross the great water." newlineSymbol "Good fortune."
			lpStrIndexChien4+5,		"Modesty that comes to expression." newlineSymbol "Perseverance brings good fortune."
			lpStrIndexChien4+6,		"A superior man of modesty and merit" newlineSymbol "Carries things to conclusion." newlineSymbol "Good fortune."
			lpStrIndexChien4+7,		"Nothing that would not further modesty" newlineSymbol "In movement."
			lpStrIndexChien4+8,		"No boasting of wealth before one" apostropheSymbol "s neighbor." newlineSymbol "It is favorable to attack with force." newlineSymbol "Nothing that would not further."
			lpStrIndexChien4+9,		"Modesty that comes to expression." newlineSymbol "It is favorable to set armies marching" newlineSymbol "To chastise one" apostropheSymbol "s own city and one" apostropheSymbol "s own country."

			lpStrIndexHsiaoKo,		"Hsiao Ko"
			lpStrIndexHsiaoKo+1,	"Preponderance of the Small"
			lpStrIndexHsiaoKo+2,	"PREPONDERANCE OF THE SMALL. Success." newlineSymbol "Perseverance furthers." newlineSymbol "Small things may be done; great things should not be done." newlineSymbol "The flying bird brings the message:" newlineSymbol "It is not well to strive upward," newlineSymbol "It is well to remain below." newlineSymbol "Great good fortune."
			lpStrIndexHsiaoKo+3,	"Thunder on the mountain:" newlineSymbol "The image of PREPONDERANCE OF THE SMALL." newlineSymbol "Thus in his conduct the superior man gives preponderance to reverence." newlineSymbol "In bereavement he gives preponderance to grief." newlineSymbol "In his expenditures he gives preponderance to thrift."
			lpStrIndexHsiaoKo+4,	"The bird meets with misfortune through flying."
			lpStrIndexHsiaoKo+5,	"She passes by her ancestor" newlineSymbol "And meets her ancestress." newlineSymbol "He does not reach his prince" newlineSymbol "And meets the official." newlineSymbol "No blame."
			lpStrIndexHsiaoKo+6,	"If one is not extremely careful," newlineSymbol "Somebody may come up from behind and strike him." newlineSymbol "Misfortune."
			lpStrIndexHsiaoKo+7,	"No blame. He meets him without passing by." newlineSymbol "Going brings danger. One must be on guard." newlineSymbol "Do not act. Be constantly persevering."
			lpStrIndexHsiaoKo+8,	"Dense clouds," newlineSymbol "No rain from our western territory." newlineSymbol "The prince shoots and hits him who is in the cave."
			lpStrIndexHsiaoKo+9,	"He passes him by, not meeting him." newlineSymbol "The flying bird leaves him." newlineSymbol "Misfortune." newlineSymbol "This means bad luck and injury."

			lpStrIndexKueiMei,		"Kuei Mei"
			lpStrIndexKueiMei+1,	"The Marrying Maiden"
			lpStrIndexKueiMei+2,	"THE MARRYING MAIDEN." newlineSymbol "Undertakings bring misfortune." newlineSymbol "Nothing that would further."
			lpStrIndexKueiMei+3,	"Thunder over the lake:" newlineSymbol "The image of THE MARRYING MAIDEN." newlineSymbol "Thus the superior man" newlineSymbol "Understands the transitory" newlineSymbol "In the light of the eternity of the end."
			lpStrIndexKueiMei+4,	"The marrying maiden as a concubine." newlineSymbol "A lame man who is able to tread." newlineSymbol "Undertakings bring good fortune."
			lpStrIndexKueiMei+5,	"A one-eyed man who is able to see." newlineSymbol "The perseverance of a solitary man furthers."
			lpStrIndexKueiMei+6,	"The marrying maiden as a slave." newlineSymbol "She marries as a concubine."
			lpStrIndexKueiMei+7,	"The marrying maiden draws out the allotted time." newlineSymbol "A late marriage comes in due course."
			lpStrIndexKueiMei+8,	"The sovereign I gave his daughter in marriage." newlineSymbol "The embroidered garments of the princess" newlineSymbol "Were not as gorgeous" newlineSymbol "As those of the servingmaid." newlineSymbol "The moon that is nearly full brings good fortune."
			lpStrIndexKueiMei+9,	"The woman holds the basket, but there are no fruits in it." newlineSymbol "The man stabs the sheep, but no blood flows." newlineSymbol "Nothing that acts to further."				
		END

#else								// Must be Mac OS Resource Compiler

	resource 'STR#' (LPobjID, LPobjName) {
		{	/* array StringArray */
			LPStdStrings,					// Standard Litter Strings
			
			// Assist strings
			LPAssistIn1, LPAssistIn2,		// Inlets
			LPAssistOut1					// Outlets
		}
	};

	resource 'STR#' (17516, "lp.i Common") {
		{	/* array StringArray */
			"above",					/* For trigrams */
			"below",
			"upper middle",
			"lower middle",
			"Governing Rulers:",		/* Major constituents of oracle */
			"Constituting Rulers:",
			"The Judgement",
			"The Image",
			"The Lines",
			"  ",						/* Indicates no rulers; specific ruler symbols
											generated on the fly */
			"Six",						/* Major constituents of individual lines */
			"Nine",
			"at the beginning",
			"in the second place",
			"in the third place",
			"in the fourth place",
			"in the fifth place",
			"at the top",
			"means:",
			"The Future"
		}
	};

	resource 'STR#' (17517, "lp.i Trigrams") {
		{	/* array StringArray: 16 elements */
			"Ch’ien",
			"The Creative Principle; The Heaven",
			"Kun",
			"The Passive Principle; The Earth",
			"Chen",
			"Movement; The Thunderclap",
			"K’an",
			"The Abyss; Water, Danger",
			"Ken",
			"Inaction; The Mountain",
			"Sun",
			"The Gentle One; Wind, Wood",
			"Li",
			"Clinging; Fire",
			"Tui",
			"Joy; The Lake"
		}
	};

	resource 'STR#' (17518, "lp.i Ch’ien") {
		{	/* array StringArray */
			"Ch’ien",
			"The Creative",
			"THE CREATIVE works sublime success,\n"
				"Furthering through perseverance.",
			"The movement of heaven is full of power.\n"
				"Thus the superior man makes himself strong and untiring.",
			"",
			"Hidden dragon. Do not act.",
			"Dragon appearing in the field.\n"
				"It furthers one to see the great man.",
			"All day long the superior man is creatively active.\n"
				"At nightfall his mind is still beset with cares.\n"
				"Danger. No blame.",
			"Wavering flight over the depths.\n"
				"No blame.",
			"Flying dragon in the heavens.\n"
				"It furthers one to see the great man.",
			"Arrogant dragon will have cause to repent.\n"
				"When all the lines are nines, it means:\n"
				"There appears a flight of dragons without heads.\n"
				"Good fortune.",
				
			"Kou",
			"Coming to Meet",
			"COMING TO MEET. The maiden is powerful.\n"
				"One should not marry such a maiden.",
			"Under heaven, wind:\n"
				"The image of COMING TO MEET.\n"
				"Thus does the prince act when disseminating his commands\n"
				"And proclaiming them to the four quarters of heaven.",
			"",
			"It must be checked with a brake of bronze.\n"
				"Perseverance brings good fortune.\n"
				"If one lets it take its course, one experiences misfortune.\n"
				"Even a lean pig has it in him to rage around.",
			"There is a fish in the tank.  No blame.\n"
				"Does not further guests.",
			"There is no skin on his thighs,\n"
				"And walking comes hard.\n"
				"If one is mindful of the danger,\n"
				"No great mistake is made.\n",
			"No fish in the tank.\n"
				"This leads to misfortune.",
			"A melon covered with willow leaves.\n"
				"Hidden lines.\n"
				"Then it drops down to one from heaven.",
			"He comes to meet with his horns.\n"
				"Humiliation. No blame.",
				
			"Tun",
			"Retreat",
			"RETREAT. Success.\n"
				"In what is small, perseverance furthers.",
			"Mountain under heaven: the image of RETREAT.\n"
				"Thus the superior man keeps the inferior man at a distance,\n"
				"Not angrily but with reserve.",
			"",
			"At the tail in retreat. This is dangerous.\n"
				"One must not wish to undertake anything.",
			"He holds him fast with yellow oxhide.\n"
				"No one can tear him loose.",
			"A halted retreat\n"
				"Is nerve-wracking and dangerous.\n"
				"To retain people as men- and maidservants\n"
				"Brings good fortune.",
			"Voluntary retreat brings good fortune to the superior man\n"
				"And downfall to the inferior man.",
			"Friendly retreat. Perseverance brings good fortune.",
			"Cheerful retreat. Everything serves to further.",
			
			"P’i",
			"Standstill (Stagnation)",
			"STANDSTILL. Evil people do not further\n"
				"The perseverance of the superior man.\n"
				"The great departs; the small approaches.",
			"Heaven and earth do not unite:\n"
				"The image of STANDSTILL.\n"
				"Thus the superior man falls back upon his inner worth\n"
				"In order to escape the difficulties.\n"
				"He does not permit himself to be honored with revenue.",
			"",
			"When ribbon grass is pulled up, the sod comes with it.\n"
				"Each according to his kind.\n"
				"Perseverance brings good fortune and success.",
			"They bear and endure;\n"
				"This means good fortune for inferior people.\n"
				"The standstill serves to help the great man to attain success.",
			"They bear shame.",
			"He who acts at the command of the highest\n"
				"Remains without blame.\n"
				"Those of like mind partake of the blessing.",
			"Standstill is giving way.\n"
				"Good fortune for the great man.\n"
				"\"What if it should fail, what if it should fail?\","
				"In this way, he ties it to a cluster of mulberry roots.",
			"The standstill comes to an end.\n"
				"First standstill, then good fortune.",
				
			"Kuan",
			"Contemplation (View)",
			"CONTEMPLATION. The ablution has been made,\n"
				"But not yet the offering.\n"
				"Full of trust, they look up to him.",
			"The wind blows over the earth:\n"
				"The image of CONTEMPLATION.\n"
				"Thus the kings of old visited the regions of the world,\n"
				"Contemplated the people,\n"
				"And gave them instruction.",
			"",
			"Boy-like contemplation.\n"
				"For an inferior man, no blame.\n"
				"For a superior man, humiliation.",
			"Contemplation through the crack of the door.\n"
				"Furthering for the perseverance of a woman.",
			"Contemplation of my life\n"
				"Decides the choice\n"
				"Between advance and retreat.",
			"Contemplation of the light of the kingdom.\n"
				"It furthers one to exert influence as the guest of a king.",
			"Contemplation of my life.\n"
				"The superior man is without blame.",
			"Contemplation of his life.\n"
				"The superior man is without blame.",
				
			"Po",
			"Splitting Apart",
			"SPLITTING APART. It does not further one\n"
				"To go anywhere.",
			"The mountain rests on the earth:\n"
				"The image of SPLITTING APART.\n"
				"Thus those above can ensure their position\n"
				"Only by giving generously to those below.",
			"",
			"The leg of the bed is split.\n"
				"Those who persevere are destroyed.",
			"The bed is split at the edge.\n"
				"Those who persevere are destroyed.\n"
				"Misfortune.",
			"He splits with them. No blame.",
			"The bed is split up to the skin.\n"
				"Misfortune.",
			"A shoal of fishes. Favor comes through the court ladies.\n"
				"Everything acts to further.",
			"There is a large fruit still un-eaten.\n"
				"The superior man receives a carriage.\n"
				"The house of the inferior man is split apart.",
				
			"Chin",
			"Progress",
			"PROGRESS. The powerful prince\n"
				"Is honored with horses in large numbers.\n"
				"In a single day he is granted audience three times.",
			"The sun rises over the earth:\n"
				"The image of PROGRESS.\n"
				"Thus the superior man himself\n"
				"Brightens his bright virtue.",
			"",
			"Progressing, but turned back.\n"
				"Perseverance brings good fortune.\n"
				"If one meets with no confidence, one should remain calm.\n"
				"No mistake.",
			"Progressing, but in sorrow.\n"
				"Perseverance brings good fortune.\n"
				"Then one obtains great happiness from one's ancestress.",
			"All are in accord. Remorse disappears.",
			"Progress like a hamster.\n"
				"Perseverance brings danger.",
			"Remorse disappears.\n"
				"Take not gain and loss to heart.\n"
				"Undertakings bring good fortune.\n"
				"Everything serves to further.",
			"Making progress with the horns is permissible\n"
				"One for the purpose of punishing one's own city.\n"
				"To be conscious of danger brings good fortune.\n"
				"No blame.\n"
				"Perseverance brings humiliation.",
				
			"Ta Yu",
			"Possession in Great Measure",
			"POSSESSION IN GREAT MEASURE.\n"
				"Supreme success.",
			"Fire in heaven above:\n"
				"The image of POSSESSION IN GREAT MEASURE.\n"
				"Thus the superior man curbs evil and furthers good,\n"
				"And thereby orders the benevolent will of heaven.",
			"",
			"No relationship with what is harmful;\n"
				"There is no blame in this.\n"
				"If one remains conscious of difficulty,\n"
				"One remains without blame.",
			"A big wagon for loading.\n"
				"One may undertake something.\n"
				"No blame.",
			"A prince offers it to the Son of Heaven.\n"
				"A petty man cannot do this.",
			"He makes a difference\n"
				"Between himself and his neighbor.\n"
				"No blame.",
			"He whose truth is accessible, yet dignified,\n"
				"Has good fortune.",
			"He is blessed by heaven.\n"
				"Good fortune.\n"
				"Nothing that does not further."
		}
	};

	resource 'STR#' (17519, "lp.i K’an") {
		{	/* array StringArray */
			"K’an",
			"The Abysmal (Water)",
			"The Abysmal repeated.\n"
				"If you are sincere, you have success in your heart,\n"
				"And whatever you do succeeds.",
			"Water flows on uninterruptedly and reaches its goal:\n"
				"The image of the Abysmal repeated.\n"
				"Thus the superior man walks in lasting virtue\n"
				"And carries on the business of teaching.",
			"",
			"Repetition of the Abysmal.\n"
				"In the abyss one falls into a pit.\n"
				"Misfortune.",
			"The abyss is dangerous.\n"
				"One should strive to attain small things only.\n",
			"Forward and backward, abyss on abyss.\n"
				"In danger like this, pause at first and wait,\n"
				"Otherwise you will fall into a pit in the abyss.\n"
				"Do not act in this way.",
			"A jug of wine, a bowl of rice with it;\n"
				"Earthen vessels\n"
				"Simply handed in through the window.\n"
				"There is certainly no blame in this.",
			"The abyss is not filled to overflowing,\n"
				"It is filled only to the rim.\n"
				"No blame.",
			"Bound with cords and ropes,\n"
				"Shut in between thorn-edged prison walls:\n"
				"For three years one does not find the way.\n"
				"Misfortune.",
				
			"Chieh",
			"Limitation",
			"LIMITATION. Success.\n"
				"Galling limitation must not be persevered in.",
			"Water over lake: the image of LIMITATION.\n"
				"Thus the superior man\n"
				"Creates number and measure,\n"
				"And examines the nature of virtue and correct conduct.",
			"",
			"Not going out of the door and the courtyard\n"
				"Is without blame.",
			"Not going out of the gate and the courtyard \n"
				"Brings misfortune.",
			"He who knows no limitation\n"
				"Will have cause to lament.\n"
				"No blame.",
			"Contented limitation. Success.",
			"Sweet limitation brings good fortune.\n"
				"Going brings esteem.",
			"Galling limitation.\n"
				"Perseverance brings misfortune.\n"
				"Remorse disappears.",
				
			"Chun",
			"Difficulty at the Beginning",
			"DIFFICULTY AT THE BEGINNING works supreme success,\n"
				"Furthering through perseverance.\n"
				"Nothing should be undertaken.\n"
				"It furthers one to appoint helpers.\n",
			"Clouds and thunder:\n"
				"The image of DIFFICULTY AT THE BEGINNING.\n"
				"Thus the superior man brings order out of confusion.",
			"",
			"Hesitation and hindrance.\n"
				"It furthers one to remain persevering.\n"
				"It furthers one to appoint helpers.",
			"Difficulties pile up.\n"
				"Horse and wagon part.\n"
				"He is not a robber;\n"
				"He wants to woo when the time comes.\n"
				"The maiden is chaste,\n"
				"She does not pledge herself.\n"
				"Ten years -- then she pledges herself.",
			"Whoever hunts deer without the forester\n"
				"Only loses his way in the forest.\n"
				"The superior man understands the signs of the time\n"
				"And prefers to desist.\n"
				"To go on brings humiliation.\n",
			"Horse and wagon part.\n"
				"Strive for union.\n"
				"To go brings good fortune.\n"
				"Everything acts to further.",
			"Difficulties in blessing.\n"
				"A little perseverance brings good fortune.\n"
				"Great perseverance brings misfortune.",
			"Horse and wagon part.\n"
				"Bloody tears flow.",
				
			"Chi Chi",
			"After Completion",
			"AFTER COMPLETION. Success in small matters.\n"
				"Perseverance furthers.\n"
				"At the beginning good fortune,\n"
				"At the end disorder.",
			"Water over fire: the image of the condition\n"
				"In AFTER COMPLETION.\n"
				"Thus the superior man\n"
				"Takes thought of misfortune\n"
				"And arms himself against it in advance.",
			"",
			"He brakes his wheels.\n"
				"He gets his tail in the water.\n"
				"No blame.",
			"The woman loses the curtain of her carriage.\n"
				"Do not run after it;\n"
				"One the seventh day you will get it.",
			"The Illustrious Ancestor\n"
				"Disciplines the Devil's Country.\n"
				"After three years he conquers it.\n"
				"Inferior people must not be employed.",
			"The finest clothes turn to rags.\n"
				"Be careful all day long.",
			"The neighbor in the east who slaughters an ox\n"
				"Does not attain as much real happiness\n"
				"As the neighbor in the west\n"
				"With his small offering.",
			"He gets his head in the water. Danger.",
			
			"Ko",
			"Revolution (Molting)",
			"REVOLUTION. On your own day\n"
				"You are believed.\n"
				"Supreme success,\n"
				"Furthering through perseverance.\n"
				"Remorse disappears.",
			"Fire in the lake: the image of REVOLUTION.\n"
				"Thus the superior man\n"
				"Sets the calendar in order\n"
				"And makes the seasons clear.",
			"",
			"Wrapped in the hide of a yellow cow.",
			"When one's own day comes, one may create revolution.\n"
				"Starting brings good fortune. No blame.",
			"Starting brings misfortune.\n"
				"Perseverance brings danger.\n"
				"When talk of revolution has gone the rounds three times,\n"
				"One may commit himself,\n"
				"And men will believe him.",
			"Remorse disappears. Men believe him.\n"
				"Changing the form of government brings good fortune.",
			"The great man changes like a tiger.\n"
				"Even before he questions the oracle\n"
				"He is believed.",
			"The superior man changes like a panther.\n"
				"The inferior man molts in the face.\n"
				"Starting brings misfortune.\n"
				"To remain persevering brings good fortune.",
			
			"Feng",
			"Abundance (Fullness)",
			"ABUNDANCE has success.\n"
				"The king attains abundance.\n"
				"Be not sad.\n"
				"Be like the sun at midday.",
			"Both thunder and lightning come:\n"
				"The image of ABUNDANCE.\n"
				"Thus the superior man decides lawsuits\n"
				"And carries out punishments.",
			"",
			"When a man meets his destined ruler,\n"
				"The can be together ten days,\n"
				"And it is not a mistake.\n"
				"Going meets with recognition.",
			"The curtain is of such fullness\n"
				"That the polestars can be seen at noon.\n"
				"Through going one meets with mistrust and hate.\n"
				"If one rouses him through truth,\n"
				"Good fortune comes.",
			"The underbrush is of such abundance\n"
				"That the small stars can be seen at noon.\n"
				"He breaks his right arm. No blame.",
			"The curtain is of such fullness\n"
				"That the polestars can be seen at noon.\n"
				"He meets his ruler, who is of like mind.\n"
				"Good fortune.",
			"Lines are coming.\n"
				"Blessing and fame draw near.\n"
				"Good fortune.",
			"His house is in a state of abundance.\n"
				"He screens off his family.\n"
				"He peers through the gate\n"
				"And no longer perceives anyone.\n"
				"For three years he sees nothing.\n"
				"Misfortune.",
				
			"Ming I",
			"Darkening of the Light",
			"DARKENING OF THE LIGHT. In adversity\n"
				"It furthers one to be persevering.",
			"The light has sunk into the earth:\n"
				"The image of DARKENING OF THE LIGHT.\n"
				"Thus does the superior man live with the great mass:\n"
				"He veils his light, yet still shines.",
			"",
			"Darkening of the light during flight.\n"
				"He lowers his wings.\n"
				"The superior man does not eat for three days\n"
				"On his wanderings.\n"
				"Be he has somewhere to go.\n"
				"The host has occasion to gossip about him.",
			"Darkening of the light injures him in the left thigh.\n"
				"He gives aid with the strength of a horse.\n"
				"Good fortune.",
			"Darkening of the light during the hunt in the south.\n"
				"Their great leader is captured.\n"
				"One must not expect perseverance too soon.",
			"He penetrates the left side of the belly.\n"
				"One gets at the very heart of the darkening of the light,\n"
				"And leaves gate and courtyard.",
			"Darkening of the light as with Prince Chi.\n"
				"Perseverance furthers.",
			"Not light but darkness.\n"
				"First he climbed up to heaven,\n"
				"The he plunged into the depths of the earth.",
				
			"Shih",
			"The Army",
			"THE ARMY. The army needs perseverance\n"
				"And a strong man.\n"
				"Good fortune without blame.",
			"In the middle of the earth is water:\n"
				"The image of THE ARMY.\n"
				"Thus the superior man increases his masses\n"
				"By generosity toward the people.",
			"",
			"An army must set forth in proper order.\n"
				"If the order is not good, misfortune threatens.",
			"In the midst of the army.\n"
				"Good fortune. No blame.\n"
				"The king bestows a triple decoration.\n",
			"Perchance the army carries corpses in the wagon.\n"
				"Misfortune.\n",
			"The army retreats.  No blame.",
			"There is game in the field.\n"
				"It furthers one to catch it.\n"
				"Without blame.\n"
				"Let the eldest lead the army.\n"
				"The younger transport corpses;\n"
				"Then perseverance brings misfortune.",
			"The great prince issues commands,\n"
				"Founds estates, vests families with fiefs.\n"
				"Inferior people should not be employed."
		}
	};

	resource 'STR#' (17520, "lp.i Ken") {
		{	/* array StringArray */
			"Ken",
			"Keeping Still, Mountain",
			"KEEPING STILL. Keeping his back still\n"
				"So that he no longer feels his body.\n"
				"He goes into his courtyard\n"
				"And does not see his people.\n"
				"No blame.",
			"Mountains standing close together:\n"
				"The image of KEEPING STILL.\n"
				"Thus the superior man\n"
				"Does not permit his thoughts\n"
				"To go beyond his situation.",
			"",
			"Keeping his toes still.\n"
				"No blame.\n"
				"Continued perseverance furthers.",
			"Keeping his calves still.\n"
				"He cannot rescue him whom he follows.\n"
				"His heart is not glad.",
			"Keeping his hips still.\n"
				"Making his sacrum stiff.\n"
				"Dangerous.  The heart suffocates.",
			"Keeping his trunk still.\n"
				"No blame.",
			"Keeping his jaws still.\n"
				"The words have order.\n"
				"Remorse disappears.",
			"Noble-hearted keeping still.\n"
				"Good fortune.",
				
			"Pi",
			"Graces",
			"GRACE has success.\n"
				"In small matters\n"
				"It is favorable to undertake something.",
			"Fire at the foot of the mountain:\n"
				"The image of GRACE.\n"
				"Thus does the superior man proceed\n"
				"When clearing up current affairs.\n"
				"But he dare not decide controversial issues in this way.",
			"",
			"He lends grace to his toes, leaves the carriage\n and walks.",
			"Lends grace to the beard on his chin.",
			"Graceful and moist.\n"
				"Constant perseverance brings good fortune.",
			"Grace or simplicity?\n"
				"A white horse comes as if on wings.\n"
				"He is not a robber,\n"
				"He will woo at the right time.",
			"Grace in hills and gardens.\n"
				"The roll of silk is meager and small.\n"
				"Humiliation, but in the end good fortune.",
			"Simple grace. No blame.",
			
			"Ta Ch’u",
			"The Taming Power of the Great",
			"THE TAMING POWER OF THE GREAT.\n"
				"Perseverance furthers.\n"
				"Not eating at home brings good fortune.\n"
				"It furthers one to cross the great water.",
			"Heaven within the mountain:\n"
				"The image of THE TAMING POWER OF THE GREAT.\n"
				"Thus the superior man acquaints himself with many sayings of antiquity\n"
				"And many deeds of the past,\n"
				"In order to strengthen his character thereby.",
			"",
			"Danger is at hand. It furthers one to desist.",
			"The axletrees are taken from the wagon.",
			"A good horse that follows others.\n"
				"Awareness of danger,\n"
				"With perseverance, furthers.\n"
				"Practice chariot driving and armed defense daily.\n"
				"It furthers one to have somewhere to go.",
			"The headboard of a young bull.\n"
				"Great good fortune.",
			"The tusk of a gelded boar.\n"
				"Good fortune.",
			"One attains the way of heaven. Success.",
			
			"Sun",
			"Decrease",
			"DECREASE combined with sincerity\n"
				"Brings about supreme good fortune\n"
				"Without blame.\n"
				"One may be persevering in this.\n"
				"It furthers one to undertake something.\n"
				"How is this to be carried out?\n"
				"One may use two small bowls for the sacrifice.",
			"At the foot of the mountain, the lake:\n"
				"The image of DECREASE.\n"
				"Thus the superior man controls his anger\n"
				"And restricts his instincts.",
			"",
			"Going quickly when one's tasks are finished\n"
				"Is without blame.\n"
				"But one must reflect on how much one may decrease others.",
			"Perseverance furthers.\n"
				"To undertake something brings misfortune.\n"
				"Without decreasing oneself,\n"
				"One is able to bring increase to others.",
			"When three people journey together,\n"
				"Their number decreases by one.\n"
				"When one man journeys alone,\n"
				"He finds a companion.",
			"If a man decreases his faults,\n"
				"It makes the other hasten to come and rejoice.\n"
				"No blame.",
			"Someone does indeed increase him.\n"
				"Ten pairs of tortoises cannot oppose it.\n"
				"Supreme good fortune.",
			"If one is increased without depriving others,\n"
				"There is no blame.\n"
				"Perseverance brings good fortune.\n"
				"It furthers one to undertake something.\n"
				"One obtains servants\n"
				"But no longer has a separate home.",
			
			"K’uei",
			"Opposition",
			"OPPOSITION. In small matters, good fortune.",
			"Above, fire; below, the lake:\n"
				"The image of OPPOSITION.\n"
				"Thus amid all fellowship\n"
			"The superior man retains his individuality.",
			"He is not a robber; he will woo at the right time.\n"
				"As one goes, rain falls; then good fortune comes.",
			"Remorse disappears.\n"
				"If you lose your horse, do not run after it;\n"
				"It will come back of its own accord.\n"
				"When you see evil people,\n"
				"Guard yourself against mistakes.",
			"One meets his lord in a narrow street.\n"
				"No blame.",
			"One sees the wagon dragged back,\n"
				"The oxen halted,\n"
				"A man's hair and nose cut off."
				"Not a good beginning, but a good end.",
			"Isolated through opposition,\n"
				"One meets a like-minded man\n"
				"With whom one can associate in good faith.\n"
				"Despite the danger, no blame.",
			"Remorse disappears.\n"
				"The companion bites his way through the wrappings.\n"
				"If one goes to him,\n"
				"How could it be a mistake.",
			"Isolated through opposition,\n"
				"One sees one's companion as a pig covered with dirt,\n"
				"As a wagon full of devils.\n"
				"First one draws a bow against him,\n"
				"The one lays the bow aside.+",
				
			"Lü",
			"Treading (Conduct)",
			"TREADING. Treading upon the tail of the tiger.\n"
				"It does not bite the man. Success.",
			"Heaven above, the lake below:\n"
				"The image of TREADING.\n"
				"Thus the superior man discriminates between high and low,\n"
				"And thereby fortifies the thinking of the people.",
			"",
			"Simple conduct. Progress without blame.",
			"Treading a smooth, level course.\n"
				"The perseverance of a dark man\n"
				"Brings good fortune.",
			"A one-eyed man is able to see,\n"
				"A lame man is able to tread.\n"
				"He treads on the tail of the tiger.\n"
				"The tiger bites the man.\n"
				"Misfortune.\n"
				"Thus does a warrior act on behalf of his great prince.",
			"He treads on the tail of the tiger.\n"
				"Caution and circumspection\n"
				"Lead ultimately to good fortune.",
			"Resolute conduct.\n"
				"Perseverance with awareness of danger.",
			"Look to your conduct and weigh the favorable signs.\n"
				"When everything is fulfilled, supreme good fortune comes.",
				
			"Chung Fu",
			"Inner Truth",
			"INNER TRUTH. Pigs and fishes.\n"
				"Good fortune.\n"
				"It furthers one to cross the great water.\n"
				"Perseverance furthers.",
			"Wind over lake: the image of INNER TRUTH.\n"
				"Thus the superior man discusses criminal cases\n"
				"In order to delay executions.",
			"",
			"Being prepared brings good fortune.\n"
				"If there are secret designs, it is disquieting.",
			"A crane calling in the shade.\n"
				"Its young answers it.\n"
				"I have a good goblet.\n"
				"I will share it with you.",
			"He finds a comrade.\n"
				"Now he beats the drum, now he stops.\n"
				"Now he sobs, now he sings.",
			"The moon nearly at the full.\n"
				"The team horse goes astray.\n"
				"No blame.",
			"He possess truth, which links together.\n"
				"No blame.",
			"Cockcrow penetrating to heaven.\n"
				"Perseverance brings misfortune.",
				
			"Chien",
			"Development (Gradual Progress)",
			"DEVELOPMENT. The maiden\n"
				"Is given in marriage.\n"
				"Good fortune.\n"
				"Perseverance furthers.",
			"On the mountain, a tree:\n"
				"The image of DEVELOPMENT.\n"
				"Thus the superior man abides in dignity and virtue,\n"
				"In order to improve the mores.",
			"",
			"The wild goose gradually draws near the shore.\n"
				"The young son is in danger.\n"
				"There is talk. No blame.",
			"The wild goose draws gradually near the cliff.\n"
				"Eating and drinking in peace and accord.\n"
				"Good fortune.",
			"The wild goose gradually draws near the plateau.\n"
				"The man goes forth and does not return.\n"
				"The woman carries a child but does not bring it forth.\n"
				"Misfortune.\n"
				"It furthers one to fight off robbers.",
			"The wild goose gradually draws near the tree.\n"
				"Perhaps it will find a flat branch. No blame.",
			"The wild goose draws near the summit.\n"
				"For three years the woman has no child.\n"
				"In the end nothing can hinder her.\n"
				"Good fortune.",
			"The wild goose gradually draws near the cloud heights.\n"
				"Its feathers can be used for the sacred dance.\n"
				"Good fortune."
		}
	};

	resource 'STR#' (17521, "lp.i Chen") {
		{	/* array StringArray */
			"Chen",
			"The Arousing (Shock, Thunder)",
			"SHOCK brings success.\n"
				"Shock comes -- oh, oh!\n"
				"Laughing words -- ha, ha!\n"
				"The shock terrifies for a hundred miles,\n"
				"And he does not let fall the sacrificial spoon and chalice.",
			"Thunder repeated: the image of SHOCK. \n"
				"Thus in fear and trembling\n"
				"The superior man sets his life in order\n"
				"And examines himself.",
			"",
			"Shock comes -- oh, oh!\n"
				"Then follow laughing words -- ha, ha!\n"
				"Good fortune.",
			"Shock comes from bringing danger.\n"
				"A hundred thousand times\n"
				"You lose your treasures\n"
				"And must climb the nine hills.",
			"Shock comes and makes one distraught.\n"
				"If shock spurs to action\n"
				"One remains free of misfortune.",
			"Shock is mired.",
			"Shock goes hither and thither.\n"
				"Danger.\n"
				"However, nothing at all is lost.\n"
				"Yet there are things to be done.",
			"Shock brings ruin and terrified gazing around.\n"
				"Going ahead brings misfortune.\n"
				"If it has not yet touched one's own body\n"
				"But has reached one's neighbor first,\n"
				"There is no blame.\n"
				"One's comrades have something to talk about.",
				
			"Yü",
			"Enthusiasm",
			"ENTHUSIASM. It furthers one to install helpers\n"
				"And to set armies marching.",
			"Thunder comes resounding out of the earth:\n"
				"The image of ENTHUSIASM.\n"
				"Thus the ancient kings made music\n"
				"In order to honor merit,\n"
				"And offered it with splendor\n"
				"To the Supreme Deity,\n"
				"Inviting their ancestors to be present.",
			"",
			"Enthusiasm that expresses itself\n"
				"Brings misfortune.",
			"Firm as a rock. Not a whole day.\n"
				"Perseverance brings good fortune.",
			"Enthusiasm that looks upward creates remorse.\n"
				"Hesitation brings remorse.",
			"The source of enthusiasm.\n"
				"He achieves great things.\n"
				"Doubt not.\n"
				"You gather friends around you\n"
				"As a hair clasp gathers the hair.",
			"Persistently ill, and still does not die.",
			"Deluded enthusiasm.\n"
				"But if after completion one changes,\n"
				"There is no blame.",
				
			"Hsieh",
			"Deliverance",
			"DELIVERANCE. The southwest furthers.\n"
				"If there is no longer anything where one has to go,\n"
				"Return brings good fortune.\n"
				"If there is still something where one has to go,\n"
				"Hastening brings good fortune.",
			"Thunder and rain set in:\n"
				"The image of DELIVERANCE.\n"
				"Thus the superior man pardons mistakes\n"
				"And forgives misdeeds.",
			"",
			"Without blame.",
			"One kills three foxes in the field\n"
				"And receives a yellow arrow.\n"
				"Perseverance brings good fortune.",
			"If a man carries a burden on his back\n"
				"And nonetheless rides in a carriage,\n"
				"He thereby encourages robbers to draw near.\n"
				"Perseverance leads to humiliation.",
			"Deliver yourself from your great toe.\n"
				"Then the companion comes,\n"
				"And him you can trust.",
			"If only the superior man can deliver himself,\n"
				"It brings good fortune.\n"
				"Thus he proves to inferior men that he is in earnest.",
			"The prince shoots at a hawk on a high wall.\n"
				"He kills it. Everything serves to further.",
				
			"Heng",
			"Duration",
			"DURATION. Success. No blame.\n"
				"Perseverance furthers.\n"
				"It furthers one to have somewhere to go.",
			"Thunder and wind: the image of DURATION.\n"
				"Thus the superior man stands firm\n"
				"And does not change his direction.",
			"",
			"Seeking duration too hastily brings misfortune persistently.\n"
				"Nothing that would further.",
			"Remorse disappears.",
			"He who does not give duration to his character\n"
				"Meets with disgrace.\n"
				"Persistent humiliation.",
			"No game in the field.",
			"Giving duration to one's character through perseverance.\n"
				"This is good fortune for a woman, misfortune for a man.",
			"Restlessness as an enduring condition brings misfortune.",
			
			"Sheng",
			"Pushing Upward",
			"PUSHING UPWARD has supreme success.\n"
				"One must see the great man.\n"
				"Fear not.\n"
				"Departure toward the south\n"
				"Brings good fortune.",
			"Within the earth, wood grows:\n"
				"The image of PUSHING UPWARD.\n"
				"Thus the superior man of devoted character\n"
				"Heaps up small things\n"
				"In order to achieve something high and great.",
			"",
			"Pushing upward that meets with confidence.",
			"If one is sincere,\n"
				"It furthers one to bring even a small offering.\n"
				"No blame.",
			"One pushes upward into an empty city.",
			"The king offers him Mount Ch'i.\n"
				"Good fortune. No blame.",
			"Perseverance brings good fortune.\n"
				"One pushes upward by steps.",
			"Pushing upward in darkness.\n"
				"It furthers one\n"
				"To be unremittingly persevering.",
				
			"Ching",
			"The Well",
			"THE WELL. The town may be changed,\n"
				"But the well cannot be changed.\n"
				"It neither decreases nor increases.\n"
				"They come and go and draw from the well.+",
			"Water over wood: the image of THE WELL.\n"
				"Thus the superior man encourages the people at their work,\n"
				"And exhorts them to help one another.",
			"If one gets down almost to the water\n"
				"And the rope does not go all the way,\n"
				"Or the jug breaks, it brings misfortune.",
			"One does not drink the mud of the well.\n"
				"No animals come to an old well.",
			"At the wellhole one shoot fishes.\n"
				"The jug is broken and leaks.",
			"The well is cleaned, but no one drinks from it.\n"
				"This is my heart's sorrow.\n"
				"For one might draw from it.\n"
				"If the king were clear-minded,\n"
				"Good fortune might be enjoyed in common.",
			"The well is being lined. No blame.",
			"In the well there is a clear, cold spring\n"
				"From which one can drink.",
			"One draws from the well\n"
				"Without hindrance.\n"
				"It is dependable.\n"
				"Supreme good fortune.",
				
			"Ta Kuo",
			"Preponderance of the Great",
			"PREPONDERANCE OF THE GREAT.\n"
				"The ridgepole sags at the breaking point.\n"
				"It furthers one to have somewhere to go.\n"
				"Success.",
			"The lake rises above the trees:\n"
				"The image of PREPONDERANCE OF THE GREAT.\n"
				"Thus the superior man, when he stands alone,\n"
				"Is unconcerned,\n"
				"And if he has to renounce the world,\n"
				"He is undaunted.",
			"",
			"To spread white rushes underneath.\n"
				"No blame.",
			"A dry poplar sprouts at the root.\n"
				"An older man takes a young wife.\n"
				"Everything furthers.",
			"The ridgepole sags to the breaking point.\n"
				"Misfortune.",
			"The ridgepole is braced. Good fortune.\n"
				"If there are ulterior motives, it is humiliating.",
			"A withered poplar puts forth flowers.\n"
				"An older woman takes a husband.\n"
				"No blame.  No praise",
			"One must go through the water.\n"
				"It goes over one's head.\n"
				"Misfortune. No blame.",
			"Sui",
			"Following",
			"FOLLOWING has supreme success.\n"
				"Perseverance furthers. No blame.",
			"Thunder in the middle of the lake:\n"
				"The image of FOLLOWING.\n"
				"Thus the superior man at nightfall\n"
				"Goes indoors for rest and recuperation.",
			"",
			"The standard is changing.\n"
				"Perseverance brings good fortune.\n"
				"To go out of the door in company\n"
				"Produces deeds.",
			"If one clings to the little boy,\n"
				"One loses the strong man.\n",
			"If one clings to the strong man,\n"
				"One loses the little boy.\n"
				"Though following one finds what one seeks.\n"
				"It furthers one to remain persevering.",
			"Following creates success.\n"
				"Perseverance brings good fortune.\n"
				"To go one's way with sincerity brings clarity.\n"
				"How could there be blame in this?",
			"Sincere in the good. Good fortune.",
			"He meets with firm allegiance\n"
				"And is still further bound.\n"
				"The king introduces him\n"
				"To the Western Mountain."
		}
	};

	resource 'STR#' (17522, "lp.i Sun") {
		{	/* array StringArray */
			"Sun",
			"The Gentle (The Penetrating, Wind)",
			"THE GENTLE. Success through what is small.\n"
				"It furthers one to have somewhere to go.\n"
				"It furthers one to see the great man.",
			"Winds following one upon the other:\n"
				"The image of THE GENTLY PENETRATING.\n"
				"Thus the superior man\n"
				"Spreads his commands abroad\n"
				"And carries out his undertakings.",
			"",
			"In advancing and in retreating,\n"
				"The perseverance of a warrior furthers.",
			"Penetration under the bed.\n"
				"Priests and magicians are used in great number.\n"
				"Good fortune. No blame.",
			"Repeated penetration. Humiliation.",
			"Remorse vanishes.\n"
				"During the hunt\n"
				"Three kinds of game are caught.",
			"Perseverance brings good fortune.\n"
				"Remorse vanishes.\n"
				"Nothing that does not further.\n"
				"No beginning, but an end.\n"
				"Before the change, three days.\n"
				"After the change, three days.\n"
				"Good fortune.",
			"Penetration under the bed.\n"
				"He loses his property and his ax.\n"
				"Perseverance brings misfortune.",
				
			"Hsiao Ch’u",
			"The Taming Power of the Small",
			"THE TAMING POWER OF THE SMALL\n"
				"Has success.\n"
				"Dense clouds, no rain from our western region.",
			"The wind drives across heaven:\n"
				"The image of THE TAMING POWER OF THE SMALL.\n"
				"Thus the superior man\n"
				"Refines the outward aspect of his nature.",
			"",
			"Return to the way.\n"
				"How could there be blame in this?\n"
				"Good fortune.",
			"He allows himself to be drawn into returning.\n"
				"Good fortune.",
			"The spokes burst out of the wagon wheels.\n"
				"Man and wife roll their eyes.",
			"If you are sincere, blood vanishes and fear gives way.\n"
				"No blame.",
			"If you are sincere and loyally attached,\n"
				"You are rich in your neighbor.",
			"The rain comes, there is rest.\n"
				"This is due to the lasting effect of the character.\n"
				"Perseverance brings the woman into danger.\n"
				"The moon is nearly full.\n"
				"If the superior man persists,\n"
				"Misfortune comes.",
				
			"Chia Jen",
			"The Family (The Clan)",
			"THE FAMILY. The perseverance of the woman furthers.",
			"Wind comes forth from fire:\n"
				"The image of THE FAMILY.\n"
				"Thus the superior man has substance in his words\n"
				"And duration in his way of life.",
			"",
			"Firm seclusion within the family.\n"
				"Remorse disappears.",
			"She could not follow her whims.\n"
				"She must attend within to the food.\n"
				"Perseverance brings good fortune.",
			"When tempers flare up in the family,\n"
				"Too great severity brings remorse.\n"
				"Good fortune nonetheless.\n"
				"When woman and child dally and laugh,\n"
				"It leads in the end to humiliation.",
			"She is the treasure of the house.\n"
				"Good fortune.",
			"As a king he approaches his family.\n"
				"Fear not.\n"
				"Good fortune.",
			"His work commands respect.\n"
				"In the end good fortune comes.",
				
			"I",
			"Increase",
			"INCREASE. It furthers one\n"
				"To undertake something.\n"
				"It furthers one to cross the great water.",
			"Wind and thunder: the image of INCREASE.\n"
				"Thus the superior man:\n"
				"If he sees good, he imitates it;\n"
				"If he has faults, he rids himself of them.",
			"",
			"It furthers one to accomplish great deeds.\n"
				"Supreme good fortune. No blame.",
			"Someone does indeed increase him;\n"
				"Ten pairs of tortoises cannot oppose it.\n"
				"Constant perseverance brings good fortune.\n"
				"The presents him before God.\n"
				"Good fortune.",
			"One is enriched through unfortunate events.\n"
				"No blame, if you are sincere\n"
				"And walk in the middle,\n"
				"And report with a seal to the prince.",
			"If you walk in the middle \n"
				"And report to the prince,\n"
				"He will follow.\n"
				"It furthers one to be used\n"
				"In the removal of the capital.",
			"If in truth you have a kind heart, ask not.\n"
				"Supreme good fortune.\n"
				"Truly, kindness will be recognized as your virtue.",
			"He brings increase to no one.\n"
				"Indeed, someone even strikes him.\n"
				"He does not keep his heart constantly steady.\n"
				"Misfortune.",
				
			"Wu Wang",
			"Innocence (The Unexpected)",
			"INNOCENCE. Supreme success.\n"
				"Perseverance furthers.\n"
				"If someone is not as he should be,\n"
				"He has misfortune,\n"
				"And it does not further him\n"
				"To undertake anything.",
			"Under heaven thunder rolls:\n"
				"All things attain the natural state of innocence.\n"
				"Thus the kings of old,\n"
				"Rich in virtue, and in harmony with the time,\n"
				"Fostered and nourished all beings.",
			"",
			"Innocent behavior brings good fortune.",
			"If one does not count on the harvest while plowing,\n"
				"Nor on the use of the ground while clearing it,\n"
				"It furthers one to undertake something.",
			"Undeserved misfortune.\n"
				"The cow that was tethered by someone\n"
				"Is the wanderer’s gain, the citizen’s loss.",
			"He who can be persevering \n"
				"Remains without blame.",
			"Use no medicine in an illness\n"
				"Incurred through no fault of your own.\n"
				"It will pass of itself.",
			"Innocent action brings misfortune.\n"
				"Nothing furthers.",
				
			"Shih Ho",
			"Biting Through",
			"BITING THROUGH has success.\n"
				"It is favorable to let justice be administered.",
			"Thunder and lightning:\n"
				"The image of BITING THROUGH.\n"
				"Thus the kings of former times made firm the laws\n"
				"Through clearly defined penalties.",
			"",
			"His feet are fastened in stocks,\n"
				"So that his toes disappear.\n"
				"No blame.",
			"Bites through tender meat,\n"
				"So that his nose disappears.\n"
				"No blame.",
			"Bites on old dried meat\n"
				"And strikes on something poisonous.\n"
				"Slight humiliation.  No blame.",
			"Bites on dried gristly meat.\n"
				"Receives metal arrows.\n"
				"It furthers one to be mindful of difficulties\n"
				"And to be persevering.\n"
				"Good fortune.",
			"Bites on dried lean meat.\n"
				"Receives yellow gold.\n"
				"Perseveringly aware of danger.\n"
				"No blame.",
			"His neck is fastened in the wooden cangue,\n"
				"So that his ears disappear.\n"
				"Misfortune.",
				
			"I",
			"The Corners of the Mouth (Providing Nourishment)",
			"THE CORNERS OF THE MOUTH.\n"
				"Perseverance brings good fortune.\n"
				"Pay heed to the providing of nourishment\n"
				"And to what a man seeks\n"
				"To fill his own mouth with.",
			"At the foot of the mountain, thunder:\n"
				"The image of PROVIDING NOURISHMENT.\n"
				"Thus the superior man is careful of his own words\n"
				"And temperate in eating and drinking.",
			"",
			"You let your magic tortoise go,\n"
				"And look at me with the corners of your mouth drooping.\n"
				"Misfortune.",
			"Turning to the summit for nourishment,\n"
				"Deviating from the path\n"
				"To seek nourishment from the hill.\n"
				"Continuing to do so brings misfortune.",
			"Turning away from nourishment.\n"
				"Perseverance brings misfortune.\n"
				"Do not act thus for ten years.\n"
				"Nothing serves to further.",
			"Turning to the summit\n"
				"For provision of nourishment\n"
				"Brings good fortune.\n"
				"Spying about with sharp eyes\n"
				"Like a tiger with insatiable craving.\n"
				"No blame.",
			"Turning away from the path.\n"
				"To remain persevering brings good fortune.\n"
				"One should not cross the great water.",
			"The source of nourishment.\n"
				"Awareness of danger brings good fortune.\n"
				"It furthers one to cross the great water.",
				
			"Ku",
			"Work on what has been Spoiled (Decay)",
			"WORK ON WHAT HAS BEEN SPOILED\n"
				"Has supreme success.\n"
				"It furthers one to cross the great water.\n"
				"Before the starting point, three days.\n"
				"After the starting point, three days.",
			"The wind blows low on the mountain:\n"
				"The image of DECAY.\n"
				"Thus the superior man stirs up the people\n"
				"And strengthens their spirit.",
			"",
			"Setting right what has been spoiled by the father.\n"
				"If there is a son,\n"
				"No blame rests on the departed father.\n"
				"Danger. In the end good fortune.",
			"Setting right what has been spoiled by the mother.\n"
				"One must not be too persevering.",
			"Setting right what has been spoiled by the father.\n"
				"There will be a little remorse. No great blame.",
			"Tolerating what has been spoiled by the father.\n"
				"In continuing one sees humiliation.",
			"Setting right what has been spoiled by the father.\n"
				"One meets with praise.",
			"He does not serve kings and princes,\n"
				"Set himself higher goals."
		}
	};

	resource 'STR#' (17523, "lp.i Li") {
		{	/* array StringArray */
			" Li",
			"The Clinging, Fire",
			"THE CLINGING. Perseverance furthers.\n"
				"It brings success.\n"
				"Care of the cow brings good fortune.",
			"That which is bright rises twice:\n"
				"The image of FIRE.\n"
				"Thus the great man, by perpetuating this brightness,\n"
				"Illuminates the four quarters of the world.",
			"",
			"The footprints run crisscross.\n"
				"If one is seriously intent, no blame.",
			"Yellow light. Supreme good fortune.",
			"In the light of the setting sun,\n"
				"Men either beat the pot and sing\n"
				"Or loudly bewail the approach of old age.\n"
				"Misfortune.",
			"Its coming is sudden.\n"
				"It flames up, dies down\n is thrown away.",
			"Tears in floods, sighing and lamenting.\n"
				"Good fortune.",
			"The king uses him to march forth and chastise.\n"
				"Then it is best to kill the leaders\n"
				"And take captive the followers. No blame.",
				
			"Lü",
			"The Wanderer",
			"THE WANDERER. Success through smallness.\n"
				"Perseverance brings good fortune\n"
				"To the wanderer.",
			"Fire on the mountain:\n"
				"The image of THE WANDERER.\n"
				"Thus the superior man\n"
				"Is clear-minded and cautious\n"
				"In imposing penalties,\n"
				"And protracts no lawsuits.",
			"",
			"If the wanderer busies himself with trivial things,\n"
				"He draws down misfortune upon himself.",
			"The wanderer comes to an inn.\n"
				"He has his property with him.\n"
				"He wins the steadfastness of a young servant.",
			"The wanderer’s in burns down.\n"
				"He loses the steadfastness of his young servant.\n"
				"Danger.",
			"The wanderer rests in a shelter.\n"
				"He obtains his property and an ax.\n"
				"My heart is not glad.",
			"He shoots a pheasant.\n"
				"It drops with the first arrow.\n"
				"In the end this brings both praise and office.",
			"The bird’s nest burns up.\n"
				"The wanderer laughs at first,\n"
				"Then must needs lament and weep.\n"
				"Through carelessness he loses his cow.\n"
				"Misfortune.",
				
			"Ting",
			"The Caldron",
			"THE CALDRON. Supreme good fortune.\n"
				"Success.",
			"Fire over wood:\n"
				"The image of THE CALDRON.\n"
				"Thus the superior man consolidates his fate\n"
				"By making his position correct.",
			"",
			"A ting with legs upturned.\n"
				"Furthers removal of stagnating stuff.\n"
				"One takes a concubine for the sake of her son.\n"
				"No blame.",
			"There is food in the ting.\n"
				"My comrades are envious,\n"
				"But they cannot harm me.\n"
				"Good fortune.",
			"The handle of the ting is altered.\n"
				"One is impeded in his way of life.\n"
				"The fat of the pheasant is not eaten.\n"
				"Once rain falls, remorse is spent.\n"
				"Good fortune comes in the end.",
			"The legs of the ting are broken.\n"
				"The prince’s meal is spilled\n"
				"And his person soiled.\n"
				"Misfortune.",
			"The ting has yellow handles, golden carrying rings.\n"
				"Perseverance furthers.",
			"The ting has rings of jade.\n"
				"Great good fortune.\n"
				"Nothing that would not act to further.",
				
			"Wei Chi",
			"Before Completion",
			"BEFORE COMPLETION. Success.\n"
				"But if the little fox, after nearly completing the crossing,\n"
				"Gets his tail in the water,\n"
				"There is nothing that would further.",
			"Fire over water:\n"
				"The image of the condition before transition.\n"
				"Thus the superior man is careful\n"
				"In the differentiation of things,\n"
				"So that each finds its place.",
			"",
			"He gets his tail in the water.\n"
				"Humiliating.",
			"He brakes his wheels.\n"
				"Perseverance brings good fortune.",
			"Before completion, attack brings misfortune.\n"
				"It furthers one to cross the great water.",
			"Perseverance brings good fortune.\n"
				"Remorse disappears.\n"
				"Shock, thus to discipline the Devil’s Country.\n"
				"For three years, great realms are awarded.",
			"Perseverance brings good fortune.\n"
				"No remorse.\n"
				"The light of the superior man is true.\n"
				"Good fortune.",
			"There is drinking of wine\n"
				"In genuine confidence. No blame.\n"
				"But if one wets his head,\n"
				"He loses it, in truth.",
				
			"Meng",
			"Youthful Folly",
			"YOUTHFUL FOLLY has success;\n"
				"It is not I who seek the young fool;\n"
				"The young fool seeks me.\n"
				"At the first oracle I inform him.\n"
				"If he asks two or three times, it is importunity.\n"
				"If he importunes, I give him no information.\n"
				"Perseverance furthers.\n",
			"A spring wells up at the foot of the mountain:\n"
				"The image of YOUTH.\n"
				"Thus the superior man fosters his character\n"
				"By thoroughness in all that he does.",
			"",
			"To make a fool develop\n"
				"It furthers one to apply discipline.\n"
				"The fetters should be removed.\n"
				"To go on in this way brings humiliation.",
			"To bear with fools in kindliness brings good fortune.\n"
				"To know how to take women brings good fortune.\n"
				"The son is capable of taking charge of the household.",
			"Take not a maiden who, when she sees a man of bronze,\n"
				"Loses possession of herself.\n"
				"Nothing furthers.",
			"Entangled folly brings humiliation.",
			"Childlike folly brings good fortune.",
			"In punishing folly,\n"
				"It does not further one\n"
				"To commit transgressions.\n"
				"The only thing that furthers\n"
				"Is to prevent transgressions.",
				
			"Huan",
			"Dispersion (Dissolution)",
			"DISPERSION. Success.\n"
				"The king approaches his temple.\n"
				"It furthers one to cross the great water.\n"
				"Perseverance furthers.",
			"The wind drives over the water:\n"
				"The image of DISPERSION.\n"
				"Thus the kings of old sacrificed to the Lord\n"
				"And built temples.",
			"",
			"He brings help with the strength of a horse.\n"
				"Good fortune.",
			"At the dissolution\n"
				"He hurries to that which supports him.\n"
				"Remorse disappears.",
			"He dissolves his self. No remorse.",
			"He dissolves his bond with his group.\n"
				"Supreme good fortune.\n"
				"Dispersion leads in turn to accumulation.\n"
				"This is something that ordinary men do not think of.",
			"His loud cries are dissolving as sweat.\n"
				"Dissolution! A king abides without blame.",
			"He dissolves his blood.\n"
				"Departing, keeping at a distance, going out,\n"
				"Is without blame.",
				
			"Sung",
			"Conflict",
			"CONFLICT. You are sincere\n"
				"And are being obstructed.\n"
				"A cautious halt halfway brings good fortune.\n"
				"Going through to the end brings misfortune.\n"
				"It furthers one to see the great man.\n"
				"It does not further one to cross the great water.\n",
			"Heaven and water go their opposite ways:\n"
				"The image of CONFLICT.\n"
				"Thus in all his transactions the superior man\n"
				"Carefully considers the beginning.",
			"",
			"If one does not perpetuate the affair,\n"
				"There is a little gossip.\n"
				"In the end, good fortune comes.",
			"One cannot engage in conflict;\n"
				"One returns home, gives way.\n"
				"The people of his town,\n"
				"Three hundred households,\n"
				"Remain free of guilt.",
			"To nourish oneself on ancient virtue induces perseverance.\n"
				"Danger. In the end, good fortune comes.\n"
				"If by chance you are in the service of a king,\n"
				"Seek not works.",
			"One cannot engage in conflict.\n"
				"One turns back and submits to fate,\n"
				"Changes one’s attitude,\n"
				"And finds peace in perseverance.\n"
				"Good fortune.",
			"To contend before him\n"
				"Brings supreme good fortune.",
			"Even if by chance a leather belt is bestowed on one,\n"
				"By the end of a morning\n"
				"It will have been snatched away three times.",
				
			"T’ung Jen",
			"Fellowship with Men",
			"FELLOWSHIP WITH MEN in the open.\n"
				"Success.\n"
				"It furthers one to cross the great water.\n"
				"The perseverance of the superior man furthers.",
			"Heaven together with fire:\n"
				"The image of FELLOWSHIP WITH MEN.\n"
				"Thus the superior man organizes the clans\n"
				"And makes distinctions between things.",
			"",
			"Fellowship with men at the gate.\n"
				"No blame.",
			"Fellowship with men in the clan.\n"
				"Humiliation.",
			"He hides weapons in the thicket;\n"
				"He climbs the high hill in front of it.\n"
				"For three years he does not rise up.",
			"He climbs up on his wall; he cannot attack.\n"
				"Good fortune.",
			"Men bound in fellowship first weep and lament,\n"
				"But afterward they laugh.\n"
				"After great struggles they succeed in meeting.",
			"Fellowship with men in the meadow.\n"
				"No remorse."
		}
	};

	resource 'STR#' (17524, "lp.i K’un") {
		{	/* array StringArray */
			"K’un",
			"The Receptive",
			"THE RECEPTIVE brings about sublime success,\n"
				"Furthering through the perseverance of a mare.\n"
				"If the superior man undertakes something and tries to lead,\n"
				"He goes astray;\n"
				"But if he follows, he finds guidance.+",
			"The earth's condition is receptive devotion.\n"
				"Thus the superior man who has breadth of characte\n"
				"Carries the outer world.\n",
			"It is favorable to find friends in the west and south,\n"
				"To forego friends in the east and north.\n"
				"Quiet perseverance brings good fortune.",
			"When there is hoarfrost underfoot,\n"
				"Solid ice is not far off.\n",
			"Straight, square, great.\n"
				"Without purpose,\n"
				"Yet nothing remains unfurthered.",
			"Hidden lines.\n"
				"One is able to remain persevering.\n"
				"If by chance you are in the service of a king,\n"
				"Seek not works, but bring to completion.",
			"A tied-up sack. No blame, no praise.",
			"A yellow lower garment brings supreme good fortune.",
			"Dragons fight in the meadow.\n"
				"Their blood is black and yellow.\n"
				"When all the lines are sixes, it means:\n"
				"Lasting perseverance furthers.",
				
			"Fu",
			"Return (The Turning Point)",
			"RETURN. Success.\n"
				"Going out and coming in without error.\n"
				"Friends come without blame.\n"
				"To and fro goes the way.\n"
				"On the seventh day comes return.\n"
				"It furthers one to have somewhere to go.",
			"Thunder within the earth:\n"
				"The image of THE TURNING POINT.\n"
				"Thus the kings of antiquity closed the passes\n"
				"At the time of the solstice.\n"
				"Merchants and strangers did not go about,\n"
				"And the ruler\n"
				"Did not travel through the provinces.",
			"",
			"Return from a short distance.\n"
				"No need for remorse.",
			"Quiet return. Good fortune.",
			"Repeated return. Danger. No blame.",
			"Walking in the midst of others,\n"
				"One returns alone.",
			"Noble-hearted return. No remorse.",
			"Missing the return. Misfortune.\n"
				"Misfortune from within and without.\n"
				"If armies are set to marching in this way,\n"
				"One will in the end suffer a great defeat,\n"
				"Disastrous for the ruler of the country.\n"
				"For ten years\n"
				"It will not be possible to attack again",
				
			"Lin",
			"Approach",
			"APPROACH has supreme success.\n"
				"Perseverance furthers.\n"
				"When the eighth month comes,\n"
				"There will be misfortune.",
			"The earth above the lake:\n"
				"The image of APPROACH.\n"
				"Thus the superior man is inexhaustible\n"
				"In his will to teach,\n"
				"And without limits\n"
				"In his tolerance and protection of the people.",
			"",
			"Joint approach.\n"
				"Perseverance brings good fortune.",
			"Joint approach.\n"
				"Good fortune.\n"
				"Everything furthers.",
			"Comfortable approach.\n"
				"Nothing that would further.\n"
				"If one is induced to grieve over it,\n"
				"One becomes free of blame.",
			"Complete approach.\n"
				"No blame.",
			"Wise approach.\n"
				"This is right for a great prince.\n"
				"Good fortune.",
			"Great-hearted approach.\n"
				"Good fortune. No blame.",
				
			"T’ai",
			"Peace",
			"PEACE. The small departs,\n"
				"The great approaches.\n"
				"Good fortune. Success.",
			"Heaven and earth unite: the image of PEACE.\n"
				"Thus the ruler\n"
				"Divides and completes the course of heaven and earth;\n"
				"He furthers and regulates the gifts of heaven and earth,\n"
				"And so aids the people.",
			"",
			"When ribbon grass is pulled up, the sod comes with it.\n"
				"Each according to his kind.\n"
				"Undertakings bring good fortune.",
			"Bearing with the uncultured in gentleness,\n"
				"Fording the river with resolution,\n"
				"Not neglecting what is distant,\n"
				"Not regarding one's companions:\n"
				"Thus one may manage to walk in the middle.\n",
			"No plain not followed by a slope.\n"
				"No going not followed by a return.\n"
				"He who remains persevering in danger\n"
				"Is without blame.\n"
				"Do not complain about this truth;\n"
				"Enjoy the good fortune you still possess.",
			"He flutters down, not boasting of his wealth,\n"
				"Together with his neighbor,\n"
				"Guileless and sincere.",
			"The sovereign I\n"
				"Gives his daughter in marriage.\n"
				"This brings blessing\n"
				"And supreme good fortune.",
			"The wall falls back into the moat.\n"
				"Use no army now.\n"
				"Make your commands known within your own town.\n"
				"Perseverance brings humiliation.",
				
			"Ta Chuang",
			"The Power of the Great",
			"THE POWER OF THE GREAT. Perseverance furthers.",
			"Thunder in heaven above:\n"
				"The image of THE POWER OF THE GREAT.\n"
				"Thus the superior man does not tread upon paths\n"
				"That do not accord with established order.",
			"",
			"Power in the toes.\n"
				"Continuing brings misfortune.\n"
				"This is certainly true.",
			"Perseverance brings good fortune.",
			"The inferior man works through power.\n"
				"The superior man does not act thus.\n"
				"To continue is dangerous.\n"
				"A goat butts against a hedge\n"
				"And gets its horns entangled.",
			"Perseverance brings good fortune.\n"
				"Remorse disappears.\n"
				"The hedge opens; there is no entanglement.\n"
				"Power depends upon the axle of a big cart.",
			"Loses the goat with ease.\n"
				"No remorse.",
			"A goat butts against a hedge.\n"
				"It cannot go backward, it cannot go forward.\n"
				"Nothing serves to further.\n"
				"If one notes the difficulty, this brings good fortune.",
				
			"Kuai",
			"Break-through (Resoluteness)",
			"BREAK-THROUGH. One must resolutely make the matter known\n"
				"At the court of the king.\n"
				"It must be announced truthfully. Danger.\n"
				"It is necessary to notify one's own city.\n"
				"It does not further one to resort to arms.\n"
				"It furthers one to undertake something.",
			"The lake has risen up to heaven:\n"
				"The image of BREAK-THROUGH.\n"
				"Thus the superior man\n"
				"Dispenses riches downward\n"
				"And refrains from resting on his virtue.",
			"",
			"Mighty in the forward-striding toes.\n"
				"When one goes and is not equal to the task,\n"
				"One makes a mistake.",
			"A cry of alarm. Arms at evening and at night.\n"
				"Fear nothing.",
			"To be powerful in the cheekbones\n"
				"Brings misfortune.\n"
				"The superior man is firmly resolved.\n"
				"He walks alone and is caught in the rain.\n"
				"He is bespattered,\n"
				"And people murmur against him.\n"
				"No blame.",
			"There is no skin on his thighs,\n"
				"And walking comes hard.\n"
				"If a man were to let himself be led like a sheep,\n"
				"Remorse would disappear.\n"
				"But if these words are heard,\n"
				"They will not be believed.",
			"In dealing with weeds,\n"
				"Firm resolution is necessary.\n"
				"Walking in the middle\n"
				"Remains free of blame.",
			"No cry.\n"
				"In the end misfortune comes.",
				
			"Hsü",
			"Waiting (Nourishment)",
			"WAITING. If you are sincere,\n"
				"You have light and success.\n"
				"Perseverance brings good fortune.\n"
				"It furthers one to cross the great water.",
			"Clouds rise up to heaven:\n"
				"The image of WAITING.\n"
				"Thus the superior man eats and drinks,\n"
				"Is joyous and of good cheer.",
			"",
			"Waiting in the meadow.\n"
				"It furthers one to abide in what endures.",
			"Waiting on the sand.\n"
				"There is some gossip.\n"
				"The end brings good fortune.",
			"Waiting in the mud\n"
				"Brings about the arrival of the enemy.",
			"Waiting in blood.\n"
				"Get out of the pit.",
			"Waiting at meat and drink.\n"
				"Perseverance brings good fortune.",
			"One falls into the pit.\n"
				"Three uninvited guests arrive.\n"
				"Honor them, and in the end there will be good fortune.",
				
			"Pi",
			"Holding Together (Union)",
			"HOLDING TOGETHER brings good fortune.\n"
				"Inquire of the oracle once again\n"
				"Whether you possess sublimity, constancy, and perseverance;\n"
				"Then there is no blame.\n"
				"Those who are uncertain gradually join.\n"
				"Whoever comes too late\n"
				"Meets with misfortune.",
			"On the earth is water:\n"
				"The image of HOLDING TOGETHER.\n"
				"Thus the kings of antiquity\n"
				"Bestowed the different states as fiefs\n"
				"And cultivated friendly relations\n"
				"With the feudal lords.",
			"",
			"Hold to him in truth and loyalty;\n"
				"This is without blame.\n"
				"Truth, like a full earthen bowl:\n"
				"Thus in the end\n"
				"Good fortune comes from without.",
			"Hold to him inwardly.\n"
				"Perseverance brings good fortune.",
			"You hold together with the wrong people.",
			"Hold to him outwardly also.\n"
				"Perseverance brings good fortune.",
			"Manifestation of holding together.\n"
				"In the hunt the king uses beaters on three sides only\n"
				"And foregoes the game that runs off in front.\n"
				"The citizens need no warning.\n"
				"Good fortune.",
			"He finds no head for holding together.\n"
				"Misfortune."
		}
	};

	resource 'STR#' (17525, "lp.i Tui") {
		{	/* array StringArray */
			"Tui",
			"The Joyous, Lake",
			"THE JOYOUS. Success.\n"
				"Perseverance is favorable.",
			"Lakes resting one on the other:\n"
				"The image of THE JOYOUS.\n"
				"Thus the superior man joins with his friends\n"
				"For discussion and practice.",
			"",
			"Contented joyousness. Good fortune.",
			"Sincere joyousness. Good fortune.\n"
				"Remorse disappears.",
			"Coming joyousness. Misfortune.",
			"Joyousness that is weighed is not at peace.\n"
				"After ridding himself of mistakes a man has joy.",
			"Sincerity toward disintegrating influences is dangerous.",
			"Seductive joyousness.",
			
			"K'un",
			"Oppression (Exhaustion)",
			"OPPRESSION. Success. Perseverance.\n"
				"The great man brings about good fortune.\n"
				"No blame.\n"
				"When one has something to say,\n"
				"It is not believed.",
			"There is no water in the lake:\n"
				"The image of EXHAUSTION.\n"
				"Thus the superior man stakes his life\n"
				"On following his will.",
			"",
			"One sits oppressed under a bare tree\n"
				"And strays into a gloomy valley.\n"
				"For three years one sees nothing.",
			"One is oppressed while at meat and drink.\n"
				"The man with the scarlet knee bands is just coming.\n"
				"It furthers one to offer sacrifice.\n"
				"To set forth brings misfortune.\n"
				"No blame.",
			"A man permits himself to be oppressed by stone,\n"
				"And leans on thorns and thistles.\n"
				"He enters his house and does not see his wife.\n"
				"Misfortune.",
			"He comes very quietly, oppressed in a golden carriage.\n"
				"Humiliation, but the end is reached.",
			"His nose and feet are cut off.\n"
				"Oppression at the hands of the man with the purple knee bands.\n"
				"Joy comes softly.\n"
				"It furthers one to make offerings and libations.",
			"He is oppressed by creeping vines.\n"
				"He moves uncertainly and says, \"Movement brings remorse.\","
				"If one feels remorse over this and makes a start,\n"
				"Good fortune comes.",
				
			"Ts'ui",
			"Gathering Together (Massing)",
			"GATHERING TOGETHER. Success.\n"
				"The king approaches his temple.\n"
				"It furthers one to see the great man.\n"
				"This brings success. Perseverance furthers.\n"
				"To bring great offerings creates good fortune.\n"
				"It furthers one to undertake something.",
			"Over the earth, the lake:\n"
				"The image of GATHERING TOGETHER.\n"
				"Thus the superior man renews his weapons\n"
				"In order to meet the unforeseen.",
			"",
			"If you are sincere, but not to the end,\n"
				"There will sometimes be confusion, sometimes gathering together.\n"
				"If you call out,\n"
				"Then after one grasp of the hand you can laugh again.\n"
				"Regret not. Going is without blame.",
			"Letting oneself be drawn\n"
				"Brings good fortune and remains blameless.\n"
				"If one is sincere,\n"
				"It furthers one to bring even a small offering.",
			"Gathering together amid sighs.\n"
				"Nothing that would further.\n"
				"Going is without blame.\n"
				"Slight humiliation.",
			"Great good fortune. No blame.",
			"If in gathering together one has position,\n"
				"This brings no blame.\n"
				"If there are some who are not yet sincerely in the work,\n"
				"Sublime and enduring perseverance is needed.\n"
				"The remorse disappears.",
			"Lamenting and sighing, floods of tears.\n"
				"No blame.",
				
			"Hsien",
			"Influence (Wooing)",
			"Influence. Success.\n"
				"Perseverance furthers.\n"
				"To take a maiden to wife brings good fortune.",
			"A lake on the mountain.\n"
				"The image of influence.\n"
				"Thus the superior man encourages people to approach him\n"
				"By his readiness to receive them.",
			"",
			"The influence shows itself in the big toe.",
			"The influence shows itself in the calves of the legs.\n"
				"Misfortune.\n"
				"Tarrying brings good fortune.",
			"The influence shows itself in the thighs.\n"
				"Holds to that which follows it.\n"
				"To continue is humiliating.",
			"Perseverance brings good fortune.\n"
				"Remorse disappears.\n"
				"If a man is agitated in mind,\n"
				"And his thoughts go hither and thither,\n"
				"Only those friends \n"
				"On whom he fixes his conscious thoughts\n"
				"Will follow.",
			"The influence shows itself in the back of the neck.\n"
				"No remorse.",
			"The influence shows itself in the jaws, cheeks\n and tongue.",
			
			"Chien",
			"Obstruction",
			"OBSTRUCTION. The southwest furthers.\n"
				"The northeast does not further.\n"
				"It furthers one to see the great man.\n"
				"Perseverance brings good fortune.",
			"Water on the mountain:\n"
				"The image of OBSTRUCTION.\n"
				"Thus the superior man turns his attention to himself\n"
				"And molds his character.",
			"",
			"Going leads to obstructions,\n"
				"Coming meets with praise.",
			"The king's servant is beset by obstruction upon obstruction,\n"
				"But it is not his own fault.",
			"Going leads to obstructions;\n"
				"Hence he comes back.",
			"Going leads to obstructions,\n"
				"Coming leads to union.",
			"In the midst of the greatest obstructions,\n"
				"Friends come.",
			"Going leads to obstructions,\n"
				"Coming leads to great good fortune.\n"
				"It furthers one to see the great man.",
				
			"Ch’ien",
			"Modesty",
			"MODESTY creates success.\n"
				"The superior man carries things through.",
			"Within the earth, a mountain:\n"
				"The image of MODESTY.\n"
				"Thus the superior man reduces that which is too much,\n"
				"And augments that which is too little.\n"
				"He weighs things and makes them equal.",
			"",
			"A superior man modest about his modesty\n"
				"May cross the great water.\n"
				"Good fortune.",
			"Modesty that comes to expression.\n"
				"Perseverance brings good fortune.",
			"A superior man of modesty and merit\n"
				"Carries things to conclusion.\n"
				"Good fortune.",
			"Nothing that would not further modesty\n"
				"In movement.",
			"No boasting of wealth before one's neighbor.\n"
				"It is favorable to attack with force.\n"
				"Nothing that would not further.",
			"Modesty that comes to expression.\n"
				"It is favorable to set armies marching\n"
				"To chastise one's own city and one's own country.",
				
			"Hsiao Ko",
			"Preponderance of the Small",
			"PREPONDERANCE OF THE SMALL. Success.\n"
				"Perseverance furthers.\n"
				"Small things may be done; great things should not be done.\n"
				"The flying bird brings the message:\n"
				"It is not well to strive upward,\n"
				"It is well to remain below.\n"
				"Great good fortune.",
			"Thunder on the mountain:\n"
				"The image of PREPONDERANCE OF THE SMALL.\n"
				"Thus in his conduct the superior man gives preponderance to reverence.\n"
				"In bereavement he gives preponderance to grief.\n"
				"In his expenditures he gives preponderance to thrift.",
			"",
			"The bird meets with misfortune through flying.",
			"She passes by her ancestor\n"
				"And meets her ancestress.\n"
				"He does not reach his prince\n"
				"And meets the official.\n"
				"No blame.",
			"If one is not extremely careful,\n"
				"Somebody may come up from behind and strike him.\n"
				"Misfortune.",
			"No blame. He meets him without passing by.\n"
				"Going brings danger. One must be on guard.\n"
				"Do not act. Be constantly persevering.",
			"Dense clouds,\n"
				"No rain from our western territory.\n"
				"The prince shoots and hits him who is in the cave.",
			"He passes him by, not meeting him.\n"
				"The flying bird leaves him.\n"
				"Misfortune.\n"
				"This means bad luck and injury.",
				
			"Kuei Mei",
			"The Marrying Maiden",
			"THE MARRYING MAIDEN.\n"
				"Undertakings bring misfortune.\n"
				"Nothing that would further.",
			"Thunder over the lake:\n"
				"The image of THE MARRYING MAIDEN.\n"
				"Thus the superior man\n"
				"Understands the transitory\n"
				"In the light of the eternity of the end.",
			"",
			"The marrying maiden as a concubine.\n"
				"A lame man who is able to tread.\n"
				"Undertakings bring good fortune.",
			"A one-eyed man who is able to see.\n"
				"The perseverance of a solitary man furthers.",
			"The marrying maiden as a slave.\n"
				"She marries as a concubine.",
			"The marrying maiden draws out the allotted time.\n"
				"A late marriage comes in due course.",
			"The sovereign I gave his daughter in marriage.\n"
				"The embroidered garments of the princess\n"
				"Were not as gorgeous\n"
				"As those of the servingmaid.\n"
				"The moon that is nearly full brings good fortune.",
			"The woman holds the basket, but there are no fruits in it.\n"
				"The man stabs the sheep, but no blood flows.\n"
				"Nothing that acts to further."
		}
	};
#endif


