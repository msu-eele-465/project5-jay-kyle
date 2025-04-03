/* --COPYRIGHT--,BSD_EX
 * Copyright (c) 2016, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *******************************************************************************
 *
 *                       MSP430 CODE EXAMPLE DISCLAIMER
 *
 * MSP430 code examples are self-contained low-level programs that typically
 * demonstrate a single peripheral function or device feature in a highly
 * concise manner. For this the code may rely on the device's power-on default
 * register values and settings such as the clock configuration and care must
 * be taken when combining code from several examples to avoid potential side
 * effects. Also see www.ti.com/grace for a GUI- and www.ti.com/msp430ware
 * for an API functional library-approach to peripheral configuration.
 *
 * --/COPYRIGHT--*/
//******************************************************************************
//  MSP430FR235x Demo - Toggle P1.0 using software
//
//  Description: Toggle P1.0 every 0.1s using software.
//  By default, FR235x select XT1 as FLL reference.
//  If XT1 is present, the PxSEL(XIN & XOUT) needs to configure.
//  If XT1 is absent, switch to select REFO as FLL reference automatically.
//  XT1 is considered to be absent in this example.
//  ACLK = default REFO ~32768Hz, MCLK = SMCLK = default DCODIV ~1MHz.
//
//           MSP430FR2355
//         ---------------
//     /|\|               |
//      | |               |
//      --|RST            |
//        |           P1.0|-->LED
//
//   Cash Hao
//   Texas Instruments Inc.
//   November 2016
//   Built with IAR Embedded Workbench v6.50.0 & Code Composer Studio v6.2.0
//******************************************************************************
#include <msp430.h>
#include <math.h>

int status = 0;                             // tracks locked, unlocked, or unlocking
int locked = 0;                             // status value for locked
int unlocking = 1;                          // status value for unlocking
int unlocked = 2;                           // status value for unlocked

int mode = 0;                               // tracks normal, window, or pattern
int normal = 0;                             // mode value for normal operation
int window_set = 1;                         // mode value for window set operation
int pattern_set = 2;                        // mode value for pattern set operation

int col = 0;                                // variable that marks what columb of the keypad was pressed
int key_pad_flag = 0;
int int_en = 0;                             //stops intterupt from flagging after inputs go high
int pressed = 0;
char key = 'N';                             // starts the program at NA until a key gets pressed

int pattern = 0xFF;                         // tracks pattern that is set

int window_digit1 = -1;
int window_digit2 = -1;

char password_char1 = '5';                  // first digit of password
char password_char2 = '2';                  // second digit of password
char password_char3 = '9';                  // third digit of password
char password_char4 = '3';                  // fourth digit of password
int password_index = 1;                     // tracks which digit is being entered

int LED_Data_Cnt = 0;
int LCD_Data_Cnt = 0;
char LED_Data_Packet[] = {0x00, 0xFF};                      // status, key_num
char LCD_Data_Packet[] = {0x00, 0x00, 0x00, 0x00, 0x00};    // mode, key_num, F/C, temp, n

unsigned int ADC_Value;                     // stores adc value
float voltage;                              // stores adc value converted to a voltage
float temp;                                 // stores temp value in celcius
int temp_type = 0;                          // tracks if celcius or fahrenheit should be displayed
int c = 0;                                  // temp_type value for celcius
int f = 1;                                  // temp_type value for fahrenheit

#define MAX_WINDOW 16 
float temp_buffer[MAX_WINDOW];              // max window size = 10 
int window_size = 3;                        // default window size = 3 
int temp_index = 0;                         // tracks where to insert new temp
int samples_collected = 0;                  // counts how many values have been collected
float temp_sum = 0;
float temp_avg = 0;     

void init_ADC(void) {
    WDTCTL = WDTPW | WDTHOLD;               // Stop watchdog timer           
    PM5CTL0 &= ~LOCKLPM5;                   // Disable High Z mode

    P1SEL1 |= BIT2;                         // Configure P1.2 Pin for A2
    P1SEL0 |= BIT2; 

    ADCCTL0 &= ~ADCSHT;                     // Clear ADCSHT from def. of ADCSHT=01
    ADCCTL0 |= ADCSHT_2;                    // Conversion Cycles = 16 (ADCSHT=10)
    ADCCTL0 |= ADCON;                       // Turn ADC ON
    ADCCTL1 |= ADCSSEL_2;                   // ADC Clock Source = SMCLK
    ADCCTL1 |= ADCSHP;                      // Sample signal source = sampling timer
    ADCCTL2 &= ~ADCRES;                     // Clear ADCRES from def. of ADCRES=01
    ADCCTL2 |= ADCRES_2;                    // Resolution = 12-bit (ADCRES = 10)
    ADCMCTL0 |= ADCINCH_2;                  // ADC Input Channel = A2 (P1.2)
    ADCIE |= ADCIE0;                        // Enable ADC Conv Complete IRQ

    __enable_interrupt();                   // Enable interrupts
}

void init_rgb_led(void) {
    WDTCTL = WDTPW | WDTHOLD;               // Stop watchdog timer           
    PM5CTL0 &= ~LOCKLPM5;                   // Disable High Z mode

    // Set P6.0 (red), P6.1 (green), P6.2 (blue) as outputs for RGB led
    P6DIR |= (BIT0 | BIT1 | BIT2);  
    P6OUT &= ~(BIT0 | BIT1 | BIT2); 

    // Configure Timer B3
    TB3CTL |= (TBSSEL__SMCLK | MC__UP | TBCLR);  // Use SMCLK, up mode, clear
    TB3CCR0 = 16320;                             // Set PWM period (adjust for desired frequency)

    // Enable and clear interrupts for each color channel
    TB3CCTL0 |= CCIE;                            // Interrupt for base period
    TB3CCTL0 &= ~CCIFG; 
    TB3CCTL1 |= CCIE;                            // Interrupt for Red
    TB3CCTL1 &= ~CCIFG; 
    TB3CCTL2 |= CCIE;                            // Interrupt for Green
    TB3CCTL2 &= ~CCIFG;
    TB3CCTL3 |= CCIE;                            // Interrupt for Blue
    TB3CCTL3 &= ~CCIFG;

    __enable_interrupt();                        // enable interrupts
}

void init_keypad(void) {
    WDTCTL = WDTPW | WDTHOLD;                    // Stop watchdog timer           
    PM5CTL0 &= ~LOCKLPM5;                        // Disable High Z mode

    //--set up ports
    P1DIR |=  (BIT4   |   BIT5   |   BIT6   |   BIT7);  // Set keypad row pins as outputs
    P1OUT |=  (BIT4   |   BIT5   |   BIT6   |   BIT7);  // sets output high to start

    // Set column pins as input with pull-down resistor
    P2DIR &= ~(BIT0   |   BIT1   |   BIT2   |   BIT3); // Set P1.4 - p1.7 as input
    P2REN |=  (BIT0   |   BIT1   |   BIT2   |   BIT3); // Enable pull-up/down resistors
    P2OUT &= ~(BIT0   |   BIT1   |   BIT2   |   BIT3); // Set as pull-down
}

void init_i2c_b0(void) {
    WDTCTL = WDTPW | WDTHOLD;               // Stop watchdog timer

    UCB0CTLW0 |= UCSWRST;                   // Put eUSCI_B0 in SW Reset
    UCB0CTLW0 |= UCSSEL__SMCLK;             // Choose BRCLK = SMCLK = 1Mhz
    UCB0BRW = 10;                           // Divide BRCLK by 10 for SCL = 100kHz
    UCB0CTLW0 |= UCMODE_3;                  // Put into I2C mode
    UCB0CTLW0 |= UCMST;                     // Put into master mode
    UCB0CTLW0 |= UCTR;                      // Put into Tx mode
    UCB0I2CSA = 0x0020;                     // Slave address = 0x20
    UCB0CTLW1 |= UCASTP_2;                  // Auto STOP when UCB0TBCNT reached
    UCB0TBCNT = sizeof(LCD_Data_Packet);    // # of bytes in packet

    P1SEL1 &= ~BIT3;                        // P1.3 = SCL
    P1SEL0 |= BIT3;                            
    P1SEL1 &= ~BIT2;                        // P1.2 = SDA
    P1SEL0 |= BIT2;

    PM5CTL0 &= ~LOCKLPM5;                   // Disable low power mode

    UCB0CTLW0 &= ~UCSWRST;                  // Take eUSCI_B0 out of SW Reset

    UCB0IE |= UCTXIE0;                      // Enable I2C Tx0 IR1
    __enable_interrupt();                   // Enable Maskable IRQs
}

void init_i2c_b1(void) {
    WDTCTL = WDTPW | WDTHOLD;               // Stop watchdog timer

    UCB1CTLW0 |= UCSWRST;                   // Put eUSCI_B1 in SW Reset
    UCB1CTLW0 |= UCSSEL__SMCLK;             // Choose BRCLK = SMCLK = 1Mhz
    UCB1BRW = 10;                           // Divide BRCLK by 10 for SCL = 100kHz
    UCB1CTLW0 |= UCMODE_3;                  // Put into I2C mode
    UCB1CTLW0 |= UCMST;                     // Put into master mode
    UCB1CTLW0 |= UCTR;                      // Put into Tx mode
    UCB1I2CSA = 0x0020;                     // Slave address = 0x20
    UCB1CTLW1 |= UCASTP_2;                  // Auto STOP when UCB0TBCNT reached
    UCB1TBCNT = sizeof(LED_Data_Packet);    // # of bytes in packet

    P4SEL1 &= ~BIT7;                        // P4.7 = SCL
    P4SEL0 |= BIT7;                            
    P4SEL1 &= ~BIT6;                        // P4.6 = SDA
    P4SEL0 |= BIT6;

    PM5CTL0 &= ~LOCKLPM5;                   // Disable low power mode

    UCB1CTLW0 &= ~UCSWRST;                  // Take eUSCI_B1 out of SW Reset

    UCB1IE |= UCTXIE0;                      // Enable I2C Tx0 IR1
    __enable_interrupt();                   // Enable Maskable IRQs
}

int main(void) {
    init_rgb_led();
    init_keypad();
    init_i2c_b0();
    init_i2c_b1();
    init_ADC();

    update_rgb_led(locked);                 // start RGB led 
    ADCCTL0 |= ADCENC | ADCSC;              // start ADC

    while(1) {
        pressed = (P2IN & 0b00001111);
        if (pressed > 0 && int_en == 0){
            key_pad_flag = 1;
            int_en = 1;
        }
        if (pressed == 0){
            int_en = 0;
        }
        if(key_pad_flag == 1){
            get_key();
            P1DIR |=  (BIT4 | BIT5 | BIT6 | BIT7); 
            key_pad_flag = 0;               // stops the ISR from prematurly setting keypad flag
        }
    }
}    

void get_key() {
    P1OUT &= ~(BIT5 | BIT6 | BIT7);  // clears outputs to start other than BIT4
    P1OUT |= BIT4; // Activate first row
    get_column(); 

    switch(col) {
        case 1: key = '1'; break;
        case 2: key = '2'; break;
        case 3: key = '3'; break;
        case 4: key = 'A'; break;
        case 0: break;
    }

    P1OUT &= ~BIT4;      
    P1OUT |= BIT5; // Activate second row
    get_column();  
        
    switch(col){
        case 1: key = '4'; break;
        case 2: key = '5'; break;
        case 3: key = '6'; break;
        case 4: key = 'B'; break;
        case 0: break;
    }

    P1OUT &= ~BIT5; 
    P1OUT |= BIT6; // Activate third row
    get_column();  

    switch(col){
        case 1: key = '7'; break;
        case 2: key = '8'; break;
        case 3: key = '9'; break;
        case 4: key = 'C'; break;
        case 0: break;
    }

    P1OUT &= ~BIT6; 
    P1OUT |= BIT7; // Activate forth row
    get_column();  

    switch(col){
        case 1: key = '*'; break;
        case 2: key = '0'; break;
        case 3: key = '#'; break;
        case 4: key = 'D'; break;
        case 0: break;
    }

    P1OUT &= ~BIT7; 
    P1OUT |=  (BIT4   |   BIT5   |   BIT6   |   BIT7);  // sets output high to start
    process_key(key);
}

void get_column() {
    int col_1 = (P2IN & BIT0) ? 1 : 0;
    int col_2 = (P2IN & BIT1) ? 1 : 0;
    int col_3 = (P2IN & BIT2) ? 1 : 0;
    int col_4 = (P2IN & BIT3) ? 1 : 0;

    if (col_1 == 1) {
        col = 1;
    } else if (col_2 == 1) {
        col = 2;
    } else if (col_3 == 1) {
        col = 3;
    } else if (col_4 == 1) {
        col = 4;
    } else {
        col = 0;  // No key pressed
    }
}

void process_key(int key) {
    if (status == locked || status == unlocking) {
        check_password(key);
    } else {
        if (mode == normal) {
            switch (key) {
                case 'A': mode = window_set; i2c_write(); break;
                case 'B': mode = pattern_set; i2c_write(); break;
                case 'C': 
                    status = locked;
                    update_rgb_led(status);
                    mode = normal;
                    i2c_write();
                    break;
            }
        } else if (mode == window) {
            switch (key) {
                case '1': window_size = 1; i2c_write(); mode = normal; break;
                case '2': window_size = 2; i2c_write(); mode = normal; break;
                case '3': window_size = 3; i2c_write(); mode = normal; break;
                case '4': window_size = 4; i2c_write(); mode = normal; break;
                case '5': window_size = 5; i2c_write(); mode = normal; break;
                case '6': window_size = 6; i2c_write(); mode = normal; break;
                case '7': window_size = 7; i2c_write(); mode = normal; break;
                case '8': window_size = 8; i2c_write(); mode = normal; break;
                case '9': window_size = 9; i2c_write(); mode = normal; break;
            }
        } else if (mode == pattern_set) {
            switch (key) {
                case '0': pattern = 0; i2c_write(); mode = normal; break;
                case '1': pattern = 1; i2c_write(); mode = normal; break;
                case '2': pattern = 2; i2c_write(); mode = normal; break;
                case '3': pattern = 3; i2c_write(); mode = normal; break;
                case '4': pattern = 4; i2c_write(); mode = normal; break;
                case '5': pattern = 5; i2c_write(); mode = normal; break;
                case '6': pattern = 6; i2c_write(); mode = normal; break;
                case '7': pattern = 7; i2c_write(); mode = normal; break;
            }
        }
    }
}

void check_password(int key) {
    switch (password_index) {
        case 1:
            if (key == password_char1) {
                status = unlocking;
                update_rgb_led(unlocking);
                password_index = 2;
                i2c_write();
            } else {
                status = locked;
                update_rgb_led(locked);
                password_index = 1;
                i2c_write();
            }
            break;
        case 2:
            if (key == password_char2) {
                password_index = 3;
            } else {
                status = locked;
                update_rgb_led(locked);
                password_index = 1;
                i2c_write();
            }
            break;
        case 3:
            if (key == password_char3) {
                password_index = 4;
            } else {
                status = locked;
                update_rgb_led(locked);
                password_index = 1;
                i2c_write();
            }
            break;
        case 4:
            if (key == password_char4) {
                status = unlocked;
                pattern = -1;
                update_rgb_led(unlocked);
                password_index = 1;
                mode = normal;
                i2c_write();
            } else {
                status = locked;
                update_rgb_led(locked);
                password_index = 1;
                i2c_write();
            }
            break;
    }
}

void update_rgb_led(int status) {
    switch(status) {
        case 0: set_rgb_led_pwm(254,1,1);  break;   // locked = red
        case 1: set_rgb_led_pwm(254,20,1); break;   // unlocking = orange
        case 2: set_rgb_led_pwm(1,1,254);  break;   // unlocked = blue
    }
}

void set_rgb_led_pwm(int red, int green, int blue) {
    TB3CCR1 = red*64;   // Red brightness
    TB3CCR2 = green*64; // Green brightness
    TB3CCR3 = blue*64;  // Blue brightness
}

void i2c_write(void) {
    LED_Data_Packet[0] = status;
    LED_Data_Packet[1] = pattern;
    UCB1CTLW0 |= UCTXSTT;                               // start condition
    __delay_cycles(100);

    LCD_Data_Packet[0] = mode;
    LCD_Data_Packet[1] = pattern;
    LCD_Data_Packet[2] = temp_type;
    LCD_Data_Packet[3] = (int)(temp_avg * 10 + 0.5f);   // ex. 23.12 --> 231
    LCD_Data_Packet[4] = window_size;                 
    UCB0CTLW0 |= UCTXSTT;                               // start condition
    __delay_cycles(100);
}

void moving_average(float new_temp) {
    temp_sum -= temp_buffer[temp_index];            // substract old value at index
    temp_buffer[temp_index] = new_temp;             // add new value and overwrite
    temp_sum += new_temp;
    temp_index = (temp_index + 1) % window_size;    // move to next index

    if (samples_collected < window_size)            // track if buffer has been filled
        samples_collected++;

    if (samples_collected == window_size) {         // calculate average if we have enough values
        temp_avg = temp_sum / window_size;
        if (mode == normal) {
            i2c_write(); 
        }
    }
}

#pragma vector = TIMER3_B0_VECTOR
__interrupt void RGB_Period_ISR(void) {
    P6OUT |= (BIT0 | BIT1 | BIT2);  // Turn ON all LEDs at start of period
    TB3CCTL0 &= ~CCIFG;             // Clear interrupt flag
    ADCCTL0 |= ADCENC | ADCSC;      // restart ADC every 0.5s
}

#pragma vector = TIMER3_B1_VECTOR
__interrupt void RGB_Duty_ISR(void) {
    switch (TB3IV) {
        case 0x02:  // TB3CCR1 - Red
            P6OUT &= ~BIT0; // Turn OFF Red
            TB3CCTL1 &= ~CCIFG; 
            break;
        case 0x04:  // TB3CCR2 - Green
            P6OUT &= ~BIT1; // Turn OFF Green
            TB3CCTL2 &= ~CCIFG;
            break;
        case 0x06:  // TB3CCR3 - Blue
            P6OUT &= ~BIT2; // Turn OFF Blue
            TB3CCTL3 &= ~CCIFG;
            break;        
    }
}

#pragma vector=EUSCI_B0_VECTOR
__interrupt void LCD_I2C_ISR(void){
    if (LCD_Data_Cnt == (sizeof(LCD_Data_Packet) - 1)) {
        UCB0TXBUF = LCD_Data_Packet[LCD_Data_Cnt];
        LCD_Data_Cnt = 0;
    } else {
        UCB0TXBUF = LCD_Data_Packet[LCD_Data_Cnt];
        LCD_Data_Cnt++;
    }
}

#pragma vector=EUSCI_B1_VECTOR
__interrupt void LED_I2C_ISR(void){
    if (LED_Data_Cnt == (sizeof(LED_Data_Packet) - 1)) {
        UCB1TXBUF = LED_Data_Packet[LED_Data_Cnt];
        LED_Data_Cnt = 0;
    } else {
        UCB1TXBUF = LED_Data_Packet[LED_Data_Cnt];
        LED_Data_Cnt++;
    }
}

#pragma vector=ADC_VECTOR
__interrupt void ADC_ISR(void){
    ADC_Value = ADCMEM0;                // read ADC value
    voltage = ADC_Value * 3.3f / 4095.0f;
    temp = -1481.96f + sqrt(2.1962e6f + ((1.8639f - voltage) / 3.88e-6f));  // need to round this value to only have one decimal point (i.e 23.6 not 23.61)
    moving_average(temp);
}