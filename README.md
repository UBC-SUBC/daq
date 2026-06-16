# SUBC DAQ Firmware

Zephyr-based data acquisition firmware for the Nordic nRF52840 DK. The firmware currently reads the working environmental/pressure sensors and logs rows to a FAT-formatted SD card over SPI.

## What Works Now

Current working devices:

- SD card logging over SPI1
- BME280 over SPI3
- Blue Robotics Bar30 / MS5837-style pressure sensor over I2C0

Planned / placeholder fields:

- RPM
- X, Y, Z
- IMU and hall-effect sensor status entries exist in software, but they are not currently read into the CSV log.

## How It Works

Startup happens in two stages:

1. `onboard_peripherals_init()` checks that the board-side buses are ready:
   - SPI1 for the SD card
   - I2C0 for the Bar30

2. `external_peripherals_init()` initializes the external devices:
   - Mounts the SD card as disk `SD`
   - Creates/opens `/SD:/data.txt`
   - Writes the CSV header
   - Initializes the BME280 SPI driver
   - Scans and initializes the Bar30 pressure sensor

After initialization, `main()` enters a measurement loop. The current loop runs for about 10 seconds:

```c
while (k_uptime_get() - test_counter < 10000)
```

Inside the loop:

- BME280 is read when its status is `pending`.
- Bar30 pressure is read when its status is `pending`.
- Every 1000 ms, the latest completed sensor values are written to the SD card.
- After a successful SD write, BME280 and Bar30 status are reset to `pending` so the next sample can be collected.

The status flow is:

```text
pending -> complete -> logged to SD -> pending
```

If a sensor is not `complete` at the time of logging, its CSV value is filled with all-ones sentinel data. For example:

- `uint8_t` fields use `255`
- `uint16_t` fields use `65535`
- `int32_t` pressure uses `-1`

## SD Card Output

The log file is:

```text
/SD:/data.txt
```

CSV format:

```csv
time(ms),temp(C),hum(%%),pres,rpm,x,y,z
1234,24,45,1013,255,255,255,255
2234,24,46,1014,255,255,255,255
```

Current columns:

| Column | Source | Notes |
| --- | --- | --- |
| `time(ms)` | `k_uptime_get()` | System uptime when the row is formatted |
| `temp(C)` | BME280 | Cast to `uint8_t` before logging |
| `hum(%%)` | BME280 | Cast to `uint16_t` before logging |
| `pres` | Bar30 | Pressure in mbar |
| `rpm` | Placeholder | Currently `255` |
| `x` | Placeholder | Currently `255` |
| `y` | Placeholder | Currently `255` |
| `z` | Placeholder | Currently `255` |

## Current Pin Mapping

Target board from the generated devicetree:

```text
nrf52840dk/nrf52840
```

### Bar30 Pressure Sensor

Bar30 uses I2C0 at address `0x76`.

| Signal | nRF52840 pin | Devicetree source |
| --- | --- | --- |
| SDA | `P0.26` | `app.overlay`, `i2c0_default` |
| SCL | `P0.27` | `app.overlay`, `i2c0_default` |

I2C speed is configured as standard mode:

```dts
clock-frequency = <I2C_BITRATE_STANDARD>;
```

### SD Card

The SD card uses SPI1 through Zephyr's SPI SDHC slot.

| Signal | nRF52840 pin | Notes |
| --- | --- | --- |
| SCK | `P0.31` | Resolved from board `spi1_default` |
| MOSI | `P0.30` | Resolved from board `spi1_default` |
| MISO | `P1.08` | Resolved from board `spi1_default` |
| CS | `P1.12` | Set in `app.overlay` |

SD card SPI max frequency:

```dts
spi-max-frequency = <4000000>;
```

### BME280

The BME280 uses SPI3 with a custom Bosch-driver wrapper.

| Signal | nRF52840 pin | Notes |
| --- | --- | --- |
| SCK | `P1.15` | Resolved from board `spi3_default` |
| MOSI | `P1.13` | Resolved from board `spi3_default` |
| MISO | `P1.14` | Resolved from board `spi3_default` |
| CS | `P1.11` | Set in `app.overlay` |

BME280 SPI frequency in the driver:

```c
.frequency = 1000000
```

### Enabled But Not Currently Used

`spi2` is enabled in `app.overlay` with CS on `P0.00`, but no current application code uses it.

## Important Files

| File | Purpose |
| --- | --- |
| `src/main.c` | Main init, measurement loop, status handling, SD writes |
| `src/main.h` | Device status enum and status struct |
| `driver/sd_card/` | SD card mount, file creation, CSV row writing |
| `driver/BME280/` | BME280 SPI wrapper around the Bosch driver |
| `driver/bar30/` | Bar30 reset, PROM read, ADC conversion, pressure compensation |
| `app.overlay` | Bus enablement, I2C pins, chip-select pins |
| `prj.conf` | Zephyr config for GPIO, SPI, I2C, SD, FAT filesystem, console |

## Build

Typical Zephyr build command for the current target:

```sh
west build -b nrf52840dk/nrf52840
```

Then flash with:

```sh
west flash
```

