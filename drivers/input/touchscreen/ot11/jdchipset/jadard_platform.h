#ifndef JADARD_PLATFORM_H
#define JADARD_PLATFORM_H

#include "jadard_common.h"
#include <linux/spi/spi.h>

#if defined(CONFIG_JD_DB)
#include <linux/regulator/consumer.h>
#endif
/*Tab A9 code for SR-AX6739A-01-682 by hehaoran5 at 20230610 start*/
#include <linux/platform_data/spi-mt65xx.h>
#define JD_CS_SETUPTIME        220   //cs~clk > 200ns,1us == 100*10 ns
/*Tab A9 code for SR-AX6739A-01-682 by hehaoran5 at 20230610 end*/

#define JADARD_common_NAME "jadard_tp"
#define JADARD_BUS_RETRY_TIMES 10

#if defined(CONFIG_TOUCHSCREEN_JADARD_DEBUG)
extern bool jd_g_dbg_enable;
extern bool jd_g_esd_check_enable;
#define JD_I(x...) \
do { if (!jd_g_esd_check_enable) printk("[JDTP] " x); } while (0)
#define JD_W(x...) printk("[JDTP][WARNING] " x)
#define JD_E(x...) printk("[JDTP][ERROR] " x)
#define JD_D(x...) \
do { if (jd_g_dbg_enable) printk("[JDTP][DEBUG] " x); } while (0)
#else
#define JD_I(x...)
#define JD_W(x...)
#define JD_E(x...)
#define JD_D(x...)
#endif

struct jadard_i2c_platform_data {
    int abs_x_min;
    int abs_x_max;
    int abs_x_fuzz;
    int abs_y_min;
    int abs_y_max;
    int abs_y_fuzz;
    int abs_pressure_min;
    int abs_pressure_max;
    int abs_pressure_fuzz;
    int abs_width_min;
    int abs_width_max;
    uint8_t usb_status[2];
    int gpio_irq;
    int gpio_reset;

#if defined(CONFIG_JD_DB)
    struct regulator *vcc_ana; /* For Dragon Board */
    struct regulator *vcc_dig; /* For Dragon Board */
#endif
};

int jadard_bus_read(uint8_t *cmd, uint8_t cmd_len, uint8_t *data, uint32_t data_len, uint8_t toRetry);
int jadard_bus_write(uint8_t *cmd, uint8_t cmd_len, uint8_t *data, uint32_t data_len, uint8_t toRetry);
void jadard_int_en_set(bool enable);
void jadard_int_enable(bool enable);
int jadard_ts_register_interrupt(void);
void jadard_ts_free_interrupt(void);
int jadard_gpio_power_config(struct jadard_i2c_platform_data *pdata);
void jadard_gpio_power_deconfig(struct jadard_i2c_platform_data *pdata);

#if defined(JD_CONFIG_FB)
int jadard_fb_notifier_callback(struct notifier_block *self, unsigned long event, void *data);
#endif

#ifdef JD_RST_PIN_FUNC
void jadard_gpio_set_value(int pin_num, uint8_t value);
#endif

#endif
