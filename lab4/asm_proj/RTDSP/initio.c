/*************************************************************************************
			       DEPARTMENT OF ELECTRICAL AND ELECTRONIC ENGINEERING
					   		     IMPERIAL COLLEGE LONDON 

 				      EE 3.19: Real Time Digital Signal Processing
					       Dr Paul Mitcheson and Daniel Harvey

				        		  LAB 3: Interrupt I/O

 				            ********* I N T I O. C **********

  Demonstrates inputing and outputing data from the DSK's audio port using interrupts. 

 *************************************************************************************
 				Updated for use on 6713 DSK by Danny Harvey: May-Aug 2006
				Updated for CCS V4 Sept 10
 ************************************************************************************/
/*
 *	You should modify the code so that interrupts are used to service the 
 *  audio port.
 */
/**************************** Pre-processor statements ******************************/

#include <stdlib.h>
#include <stdio.h>
// math library (trig functions)
#include <math.h>
//  Included so program can make use of DSP/BIOS configuration tool.  
#include "dsp_bios_cfg.h"

/* The file dsk6713.h must be included in every program that uses the BSL.  This 
   example also includes dsk6713_aic23.h because it uses the 
   AIC23 codec module (audio interface). */
#include "dsk6713.h"
#include "dsk6713_aic23.h"
//include coefficent variables
#include "coef.txt"

// math library (trig functions)
#include <math.h>

// Some functions to help with writing/reading the audio ports when using interrupts.
#include <helper_functions_ISR.h>
#define BUFFER_BYTE_SIZE 1024
#define BUFFER_SIZE 128
double x_buffer[BUFFER_SIZE];
double * X_PTR = x_buffer;
#pragma DATA_ALIGN(x_buffer, BUFFER_BYTE_SIZE)
int var;
int rectify = 1;
int sampling_freq = 8000;
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
// Holds the value of the current sample 
double output; 
double sample;
#define N 93
double x[2*N];
int i = N-1;
int a=0;
int index = 0;
int index2 = N;
 /******************************* Function prototypes ********************************/
void init_hardware(void);     
void init_HWI(void);  
void ISR_AIC(void);    
double non_circ_FIR();  
double base_circ_FIR();  
double circ_FIR();     
double doublesize_circ_FIR();
extern void circ_FIR_DP(double **ptr, double *coef, double *input_samp, double *filtered_samp ,unsigned int numCoefs);      
/********************************** Main routine ************************************/
void main(){      

 	
	// initialize board and the audio port
  init_hardware();
	
  /* initialize hardware interrupts */
  init_HWI();
  	 		
  /* loop indefinitely, waiting for interrupts */  					
  while(1) 
  {
  };
  
}
        

/********************************** init_hardware() **********************************/  
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

/******************** WRITE YOUR INTERRUPT SERVICE ROUTINE HERE***********************/  
/********************************** ISR_AIC *******************************/
void ISR_AIC(){
	//uncomment to sue assembly code
	/*sample = (double)mono_read_16Bit();
	circ_FIR_DP(&X_PTR,&b[0],&sample,&output, N);
	mono_write_16Bit(output);*/
	mono_write_16Bit(circ_FIR());
	/*
	for (i = N-1;i>0;i--)
	{
		x[i]=x[i-1]; //move data along buffer from lower 
	} // element to next higher 
	x[0] = sample; // put new sample into buffer */
}
/**************************************************************************/
double non_circ_FIR(){
	//this fucntions performs convolution of an input sample with
	//coefficient values and past inputs in a non-circular buffer
	//implementation 
	//reset output, a global variable, to zera
	output = 0;
	x[0] = mono_read_16Bit(); // place new sample into buffer
	//perform convolution by looping through all past samples and mutliple them
	//by their respective coefficients
	for (i=0;i<N;i++){
		//accumulate each multiply result to output
		output += b[i]*x[i];
	}
	//shift all elements such that input value can be placed in zero.
	for (i = N-1;i>0;i--)
	{
		x[i]=x[i-1]; //move data along buffer from lower 
	} // element to next higher 
	return output; //return output value
	
}

/**************************************************************************/
double circ_FIR(){
	x[index] = mono_read_16Bit(); //read input
	output = 0; //reset output
	//check where index is to determine which values will overflow
	if (index <= (N-1)/2){
		//handle the case where index -1 - i may overflow.
		//first loop which handles all values without overflow
		for (i=0;i<index;i++){
			//performs factorized convolution
			output += (x[index-1-i]+x[index + i])*b[i];
		}
		//second loop handling the overflow
		for (i=index;i<(N-1)/2;i++){
			output += (x[index-1+N-i]+x[index + i])*b[i];
		}
		//handles the middle coefficient seperate to avoid doubling
		//a component of the output. This is only necessary for odd
		//N values.
		output += (x[index + (N-1)/2])*b[(N-1)/2];
	}else
	{
		//handle the case where index + i may overflow.
		//first loop
		for (i=0;i<N-index;i++){
			//performs factorized convolution
			output += (x[index-1-i]+x[index + i])*b[i];
		}
		//second loop
		for (i=N-index;i<(N-1)/2;i++){
			//performs factorized convolution
			output += (x[index-1-i]+x[index - N +i])*b[i];
			
		}
		//handle odd case
		output += (x[index-(N-1)/2 - 1])*b[(N-1)/2];
	}
	index--;//decrease index
	if (index<0){index=N-1;}//handle overflow of index
	return output; //return output
}
/**************************************************************************/
double base_circ_FIR(){
	x[index] = mono_read_16Bit(); //store in x[index] the input
	output = 0;					//reset output
	//loop from 0 to N-index which is the point just before
	//which overflow occurs. This avoids an 
	//if function inside a for loop
	for(i = 0;i<N-index;i++){	
		//simply accumulate the convolution result
		output+=x[index+i]*b[i];
	}
	//loop from N-index to N which when
	//overflow occurs. This handles the overflow.
	for(i = N-index;i<N;i++){
		output+=x[index+i-N]*b[i];//simply accumulate result
	}
	index++; //increase index
	if (index==N){index=0;} //check for overflow
	return output; //return value
}
/**************************************************************************/
double doublesize_circ_FIR(){
    //read input and place in memory
    //this is more efficient than reading input twice
    sample = (double)mono_read_16Bit(); 
    x[index] = sample; //store sample in its first index
    x[index2] = sample; //store sample in its second index
    output = 0; //reset output
    //loop from 0 to N-1/2 
    for(i=0;i<(N-1)/2;i++){
    	//use symmetry properties and perform convolution
    	output += b[i]*(x[index+i]+x[index2-i-1]);
    }
    //use for the case where N is odd
    output += b[(N-1)/2]*x[index+(N-1)/2];
    index++; //increase index
    if (index==N){index=0;} //handle overflow
    index2 = index+N; //make index2 follow index
    return output; //return output value.
}
