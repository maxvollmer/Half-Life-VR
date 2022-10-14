#pragma once

enum class VRAchievement : int
{
	// Hazard Course
	HC_INDEX = 0,
	HC_SAFETYFIRST,		// Safety First! - Complete the Hazard Course

	// Black Mesa Inbound
	BMI_INDEX = 100,
	BMI_STARTTHEGAME,	// Black Mesa Inbound - Start the game

	// Anomalous Materials
	AM_INDEX = 200,
	AM_HELLOGORDON,		// Hello, Gordon! - Get the HEV suit
	AM_WELLPREPARED,	// Well Prepared - Grab the battery in your locker
	AM_SMALLCUP,		// A Small Cup - Find the small cup
	AM_BOTHERSOME,		// Bothersome - Speak to all scientists in the pre-disaster maps
	AM_MAGNUSSON,		// The Magnusson Experiment - Do the microwave thing
	AM_IMPORTANTMSG,	// Important Messages - Play with the laptop in the entrance area
	AM_TROUBLE,			// Trouble - Press the alarm button in the entrance area
	AM_LIGHTSOFF,		// Lights Out! - Turn off the lights in the office in the starting area

	// Unforeseen Consequences
	UC_INDEX = 300,
	UC_RIGHTTOOL,		// The Right Tool in the Right Place - Get the crowbar
	UC_OOPS,			// Unforeseen Consequences - Make the hanging scientist in the entrance area fall to their death
	UC_CPR,				// CPR Expert - Your moral support saved them!

	// Office Complex
	OC_INDEX = 400,
	OC_OSHAVIOLATION,	// OSHA Violation - Get killed by the eletric ceiling light
	OC_DUCK,			// Duck! - Get killed by the fan in the vents

	// "We've Got Hostiles"
	WGH_INDEX = 500,
	WGH_DOOMED,			// Doomed! - Fail to save the "we're doomed" scientist
	WGH_NOTDOOMED,		// Doomed No More! - Save the "we're doomed" scientist
	WGH_PERFECT,		// Perfect Balance - Complete chapter without activating any laser traps or trip mines

	// Blast Pit
	BP_INDEX = 600,
	BP_PERFECT,			// Perfect Landing - Fly directly into the pipe from the train
	BP_BLOWUPBRIDGES,	// How Did The Pipes Survive? - Blow up all the bridges
	BP_BROKENELVTR,		// It's a Trap! - Use the broken elevator
	BP_BADDESIGN,		// Bad Design - Solve the Blast Pit fan puzzle
	BP_SNEAKY,			// Sneaky I - Never get noticed by the tentacles

	// Power Up
	PU_INDEX = 700,
	PU_BBQ,				// Smells Like BBQ - Fry the Gargantua

	// On A Rail
	OAR_INDEX = 800,
	OAR_CHOOCHOO,		// Choo! Choo! - Drive a train
	OAR_INFINITY,		// To Infinity And Beyond - Launch the satelite rocket

	// Apprehension
	APP_INDEX = 900,

	// Residue Processing
	RP_INDEX = 1000,
	RP_PARENTAL,		// Parental Instinct - Safe the barney from running into the barnacle
	RP_MOEBIUS,			// Möbius trip - Repeat the conveyor loop

	// Questionable Ethics
	QE_INDEX = 1100,
	QE_RIGOROUS,		// Rigorous Research - Kill all the aliens with all the experiments
	QE_EFFECTIVE,		// Effective Research - Kill a soldier with an experiment
	QE_ATALLCOSTS,		// Research At All Costs - Kill a barney or scientist with an experiment
	QE_PRECISURGERY,	// Precision Surgery - Kill a scientist with the instruments

	// Surface Tension
	ST_INDEX = 1200,
	ST_VIEW,			// What a View I - Reach the cliffs
	ST_SNEAKY,			// Sneaky II - Surface behind the tank

	// "Forget About Freeman!"
	FAF_INDEX = 1300,
	FAF_FIREINHOLE,		// Fire In The Hole! - Explode the Gargantua with airstrikes

	// Lambda Core
	LC_INDEX = 1400,

	// Xen
	XEN_INDEX = 1500,
	XEN_VIEW,			// What a View II - Reach Xen

	// Gonarch's Lair
	GL_INDEX = 1600,

	// Interloper
	INT_INDEX = 1700,

	// Nihilanth
	N_INDEX = 1800,
	N_WHAT,				// What is that thing? - Meat the Nihilanth
	N_BRRR,				// Sky Baby go brrr - Beat the Nihilanth
	N_EXPLORER,			// Explorer - Get teleported to all of Nihilanth's spaces

	// Endgame
	END_INDEX = 1900,
	END_GMAN,			// Time To Choose - Face the G-Man
	END_LIMITLESS,		// Limitless Potential - Choose to join the G-Man
	END_UNWINNABLE,		// Unwinnable Battle - Don't join the G-Man

	// General
	GEN_INDEX = 2000,
	GEN_REFRESHING,		// Refreshing! - Drink a soda can
	GEN_CATCH,			// Catch! - Throw a gib at a scientist or barney
	GEN_ALIENGIB,		// Fascinating Specimen - Pick up an alien gib
	GEN_BASEBALLED,		// Baseballed - Hit a headcrab with a melee attack in mid-air
	GEN_TEAMPLAYER,		// Team Player - Don't kill any scientist or barney the entire game
	GEN_SNIPED,			// Look up! - Get sniped
	GEN_PETDOGGO,		// Pet the doggo! - Touch a houndeye without hurting it
	GEN_PACIFIST,		// Pacifist - Don't kill anyone in the entire game (except Nihilanth and the Blast Pit tentacles ofc)

	// Hidden
	HID_INDEX = 2100,
	HID_HOW,			// How? - Jump out of the intro train
	HID_ROPES,			// Look, Gordon! Ropes! - Push or lure a scientist into a barnacle
	HID_NOTTRAINS,		// These Aren't Trains - Try to use the trash compactors as trains
	HID_TROLL,			// Troll - Get the "Bothersome", "Important Messages", "Trouble", "Magnusson Experiment", and "Lights Out!" achievements
	HID_SMALLESTCUP,	// The Smallest Cup! - Hold cup in front of Kleiner
	HID_SKIP_WGH,		// Doomed? Skip! - Skip the whole "We've Got Hostiles" chapter
	HID_SKIP_PU,		// Tripmines? Ladders! - Skip the whole "Power Up" chapter
	HID_OFFARAIL,		// Off A Rail - Complete "On A Rail" without ever using a train
	HID_NOTCHEATING,	// It's Not Cheating - Gain more than 1000 HP from a negative damage door
};
