Route1_MapScripts::
	.byte 0

Route1_EventScript_MartClerk::
	lock
	faceplayer
	goto_if_set FLAG_GOT_POTION_ON_ROUTE_1, Route1_EventScript_AlreadyGotPotion
	msgbox Route1_Text_WorkAtPokeMartTakeSample
	textcolor NPC_TEXT_COLOR_NEUTRAL
	checkitemspace ITEM_NEST_BALL
	goto_if_eq VAR_RESULT, FALSE, EventScript_BagIsFull
	bufferitemname STR_VAR_2, ITEM_NEST_BALL
	playfanfare MUS_LEVEL_UP
	message Text_ObtainedTheX_plural
	waitmessage
	waitfanfare
	additem ITEM_NEST_BALL
	additem ITEM_NEST_BALL
	additem ITEM_NEST_BALL
	additem ITEM_NEST_BALL
	additem ITEM_NEST_BALL
	additem ITEM_NEST_BALL
	additem ITEM_NEST_BALL
	additem ITEM_NEST_BALL
	additem ITEM_NEST_BALL
	additem ITEM_NEST_BALL
	msgbox Route1_Text_PutPotionAway
	call EventScript_RestorePrevTextColor
	setflag FLAG_GOT_POTION_ON_ROUTE_1
	release
	end

Route1_EventScript_AlreadyGotPotion::
	msgbox Route1_Text_ComeSeeUsIfYouNeedPokeBalls
	release
	end

Route1_EventScript_Boy::
	trainerbattle_single TRAINER_BUG_CATCHER_ANTHONY, Route1_Text_AnthonyIntro, Route1_Text_AnthonyDefeat
	msgbox Route1_Text_AnthonyPostBattle, MSGBOX_AUTOCLOSE
	end

Route1_Text_AnthonyIntro::
    .string "I caught a very weird POKéMON but\n"
    .string "I have no idea what it is!$"

Route1_Text_AnthonyDefeat::
    .string "Huh so it\n"
    .string "wasn't a strong POKéMON.$"

Route1_Text_AnthonyPostBattle::
    .string "Why do I have so many…?$"

Route1_EventScript_RouteSign::
	msgbox Route1_Text_RouteSign, MSGBOX_SIGN
	end
