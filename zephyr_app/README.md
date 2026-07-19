# Zephyr App (NUCLEO-F722ZE)

## Install west using uv

To install `west`, use the following command:

```sh
uv venv --python 3.13.13
. .venv/bin/activate
uv pip install west
west init .
west update

uv pip install -r zephyr/scripts/requirements-base.txt
```

## Install Zephyr SDK

```sh
$ west sdk list
FATAL ERROR: No Zephyr SDK installed.
$ west sdk install
```

## Build

From the repository root:

```sh
west build -d build/nucleo_f722ze -b nucleo_f722ze zephyr_app
```

## Flash

* Connect USB to CN1 of NUCLEO-F722ZE

```sh
west flash -d build/nucleo_f722ze
```

## Testing

* `tio /dev/ttyACM0` to see log messages
* ISR_GPIO_BUTTON_LED_HAL: Press the blue button -> green LD1
* ISR_GPIO_DIRECT: GPIO_DIRECT_IN -> GPIO_DIRECT_OUT

  ```python
  uvx mpremote a1 run response_time_analyser.py
  ```

## Current Status

- GPIO ISR mirror scaffold: `PC13 -> PB0` (button to LED)
- Board overlay enables `adc2`, `adc3`, and `dac1` (including DAC2 as channel 2 of dac1)
- Analog data-path is scaffolded and ready for the next iteration
