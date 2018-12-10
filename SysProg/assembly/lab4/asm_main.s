	AREA storage,DATA
array
	; Total is 232 for the example list
	DCD 3, 7, 31, 193, 2, -5, 1
array_end

	AREA main,CODE
	EXPORT asm_main
asm_main
	; Your code goes here

	B .			; Loop forever

	ALIGN
	END
