#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#define LED_PIN 25

//********************************Comandos nRF24***************
#define NOP 0xFF
#define PWR_UP 0x0A
#define CHANNEL 0x5A
#define TX_PAYLOAD 0xA0
#define RX_PAYLOAD 0x61

//********************************Registros nRF24**************
#define CONFIG 0x00
#define EN_AA 0x01
#define STATUS 0x07
#define RF_CH 0x05 
#define RX_ADDR 0x0A
#define TX_ADDR 0x10
#define RX_PW 0x11
#define FIFO_STATUS 0x17
 
 //*******************************funciones********************
void csLow();
void csHigh();
void ceLow();
void ceHigh();
uint8_t leerReg(spi_inst_t *spi, uint8_t reg);
void escribirReg(spi_inst_t *spi, uint8_t reg, uint8_t data);
void config(spi_inst_t *spi);
void leerBytes(spi_inst_t *spi, uint8_t reg, char *msg, uint8_t size);
void ModoTx(spi_inst_t* spi);
void ModoRx(spi_inst_t* spi);
void enviarMsg(spi_inst_t* spi, char *msg);
void recirbirMsg(spi_inst_t* spi, char *msg);
uint8_t nuevoMsg(spi_inst_t* spi);
void modoStby(spi_inst_t* spi);
// *************************************funciones uart*********
void enviar_datos(int acx, int acy, int acz, int gx, int gy, int gz);

//********************************Variables Globales***********
const int sck_pin = 10;
const int mosi_pin = 11;
const int miso_pin = 12;
const int cs_pin = 13; // cns
const int ce_pin = 22; // RX__TX

//********************************Variables uart***************
short sqn = 0;
short sqn1 = 0;
int16_t input;
char get; 
char buffer[32];

//*********************************Interrupcion por timer******
int led_value = 0;
int estado = 0;
bool repeating_timer_callback(struct repeating_timer *t)
{
    led_value = 1 - led_value;
    gpio_put(LED_PIN, led_value);
    estado = 1 - estado;

    return true;
}

//*********************************Codigo tipo C***************
int main() {
    sleep_ms(100);
    spi_inst_t *spi = spi1;

    // Initialize chosen serial port
    stdio_init_all();

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    struct repeating_timer timer;
    add_repeating_timer_ms(500, repeating_timer_callback, NULL, &timer);
    

    gpio_init(cs_pin);
    gpio_set_dir(cs_pin, GPIO_OUT);
    gpio_put(cs_pin, 1);

    gpio_init(ce_pin);
    gpio_set_dir(ce_pin, GPIO_OUT);
    gpio_put(ce_pin, 0);

    // Inicializar el spi a 10MHz
    spi_init(spi, 10*1000*1000);

    gpio_set_function(sck_pin, GPIO_FUNC_SPI);
    gpio_set_function(mosi_pin, GPIO_FUNC_SPI);
    gpio_set_function(miso_pin, GPIO_FUNC_SPI);

    config(spi);
   
    char mensaje[32], 
    delimitador[] = " ",
    primerMensaje[32],
    envioMensaje[32];
    char *token;
    char accelX[16], accelY[16], accelZ[16];
    int cont = 0;
    int acX, acY, acZ, gX, gY, gZ;
    // Timers

    // Loop forever
    ModoRx(spi);

    while (true) {

        if(nuevoMsg(spi)==1){
            recirbirMsg(spi, (uint8_t*)&mensaje);
            char *token = strtok(mensaje, delimitador);
            if(token != NULL){
            while(token != NULL){

                if(cont >= 0 && cont <= 5){
                    if(cont==0){
                        acX = atoi(token);
                        //printf("\nAceleración X =%d", acX);
                        token = strtok(NULL, delimitador);
                    }else if(cont==1){
                        acY = atoi(token);
                        //printf("\nAceleración Y =%d", acY);   
                        token = strtok(NULL, delimitador); 
                    }else if(cont==2){
                        acZ = atoi(token);
                        //printf("\nAceleración Z =%d", acZ);
                        token = strtok(NULL, delimitador);
                    }else if(cont==3){
                        gX = atoi(token);
                        //printf("\nGyro X =%d", gX);
                        token = strtok(NULL, delimitador);
                    }else if(cont==4){
                        gY = atoi(token);
                        //printf("\nGyro Y =%d", gY);
                        token = strtok(NULL, delimitador);
                    }else if(cont==5){
                        gZ = atoi(token);
                        //printf("\nGyro Z =%d", gZ);
                        token = strtok(NULL, delimitador);
                    }
                    cont++;
                }else{
                    cont = 0;
                }
            }
    }
            
        }
		
		enviar_datos(acX, acY, acZ, gX, gY, gZ);

        sleep_ms(100);
    }
    return 0;
}

//***************************** contexto de las funciones******


void csLow(){
    gpio_put(cs_pin, 0);
}

void csHigh(){
    gpio_put(cs_pin, 1);
}

void ceLow(){
    gpio_put(ce_pin, 0);
}

void ceHigh(){
    gpio_put(ce_pin, 1);
}

uint8_t leerReg(spi_inst_t *spi, uint8_t reg){
    uint8_t resultado = 0x00 | (0x1F & reg);
    csLow();
    spi_write_blocking(spi, &reg, 1);
    spi_read_blocking(spi, NOP, &resultado, 1);
    csHigh();

    return resultado;
}

void leerBytes(spi_inst_t *spi, uint8_t reg, char *msg, uint8_t size){
    reg = 0x00 | (0x1F & reg);
    csLow();
    spi_write_blocking(spi, &reg, 1);
    spi_read_blocking(spi, NOP, (uint8_t*)msg, size);
    csHigh();
}

void escribirReg(spi_inst_t *spi, uint8_t reg, uint8_t data){
    reg = 0x20 | (0x1F & reg);
    csLow();
    spi_write_blocking(spi, &reg, 1);
    spi_write_blocking(spi, &data, 1);
    csHigh();
}

void escribirRegBytes(spi_inst_t *spi, uint8_t reg, uint8_t *data, uint8_t size){
    reg = 0x20 | (0x1F & reg);
    csLow();
    spi_write_blocking(spi, &reg, 1);
    spi_write_blocking(spi, (uint8_t*)data, size);
    csHigh();
}

void config(spi_inst_t *spi){
    ceLow();
    csHigh();
    sleep_ms(11);

    escribirReg(spi, CONFIG, PWR_UP);
    sleep_us(1500);
    
    escribirReg(spi, EN_AA, 0x00); // shockburst desactivado
    
    escribirReg(spi, RF_CH, CHANNEL); // canal de radio frecuencia #90 

    escribirRegBytes(spi, RX_ADDR, (uint8_t*)"beast", 5);

    escribirRegBytes(spi, TX_ADDR, (uint8_t*)"beast", 5);

    escribirReg(spi, RX_PW, 32);
    
}

void ModoTx(spi_inst_t* spi){
    uint8_t reg = leerReg(spi, CONFIG);
    reg &= ~(1<<0);
    escribirReg(spi, CONFIG, reg);
}

void ModoRx(spi_inst_t* spi){
    uint8_t reg = leerReg(spi, CONFIG);
    reg &= ~(1<<0);
    reg |= (1<<0);
    escribirReg(spi, CONFIG, reg);
    ceLow();
    ceHigh();
    sleep_us(130);
}

void enviarMsg(spi_inst_t* spi, char *msg){
    uint8_t cmd = TX_PAYLOAD;
    csLow();
    spi_write_blocking(spi, &cmd, 1);
    spi_write_blocking(spi, (uint8_t*)msg, 32);
    csHigh();

    ceHigh();
    sleep_us(10);
    ceLow();
}

void recirbirMsg(spi_inst_t* spi, char *msg){
    uint8_t cmd = RX_PAYLOAD;
    csLow();
    spi_write_blocking(spi, &cmd, 1);
    spi_read_blocking(spi, NOP, (uint8_t*)msg, 32);
    csHigh();
}

uint8_t nuevoMsg(spi_inst_t* spi)
{
    uint8_t fifo_status = leerReg(spi, FIFO_STATUS) & 0x01;
    return !fifo_status;
}

void modoStby(spi_inst_t* spi)
{
    if(gpio_get(ce_pin))
    {
        ceLow();
    }
}


//*********************************************contexto funciones uart
void enviar_datos(int acx, int acy, int acz, int gx, int gy, int gz){

    short SQN = 0, SQN1 = 0;
    uint8_t pre = 253;
    int8_t mid = 11;

     int sumAr;
    int8_t len;
    int8_t ldata;
    int8_t ldata2;
    int8_t ldata3;


       SQN++;
       SQN1 = SQN*2;
       ldata = sizeof(acx) + sizeof(gx);
       ldata2 = sizeof(acy) + sizeof(gy);
       ldata3 = sizeof(acz) + sizeof(gz);
       len = ldata + ldata2 + ldata3;
       sumAr = acx + acy + acz + gx + gy + gz;
       printf("%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",pre,SQN1,mid,len, acx, acy, acz, gx, gy, gz, sumAr);
       sleep_ms(500);

}