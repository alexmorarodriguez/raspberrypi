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
#include <geniePi.h>


#define ADC1 0x27
#define PIN_RED 1
#define PIN_GREEN 4
#define PIN_BLUE 5
#define PIN_LED 6

#define ALARMA_TEMPERATURA 27

int alarmaTemperatura;


// Devuelve true o false según el bit sea 1 o 0
int getBit (int n, int pos){
    int b=n>>pos;
    b=b&1;
    return b;
}


//Hilo para manejar el sensor y LEDS
static void *handleSensor(void *data){

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


/***************************************************
//Test 
    genieWriteObj(GENIE_OBJ_FORM, 0, 0);

//digitos temperatura 
    genieWriteObj(GENIE_OBJ_CUSTOM_DIGITS, 0x00, ALARMA_TEMPERATURA);
    printf("\n custom_digits");

// digitos hora verde 
    genieWriteObj(GENIE_OBJ_LED_DIGITS, 0x00, 8888);
    printf("\n led_digits");

// digitos hora rojo 
    genieWriteObj(GENIE_OBJ_LED_DIGITS, 0x01, 99);
    printf("\n led_digits");

//knob temperatura 
    genieWriteObj(GENIE_OBJ_KNOB, 0x00, ALARMA_TEMPERATURA);
    printf("\n GENIE_OBJ_KNOB");

//humedad
    genieWriteObj(GENIE_OBJ_METER, 0x00, 100);
    printf("\n GENIE_OBJ_METER");

//temperatura 
    int iniTemp=25;
    genieWriteObj(GENIE_OBJ_ANGULAR_METER, 0x00, 30+iniTemp);
    printf("\n GENIE_OBJ_ANGULAR_METER");

//user led 
    genieWriteObj(GENIE_OBJ_USER_LED, 0x00, 1);
    printf("\n GENIE_OBJ_USER_LED");
*****************************************************/

    // Especificamos la dirección del sensor (esclavo) con el que queremos establecer comunicación
    // Si hubiese más de un esclavo, tendriamos que repetir esta operación cada vez que quisiesemos
    // establecer comunicación con uno diferente.
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

        // Para extraer una lectura de humedad y temperatura, se envía (el maestro) una condición de START
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
            // 0 Normal Operation: Datos válidos que no se han ido a buscar desde el último ciclo de medida.
            // 1 Stale Data: Datos inconsistentes o bien porque ya se leyeron en el anterior ciclo de medida
            //               o se han leido antes de que la medición se haya completado.
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

                genieWriteObj(GENIE_OBJ_METER, 0x00, humedad);

                // El rango para la temperatura es de -40ºC a 125ºC.
                // Se representa con 14 bits (8 del byte 2 y 6 del byte 3).
                // Los dos ultimos bits del byte 3 no tienen uso.
                // La formula de calculo es la indicada por el fabricante
                temperatura=(float)(((buffer[2]<<8)+buffer[3])>>2);
                temperatura=(temperatura/(pow(2,14)-2))*165-40;

                genieWriteObj(GENIE_OBJ_ANGULAR_METER, 0x00, temperatura);

                //Si temperatura > alarmaTemperatura -> led ON
                printf("\nTemperatura = %f",temperatura);
                if (temperatura>alamarmaTemperatura) {
                    digitalWrite(PIN_LED, HIGH);
                    genieWriteObj(GENIE_OBJ_USER_LED, 0x00, 1)
                    printf("\n - LED ON");
                } else {
                    digitalWrite(PIN_LED, LOW);
                    genieWriteObj(GENIE_OBJ_USER_LED, 0x00, 0)
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


//Procesamos los mensajes recibidos del display  
void handleGenieEvent(struct genieReplyStruct * reply) {
    //Si es un evento reconocido 
    if(reply->cmd == GENIE_REPORT_EVENT) {
        // ... del tipo knob -> sincronizamos/actualizamos los digitos del display con el dato temperatura que nos suministra
        if(reply->object == GENIE_OBJ_KNOB){
            alarmaTemperatura = reply->data;
            genieWriteObj(GENIE_OBJ_LED_DIGITS, 0x02, alarmaTemperatura);
        }
    } else {
         printf("Evento desconocido: comando: %2d, objeto: %2d, indice: %d, data: %d \r\n", reply->cmd, reply->object, reply->index, reply->data);
    }
}
 
//Thread de manejo del reloj
static void *handleClock(void *data)
{
    int seconds = 0;
    int hours = 0;
    time_t timer;
    struct tm* tm_info;
 
    // Arrancamos un bucle infinito
    for(;;)             
    {
        time(&timer);
        tm_info = localtime(&timer);
        seconds = tm_info->tm_sec;
        hours = (tm_info->tm_hour * 100) + tm_info->tm_min;
 
        //Actualizamos los digitos del reloj del display con la hora y los segundos
        genieWriteObj(GENIE_OBJ_LED_DIGITS, 0x00, seconds);
        genieWriteObj(GENIE_OBJ_LED_DIGITS, 0x01, hours);
 
        usleep(1000000);    //Esperamos 1 segundo
 
    }
    return NULL;
}

// Programa principal
int main (void) {

    // Hilo para el rejoj
    pthread_t relojThread;

    // hHilo para el sensr
    pthread_t sensorThread;

    // Struct de tipo genieReplyStruct
    struct genieReplyStruct reply ;
 
    printf("\n\n");
    printf("Visi-Genie && Raspberry-Pi -> Sensorizacion de Temperatura & Humedad\n");
    printf("====================================================================\n");
 
    //Se abre el puerto serie de la Raspberry Pi a 115200
    //El programa del display Visi-Genie debe haber sido generado para trabajar a los mismos baudios
    genieSetup("/dev/ttyAMA0", 115200);

    //Inicializamos los displays de la pantalla (knob y digitos del led) al valor de alarma inicial de por defecto
    alarmaTemperatura= ALARMA_TEMPERATURA;
    genieWriteObj(GENIE_OBJ_LED_DIGITS, 0x02, alarmaTemperatura);
    genieWriteObj(GENIE_OBJ_KNOB, 0x00, alarmaTemperatura);
 
    //Arrancamos un hilo para la escritura en el reloj
    (void)pthread_create (&clockThread,  NULL, handleClock, NULL);

    //Arrancamos un hilo para la escritura de los sensores 
    (void)pthread_create (&sensorThread,  NULL, handleSensor, NULL);
 
    //Arrancamos un bucle infinito
    for(;;) {
        // Esperamos hasta que haya algun mensaje del display
        while(genieReplyAvail()) {
            // Cuando haya algun mensaje (uno o mas), extraemos uno del buffer de eventos
            genieGetReply(&reply);
            //Llamamos al manejador de eventos para que procese el mensaje que hemos extraido
            handleGenieEvent(&reply);
        }
        //Esperamos 10 milisegundos para no saturar a la raspberry
        usleep(10000); 
    }                                   
    return 0;
}
