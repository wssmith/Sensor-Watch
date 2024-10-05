/*
 * MIT License
 *
 * Copyright (c) 2022 Joey Castillo
 *
 * Ported from MIT-licensed code from CircuitPython
 * Copyright (c) 2016, 2017 Scott Shawcroft for Adafruit Industries
 *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "spiflash.h"

#define SPI_FLASH_FAST_READ false

static void flash_enable(void) {
    watch_set_pin_level(A3, false);
}

static void flash_disable(void) {
    watch_set_pin_level(A3, true);
}

static bool transfer(uint8_t *command, uint32_t command_length, uint8_t *data_in, uint8_t *data_out, uint32_t data_length) {
    bool status = watch_spi_write(command, command_length);
    if (status) {
        if (data_in != NULL && data_out != NULL) {
            status = watch_spi_transfer(data_out, data_in, data_length);
        } else if (data_out != NULL) {
            status = watch_spi_read(data_out, data_length);
        } else if (data_in != NULL) {
            status = watch_spi_write(data_in, data_length);
        }
    }
    flash_disable();
    return status;
}

static bool transfer_command(uint8_t command, uint8_t *data_in, uint8_t *data_out, uint32_t data_length) {
    return transfer(&command, 1, data_in, data_out, data_length);
}

bool spi_flash_command(uint8_t command) {
    return transfer_command(command, NULL, NULL, 0);
}

bool spi_flash_read_command(uint8_t command, uint8_t *data, uint32_t data_length) {
    return transfer_command(command, NULL, data, data_length);
}

bool spi_flash_write_command(uint8_t command, uint8_t *data, uint32_t data_length) {
    return transfer_command(command, data, NULL, data_length);
}

// Pack the low 24 bits of the address into a uint8_t array.
static void address_to_bytes(uint32_t address, uint8_t *bytes) {
    bytes[0] = (address >> 16) & 0xff;
    bytes[1] = (address >> 8) & 0xff;
    bytes[2] = address & 0xff;
}

bool spi_flash_sector_command(uint8_t command, uint32_t address) {
    uint8_t request[4] = {command, 0x00, 0x00, 0x00};
    address_to_bytes(address, request + 1);
    return transfer(request, 4, NULL, NULL, 0);
}

bool spi_flash_write_data(uint32_t address, uint8_t *data, uint32_t data_length) {
    uint8_t request[4] = {CMD_PAGE_PROGRAM, 0x00, 0x00, 0x00};
    // Write the SPI flash write address into the bytes following the command byte.
    address_to_bytes(address, request + 1);
    flash_enable();
    bool status = watch_spi_write(request, 4);
    if (status) {
        status = watch_spi_write(data, data_length);
    }
    flash_disable();
    return status;
}

bool spi_flash_read_data(uint32_t address, uint8_t *data, uint32_t data_length) {
    uint8_t request[5] = {CMD_READ_DATA, 0x00, 0x00, 0x00};
    uint8_t command_length = 4;
    if (SPI_FLASH_FAST_READ) {
        request[0] = CMD_FAST_READ_DATA;
        command_length = 5;
    }
    // Write the SPI flash read address into the bytes following the command byte.
    address_to_bytes(address, request + 1);
    flash_enable();
    bool status = watch_spi_write(request, command_length);
    if (status) {
        status = watch_spi_read(data, data_length);
    }
    flash_disable();
    return status;
}

void spi_flash_init(void) {
	gpio_set_pin_level(A3, true);
	gpio_set_pin_direction(A3, GPIO_DIRECTION_OUT);
    watch_enable_spi();
}
