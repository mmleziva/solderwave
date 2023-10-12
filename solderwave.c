/* 
 * File:   solderwave.c
 * Author: mleziva
 *
 * Created on 12. ?íjna 2023, 16:03
 */

#include <stdio.h>
#include <stdlib.h>
#include <xc.h>
 // outputs
#define CERPADLO    RC0
#define PWM_1kHz    RC1	
#define CHLAZENI    RC3	
#define FLUX        RC5
#define POJEZD      RC6
#define LEDZ        RA3
#define LEDM        RA4
#define LEDR        RA5

//filter inputs
union
{
    uint8_t B;
    struct
    {
      uint8_t STOP: 1;    
      uint8_t CHLAZIN: 1; 
      uint8_t PREHIN: 1; 
      uint8_t EFLUXIN: 1; 
      uint8_t VREADY:1; 
      uint8_t START: 1; 
      uint8_t       : 1; 
      uint8_t RESTART: 1; 
    };
} in;

uint8_t step;
int main(int argc, char** argv)
{
   TRISC=0x80;     //outs enable
   TRISAbits.TRISA3=0;     //LED out enable
   TRISAbits.TRISA4=0;     //LED out enable
   TRISAbits.TRISA5=0;     //LED out enable
   ADCON1bits.ADFM= 1;//right just. ADC
   CCPR2=125;
   CCP2CONbits.CCP2M= 0xf;//PWM RC1
   CCPR1=125;
   CCP1CONbits.CCP1M= 0xf;//PWM RC2
   T2CONbits.T2CKPS= 1;//prescaller=4
   PR2=250;
   T2CONbits.TMR2ON= 1;//start T2
   
   step=0;
   while(1)
   {    
     if(TMR2IF)
     {
       TMR2IF=0;  
       switch (step)
       {
        case 0:
            break;
        default:
            break;
       }
     }
   }
    return (EXIT_SUCCESS);
}

