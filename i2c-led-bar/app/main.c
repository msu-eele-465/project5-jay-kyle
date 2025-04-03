/* --COPYRIGHT--,BSD_EX
 * Copyright (c) 2014, Texas Instruments Incorporated
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
//  MSP430FR231x Demo - Toggle P1.0 using software
//
//  Description: Toggle P1.0 every 0.1s using software.
//  By default, FR231x select XT1 as FLL reference.
//  If XT1 is present, the PxSEL(XIN & XOUT) needs to configure.
//  If XT1 is absent, switch to select REFO as FLL reference automatically.
//  XT1 is considered to be absent in this example.
//  ACLK = default REFO ~32768Hz, MCLK = SMCLK = default DCODIV ~1MHz.
//
//           MSP430FR231x
//         ---------------
//     /|\|               |
//      | |               |
//      --|RST            |
//        |           P1.0|-->LED
//
//   Darren Lu
//   Texas Instruments Inc.
//   July 2015
//   Built with IAR Embedded Workbench v6.30 & Code Composer Studio v6.1 
//******************************************************************************
#include <msp430.h>

int status = 0;         // status of the keypad
int unlocked = 2;       
int unlocking = 1;
int locked = 0;

int key_num = -1;
int pattern = -1;     
int current_pattern = -1;   
int next_pattern = -1;    
int pattern1 = 0;                       
int pattern2 = 0;                           
int pattern3 = 0;
int pattern4 = 0;
int pattern5 = 0;
int pattern6 = 0;
int pattern7 = 0;
int pattern3_step = 0;
int pattern6_step = 0;

int Data_Cnt = 0;
int Data_In[] = {0x00, 0x00};

void init_led_bar(void) {
    WDTCTL = WDTPW | WDTHOLD;                    // Stop watchdog timer           
    PM5CTL0 &= ~LOCKLPM5;                        // Disable High Z mode

    // Set P1.0-1.1, P1.4-1.7, P2.6-2.7 as outputs for led bar
    P1DIR |= (BIT0 | BIT1 | BIT4 | BIT5 | BIT6 | BIT7);
    P1OUT &= ~(BIT0 | BIT1 | BIT4 | BIT5 | BIT6 | BIT7);
    P2DIR |= (BIT6 | BIT7);
    P2OUT &= ~(BIT6 | BIT7);

    // Configure Timer B0
    TB0CTL |= (TBSSEL__ACLK | MC__UP | TBCLR);  // Use ACLK, up mode, clear
    TB0CCR0 = 32768;                            // 1s for ACLK (32768Hz)

    // Enable and clear interrupts for each color channel
    TB0CCTL0 |= CCIE;                           // Interrupt for pattern transistions
    TB0CCTL0 &= ~CCIFG;

    __enable_interrupt();                       // enable interrupts
}

void i2c_b0_init(void) {
    WDTCTL = WDTPW | WDTHOLD;                   // Stop watchdog timer

    UCB0CTLW0 |= UCSWRST;                       // Put eUSCI_B0 in SW Reset
    UCB0CTLW0 |= UCMODE_3;                      // Put into I2C mode
    UCB0CTLW0 |= UCSYNC;                        // Synchronous
    UCB0CTLW0 &= ~UCMST;                        // Put into slave mode
    UCB0I2COA0 = 0x0020 | UCOAEN;               // Own address + enable bit
    UCB0CTLW1 &= ~UCASTP_3;                     // Use manual stop detection

    P1SEL1 &= ~BIT3;                            // P1.3 = SCL
    P1SEL0 |= BIT3;                            
    P1SEL1 &= ~BIT2;                            // P1.2 = SDA
    P1SEL0 |= BIT2;

    PM5CTL0 &= ~LOCKLPM5;                       // Disable low power mode

    UCB0CTLW0 &= ~UCSWRST;                      // Take eUSCI_B0 out of SW Reset

    UCB0IE |= UCRXIE0 | UCSTPIE | UCSTTIE;      // Enable I2C Rx0 IR1
    __enable_interrupt();                       // Enable Maskable IRQs
}

void i2c_status_led_init( void) {
    WDTCTL = WDTPW | WDTHOLD;                   // Stop watchdog timer
    PM5CTL0 &= ~LOCKLPM5;                       // Disable High Z mode

    P2DIR |= BIT0;                              // Set P2.0 as output
    P2OUT &= ~BIT0;                             // Clear P2.0

    // Configure Timer B1
    TB1CTL |= (TBSSEL__ACLK | MC__UP | TBCLR);  // Use ACLK, up mode, clear
    TB1CCR0 = 32768;                            // 1s timer

    // Enable and clear interrupts for each color channel
    TB1CCTL0 |= CCIE;                           // Interrupt for pattern transistions
    TB1CCTL0 &= ~CCIFG;

    __enable_interrupt();                       // enable interrupts
}

int main(void) {
    init_led_bar();
    i2c_b0_init();
    i2c_status_led_init();

    while(1) {
        __no_operation();
    }
}

void process_i2c_data(void) {
    status = Data_In[0];
    key_num = Data_In[1];

    if (key_num >= 0 && key_num <= 7) {
        pattern = key_num;
        update_led_bar(status, pattern);
    } else if (key_num == -1) {
        write_8bit_value(0b00000000);
    }
}

void write_8bit_value(int byte) {
    P1OUT &= ~(BIT0 | BIT1 | BIT4 | BIT5 | BIT6 | BIT7);
    P2OUT &= ~(BIT6 | BIT7);

    if (byte & (1 << 0)) P1OUT |= BIT7;  // bit 0 → P1.7
    if (byte & (1 << 1)) P1OUT |= BIT6;  // bit 1 → P1.6
    if (byte & (1 << 2)) P1OUT |= BIT5;  // bit 2 → P1.5
    if (byte & (1 << 3)) P1OUT |= BIT4;  // bit 3 → P1.4
    if (byte & (1 << 4)) P2OUT |= BIT7;  // bit 4 → P2.7
    if (byte & (1 << 5)) P2OUT |= BIT6;  // bit 5 → P2.6
    if (byte & (1 << 6)) P1OUT |= BIT1;  // bit 6 → P1.1
    if (byte & (1 << 7)) P1OUT |= BIT0;  // bit 7 → P1.0
}

void update_led_bar(int status, int pattern) {
    if (status == unlocked) {  
        next_pattern = pattern;
        if (next_pattern == current_pattern) {
            switch (pattern) {
                case 1: pattern1 = 0b01010101; break;
                case 2: pattern2 = 0b11111111; break;
                case 3: pattern3_step = 0;     break;
                case 4: pattern4 = 0b00000000; break;
                case 5: pattern5 = 0b10000000; break;
                case 6: pattern6_step = 0;     break;
                case 7: pattern7 = 0b11111111; break;
            }
        }
        current_pattern = next_pattern;
    } else {
        pattern = -1;                   // reset everything when system gets locked
        current_pattern = -1;
        next_pattern = -1;
        write_8bit_value(0x00);  
    }      
}

#pragma vector = TIMER0_B0_VECTOR
__interrupt void Pattern_Transition_ISR(void) {
    if (status == unlocked) {
        if (pattern == 0) {
            write_8bit_value(0b10101010);
        } else if (pattern == 1) {
            if (pattern1 == 0b01010101) { 
                pattern1 = 0b10101010;       
            } else {
                pattern1 = 0b01010101;
            }
            write_8bit_value(pattern1);
        } else if (pattern == 2) {
            if (pattern2 != 0b11111111) {
                pattern2 = pattern2 + 1;
            } else {
                pattern2 = 0b00000000;
            }
            write_8bit_value(pattern2);
        } else if (pattern == 3) {
            if (pattern3_step == 0) {
                pattern3 = 0b00011000;
                pattern3_step = 1;
            } else if (pattern3_step == 1) {
                pattern3 = 0b00100100;
                pattern3_step = 2;
            } else if (pattern3_step == 2) {
                pattern3 = 0b01000010;
                pattern3_step = 3;
            } else if (pattern3_step == 3) {
                pattern3 = 0b10000001;
                pattern3_step = 4;
            } else if (pattern3_step == 4) {
                pattern3 = 0b01000010;
                pattern3_step = 5;
            } else if (pattern3_step == 5) {
                pattern3 = 0b00100100;
                pattern3_step = 0;
            }
            write_8bit_value(pattern3);
        } else if (pattern == 4) {
            if (pattern4 == 0b00000000) {
                pattern4 = 0b11111111;
            } else {
                pattern4 = pattern4 - 1;
            }
            write_8bit_value(pattern4);
        } else if (pattern == 5) {
            if (pattern5 == 0 || pattern5 == 0b10000000) {
                pattern5 = 0b00000001;
            } else {
                pattern5 = pattern5 * 2;       
            } 
            write_8bit_value(pattern5);
        } else if (pattern == 6) {
            if (pattern6_step == 0) {
                pattern6 = 0b01111111;
                pattern6_step = 1;
            } else if (pattern6_step == 1) {
                pattern6 = 0b10111111;
                pattern6_step = 2;
            } else if (pattern6_step == 2) {
                pattern6 = 0b11011111;
                pattern6_step = 3;
            } else if (pattern6_step == 3) {
                pattern6 = 0b11101111;
                pattern6_step = 4;
            } else if (pattern6_step == 4) {
                pattern6 = 0b11110111;
                pattern6_step = 5;
            } else if (pattern6_step == 5) {
                pattern6 = 0b11111011;
                pattern6_step = 6;
            } else if (pattern6_step == 6) {
                pattern6 = 0b11111101;
                pattern6_step = 7;
            } else if (pattern6_step == 7) {
                pattern6 = 0b11111110;
                pattern6_step = 0;
            }
            write_8bit_value(pattern6);
        } else if (pattern == 7) {
            if (pattern7 != 0b11111111) {
                pattern7 = pattern7 * 2 + 1;       
            } else {
                pattern7 = 0b00000001;
            }
            write_8bit_value(pattern7);
        }
    } else {
        pattern = -1;                   // reset everything when system gets locked
        current_pattern = -1;
        next_pattern = -1;
        write_8bit_value(0b00000000);
    }
    TB0CCTL0 &= ~ CCIFG;            // Clear interrupt flag
}

#pragma vector=EUSCI_B0_VECTOR
__interrupt void LED_I2C_ISR(void){
    switch(__even_in_range(UCB0IV, USCI_I2C_UCBIT9IFG)) {
        case USCI_NONE: break;

        case USCI_I2C_UCSTTIFG:   // START condition
            Data_Cnt = 0;         // Reset buffer index
            break;

        case USCI_I2C_UCRXIFG0:   // Byte received
            if (Data_Cnt < 2) {
                Data_In[Data_Cnt++] = UCB0RXBUF;
            }
            if (Data_Cnt == 2) {
                process_i2c_data(); 
            }
            break;

        case USCI_I2C_UCSTPIFG:     // STOP condition
            UCB0IFG &= ~UCSTPIFG;   // Clear stop flag
            break;

        default: break;
    }
}