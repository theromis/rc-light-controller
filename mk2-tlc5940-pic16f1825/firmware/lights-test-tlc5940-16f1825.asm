;******************************************************************************
;
;   test-tlc5940-16f1825-lights.asm
;
;   This file contains test code to test the LED outputs on the
;   PIC16F1825 and TLC5940 based hardware running at 32 MHz.
;
;   The code never allows the main loop to be reached as it creates an 
;   endless loop during initialization.
;
;   It first switches each light on individually for 6 times with decreasing
;   brighness, then switches all lights fully on for 5 seconds (good for power 
;   consumption measurement), then all lights from lowest to full brightness
;   in 6 steps; each step having 1 second.
;   1 second, and then repeats.
;
;******************************************************************************
;
;   Author:         Werner Lane
;   E-mail:         laneboysrc@gmail.com
;
;******************************************************************************
    TITLE       Light output for hardware test
    RADIX       dec

    #include    hw.tmp

    GLOBAL Init_lights
    GLOBAL Output_lights
    
    
    ; Functions and variables imported from utils.asm
    EXTERN Init_TLC5940    
    EXTERN TLC5940_send
    
    EXTERN xl
    EXTERN temp
    EXTERN light_data

#define VAL_STEP1 63
#define VAL_STEP2 31
#define VAL_STEP3 15
#define VAL_STEP4 7
#define VAL_STEP5 3
#define VAL_STEP6 1

#define VAL_FULL 63


    
;******************************************************************************
; Relocatable variables section
;******************************************************************************
.data_lights UDATA
step_value  res 1
d1 res 1
d2 res 1
d3 res 1



;******************************************************************************
;* MACROS
;******************************************************************************
output_one_light macro   x
    call    Clear_light_data
    BANKSEL step_value
    movfw   step_value
    BANKSEL light_data
    movwf   light_data + x
    call    TLC5940_send   
    call    Delay_100ms
    endm



;============================================================================
;============================================================================
;============================================================================
.lights CODE

;******************************************************************************
; Init_lights
;******************************************************************************
Init_lights
    call    Init_TLC5940

_init_loop
    BANKSEL step_value
    movlw   VAL_STEP1
    movwf   step_value
    call    _sequence_lights

    BANKSEL step_value
    movlw   VAL_STEP2
    movwf   step_value
    call    _sequence_lights

    BANKSEL step_value
    movlw   VAL_STEP3
    movwf   step_value
    call    _sequence_lights

    BANKSEL step_value
    movlw   VAL_STEP4
    movwf   step_value
    call    _sequence_lights

    BANKSEL step_value
    movlw   VAL_STEP5
    movwf   step_value
    call    _sequence_lights

    BANKSEL step_value
    movlw   VAL_STEP6
    movwf   step_value
    call    _sequence_lights

    movlw   VAL_FULL
    call    Set_light_data
    call    TLC5940_send   
    call    Delay_1s
    call    Delay_1s
    call    Delay_1s
    call    Delay_1s
    call    Delay_1s

    movlw   VAL_STEP6
    call    Set_light_data
    call    TLC5940_send   
    call    Delay_1s

    movlw   VAL_STEP5
    call    Set_light_data
    call    TLC5940_send   
    call    Delay_1s

    movlw   VAL_STEP4
    call    Set_light_data
    call    TLC5940_send   
    call    Delay_1s

    movlw   VAL_STEP3
    call    Set_light_data
    call    TLC5940_send   
    call    Delay_1s

    movlw   VAL_STEP2
    call    Set_light_data
    call    TLC5940_send   
    call    Delay_1s

    movlw   VAL_STEP1
    call    Set_light_data
    call    TLC5940_send   
    call    Delay_1s

    call    Clear_light_data
    call    TLC5940_send   
    call    Delay_1s
    
    goto    _init_loop


_sequence_lights
    output_one_light 0
    output_one_light 1
    output_one_light 2
    output_one_light 3
    output_one_light 4
    output_one_light 5
    output_one_light 6
    output_one_light 7
    output_one_light 8
    output_one_light 9
    output_one_light 10
    output_one_light 11
    output_one_light 12
    output_one_light 13
    output_one_light 14
    output_one_light 15    
    return


;******************************************************************************
; Generated by http://www.piclist.com/cgi-bin/delay.exe (December 7, 2005 version)
;******************************************************************************
Delay_100ms
	movlw	0x6D
	movwf	d1
	movlw	0xBF
	movwf	d2
	movlw	0x02
	movwf	d3
Delay_100ms_0
	decfsz	d1, f
	goto	$+2
	decfsz	d2, f
	goto	$+2
	decfsz	d3, f
	goto	Delay_100ms_0
    return


;******************************************************************************
; Generated by http://www.piclist.com/cgi-bin/delay.exe (December 7, 2005 version)
;******************************************************************************
Delay_1s
	movlw	0x47
	movwf	d1
	movlw	0x71
	movwf	d2
	movlw	0x12
	movwf	d3
Delay_1s_0
	decfsz	d1, f
	goto	$+2
	decfsz	d2, f
	goto	$+2
	decfsz	d3, f
	goto	Delay_1s_0

	goto	$+1
	goto	$+1
	goto	$+1
	return


;******************************************************************************
; Output_lights
;******************************************************************************
Output_lights
    return              ; Never called, just here to get things compiling.


;******************************************************************************
; Clear_light_data
;
; Clear all light_data variables, i.e. by default all lights are off.
;******************************************************************************
Clear_light_data
    movlw   HIGH light_data
    movwf   FSR0H
    movlw   LOW light_data
    movwf   FSR0L
    movlw   16          ; There are 16 bytes in light_data
    movwf   temp
    clrw   
clear_light_data_loop
    movwi   FSR0++    
    decfsz  temp, f
    goto    clear_light_data_loop
    return


;******************************************************************************
; Set_light_data
;
; Set all light_data variables to a value given in W
;******************************************************************************
Set_light_data
    movwf   xl
    movlw   HIGH light_data
    movwf   FSR0H
    movlw   LOW light_data
    movwf   FSR0L
    movlw   16          ; There are 16 bytes in light_data
    movwf   temp
    movfw   xl
    goto    clear_light_data_loop

    END
