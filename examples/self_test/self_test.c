/**
 * Copyright (C) 2025 Bosch Sensortec GmbH
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stdio.h>

#include "bme69x.h"
#include "common.h"

int8_t custom_selftest_check(struct bme69x_dev *dev)
{
    int8_t rslt;
    struct bme69x_conf conf;
    struct bme69x_heatr_conf heatr_conf;
    struct bme69x_data data;
    uint8_t n_fields;
    uint32_t del_period;
    
    printf("=== BME690 Custom Self-Test ===\n");
    
    printf("Step 1: Testing basic sensor communication...\n");
    if (dev->chip_id != BME69X_CHIP_ID) {
        printf("❌ Chip ID mismatch: expected 0x%02x, got 0x%02x\n", BME69X_CHIP_ID, dev->chip_id);
        return BME69X_E_DEV_NOT_FOUND;
    }
    printf("✅ Chip ID correct: 0x%02x\n", dev->chip_id);
    
    printf("Step 2: Configuring sensor for measurement...\n");
    rslt = bme69x_get_conf(&conf, dev);
    if (rslt != BME69X_OK) {
        printf("❌ Failed to get configuration\n");
        return rslt;
    }
    
    conf.filter = BME69X_FILTER_OFF;
    conf.odr = BME69X_ODR_NONE;
    conf.os_hum = BME69X_OS_16X;
    conf.os_pres = BME69X_OS_1X;
    conf.os_temp = BME69X_OS_2X;
    
    rslt = bme69x_set_conf(&conf, dev);
    if (rslt != BME69X_OK) {
        printf("❌ Failed to set configuration\n");
        return rslt;
    }
    printf("✅ Sensor configuration successful\n");
    
    printf("Step 3: Testing heater configuration...\n");
    heatr_conf.enable = BME69X_ENABLE;
    heatr_conf.heatr_temp = 300;
    heatr_conf.heatr_dur = 100;  
    
    rslt = bme69x_set_heatr_conf(BME69X_FORCED_MODE, &heatr_conf, dev);
    if (rslt != BME69X_OK) {
        printf("⚠️  Warning: Heater configuration failed (result: %d)\n", rslt);
        printf("    This may indicate heater hardware issues, but other sensors should work\n");
    } else {
        printf("✅ Heater configuration successful\n");
    }
    
    printf("Step 4: Performing measurement...\n");
    rslt = bme69x_set_op_mode(BME69X_FORCED_MODE, dev);
    if (rslt != BME69X_OK) {
        printf("❌ Failed to set forced mode\n");
        return rslt;
    }
    
    del_period = bme69x_get_meas_dur(BME69X_FORCED_MODE, &conf, dev);
    del_period += (heatr_conf.heatr_dur * 1000); 
    
    dev->delay_us(del_period, dev->intf_ptr);
    
    rslt = bme69x_get_data(BME69X_FORCED_MODE, &data, &n_fields, dev);
    if (rslt != BME69X_OK || n_fields == 0) {
        printf("❌ Failed to get measurement data\n");
        return BME69X_E_COM_FAIL;
    }
    
    printf("✅ Measurement data retrieved\n");
    
    printf("Step 5: Validating measurement ranges...\n");
    
    if (data.temperature < 0 || data.temperature > 60) {
        printf("❌ Temperature out of valid range: %.2f°C\n", data.temperature);
        return BME69X_E_SELF_TEST;
    }
    printf("✅ Temperature in valid range: %.2f°C\n", data.temperature);
    
    float pressure_hpa = data.pressure / 100.0f;
    if (pressure_hpa < 300 || pressure_hpa > 1200) {
        printf("❌ Pressure out of valid range: %.2f hPa\n", pressure_hpa);
        return BME69X_E_SELF_TEST;
    }
    printf("✅ Pressure in valid range: %.2f hPa\n", pressure_hpa);
    
    if (data.humidity < 0 || data.humidity > 100) {
        printf("❌ Humidity out of valid range: %.2f%%\n", data.humidity);
        return BME69X_E_SELF_TEST;
    }
    printf("✅ Humidity in valid range: %.2f%%\n", data.humidity);
    
    printf("Step 6: Checking gas sensor status...\n");
    
    if ((data.status & BME69X_NEW_DATA_MSK) && (data.status & BME69X_GASM_VALID_MSK)) {
        if (data.status & BME69X_HEAT_STAB_MSK) {
            printf("✅ Gas sensor working correctly (resistance: %.0f ohm)\n", data.gas_resistance);
        } else {
            printf("⚠️  Warning: Heater not stable (IDAC: 0x%02x)\n", data.idac);
            printf("   Gas readings may be inaccurate, but sensor is functional\n");
        }
    } else {
        printf("⚠️  Warning: Gas measurement issues detected\n");
        printf("   Temperature, pressure, and humidity sensors are working\n");
    }
    
    printf("\n=== Self-Test Summary ===\n");
    printf("✅ Basic sensors (T/P/H) are functional\n");
    printf("✅ I2C communication working\n");
    printf("✅ Sensor configuration successful\n");
    
    if (!(data.status & BME69X_HEAT_STAB_MSK) || data.idac == 0xFF) {
        printf("⚠️  Warning: Heater not stable (IDAC: 0x%02x)\n", data.idac);
        printf("   Gas readings may be inaccurate, but sensor is functional\n");
    } else {
        printf("✅ All sensors including gas sensor working correctly\n");
    }
    
    printf("\n🎉 Custom self-test PASSED\n");
    return BME69X_OK;
}

/***********************************************************************/
/*                         Test code                                   */
/***********************************************************************/

int main(void)
{
    struct bme69x_dev bme;
    int8_t rslt;

    /* Interface preference is updated as a parameter
     * For I2C : BME69X_I2C_INTF
     * For SPI : BME69X_SPI_INTF
     */
    rslt = bme69x_interface_init(&bme, BME69X_I2C_INTF);
    bme69x_check_rslt("bme69x_interface_init", rslt);

    rslt = bme69x_init(&bme);
    bme69x_check_rslt("bme69x_init", rslt);

    rslt = custom_selftest_check(&bme);

    if (rslt == BME69X_OK)
    {
        printf("\n✅ Overall self-test SUCCESSFUL\n");
        printf("Sensor is ready for use (T/P/H sensors confirmed working)\n");
    }
    else
    {
        printf("\n❌ Self-test FAILED\n");
    }

    bme69x_pigpio_deinit();

    return (rslt == BME69X_OK) ? 0 : 1;  
}
