@ Handled by data/scripts/day_care.inc

Route5_PokemonDayCare_MapScripts::
	.byte 0

PokemonDayCare_EventScript_Chihiro::
	lock
	faceplayer
	setvar VAR_0x8008, INGAME_TRADE_DITTO
	call EventScript_GetInGameTradeSpeciesInfo
	goto_if_set FLAG_DID_COPYCAT_TRADE, PokemonDayCare_EventScript_AlreadyTraded
	msgbox Trade_Text_LookingForMonWannaTradeForMon, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, PokemonDayCare_EventScript_DeclineTrade
	call EventScript_ChooseMonForInGameTrade
	goto_if_ge VAR_0x8004, PARTY_SIZE, PokemonDayCare_EventScript_DeclineTrade
	call EventScript_GetInGameTradeSpecies
	goto_if_ne VAR_RESULT, VAR_0x8009, PokemonDayCare_EventScript_NotRequestedMon
	call EventScript_DoInGameTrade
	msgbox Trade_Text_HeyThanks
	setflag FLAG_DID_COPYCAT_TRADE
	release
	end

PokemonDayCare_EventScript_DeclineTrade::
	msgbox Trade_Text_AwwOhWell
	release
	end

PokemonDayCare_EventScript_NotRequestedMon::
	bufferspeciesname STR_VAR_1, VAR_0x8009
	msgbox Trade_Text_WhatThatsNoMon
	release
	end

PokemonDayCare_EventScript_AlreadyTraded::
	msgbox Trade_Text_IsntMyOldMonGreat
	release
	end
