; ***************************************************************************************
;			        DEPARTMENT OF ELECTRICAL AND ELECTRONIC ENGINEERING
;					               IMPERIAL COLLEGE LONDON 
;
; 				       EE 3.19: Real Time Digital Signal Processing
;					         Course  by: Dr Paul Mitcheson
;
;				        LAB 1: Getting Started with the TI C6x DSP 
;
; 				            *********** LOAD.ASM ***********
;
;	Part of the volume example. Demonstrates that assembly code can be called from C.
;		          Puts a dummy load on processor which can be varied
;		                  by modiying the value of processingLoad
;				(Where ProcessingLoad is a variable passed to the function in C).
; ***************************************************************************************
; 				         Modified by D. Harvey: 24 April 2006
;				Modified to include resource references 23 Jan 2008
; ***************************************************************************************
;
; 
        .global _load

        .text

N       .set    1000

;
;  ********************************* _load description **********************************
;  This function simulates a load on the DSP by executing N * processingLoad
;  instructions, where processingLoad is the input parameter to load() from volume.c:
;
;  								load(processingLoad);
;
;  The loop is using 8 instructions. One instruction for sub, nop and b, plus nop 5.
;  (The extra nop added after sub is to make the number of instructions in the loop 8).
;  By dividing  N * processingLoad by 8 and using the result as the loop counter,
;  N * processingLoad = the number of instruction cycles used when the function is called.
;
;  See Real Time Digial signal processing by Nasser Kehtarnavaz (page 146) for more 
;  info on mixing C and Assembly.
; ****************************************************************************************
;
_load:

        mv 		.s2 	a4, b0      ; move parameter (processingLoad) passed from C into b0 
  [!b0] b 		.s2 	lend  		; end code if processingLoad is zero 
        mvk 	.s2  	N,b1		; set b1 to value of N (set above as 1000)
        mpy 	.m2 	b1,b0,b0	; N * processingLoad -> b0
        nop							; stall the pipeline
        shru	.s2 	b0,3,b0    	; Divide b0 by 8 (b0 will be used as the loop counter)

		
loop:					; *************** 8 Instruction Loop ******************
        sub 	.d2 	b0,1,b0		; b0 - 1 -> b0
        nop							; added to make loop code 8 instruction cycles long
   [b0] b 		.s2 	loop		; loop back if b0 is not zero
        nop 			5			; stall the pipeline
						; ******************* End of Loop *********************

lend:   b 		.s2 	b3			; branch to b3 (register b3 holds the return address)
        nop 			5           ; stall the pipeline
        
        .end

