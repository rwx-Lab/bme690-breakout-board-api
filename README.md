# BME690 API using pigpio for Raspberry Pi

This is a port of the [Bosch Sensortec BME690 SensorAPI](https://github.com/boschsensortec/BME690_SensorAPI) 
to work with the pigpio library on Raspberry Pi.

Based on the BME690 SensorAPI by Bosch Sensortec GmbH, licensed under BSD-3-Clause.

## Getting Started on Raspberry Pi

This repository provides examples for interfacing with the BME690 sensor directly on Raspberry Pi using the pigpio library. 

### Installation

1. **Install the pigpio library**:
   ```bash
   sudo apt update
   sudo apt install pigpio pigpio-tools libpigpio-dev
   ```

2. **Clone the repository**:
   ```bash
   git clone https://github.com/rwx-Lab/bme690-breakout-board-api.git
   cd bme690-breakout-board-api
   ```

### Building Examples

Navigate to any example directory and build:

```bash
cd examples/forced_mode
make
```

Available examples:
- `forced_mode` - Single-shot measurements
- `parallel_mode` - Continuous measurements with multiple heater profiles
- `sequential_mode` - Sequential measurements with different heater profiles  
- `self_test` - Sensor validation and diagnostics

### Running Examples

**Important**: The examples use pigpio directly and require root privileges because they initialize their own pigpio instance.

1. **Stop the pigpio daemon** (if running):
   ```bash
   sudo systemctl stop pigpiod
   ```
   
   If the service doesn't stop properly, you can force-kill any remaining processes:
   ```bash
   sudo killall pigpiod
   ```

2. **Run the example**:
   ```bash
   sudo ./forced_mode
   ```

**Why sudo is required**: The examples use `gpioInitialise()` to directly access GPIO hardware, which requires root privileges. This conflicts with the system pigpiod daemon, so it must be stopped first.