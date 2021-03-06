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
#include <math.h>


#define ADC1 0x27
#define PIN_RED 1
#define PIN_GREEN 4
#define PIN_BLUE 5
#define PIN_LED 6

#define ALARMA_TEMPERATURA 27



// Devuelve true o false seg�n el bit sea 1 o 0
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
    char read_request[9];

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
    if ((file = open(filename,O_RDWR))<0) {
        fprintf(stderr, "\n%s", strerror(errno));
        printf("\nNo se puede configurar el dispositivo I2C");
        fflush(stdout);
        return -1;
    }


    // Especificamos la direcci�n del sensor (esclavo) con el que queremos establecer comunicaci�n
    // Si hubiese m�s de un esclavo, tendriamos que repetir esta operaci�n cada vez que quisiesemos
    // establecer comunicaci�n con uno diferente.
    printf("\nEscribimos %d en el bus i2c para hablar con el esclavo.", ADC1);
    fflush(stdout);
    int error= ioctl(file,I2C_SLAVE,ADC1);
    if (error < 0) {
          fprintf(stderr, "\n%s", strerror(errno));
          printf("\nError %d para adquirir acceso al bus y/o hablar con el esclavo.",error);
          fflush(stdout);
          return 1;
    }

    // Repetimos indefinidamente la secuencia de acciones que consiste en
    // activar y desactivar por intervalos de 500 ms. el pin 0 de la GPIO.
    for(;;){
        //mostrar fecha y hora
        time ( &rawtime );
        timeinfo = localtime ( &rawtime );

        printf("\n\n***************************************************************************************");
        printf( "\n\n[%d/%d/%d %d:%d:%d]",timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
        fflush(stdout);

        // Para despertar al sensor (esclavo) y completar un ciclo de medida, se envia (el maestro) un comando MR (Measurement Request)
        // Se le indica escribiendo su direccion + 0 (write)
        addr=0b00011011;
        addr=addr<<1;
        sprintf(read_request,"%d",addr);
        printf("\nEscribimos %s en el bus i2c para despertar al esclavo (comando RM).", read_request);
        fflush(stdout);
        if (write(file, read_request, 1) !=1){
          fprintf(stderr, "\n%s", strerror(errno));
          printf("\nError para adquirir acceso al bus y/o despertar al esclavo (comando RM).");
          fflush(stdout);
          return 1;
        }

        // Para extraer una lectura de humedad y temperatura, se env�a (el maestro) una condici�n de START
        // Se le indica escribiendo su direccion + 1 (read)
        addr=addr+1;
        sprintf(read_request,"%d",addr);
        printf("\nEscribimos %s en el bus i2c para recibir la lectura del esclavo (comando START).", read_request);
        fflush(stdout);
        if (write(file, read_request, 1) !=1){
          printf("\nError para adquirir acceso al bus y/o recibir la lectura del esclavo (comando START).");
          fflush(stdout);
          return 1;
        }

        printf("\nLeemos 4 bytes del esclavo.");
        fflush(stdout);
        // Leemos los cuatro bytes de temperatura y humedad
        char buffer[32]={0};
        if (read(file,buffer,4) != 4) {
          printf("\nError al leer los 4 bytes del bus i2c.");
          fflush(stdout);
        } else {

            printf("\n- Byte 0: -%o-", buffer[0]);
            printf("\n- Byte 1: -%o-", buffer[1]);
            printf("\n- Byte 2: -%o-", buffer[2]);
            printf("\n- Byte 3: -%o-", buffer[3]);

            // Estado: Los dos primeros bits del byte 0 representan el estado.
            // 0 Normal Operation: Datos v�lidos que no se han ido a buscar desde el �ltimo ciclo de medida.
            // 1 Stale Data: Datos inconsistentes o bien porque ya se leyeron en el anterior ciclo de medida
            //               o se han leido antes de que la medici�n se haya completado.
            // 2 Dispositivo en modo comando: Se utiliza para programar el sensor.
            // 3 No se utiliza.
            int status = buffer[0]&0b11000000<<8;
            printf("\nStatus= %d",status);
            fflush(stdout);

            if (status==0) {

                // El rango para la humedad es de 0% a 100%.
                // Se representa con 14 bits (6 del byte 0 y 8 del byte 1).
                // Los dos primeros bits del byte 0 representan el estado.
                // La formula de calculo es la indicada por el fabricante
                humedad=(float)(((buffer[0]&0b00111111)<<8)+buffer[1]);
                humedad=(humedad/(pow(2,14)-2))*100;

                // El rango para la temperatura es de -40�C a 125�C.
                // Se representa con 14 bits (8 del byte 2 y 6 del byte 3).
                // Los dos ultimos bits del byte 3 no tienen uso.
                // La formula de calculo es la indicada por el fabricante
                temperatura=(float)(((buffer[2]<<8)+buffer[3])>>2);
                temperatura=(temperatura/(pow(2,14)-2))*165-40;

                //Si temperatura > 27 -> led ON
                printf("\nTemperatura = %f",temperatura);
                if (temperatura>27) {
                    digitalWrite(PIN_LED, HIGH);
                    printf("\n - LED ON");
                } else {
                    digitalWrite(PIN_LED, LOW);
                    printf("\n - LED OFF");
                }

                printf("\nHumedad = %f",humedad);
                fflush(stdout);
                //Si humedad <40% -> red
                if (humedad<40) {
                    digitalWrite(PIN_GREEN, LOW);
                    digitalWrite(PIN_BLUE, LOW);
                    digitalWrite(PIN_RED, HIGH);
                    printf("\n - RED");
                //Si humedad >=40% y <=70% -> green
                } else if(humedad>=40 && humedad<70){
                    digitalWrite(PIN_RED, LOW);
                    digitalWrite(PIN_BLUE, LOW);
                    digitalWrite(PIN_GREEN, HIGH);
                    printf("\n - GREEN");
                //Si humedad >70% -> blue
                } else {
                    digitalWrite(PIN_RED, LOW);
                    digitalWrite(PIN_GREEN, LOW);
                    digitalWrite(PIN_BLUE, HIGH);
                    printf("\n - BLUE");
                }

            }


        }

        fflush(stdout);
        //cada 3 segundos repetimos la operacion
        delay(3000);

    }

    close(file);
    return 0;
}
