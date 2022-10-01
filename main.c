#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"

//********************************Comandos nRF24***************
#define NOP 0xFF
#define PWR_UP 0x0A
#define CHANNEL 0x5A

//********************************Registros nRF24**************
#define CONFIG 0x00
#define EN_AA 0x01
#define STATUS 0x07
#define RF_CH 0x05 
#define RX_ADDR 0x0A
#define TX_ADDR 0x10
#define RX_PW 0x11
 
 //*******************************funciones********************
void csLow();
void csHigh();
void ceLow();
void ceHigh();
uint8_t leerReg(spi_inst_t *spi, uint8_t reg);
void escribirReg(spi_inst_t *spi, uint8_t reg, uint8_t data);
void config(spi_inst_t *spi);
void leerBytes(spi_inst_t *spi, uint8_t reg, char *msg, uint8_t size);

//********************************Variables Globales***********
const int sck_pin = 10;
const int mosi_pin = 11;
const int miso_pin = 12;
const int cs_pin = 13; // cns
const int ce_pin = 22; // RX__TX

int main() {
    sleep_ms(100);
    spi_inst_t *spi = spi1;

    // Initialize chosen serial port
    stdio_init_all();

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
    char datoRx[5];
    char datoTx[5];
    // Loop forever
    while (true) {
        if(leerReg(spi, CONFIG) != 0x0A){
        printf("\n Off");
        }else{
            printf("\n On ");
        }

        printf("\n Shockburst:%x", leerReg(spi, EN_AA));
        printf("\n Canal:%x", leerReg(spi, RF_CH));
        leerBytes(spi, RX_ADDR, (uint8_t*)&datoRx, 5);
        printf("\n Rx:%s", datoRx); 
        leerBytes(spi, TX_ADDR, (uint8_t*)&datoTx, 5);
        printf("\n Tx:%s", datoTx); 
        printf("\n paquetes:%d", leerReg(spi, RX_PW)); 
        printf("\n STATUS = %x", leerReg(spi, CONFIG));
        sleep_ms(1000);

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

