#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"

//********************************Comandos nRF24***************
#define NOP 0xFF
#define PWR_UP 0x0A

//********************************Registros nRF24**************
#define CONFIG 0x00
#define STATUS 0x07
 
 //*******************************funciones********************
void csLow();
void csHigh();
void ceLow();
void ceHigh();
uint8_t leerReg(spi_inst_t *spi, uint8_t reg);
void escribirReg(spi_inst_t *spi, uint8_t reg, uint8_t data);
void configRx(spi_inst_t *spi);
void StndBy(spi_inst_t *spi);

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

    escribirReg(spi, CONFIG, PWR_UP);
    sleep_us(1500);

    configRx(spi);

    // Loop forever
    while (true) {

        //printf("\n STATUS = %x", leerReg(spi, CONFIG));
        StndBy(spi);
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

void escribirReg(spi_inst_t *spi, uint8_t reg, uint8_t data){
    reg = 0x20 | (0x1F & reg);
    csLow();
    spi_write_blocking(spi, &reg, 1);
    spi_write_blocking(spi, &data, 1);
    csHigh();
}

void configRx(spi_inst_t *spi){
    uint8_t reg = leerReg(spi, CONFIG);
    reg = 0x01 | reg;
    ceHigh();
    escribirReg(spi, CONFIG ,reg);
    sleep_us(130);
}

void configTx(spi_inst_t *spi){
    uint8_t reg = leerReg(spi, CONFIG);
    reg = 0x01 | reg;
    ceHigh();
    escribirReg(spi, CONFIG ,reg);
    sleep_us(130);
}


void StndBy(spi_inst_t *spi){
    ceLow();
}