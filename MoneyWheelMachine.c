#pragma config OSC = INTIO2
#pragma config WDT=OFF
#pragma config LVP=OFF
#pragma config BOR =OFF

#include <p18f4321.h>
#include <stdio.h>
#include <math.h>
#include <usart.h>
#include <stdlib.h>
#include <string.h>

#define sw0 PORTDbits.RD6
#define sw1 PORTDbits.RD7
#define button PORTAbits.RA7

void Init_ADC(void);
void init_UART(void);
void ramp_up(unsigned int);
void ramp_down(unsigned int);
void constant(void);
void LED_shift(int);
int read_switch(void);
void get_np(void);
void do_wheel(void);
void do_move_led(void);
char check_color(void);
void display_led(char);
char get_input(void);
void T1ISR(void);
void interrupt high_priority CHKISR(void);
void serialRX_ISR(void);
void blink_led(void);

unsigned int k = 10000;
unsigned char np, op = 0;
char rx_char, rx_flag, pb_flag, balance, bet;

void init_UART()                                //routine to enable the serial port
{ 
    OpenUSART(USART_TX_INT_OFF & USART_RX_INT_OFF & 
USART_ASYNCH_MODE & USART_EIGHT_BIT & USART_CONT_RX & 
USART_BRGH_HIGH, 25); 
     	OSCCON = 0x60; 
} 
void putch (char c)
{   
    while (!TRMT);       
    TXREG = c;
}
void Init_ADC()
{
    TRISA = 0x80;                // set port A as a output port except bit 7
    TRISB = 0x00;                // ser port B as a output port
    TRISC = 0x00;                // set port C as a output port
    TRISD = 0xC0;                // set port D bit0-5 as output and bit6-7 as input
    TRISE = 0x00;                // set port E as output port
    ADCON1= 0x0F;                // select pins AN0 through AN7? as digital
    INTCONbits.TMR0IE = 1;      //Enable Timer 0 interrupt
    INTCONbits.TMR0IF = 0;      //Clear Timer 0 Interrupt Flag
    INTCONbits.GIE = 1;         //Enable Global Interrupt
    TMR1H = 0x00;               //Program Timer High byte
    TMR1L = 0x2f;               //Program Timer Low byte
    T1CONbits.TMR1ON = 1;       //turn on timer 
	PIE1bits.RCIE=1;									// enable interrupt receiver
    PIE1bits.TMR1IE=1;
									// interrupt receiver =0 PIR1bits.RCIF=0;	
    INTCONbits.PEIE=1;									// enable all peripheral interrupts
	INTCONbits.GIE=1; 									// enable global interrupt
    unsigned int SEED;
    SEED = TMR1L + TMR1H;
    srand(SEED);
}

main(void)
{
    Init_ADC();
    init_UART();
    balance = 20;
    rx_flag = 0;
    while(1)
    {
        printf("\r\nYour balance is $%d" , balance);
        printf("\r\n0: red $1 \r\n1: green $2 \r\n2: orange $5 \r\n3: cyan $10\r\n");
        printf("\r\nEnter your bet (0 to 3)\r\n");                      // enter a number
        bet = get_input();
        do_wheel();
    }
}

void do_wheel()
{
    char money[4] = {1, 2, 5, 10}; 
    char color;
    if(bet < 0 | bet > 3)
    {
        printf("\r\nERROR: Number entered not within range. \r\n");
        return;
    }    
    else if(balance < money[bet])
    {
        printf("\r\nERROR: Bet is higher than the balance. Please place a different bet equal to or less than the balance.\r\n");
        return;
    }
    else
    {
        printf("\r\nYou are betting on the price of $%d", money[bet]);
        display_led(bet);
        get_np();
        printf("\r\nThe random number generated is: %d\r\n", np);
        color = check_color();
        printf("\r\nPlease wait for the result... \r\nThe winning number is $%d", money[color]);
        printf("\r\nThe color is %d", color);
        printf("\r\nYour bet is %d\r\n", bet);
        do_move_led();
    }
    if(color == bet)
    {
        blink_led();
        LED_shift(op);    
        balance = balance + money[bet];
        printf("\r\nCongratulation! You have won your bet of $%d\r\n", money[bet]);
        printf("\r\nYour new balance is $%d\r\n", balance);
    }
    else
    {
        LED_shift(op);
        balance = balance - money[bet];
        printf("\r\nSorry you didn't win. Your new balance is $%d\r\n", balance);
        if (balance == 0)
        {
            printf("\r\nGame over!\r\n\n\n");
            printf("STARTING OVER\r\n");
            balance = 20;            
        }
    } 
}
char check_color()
{
    int color;
    if(np%2 == 1)       //red
        color = 0;
    else if((np+1)%3 == 0)  //orange
        color = 2;
    else if(np == 4 | np == 16) //cyan
        color = 3;
    else                //green
        color = 1;
    return color;
}
int read_switch()
{
    int i;
    if(sw0==0 && sw1==0)         //red
        i = 0;
    else if(sw0==0 && sw1==1)    //green
        i = 1;
    else if(sw0==1 && sw1==0)    //orange
        i = 2;
    else                        //cyan
        i = 3;
    return i;
}
void constant()     //function to wait half a second
{
    unsigned int i = k;
    for(i; i > 0; i--);
}
void ramp_up(unsigned int n) //to go from 10000 -> 500 in when n = 0-47
{
    k = -n*200 + 10000;
    unsigned int i = k;
    for(i; i > 0; i--);
}
void ramp_down(unsigned int n) //to go from 500 -> 10000 in when n = 0-47
{
    k = n*200 + 600;
    unsigned int i = k;
    for(i; i > 0; i--);
}
void do_move_led()
{
    int n, lim, limit, limits, dif;
    if (np < op)
    {
        dif = op - np;
        limits = op + 72 - dif;    
    }    
    else if (op < np)
    {
        dif = np - op; 
        limits = op + 72 + dif;    
    }
    else
        limits = op + 48;
    
    lim = op + 47;           //circle 2 times
    limit = op + 119;        //circle 4 times

    for(n = op; n < lim; n++)
    {
        LED_shift(n);
        ramp_up(n-op);
    }
    for(n = op; n < limit; n++)
    {
        LED_shift(n);
        constant();
    }
    for(n = op; n < limits; n++)
    {
        LED_shift(n);
        ramp_down(n-op);
    }
    op = np;
}
void LED_shift(int i)
{
    int n,p,b,d;
    PORTA = 0x3F;
    PORTB = 0x3F;
    PORTC = 0x3F;
    PORTD = 0x3F;
    n = i%24;
    p = n/6;
    b = n%6;
    d = (0x01 << b) ^ 0x3f;
    switch(p)
    {
        case 0: PORTA = d; break;
        case 1: PORTB = d; break;
        case 2: PORTC = d; break;
        case 3: PORTD = d; break;
    }  
}  
void display_led(char n)
{
    if (n == 0)
        PORTE = 0x01;
    else if (n == 1)
        PORTE = 0x02;
    else if (n == 2)
        PORTE = 0x03;
    else
        PORTE = 0x04;
}
void blink_led()
{
    unsigned int n;
    for(unsigned int i = 10; i > 0; i--)
    {
        LED_shift(np);
        for(n = 10000; n > 0; n--);
        PORTA = 0x3F;
        PORTB = 0x3F;
        PORTC = 0x3F;
        PORTD = 0x3F; 
        for(n = 10000; n > 0; n--);
    }
}
char get_input(void)										// get input from user
{
	char dip_switch;
    while((rx_flag==0) && (pb_flag == 0))       		// wait for receive char from serial port or push-button
	{
        dip_switch = read_switch();
        display_led(dip_switch);
        if (rx_flag == 1)
        {
            rx_char = rx_char - '0';                    // adjust value to subtract the ascii '0'
        }
        else if(button == 0)
        {
            pb_flag = 1;
            rx_char = dip_switch;
        }
    }
    rx_flag = 0;                                        // clear rx_flag for the next receive char to come in
    pb_flag = 0;                                        // clear pb_flag 													
    return (rx_char);
}
void T1ISR()
{
    PIR1bits.TMR1IF = 0;
    TMR1L=0x55;
    TMR1H=0x55; 

}
void interrupt high_priority CHKISR()
{
    if (PIR1bits.TMR1IF == 1)
        T1ISR();
    if (PIR1bits.RCIF == 1)
        serialRX_ISR();
}
void serialRX_ISR()
{
	rx_char = RCREG;                                    // receiver char 
	rx_flag = 1;	

}
void get_np()
{  
    np = rand()%24;           
}