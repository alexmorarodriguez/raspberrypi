#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <stdio.h>
#include <time.h>

#define ADC1 0x27
#define PIN_RED 1
#define PIN_GREEN 4
#define PIN_BLUE 5
#define PIN_LED 6

int main (void){

    // Inicializamos wiringPi y asumimos que el programa va a estar utilizando
    // el esquema de numeracion de pin de wiringPi. Se numeran del 0 al 16.
    // Se tiene que llamar con privilegios de root.
    wiringPiSetup();

    // Establece el modo de un pin a INPUT, OUTPUT, PWM_OUTPUT o GPI_CLOCK.
    // En este caso ponemos los pin a OUTPUT
    pinMode(PIN_LED, OUTPUT);
    pinMode(PIN_RED, OUTPUT);
    pinMode(PIN_GREEN, OUTPUT);
    pinMode(PIN_BLUE, OUTPUT);

    int data=0;
    int reg=0;
    int temperatura=0;
    int humedad=0;
    time_t rawtime;
    struct tm * timeinfo;

    if (wiringPiI2CSetup(ADC1)==-1){
        printf("\nNo se puede configurar el dispositivo I2C");
        return -1;
    }

    // Repetimso indefinidamente la secuencia de acciones que consiste en
    // activar y desactivar por intervalos de 500 ms. el pin 0 de la GPIO.
    for(;;){
        //mostrar fecha y hora
        time ( &rawtime );
        timeinfo = localtime ( &rawtime );

        printf( "\n\n[%d/%d/%d %d:%d:%d]",timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

        //mostrar medida
        data=wiringPiI2CRead(ADC1);
        printf("\nDatos = %d",data);

        wiringPiI2CReadReg16(ADC1,reg);
        printf("\nReg = %d",reg);


        if (data==-1){
            printf("\nNo hay datos");
            digitalWrite(PIN_LED, HIGH);
            digitalWrite(PIN_GREEN, HIGH);
        } else {

            printf("\nDatos = %d",data);

            //temperatura > 27 -> led
            printf("\nTemperatura = %d",temperatura);
            if (temperatura>27) {
                digitalWrite(PIN_LED, HIGH);
                printf("\nLED ON");
            } else {
                digitalWrite(PIN_LED, LOW);
                printf("\nLED OFF");
            }

            printf("\nHumedad = %d",humedad);
            //humedad <40% -> red
            if (humedad<40) {
                digitalWrite(PIN_GREEN, LOW);
                digitalWrite(PIN_BLUE, LOW);
                digitalWrite(PIN_RED, HIGH);
                printf("\nRED");
            //humedad >=40% y <=70% -> green
            } else if(humedad>=40 && humedad<70){
                digitalWrite(PIN_RED, LOW);
                digitalWrite(PIN_BLUE, LOW);
                digitalWrite(PIN_GREEN, HIGH);
                printf("\nGREEN");
            //humedad >70% -> blue
            } else {
                digitalWrite(PIN_RED, LOW);
                digitalWrite(PIN_GREEN, LOW);
                digitalWrite(PIN_BLUE, HIGH);
                printf("\nBLUE");
            }
        }

        fflush(stdout);
        //cada 3 segundos
        delay(3000);

    }
    return 0;
}
