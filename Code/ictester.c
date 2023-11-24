/*******************************************************
This program was created by the
CodeWizardAVR V3.12 Advanced
Automatic Program Generator
© Copyright 1998-2014 Pavel Haiduc, HP InfoTech s.r.l.
http://www.hpinfotech.com

Project :   analog_pulse Labratoary Ic tester
Version :     0.8
Date    : 16-Jan-2017
Author  :  masoud-babaabasi
Company : 
Comments: 

Chip type               : ATmega32
Program type            : Application
AVR Core Clock frequency: 8.000000 MHz
Memory model            : Small
External RAM size       : 0
Data Stack size         : 256
*******************************************************/
#include <mega32.h>
#include <alcd.h>
#include <delay.h>
#include <stdio.h>
#include <stdlib.h>
#include "waves.h"

/***********************************************************************************/
#define XR_8038 3
#define CD_4007 2
#define CA_3083 1
#define CD_4017 0
/***********************************************************************************/
#define CA_3083_IN PORTB.5
#define CA_3083_OUT PINB.4
#define CD_4007_IN PORTB.6
#define CD_4007_OUT PINB.7
/***********************************************************************************/
#define CD_4017_RST PORTB.0
#define CD_4017_CLK PORTB.1
#define CD_4017_EN PORTA.3
#define CD_4017_Q0 PINC.5
#define CD_4017_Q1 PINC.6
#define CD_4017_Q2 PINC.4
#define CD_4017_Q3 PINC.1
#define CD_4017_Q4 PINB.3
#define CD_4017_Q5 PINC.7
#define CD_4017_Q6 PINC.3
#define CD_4017_Q7 PINC.2
#define CD_4017_Q8 PINC.0
#define CD_4017_Q9 PINB.2
/***********************************************************************************/
#define SAMPLE_CNT 600
#define WAVE_SIZE 96
/***********************************************************************************/
#define SIN_WAVE 1
#define TRG_WAVE 2
#define SIN_DEV 1500
#define TRG_DEV 2000
/***********************************************************************************/
#define TIMER2_ENABLE TCCR2 |= (0<<CS22) | (1<<CS21) | (1<<CS20) 
#define TIMER2_DISABLE TCCR2 &= ~((0<<CS22) | (1<<CS21) | (1<<CS20))
/***********************************************************************************/
// Voltage Reference: AVCC pin
#define ADC_VREF_TYPE ((0<<REFS1) | (1<<REFS0) | (1<<ADLAR))
/***********************************************************************************/

unsigned char adc_pin,ic_number;
unsigned char sample[SAMPLE_CNT];
int sample_count;
int is_sampling = 0;
/***********************************************************************************/

// Read the 8 most significant bits
// of the AD conversion result
unsigned char read_adc(unsigned char adc_input)
{
    ADMUX=adc_input | ADC_VREF_TYPE;
    // Delay needed for the stabilization of the ADC input voltage
    delay_us(10);
    // Start the AD conversion
    ADCSRA|=(1<<ADSC);
    // Wait for the AD conversion to complete
    while ((ADCSRA & (1<<ADIF))==0);
    ADCSRA|=(1<<ADIF);
    return ADCH;
}
/***********************************************************************************/
// Timer2 output compare interrupt service routine
interrupt [TIM2_COMP] void timer2_comp_isr(void)
{
    sample[sample_count] = read_adc(adc_pin); 
    sample_count++; 
    
    if(sample_count==SAMPLE_CNT)
    {
        sample_count=0;
        TIMER2_DISABLE;
        is_sampling = 0;
    }
} 
/***********************************************************************************/
// Timer1 output compare A interrupt service routine
interrupt [TIM1_COMPA] void timer1_compa_isr(void)
{
    //CD_4017_CLK = ~CD_4017_CLK;
} 
/***********************************************************************************/
// External Interrupt 0 service routine
interrupt [EXT_INT0] void ext_int0_isr(void)
{
    ic_number++;
    if(ic_number == 4) ic_number = 0;
    switch(ic_number)
    {
       case CD_4017 :{ PORTA.7=1;PORTA.6=0;PORTA.5=0;PORTA.4=0; break;}
       case CA_3083 :{ PORTA.7=0;PORTA.6=1;PORTA.5=0;PORTA.4=0;  break;}
       case CD_4007 :{ PORTA.7=0;PORTA.6=0;PORTA.5=1;PORTA.4=0;  break;}
       case XR_8038 :{ PORTA.7=0;PORTA.6=0;PORTA.5=0;PORTA.4=1;  break;}
    }
}
/***********************************************************************************/
int get_deviation(unsigned char wave[], int start)
{
    int i;
    int dev = 0;

    for(i=0; i<WAVE_SIZE; i++)
        dev += abs(wave[i] - sample[i+start]);

    return dev;
}
/***********************************************************************************/
int start_sample_index()
{
    unsigned char min_sample = 255;
    int i;
    int index;

    for(i=0; i<SAMPLE_CNT - 2*WAVE_SIZE; i++)
    {
        if(sample[i] < min_sample) 
        {
            min_sample = sample[i];
            index = i; 
        }
    }

    return index;
}
/***********************************************************************************/
void ca3083_test()
{
    int is_ok1 = 0, is_ok2 = 0;
    
    CA_3083_IN = 1;
    delay_ms(50);
    if(CA_3083_OUT == 0)   
        is_ok1 = 1;
    else
        is_ok1 = 0;
    
    CA_3083_IN = 0;
    delay_ms(50);
    if(CA_3083_OUT == 1)
        is_ok2 = 1;
    else
        is_ok2 = 0;
    
    lcd_gotoxy(0, 0);
    if(is_ok1 == 1 && is_ok2 == 1)
        lcd_putsf("CA3083 OK    ");
    else
        lcd_putsf("CA3083 Reject");
}
/***********************************************************************************/
void cd4007_test()
{
    int is_ok1 = 0, is_ok2 = 0;
    
    CD_4007_IN = 1;
    delay_ms(50);
    if(CD_4007_OUT == 0)   
        is_ok1 = 1;
    else
        is_ok1 = 0;
    
    CD_4007_IN = 0;
    delay_ms(50);
    if(CD_4007_OUT == 1)
        is_ok2 = 1;
    else
        is_ok2 = 0;
    
    lcd_gotoxy(0, 0);
    if(is_ok1 == 1 && is_ok2 == 1)
        lcd_putsf("CD4007 OK    ");
    else
        lcd_putsf("CD4007 Reject");
}
/***********************************************************************************/
void xr8038_test()
{
    int i, start_sample, deviation;
    int is_ok1 = 0, is_ok2 = 0;
    
    //sin wave test
    adc_pin = SIN_WAVE;
    TIMER2_ENABLE;
    is_sampling = 1;
    while(is_sampling == 1); 
    start_sample = start_sample_index();
//    printf("start %d\n", start_sample);  
    deviation = get_deviation(sin_wave, start_sample);
//    printf("dev %d\n", deviation); 

    if(deviation < SIN_DEV)
        is_ok1 = 1;
    else
        is_ok1 = 0;
    
    //triangle wave test
    adc_pin = TRG_WAVE;
    TIMER2_ENABLE;
    is_sampling = 1;
    while(is_sampling == 1);    
    start_sample = start_sample_index(); 
//    printf("start %d\n", start_sample);    
    deviation = get_deviation(trg_wave, start_sample);
//    printf("dev %d\n", deviation);

    if(deviation < TRG_DEV)
        is_ok2 = 1;
    else
        is_ok2 = 0;    
    
//    for(i=start_sample; i<start_sample+WAVE_SIZE; i++)
//            printf("%03d ,", sample[i]);
//        printf("\r\n");
//    delay_ms(1000);

    lcd_gotoxy(0, 0);
    if(is_ok1 == 1 && is_ok2 == 1)
        lcd_putsf("XR8038 OK    ");
    else
        lcd_putsf("XR8038 Reject");    
} 
/***********************************************************************************/
void cd4017_test()
{
    int i, clk_num;
    int is_ok = 0;
    int cd4017_output;
    int q[11];
    char strs[11];
    
    
    CD_4017_RST = 1;
    //while(CD_4017_CLK == 0);
    delay_ms(10);
    CD_4017_RST = 0;
    CD_4017_EN = 0;
    CD_4017_CLK=0; 
    delay_ms(10);
    for(i=0;i<20;i++){
        CD_4017_CLK = ~CD_4017_CLK;
        delay_ms(10);
        
        //clk_num = i/2 + 1;
        switch(i)
        {
            case 1:
                {
                q[1]= ((~CD_4017_Q0) & (CD_4017_Q1) & (~CD_4017_Q2) & (~CD_4017_Q3) & (~CD_4017_Q4) & (~CD_4017_Q5) & (~CD_4017_Q6) & (~CD_4017_Q7) &(~CD_4017_Q8) &(~CD_4017_Q9));
//                sprintf(strs,"i=%u , q0=%d",i,q[1]);
//                lcd_gotoxy(0,0);
//                lcd_puts(strs);
                break;
                }
            case 3:
                {
                q[2]= ((~CD_4017_Q0) & (~CD_4017_Q1) & (CD_4017_Q2) & (~CD_4017_Q3) & (~CD_4017_Q4) & (~CD_4017_Q5) & (~CD_4017_Q6) & (~CD_4017_Q7) &(~CD_4017_Q8) &(~CD_4017_Q9));
//                sprintf(strs,"i=%u , q1=%u",i,q[1]);
//                lcd_gotoxy(0,0);
//                lcd_puts(strs);
                break;
                } 
            case 5:
                {
                q[3]= ((~CD_4017_Q0) & (~CD_4017_Q1) & (~CD_4017_Q2) & (CD_4017_Q3) & (~CD_4017_Q4) & (~CD_4017_Q5) & (~CD_4017_Q6) & (~CD_4017_Q7) &(~CD_4017_Q8) &(~CD_4017_Q9));
//                sprintf(strs,"i=%u , q2=%u",i,q[1]);
//                lcd_gotoxy(0,0);
//                lcd_puts(strs);
                break;
                } 
            case 7:
                {
                q[4]= ((~CD_4017_Q0) & (~CD_4017_Q1) & (~CD_4017_Q2) & (~CD_4017_Q3) & (CD_4017_Q4) & (~CD_4017_Q5) & (~CD_4017_Q6) & (~CD_4017_Q7) &(~CD_4017_Q8) &(~CD_4017_Q9));
//                sprintf(strs,"i=%u , q3=%u",i,q[1]);
//                lcd_gotoxy(0,0);
//                lcd_puts(strs);
                break;
                }
            case 9:
                {
                q[5]= ((~CD_4017_Q0) & (~CD_4017_Q1) & (~CD_4017_Q2) & (~CD_4017_Q3) & (~CD_4017_Q4) & (CD_4017_Q5) & (~CD_4017_Q6) & (~CD_4017_Q7) &(~CD_4017_Q8) &(~CD_4017_Q9));
//                sprintf(strs,"i=%u , q4=%u",i,q[1]);
//                lcd_gotoxy(0,0);
//                lcd_puts(strs);
                break;
                }
            case 11:
                {
                q[6]= ((~CD_4017_Q0) & (~CD_4017_Q1) & (~CD_4017_Q2) & (~CD_4017_Q3) & (~CD_4017_Q4) & (~CD_4017_Q5) & (CD_4017_Q6) & (~CD_4017_Q7) &(~CD_4017_Q8) &(~CD_4017_Q9));
//                sprintf(strs,"i=%u , q5=%u",i,q[1]);
//                lcd_gotoxy(0,0);
//                lcd_puts(strs);
                break;
                }
            case 13:
                {
                q[7]= ((~CD_4017_Q0) & (~CD_4017_Q1) & (~CD_4017_Q2) & (~CD_4017_Q3) & (~CD_4017_Q4) & (~CD_4017_Q5) & (~CD_4017_Q6) & (CD_4017_Q7) &(~CD_4017_Q8) &(~CD_4017_Q9));
//                sprintf(strs,"i=%u , q6=%u",i,q[1]);
//                lcd_gotoxy(0,0);
//                lcd_puts(strs);
                break;
                } 
            case 15:
                {
                q[8]= ((~CD_4017_Q0) & (~CD_4017_Q1) & (~CD_4017_Q2) & (~CD_4017_Q3) & (~CD_4017_Q4) & (~CD_4017_Q5) & (~CD_4017_Q6) & (~CD_4017_Q7) &(CD_4017_Q8) &(~CD_4017_Q9));
//                sprintf(strs,"i=%u , q7=%u",i,q[1]);
//                lcd_gotoxy(0,0);
//                lcd_puts(strs);
                break;
                } 
            case 17:
                {
                q[9]= ((~CD_4017_Q0) & (~CD_4017_Q1) & (~CD_4017_Q2) & (~CD_4017_Q3) & (~CD_4017_Q4) & (~CD_4017_Q5) & (~CD_4017_Q6) & (~CD_4017_Q7) &(~CD_4017_Q8) &(CD_4017_Q9));
//                sprintf(strs,"i=%u , q8=%u",i,q[1]);
//                lcd_gotoxy(0,0);
//                lcd_puts(strs);
                break;
                }
            case 19:
                {
                q[10]= ((CD_4017_Q0) & (~CD_4017_Q1) & (~CD_4017_Q2) & (~CD_4017_Q3) & (~CD_4017_Q4) & (~CD_4017_Q5) & (~CD_4017_Q6) & (~CD_4017_Q7) &(~CD_4017_Q8) &(~CD_4017_Q9));
//                sprintf(strs,"i=%u , q9=%u",i,q[1]);
//                lcd_gotoxy(0,0);
//                lcd_puts(strs);
                break;
                }  
                               
        }
    }
    
    lcd_gotoxy(0, 0);
    if(q[1]==1 && q[2]==1 && q[3]==1 && q[4]==1 && q[5]==1 && q[6]==1 && q[7]==1 && q[8]==1 && q[9]==1 && q[10]==1) lcd_puts("CD4017 OK    ");
    else lcd_puts("CD4017 Reject");
    //delay_ms(1000);


}
/***********************************************************************************/

void main(void)
{
    DDRA = (0xf0|0x08); //led pins for incicating Ic under TEST OR 4017_EN
    PORTA = 0x80;//FIRST LED
    
    DDRB = 0x63;
    
    ic_number=0;
    CD_4017_CLK=0;
    lcd_init(16);
//    lcd_puts("test");
//    delay_ms(100);
//    lcd_clear();
    
    // Timer/Counter 2 initialization
    // Clock source: System Clock
    // Clock value: 375.000 kHz
    // Mode: CTC top=OCR2A
    // OC2 output: Disconnected
    // Timer Period: 0.50133 ms
    ASSR=0<<AS2;
    TCCR2=(0<<PWM2) | (0<<COM21) | (0<<COM20) | (1<<CTC2) | (0<<CS22) | (1<<CS21) | (1<<CS20);
    TCNT2=0x00;
    OCR2=0xBB;

    // Timer(s)/Counter(s) Interrupt(s) initialization
    TIMSK=(1<<OCIE2) | (0<<TOIE2) | (0<<TICIE1) | (0<<OCIE1A) | (0<<OCIE1B) | (0<<TOIE1) | (0<<OCIE0) | (0<<TOIE0);


    // ADC initialization
    // ADC Clock frequency: 187.500 kHz
    // ADC Voltage Reference: AREF pin
    // ADC Auto Trigger Source: ADC Stopped
    // Only the 8 most significant bits of
    // the AD conversion result are used
    ADMUX=ADC_VREF_TYPE;
    ADCSRA=(1<<ADEN) | (0<<ADSC) | (0<<ADATE) | (0<<ADIF) | (0<<ADIE) | (1<<ADPS2) | (1<<ADPS1) | (0<<ADPS0);
    SFIOR=(0<<ADTS2) | (0<<ADTS1) | (0<<ADTS0);
    
    
    
    // External Interrupt(s) initialization
    // INT0: On
    // INT0 Mode: Falling Edge
    // INT1: Off
    // INT2: Off
    GICR|=(0<<INT1) | (1<<INT0) | (0<<INT2);
    MCUCR=(0<<ISC11) | (0<<ISC10) | (1<<ISC01) | (0<<ISC00);
    MCUCSR=(0<<ISC2);
    GIFR=(0<<INTF1) | (1<<INTF0) | (0<<INTF2);
    
    // USART initialization
    // Communication Parameters: 8 Data, 1 Stop, No Parity
    // USART Receiver: On
    // USART Transmitter: On
    // USART Mode: Asynchronous
    // USART Baud Rate: 57600
//    UCSRA=(0<<RXC) | (0<<TXC) | (0<<UDRE) | (0<<FE) | (0<<DOR) | (0<<UPE) | (0<<U2X) | (0<<MPCM);
//    UCSRB=(0<<RXCIE) | (0<<TXCIE) | (0<<UDRIE) | (1<<RXEN) | (1<<TXEN) | (0<<UCSZ2) | (0<<RXB8) | (0<<TXB8);
//    UCSRC=(1<<URSEL) | (0<<UMSEL) | (0<<UPM1) | (0<<UPM0) | (0<<USBS) | (1<<UCSZ1) | (1<<UCSZ0) | (0<<UCPOL);
//    UBRRH=0x00;
//    UBRRL=0x0C;

    #asm("sei")

    while(1)
    {
        switch(ic_number)
        {
            case(XR_8038):{
                xr8038_test();        
                break;  }
            case(CD_4007):{
                cd4007_test();    
                break; }
            case(CA_3083): {
                ca3083_test();    
                break;  }
            case(CD_4017): {
                cd4017_test();    
                break; }
        }       
    }
}
