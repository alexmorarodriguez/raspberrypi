#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#define ADC1 0x27
#define PIN_RED 1
#define PIN_GREEN 4
#define PIN_BLUE 5
#define PIN_LED 6

// Devuelve true o false según el bit sea 1 o 0
int getBit (int n, int pos){
    int b=n>>pos;
    b=b&1;
    return b;
}

int main (void){

    float temperatura=0;
    float humedad=0;
    time_t rawtime;
    struct tm * timeinfo;
    int addr;

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

    // Configuramos el disposivo
    int file;
    char *filename="/dev/i2c-1";
    if ((file = open(filename,O_RDWR)<0)) {
        printf("\nNo se puede configurar el dispositivo I2C");
        fflush(stdout);
        return -1;
    }

    // Repetimos indefinidamente la secuencia de acciones que consiste en
    // activar y desactivar por intervalos de 500 ms. el pin 0 de la GPIO.
    for(;;){
        //mostrar fecha y hora
        time ( &rawtime );
        timeinfo = localtime ( &rawtime );

        printf( "\n\n[%d/%d/%d %d:%d:%d]",timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
        fflush(stdout);

        // Despertamos al sensor
        // ADC1<<1;
/*
        printf("\nEscribimos %d en el bus i2c para hablar con el esclavo.", addr);
        fflush(stdout);
        if (ioctl(file,I2C_SLAVE,addr) < 0) {
          printf("\nError para adquirir acceso al bus y/o hablar con el esclavo.");
          fflush(stdout);
          return 1;
        }
*/
        char read_request[9]; 
        addr=0b00011011;
        addr=addr<<1;
        sprintf(read_request,"%d",addr);
        printf("\nEscribimos %s en el bus i2c para hablar con el esclavo.", read_request);
        fflush(stdout);
        if (write(file, read_request, 1) !=1){
          printf("\nError para adquirir acceso al bus y/o hablar con el esclavo.");
          fflush(stdout);
          return 1;
        }

        addr=addr+1;
        sprintf(read_request,"%d",addr);
        printf("\nEscribimos %s en el bus i2c para hablar con el esclavo.", read_request);
        fflush(stdout);
        if (write(file, read_request, 1) !=1){
          printf("\nError para adquirir acceso al bus y/o hablar con el esclavo.");
          fflush(stdout);
          return 1;
        }

        printf("\nLeemos 4 bytes del esclavo.");
        fflush(stdout);
        // Leemos los cuatro bytes de temperatura y humedad
        char buffer[2000] = {0};
        if (read(file,buffer,4) != 4) {
          printf("\nError al leer del bus i2c.");
          fflush(stdout);
        } else { 
            printf("\nLeemos del bus i2c.");
            fflush(stdout);
            humedad=(float)(((buffer[0]&0b00111111)<<8)+buffer[1]);
            humedad=(humedad)*100;

            temperatura=(float)(((buffer[2]<<8)+buffer[3])>>2);
            temperatura=(temperatura)*165-40;

            //temperatura > 27 -> led
            printf("\nTemperatura = %f",temperatura);
            if (temperatura>27) {
                digitalWrite(PIN_LED, HIGH);
                printf("\nLED ON");
            } else {
                digitalWrite(PIN_LED, LOW);
                printf("\nLED OFF");
            }

            printf("\nHumedad = %f",humedad);
            fflush(stdout);
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
        //cada 3 segundos repetimos la operacion
        delay(3000);

    }

    close(file);
    return 0;
}
