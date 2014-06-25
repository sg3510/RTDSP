/*************************************************************************************
			       DEPARTMENT OF ELECTRICAL AND ELECTRONIC ENGINEERING
					   		     IMPERIAL COLLEGE LONDON 

 				      EE 3.19: Real Time Digital Signal Processing
					       Dr Paul Mitcheson and Daniel Harvey

				        		  LAB 6: Frame Processing

 				            ********* F R A M E. C **********

  				Demonstrates Frame Processing (Interrupt driven) on the DSK. 

 *************************************************************************************
 				Updated for use on 6713 DSK by Danny Harvey: May-Aug 2006
				Updated for ccsV4 Sept 2010
 ************************************************************************************/
/*
 *	You should modify the code so that an FFT is applied to an input frame 
 *  which is then IFFT'd and sent to the audio port.
 */
/**************************** Pre-processor statements ******************************/

//  Included so program can make use of DSP/BIOS configuration tool.  
#include <stdlib.h>
#include "dsp_bios_cfg.h"

/* The file dsk6713.h must be included in every program that uses the BSL.  This 
   example also includes dsk6713_aic23.h because it uses the 
   AIC23 codec module (audio interface). */
#include "dsk6713.h"
#include "dsk6713_aic23.h"

// math library (trig functions)
#include <math.h>

/* Some functions to help with Complex algebra and FFT. */
#include "cmplx.h"      
#include "fft_functions.h"  

// Some functions to help with writing/reading the audio ports when using interrupts.
#include <helper_functions_ISR.h>
 
 

/******************************* Global declarations ********************************/

/* Audio port configuration settings: these values set registers in the AIC23 audio 
   interface to configure it. See TI doc SLWS106D 3-3 to 3-10 for more info. */
DSK6713_AIC23_Config Config = { \
			 /**********************************************************************/
			 /*   REGISTER	            FUNCTION			      SETTINGS         */ 
			 /**********************************************************************/\
    0x0017,  /* 0 LEFTINVOL  Left line input channel volume  0dB                   */\
    0x0017,  /* 1 RIGHTINVOL Right line input channel volume 0dB                   */\
    0x01f9,  /* 2 LEFTHPVOL  Left channel headphone volume   0dB                   */\
    0x01f9,  /* 3 RIGHTHPVOL Right channel headphone volume  0dB                   */\
    0x0011,  /* 4 ANAPATH    Analog audio path control       DAC on, Mic boost 20dB*/\
    0x0000,  /* 5 DIGPATH    Digital audio path control      All Filters off       */\
    0x0000,  /* 6 DPOWERDOWN Power down control              All Hardware on       */\
    0x0043,  /* 7 DIGIF      Digital audio interface format  16 bit                */\
    0x008d,  /* 8 SAMPLERATE Sample rate control             8 KHZ                 */\
    0x0001   /* 9 DIGACT     Digital interface activation    On                    */\
			 /**********************************************************************/
};

// Codec handle:- a variable used to identify audio interface  
DSK6713_AIC23_CodecHandle H_Codec;

// PI defined here for use in your code 
#define PI 3.141592653589793

#define BUFLEN 128  /* Frame buffer length must be even for real fft */


/* Pointers to data buffers */                        
float *input;
float *intermediate;
float *output;
volatile int index = 0; 
 

 /******************************* Function prototypes *******************************/
void init_hardware(void);     
void init_HWI(void);   
void ISR_AIC(void);
void init_arrays(void);
void wait_buffer(void); 
                       
/********************************** Main routine ************************************/
void main()
{      

  /* setup arrays */
  init_arrays();  

	/* initialize board and the audio port */
  init_hardware();
	
  /* initialize hardware interrupts */
  init_HWI();

    
  /* loop indefinitely, waiting for interrupts */  					
  while(1) 
  {
  	  	wait_buffer();
  };
  
}

    
    
/********************************** init_hardware() *********************************/  
void init_hardware()
{
    // Initialize the board support library, must be called first 
    DSK6713_init();
    
    // Start the AIC23 codec using the settings defined above in config 
    H_Codec = DSK6713_AIC23_openCodec(0, &Config);

	/* Function below sets the number of bits in word used by MSBSP (serial port) for 
	receives from AIC23 (audio port). We are using a 32 bit packet containing two 
	16 bit numbers hence 32BIT is set for  receive */
	MCBSP_FSETS(RCR1, RWDLEN1, 32BIT);	

	/* Configures interrupt to activate on each consecutive available 32 bits 
	from Audio port hence an interrupt is generated for each L & R sample pair */	
	MCBSP_FSETS(SPCR1, RINTM, FRM);

	/* These commands do the same thing as above but applied to data transfers to 
	the audio port */
	MCBSP_FSETS(XCR1, XWDLEN1, 32BIT);	
	MCBSP_FSETS(SPCR1, XINTM, FRM);	
	

}
/********************************** init_HWI() **************************************/ 
void init_HWI(void)
{
	IRQ_globalDisable();			// Globally disables interrupts
	IRQ_nmiEnable();				// Enables the NMI interrupt (used by the debugger)
	IRQ_map(IRQ_EVT_RINT1,4);		// Maps an event to a physical interrupt
	IRQ_enable(IRQ_EVT_RINT1);		// Enables the event
	IRQ_globalEnable();				// Globally enables interrupts

} 
/************************** Allocate memory for arrays *******************************/        
void init_arrays(void)
{
	input        = (float *) calloc(BUFLEN, sizeof(float)); /* Input array */
    output       = (float *) calloc(BUFLEN, sizeof(float)); /* Output array */
    intermediate = (float *) calloc(BUFLEN, sizeof(float)); /* Array for processing*/
}

/*************************** INTERRUPT SERVICE ROUTINE  ******************************/

// Map this to the appropriate interrupt in the DSP BIOS

void ISR_AIC(void)
{       
	short sample;
	float scale = 11585;  
	
	sample = mono_read_16Bit();

	/* add new data to input buffer
	   and scale so that 1v ~= 1.0 */
	input[index] = ((float)sample)/scale;
		
	/* write new output data */
	mono_write_16Bit((short)(output[index]*scale)); 
	
	/* update index and check for full buffer */
	if (++index == BUFLEN)
		index=0;		
}

/******************* Wait for buffer of data to be input/output **********************/
void wait_buffer(void)
{
	float *p;  

	/* wait for array index to be set to zero by ISR */
	while(index);
	
	/* rotate data arrays */
	p = input;
	input = output;
	output = intermediate;
	intermediate = p;
	
	/************************* DO PROCESSING OF FRAME  HERE **************************/                


								/*please add your code */	    


	/**********************************************************************************/
	
	/* wait here in case next sample has not yet been read in */                          
	while(!index);
}        



