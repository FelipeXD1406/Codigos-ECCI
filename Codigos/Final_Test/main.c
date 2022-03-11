/*
 * File:   main.c
 * Author: Alejandro
 *
 * Created on 10 de marzo de 2022, 10:20 PM
 */

//Code Github 

#include <xc.h> 
#include <string.h> 
#include <stdio.h>

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

void __interrupt() UART_Rx(void); //Función de interrupción
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

void manual(void);
void automatico(void);

int i, x, modo, PWM = 0, mood = 0, z, s;
int y = 1;
int a;
float duty_cicle1, duty_cicle2;
float conversion1, conversion2, conversion3, ciclo_util1, ciclo_util2, d1, d2, c, b;
int n = 0;

void main(void) {
    while (1) {

    }
}
