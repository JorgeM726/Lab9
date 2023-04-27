// CONFIG1
#pragma config FOSC = INTRC_CLKOUT// Oscillator Selection bits (INTOSC oscillator: CLKOUT function on RA6/OSC2/CLKOUT pin, I/O function on RA7/OSC1/CLKIN)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled and can be enabled by SWDTEN bit of the WDTCON register)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config MCLRE = OFF      // RE3/MCLR pin function select bit (RE3/MCLR pin function is digital input, MCLR internally tied to VDD)
#pragma config CP = OFF         // Code Protection bit (Program memory code protection is disabled)
#pragma config CPD = OFF        // Data Code Protection bit (Data memory code protection is disabled)
#pragma config BOREN = OFF      // Brown Out Reset Selection bits (BOR disabled)
#pragma config IESO = OFF       // Internal External Switchover bit (Internal/External Switchover mode is disabled)
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enabled bit (Fail-Safe Clock Monitor is disabled)
#pragma config LVP = OFF        // Low Voltage Programming Enable bit (RB3 pin has digital I/O, HV on MCLR must be used for programming)

// CONFIG2
#pragma config BOR4V = BOR40V   // Brown-out Reset Selection bit (Brown-out Reset set to 4.0V)
#pragma config WRT = OFF        // Flash Program Memory Self Write Enable bits (Write protection off)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#include <xc.h>
#include <stdint.h>

/*------------------------------------------------------------------------------
 * CONSTANTES 
 ------------------------------------------------------------------------------*/

#define _XTAL_FREQ 1000000 

/*------------------------------------------------------------------------------
 * VARIABLES 
 ------------------------------------------------------------------------------*/

uint8_t potVal =0; 
uint8_t adress =0x01; 
int pSleep =0;   

/*------------------------------------------------------------------------------
 * PROTOTIPO DE FUNCIONES 
 ------------------------------------------------------------------------------*/

void setup(void); 
uint8_t EEPROM_read(uint8_t adress); 
void EEPROM_write(uint8_t adress, uint8_t data); 

/*------------------------------------------------------------------------------
 * INTERRUPCIONES 
 ------------------------------------------------------------------------------*/

void __interrupt() isr(void ) {  
    
    //Interrupción de ADC
    if (PIR1bits.ADIF) { 
        if (ADCON0bits.CHS==0) { 
            potVal = ADRESH; //Guardar en variable para poderla enviar a función Write
            PORTC = potVal; //Enviar a puerto C para display
        } 
        PIR1bits.ADIF=0; 
    } 
    //Interrupción PORTB
    if (INTCONbits.RBIF){
        //Regresar de Sleep
        if (PORTBbits.RB1==0){ 
            pSleep =0; 
            PORTEbits.RE0=0; //Apagar bandera de sleep 
        } 
        //Entrar en sleep
        else if (PORTBbits.RB0 ==0){ 
            pSleep =1; 
            PORTEbits.RE0=1; //Encender bandera de sleep
            SLEEP(); 
        } 
        //Guardado de datos
        else if (PORTBbits.RB2==0){ 
            pSleep=0; 
            PORTEbits.RE0=0; //Apagar bandera de sleep
            EEPROM_write(adress, potVal); //Enviar valor de potenciómetro a función de escritura
        } 
        INTCONbits.RBIF=0;
    } 
    return; 
}

/*------------------------------------------------------------------------------
 * CICLO PRINCIPAL
 ------------------------------------------------------------------------------*/
void main(void) { 
    setup(); 
    
    while (1){ 
        if (pSleep ==0){ //Si el PIC no está en sleep, realizar conversión de ADC
            if (ADCON0bits.GO==0) { //Iniciar conversión al terminar la pasada
                ADCON0bits.GO=1; 
                __delay_us(40); 
            }
        } 
        PORTD = EEPROM_read(adress); //Leer de EEPROM
    }
    return;
} 

/*------------------------------------------------------------------------------
 * CONFIGURACION 
 ------------------------------------------------------------------------------*/

void setup(void) {
    ANSEL =0b00000001; 
    ANSELH=0; 
    TRISA=0b00000001; 
    PORTA=0; 
    TRISC =0; 
    PORTC=0; 
    TRISD=0; 
    PORTD=0; 
    TRISE=0; 
    PORTE=0; 
    
    //cofiguracion del osccon
    OSCCONbits.IRCF=0b0100; 
    OSCCONbits.SCS=1; 
    //config de interrupciones 
    INTCONbits.GIE=1; 
    INTCONbits.PEIE=1; 
    INTCONbits.RBIE=1; 
    IOCBbits.IOCB0=1; 
    IOCBbits.IOCB1=1; 
    IOCBbits.IOCB2=1; 
    INTCONbits.RBIF=0;
    PIR1bits.ADIF=0; 
    PIE1bits.ADIE=1; 
    
    //configuracion pushbuttoms en portb 
    TRISBbits.TRISB0=1; 
    TRISBbits.TRISB1=1; 
    TRISBbits.TRISB2=1; 
    OPTION_REGbits.nRBPU=0; 
    WPUBbits.WPUB0=1; 
    WPUBbits.WPUB1=1; 
    WPUBbits.WPUB2=1; 
    //configuraciin del adc  
    ADCON0bits.ADCS=0b01; 
    ADCON1bits.VCFG0=0; 
    ADCON1bits.VCFG1=0; 
    
    ADCON0bits.CHS=0b0000; 
    ADCON1bits.ADFM=0; 
    ADCON0bits.ADON=1; 
    __delay_us(40); 
} 
//Lectura
uint8_t EEPROM_read(uint8_t adress){ 
    
    EEADR == adress; //Registro de dirección
    EECON1bits.EEPGD=0; //Acceso a memoria de datos
    EECON1bits.RD=1; //Read control activado
    return EEDAT; //Registro de memoria de datos
}

//Escritura
void EEPROM_write (uint8_t adress, uint8_t data){ 
    EEADR = adress;  //Registro de dirección
    EEDAT =data;     //Registro de datos
    EECON1bits.EEPGD=0; //Acceso a memoria de datos
    EECON1bits.WREN=1; //Write enable
    INTCONbits.GIE=0; //Apagar interrupciones globales
    EECON2 = 0x55;  //
    EECON2 = 0xAA; 
    EECON1bits.WR=1; //Empieza ciclo de escritura
    EECON1bits.WREN=0; //Apagar ciclo de escritura
    INTCONbits.RBIF=0; //Bajar interrupción de PORTB
    INTCONbits.GIE=1; //Habilitar interrupciones globales
}