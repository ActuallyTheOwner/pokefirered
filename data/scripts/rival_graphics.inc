Common_EventScript_SetupRivalGfxId::
	checkplayergender
	goto_if_eq VAR_RESULT, MALE, EventScript_SetupRivalGfxIdFemale
	goto_if_eq VAR_RESULT, FEMALE, EventScript_SetupRivalGfxIdMale
	end

EventScript_SetupRivalGfxIdFemale::
	setvar VAR_OBJ_GFX_ID_8, OBJ_EVENT_GFX_RS_MAY
	return

EventScript_SetupRivalGfxIdMale::
	setvar VAR_OBJ_GFX_ID_8, OBJ_EVENT_GFX_RS_BRENDAN
	return
