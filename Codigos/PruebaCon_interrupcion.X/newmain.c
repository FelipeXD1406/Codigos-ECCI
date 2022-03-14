/*
 * File:   newmain.c
 * Author: casa
 *
 * Created on 14 de marzo de 2022, 12:40 PM
 */


#include <xc.h>//libreria
#include <string.h>// Libreria cadena de caracteres  
#include <stdio.h>//libreria

#pragma config FOSC = INTOSC_HS//Configuracion de la Frecuencia interna
#pragma config WDT = OFF//perro guardian
#pragma config LVP = OFF//Voltajes bajos

#define _XTAL_FREQ 8000000//Frecuencia
#define time 10//tiempo

//constantes (MOTOR)
#define in1 LATB0
#define in2 LATB1
#define in3 LATB2
#define in4 LATB3  

void settings(void); //funcion configuraciones principales

void __interrupt(high_priority) UART_Rx(void); //Función de interrupción
void start(void); //Funciones ADC
void start1(void);
void start2(void);

void adelante(void); //funciones auxiliares
void atras(void);
void derecha(void);
void izquierda(void);
void freno(void);
void sensorOFF(void);
void infrared(void);
void pwm(void);
void pwmD(void);
void pwmI(void);
void pwmA1(void);
void pwmA2(void);
void pwmB1(void);
void pwmB2(void);
void __interrupt(low_priority) sensor(void);

//modos
void manual(void);
void automatico(void);

int i, x, modo, PWM = 0, mood = 0, z, s;
int y = 1;
int a;
float duty_cicle1, duty_cicle2;
float conversion1, conversion2, conversion3, ciclo_util1, ciclo_util2, d1, d2, c, b;
int n = 0;

void main(void) {//funcion principal
    settings(); //funcion configuraciones
    while (1) {//bucle principal
        //        if (PORTAbits.RA3 == 1) {
        //            LATAbits.LA4 = 0;
        switch (mood) {
            case 1:
                LATAbits.LA5 = 0;
                LATEbits.LATE0 = 1;
                manual();
                break;
            case 2:
                LATAbits.LA5 = 1;
                LATEbits.LATE0 = 0;
                automatico();
                break;
            default:
                mood = 0;
        }
        //} else if (PORTAbits.RA3 == 0) {
        //         sensorOFF();
        //            LATAbits.LA4 = 1;
        //        }
        start(); //llama funciones no se utiliza el ADIF
        start1();
    }
}

void settings(void) {//funcion configuraciones
    OSCCON = 0x72; //configurar la oscilacion interna 7= bit de reposos en 0 y 111= 8MHz - 2= oscilador timer 1

    ADCON0 = 0x01; // Definir que canal esta  Go done = 0
    ADCON1 = 0x0C; // Definir que canales son Digitales-analogos voltajes ref
    ADCON2 = 0x91; // 7 Config derecha, 5-3 son (010) 4 tiempos de adq , 2-0 (001)= Frec osc int/8 =1MHz

    TRISB = 0xF0; //Define (0b11110000)
    LATB = 0xF0; //(0b11110000)
    TRISA = 0x0F;
    LATA = 0x0F;
    TRISE = 0;
    LATE = 0;

    //PWM
    PR2 = 0x7C; //registro TMR2 de 8 bits a 1ms
    T2CON = 0x03; //bit 7 dont care, bit 6-3 -> post escala 1:1, bit 2 activa el TMR2ON  Bit 1-0 Pre-scaler 16
    TRISC = 0; //puerto C como salida
    LATC = 0;
    CCPR1L = 0x7D; //ciclo util inicial de 100%     
    CCP1CON = 0x0C; //bit 7-6 -> dont care, bit 5-4 -> receptores modo pwm CCPR1L, bit 3-0 -> PWM mode
    CCPR2L = 0x7D; //ciclo util inicial de 100%
    CCP2CON = 0x0C; //bit 7-6 -> dont care, bit 5-4 -> receptores modo pwm CCPR1L, bit 3-0 -> PWM mode (aunque los CCP2MX se apagan)

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
    RCIP = 1;

    INT1IE = 1; //Se toma el INT2 y se habilita con IE
    INT1IF = 0; //Es la bandera INT2 0 = no se ha cumplido la condicion
    INTEDG1 = 1; //Seleccion flancos 1=flanco de subida
    INT1IP = 0;

    INT2IE = 1; //Se toma el INT2 y se habilita con IE
    INT2IF = 0; //Es la bandera INT2 0 = no se ha cumplido la condicion
    INTEDG2 = 1; //Seleccion flancos 1=flanco de subida
    INT2IP = 0;

    //ADC
    ADCON0bits.GO = 1; //habilita la conversion en el ADCON0

    TMR2 = 0; //valor inicial
    T2CONbits.TMR2ON = 1; //enciende el TMR2
}

void start(void) {
    ADCON0bits.GO = 1; //habilita la conversion en el ADCON0
    while (GO == 1); //mientras se ejecute el factor de conversion
    if (x == 0) {//nueva condicion dependiendo de la variable anterior
        __delay_ms(time); //tiempo
        a = ADRESH << 8 | ADRESL; //con esta operacion se justifica el bit 7 del ADCON2 (ADFM = todos los 10 bits del conversor a la derecha)
        c = (float) a * (5.0 / 1023.0); //operacion conversora (el valor de a es el valor del conversor, h es un rango y 1023 es el valor maximo del conversor)
        d1 = ((0.286 * (c * c))+(0.602 * c)-(0.1415)); //y = 0,286x2 + 0,602x - 0,1415
        __delay_us(3); // Para comenzar la siguiente adquisición se requiere un tiempo mínimo de 3 TA
        ADCON0 = 0x05; //canal AN1
        x = 1; //"reinicia la condicion"
    }
}

void start1(void) {
    ADCON0bits.GO = 1; //habilita la conversion en el ADCON0
    while (GO == 1); //mientras se ejecute el factor de conversion
    if (x == 1) {//nueva condicion dependiendo de la variable anterior
        __delay_ms(time); //tiempo
        a = ADRESH << 8 | ADRESL; //con esta operacion se justifica el bit 7 del ADCON2 (ADFM = todos los 10 bits del conversor a la derecha)
        c = (float) a * (5.0 / 1023.0); //operacion conversora (el valor de a es el valor del conversor, h es un rango y 1023 es el valor maximo del conversor)
        d2 = ((0.286 * (c * c))+(0.602 * c)-(0.1415)); //y = 0,286x2 + 0,602x - 0,1415
        __delay_us(3); // Para comenzar la siguiente adquisición se requiere un tiempo mínimo de 3 TA
        ADCON0 = 0x09; //canal AN2
        x = 2; //"reinicia la condicion"
    }
}

void start2(void) {
    ADCON0bits.GO = 1; //habilita la conversion en el ADCON0
    while (GO == 1); //mientras se ejecute el factor de conversion
    if (x == 2) {//nueva condicion dependiendo de la variable anterior
        __delay_ms(time); //tiempo
        a = ADRESH << 8 | ADRESL; //con esta operacion se justifica el bit 7 del ADCON2 (ADFM = todos los 10 bits del conversor a la derecha)
        b = (float) a * (5.0 / 1023.0); //operacion conversora (el valor de a es el valor del conversor, h es un rango y 1023 es el valor maximo del conversor)
        __delay_us(3); // Para comenzar la siguiente adquisición se requiere un tiempo mínimo de 3 TAD
    }
}

void __interrupt() UART_Rx(void) {//bandera
    if (RCIF == 1) {//bandera/ ocurrio algo
        a = RCREG; //x es = al registro de recepcion guardando bits del modulo EUSART
        if (a == 'm' || a == 'M') {//cuando a es 0
            mood = 1;
        } else if (a == 'a' || a == 'A') {
            mood = 2;
        } else if (a == 's' || a == 'S') {
            mood = 3;
        } else if (a == '0') {
            modo = 0;
        } else if (a == '1') {
            modo = 1;
        } else if (a == '2') {
            modo = 2;
        } else if (a == '3') {
            modo = 3;
        } else if (a == '4') {
            modo = 4;
        } else if (a == '5') {
            PWM = 1;
        } else if (a == '6') {
            PWM = 2;
        } else if (a == '7') {
            PWM = 3;
        } else if (a == '8') {
            PWM = 4;
        }
    }
}

void __interrupt(low_priority) sensor(void) {
    if (INT1IF) {//habilita la bandera osea cumple condicion
        sensorOFF();
        INT1IF = 0; //apaga la bandera 
    }
    if (INT2IF) {//habilita la bandera osea cumple condicion
        sensorOFF();
        INT2IF = 0; //apaga la bandera 
    }
}

void automatico(void) {
    for (i = 0; i <= 3; i++) {
        //        if (s == 1) {
        start2();
        while (b >= 4.5) {
            start2();
        }
        adelante();
        __delay_ms(2400);
        freno();
        __delay_ms(2000);
        izquierda();
        __delay_ms(800);
        freno();
        __delay_ms(2000);
        adelante();
        __delay_ms(2200);
        freno();
        __delay_ms(2900);
        start2();

        while (b < 4.5) {
            start2();
        }
        izquierda();
        __delay_ms(800);
        freno();
        __delay_ms(2000);
        adelante();
        __delay_ms(1900);
        freno();
        __delay_ms(2000);
        izquierda();
        __delay_ms(800);
        freno();
        __delay_ms(2000);
        adelante();
        __delay_ms(1800);
        freno();
        __delay_ms(2000);
        izquierda();
        __delay_ms(800);
        freno();
        __delay_ms(5000);
        //        } else {
        //        }
    }
    ADCON0 = 0x01; //canal AN0
    x = 0; //"reinicia la condicion"
    freno();
}

void manual(void) {
    if (PORTBbits.RB4 == 1) {
        LATAbits.LA4 = 0;
        if (modo == 0) {
            freno();
        } else if (modo == 1) {
            adelante();
            if (PWM == 1) {
                pwmA1();
                PWM = 0;
            } else if (PWM == 2) {
                pwmA2();
                PWM = 0;
            } else if (PWM == 3) {
                pwmB1();
                PWM = 0;
            } else if (PWM == 4) {
                pwmB2();
                PWM = 0;
            }
        } else if (modo == 2) {
            atras();
            if (PWM == 1) {
                pwmA1();
                PWM = 0;
            } else if (PWM == 2) {
                pwmA2();
                PWM = 0;
            } else if (PWM == 3) {
                pwmB1();
                PWM = 0;
            } else if (PWM == 4) {
                pwmB2();
                PWM = 0;
            }
        } else if (modo == 3) {
            derecha();
            if (PWM == 1) {
                pwmA1();
                PWM = 0;
            } else if (PWM == 2) {
                pwmA2();
                PWM = 0;
            } else if (PWM == 3) {
                pwmB1();
                PWM = 0;
            } else if (PWM == 4) {
                pwmB2();
                PWM = 0;
            }
        } else if (modo == 4) {
            izquierda();
            if (PWM == 1) {
                pwmA1();
                PWM = 0;
            } else if (PWM == 2) {
                pwmA2();
                PWM = 0;
            } else if (PWM == 3) {
                pwmB1();
                PWM = 0;
            } else if (PWM == 4) {
                pwmB2();
                PWM = 0;
            }
        }
    } else {
        sensorOFF();
        LATAbits.LA4 = 1;
    }
}

void pwm(void) {
    if (y == 1) {
        duty_cicle1 = 511.2;
        y = 2;
        y = 3;
    }
    conversion1 = (float) duty_cicle1 * (500.0 / 1023.0);
    CCPR1L = (int) conversion1 >> 2;
    CCP1CON = (CCP1CON & 0x0F) | (((int) conversion1 & 0x03) << 4);
    CCPR2L = (int) conversion1 >> 2;
    CCP2CON = (CCP1CON & 0x0F) | (((int) conversion1 & 0x03) << 4);
}

void pwmI(void) {
    if (y == 2) {
        duty_cicle1 = 190;
        duty_cicle2 = 373.3;
        y = 1;
        y = 3;
    }
    conversion1 = (float) duty_cicle1 * (500.0 / 1023.0);
    CCPR1L = (int) conversion1 >> 2;
    CCP1CON = (CCP1CON & 0x0F) | (((int) conversion1 & 0x03) << 4);
    conversion2 = (float) duty_cicle2 * (500.0 / 1023.0);
    CCPR2L = (int) conversion2 >> 2;
    CCP2CON = (CCP1CON & 0x0F) | (((int) conversion2 & 0x03) << 4);
    if (conversion1 >= conversion2) {
        conversion1 - 1;
    }
}

void pwmD(void) {
    if (y == 3) {
        duty_cicle1 = 373.3;
        duty_cicle2 = 190;
        y = 1;
        y = 2;
    }
    conversion1 = (float) duty_cicle1 * (500.0 / 1023.0);
    CCPR1L = (int) conversion1 >> 2;
    CCP1CON = (CCP1CON & 0x0F) | (((int) conversion1 & 0x03) << 4);
    conversion2 = (float) duty_cicle2 * (500.0 / 1023.0);
    CCPR2L = (int) conversion2 >> 2;
    CCP2CON = (CCP1CON & 0x0F) | (((int) conversion2 & 0x03) << 4);
    if (conversion1 <= conversion2) {
        conversion2 - 102.3;
    }
}

void pwmA1(void) {
    duty_cicle1 = duty_cicle1 + 102.3;
    if (duty_cicle1 > 1023.0) {
        duty_cicle1 = 1023.0;
    }
}

void pwmA2(void) {
    duty_cicle1 = duty_cicle1 - 102.3;
    if (duty_cicle1 < 0) {
        duty_cicle1 = 0;
    }
}

void pwmB1(void) {
    duty_cicle2 = duty_cicle2 + 102.3;
    if (duty_cicle2 > 1023.0) {
        duty_cicle2 = 1023.0;
    }
}

void pwmB2(void) {
    duty_cicle2 = duty_cicle2 - 102.3;
    if (duty_cicle2 < 0) {
        duty_cicle2 = 0;
    }
}

void adelante(void) {//Funciones de motor
    __delay_ms(100);
    pwm();
    in1 = 1;
    in2 = 0;
    in3 = 1;
    in4 = 0;
}

void atras(void) {
    __delay_ms(100);
    pwm();
    in1 = 0;
    in2 = 1;
    in3 = 0;
    in4 = 1;
}

void derecha(void) {
    pwmD();
    in1 = 1;
    in2 = 0;
    in3 = 1;
    in4 = 0;
}

void izquierda(void) {
    pwmI();
    in1 = 1;
    in2 = 0;
    in3 = 1;
    in4 = 0;
}

void freno(void) {
    in1 = 0;
    in2 = 0;
    in3 = 0;
    in4 = 0;
}

void sensorOFF(void) {
    in1 = 0;
    in2 = 0;
    in3 = 0;
    in4 = 0;
}
