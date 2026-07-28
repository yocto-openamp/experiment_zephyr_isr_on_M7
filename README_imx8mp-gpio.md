# GPIO

## libgpiod-tools

In your Yocto layers, libgpiod-tools is the package that installs the CLI binaries, including:

* gpioinfo
* gpiodetect
* gpiofind
* gpioget
* gpioset
* gpiomon

ls /dev/gpiochip*

gpioinfo

gpioset gpiochip0 23=1

gpioget gpiochip0 17

gpiomon gpiochip0 17

## Legacy sysfs interface (deprecated)

echo 23 > /sys/class/gpio/export

echo out > /sys/class/gpio/gpio23/direction

Drive high:

echo 1 > /sys/class/gpio/gpio23/value

Drive low:

echo 0 > /sys/class/gpio/gpio23/value


cat /sys/class/gpio/gpio23/value

echo 23 > /sys/class/gpio/unexport


## Mallow

| gpio | | |
| - | - | - |
| GPIO_STIMULI | GPIO1_IO00 | 
| GPIO_RESPONSE |  GPIO1_IO01 | 