Route4_MapScripts::
	.byte 0

Route4_EventScript_Unused::
	end

Route4_EventScript_Woman::
	msgbox Route4_Text_TrippedOverGeodude, MSGBOX_NPC
	end

Route4_EventScript_MtMoonSign::
	msgbox Route4_Text_MtMoonEntrance, MSGBOX_SIGN
	end

Route4_EventScript_RouteSign::
	msgbox Route4_Text_RouteSign, MSGBOX_SIGN
	end

Route4_EventScript_Bike::
	msgbox Route4_Text_Bike, MSGBOX_SIGN
	end

Route4_Text_Bike::
    .string "Someone put a bike here, so\n"
    .string "there is no way to enter the cave.$"
