/**
 * Copyright (C) 2024 Bosch Sensortec GmbH. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <pigpio.h>
#include <string.h>
#include "bme69x.h"
#include "common.h"

/******************************************************************************/
/*!                 Macro definitions                                         */
/*! Default I2C bus for Raspberry Pi */
#define BME69X_I2C_BUS          1

/*! Default SPI bus for Raspberry Pi */
#define BME69X_SPI_BUS          0

/*! Default SPI speed */
#define BME69X_SPI_SPEED        1000000  /* 1 MHz */

/******************************************************************************/
/*!                Static variable definition                                 */
static uint8_t dev_addr;
static int i2c_handle = -1;
static int spi_handle = -1;

/******************************************************************************/
/*!                User interface functions                                   */

/*!
 * I2C read function using pigpio
 */
BME69X_INTF_RET_TYPE bme69x_i2c_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    int result;
    
    (void)intf_ptr;

    if (i2c_handle < 0) {
        return BME69X_E_COM_FAIL;
    }

    result = i2cWriteDevice(i2c_handle, (char*)&reg_addr, 1);
    if (result < 0) {
        return BME69X_E_COM_FAIL;
    }

    result = i2cReadDevice(i2c_handle, (char*)reg_data, len);
    if (result != (int)len) {
        return BME69X_E_COM_FAIL;
    }

    return BME69X_INTF_RET_SUCCESS;
}

/*!
 * I2C write function using pigpio
 */
BME69X_INTF_RET_TYPE bme69x_i2c_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    int result;
    char buffer[len + 1];
    
    (void)intf_ptr;

    if (i2c_handle < 0) {
        return BME69X_E_COM_FAIL;
    }

    buffer[0] = reg_addr;
    for (uint32_t i = 0; i < len; i++) {
        buffer[i + 1] = reg_data[i];
    }

    result = i2cWriteDevice(i2c_handle, buffer, len + 1);
    if (result < 0) {
        return BME69X_E_COM_FAIL;
    }

    return BME69X_INTF_RET_SUCCESS;
}

/*!
 * SPI read function using pigpio
 */
BME69X_INTF_RET_TYPE bme69x_spi_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    int result;
    char tx_buffer[len + 1];
    char rx_buffer[len + 1];
    
    (void)intf_ptr;

    if (spi_handle < 0) {
        return BME69X_E_COM_FAIL;
    }

    tx_buffer[0] = reg_addr | 0x80;
    for (uint32_t i = 1; i <= len; i++) {
        tx_buffer[i] = 0;
    }

    result = spiXfer(spi_handle, tx_buffer, rx_buffer, len + 1);
    if (result < 0) {
        return BME69X_E_COM_FAIL;
    }

    for (uint32_t i = 0; i < len; i++) {
        reg_data[i] = rx_buffer[i + 1];
    }

    return BME69X_INTF_RET_SUCCESS;
}

/*!
 * SPI write function using pigpio
 */
BME69X_INTF_RET_TYPE bme69x_spi_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    int result;
    char tx_buffer[len + 1];
    
    (void)intf_ptr;

    if (spi_handle < 0) {
        return BME69X_E_COM_FAIL;
    }

    tx_buffer[0] = reg_addr & 0x7F;
    for (uint32_t i = 0; i < len; i++) {
        tx_buffer[i + 1] = reg_data[i];
    }

    result = spiWrite(spi_handle, tx_buffer, len + 1);
    if (result < 0) {
        return BME69X_E_COM_FAIL;
    }

    return BME69X_INTF_RET_SUCCESS;
}

/*!
 * Delay function using pigpio
 */
void bme69x_delay_us(uint32_t period, void *intf_ptr)
{
    (void)intf_ptr;
    gpioDelay(period);
}

uint32_t bme69x_get_millis(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint32_t)((tv.tv_sec * 1000) + (tv.tv_usec / 1000));
}

void bme69x_check_rslt(const char api_name[], int8_t rslt)
{
    switch (rslt)
    {
        case BME69X_OK:

            /* Do nothing */
            break;
        case BME69X_E_NULL_PTR:
            printf("API name [%s]  Error [%d] : Null pointer\r\n", api_name, rslt);
            break;
        case BME69X_E_COM_FAIL:
            printf("API name [%s]  Error [%d] : Communication failure\r\n", api_name, rslt);
            break;
        case BME69X_E_INVALID_LENGTH:
            printf("API name [%s]  Error [%d] : Incorrect length parameter\r\n", api_name, rslt);
            break;
        case BME69X_E_DEV_NOT_FOUND:
            printf("API name [%s]  Error [%d] : Device not found\r\n", api_name, rslt);
            break;
        case BME69X_E_SELF_TEST:
            printf("API name [%s]  Error [%d] : Self test error\r\n", api_name, rslt);
            break;
        case BME69X_W_NO_NEW_DATA:
            printf("API name [%s]  Warning [%d] : No new data found\r\n", api_name, rslt);
            break;
        default:
            printf("API name [%s]  Error [%d] : Unknown error code\r\n", api_name, rslt);
            break;
    }
}

int8_t bme69x_interface_init(struct bme69x_dev *bme, uint8_t intf)
{
    int8_t rslt = BME69X_OK;

    if (bme != NULL)
    {
        if (gpioInitialise() < 0)
        {
            printf("Failed to initialize pigpio library\n");
            return BME69X_E_COM_FAIL;
        }

        printf("pigpio library initialized successfully\n");

        if (intf == BME69X_I2C_INTF)
        {
            printf("I2C Interface\n");
            dev_addr = BME69X_I2C_ADDR_HIGH;
            
            i2c_handle = i2cOpen(BME69X_I2C_BUS, dev_addr, 0);
            if (i2c_handle < 0)
            {
                printf("Failed to open I2C bus %d, device 0x%02X\n", BME69X_I2C_BUS, dev_addr);
                gpioTerminate();
                return BME69X_E_COM_FAIL;
            }
            
            printf("I2C connection opened successfully (handle: %d)\n", i2c_handle);
            
            bme->read = bme69x_i2c_read;
            bme->write = bme69x_i2c_write;
            bme->intf = BME69X_I2C_INTF;
        }
        /* Bus configuration : SPI */
        else if (intf == BME69X_SPI_INTF)
        {
            printf("SPI Interface\n");
            
            spi_handle = spiOpen(BME69X_SPI_BUS, BME69X_SPI_SPEED, 0);
            if (spi_handle < 0)
            {
                printf("Failed to open SPI bus %d\n", BME69X_SPI_BUS);
                gpioTerminate();
                return BME69X_E_COM_FAIL;
            }
            
            printf("SPI connection opened successfully (handle: %d)\n", spi_handle);
            
            bme->read = bme69x_spi_read;
            bme->write = bme69x_spi_write;
            bme->intf = BME69X_SPI_INTF;
        }

        bme->delay_us = bme69x_delay_us;
        bme->intf_ptr = &dev_addr;
        bme->amb_temp = 25; /* The ambient temperature in deg C is used for defining the heater temperature */
    }
    else
    {
        rslt = BME69X_E_NULL_PTR;
    }

    return rslt;
}

void bme69x_pigpio_deinit(void)
{
    (void)fflush(stdout);

    if (i2c_handle >= 0)
    {
        i2cClose(i2c_handle);
        i2c_handle = -1;
    }

    if (spi_handle >= 0)
    {
        spiClose(spi_handle);
        spi_handle = -1;
    }

    gpioTerminate();
}
