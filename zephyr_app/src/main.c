#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/dac.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

#define ZEPHYR_USER_NODE DT_PATH(zephyr_user)

#if !DT_NODE_HAS_PROP(ZEPHYR_USER_NODE, gpio_in_gpios)
#error "Missing gpio-in-gpios in zephyr,user devicetree node"
#endif

#if !DT_NODE_HAS_PROP(ZEPHYR_USER_NODE, gpio_out_gpios)
#error "Missing gpio-out-gpios in zephyr,user devicetree node"
#endif

#define SW0_NODE DT_ALIAS(sw0)
#define LED0_NODE DT_ALIAS(led0)

#if !DT_NODE_HAS_STATUS(SW0_NODE, okay)
#error "Missing sw0 alias in board devicetree"
#endif

#if !DT_NODE_HAS_STATUS(LED0_NODE, okay)
#error "Missing led0 alias in board devicetree"
#endif

#define DAC1_NODE DT_NODELABEL(dac1)
#define ADC1_NODE DT_NODELABEL(adc1)
#define ADC2_NODE DT_NODELABEL(adc2)
#define ADC3_NODE DT_NODELABEL(adc3)

static const struct gpio_dt_spec gpio_in = GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, gpio_in_gpios);
static const struct gpio_dt_spec gpio_out = GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, gpio_out_gpios);
static const struct gpio_dt_spec sw0_in = GPIO_DT_SPEC_GET(SW0_NODE, gpios);
static const struct gpio_dt_spec led0_out = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

static const struct device *const dac1_dev = DEVICE_DT_GET(DAC1_NODE);
static const struct device *const adc1_dev = DEVICE_DT_GET(ADC1_NODE);
static const struct device *const adc2_dev = DEVICE_DT_GET(ADC2_NODE);
static const struct device *const adc3_dev = DEVICE_DT_GET(ADC3_NODE);

static struct gpio_callback gpio_in_cb;
static struct gpio_callback sw0_cb;

static void sw0_isr(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	int in_val = gpio_pin_get_dt(&sw0_in);
	if (in_val >= 0) {
		(void)gpio_pin_set_dt(&led0_out, in_val > 0);
	}
}

static void gpio_in_isr(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	int in_val = gpio_pin_get_dt(&gpio_in);
	if (in_val >= 0) {
		(void)gpio_pin_set_dt(&gpio_out, in_val > 0);
	}
}

static int init_sw0_led0_isr(void)
{
	int ret;

	if (!device_is_ready(sw0_in.port)) {
		LOG_ERR("sw0 input device not ready");
		return -ENODEV;
	}

	if (!device_is_ready(led0_out.port)) {
		LOG_ERR("led0 output device not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&sw0_in, GPIO_INPUT);
	if (ret != 0) {
		LOG_ERR("sw0 configure failed: %d", ret);
		return ret;
	}

	ret = gpio_pin_configure_dt(&led0_out, GPIO_OUTPUT_INACTIVE);
	if (ret != 0) {
		LOG_ERR("led0 configure failed: %d", ret);
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&sw0_in, GPIO_INT_EDGE_BOTH);
	if (ret != 0) {
		LOG_ERR("sw0 irq configure failed: %d", ret);
		return ret;
	}

	gpio_init_callback(&sw0_cb, sw0_isr, BIT(sw0_in.pin));
	ret = gpio_add_callback(sw0_in.port, &sw0_cb);
	if (ret != 0) {
		LOG_ERR("sw0 add callback failed: %d", ret);
		return ret;
	}

	LOG_INF("isrButton ready (in: port=%s pin=%d, out: port=%s pin=%d)",
		sw0_in.port->name, sw0_in.pin, led0_out.port->name, led0_out.pin);
	return 0;
}

static int init_gpio_isr(void)
{
	int ret;

	if (!device_is_ready(gpio_in.port)) {
		LOG_ERR("GPIO input device not ready");
		return -ENODEV;
	}

	if (!device_is_ready(gpio_out.port)) {
		LOG_ERR("GPIO output device not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&gpio_in, GPIO_INPUT);
	if (ret != 0) {
		LOG_ERR("gpio_in configure failed: %d", ret);
		return ret;
	}

	ret = gpio_pin_configure_dt(&gpio_out, GPIO_OUTPUT_INACTIVE);
	if (ret != 0) {
		LOG_ERR("gpio_out configure failed: %d", ret);
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&gpio_in, GPIO_INT_EDGE_BOTH);
	if (ret != 0) {
		LOG_ERR("gpio_in irq configure failed: %d", ret);
		return ret;
	}

	gpio_init_callback(&gpio_in_cb, gpio_in_isr, BIT(gpio_in.pin));
	ret = gpio_add_callback(gpio_in.port, &gpio_in_cb);
	if (ret != 0) {
		LOG_ERR("gpio_in add callback failed: %d", ret);
		return ret;
	}

	LOG_INF("isrGpio ready (in: port=%s pin=%d, out: port=%s pin=%d)",
		gpio_in.port->name, gpio_in.pin, gpio_out.port->name, gpio_out.pin);
	return 0;
}

static void init_analog_devices(void)
{
	if (device_is_ready(dac1_dev)) {
		struct dac_channel_cfg dac_cfg = {
			.channel_id = 1,
			.resolution = 12,
		};
		int ret = dac_channel_setup(dac1_dev, &dac_cfg);
		if (ret == 0) {
			LOG_INF("DAC1 channel 1 configured");
		} else {
			LOG_WRN("DAC1 channel setup failed: %d", ret);
		}
	} else {
		LOG_WRN("DAC1 not ready");
	}

	if (device_is_ready(adc1_dev)) {
		struct adc_channel_cfg adc_cfg = {
			.channel_id = 3,
			.gain = ADC_GAIN_1,
			.reference = ADC_REF_INTERNAL,
			.acquisition_time = ADC_ACQ_TIME_DEFAULT,
		};
		int ret = adc_channel_setup(adc1_dev, &adc_cfg);
		if (ret == 0) {
			LOG_INF("ADC1 channel 3 configured");
		} else {
			LOG_WRN("ADC1 channel setup failed: %d", ret);
		}
	} else {
		LOG_WRN("ADC1 not ready");
	}

	LOG_INF("ADC readiness: adc1=%d adc2=%d adc3=%d",
		device_is_ready(adc1_dev),
		device_is_ready(adc2_dev),
		device_is_ready(adc3_dev));
}

int main(void)
{
	int ret = init_sw0_led0_isr();
	if (ret != 0) {
		return ret;
	}

	ret = init_gpio_isr();
	if (ret != 0) {
		return ret;
	}

	init_analog_devices();

	uint16_t dac2_value = 0;
	float angle = 0.0;

	while (1) {
        // DAC1: Read ADC1 and write to DAC1
		if (device_is_ready(dac1_dev) && device_is_ready(adc1_dev)) {
			uint16_t adc1_value;
			struct adc_sequence seq = {
				.channels = BIT(3),
				.buffer = &adc1_value,
				.buffer_size = sizeof(adc1_value),
				.resolution = 12,
			};
			// Read ADC1
			if (adc_read(adc1_dev, &seq) == 0) {
				// Write ADC1 value to DAC1
				(void)dac_write_value(dac1_dev, 1, adc1_value);
			}
		}

        // DAC2: calculate a sin and write it to DAC2 as fast as possible.
		if (device_is_ready(dac1_dev)) {
			// Calculate sine wave value for DAC2
			dac2_value = (uint16_t)((sinf(angle) + 1.0) * 2047.5); // Scale to 12-bit range
			(void)dac_write_value(dac1_dev, 2, dac2_value);
			angle += 0.1; // Increment angle
			if (angle >= 2 * M_PI) {
				angle -= 2 * M_PI;
			}
		}
	}
}
