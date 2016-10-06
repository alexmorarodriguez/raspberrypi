#include <wiringPi.h>
int main (void){

    // Inicializamos wiringPi y asumimos que el programa va a estar utilizando
    // el esquema de numeracion de pin de wiringPi. Se numeran del 0 al 16.
    // Se tiene que llamar con privilegios de root.
    wiringPiSetup();

    // Establece el modo de un pin a INPUT, OUTPUT, PWM_OUTPUT o GPI_CLOCK.
    // En este caso ponemos el pin 0 a OUTPUT
    pinMode(0,OUTPUT);
    // Repetimso indefinidamente la secuencia de acciones que consiste en
    // activar y desactivar por intervalos de 500 ms. el pin 0 de la GPIO.
    for(;;){
        // Escribimos el valor HIGH en el pin 0 y esperamos 500 ms.
        digitalWrite(0,HIGH);delay(500);
        // Escribimos el valor LOW en el pin 0 y esperamos 500 ms.
        digitalWrite(0,LOW);delay(500);
    }
    return 0;
}
