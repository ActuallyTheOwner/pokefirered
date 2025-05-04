	.ifdef ENGLISH
	.equiv GAME_LANGUAGE, 2
	.else
	.ifdef GERMAN
	.equiv GAME_LANGUAGE, 5
	.endif
	.endif
