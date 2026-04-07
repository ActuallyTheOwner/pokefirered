ViridianCity_PokemonCenter_1F_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, ViridianCity_PokemonCenter_1F_OnTransition
	map_script MAP_SCRIPT_ON_RESUME, CableClub_OnResume
	.byte 0

ViridianCity_PokemonCenter_1F_OnTransition::
	setrespawn HEAL_LOCATION_VIRIDIAN_CITY
	end

ViridianCity_PokemonCenter_1F_EventScript_Nurse::
	lock
	faceplayer
	call EventScript_PkmnCenterNurse
	release
	end

ViridianCity_PokemonCenter_1F_EventScript_Scientist::
	msgbox ViridianCity_PokemonCenter_1F_Text_Scientist, MSGBOX_NPC
	end

ViridianCity_PokemonCenter_1F_EventScript_Gentleman::
	msgbox ViridianCity_PokemonCenter_1F_Text_FeelFreeToUsePC, MSGBOX_NPC
	end

@Unused until I figure out how to bugfix for later
ViridianCity_PokemonCenter_1F_Text_Rocket::
    .string "Hey you! Nurse!\n"
    .string "Battle me!$"

ViridianCity_PokemonCenter_1F_Text_Rocket_Intro::
    .string "I have no idea who you are, nor\n"
    .string "do I care anymore for whever I am.\p"
    .string "You will hand over all your POKéMON!$"

ViridianCity_PokemonCenter_1F_Text_Rocket_Defeat::
    .string "Well it was worth a shot.$"