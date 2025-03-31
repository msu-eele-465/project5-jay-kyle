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

int status = 0;                             // tracks locked, unlocked, or unlocking
int locked = 0;                             // status value for locked
int unlocking = 1;                          // status value for unlocking
int unlocked = 2;                           // status value for unlocked

int col = 0;                                // variable that marks what columb of the keypad was pressed
int key_pad_flag = 0;
int int_en = 0;                             //stops intterupt from flagging after inputs go high
int pressed = 0;
char key = 'N';                             // starts the program at NA until a key gets pressed
int key_num = -1;                            // binary representation of key that was pressed

char password_char1 = '5';                  // first digit of password
char password_char2 = '2';                  // second digit of password
char password_char3 = '9';                  // third digit of password
char password_char4 = '3';                  // fourth digit of password
int password_index = 1;                     // tracks which digit is being entered

float base_transition_period = 1.0;         // stores base transition period for led bar patterns

int LED_Data_Cnt = 0;
int LCD_Data_Cnt = 0;
char Data_Packet[] = {0x00, 0x00, 0x00};      // status, key_num, base_period

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

void i2c_b0_init(void) {
    WDTCTL = WDTPW | WDTHOLD;               // Stop watchdog timer

    UCB0CTLW0 |= UCSWRST;                   // Put eUSCI_B0 in SW Reset
    UCB0CTLW0 |= UCSSEL__SMCLK;             // Choose BRCLK = SMCLK = 1Mhz
    UCB0BRW = 10;                           // Divide BRCLK by 10 for SCL = 100kHz
    UCB0CTLW0 |= UCMODE_3;                  // Put into I2C mode
    UCB0CTLW0 |= UCMST;                     // Put into master mode
    UCB0CTLW0 |= UCTR;                      // Put into Tx mode
    UCB0I2CSA = 0x0020;                     // Slave address = 0x20
    UCB0CTLW1 |= UCASTP_2;                  // Auto STOP when UCB0TBCNT reached
    UCB0TBCNT = sizeof(Data_Packet);         // # of bytes in packet

    P1SEL1 &= ~BIT3;                        // P1.3 = SCL
    P1SEL0 |= BIT3;                            
    P1SEL1 &= ~BIT2;                        // P1.2 = SDA
    P1SEL0 |= BIT2;

    PM5CTL0 &= ~LOCKLPM5;                   // Disable low power mode

    UCB0CTLW0 &= ~UCSWRST;                  // Take eUSCI_B0 out of SW Reset

    UCB0IE |= UCTXIE0;                      // Enable I2C Tx0 IR1
    __enable_interrupt();                   // Enable Maskable IRQs
}

void i2c_b1_init(void) {
    WDTCTL = WDTPW | WDTHOLD;               // Stop watchdog timer

    UCB1CTLW0 |= UCSWRST;                   // Put eUSCI_B1 in SW Reset
    UCB1CTLW0 |= UCSSEL__SMCLK;             // Choose BRCLK = SMCLK = 1Mhz
    UCB1BRW = 10;                           // Divide BRCLK by 10 for SCL = 100kHz
    UCB1CTLW0 |= UCMODE_3;                  // Put into I2C mode
    UCB1CTLW0 |= UCMST;                     // Put into master mode
    UCB1CTLW0 |= UCTR;                      // Put into Tx mode
    UCB1I2CSA = 0x0020;                     // Slave address = 0x20
    UCB1CTLW1 |= UCASTP_2;                  // Auto STOP when UCB0TBCNT reached
    UCB1TBCNT = sizeof(Data_Packet);         // # of bytes in packet

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
    i2c_b0_init();
    i2c_b1_init();
    update_rgb_led(locked);                 // start up RGB led 

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
            key_pad_flag = 0;                                   // stops the ISR from prematurly setting keypad flag
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
        switch (key) {
            case '0': key_num = 0;  break;
            case '1': key_num = 1;  break;
            case '2': key_num = 2;  break;
            case '3': key_num = 3;  break;
            case '4': key_num = 4;  break;
            case '5': key_num = 5;  break;
            case '6': key_num = 6;  break;
            case '7': key_num = 7;  break;
            case '8': key_num = 8;  break;
            case '9': key_num = 9;  break;
            case '*': key_num = 14; break;
            case '#': key_num = 15; break;
            case 'A':
                if (base_transition_period != 0.25) {
                    base_transition_period = base_transition_period - 0.25;
                }
                key_num = 10;
                break;
            case 'B':
                base_transition_period = base_transition_period + 0.25;
                key_num = 11;
                break;
            case 'C':
                key_num = 12;
                break;
            case 'D':
                status = locked;
                update_rgb_led(status);
                key_num = 13;
                break;
        }
    }
    i2c_write();        // led bar --> status, keynum, base period
                        // lcd --> status, keynum, base period
}

void i2c_write(void) {
    Data_Packet[0] = status;
    Data_Packet[1] = key_num;
    Data_Packet[2] = base_transition_period / 0.25;     // scalar for base period of 1.0s
    UCB1CTLW0 |= UCTXSTT;                               // start condition
    __delay_cycles(100);
    UCB0CTLW0 |= UCTXSTT;                               // start condition
    __delay_cycles(100);
}

void check_password(int key) {
    switch (password_index) {
        case 1:
            if (key == password_char1) {
                status = unlocking;
                update_rgb_led(unlocking);
                password_index = 2;
            } else {
                status = locked;
                update_rgb_led(locked);
                password_index = 1;
            }
            break;
        case 2:
            if (key == password_char2) {
                password_index = 3;
            } else {
                status = locked;
                update_rgb_led(locked);
                password_index = 1;
            }
            break;
        case 3:
            if (key == password_char3) {
                password_index = 4;
            } else {
                status = locked;
                update_rgb_led(locked);
                password_index = 1;
            }
            break;
        case 4:
            if (key == password_char4) {
                status = unlocked;
                key_num = -1;
                update_rgb_led(unlocked);
                password_index = 1;
            } else {
                status = locked;
                update_rgb_led(locked);
                password_index = 1;
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

#pragma vector = TIMER3_B0_VECTOR
__interrupt void RGB_Period_ISR(void) {
    P6OUT |= (BIT0 | BIT1 | BIT2);  // Turn ON all LEDs at start of period
    TB3CCTL0 &= ~CCIFG;             // Clear interrupt flag
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
    if (LCD_Data_Cnt == (sizeof(Data_Packet) - 1)) {
        UCB0TXBUF = Data_Packet[LCD_Data_Cnt];
        LCD_Data_Cnt = 0;
    } else {
        UCB0TXBUF = Data_Packet[LCD_Data_Cnt];
        LCD_Data_Cnt++;
    }
}

#pragma vector=EUSCI_B1_VECTOR
__interrupt void LED_I2C_ISR(void){
    if (LED_Data_Cnt == (sizeof(Data_Packet) - 1)) {
        UCB1TXBUF = Data_Packet[LED_Data_Cnt];
        LED_Data_Cnt = 0;
    } else {
        UCB1TXBUF = Data_Packet[LED_Data_Cnt];
        LED_Data_Cnt++;
    }
}