.equ PLAYER_GET_TIME,      VAR_TEMP_1

PalletTown_PlayersHouse_2F_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, PalletTown_PlayersHouse_2F_OnTransition
	map_script MAP_SCRIPT_ON_WARP_INTO_MAP_TABLE, PalletTown_PlayersHouse_2F_OnWarp
	.byte 0

PalletTown_PlayersHouse_2F_OnTransition::
	call_if_eq VAR_MAP_SCENE_PALLET_TOWN_PLAYERS_HOUSE_2F, 0, PalletTown_PlayersHouse_2F_EventScript_SetRespawn
	end

PalletTown_PlayersHouse_2F_EventScript_SetRespawn::
	setrespawn HEAL_LOCATION_PALLET_TOWN
	return

PalletTown_PlayersHouse_2F_OnWarp::
	map_script_2 VAR_MAP_SCENE_PALLET_TOWN_PLAYERS_HOUSE_2F, 0, PalletTown_PlayersHouse_2F_FirstWarpIn
	.2byte 0

PalletTown_PlayersHouse_2F_FirstWarpIn::
	turnobject LOCALID_PLAYER, DIR_NORTH
	setvar VAR_MAP_SCENE_PALLET_TOWN_PLAYERS_HOUSE_2F, 1
	end

PalletTown_PlayersHouse_2F_EventScript_NES::
	msgbox PalletTown_PlayersHouse_2F_Text_PlayedWithNES, MSGBOX_SIGN
	end

PalletTown_PlayersHouse_2F_EventScript_Sign::
	msgbox PalletTown_PlayersHouse_2F_Text_PressLRForHelp, MSGBOX_SIGN
	end

PalletTown_PlayersHouse_2F_EventScript_PC::
	goto_if_questlog EventScript_ReleaseEnd
	lockall
	setvar VAR_0x8004, HELPCONTEXT_BEDROOM_PC
	special Script_SetHelpContext
	setvar VAR_0x8004, 1
	special AnimatePcTurnOn
	playse SE_PC_ON
	msgbox Text_PlayerBootedUpPC
	special BedroomPC
	waitstate
	special SetHelpContextForMap
	releaseall
	end

EventScript_PalletTown_PlayersHouse_2F_ShutDownPC::
	setvar VAR_0x8004, 1
	playse SE_PC_OFF
	special AnimatePcTurnOff
	special SetHelpContextForMap
	releaseall
	end

EventScript_PalletTown_PlayersHouse_2F_Bed::
	lock
	msgbox Text_PalletTown_PlayersHouse_2F_Bed_Ask, MSGBOX_YESNO
	waitmessage
	goto_if_eq VAR_RESULT, YES, EventScript_BufferTimeNameGet
	goto_if_eq VAR_RESULT, NO, EventScript_PalletTown_PlayersHouse_2F_Bed_NO
	goto EventScript_PalletTown_PlayersHouse_2F_Bed_NO
	end

EventScript_BufferTimeNameGet::
	delay 45
	applymovement OBJ_EVENT_ID_PLAYER, Common_Movement_FaceUp
	waitmovement
	delay 70
	applymovement OBJ_EVENT_ID_PLAYER, Common_Movement_WalkInPlaceFasterDown
	waitmovement
	delay 45
	playse SE_M_YAWN
	delay 15
	applymovement OBJ_EVENT_ID_PLAYER,Common_Movement_Smile
	waitmovement
	delay 60
	setvar VAR_0x8004, LISTMENU_TIME_OF_DAY
	special ListMenu
	waitstate
	goto_if_eq VAR_RESULT, 0, EventScript_PalletTown_PlayersHouse_2F_DAWN
	goto_if_eq VAR_RESULT, 1, EventScript_PalletTown_PlayersHouse_2F_DAWN
	goto_if_eq VAR_RESULT, 2, EventScript_PalletTown_PlayersHouse_2F_NOON
	goto_if_eq VAR_RESULT, 3, EventScript_PalletTown_PlayersHouse_2F_DUSK
	goto_if_eq VAR_RESULT, 4, EventScript_PalletTown_PlayersHouse_2F_MIDNIGHT
	goto_if_eq VAR_RESULT, 5, EventScript_PalletTown_PlayersHouse_2F_Bed_NO
	goto EventScript_PalletTown_PlayersHouse_2F_Bed_NO
	end

EventScript_PalletTown_PlayersHouse_2F_PREDAWN::
	closemessage
	call EventScript_OutOfCenterPartyHeal

	setvar VAR_0x8003, TIME_PREDAWN
	special SetTimeOfDay
	specialvar VAR_TEMP_1, GetTimeOfDay
	buffernumberstring STR_VAR_1, VAR_TEMP_1
	msgbox Text_PalletTown_PlayersHouse_2F_PREDAWN
	waitmessage
	release
	end

EventScript_PalletTown_PlayersHouse_2F_DAWN::
	closemessage
	call EventScript_OutOfCenterPartyHeal

	setvar VAR_0x8003, TIME_DAWN
	special SetTimeOfDay
	specialvar VAR_TEMP_1, GetTimeOfDay
	buffernumberstring STR_VAR_1, VAR_TEMP_1
	msgbox Text_PalletTown_PlayersHouse_2F_DAWN
	waitmessage
	release
	end

EventScript_PalletTown_PlayersHouse_2F_NOON::
	closemessage
	call EventScript_OutOfCenterPartyHeal

	setvar VAR_0x8003, TIME_NOON
	special SetTimeOfDay
	specialvar VAR_TEMP_1, GetTimeOfDay
	buffernumberstring STR_VAR_1, VAR_TEMP_1
	msgbox Text_PalletTown_PlayersHouse_2F_NOON
	waitmessage
	release
	end

EventScript_PalletTown_PlayersHouse_2F_DUSK::
	closemessage
	call EventScript_OutOfCenterPartyHeal

	setvar VAR_0x8003, TIME_DUSK
	special SetTimeOfDay
	specialvar VAR_TEMP_1, GetTimeOfDay
	buffernumberstring STR_VAR_1, VAR_TEMP_1
	msgbox Text_PalletTown_PlayersHouse_2F_DUSK
	waitmessage
	release
	end
	
EventScript_PalletTown_PlayersHouse_2F_MIDNIGHT::
	closemessage
	call EventScript_OutOfCenterPartyHeal

	setvar VAR_0x8003, TIME_MIDNIGHT
	special SetTimeOfDay
	specialvar VAR_TEMP_1, GetTimeOfDay
	buffernumberstring STR_VAR_1, VAR_TEMP_1
	msgbox Text_PalletTown_PlayersHouse_2F_MIDNIGHT
	waitmessage
	release
	end

Text_PalletTown_PlayersHouse_2F_PREDAWN::
    .string "It's {STR_VAR_1} O' Hundred Hours!\n"
    .string "It's DARK!$"

Text_PalletTown_PlayersHouse_2F_DAWN::
    .string "It's {STR_VAR_1} O' Hundred Hours!\n"
    .string "Good MORNING!$"

Text_PalletTown_PlayersHouse_2F_NOON::
    .string "It's {STR_VAR_1} O' Hundred Hours!\n"
    .string "The SUN is BRIGHT!$"

Text_PalletTown_PlayersHouse_2F_DUSK::
    .string "It's {STR_VAR_1} O' Hundred Hours!\n"
    .string "It's getting LATE!$"

Text_PalletTown_PlayersHouse_2F_MIDNIGHT::
    .string "It's {STR_VAR_1} O' Hundred Hours!\n"
    .string "The MOON is GLISTENING!$"

EventScript_PalletTown_PlayersHouse_2F_Bed_NO::
	waitmessage
	msgbox Text_PalletTown_PlayersHouse_2F_Bed_Unchanged
	release
	end

Text_PalletTown_PlayersHouse_2F_Bed_Ask::
    .string "Is it time to take a rest?$"

Text_PalletTown_PlayersHouse_2F_Bed_As2k::
    .string "What Time is it?$"

Text_PalletTown_PlayersHouse_2F_Bed_Unchanged::
    .string "Not quite time to sleep$"

