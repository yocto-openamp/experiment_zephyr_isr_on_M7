#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#if defined(CONFIG_ADC)
#include <zephyr/drivers/dac.h>
#include <zephyr/drivers/adc.h>
#endif // CONFIG_ADC
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <zephyr/init.h>
#include <zephyr/sys/printk.h>
#if defined(CONFIG_RPMSG_SERVICE)
#include <zephyr/ipc/rpmsg_service.h>
#endif
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

#define ZEPHYR_USER_NODE DT_PATH(zephyr_user)

#if !DT_NODE_HAS_PROP(ZEPHYR_USER_NODE, gpio_direct_button_in_gpios)
#error "Missing gpio-direct-button-in-gpios in zephyr,user devicetree node"
#endif

#if !DT_NODE_HAS_PROP(ZEPHYR_USER_NODE, gpio_direct_led_out_gpios)
#error "Missing gpio-direct-led-out-gpios in zephyr,user devicetree node"
#endif

#if !DT_NODE_HAS_PROP(ZEPHYR_USER_NODE, gpio_direct_in_gpios)
#error "Missing gpio-direct-in-gpios in zephyr,user devicetree node"
#endif

#if !DT_NODE_HAS_PROP(ZEPHYR_USER_NODE, gpio_direct_out_gpios)
#error "Missing gpio-direct-out-gpios in zephyr,user devicetree node"
#endif

#define SW0_NODE DT_ALIAS(sw0)

#if !DT_NODE_HAS_STATUS(SW0_NODE, okay)
#error "Missing sw0 alias in board devicetree"
#endif

#if defined(CONFIG_ADC)
#define DAC1_NODE DT_NODELABEL(dac1)
#define ADC1_NODE DT_NODELABEL(adc1)
#define ADC2_NODE DT_NODELABEL(adc2)
#define ADC3_NODE DT_NODELABEL(adc3)
#endif // CONFIG_ADC

#define EXTI_GPIO_DIRECT_BUTTON_IN_LINE BIT(gpio_direct_button_in.pin)
#define EXTI_GPIO_DIRECT_IN_LINE BIT(gpio_direct_in.pin)
#define EXTI_DIRECT_LINES (EXTI_GPIO_DIRECT_BUTTON_IN_LINE | EXTI_GPIO_DIRECT_IN_LINE)

#define GPIO_DIRECT_BUTTON_IN ((GPIO_TypeDef *)DT_REG_ADDR(DT_GPIO_CTLR(ZEPHYR_USER_NODE, gpio_direct_button_in_gpios)))
#define GPIO_DIRECT_LED_OUT ((GPIO_TypeDef *)DT_REG_ADDR(DT_GPIO_CTLR(ZEPHYR_USER_NODE, gpio_direct_led_out_gpios)))
#define GPIO_DIRECT_IN ((GPIO_TypeDef *)DT_REG_ADDR(DT_GPIO_CTLR(ZEPHYR_USER_NODE, gpio_direct_in_gpios)))
#define GPIO_DIRECT_OUT ((GPIO_TypeDef *)DT_REG_ADDR(DT_GPIO_CTLR(ZEPHYR_USER_NODE, gpio_direct_out_gpios)))

#define GPIO_DIRECT_IRQ_PRIO 0
#if defined(CONFIG_ZERO_LATENCY_IRQS)
#define GPIO_DIRECT_IRQ_FLAGS IRQ_ZERO_LATENCY
#else
#define GPIO_DIRECT_IRQ_FLAGS 0
#endif

#if DT_NODE_HAS_PROP(ZEPHYR_USER_NODE, gpio_hal_in_gpios)
static const struct gpio_dt_spec gpio_hal_in = GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, gpio_hal_in_gpios);
static const struct gpio_dt_spec gpio_hal_out = GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, gpio_hal_out_gpios);
#endif // gpio_hal_in_gpios
static const struct gpio_dt_spec gpio_direct_button_in = GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, gpio_direct_button_in_gpios);
static const struct gpio_dt_spec gpio_direct_led_out = GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, gpio_direct_led_out_gpios);
static const struct gpio_dt_spec gpio_direct_in = GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, gpio_direct_in_gpios);
static const struct gpio_dt_spec gpio_direct_out = GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, gpio_direct_out_gpios);

#if defined(CONFIG_ADC)
static const struct device *const dac1_dev = DEVICE_DT_GET(DAC1_NODE);
static const struct device *const adc1_dev = DEVICE_DT_GET(ADC1_NODE);
static const struct device *const adc2_dev = DEVICE_DT_GET(ADC2_NODE);
static const struct device *const adc3_dev = DEVICE_DT_GET(ADC3_NODE);
#endif // CONFIG_ADC

#if DT_NODE_HAS_PROP(ZEPHYR_USER_NODE, gpio_hal_in_gpios)
static struct gpio_callback gpio_callback_isr_gpioHAL;
#endif // gpio_hal_in_gpios

#if defined(CONFIG_RPMSG_SERVICE)
#ifndef CONFIG_RPMSG_SERVICE_DEMO_CYCLES
#define CONFIG_RPMSG_SERVICE_DEMO_CYCLES 100
#endif

K_THREAD_STACK_DEFINE(rpmsg_thread_stack, CONFIG_MAIN_STACK_SIZE);
static struct k_thread rpmsg_thread_data;

static volatile unsigned int rpmsg_received_data;
static K_SEM_DEFINE(rpmsg_data_rx_sem, 0, 1);
static int rpmsg_ep_id;

static int rpmsg_endpoint_cb(struct rpmsg_endpoint *ept, void *data, size_t len,
			     uint32_t src, void *priv)
{
	ARG_UNUSED(ept);
	ARG_UNUSED(len);
	ARG_UNUSED(src);
	ARG_UNUSED(priv);

	rpmsg_received_data = *((unsigned int *)data);
	k_sem_give(&rpmsg_data_rx_sem);

	return RPMSG_SUCCESS;
}

static unsigned int rpmsg_receive_message(void)
{
	k_sem_take(&rpmsg_data_rx_sem, K_FOREVER);
	return rpmsg_received_data;
}

static int rpmsg_send_message(unsigned int message)
{
	return rpmsg_service_send(rpmsg_ep_id, &message, sizeof(message));
}

static void rpmsg_app_task(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	int status;
	unsigned int message = 0U;

	printk("\r\nRPMsg Service [master] demo started\r\n");

	while (!rpmsg_service_endpoint_is_bound(rpmsg_ep_id)) {
		k_sleep(K_MSEC(1));
	}

	while (message < CONFIG_RPMSG_SERVICE_DEMO_CYCLES) {
		status = rpmsg_send_message(message);
		if (status < 0) {
			printk("send_message(%u) failed with status %d\n", message, status);
			break;
		}

		message = rpmsg_receive_message();
		printk("Master core received a message: %u\n", message);

		message++;
	}

	printk("RPMsg Service demo ended.\n");
}

/* Register endpoint before RPMsg Service initialization. */
static int rpmsg_register_endpoint(void)
{
	int status;

	status = rpmsg_service_register_endpoint("demo", rpmsg_endpoint_cb);
	if (status < 0) {
		printk("rpmsg_create_ept failed %d\n", status);
		return status;
	}

	rpmsg_ep_id = status;
	return 0;
}

SYS_INIT(rpmsg_register_endpoint, POST_KERNEL, CONFIG_RPMSG_SERVICE_EP_REG_PRIORITY);
#endif // CONFIG_RPMSG_SERVICE

/*
ISR_GPIO_DIRECT

The Direct Interrupt Service Routine (ISR) bypasses the zephyr HAL
*/

#ifdef EXTI
ISR_DIRECT_DECLARE(exti15_10_direct_isr)
{
	uint32_t pending = EXTI->PR & EXTI_DIRECT_LINES;

	if (pending == 0U) {
		return 0;
	}

	/* Clear pending bits first to minimize IRQ service latency. */
	EXTI->PR = pending;

	if ((pending & EXTI_GPIO_DIRECT_IN_LINE) != 0U) {
		if ((GPIO_DIRECT_IN->IDR & BIT(gpio_direct_in.pin)) != 0U) {
			GPIO_DIRECT_OUT->BSRR = BIT(gpio_direct_out.pin);
		} else {
			GPIO_DIRECT_OUT->BSRR = BIT(gpio_direct_out.pin + 16);
		}
	}

	if ((pending & EXTI_GPIO_DIRECT_BUTTON_IN_LINE) != 0U) {
		if ((GPIO_DIRECT_BUTTON_IN->IDR & BIT(gpio_direct_button_in.pin)) != 0U) {
			GPIO_DIRECT_LED_OUT->BSRR = BIT(gpio_direct_led_out.pin);
		} else {
			GPIO_DIRECT_LED_OUT->BSRR = BIT(gpio_direct_led_out.pin + 16);
		}
	}

	return 0;
}
#endif // EXTI

static int init_isr_gpioDirect(void)
{
	int ret;

	/*
	gpio_direct_button_in
	gpio_direct_led_out
	*/
	if (!device_is_ready(gpio_direct_button_in.port)) {
		LOG_ERR("button input device not ready");
		return -ENODEV;
	}

	if (!device_is_ready(gpio_direct_led_out.port)) {
		LOG_ERR("led0 output device not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&gpio_direct_led_out, GPIO_OUTPUT_INACTIVE);
	if (ret != 0) {
		LOG_ERR("led0 configure failed: %d", ret);
		return ret;
	}

	ret = gpio_pin_configure_dt(&gpio_direct_button_in, GPIO_INPUT);
	if (ret != 0) {
		LOG_ERR("gpio_direct_button_in configure failed: %d", ret);
		return ret;
	}

	LOG_INF("isr_button_led ready (in: port=%s pin=%d, out: port=%s pin=%d)",
		gpio_direct_button_in.port->name, gpio_direct_button_in.pin, gpio_direct_led_out.port->name, gpio_direct_led_out.pin);

    /*
	gpio_direct_in
	gpio_direct_out
	*/
	if (!device_is_ready(gpio_direct_in.port)) {
		LOG_ERR("GPIO_DIRECT_IN device not ready");
		return -ENODEV;
	}

	if (!device_is_ready(gpio_direct_out.port)) {
		LOG_ERR("GPIO_DIRECT_OUT device not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&gpio_direct_in, GPIO_INPUT);
	if (ret != 0) {
		LOG_ERR("gpio_direct_in configure failed: %d", ret);
		return ret;
	}

	ret = gpio_pin_configure_dt(&gpio_direct_out, GPIO_OUTPUT_INACTIVE);
	if (ret != 0) {
		LOG_ERR("gpio_direct_out configure failed: %d", ret);
		return ret;
	}

#ifdef EXTI
	/* PC13 (button) and PF15 (GPIO_DIRECT_IN) share EXTI15_10. Handle both directly. */
	RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
	SYSCFG->EXTICR[3] &= ~(SYSCFG_EXTICR4_EXTI13 | SYSCFG_EXTICR4_EXTI15);
	SYSCFG->EXTICR[3] |= SYSCFG_EXTICR4_EXTI13_PC | SYSCFG_EXTICR4_EXTI15_PF;

	EXTI->IMR |= EXTI_DIRECT_LINES;
	EXTI->RTSR |= EXTI_DIRECT_LINES;
	EXTI->FTSR |= EXTI_DIRECT_LINES;
	EXTI->PR = EXTI_DIRECT_LINES;

	IRQ_DIRECT_CONNECT(EXTI15_10_IRQn, GPIO_DIRECT_IRQ_PRIO,
			exti15_10_direct_isr, GPIO_DIRECT_IRQ_FLAGS);
	irq_enable(EXTI15_10_IRQn);

	/* EXTI15_10 is serviced by the direct IRQ handler for low-latency comparison. */
#endif // EXTI

	LOG_INF("ISR_GPIO_DIRECT ready (in: port=%s pin=%d, out: port=%s pin=%d)",
		gpio_direct_in.port->name, gpio_direct_in.pin,
		gpio_direct_out.port->name, gpio_direct_out.pin);
	
	return 0;
}

/*
isrGpioHAL

Interrupt Service Routine (ISR) using the zephyr HAL
*/
#if DT_NODE_HAS_PROP(ZEPHYR_USER_NODE, gpio_hal_in_gpios)
static void callback_isr_gpioHAL(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	int in_val = gpio_pin_get_dt(&gpio_hal_in);
	if (in_val >= 0) {
		(void)gpio_pin_set_dt(&gpio_hal_out, in_val > 0);
	}
}

static int init_isr_gpioHAL(void)
{
	int ret;

	if (!device_is_ready(gpio_hal_in.port)) {
		LOG_ERR("GPIO input device not ready");
		return -ENODEV;
	}

	if (!device_is_ready(gpio_hal_out.port)) {
		LOG_ERR("GPIO output device not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&gpio_hal_in, GPIO_INPUT);
	if (ret != 0) {
		LOG_ERR("gpio_hal_in configure failed: %d", ret);
		return ret;
	}

	ret = gpio_pin_configure_dt(&gpio_hal_out, GPIO_OUTPUT_INACTIVE);
	if (ret != 0) {
		LOG_ERR("gpio_hal_out configure failed: %d", ret);
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&gpio_hal_in, GPIO_INT_EDGE_BOTH);
	if (ret != 0) {
		LOG_ERR("gpio_hal_in irq configure failed: %d", ret);
		return ret;
	}

	gpio_init_callback(&gpio_callback_isr_gpioHAL, callback_isr_gpioHAL, BIT(gpio_hal_in.pin));
	ret = gpio_add_callback(gpio_hal_in.port, &gpio_callback_isr_gpioHAL);
	if (ret != 0) {
		LOG_ERR("gpio_hal_in add callback failed: %d", ret);
		return ret;
	}

	LOG_INF("ISR_GPIO_HAL ready (in: port=%s pin=%d, out: port=%s pin=%d)",
		gpio_hal_in.port->name, gpio_hal_in.pin, gpio_hal_out.port->name, gpio_hal_out.pin);
	return 0;
}
#endif // gpio_hal_in_gpios

/*
DAC/ADC
*/
#if defined(CONFIG_ADC)
static void init_adc_dac(void)
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

static void endless_loop_adc_dac(void)
{
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
			dac2_value = (uint16_t)((sinf(angle) + 1.0f) * 2047.5f); // Scale to 12-bit range
			(void)dac_write_value(dac1_dev, 2, dac2_value);
			angle += 0.1f; // Increment angle
			if (angle >= 2 * M_PI) {
				angle -= 2 * M_PI;
			}
		}
	}
}
#endif // CONFIG_ADC

int main(void)
{
	int ret = init_isr_gpioDirect();
	if (ret != 0) {
		return ret;
	}


#if DT_NODE_HAS_PROP(ZEPHYR_USER_NODE, gpio_hal_in_gpios)
	ret = init_isr_gpioHAL();
	if (ret != 0) {
		return ret;
	}
#endif // gpio_hal_in_gpios

#if defined(CONFIG_RPMSG_SERVICE)
	printk("Starting RPMsg application thread!\n");
	k_thread_create(&rpmsg_thread_data, rpmsg_thread_stack, CONFIG_MAIN_STACK_SIZE,
			rpmsg_app_task, NULL, NULL, NULL, K_PRIO_COOP(7), 0, K_NO_WAIT);
#if defined(CONFIG_SOC_AN521) || defined(CONFIG_SOC_MUSCA_B1)
	wakeup_cpu1();
	k_msleep(500);
#endif
#endif // CONFIG_RPMSG_SERVICE

#if defined(CONFIG_ADC)
	init_adc_dac();

	endless_loop_adc_dac();
#endif // CONFIG_ADC

	return 0;
}
