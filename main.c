#include "msp430g2553.h"
                                                       // Hardware-related definitions
#define UART_TXD 0x02                                  // TXD on P1.1 (Timer0_A.OUT0)
#define UART_RXD 0x04                                  // RXD on P1.2 (Timer0_A.CCI1A)
#define LED0 BIT0
#define LED1 BIT6
#define triac1 BIT0
#define triac2 BIT1
#define triac3 BIT2
#define zerocross BIT4
 #define potpin INCH_5
 #define AUXin INCH_7

#define UART_TBIT_DIV_2 (1000000 / (9600 * 2))         // Conditions for 9600 Baud SW UART, SMCLK = 1MHz
#define UART_TBIT (1000000 / 9600)
                                                       // Globals for full-duplex UART communication
unsigned int txData;                                   // UART internal variable for TX
unsigned char rxBuffer =1;                                // Received UART character
unsigned int BlueData =1;
unsigned int dimvalue =1;
unsigned int dimloopvalue =1;
long tickvalue =0;
unsigned int adcvalue1 = 0;// variable for storing digital value of CH4 input
unsigned int adcvalue2 = 0;// variable for storing digital value of CH4 input
int loopcount =0;
int adcCount = 990;

void TimerA_UART_tx(unsigned char byte);               // Function prototypes
void TimerA_UART_print(char *string);
void delay(long d);

unsigned int analogRead(unsigned int pin) {
 ADC10CTL0 = ADC10ON + ADC10SHT_2 + SREF_0;
 ADC10CTL1 = ADC10SSEL_0 + pin;
 if(pin==INCH_5){
 ADC10AE0 = 0x20;
 }
 else if(pin==INCH_6){
 ADC10AE0 = 0x40;
 }
 ADC10CTL0 |= ENC + ADC10SC;
 while (1) {
 if (((ADC10CTL0 & ADC10IFG)==ADC10IFG)) {
 ADC10CTL0 &= ~(ADC10IFG +ENC);
 break;
 }
 }
 return ADC10MEM;
 }

void main(void){

  WDTCTL = WDTPW + WDTHOLD;                            // Stop watchdog timer
  DCOCTL = 0x00;                                       // Set DCOCLK to 1MHz
  BCSCTL1 = CAL_BC1_1MHZ;
  DCOCTL = CAL_DCO_1MHZ;
  P1DIR = 0x00;// Initialize all GPIO

  P1SEL = UART_TXD + UART_RXD;                         // Timer function for TXD/RXD pins
  P1DIR |= (UART_TXD + LED0 + LED1);                            // Set all pins input
  P2DIR |= (triac1 + triac2 + triac3);

                                                       // Configures Timer_A for full-duplex UART operation
  TA0CCTL0 = OUT;                                      // Set TXD Idle as Mark = '1'
  TA0CCTL1 = SCS + CM1 + CAP + CCIE;                   // Sync, Neg Edge, Capture, Int
  TA0CTL = TASSEL_2 + MC_2;                            // SMCLK, start in continuous mode
  __enable_interrupt();


  TA1CCTL0 = CCIE;                             // CCR0 interrupt enabled
  TA1CTL = TASSEL_2 + MC_1 + ID_3;           // SMCLK/8, upmode
  TA1CCR0 =  10;                     // 12.5 Hz



 // P1DIR &= ~zerocross;
  P1OUT |= (LED0 + LED1); // set P1.0 to 0 (LED OFF)
  P2OUT |= (triac1 + triac2 + triac3);
  P1IE |= zerocross; // P1.3 interrupt enabled
  P1IES &= ~zerocross; // P1.3 interrupt enabled
  P1IFG &= ~zerocross; // P1.3 IFG cleared

  while(1){
	  // Wait for incoming character
	  adcvalue1 = analogRead( potpin );// Read the analog input from channel 4
	  adcvalue2 = analogRead( AUXin );// Read the analog input from channel 4
	  if ((adcvalue2>480))
	  	  {
		  adcCount = 0;
	  	  }

	  if ((adcCount < 50))
	  {
	  P1OUT |= LED0; //Glow LED2 if channel4 input > channel5 input
	//  dimloopvalue = 2;
if(dimloopvalue>3)
{
	dimloopvalue -= 2;
}
	  }

	  else
	  {
		  P1OUT &= ~LED0; //Glow LED2 if channel4 input > channel5 input
		  if((loopcount % 4) == 0)
		  {
		  dimloopvalue++;
		  if (dimloopvalue >100)
		  {
			  dimloopvalue =100;
		  }
		  }
	  }
    BlueData = rxBuffer;
//dimvalue = BlueData*2;                     // Transmit the received data
    loopcount++;
    if(loopcount >1000)
    {
    	loopcount =0;
    }
    adcCount++;
    if (adcCount >1000)
    {
    	adcCount = 990;
    }
  }
}



void TimerA_UART_tx(unsigned char byte) {              // Outputs one byte using the Timer_A UART

  while (TACCTL0 & CCIE);                              // Ensure last char got TX'd

  TA0CCR0 = TAR;                                       // Current state of TA counter

  TA0CCR0 += UART_TBIT;                                // One bit time till first bit

  TA0CCTL0 = OUTMOD0 + CCIE;                           // Set TXD on EQU0, Int

  txData = byte;                                       // Load global variable

  txData |= 0x100;                                     // Add mark stop bit to TXData

  txData <<= 1;                                        // Add space start bit
}


void TimerA_UART_print(char *string) {                 // Prints a string using the Timer_A UART

  while (*string)
    TimerA_UART_tx(*string++);
}


#pragma vector = TIMER0_A0_VECTOR                      // Timer_A UART - Transmit Interrupt Handler

   __interrupt void Timer_A0_ISR(void) {
  static unsigned char txBitCnt = 10;

  TA0CCR0 += UART_TBIT;                                // Add Offset to CCRx

  if (txBitCnt == 0) {                                 // All bits TXed?

    TA0CCTL0 &= ~CCIE;                                 // All bits TXed, disable interrupt
    TA1CCTL0 |= CCIE;
    P1IE |= zerocross;

    txBitCnt = 10;                                     // Re-load bit counter
  }
  else {
	  P1IE &= ~zerocross;
	  TA1CCTL0 &= ~CCIE;
    if (txData & 0x01)
      TA0CCTL0 &= ~OUTMOD2;                            // TX Mark '1'
    else
      TA0CCTL0 |= OUTMOD2;                             // TX Space '0'
  }
  txData >>= 1;                                        // Shift right 1 bit
  txBitCnt--;
}


#pragma vector = TIMER0_A1_VECTOR                      // Timer_A UART - Receive Interrupt Handler

  __interrupt void Timer_A1_ISR(void) {

  static unsigned char rxBitCnt = 8;

  static unsigned char rxData = 0;

  switch (__even_in_range(TA0IV, TA0IV_TAIFG)) {       // Use calculated branching

    case TA0IV_TACCR1:                                 // TACCR1 CCIFG - UART RX

         TA0CCR1 += UART_TBIT;                         // Add Offset to CCRx

         if (TA0CCTL1 & CAP) {                         // Capture mode = start bit edge

       	  P1IE &= ~zerocross;
       	  TA1CCTL0 &= ~CCIE;

           TA0CCTL1 &= ~CAP;                           // Switch capture to compare mode

           TA0CCR1 += UART_TBIT_DIV_2;                 // Point CCRx to middle of D0
         }
         else {
           rxData >>= 1;

           if (TA0CCTL1 & SCCI)                        // Get bit waiting in receive latch
             rxData |= 0x80;
           rxBitCnt--;

           if (rxBitCnt == 0) {                        // All bits RXed?

             rxBuffer = rxData;                        // Store in global variable

             rxBitCnt = 8;                             // Re-load bit counter

             TA0CCTL1 |= CAP;                          // Switch compare to capture mode
             TA1CCTL0 |= CCIE;
             P1IE |= zerocross;

            // _BIC_SR(LPM0_EXIT);                       // wake up from low power mode.
           }
         }
         break;
   }
}

#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void)
{
dimvalue = dimloopvalue;
tickvalue=0;
P1OUT &= ~(LED1); // set P1.0 to 0 (LED OFF)
P2OUT &= ~(triac1 + triac2 + triac3);
TACTL |= TACLR;
P1IFG &= ~zerocross; // P1.3 IFG cleared
}


#pragma vector = TIMER1_A0_VECTOR
__interrupt void Timer1_A0 (void)
{
                 // Toggle P1.0
	if(tickvalue == dimvalue)
	{
		P1OUT |= (LED1); // set P1.0 to 0 (LED OFF)
		P2OUT |= (triac1 + triac2 + triac3);
	}
	tickvalue++;
}  // Port 1 interrupt service routine




void delay(long d)
{
	int i;
	for (i = 0; i<d; i++)
	{
	}
}
