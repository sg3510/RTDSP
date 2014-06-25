/*************************************************************************************
			       DEPARTMENT OF ELECTRICAL AND ELECTRONIC ENGINEERING
					   		     IMPERIAL COLLEGE LONDON 

 				      EE 3.19: Real Time Digital Signal Processing
					       Dr Paul Mitcheson and Daniel Harvey

				        LAB 1: Getting Started with the TI C6x DSP 

 				          ********* V O L U M E . C **********

             Part of the volume example. Demonstrates connecting the DSK 
         			    to sinewave data stored on the host PC.
				    Sinewave is then processed by increasing its gain.
		      Additionally demonstrates calling an assembly routine from C. 
 *************************************************************************************
 				Updated for use on 6713 DSK by Danny Harvey: May-Aug 2006
 ************************************************************************************/
/* "@(#) DSP/BIOS 4.90.270 01-08-04 (bios,dsk6713-c04)" */

/************************************ Includes **************************************/
#include <stdio.h>

#include "volume.h"

/******************************* Global declarations ********************************/
// note: for see volume.h for initialisaton of BUFSIZE, MINGAIN and BASELOAD 
// and defintion of PARMS

int inp_buffer[BUFSIZE];       			 // processing data buffers 
int out_buffer[BUFSIZE];
int gain = MINGAIN;                      // volume control variable 
unsigned int processingLoad = BASELOAD;  // processing routine load value 

struct PARMS str =						 // A structure of type PARMS is initialised. 
{										 // This struct is not used in the logic of  
    2934,								 // the program, it is included to demonstrate 
    9432,								 // how to use watch windows on structs.
    213,								
    9432,
    &str
};

/******************************* Function prototypes ********************************/

extern void load(unsigned int loadValue);
static int processing(int *input, int *output);
static void dataIO(void);

/********************************** Main routine ************************************/
void main()
{
    int *input = &inp_buffer[0];		// use pointers to point to first element in
    int *output = &out_buffer[0];		// input and output buffers

    puts("volume example started\n");	// send a message to stdio 

    // loop forever 
    while(TRUE)
    {        
         //  Read input data using a probe-point connected to a host file. 
         //  Write output data to a graph connected through a probe-point.
         
        dataIO();

        #ifdef FILEIO
        puts("begin processing")        /****** deliberate syntax error ******/
        #endif
        
        // process signal held in input buffer array (apply gain)
        // result is returned to output buffer array
        processing(input, output);
    }
}

/*************************** Function Implementations *******************************/

/********************************** Processing() ************************************/
static int processing(int *input, int *output)
{
    int size = BUFSIZE;

	/* loop through length of input array mutiplying by gain. Put the result in 
	 the output array. */
    while(size--){
        *output++ = *input++ * gain;
    }
        
    // apply additional processing load by calling assembly function load()
    load(processingLoad);
    
    return(TRUE);
}


/************************************ dataIO() **************************************/
static void dataIO()
{
    /* This function does nothing but return. Is is used so a probepoint can be
    inserted to do data I/O */

    return;
}

