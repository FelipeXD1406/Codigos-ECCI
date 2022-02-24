/*
 * File:   main.c
 * Author: casa
 *
 * Created on 11 de septiembre de 2021, 03:03 PM
 */


#include <xc.h>//libreria
#include <string.h>// Libreria cadena de caracteres  
#include <stdio.h>//libreria

#pragma config FOSC = INTOSC_HS//Configuracion de la Frecuencia interna
#pragma config WDT = OFF//perro guardian
#pragma config LVP = OFF//Voltajes bajos

#define _XTAL_FREQ 8000000//Frecuencia
#define time 10//tiempo

//LCD
#define CD 0x01//limpiador display
#define RH 0x03//Retorno a casa (dont care) vuelve a la posicion inicial
#define EMS 0x06//configuración de entrada ID 1 incremento 0 decrementando izq a der
#define DC 0x0C//control del display(3 bits) 1 cons, on/of  (B parpadeo del cursor) (D es 0 apaga o enciende la panta) (C enciende el cursor)
#define DSr 0x1C//Configurar el display del cursor derecha, 4 primeros constantes  0b0001 11-- R/s=1 desplazamiento der
#define DSl 0x18//Configurar el display del cursor izquierda,   r/l=0 desplazamiento izq
#define FS 0x28//Configurar la interfaz de la longitud del display 3 constantes DL = 0 son 4 bits, DL=1 8bits... se trabaja con 2 lineas y se trabaja 5*8 el cuadro
#define RAW1 0x80//Configurar la direccion donde se envia el dato que se quiere escribir en la lcd
#define RAW2 0xC0//Configurar la direccion donde se envia el dato que se quiere escribir en la lcd
#define E LATE0//E en el puerto RE0
#define RS LATE1//RS en el puerto RE1

//constantes (MOTOR)
#define in1 LATB0
#define in2 LATB1
#define in3 LATB2
#define in4 LATB3  


void settings(void); //funcion configuraciones principales
//LCD
void settingsLCD(unsigned char word); //funcion de configuracion
void writeLCD(unsigned char word); //funcion de escritura
void LCD(unsigned char word); //funcion habilitacion
void Data1LCD(void); //funciones auxiliares
void Data2LCD(void);
void Data3LCD(void);
void Data4LCD(void);

void __interrupt() movimiento(void); //Función de interrupción
void start(void); //Funciones ADC
void start2(void);
void start3(void);
void start4(void);
void adelante(void); //funciones auxiliares
void atras(void);
void derecha(void);
void izquierda(void);
void sensorOFF(void);


char texto[] = {"PWM CCP1:"}, texto3[] = {"PWM CCP2:"}, texto2[20], texto4[20]; //arreglos
int i, x, y;
int digital, a;
float conversion1, conversion2, conversion3, ciclo_util1, ciclo_util2, d, c;

void main(void) {//funcion principal
    settings(); //funcion configuraciones
    while (1) {//bucle principal
        start(); //llama funciones no se utiliza el ADIF
        start2();
        start3();
        start4();

    }
}

void settings(void) {//funcion configuraciones
    OSCCON = 0x72; //configurar la oscilacion interna 7= bit de reposos en 0 y 111= 8MHz - 2= oscilador timer 1

    ADCON0 = 0x01; // Definir que canal esta  Go done = 0
    ADCON1 = 0x0B; // Definir que canales son Digitales-analogos voltajes ref
    ADCON2 = 0x91; // 7 Config derecha, 5-3 son (010) 4 tiempos de adq , 2-0 (001)= Frec osc int/8 =1MHz

    TRISB = 0xF0; //Define (0b11110000)
    LATB = 0xF0; //(0b11110000)
    TRISA = 0x0F; // define (0b00001111)
    LATA = 0x0F;
    TRISD = 0; //salidas
    LATD = 0;
    TRISE = 0;
    LATE = 0;

    //PWM
    PR2 = 0x7C; //registro TMR2 de 8 bits a 1ms
    T2CON = 0x02; //bit 7 dont care, bit 6-3 -> post escala 1:1, bit 2 activa el TMR2ON  Bit 1-0 Pre-scaler 16
    TRISC = 0; //puerto C como salida
    LATC = 0;
    CCPR1L = 0x7D; //ciclo util inicial de 100%
    CCP1CON = 0x0C; //bit 7-6 -> dont care, bit 5-4 -> receptores modo pwm CCPR1L, bit 3-0 -> PWM mode
    CCPR2L = 0x7D; //ciclo util inicial de 100%
    CCP2CON = 0x0C; //bit 7-6 -> dont care, bit 5-4 -> receptores modo pwm CCPR1L, bit 3-0 -> PWM mode (aunque los CCP2MX se apagan)

    //interrupcion
    GIE = 1; //Carpeta Principal (global)
    RBIE = 1; //Interrupt Enable
    RBIF = 0; //Interrupt Flag

    //ADC
    ADCON0bits.GO = 1; //habilita la conversion en el ADCON0

    //LCD
    settingsLCD(0x02); //nibbles (4MSB y 4LSB)
    settingsLCD(EMS); // llama funciones LCD
    settingsLCD(DC);
    settingsLCD(FS);
    settingsLCD(CD);
    Data1LCD();
    Data3LCD();

    TMR2 = 0; //valor inicial
    T2CONbits.TMR2ON = 1; //enciende el TMR2
    
    //comucacion UART
    TRISCbits.RC7 = 1; //RX entrada
    TRISCbits.RC6 = 0; //TX salida
    SPBRG = 0x0C; //registro que permite configurar la velocidad de transmisión de los datos en la comunicación serial(8MHz/64*9600=0C)
    RCSTA = 0x90; //recepcion de datos(trabaja el puerto serial,8 bits,[dont care],asincrono=1, sin direccioniento, 2-1-0 de lectura)
    TXSTA = 0x20; //transmision de datos(habilita la transmision)

    //interrupcion externa
    GIE = 1; //interrupcion general
    PEIE = 1; //interrupciones perifericas
    RCIE = 1; //habilita la interrupcion de recepcion 
    RCIF = 0; //bandera de recepcion(1=lleno/ se limpia automatico cuando se lee)
}

void __interrupt() UART_Rx(void) {//bandera
    if (RCIF == 1) {//bandera/ ocurrio algo
        a = RCREG; //x es = al registro de recepcion guardando bits del modulo EUSART
        if (a == '1') {//cuando a es 1
           
        } else if (a == '0') {//cuando a es 0
            adelante();
        } else if (a == 'P') {//cuando a es P
            atras();
        } else if (a == 'W') {//cuando a es W
            derecha();
        } else if (a == 'S') {//cuando a es S
            izquierda();
        } else if (a == 'D') {//cuando a es S
            sensorOFF();
        } 
    }
}
void adelante(void) {//Funciones de motor
    in1 = 1;
    in2 = 0;
    in3 = 1;
    in4 = 0;
}

void atras(void) {
    in1 = 0;
    in2 = 1;
    in3 = 0;
    in4 = 1;
}

void derecha(void) {
    in1 = 1;
    in2 = 0;
    in3 = 0;
    in4 = 0;
}

void izquierda(void) {
    in1 = 0;
    in2 = 0;
    in3 = 1;
    in4 = 0;
}

void sensorOFF(void) {
    in1 = 0;
    in2 = 0;
    in3 = 0;
    in4 = 0;
}

