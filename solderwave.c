/* 
 * File:   solderwave.c
 * Author: mleziva
 *
 * Created on 12. ?�jna 2023, 16:03
 */

#include <stdio.h>
#include <stdlib.h>
#include <xc.h>
#pragma config FOSC = XT
#pragma config LVP = ON
#pragma config PWRTE = OFF
#pragma config BOREN = ON
#pragma config WDTE = ON

#define DelayIni T1CON=0b000000000;
#define delay(x) {TMR1= ~(x); tim1();}
#define DelayUs(x) delay(x)
#define DelayMs(x) delay(1000*x)
void tim1(void)
{TMR1IF=0;	
 TMR1ON=1;
 while(!TMR1IF);
 TMR1ON=0;	
}

 // outputs
#define CERPADLO    RC0
#define PWM_1kHz    RC1	
#define CHLAZENI    RC3	
#define FLUX        RC5
#define POJEZD      RC6
#define LEDZ        RA3
#define LEDM        RA4
#define LEDR        RA5

//analog input channels
#define TPREC        0
#define TCHLC        1
#define TRYCHC       2

#define MULT       (6*(60/4)*(1000/8)/(0x400/(4*8)))
//filtered inputs
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
} ai,  fil, fh,fd;

uint8_t in[8], set, res, film ,aux;

uint8_t step, k, j ;

uint16_t TpreAD,TchlAD;

uint32_t Tmils,Tpre,Tchl;

uint16_t adc_read(unsigned char channel)
{  
 ADCON0 = (channel << 3) | 0xC1;		// !T enable ADC, RC osc.
 DelayUs(8); 
 GO_DONE = 1;
 while(GO_DONE)
 continue;	// wait for conversion complete
 return (((uint16_t)ADRESH)<<8) + ADRESL;
}

int main(int argc, char** argv)
{
   TRISC=0x80;     //outs enable
   OPTION_REGbits.nRBPU=0;// pull up
   TRISAbits.TRISA3=0;     //LED out enable
   TRISAbits.TRISA4=0;     //LED out enable
   TRISAbits.TRISA5=0;     //LED out enable
   ADCON1bits.ADFM= 1;//right just. ADC
   CCPR2=125;
   CCP2CONbits.CCP2M= 0xf;//PWM RC1
   CCPR1=125;
   CCP1CONbits.CCP1M= 0xf;//PWM RC2
   T2CONbits.T2CKPS= 1;//prescaller=4
   PR2=250;   //250*4=1000us
   T2CONbits.TOUTPS=1;//postscalller=2, T2IF 2ms
   T2CONbits.TMR2ON= 1;//start T2
   
  step=0;
   step=52;//t
   
   //ini filters
   ai.B= ~PORTB;
   ai.RESTART= !RC7;
   film= ai.B; //inverted inputs
   for (j=0; j<8; j++)
       {
           in[j]= film;
        }
        fil.B= film;
   fd.B=0;
   fh.B=0;
   
   while(1)
   {    
     CLRWDT();
     if(TMR2IF)//2ms
     {
       TMR2IF=0;  
       k++;
       k %=8;
                //filters
       ai.B= ~PORTB;    //inverted inputs
       ai.RESTART= !RC7;
       in[k]= ai.B;
       set=0xff;
       res=0;
       for (j=0; j<8; j++)
       {
           set &= in[j];
           res |= (in[j]);
       }
       fil.B= ((~film) & set) | (film & (res));
       fd.B= fd.B |(film & (~fil.B) );//fall edge        
       fh.B= fh.B |((~film) & fil.B ); //rise edge 
       film= fil.B;// memory
       
       switch (step)
       {
        case 10://ini
            if(fh.VREADY)//vozik najety
            {
             fh.VREADY=0;
             step=20;
            }
            break;
        case 20: //ready
            if(fil.VREADY & fh.START)
            {
                fh.START=0;
                fd.VREADY=0;
                POJEZD= 1;
                step=30;
                eeprom_write(0,step);
            }
            break;
        case 30: //arrive
            if(fd.VREADY )
            {
                fd.VREADY=0;
                step=40;
                FLUX= 1;
            }
            break;
        case 40: //flux
            if(fh.EFLUXIN )
            {
                fh.EFLUXIN=0;
                FLUX=0;
                step=50;
            }
            break;
        case 50: //go to preheating
            if(fh.PREHIN )
            {
                fh.PREHIN=0;
                POJEZD=0;
                Tmils=0;
                step=52;
            }
            break;
        case 52: //preheating
            TpreAD= adc_read(TPREC);
            Tpre= MULT* (uint32_t)TpreAD;
            if(Tmils >= Tpre)
            {
                POJEZD=1;
                step=60;
            }
            else
                Tmils+=2;
            break;
        case 60: //go to wave
            if(!fil.PREHIN)
            {
                CERPADLO=1;
                step=70;
            }
            break;
        case 70: //wave pump
            if(fil.CHLAZIN)
            {
                CERPADLO=0;
                POJEZD=0;
                CHLAZENI=1;
                Tmils=0;
                step=80;
            }
            break;
        case 80: //cooling
            TchlAD= adc_read(TCHLC);
            Tchl= MULT* (uint32_t)TchlAD;
            if(Tmils >= Tchl)
            {
                CHLAZENI=0;
                step=90;
            }
            else
                Tmils+=2;
            break;
        case 90://finish
            if(!fil.CHLAZIN)//vozik odjety
            {
             step=10;
            }
            break;
        default:
            break;
       }     
     }
   }
    return (EXIT_SUCCESS);
}

