#ifndef EXPORT_SYMTAB
#define EXPORT_SYMTAB
#endif

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/signal.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/init.h>
#include <linux/resource.h>
#include <linux/proc_fs.h>
#include <linux/miscdevice.h>
#include <asm/types.h>
#include <asm/irq.h>
#include <asm/delay.h>
#include <asm/system.h>

#include <atheros.h>

#define ATH_FACTORY_RESET		0x89ABCDEF

static atomic_t ath_fr_status = ATOMIC_INIT(0);
static volatile int ath_fr_opened = 0;
static wait_queue_head_t ath_fr_wq;
static u_int32_t push_time = 0;
struct timer_list os_timer_t;
struct timer_list os_butdet_t; /* Button detect */
struct timer_list os_staled_t; /* Status LED blinking */
struct timer_list os_diswps_t; /* WPS LED blinking while WPS error occured,

                                   like push button is pressed if WPS was disabled */
/* AR9341 GPIOs */
#define WAN_AMBER_LED_GPIO      22
#define WAN_GREEN_LED_GPIO      17
#define WPS_LED_GPIO            2
#define WLAN_LED_GPIO           12
#define STATUS_LED_GPIO         1
#define POWER_LED_GPIO          0
#define WPS_BUTTON_GPIO         3
#define WLAN_BUTTON_GPIO        11
#define RESET_BUTTON_GPIO       4

#define frdbg printk

#define WPS_LED_OFF	1
#define WPS_LED_ON	0

#define USB_LED_OFF 1
#define USB_LED_ON 0

#define SIMPLE_CONFIG_OFF	1
#define SIMPLE_CONFIG_ON	2
#define SIMPLE_CONFIG_BLINK	3

#define LED_ON 0
#define LED_OFF 1

#define POLL_INTERVAL           100     /* millisecond */
#define TIME_5SECONDS           5000    /* millisecond */
#define COUNT_5SECONDS          50      /* (TIME_5SECONDS / POLL_INTERVAL) */
#define TIME_750MSECONDS        750     /* millisecond */
#define TIME_250MSECONDS        250     /* millisecond */
#define WPS_LED_ERR_INTERVAL    100     /* millisecond */

typedef enum {
        free = 0,       /* release button, pop up */
        push = 1,       /* press button */
        hold = 2,       /* press button and hold pushed state */
}but_state_t;
static but_state_t wifibut = free;      /* WLAN switch */
static but_state_t pushbut = free;      /* Push button */
static but_state_t resetbut = free;     /* Reset button */

static int rstbut_count = 0;

#define OS_TIMER_FUNC(_fn)	\
	void _fn(unsigned long timer_arg)

#define OS_GET_TIMER_ARG(_arg, _type)	\
	(_arg) = (_type)(timer_arg)

#define OS_INIT_TIMER(_osdev, _timer, _fn, _arg)	\
do {							\
	init_timer(_timer);				\
	(_timer)->function = (_fn);			\
	(_timer)->data = (unsigned long)(_arg);		\
} while (0)

#define OS_SET_TIMER(_timer, _ms)	\
	mod_timer(_timer, jiffies + ((_ms)*HZ)/1000)

#define OS_CANCEL_TIMER(_timer)		del_timer(_timer)

/*
 * GPIO interrupt stuff
 */
typedef enum {
	INT_TYPE_EDGE,
	INT_TYPE_LEVEL,
} ath_gpio_int_type_t;

typedef enum {
	INT_POL_ACTIVE_LOW,
	INT_POL_ACTIVE_HIGH,
} ath_gpio_int_pol_t;

/*
** Simple Config stuff
*/
typedef irqreturn_t (*sc_callback_t) (int, void *, void *, void *);

/*
 * Multiple Simple Config callback support
 * For multiple radio scenarios, we need to post the button push to
 * all radios at the same time.  However, there is only 1 button, so
 * we only have one set of GPIO callback pointers.
 *
 * Creating a structure that contains each callback, tagged with the
 * name of the device registering the callback.  The unregister routine
 * will need to determine which element to "unregister", so the device
 * name will have to be passed to unregister also
 */

typedef struct {
	char		*name;
	sc_callback_t	registered_cb;
	void		*cb_arg1;
	void		*cb_arg2;
} multi_callback_t;

/*
 * Specific instance of the callback structure
 */
static multi_callback_t sccallback[2];
static volatile int ignore_pushbutton = 0;
static struct proc_dir_entry *simple_config_entry = NULL;
static struct proc_dir_entry *simulate_push_button_entry = NULL;
static struct proc_dir_entry *simple_config_led_entry = NULL;
static struct proc_dir_entry *wps_entry  = NULL;
static struct proc_dir_entry *button_state = NULL;
static struct proc_dir_entry *disable_wps = NULL; /* WPS is disabled? write into: 1 => disable, 0 => enable */
static int diswps = 0;  /* 0 => WPS is enabled, 1 => WPS is disabled */
static int diswps_blink_count = 0; /* max value: 50, blinking 5 seconds */

static int wps_led_blinking = 0;


#define DNI_SIMPLE_CONFIG_SET 1
#if DNI_SIMPLE_CONFIG_SET
static struct proc_dir_entry *scset_entry; /* simple config set entry, configure all GPIOs */
#endif

void ath_gpio_config_int(int gpio,
			 ath_gpio_int_type_t type,
			 ath_gpio_int_pol_t polarity)
{
	u32 val;

	/*
	 * allow edge sensitive/rising edge too
	 */
	if (type == INT_TYPE_LEVEL) {
		/* level sensitive */
		ath_reg_rmw_set(ATH_GPIO_INT_TYPE, (1 << gpio));
	} else {
		/* edge triggered */
		val = ath_reg_rd(ATH_GPIO_INT_TYPE);
		val &= ~(1 << gpio);
		ath_reg_wr(ATH_GPIO_INT_TYPE, val);
	}

	if (polarity == INT_POL_ACTIVE_HIGH) {
		ath_reg_rmw_set(ATH_GPIO_INT_POLARITY, (1 << gpio));
	} else {
		val = ath_reg_rd(ATH_GPIO_INT_POLARITY);
		val &= ~(1 << gpio);
		ath_reg_wr(ATH_GPIO_INT_POLARITY, val);
	}

	ath_reg_rmw_set(ATH_GPIO_INT_ENABLE, (1 << gpio));
}

void ath_gpio_config_output(int gpio)
{
#ifdef CONFIG_MACH_AR934x
	ath_reg_rmw_clear(ATH_GPIO_OE, (1 << gpio));
#else
	ath_reg_rmw_set(ATH_GPIO_OE, (1 << gpio));
#endif
}
EXPORT_SYMBOL(ath_gpio_config_output);

void ath_gpio_config_input(int gpio)
{
#ifdef CONFIG_MACH_AR934x
	ath_reg_rmw_set(ATH_GPIO_OE, (1 << gpio));
#else
	ath_reg_rmw_clear(ATH_GPIO_OE, (1 << gpio));
#endif
}

void ath_gpio_out_val(int gpio, int val)
{
	if (val & 0x1) {
		ath_reg_rmw_set(ATH_GPIO_OUT, (1 << gpio));
	} else {
		ath_reg_rmw_clear(ATH_GPIO_OUT, (1 << gpio));
	}
}
EXPORT_SYMBOL(ath_gpio_out_val);

int ath_gpio_in_val(int gpio)
{
	return ((1 << gpio) & (ath_reg_rd(ATH_GPIO_IN)));
}

static void
ath_gpio_intr_enable(unsigned int irq)
{
	ath_reg_rmw_set(ATH_GPIO_INT_MASK,
				(1 << (irq - ATH_GPIO_IRQ_BASE)));
}

static void
ath_gpio_intr_disable(unsigned int irq)
{
	ath_reg_rmw_clear(ATH_GPIO_INT_MASK,
				(1 << (irq - ATH_GPIO_IRQ_BASE)));
}

static unsigned int
ath_gpio_intr_startup(unsigned int irq)
{
	ath_gpio_intr_enable(irq);
	return 0;
}

static void
ath_gpio_intr_shutdown(unsigned int irq)
{
	ath_gpio_intr_disable(irq);
}

static void
ath_gpio_intr_ack(unsigned int irq)
{
	ath_gpio_intr_disable(irq);
}

static void
ath_gpio_intr_end(unsigned int irq)
{
	if (!(irq_desc[irq].status & (IRQ_DISABLED | IRQ_INPROGRESS)))
		ath_gpio_intr_enable(irq);
}

static int
ath_gpio_intr_set_affinity(unsigned int irq, const struct cpumask *dest)
{
	/*
	 * Only 1 CPU; ignore affinity request
	 */
	return 0;
}

struct irq_chip /* hw_interrupt_type */ ath_gpio_intr_controller = {
	.name = "ATH GPIO",
	.startup = ath_gpio_intr_startup,
	.shutdown = ath_gpio_intr_shutdown,
	.enable = ath_gpio_intr_enable,
	.disable = ath_gpio_intr_disable,
	.ack = ath_gpio_intr_ack,
	.end = ath_gpio_intr_end,
	.eoi = ath_gpio_intr_end,
	.set_affinity = ath_gpio_intr_set_affinity,
};

void ath_gpio_irq_init(int irq_base)
{
	int i;

	for (i = irq_base; i < irq_base + ATH_GPIO_IRQ_COUNT; i++) {
		irq_desc[i].status = IRQ_DISABLED;
		irq_desc[i].action = NULL;
		irq_desc[i].depth = 1;
		//irq_desc[i].chip = &ath_gpio_intr_controller;
		set_irq_chip_and_handler(i, &ath_gpio_intr_controller,
					 handle_percpu_irq);
	}
}

int32_t register_simple_config_callback(char *cbname, void *callback, void *arg1, void *arg2)
{
	printk("SC Callback Registration for %s\n", cbname);
	if (!sccallback[0].name) {
		sccallback[0].name = (char*)kmalloc(strlen(cbname), GFP_KERNEL);
		strcpy(sccallback[0].name, cbname);
		sccallback[0].registered_cb = (sc_callback_t) callback;
		sccallback[0].cb_arg1 = arg1;
		sccallback[0].cb_arg2 = arg2;
	} else if (!sccallback[1].name) {
		sccallback[1].name = (char*)kmalloc(strlen(cbname), GFP_KERNEL);
		strcpy(sccallback[1].name, cbname);
		sccallback[1].registered_cb = (sc_callback_t) callback;
		sccallback[1].cb_arg1 = arg1;
		sccallback[1].cb_arg2 = arg2;
	} else {
		printk("@@@@ Failed SC Callback Registration for %s\n", cbname);
		return -1;
	}
	return 0;
}
EXPORT_SYMBOL(register_simple_config_callback);

int32_t unregister_simple_config_callback(char *cbname)
{
	if (sccallback[1].name && strcmp(sccallback[1].name, cbname) == 0) {
		kfree(sccallback[1].name);
		sccallback[1].name = NULL;
		sccallback[1].registered_cb = NULL;
		sccallback[1].cb_arg1 = NULL;
		sccallback[1].cb_arg2 = NULL;
	} else if (sccallback[0].name && strcmp(sccallback[0].name, cbname) == 0) {
		kfree(sccallback[0].name);
		sccallback[0].name = NULL;
		sccallback[0].registered_cb = NULL;
		sccallback[0].cb_arg1 = NULL;
		sccallback[0].cb_arg2 = NULL;
	} else {
		printk("!&!&!&!& ERROR: Unknown callback name %s\n", cbname);
		return -1;
	}
	return 0;
}
EXPORT_SYMBOL(unregister_simple_config_callback);

static OS_TIMER_FUNC(wps_led_blink)
{
	static int WPSled = WPS_LED_ON, sec = 0;
	ath_gpio_out_val(WPS_LED_GPIO, WPSled);
	WPSled = !WPSled;
	sec++;
	if (sec < 130) {
		OS_SET_TIMER(&os_timer_t, 1000);
	} else {
		sec = 0;
		wps_led_blinking = 0;
		OS_CANCEL_TIMER(&os_timer_t);
		ath_gpio_out_val(WPS_LED_GPIO, WPS_LED_OFF);
	}
}

int ath_simple_config_invoke_cb(int simplecfg_only, int irq_enable, int cpl)
{
	printk("%s: sc %d, irq %d, ignorepb %d, jiffies %lu\n", __func__,
		simplecfg_only, irq_enable, ignore_pushbutton, jiffies);
	if (simplecfg_only) {
		if (ignore_pushbutton) {
			ath_gpio_config_int(JUMPSTART_GPIO, INT_TYPE_LEVEL,
						INT_POL_ACTIVE_HIGH);
			ignore_pushbutton = 0;
			push_time = jiffies;
			return IRQ_HANDLED;
		}

		ath_gpio_config_int(JUMPSTART_GPIO, INT_TYPE_LEVEL,
					INT_POL_ACTIVE_LOW);
		ignore_pushbutton = 1;
	}

	if (irq_enable)
		local_irq_enable();

	if (push_time) {
		push_time = jiffies - push_time;
	}
	printk ("simple_config callback.. push dur in sec %d\n", push_time/HZ);

	if (sccallback[0].registered_cb) {
		if (sccallback[0].cb_arg2) {
			*(u_int32_t *)sccallback[0].cb_arg2 = push_time/HZ;
		}
		sccallback[0].registered_cb (cpl, sccallback[0].cb_arg1, NULL, sccallback[0].cb_arg2);
	}
	if (sccallback[1].registered_cb) {
		if (sccallback[1].cb_arg2) {
			*(u_int32_t *)sccallback[1].cb_arg2 = push_time/HZ;
		}
		sccallback[1].registered_cb (cpl, sccallback[1].cb_arg1, NULL, sccallback[1].cb_arg2);
	}

	return IRQ_HANDLED;
}

/*
 * Irq for front panel SW jumpstart switch
 * Connected to XSCALE through GPIO4
 */
irqreturn_t jumpstart_irq(int cpl, void *dev_id)
{
	unsigned int delay;

	if (atomic_read(&ath_fr_status)) {
		local_irq_disable();

#define UDELAY_COUNT 4000

		for (delay = UDELAY_COUNT; delay; delay--) {
			if (ath_gpio_in_val(JUMPSTART_GPIO)) {
				break;
			}
			udelay(1000);
		}

		if (!delay) {
			atomic_dec(&ath_fr_status);
			/*
			 * since we are going to reboot the board, we
			 * don't need the interrupt handler anymore,
			 * so disable it.
			 */
			disable_irq(ATH_GPIO_IRQn(JUMPSTART_GPIO));
			wake_up(&ath_fr_wq);
			printk("\nath: factory configuration restored..\n");
			local_irq_enable();
			return IRQ_HANDLED;
		} else {
			return (ath_simple_config_invoke_cb
				(0, 1, cpl));
		}
	} else
		return (ath_simple_config_invoke_cb(1, 0, cpl));
}

static int push_button_read(char *page, char **start, off_t off,
				int count, int *eof, void *data)
{
	return 0;
}

static int push_button_write(struct file *file, const char *buf,
				unsigned long count, void *data)
{
	if (sccallback[0].registered_cb) {
		sccallback[0].registered_cb (0, sccallback[0].cb_arg1, 0, sccallback[0].cb_arg2);
	}
	if (sccallback[1].registered_cb) {
		sccallback[1].registered_cb (0, sccallback[1].cb_arg1, 0, sccallback[1].cb_arg2);
	}
	return count;
}


static int wps_read (char *page, char **start, off_t off,
                               int count, int *eof, void *data)
{
        if (sccallback[0].registered_cb) {
                if (sccallback[0].cb_arg2) {
                        *(u_int32_t *)sccallback[0].cb_arg2 = 0;
                }
        sccallback[0].registered_cb (0, sccallback[0].cb_arg1, (struct pt_regs *)"info",  \
                sccallback[0].cb_arg2);
        }
        if (sccallback[1].registered_cb) {
                if (sccallback[1].cb_arg2) {
                        *(u_int32_t *)sccallback[1].cb_arg2 = 0;
        }
        sccallback[1].registered_cb (0, sccallback[1].cb_arg1, (struct pt_regs *)"info",  \
                sccallback[1].cb_arg2);
        }
        return 0;
}

static int wps_write (struct file *file, const char *buf,
                                        unsigned long count, void *data)
{
        if (0 != strncmp(buf,"pin=",4) && 0 != strncmp(buf,"stop",4))
                return count;

        if (sccallback[0].registered_cb) {
                if (sccallback[0].cb_arg2) {
                        *(u_int32_t *)sccallback[0].cb_arg2 = 0;
                }
                sccallback[0].registered_cb (0, sccallback[0].cb_arg1, (struct pt_regs *)buf,  \
                        sccallback[0].cb_arg2);
        }
        if (sccallback[1].registered_cb) {
                if (sccallback[1].cb_arg2) {
                        *(u_int32_t *)sccallback[1].cb_arg2 = 0;
                }
                sccallback[1].registered_cb (0, sccallback[1].cb_arg1, (struct pt_regs *)buf,  \
                        sccallback[1].cb_arg2);
        }
        return count;
}

typedef enum {
	LED_STATE_OFF = 1,
	LED_STATE_ON = 2,
	LED_STATE_BLINKING = 3,
} led_state_e;

static led_state_e simple_config_led_state = LED_STATE_OFF;

static int gpio_simple_config_led_read(char *page, char **start, off_t off,
					int count, int *eof, void *data)
{
	return sprintf(page, "%d\n", simple_config_led_state);
}

static int gpio_simple_config_led_write(struct file *file, const char *buf,
					unsigned long count, void *data)
{
	u_int32_t val;

	if (sscanf(buf, "%d", &val) != 1)
		return -EINVAL;

	if ((val == SIMPLE_CONFIG_BLINK) && !wps_led_blinking) { /* wps LED blinking */
		wps_led_blinking = 1;
		simple_config_led_state = SIMPLE_CONFIG_BLINK;
		ath_gpio_out_val(WPS_LED_GPIO, WPS_LED_ON);
		OS_INIT_TIMER(NULL, &os_timer_t, wps_led_blink, &os_timer_t);
		OS_SET_TIMER(&os_timer_t, 1000);
	} else if (val == SIMPLE_CONFIG_ON) {	/* WPS Success */
		wps_led_blinking = 0;
		simple_config_led_state = SIMPLE_CONFIG_ON;
		OS_CANCEL_TIMER(&os_timer_t);
		ath_gpio_out_val(WPS_LED_GPIO, WPS_LED_ON);
	} else if (val == SIMPLE_CONFIG_OFF) {	/* WPS failed */
		wps_led_blinking = 0;
		simple_config_led_state = SIMPLE_CONFIG_OFF;
		OS_CANCEL_TIMER(&os_timer_t);
		ath_gpio_out_val(WPS_LED_GPIO, WPS_LED_OFF);
	}

	return count;
}

static int button_state_read (char *page, char **start, off_t off,
                               int count, int *eof, void *data)
{
        /**
         * return states of all buttons.
         * return value: <PUSHBUTTONstate><WIFIBUTTONstate><RESETBUTTONstate>
         *   <PUSHBUTTONstate>=1x       #'x' value: 0 - free, 1 - pressed
         *   <WIFIBUTTONstate>=2x       #'x' value: 0 - free, 1 - pressed
         *   <RESETBUTTONstate>=3x      #'x' value: 0 - free, 1 - pressed(time<5s), 2 - pressed(time>5s)
         **/
         return sprintf(page, "1%u2%u3%u\n", \
                (u_int32_t)pushbut, \
                (u_int32_t)wifibut, \
                (u_int32_t)resetbut);
}

static int button_state_write (struct file *file, const char *buf,
                                        unsigned long count, void *data)
{
        /**
         * set state of button.
         * valid value: <Button number><value>
         *   <Button number>: 1 - Push button, 2 - WiFi button, 3 - Reset button
         *   <value>: should be 0, that indicate the event of button pressed is handled by upper process.
         **/
         u_int32_t num, val;
         char *msg = "Error: valid value inputted into /proc/simple_config/button_state should as below.\n" \
                        "  <Button number><value>\n" \
                        "\t<Button number>: 1 - Push button, 2 - WiFi button, 3 - Reset button.\n" \
                        "\t<value> should be 0.\n";

         num = (u_int32_t)(*buf - '0');
         val = (u_int32_t)(*(buf + 1) - '0');

         if (val != 0) {
                printk("%s", msg);
                return count;
         }

         switch (num) {
         case 1:
                pushbut = free;
                break;
         case 2:
                wifibut = free;
                break;
         case 3:
                resetbut = free;
                break;
         default:
                printk("%s", msg);
                break;
         }

         return count;
}

static int disable_wps_read (char *page, char **start, off_t off,
                               int count, int *eof, void *data)
{
        return 0;
}

static int disable_wps_write (struct file *file, const char *buf,
                                        unsigned long count, void *data)
{
        u_int32_t val;

        if (sscanf(buf, "%d", &val) != 1)
                return -EINVAL;

        if (val == 1) { /* WPS was disabled */
                diswps = 1;
        } else if (val == 0) { /* WPS was enabled */
                diswps = 0;
                OS_CANCEL_TIMER(&os_diswps_t);
        }

        return count;
} 

#if DNI_SIMPLE_CONFIG_SET
static int simple_config_set_read(char *page, char **start, off_t off,
                               int count, int *eof, void *data)
{
        printk("Read from scset is not support :(\n");
        return 0;
}

static int get_word(char *p, char *ret, int len)
{
        static char word[256];
        int count = 0, index = 0;

        if (!p || *p == '\0')
                return 0;

        /* skip all space in head of the string */
        while (*p == ' ')
                count++, p++;

        while (*p != ' ' && *p != '\0' && *p != '\n' && *p != '\r')
                count++, word[index++] = *p++;
        word[index] = '\0';

        memcpy(ret, word, len);
        /* avoid overflow */
        ret[len - 1] = '\0';

        return count;
}

static int word2int(char *word)
{
        int val = 0;

        if (!word || *word == '\0')
                return -1;

        while (*word != '\0') {
                if (*word > '9' || *word < '0')
                        return -1;

                val = val * 10 + (int)(*word++ - '0');
        }

        return val;
}


static int simple_config_set_write (struct file *file, const char *buf,
                               unsigned long count, void *data)
{
        /* buf format:
         *      "set <pin> in|out <value>"
         *      "get <pin>"
         *  <pin> is the GPIO pin number on chip
         *  <value> must be 0 or 1, set the GPIO's value
         **/
        int val = -1, pin = -1, chip, offset = 0;
        char method[8], mode[8], field[8];
        char *p = buf;

        offset = get_word(p, method, sizeof(method));
        p += offset;
        offset = get_word(p, field, sizeof(field));
        pin = word2int(field);
        p += offset;
        offset = get_word(p, mode, sizeof(mode));
        p += offset;
        offset = get_word(p, field, sizeof(field));
        val = word2int(field);

        printk("\n*Info* input is: %s\n", buf);
        printk("*Info*\tmethod: %s\n"
                "\tpin: %d\n"
                "\tmode: %s\n"
                "\tvalue: %d\n",
                method, pin, mode, val);
        if (pin > 22 || pin < 0)
		goto SCSET_ERR;

        if (!strcmp(method, "get")) {
                printk("value is: %u\n", (ath_gpio_in_val(pin) != 0));
        } else if (!strcmp(method, "set")) {
		if (!strcmp(mode, "in")) {
                      ath_gpio_config_input(pin);
                } else if (!strcmp(mode, "out")) {
                        if ((val >> 1) != 0)
                           	goto SCSET_ERR;

                                ath_gpio_config_output(pin);
                                ath_gpio_out_val(pin, val);
                        } else
                                goto SCSET_ERR;
                }
	else
          	goto SCSET_ERR;

        return count;
SCSET_ERR:
        printk("Usage:\n"
                "  echo \"set <pin> in|out <value>\" > scset\n"
                "  echo \"get <pin>\" > scset\n"
                "  <pin> is the GPIO pin number on chip, [0~22].\n"
                "  in|out configure GPIO as input or output mode, invalid for get method..\n"
                "  <value> must be 0 or 1, set the GPIO's value, it's invalid for input mode or get method.\n");
        return count;
}
#endif

static OS_TIMER_FUNC(diswps_led_blink)
{
        static int onoff = 0;

        if (!diswps ||  diswps_blink_count <= 0)
                return;

        if (onoff) {
                ath_gpio_out_val(WPS_LED_GPIO, LED_OFF);
        } else {
                ath_gpio_out_val(WPS_LED_GPIO, LED_ON);
        }
        onoff = 1 - onoff;
        diswps_blink_count--;
        if (diswps_blink_count == 0) {
                ath_gpio_out_val(WPS_LED_GPIO, LED_ON);
        }
        OS_SET_TIMER(&os_diswps_t, WPS_LED_ERR_INTERVAL);
}

static OS_TIMER_FUNC(statusled_blink)
{
        static int onoff = 0;
        if (onoff) {
                ath_gpio_out_val(POWER_LED_GPIO, LED_OFF);
                ath_gpio_out_val(STATUS_LED_GPIO, LED_OFF);
                OS_SET_TIMER(&os_staled_t, TIME_750MSECONDS);
        } else {
                ath_gpio_out_val(POWER_LED_GPIO, LED_OFF);
                ath_gpio_out_val(STATUS_LED_GPIO, LED_ON);
                OS_SET_TIMER(&os_staled_t, TIME_250MSECONDS);
        }
        onoff = 1 - onoff;
}

static OS_TIMER_FUNC(button_detect)
{
        static int staled_s = 0;
        static int wifibut_s = 0;
        static int pushbut_s = 0;
        static int wlan_count = 0;

        /* AR9341: WIRELESS ON/OFF(Input Pin / Active LOW) */
        if (!((ath_gpio_in_val(WLAN_BUTTON_GPIO) >> WLAN_BUTTON_GPIO) & 0x1)) {
                wlan_count++;
                /* [Bug 24969]According to NTGR's requirement, it should only trigger enabling or
                 * disabling the wireless radio after the WLAN switch button is pressed and held
                 * for more than 1 second.
                 * Frequency of GPIO state detecting is 100 milliseconds, so if detect that WLAN
                 * switch button is pressed and held more than 9 times, just trigger the
                 * functionality to trun on/off WLAN.
                 */
                if (wlan_count > 9 && !wifibut_s) {
                        wifibut_s = 1;
                        wifibut = push;
                        OS_CANCEL_TIMER(&os_diswps_t);
                }
        } else {
                wlan_count = 0;
                if (wifibut_s) {
                        wifibut_s = 0;
                }
        }

        /* AR9341: JUMPSTART (Input Pin / Active LOW) */
        if (!((ath_gpio_in_val(WPS_BUTTON_GPIO) >> WPS_BUTTON_GPIO) & 0x1)) {
                if (!pushbut_s) {
                        pushbut_s = 1;
                        pushbut = push;
                        /* blink WPS LED more than 5 seconds if WPS is disable */
                        if (diswps) {
                                diswps_blink_count = 50;
                                OS_SET_TIMER(&os_diswps_t, WPS_LED_ERR_INTERVAL);
                        }
                }
                /* if WPS was disabled, and push button is pressed and hold, just blink WPS LED
                 * until push button is free, and then continue blink WPS LED 5 seconds */
                if (diswps)
                        diswps_blink_count = 50;
        } else {
                if (pushbut_s) {
                        pushbut_s = 0;
                }
        }

        /* AR9341: RESET SWITCH (Input Pin/Active LOW) */
        if (!((ath_gpio_in_val(RESET_BUTTON_GPIO) >> RESET_BUTTON_GPIO) & 0x1)) {
                rstbut_count++;
                /* turn off POWER LED, turn on STATUS LED (amber), set timer, blinking STATUS LED */
                if (!staled_s) {
                        ath_gpio_out_val(POWER_LED_GPIO, LED_OFF);
                        ath_gpio_out_val(STATUS_LED_GPIO, LED_ON);
                        OS_SET_TIMER(&os_staled_t, TIME_5SECONDS);
                        staled_s = 1;
                }
        } else {
                if (COUNT_5SECONDS <= rstbut_count)
                        resetbut = hold;
                else if (0 < rstbut_count && COUNT_5SECONDS > rstbut_count)
                        resetbut = push;
                /* turn off POWER & STATUS LEDs, cancel timer */
                if (staled_s) {
                        OS_CANCEL_TIMER(&os_staled_t);
                        ath_gpio_out_val(POWER_LED_GPIO, LED_OFF);
                        ath_gpio_out_val(STATUS_LED_GPIO, LED_OFF);
                        staled_s = 0;
                }
                rstbut_count = 0;
        }

        OS_SET_TIMER(&os_butdet_t, POLL_INTERVAL);
}

void ap_usb_led_on(void)
{
#ifdef CONFIG_MACH_AR934x
#if !defined(CONFIG_I2S) && defined(AP_USB_LED_GPIO)
    unsigned int rddata;

    if(AP_USB_LED_GPIO == 4) { 
     	rddata = ath_reg_rd(ATH_GPIO_OUT_FUNCTION1); //87- for USB suspend
    	rddata = rddata & 0xffffff00;
    	rddata = rddata | ATH_GPIO_OUT_FUNCTION1_ENABLE_GPIO_4(0x0);
    	ath_reg_wr(ATH_GPIO_OUT_FUNCTION1, rddata);
    }else if(AP_USB_LED_GPIO == 11) {
        rddata = ath_reg_rd(ATH_GPIO_OUT_FUNCTION2); //87- for USB suspend
        rddata = rddata & 0x00ffffff;
        rddata = rddata | ATH_GPIO_OUT_FUNCTION2_ENABLE_GPIO_11(0x0);
        ath_reg_wr(ATH_GPIO_OUT_FUNCTION2, rddata);
    }
    
    ath_reg_rmw_clear(ATH_GPIO_OE, (1<<AP_USB_LED_GPIO));
    ath_reg_rmw_clear(ATH_GPIO_OUT, (1<<AP_USB_LED_GPIO));
#endif
#else
	ath_gpio_out_val(AP_USB_LED_GPIO, USB_LED_ON);
#endif
}

EXPORT_SYMBOL(ap_usb_led_on);

void ap_usb_led_off(void)
{
#ifdef CONFIG_MACH_AR934x
#if !defined(CONFIG_I2S) && defined(AP_USB_LED_GPIO)
	ath_reg_rmw_set(ATH_GPIO_OUT, (1<<AP_USB_LED_GPIO));
#endif
#else
	ath_gpio_out_val(AP_USB_LED_GPIO, USB_LED_OFF);
#endif
}
EXPORT_SYMBOL(ap_usb_led_off);

void ap_power_led_on(void)
{
#ifdef POWER_LED_GPIO
       ath_gpio_out_val(POWER_LED_GPIO, LED_ON);
#endif
}
EXPORT_SYMBOL(ap_power_led_on);

void ap_power_led_off(void)
{
#ifdef POWER_LED_GPIO
       ath_gpio_out_val(POWER_LED_GPIO, LED_OFF);
#endif
}
EXPORT_SYMBOL(ap_power_led_off);

void ap_status_led_on(void)
{
#ifdef STATUS_LED_GPIO
       ath_gpio_out_val(STATUS_LED_GPIO, LED_ON);
#endif
}
EXPORT_SYMBOL(ap_status_led_on);

void ap_status_led_off(void)
{
#ifdef STATUS_LED_GPIO
       ath_gpio_out_val(STATUS_LED_GPIO, LED_OFF);
#endif
}
EXPORT_SYMBOL(ap_status_led_off);


static int create_simple_config_led_proc_entry(void)
{
	if (simple_config_entry != NULL) {
		printk("Already have a proc entry for /proc/simple_config!\n");
		return -ENOENT;
	}

	simple_config_entry = proc_mkdir("simple_config", NULL);
	if (!simple_config_entry)
		return -ENOENT;

	simulate_push_button_entry = create_proc_entry("push_button", 0644,
							simple_config_entry);
	if (!simulate_push_button_entry)
		return -ENOENT;

	simulate_push_button_entry->write_proc = push_button_write;
	simulate_push_button_entry->read_proc = push_button_read;

	simple_config_led_entry = create_proc_entry("simple_config_led", 0644,
							simple_config_entry);
	if (!simple_config_led_entry)
		return -ENOENT;

	simple_config_led_entry->write_proc = gpio_simple_config_led_write;
	simple_config_led_entry->read_proc = gpio_simple_config_led_read;

	wps_entry = create_proc_entry ("wps", 0644,
                                        simple_config_entry);
	if (!wps_entry)
		return -ENOENT;

	wps_entry->write_proc = wps_write;
	wps_entry->read_proc = wps_read;

	button_state = create_proc_entry ("button_state", 0644,
	                                    simple_config_entry);
	if (!button_state)
		return -ENOENT;

	button_state->write_proc = button_state_write;
	button_state->read_proc = button_state_read;

	disable_wps = create_proc_entry ("diswps", 0644,
					simple_config_entry);
	if (!disable_wps)
		return -ENOENT;

	disable_wps->write_proc = disable_wps_write;
	disable_wps->read_proc = disable_wps_read;

#if DNI_SIMPLE_CONFIG_SET
	scset_entry = create_proc_entry ("scset", 0644, simple_config_entry);
	if (!scset_entry)
		return -ENOENT;
	scset_entry->write_proc = simple_config_set_write;
	scset_entry->read_proc = simple_config_set_read;
#endif


	/* configure gpio as outputs */
	ath_gpio_config_output(WPS_LED_GPIO);
	ath_gpio_config_output(WAN_AMBER_LED_GPIO);
	ath_gpio_config_output(WAN_GREEN_LED_GPIO);
	ath_gpio_config_output(WLAN_LED_GPIO);
	ath_gpio_config_output(STATUS_LED_GPIO);
	ath_gpio_config_output(POWER_LED_GPIO);

	/* configure gpio as inputs */
	ath_gpio_config_input(WPS_BUTTON_GPIO);
	ath_gpio_config_input(WLAN_BUTTON_GPIO);
	ath_gpio_config_input(RESET_BUTTON_GPIO);

	/* switch off the led */
	ath_gpio_out_val(WPS_LED_GPIO, LED_OFF);
	ath_gpio_out_val(WAN_AMBER_LED_GPIO, LED_OFF);
	ath_gpio_out_val(WAN_GREEN_LED_GPIO, LED_OFF);
	ath_gpio_out_val(WLAN_LED_GPIO, LED_OFF);
	ath_gpio_out_val(POWER_LED_GPIO, LED_OFF);

	/* inital timer */
	OS_INIT_TIMER(NULL, &os_butdet_t, button_detect, &os_butdet_t);
	OS_INIT_TIMER(NULL, &os_staled_t, statusled_blink, &os_staled_t);
	OS_INIT_TIMER(NULL, &os_diswps_t, diswps_led_blink, &os_diswps_t);

	/* enable timer */
	OS_SET_TIMER(&os_butdet_t, POLL_INTERVAL);
	
	return 0;
}

static int
athfr_open(struct inode *inode, struct file *file)
{
	if (MINOR(inode->i_rdev) != FACTORY_RESET_MINOR) {
		return -ENODEV;
	}

	if (ath_fr_opened) {
		return -EBUSY;
	}

	ath_fr_opened = 1;
	return nonseekable_open(inode, file);
}

static int
athfr_close(struct inode *inode, struct file *file)
{
	if (MINOR(inode->i_rdev) != FACTORY_RESET_MINOR) {
		return -ENODEV;
	}

	ath_fr_opened = 0;
	return 0;
}

static ssize_t
athfr_read(struct file *file, char *buf, size_t count, loff_t * ppos)
{
	return -ENOTSUPP;
}

static ssize_t
athfr_write(struct file *file, const char *buf, size_t count, loff_t * ppos)
{
	return -ENOTSUPP;
}

static int
athfr_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
		unsigned long arg)
{
	int ret = 0;

	switch (cmd) {
	case ATH_FACTORY_RESET:
		atomic_inc(&ath_fr_status);
		sleep_on(&ath_fr_wq);
		break;

	default:
		ret = -EINVAL;
	}

	return ret;
}

static struct file_operations athfr_fops = {
	read:	athfr_read,
	write:	athfr_write,
	ioctl:	athfr_ioctl,
	open:	athfr_open,
	release:athfr_close
};

static struct miscdevice athfr_miscdev = {
	FACTORY_RESET_MINOR,
	"Factory reset",
	&athfr_fops
};

int __init ath_simple_config_init(void)
{
#ifdef CONFIG_CUS100
	u32 mask = 0;
#endif
	int req, ret;

	ret = misc_register(&athfr_miscdev);

	if (ret < 0) {
		printk("*** ath misc_register failed %d *** \n", ret);
		return -1;
	}

#ifdef CONFIG_CUS100
	mask = ath_reg_rd(ATH_MISC_INT_MASK);
	ath_reg_wr(ATH_MISC_INT_MASK, mask | (1 << 2));
	ath_gpio_config_int(JUMPSTART_GPIO, INT_TYPE_LEVEL,
				INT_POL_ACTIVE_HIGH);
	ath_gpio_intr_enable(JUMPSTART_GPIO);
	ath_gpio_config_input(JUMPSTART_GPIO);
#else
	ath_gpio_config_input(JUMPSTART_GPIO);
	/* configure Jumpstart GPIO as level triggered interrupt */
	ath_gpio_config_int(JUMPSTART_GPIO, INT_TYPE_LEVEL,
				INT_POL_ACTIVE_LOW);
	printk("%s (%s) JUMPSTART_GPIO: %d\n", __FILE__, __func__,
		JUMPSTART_GPIO);
#ifndef CONFIG_MACH_AR934x
	ath_reg_rmw_clear(ATH_GPIO_FUNCTIONS, (1 << 2));
	ath_reg_rmw_clear(ATH_GPIO_FUNCTIONS, (1 << 16));
	ath_reg_rmw_clear(ATH_GPIO_FUNCTIONS, (1 << 20));
#endif
#endif

#if 0
	req = request_irq(ATH_GPIO_IRQn(JUMPSTART_GPIO), jumpstart_irq, 0,
			"SW JUMPSTART/FACTORY RESET", NULL);
	if (req != 0) {
		printk("request_irq for jumpstart failed (error %d)\n", req);
		misc_deregister(&athfr_miscdev);
		ath_gpio_intr_shutdown(ATH_GPIO_IRQn(JUMPSTART_GPIO));
		return -1;
	}
#endif

#if !defined(CONFIG_I2S) && defined(AP_USB_LED_GPIO)
	ath_gpio_config_output(AP_USB_LED_GPIO);
#endif
	init_waitqueue_head(&ath_fr_wq);
	
	/* Disable JTAG to use GPIO0-GPIO3 */
	ath_reg_rmw_set(ATH_GPIO_FUNCTIONS, (1 << 1));	
	create_simple_config_led_proc_entry();
	return 0;
}

/*
 * used late_initcall so that misc_register will succeed
 * otherwise, misc driver won't be in a initializated state
 * thereby resulting in misc_register api to fail.
 */
#if !defined(CONFIG_ATH_EMULATION)
late_initcall(ath_simple_config_init);
#endif
