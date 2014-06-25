/*************************************************************************************
DEPARTMENT OF ELECTRICAL AND ELECTRONIC ENGINEERING
IMPERIAL COLLEGE LONDON 

EE 3.19: Real Time Digital Signal Processing
Dr Paul Mitcheson and Daniel Harvey

PROJECT: Frame Processing

********* ENHANCE. C **********
Shell for speech enhancement 

Demonstrates overlap-add frame processing (interrupt driven) on the DSK. 

*************************************************************************************
By Danny Harvey: 21 July 2006
Updated for use on CCS v4 Sept 2010
************************************************************************************/
/*
*     You should modify the code so that a speech enhancement project is built 
*  on top of this template.
*/
/**************************** Pre-processor statements ******************************/
//optimisations  - defined as preprocessors to keep code efficient
//however means optimisations cannot be switched on and off at runtime
#define optim1          0
#define optim2          1
#define optim3          0
#define optim4a   0
#define optim4b   0
#define optim4c   1 //
#define optim4d   0
#define optim5          0  //bad
#define optim6          0 //removes some bg noise better
#define optim6a   0 //calculates SNR and dynamically changes alpha
//  library required when using calloc
#include <stdlib.h>
//  Included so program can make use of DSP/BIOS configuration tool.  
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
#include "lpfcoef.txt"
//include coefs

// Some functions to help with writing/reading the audio ports when using interrupts.
#include <helper_functions_ISR.h>

#define WINCONST 0.85185                  /* 0.46/0.54 for Hamming window */
#define FSAMP 8000.0          /* sample frequency, ensure this matches Config for AIC */
#define FFTLEN 256                              /* fft length = frame length 256/8000 = 32 ms*/
#define NFREQ (1+FFTLEN/2)                /* number of frequency bins from a real FFT */
#define OVERSAMP 4                              /* oversampling ratio (2 or 4) */  
#define FRAMEINC (FFTLEN/OVERSAMP)  /* Frame increment */
#define CIRCBUF (FFTLEN+FRAMEINC)   /* length of I/O buffers */

#define OUTGAIN 4*16000.0                       /* Output gain for DAC */
#define INGAIN  (1/(16000.0))         /* Input gain for ADC  */
// PI defined here for use in your code 
#define PI 3.141592653589793
#define TFRAME FRAMEINC/FSAMP       /* time between calculation of each frame */
//added defines
#define K 0.9//(exp(-(TFRAME/0.04)))

/******************************* Global declarations ********************************/

/* Audio port configuration settings: these values set registers in the AIC23 audio 
interface to configure it. See TI doc SLWS106D 3-3 to 3-10 for more info. */
DSK6713_AIC23_Config Config = { \
      /**********************************************************************/
      /*   REGISTER                 FUNCTION                      SETTINGS         */ 
      /**********************************************************************/\
      0x0017,  /* 0 LEFTINVOL  Left line input channel volume  0dB                   */\
      0x0017,  /* 1 RIGHTINVOL Right line input channel volume 0dB                   */\
      0x01f9,  /* 2 LEFTHPVOL  Left channel headphone volume   0dB                   */\
      0x01f9,  /* 3 RIGHTHPVOL Right channel headphone volume  0dB                   */\
      0x0011,  /* 4 ANAPATH    Analog audio path control       DAC on, Mic boost 20dB*/\
      0x0000,  /* 5 DIGPATH    Digital audio path control      All Filters off       */\
      0x0000,  /* 6 DPOWERDOWN Power down control              All Hardware on       */\
      0x0043,  /* 7 DIGIF      Digital audio interface format  16 bit                */\
      0x008d,  /* 8 SAMPLERATE Sample rate control        8 KHZ-ensure matches FSAMP */\
      0x0001   /* 9 DIGACT     Digital interface activation    On                    */\
      /**********************************************************************/
};

// Codec handle:- a variable used to identify audio interface  
DSK6713_AIC23_CodecHandle H_Codec;

float *inbuffer, *outbuffer;              /* Input/output circular buffers */
float *inframe, *outframe;          /* Input and output frames */
float *inwin, *outwin;              /* Input and output windows */
float ingain, outgain;                    /* ADC and DAC gains */ 
float cpufrac;                                  /* Fraction of CPU time used */
volatile int io_ptr=0;              /* Input/ouput pointer for circular buffers */
volatile int frame_ptr=0;           /* Frame pointer */
//-----------------------------------------------------------------------------
//################################ - Added Variables - ########################
float X[FFTLEN];                          /* Temporary variable storing |X(w)| */
complex C[FFTLEN];                              /* Temporary variable storing X(w) - ie the complex version */
float *M1;                                      /* Buffers used to store past noise estimates */
float *M2;                                      /* Buffers used to store past noise estimates */
float *M3;                                      /* Buffers used to store past noise estimates */
float *M4;                                      /* Buffers used to store past noise estimates */
float N[FFTLEN];                          /* Noise estimate */
float alpha = 4;                          /* alpha coefficient used in noise overestimation */
float lambda = 0.02;                      /* lambda coefficient used in noise overestimation */
int f_count = 0;                          /* frame count */
int f_count_loop = 312;                   /* choose how often to loop back */
#if (((optim1)||(optim2))==1)
float Ptm1[FFTLEN];
#endif
#if ((optim3)==1)
float Ntm1[FFTLEN];                             /* Stores past spectral estimates of noise*/
#endif
#if (optim6a ==1)
float S_Power = 0;                              /* Signal Power estimate in able to calculate SNR */
float N_Power = 0;                              /* Noise Power estimate in able to calculate SNR */
float SNR = 1;                                  /* Calclated/estimated SNR*/
float SNR_tm1 = 1;                              /* Estimated SNR Trail */
float SNR_trail_coef = 0.99;
float SNR_Threshold = 0;                  /* Calclated/estimated SNR*/
float Threshold_coef = 0.22;                  /* Calclated/estimated SNR*/
float SNR_max[4] = {0,0,0,0};       /* Buffer of max SNR */
float SNR_min[4] = {0,0,0,0};       /* Buffer of min SNR */
float VAD_coef = 0.1;                     /* Voice activity detection factor */
float snr_inter_min = 0;
float snr_inter_max = 0;
float snr_inter_range = 0;
//float SNRpast[1000]; 
//int   snr_past_count = 0;
int   VAD_on = 1;
int   snr_index = 0;                      /* index for snr min max buffers */
int   snr_count = 0;                      /* index for snr min max buffers */
#endif
//-----------------------------------------------------------------------------
/******************************* Function prototypes *******************************/
void init_hardware(void);    /* Initialise codec */ 
void init_HWI(void);            /* Initialise hardware interrupts */
void ISR_AIC(void);             /* Interrupt service routine for codec */
void process_frame(void);       /* Frame processing routine */
//##########################################################################
//--------------------------| Added Prototypes |----------------------------
float max(float a, float b);    /* Maximum finding function for floats*/
float min(float a, float b);    /* Minimum finding function for floats*/
//##########################################################################
//---------------------| End of Added Prototypes |--------------------------

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//________________________\\Custom Functions//______________________________ 

// |Max| Finds max for two floats
float max(float a, float b){
      if (a>b)
      {
            return a;
      }else
      {
            return b;
      }
}
// |Min| Finds min for two floats
float min(float a, float b){
      if (a<b)
      {
            return a;
      }else
      {
            return b;
      }
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//_________________________\\End of Custom Functions//______________________

/********************************** Main routine ************************************/
void main()
{      

      int k; // used in various for loops

      /*  Initialise and zero fill arrays */  

      inbuffer    = (float *) calloc(CIRCBUF, sizeof(float));     /* Input array */
      outbuffer   = (float *) calloc(CIRCBUF, sizeof(float));     /* Output array */
      inframe           = (float *) calloc(FFTLEN, sizeof(float));      /* Array for processing*/
      outframe    = (float *) calloc(FFTLEN, sizeof(float));      /* Array for processing*/
      inwin       = (float *) calloc(FFTLEN, sizeof(float));      /* Input window */
      outwin            = (float *) calloc(FFTLEN, sizeof(float));      /* Output window */
      M1                = (float *) calloc(FFTLEN, sizeof(float));      /* Used for storage of past noise spectra */
      M2                = (float *) calloc(FFTLEN, sizeof(float));      /* Used for storage of past noise spectra */
      M3                = (float *) calloc(FFTLEN, sizeof(float));      /* Used for storage of past noise spectra */
      M4                = (float *) calloc(FFTLEN, sizeof(float));      /* Used for storage of past noise spectra */


      /* Initialise board and the audio port */
      init_hardware();

      /* Initialise hardware interrupts */
      init_HWI();    

      /* Initialise algorithm constants */  

      for (k=0;k<FFTLEN;k++)
      {                           
            inwin[k] = sqrt((1.0-WINCONST*cos(PI*(2*k+1)/FFTLEN))/OVERSAMP);
            outwin[k] = inwin[k]; 
            //init custom variables here
            X[k] = 0;
            M1[k] = 0;
            M2[k] = 0;
            M3[k] = 0;
            M4[k] = 0;
            #if (((optim1)||(optim2))==1)
            Ptm1[k] = 0;
            #endif
            #if ((optim3)==1)
            Ntm1[k] = 0;
            #endif
            #if (optim6 == 0)
            lpfcoef[k] = 1;
            #endif
            //end of custom added variables
      } 
      ingain=INGAIN;
      outgain=OUTGAIN;        


      /* main loop, wait for interrupt */  
      while(1)    process_frame();
}

/********************************** init_hardware() *********************************/  
void init_hardware()
{
      // Initialise the board support library, must be called first 
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

      /* These commands do the same thing as above but applied to data transfers to the 
      audio port */
      MCBSP_FSETS(XCR1, XWDLEN1, 32BIT);  
      MCBSP_FSETS(SPCR1, XINTM, FRM);     


}
/********************************** init_HWI() **************************************/ 
void init_HWI(void)
{
      IRQ_globalDisable();                // Globally disables interrupts
      IRQ_nmiEnable();                    // Enables the NMI interrupt (used by the debugger)
      IRQ_map(IRQ_EVT_RINT1,4);           // Maps an event to a physical interrupt
      IRQ_enable(IRQ_EVT_RINT1);          // Enables the event
      IRQ_globalEnable();                       // Globally enables interrupts

}

/******************************** process_frame() ***********************************/  
void process_frame(void)
{
      int k, m; 
      int io_ptr0;  
      float *Mptr; 

      /* work out fraction of available CPU time used by algorithm */    
      cpufrac = ((float) (io_ptr & (FRAMEINC - 1)))/FRAMEINC;  

      /* wait until io_ptr is at the start of the current frame */      
      while((io_ptr/FRAMEINC) != frame_ptr); 

      /* then increment the framecount (wrapping if required) */ 
      if (++frame_ptr >= (CIRCBUF/FRAMEINC)) frame_ptr=0;

      /* save a pointer to the position in the I/O buffers (inbuffer/outbuffer) where the 
      data should be read (inbuffer) and saved (outbuffer) for the purpose of processing */
      io_ptr0=frame_ptr * FRAMEINC;

      /* copy input data from inbuffer into inframe (starting from the pointer position) */ 

      m=io_ptr0;
      for (k=0;k<FFTLEN;k++)
      {                           
            inframe[k] = inbuffer[m] * inwin[k]; 
            if (++m >= CIRCBUF) m=0; /* wrap if required */
      } 

      /************************* DO PROCESSING OF FRAME  HERE **************************/
//#####################################################################################
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//|||||||||||||||||||||||||||||||||||||Added Code Here|||||||||||||||||||||||||||||||||   
      //copy values to complex variable to do fft 
      for (k=0;k<FFTLEN;k++)
      {                           
            C[k] = cmplx(inframe[k],0);/* copy input to complex */ 
      }
      //do the fft 
      fft(FFTLEN,C);
      #if optim6a == 1
      snr_count++;
      //reset input power variables to 0
      S_Power = 0;
      if (snr_count >= f_count_loop/3)
      {
            N_Power = 0;
      }
      #endif      
      //now start processing on those values
      for (k=0;k<FFTLEN/2;k++)
      { 
            //init X(w)
            X[k] = cabs(C[k]);
            #if optim6a
            //calculate power
            S_Power += X[k]*X[k];
            #endif
            #if optim1 == 1
            X[k] = X[k]*(1-K)+K*Ptm1[k]; 
            //Ptm1[k] = X[k]; 
            #elif optim2 == 1
            X[k] = (X[k]*X[k])*(1-K)+K*(Ptm1[k]*Ptm1[k]); 
            X[k] = sqrt(X[k]);
            //Ptm1[k] = X[k];
            #endif      
            //M1(w) = min(|X(w)|,M1(w))
            if (M1[k]>X[k]){
                  M1[k] = X[k];
            }
            //N(w) = alpha*min(M1,M2,M3,M4)
            #if optim3 == 1
            N[k] = (1-K)*alpha*lpfcoef[k]*min(min(M1[k],M2[k]),min(M3[k],M4[k]))+K*Ntm1[k];
            Ntm1[k] = N[k];
            #else
            N[k] = alpha*lpfcoef[k]*min(min(M1[k],M2[k]),min(M3[k],M4[k]));
            #endif
            #if (optim6a == 1)
            if (snr_count >= f_count_loop/3)
            {
                  N_Power += N[k]*N[k]/(alpha*alpha*lpfcoef[k]*lpfcoef[k]);
            }
            #endif
            //Y(w) variations
            #if optim4a == 1
            C[k] = rmul(max(lambda*(N[k]/X[k]),1-(N[k]/X[k])),C[k]);
            #elif optim4b == 1
            C[k] = rmul(max(lambda*(Ptm1[k]/X[k]),1-(N[k]/X[k])),C[k]);
            #elif optim4c == 1
            C[k] = rmul(max(lambda*(N[k]/Ptm1[k]),1-(N[k]/Ptm1[k])),C[k]);
            #elif optim4d == 1
            C[k] = rmul(max(lambda,1-(N[k]/Ptm1[k])),C[k]);
            #elif optim5 == 1
            final =sqrt(1-((N[k]*N[k])/(X[k]*X[k])));
            C[k] = rmul(max(lambda,final),C[k]);
            #else
            //Y(w)  = X(w)*max(lambda,g(w))
            C[k] = rmul(max(lambda,1-(N[k]/X[k])),C[k]);
            #endif
            #if ((optim1)||(optim2) == 1)
            Ptm1[k] = X[k];
            #endif 
      } 
      #if (optim6a == 1)
      //calculate SNR
      SNR = 10*log10(S_Power/N_Power);
      SNR_tm1 = SNR_tm1*SNR_trail_coef + (1-SNR_trail_coef)*SNR;
      /*
      if ( (SNR_tm1 <= -100) or (SNR_tm1 >= 100))){
            SNR_tm1 = 0;
      }*/
      if ((SNR_tm1 <= -20) || (SNR_tm1 >= 10)){
            SNR_tm1 = 0;
      }
      SNR_min[snr_index] = min(SNR_tm1,SNR_min[snr_index]);
      SNR_max[snr_index] = max(SNR_tm1,SNR_max[snr_index]);
      if (VAD_on&(SNR_tm1 <= SNR_Threshold)){
            for (k=0;k<FFTLEN/2;k++){
                  C[k]= rmul(VAD_coef,C[k]);//cmplx(0,0);
            }
      }
      if (snr_count >= f_count_loop/3){
        snr_count = 0;
        //SNRpast[snr_past_count++] = SNR;
        //update Threshold
        //SNR_Threshold = min(min(SNR_min[0],SNR_min[1]),min(SNR_min[2],SNR_min[3]))+Threshold_coef*(max(max(SNR_max[0],SNR_max[1]),max(SNR_max[2],SNR_max[3]))-min(min(SNR_min[0],SNR_min[1]),min(SNR_min[2],SNR_min[3])));
        snr_inter_min = min(min(SNR_min[0],SNR_min[1]),min(SNR_min[2],SNR_min[3]));
        snr_inter_max = max(max(SNR_max[0],SNR_max[1]),max(SNR_max[2],SNR_max[3]));
        //snr_inter_min = (3*SNR_min[0]+2*SNR_min[1]+2*SNR_min[2]+SNR_min[3])/8;
        //snr_inter_max = (3*SNR_max[0]+2*SNR_max[1]+2*SNR_max[2]+SNR_max[3])/8;
        snr_inter_range = snr_inter_max - snr_inter_min;

        if (N_Power <= 1)
        {
             Threshold_coef = .05;
        }else
        {
              if (snr_inter_range <= 3)
              {
                  VAD_on = 0;
                  Threshold_coef = 0;
              }else
              {
                  VAD_on = 1;
                   if (snr_inter_range <= 3.5){
                        Threshold_coef = 0.18;
                        VAD_coef = 0.3;
                  }else{
                        VAD_coef = 0.1;
                        Threshold_coef = 0.2;
                  }
                  
              }
        }
        //calculate the minimum threshold for the output to be attenuated
        SNR_Threshold = snr_inter_min+Threshold_coef*snr_inter_range;
        // shift SNR minmax buffers
        snr_index++;
        if (snr_index >= 4){
             snr_index = 0;
        }
        SNR_min[snr_index] = SNR_tm1;
        SNR_max[snr_index] = SNR_tm1;
      }
      #endif
      //shift every arbitrary number of seconds
      if (f_count >= f_count_loop){
            f_count = 0;
            //shift
            Mptr = M4;
            M4 = M3;
            M3 = M2;
            M2 = M1;
            M1 = Mptr;
            //M1(w) = |X(w)|
            for (k=0;k<FFTLEN/2;k++){
                  M1[k] = X[k];
            }
      }
     //increase frame count
      f_count++;
      //perform ifft to get back into time domain
      for (k=FFTLEN/2+1;k<FFTLEN;k++){
            //C[k]=cmplx(0,0) also works for some reason
            C[k]=cmplx(C[FFTLEN-k].r,-1*C[FFTLEN-k].i);
      }
      ifft(FFTLEN,C);
      //write to output
      for (k=0;k<FFTLEN;k++){
            outframe[k] = C[k].r;
      }

//#####################################################################################
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//|||||||||||||||||||||||||||||||||||| End of Section |||||||||||||||||||||||||||||||||   
      /********************************************************************************/

      /* multiply outframe by output window and overlap-add into output buffer */  

      m=io_ptr0;

      for (k=0;k<(FFTLEN-FRAMEINC);k++) 
      {                                                           /* this loop adds into outbuffer */
            outbuffer[m] = outbuffer[m]+outframe[k]*outwin[k];   
            if (++m >= CIRCBUF) m=0; /* wrap if required */
      }         
      for (;k<FFTLEN;k++) 
      {                           
            outbuffer[m] = outframe[k]*outwin[k];   /* this loop over-writes outbuffer */        
            m++;
      }                                        
}        
/*************************** INTERRUPT SERVICE ROUTINE  *****************************/

// Map this to the appropriate interrupt in the CDB file

void ISR_AIC(void)
{       
      short sample;
      /* Read and write the ADC and DAC using inbuffer and outbuffer */

      sample = mono_read_16Bit();
      inbuffer[io_ptr] = ((float)sample)*ingain;
      /* write new output data */
      mono_write_16Bit((int)(outbuffer[io_ptr]*outgain)); 

      /* update io_ptr and check for buffer wraparound */    

      if (++io_ptr >= CIRCBUF) io_ptr=0;
}

/************************************************************************************/
