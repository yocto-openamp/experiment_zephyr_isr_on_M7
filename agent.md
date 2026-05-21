# Zephyr on M7

## Required

* NUCLEO-F722ZE

  * STM32F722ZE MCU
    * https://www.st.com/en/microcontrollers-microprocessors/stm32f722ze.html
    * https://www.st.com/resource/en/datasheet/stm32f722ze.pdf
    * https://www.st.com/resource/en/user_manual/dm00244518-stm32-nucleo144-boardsstmicroelectronics.pdf. See chapter "Extension connectors"

* Zephyr & C

## Application

Idle time:

* DAC1: Read ADC1 and write to DAC1
* DAC2: calculate a sin and write it to DAC2 as fast as possible.

ISR:

* isrButton: Call a isr: The isrButton should read the `user button` and write the `user led`.
* isrGpio: GPIO_IN: The isrGpio should write the value of GPIO_IN to GPIO_OUT.
* isrGpioDirect: GPIO_DIRECT_IN: The isrGpioDirect should write the value of GPIO_DIRECT_IN to GPIO_DIRECT_OUT.


### ISR pin mapping

* isrButton:
  * GPIO_IN: PC13 (user button, alias `sw0`)
  * GPIO_OUT: PB0 (user LED, alias `led0`)
* isrGpio:
  * GPIO_IN: PG9 (CN10/D0 pin 16)
  * GPIO_OUT: PG14 (CN10/D1 pin 14)
* isrGpioDirect:
  * GPIO_DIRECT_IN: PF15 (CN10/D2 pin 12)
  * GPIO_DIRECT_OUT: PE13 (CN10/D3 pin 10)

Note: isrGpio pins (PG9/PG14) are independent from ADC/DAC pins. ADC/DAC stays on the analog-capable pins documented above.


## List of PINs on the NUCLEO-F722ZE

### Zephyr board and SoC naming (Devicetree)

Internal peripheral node labels (STM32F722):

* ADC peripherals: `adc1`, `adc2`, `adc3`
* DAC peripheral: `dac1`
  * DAC channel naming is channel-based inside `dac1`:
    * DAC channel 1 = PA4
    * DAC channel 2 = PA5
* GPIO ports: `gpioa`, `gpiob`, `gpioc`, `gpiod`, `gpioe`, `gpiof`, `gpiog`, `gpioh`, `gpioi`
* External interrupt controller: `exti`

Board-level aliases already defined on `nucleo_f722ze`:

* `sw0` -> user button on PC13
* `led0` -> user LED LD1 on PB0
* `led1` -> user LED LD2 on PB7
* `led2` -> user LED LD3 on PB14

### Pins relevant for this application

Pins already wired in the Zephyr board DTS for analog/peripheral use:

* ADC1:
  * IN3 -> PA3 (Arduino A0, CN9 pin 1)
  * IN10 -> PC0 (Arduino A1, CN9 pin 3)
* DAC1:
  * OUT1 -> PA4 (CN7 pin 17)

Important for your goals:

* `adc2` and `adc3` exist in the STM32F722 SoC, but are not enabled in the default board DTS.
  * We should enable/configure them in an overlay for your app.
* There is no separate `dac2` peripheral node on this MCU in Zephyr DTS.
  * What is often called "DAC2" is DAC channel 2 of `dac1` (typically PA5).




