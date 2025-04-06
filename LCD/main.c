#include "msp430fr2355.h"
#include <msp430.h> 

    int pattern = 2;
    int cursor_status = 1;
    int blink_status = 1;
    int busy = 0;

    int Data_Cnt = 0;
    int Data_In[] = {0x00, 0x00, 0x00};

    int status = -1;
    int key_num = -1;
    int last_key = -1;
/*
void i2c_b0_init(void) {
    WDTCTL = WDTPW | WDTHOLD;               // Stop watchdog timer

    UCB0CTLW0 |= UCSWRST;                   // Put eUSCI_B0 in SW Reset
    UCB0CTLW0 |= UCMODE_3;                  // Put into I2C mode
    UCB0CTLW0 |= UCSYNC;                    // Synchronous
    UCB0CTLW0 &= ~UCMST;                    // Put into slave mode
    UCB0I2COA0 = 0x0020 | UCOAEN;           // Own address + enable bit
    UCB0CTLW1 &= ~UCASTP_3;                 // Use manual stop detection

    P1SEL1 &= ~BIT3;                        // P1.3 = SCL
    P1SEL0 |= BIT3;
    P1SEL1 &= ~BIT2;                        // P1.2 = SDA
    P1SEL0 |= BIT2;

    PM5CTL0 &= ~LOCKLPM5;                   // Disable low power mode

    UCB0CTLW0 &= ~UCSWRST;                  // Take eUSCI_B0 out of SW Reset

    UCB0IE |= UCRXIE0 | UCSTPIE | UCSTTIE;  // Enable I2C Rx0 IR1
    __enable_interrupt();                   // Enable Maskable IRQs
}
 */
int main(void)
{


    //-- Setup Ports

    //-- four bits controlling display, theese will have to sithc to read at some point
    P6DIR   |= 0b00001111;      
    P6OUT   &= ~0b00001111;

    //-- control bits
    P1DIR   |= 0b00000111;
    P1OUT   &= ~0b00000111;


    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer
    PM5CTL0 &= ~LOCKLPM5;                   // Disable low power mode


    setup();
    while(1){

    clear_display();
    print_pattern(1);
    __delay_cycles(200000);
    clear_display();
    print_pattern(2);
    __delay_cycles(200000);
    clear_display();
    print_pattern(3);
    __delay_cycles(200000);
    clear_display();
    print_pattern(4);
    __delay_cycles(200000);
    clear_display();
    print_pattern(5);
    __delay_cycles(200000);
    clear_display();

    status = 2;
    key_num = 3;
    //base_transition_period = Data_In[2] * 0.25;

    if(key_num != last_key){
        if (key_num != 9 && key_num != 12){
            clear_display();}
        print_pattern(key_num);
        //print_key(key_num);
        
    }
    __delay_cycles(2000);
    last_key = key_num;

    }
    __delay_cycles(2000);
    last_key = key_num;


    
}
void process_i2c_data(void) {
    //status = Data_In[0];
    //key_num = Data_In[1];
    status = 2;
    key_num = 3;
    //base_transition_period = Data_In[2] * 0.25;

    if(key_num != last_key){
        if (key_num != 9 && key_num != 12){
            clear_display();}
        print_pattern(key_num);
        //print_key(key_num);
        
    }
    __delay_cycles(2000);
    last_key = key_num;

}

void print_pattern(int pattern){
    if(pattern == 0){
        return_home();
        byte(0b0111,0b0011,1,0);
        byte(0b0111,0b0100,1,0);
        byte(0b0110,0b0001,1,0);
        byte(0b0111,0b0100,1,0);
        byte(0b0110,0b1001,1,0);
        byte(0b0110,0b0011,1,0);
    }
    else if(pattern == 1){
        return_home();
        byte(0b0111,0b0100,1,0);
        byte(0b0110,0b1111,1,0);
        byte(0b0110,0b0111,1,0);
        byte(0b0110,0b0111,1,0);
        byte(0b0110,0b1100,1,0);
        byte(0b0110,0b0101,1,0);
    }
    else if(pattern == 2){
        return_home();
        byte(0b0111,0b0101,1,0);
        byte(0b0111,0b0000,1,0);
        shift_right();
        byte(0b0110,0b0011,1,0);
        byte(0b0110,0b1111,1,0);
        byte(0b0111,0b0101,1,0);
        byte(0b0110,0b1110,1,0);
        byte(0b0111,0b0100,1,0);
        byte(0b0110,0b0101,1,0);
        byte(0b0111,0b0010,1,0);
    }
    else if(pattern == 3){
        return_home();
        byte(0b0110,0b1001,1,0);
        byte(0b0110,0b1110,1,0);
        shift_right();
        byte(0b0110,0b0001,1,0);
        byte(0b0110,0b1110,1,0);
        byte(0b0110,0b0100,1,0);
        shift_right();
        byte(0b0110,0b1111,1,0);
        byte(0b0111,0b0101,1,0);
        byte(0b0111,0b0100,1,0);

    }
    else if(pattern == 4){
        return_home();
        byte(0b0110,0b0100,1,0);
        byte(0b0110,0b1111,1,0);
        byte(0b0111,0b0111,1,0);
        byte(0b0110,0b1110,1,0);
        shift_right();
        byte(0b0110,0b0011,1,0);
        byte(0b0110,0b1111,1,0);
        byte(0b0111,0b0101,1,0);
        byte(0b0110,0b1110,1,0);
        byte(0b0111,0b0100,1,0);
        byte(0b0110,0b0101,1,0);
        byte(0b0111,0b0010,1,0);
    }
    else if(pattern == 5){
        return_home();
        byte(0b0111,0b0010,1,0);
        byte(0b0110,0b1111,1,0);
        byte(0b0111,0b0100,1,0);
        byte(0b0110,0b0001,1,0);
        byte(0b0111,0b0100,1,0);
        byte(0b0110,0b0101,1,0);
        shift_right();
        byte(0b0011,0b0001,1,0);
        shift_right();
        byte(0b0110,0b1100,1,0);
        byte(0b0110,0b0101,1,0);
        byte(0b0110,0b0110,1,0);
        byte(0b0111,0b0100,1,0);
    }
    else if(pattern == 6){
        return_home();
        byte(0b0111,0b0010,1,0);
        byte(0b0110,0b1111,1,0);
        byte(0b0111,0b0100,1,0);
        byte(0b0110,0b0001,1,0);
        byte(0b0111,0b0100,1,0);
        byte(0b0110,0b0101,1,0);
        shift_right();
        byte(0b0011,0b0111,1,0);
        byte(0b0111,0b0010,1,0);
        byte(0b0110,0b1001,1,0);
        byte(0b0110,0b0111,1,0);
        byte(0b0110,0b1000,1,0);
        byte(0b0111,0b0100,1,0);
    }
    else if(pattern == 7){
        return_home();
        byte(0b0110,0b0110,1,0);
        byte(0b0110,0b1001,1,0);
        byte(0b0110,0b1100,1,0);
        byte(0b0110,0b1100,1,0);
        shift_right();
        byte(0b0110,0b1100,1,0);
        byte(0b0110,0b0101,1,0);
        byte(0b0110,0b0110,1,0);
        byte(0b0111,0b0100,1,0);
    }

}


int shift_right(void){

    byte(0b0010,0b0000,1,0);
}


int byte(int upper, int lower, int RS, int RW){
    check_busy();
    reset_flags(RS,RW);
    __delay_cycles(4);
    P6OUT   &= ~0b1111;
    P6OUT   |= upper;
    P1OUT   |= BIT0;        //Enable pin
    __delay_cycles(4);
    P1OUT   &= ~BIT0;
    __delay_cycles(4);
    //check_busy();
    //reset_flags(RS,RW);
    __delay_cycles(4);
    P6OUT   &= ~0b1111;
    P6OUT   |= lower;
    P1OUT   |= BIT0;
    __delay_cycles(4);
    P1OUT   &= ~BIT0;
    __delay_cycles(20);
    // End DD Ram set key-------------------------------------------
    return 0;
}

void reset_flags(RS, RW){
    if(RS == 1){
        P1OUT   |= BIT2;
    }
    else if (RS == 0) {
        P1OUT  &= ~BIT2;
    }

    if(RW == 1){
        P1OUT   |= BIT1;
    }
    else if (RW == 0) {
        P1OUT &= ~BIT1;
    }
}
int clear_display(void){
    // Clear display
    byte(0b0000, 0b0001, 0, 0);

    // End Clear display -------------------------------------------
}

int return_home(void){
    // Return Home -------------------------------------------
    byte(0b0000, 0b0010, 0, 0);

    // End Return Home -------------------------------------------
}
int setup(void){
    // 4 Bit Mode -----------------------------------------
    //__delay_cycles(4000);
    byte(0b0010, 0b1100, 0, 0);
    //__delay_cycles(200);

    //  End 4 Bit Mode -----------------------------------------

    clear_display();
     //__delay_cycles(200);

    return_home();
    //__delay_cycles(200);
    // Display on -------------------------------------------
    byte(0b0000, 0b1111,0,0);
    //__delay_cycles(200);
    //End Display on  -------------------------------------------
    return 0;
}

void check_busy(void){
    busy = 0;
    P6DIR   &= ~0b1111;
    P6REN   |= 0b1111;
    P6OUT   &= ~0b1111;

    P1OUT   &= ~BIT2;
    P1OUT   |= BIT1;
    __delay_cycles(1000);
    internal_check_busy();
    P6DIR   |= 0b00001111;      
    P6OUT   &= ~0b00001111;
    
    P1OUT   &= ~BIT2;
    P1OUT   &= ~BIT1;
    __delay_cycles(1000);

}
void internal_check_busy(){
    P1OUT   |= BIT0;
    busy    = P6IN;
    busy    &= BIT3;
    __delay_cycles(1000);
    

    P1OUT   &= ~BIT0;
    __delay_cycles(1000);

    P1OUT   |= BIT0;

    __delay_cycles(1000);

    P1OUT   &= ~BIT0;
    __delay_cycles(1000);

    if(busy != 0){
        internal_check_busy();

    }
}

/*#pragma vector=EUSCI_B0_VECTOR
__interrupt void LED_I2C_ISR(void){
    switch(__even_in_range(UCB0IV, USCI_I2C_UCBIT9IFG)) {
        case USCI_NONE: break;

        case USCI_I2C_UCSTTIFG:   // START condition
            Data_Cnt = 0;         // Reset buffer index
            break;

        case USCI_I2C_UCRXIFG0:   // Byte received
            if (Data_Cnt < 3) {
                Data_In[Data_Cnt++] = UCB0RXBUF;
            }
            if (Data_Cnt == 3) {
                process_i2c_data();
            }
            break;

        case USCI_I2C_UCSTPIFG:   // STOP condition
            UCB0IFG &= ~UCSTPIFG;  // Clear stop flag
            break;

        default: break;
    }
}*/