// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2023 ETEK Inc.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/semaphore.h>
#include <linux/pm_runtime.h>
#include <linux/workqueue.h>
#include <linux/kthread.h>
#include <linux/cpu.h>
#include <linux/version.h>
#include <uapi/linux/sched/types.h>
#include <linux/sched/clock.h>

#include "inc/pd_dbg_info.h"
#include "inc/tcpci.h"
#include "inc/et7304.h"


#include <linux/delay.h>

#if IS_ENABLED(CONFIG_RT_REGMAP)
#include <mt-plat/rt-regmap.h>
#endif /* CONFIG_RT_REGMAP */

#include <linux/sched/rt.h>

/*Tab A9 code for SR-AX6739A-01-470 by wenyaqi at 20230504 start*/
#include <linux/power/gxy_psy_sysfs.h>

/**********************************************************
 *
 *   [extern for other module]
 *
 *********************************************************/
extern void gxy_bat_set_tcpcinfo(enum gxy_bat_tcpc_info tinfo_data);
/*Tab A9 code for SR-AX6739A-01-470 by wenyaqi at 20230504 end*/

/* #define DEBUG_GPIO    66 */
#define DEBUG_GPIO 0

/* Tab A9_V code for P240807-00214 by zhangziyi at 20240927 start */
#define ET7304M_DRV_VERSION    "2.0.6_MTK"
/* Tab A9_V code for P240807-00214 by zhangziyi at 20240927 end */

#define ET7304M_TYPCA_START_PLUG    1



#define ET7304M_IRQ_WAKE_TIME    (500) /* ms */

struct et7304_chip {
    struct i2c_client *client;
    struct device *dev;
#if IS_ENABLED(CONFIG_RT_REGMAP)
    struct rt_regmap_device *m_dev;
#endif /* CONFIG_RT_REGMAP */
    struct semaphore io_lock;
    struct semaphore suspend_lock;
    struct tcpc_desc *tcpc_desc;
    struct tcpc_device *tcpc;
    struct kthread_worker irq_worker;
    struct kthread_work irq_work;
    struct task_struct *irq_worker_task;
    struct wakeup_source *irq_wake_lock;

    atomic_t poll_count;
    struct delayed_work    poll_work;

    int irq_gpio;
    int irq;
    int chip_id;
};

#if IS_ENABLED(CONFIG_RT_REGMAP)
RT_REG_DECL(TCPC_V10_REG_VID, 2, RT_NORMAL_WR_ONCE, {});
RT_REG_DECL(TCPC_V10_REG_PID, 2, RT_NORMAL_WR_ONCE, {});
RT_REG_DECL(TCPC_V10_REG_DID, 2, RT_NORMAL_WR_ONCE, {});
RT_REG_DECL(TCPC_V10_REG_TYPEC_REV, 2, RT_NORMAL_WR_ONCE, {});
RT_REG_DECL(TCPC_V10_REG_PD_REV, 2, RT_NORMAL_WR_ONCE, {});
RT_REG_DECL(TCPC_V10_REG_PDIF_REV, 2, RT_NORMAL_WR_ONCE, {});
RT_REG_DECL(TCPC_V10_REG_ALERT, 2, RT_VOLATILE, {});
RT_REG_DECL(TCPC_V10_REG_ALERT_MASK, 2, RT_NORMAL_WR_ONCE, {});
RT_REG_DECL(TCPC_V10_REG_POWER_STATUS_MASK, 1, RT_NORMAL_WR_ONCE, {});
RT_REG_DECL(TCPC_V10_REG_FAULT_STATUS_MASK, 1, RT_NORMAL_WR_ONCE, {});
RT_REG_DECL(TCPC_V10_REG_TCPC_CTRL, 1, RT_NORMAL_WR_ONCE, {});
RT_REG_DECL(TCPC_V10_REG_ROLE_CTRL, 1, RT_NORMAL_WR_ONCE, {});
RT_REG_DECL(TCPC_V10_REG_FAULT_CTRL, 1, RT_NORMAL_WR_ONCE, {});
RT_REG_DECL(TCPC_V10_REG_POWER_CTRL, 1, RT_VOLATILE, {});
RT_REG_DECL(TCPC_V10_REG_CC_STATUS, 1, RT_VOLATILE, {});
RT_REG_DECL(TCPC_V10_REG_POWER_STATUS, 1, RT_VOLATILE, {});
RT_REG_DECL(TCPC_V10_REG_FAULT_STATUS, 1, RT_VOLATILE, {});
RT_REG_DECL(TCPC_V10_REG_COMMAND, 1, RT_VOLATILE, {});
RT_REG_DECL(TCPC_V10_REG_MSG_HDR_INFO, 1, RT_NORMAL_WR_ONCE, {});
RT_REG_DECL(TCPC_V10_REG_RX_DETECT, 1, RT_NORMAL_WR_ONCE, {});
RT_REG_DECL(TCPC_V10_REG_RX_BYTE_CNT, 1, RT_VOLATILE, {});
RT_REG_DECL(TCPC_V10_REG_RX_BUF_FRAME_TYPE, 1, RT_VOLATILE, {});
RT_REG_DECL(TCPC_V10_REG_RX_HDR, 2, RT_VOLATILE, {});
RT_REG_DECL(TCPC_V10_REG_RX_DATA, 28, RT_VOLATILE, {});
RT_REG_DECL(TCPC_V10_REG_TRANSMIT, 1, RT_VOLATILE, {});
RT_REG_DECL(TCPC_V10_REG_TX_BYTE_CNT, 1, RT_NORMAL_WR_ONCE, {});
RT_REG_DECL(TCPC_V10_REG_TX_HDR, 2, RT_NORMAL_WR_ONCE, {});
RT_REG_DECL(TCPC_V10_REG_TX_DATA, 28, RT_NORMAL_WR_ONCE, {});
RT_REG_DECL(ET7304M_REG_CONFIG_GPIO0, 1, RT_NORMAL_WR_ONCE, {});
RT_REG_DECL(ET7304M_REG_PHY_CTRL1, 1, RT_NORMAL_WR_ONCE, {});
RT_REG_DECL(ET7304M_REG_CLK_CTRL2, 1, RT_NORMAL_WR_ONCE, {});
RT_REG_DECL(ET7304M_REG_CLK_CTRL3, 1, RT_NORMAL_WR_ONCE, {});
RT_REG_DECL(ET7304M_REG_PRL_FSM_RESET, 1, RT_VOLATILE, {});
RT_REG_DECL(ET7304M_REG_BMC_CTRL, 1, RT_VOLATILE, {});
RT_REG_DECL(ET7304M_REG_BMCIO_RXDZSEL, 1, RT_NORMAL_WR_ONCE, {});
RT_REG_DECL(ET7304M_REG_VCONN_CLIMITEN, 1, RT_NORMAL_WR_ONCE, {});
RT_REG_DECL(ET7304M_REG_RT_STATUS, 1, RT_VOLATILE, {});
RT_REG_DECL(ET7304M_REG_RT_INT, 1, RT_VOLATILE, {});
RT_REG_DECL(ET7304M_REG_RT_MASK, 1, RT_NORMAL_WR_ONCE, {});
RT_REG_DECL(ET7304M_REG_IDLE_CTRL, 1, RT_NORMAL_WR_ONCE, {});
RT_REG_DECL(ET7304M_REG_INTRST_CTRL, 1, RT_NORMAL_WR_ONCE, {});
RT_REG_DECL(ET7304M_REG_WATCHDOG_CTRL, 1, RT_NORMAL_WR_ONCE, {});
RT_REG_DECL(ET7304M_REG_I2CRST_CTRL, 1, RT_NORMAL_WR_ONCE, {});
RT_REG_DECL(ET7304M_REG_SWRESET, 1, RT_VOLATILE, {});
RT_REG_DECL(ET7304M_REG_TTCPC_FILTER, 1, RT_NORMAL_WR_ONCE, {});
RT_REG_DECL(ET7304M_REG_DRP_TOGGLE_CYCLE, 1, RT_NORMAL_WR_ONCE, {});
RT_REG_DECL(ET7304M_REG_DRP_DUTY_CTRL, 2, RT_NORMAL_WR_ONCE, {});
RT_REG_DECL(ET7304M_REG_BMCIO_RXDZEN, 1, RT_NORMAL_WR_ONCE, {});
RT_REG_DECL(ET7304M_REG_UNLOCK_PW_2, 2, RT_VOLATILE, {});
RT_REG_DECL(ET7304M_REG_EFUSE5, 1, RT_VOLATILE, {});

static const rt_register_map_t et7304_chip_regmap[] = {
    RT_REG(TCPC_V10_REG_VID),
    RT_REG(TCPC_V10_REG_PID),
    RT_REG(TCPC_V10_REG_DID),
    RT_REG(TCPC_V10_REG_TYPEC_REV),
    RT_REG(TCPC_V10_REG_PD_REV),
    RT_REG(TCPC_V10_REG_PDIF_REV),
    RT_REG(TCPC_V10_REG_ALERT),
    RT_REG(TCPC_V10_REG_ALERT_MASK),
    RT_REG(TCPC_V10_REG_POWER_STATUS_MASK),
    RT_REG(TCPC_V10_REG_FAULT_STATUS_MASK),
    RT_REG(TCPC_V10_REG_TCPC_CTRL),
    RT_REG(TCPC_V10_REG_ROLE_CTRL),
    RT_REG(TCPC_V10_REG_FAULT_CTRL),
    RT_REG(TCPC_V10_REG_POWER_CTRL),
    RT_REG(TCPC_V10_REG_CC_STATUS),
    RT_REG(TCPC_V10_REG_POWER_STATUS),
    RT_REG(TCPC_V10_REG_FAULT_STATUS),
    RT_REG(TCPC_V10_REG_COMMAND),
    RT_REG(TCPC_V10_REG_MSG_HDR_INFO),
    RT_REG(TCPC_V10_REG_RX_DETECT),
    RT_REG(TCPC_V10_REG_RX_BYTE_CNT),
    RT_REG(TCPC_V10_REG_RX_BUF_FRAME_TYPE),
    RT_REG(TCPC_V10_REG_RX_HDR),
    RT_REG(TCPC_V10_REG_RX_DATA),
    RT_REG(TCPC_V10_REG_TRANSMIT),
    RT_REG(TCPC_V10_REG_TX_BYTE_CNT),
    RT_REG(TCPC_V10_REG_TX_HDR),
    RT_REG(TCPC_V10_REG_TX_DATA),
    RT_REG(ET7304M_REG_CONFIG_GPIO0),
    RT_REG(ET7304M_REG_PHY_CTRL1),
    RT_REG(ET7304M_REG_CLK_CTRL2),
    RT_REG(ET7304M_REG_CLK_CTRL3),
    RT_REG(ET7304M_REG_PRL_FSM_RESET),
    RT_REG(ET7304M_REG_BMC_CTRL),
    RT_REG(ET7304M_REG_BMCIO_RXDZSEL),
    RT_REG(ET7304M_REG_VCONN_CLIMITEN),
    RT_REG(ET7304M_REG_RT_STATUS),
    RT_REG(ET7304M_REG_RT_INT),
    RT_REG(ET7304M_REG_RT_MASK),
    RT_REG(ET7304M_REG_IDLE_CTRL),
    RT_REG(ET7304M_REG_INTRST_CTRL),
    RT_REG(ET7304M_REG_WATCHDOG_CTRL),
    RT_REG(ET7304M_REG_I2CRST_CTRL),
    RT_REG(ET7304M_REG_SWRESET),
    RT_REG(ET7304M_REG_TTCPC_FILTER),
    RT_REG(ET7304M_REG_DRP_TOGGLE_CYCLE),
    RT_REG(ET7304M_REG_DRP_DUTY_CTRL),
    RT_REG(ET7304M_REG_BMCIO_RXDZEN),
    RT_REG(ET7304M_REG_UNLOCK_PW_2),
    RT_REG(ET7304M_REG_EFUSE5),
};
#define RT1711_CHIP_REGMAP_SIZE ARRAY_SIZE(et7304_chip_regmap)

#endif /* CONFIG_RT_REGMAP */

static int et7304_read_device(void *client, u32 reg, int len, void *dst)
{
    struct i2c_client *i2c = client;
    int ret = 0, count = 5;
    u64 t1 = 0, t2 = 0;

    while (1) {
        t1 = local_clock();
        ret = i2c_smbus_read_i2c_block_data(i2c, reg, len, dst);
        t2 = local_clock();
        RT1711_INFO("%s del = %lluus, reg = %02X, len = %d\n",
                __func__, (t2 - t1) / NSEC_PER_USEC, reg, len);
        if (ret < 0 && count > 1)
            count--;
        else
            break;
        udelay(100);
    }
    return ret;
}

static int et7304_write_device(void *client, u32 reg, int len, const void *src)
{
    struct i2c_client *i2c = client;
    int ret = 0, count = 5;
    u64 t1 = 0, t2 = 0;

    while (1) {
        t1 = local_clock();
        ret = i2c_smbus_write_i2c_block_data(i2c, reg, len, src);
        t2 = local_clock();
        RT1711_INFO("%s del = %lluus, reg = %02X, len = %d\n",
                __func__, (t2 - t1) / NSEC_PER_USEC, reg, len);
        if (ret < 0 && count > 1)
            count--;
        else
            break;
        udelay(100);
    }
    return ret;
}

static int et7304_reg_read(struct i2c_client *i2c, u8 reg)
{
    struct et7304_chip *chip = i2c_get_clientdata(i2c);
    u8 val = 0;
    int ret = 0;

#if IS_ENABLED(CONFIG_RT_REGMAP)
    ret = rt_regmap_block_read(chip->m_dev, reg, 1, &val);
#else
    ret = et7304_read_device(chip->client, reg, 1, &val);
#endif /* CONFIG_RT_REGMAP */
    if (ret < 0) {
        dev_err(chip->dev, "et7304 reg read fail\n");
        return ret;
    }
    return val;
}

static int et7304_reg_write(struct i2c_client *i2c, u8 reg, const u8 data)
{
    struct et7304_chip *chip = i2c_get_clientdata(i2c);
    int ret = 0;

#if IS_ENABLED(CONFIG_RT_REGMAP)
    ret = rt_regmap_block_write(chip->m_dev, reg, 1, &data);
#else
    ret = et7304_write_device(chip->client, reg, 1, &data);
#endif /* CONFIG_RT_REGMAP */
    if (ret < 0)
        dev_err(chip->dev, "et7304 reg write fail\n");
    return ret;
}

static int et7304_block_read(struct i2c_client *i2c,
            u8 reg, int len, void *dst)
{
    struct et7304_chip *chip = i2c_get_clientdata(i2c);
    int ret = 0;
#if IS_ENABLED(CONFIG_RT_REGMAP)
    ret = rt_regmap_block_read(chip->m_dev, reg, len, dst);
#else
    ret = et7304_read_device(chip->client, reg, len, dst);
#endif /* #if IS_ENABLED(CONFIG_RT_REGMAP) */
    if (ret < 0)
        dev_err(chip->dev, "et7304 block read fail\n");
    return ret;
}

static int et7304_block_write(struct i2c_client *i2c,
            u8 reg, int len, const void *src)
{
    struct et7304_chip *chip = i2c_get_clientdata(i2c);
    int ret = 0;
#if IS_ENABLED(CONFIG_RT_REGMAP)
    ret = rt_regmap_block_write(chip->m_dev, reg, len, src);
#else
    ret = et7304_write_device(chip->client, reg, len, src);
#endif /* #if IS_ENABLED(CONFIG_RT_REGMAP) */
    if (ret < 0)
        dev_err(chip->dev, "et7304 block write fail\n");
    return ret;
}

static int32_t et7304_write_word(struct i2c_client *client,
                    uint8_t reg_addr, uint16_t data)
{
    int ret;

    /* don't need swap */
    ret = et7304_block_write(client, reg_addr, 2, (uint8_t *)&data);
    return ret;
}

static int32_t et7304_read_word(struct i2c_client *client,
                    uint8_t reg_addr, uint16_t *data)
{
    int ret;

    /* don't need swap */
    ret = et7304_block_read(client, reg_addr, 2, (uint8_t *)data);
    return ret;
}

static inline int et7304_i2c_write8(
    struct tcpc_device *tcpc, u8 reg, const u8 data)
{
    struct et7304_chip *chip = tcpc_get_dev_data(tcpc);

    return et7304_reg_write(chip->client, reg, data);
}

static inline int et7304_i2c_write16(
        struct tcpc_device *tcpc, u8 reg, const u16 data)
{
    struct et7304_chip *chip = tcpc_get_dev_data(tcpc);

    return et7304_write_word(chip->client, reg, data);
}

static inline int et7304_i2c_read8(struct tcpc_device *tcpc, u8 reg)
{
    struct et7304_chip *chip = tcpc_get_dev_data(tcpc);

    return et7304_reg_read(chip->client, reg);
}

static inline int et7304_i2c_read16(
    struct tcpc_device *tcpc, u8 reg)
{
    struct et7304_chip *chip = tcpc_get_dev_data(tcpc);
    u16 data;
    int ret;

    ret = et7304_read_word(chip->client, reg, &data);
    if (ret < 0)
        return ret;
    return data;
}

#if IS_ENABLED(CONFIG_RT_REGMAP)
static struct rt_regmap_fops et7304_regmap_fops = {
    .read_device = et7304_read_device,
    .write_device = et7304_write_device,
};
#endif /* CONFIG_RT_REGMAP */

static int et7304_regmap_init(struct et7304_chip *chip)
{
#if IS_ENABLED(CONFIG_RT_REGMAP)
    struct rt_regmap_properties *props;
    char name[32];
    int len;

    props = devm_kzalloc(chip->dev, sizeof(*props), GFP_KERNEL);
    if (!props)
        return -ENOMEM;

    props->register_num = RT1711_CHIP_REGMAP_SIZE;
    props->rm = et7304_chip_regmap;

    props->rt_regmap_mode = RT_MULTI_BYTE |
                RT_IO_PASS_THROUGH | RT_DBG_SPECIAL;
    snprintf(name, sizeof(name), "et7304-%02x", chip->client->addr);

    len = strlen(name);
    props->name = kzalloc(len+1, GFP_KERNEL);
    props->aliases = kzalloc(len+1, GFP_KERNEL);

    if ((!props->name) || (!props->aliases))
        return -ENOMEM;

    strlcpy((char *)props->name, name, len+1);
    strlcpy((char *)props->aliases, name, len+1);
    props->io_log_en = 0;

    chip->m_dev = rt_regmap_device_register(props,
            &et7304_regmap_fops, chip->dev, chip->client, chip);
    if (!chip->m_dev) {
        dev_err(chip->dev, "et7304 chip rt_regmap register fail\n");
        return -EINVAL;
    }
#endif
    return 0;
}

static int et7304_regmap_deinit(struct et7304_chip *chip)
{
#if IS_ENABLED(CONFIG_RT_REGMAP)
    rt_regmap_device_unregister(chip->m_dev);
#endif
    return 0;
}

static inline int et7304_software_reset(struct tcpc_device *tcpc)
{
    int ret = et7304_i2c_write8(tcpc, ET7304M_REG_SWRESET, 1);
#if IS_ENABLED(CONFIG_RT_REGMAP)
    struct et7304_chip *chip = tcpc_get_dev_data(tcpc);
#endif /* CONFIG_RT_REGMAP */

    if (ret < 0)
        return ret;
#if IS_ENABLED(CONFIG_RT_REGMAP)
    rt_regmap_cache_reload(chip->m_dev);
#endif /* CONFIG_RT_REGMAP */
    usleep_range(1000, 2000);
    return 0;
}

static inline int et7304_command(struct tcpc_device *tcpc, uint8_t cmd)
{
    return et7304_i2c_write8(tcpc, TCPC_V10_REG_COMMAND, cmd);
}

static int et7304_init_alert_mask(struct tcpc_device *tcpc)
{
    uint16_t mask;
    struct et7304_chip *chip = tcpc_get_dev_data(tcpc);

    mask = TCPC_V10_REG_ALERT_CC_STATUS | TCPC_V10_REG_ALERT_POWER_STATUS;

#if IS_ENABLED(CONFIG_USB_POWER_DELIVERY)
    /* Need to handle RX overflow */
    mask |= TCPC_V10_REG_ALERT_TX_SUCCESS | TCPC_V10_REG_ALERT_TX_DISCARDED
            | TCPC_V10_REG_ALERT_TX_FAILED
            | TCPC_V10_REG_ALERT_RX_HARD_RST
            | TCPC_V10_REG_ALERT_RX_STATUS
            | TCPC_V10_REG_RX_OVERFLOW;
#endif

    mask |= TCPC_REG_ALERT_FAULT;

    return et7304_write_word(chip->client, TCPC_V10_REG_ALERT_MASK, mask);
}

static int et7304_init_power_status_mask(struct tcpc_device *tcpc)
{
    const uint8_t mask = TCPC_V10_REG_POWER_STATUS_VBUS_PRES;

    return et7304_i2c_write8(tcpc,
            TCPC_V10_REG_POWER_STATUS_MASK, mask);
}

static int et7304_init_fault_mask(struct tcpc_device *tcpc)
{
    const uint8_t mask =
        TCPC_V10_REG_FAULT_STATUS_VCONN_OV |
        TCPC_V10_REG_FAULT_STATUS_VCONN_OC;

    return et7304_i2c_write8(tcpc,
            TCPC_V10_REG_FAULT_STATUS_MASK, mask);
}

static int et7304_init_rt_mask(struct tcpc_device *tcpc)
{
    uint8_t rt_mask = 0;
#if CONFIG_TCPC_WATCHDOG_EN
    rt_mask |= ET7304M_REG_M_WATCHDOG;
#endif /* CONFIG_TCPC_WATCHDOG_EN */
#if CONFIG_TCPC_VSAFE0V_DETECT_IC
    rt_mask |= ET7304M_REG_M_VBUS_80;
#endif /* CONFIG_TCPC_VSAFE0V_DETECT_IC */

#if CONFIG_TYPEC_CAP_RA_DETACH
    if (tcpc->tcpc_flags & TCPC_FLAGS_CHECK_RA_DETACHE)
        rt_mask |= ET7304M_REG_M_RA_DETACH;
#endif /* CONFIG_TYPEC_CAP_RA_DETACH */

#if CONFIG_TYPEC_CAP_LPM_WAKEUP_WATCHDOG
    if (tcpc->tcpc_flags & TCPC_FLAGS_LPM_WAKEUP_WATCHDOG)
        rt_mask |= ET7304M_REG_M_WAKEUP;
#endif    /* CONFIG_TYPEC_CAP_LPM_WAKEUP_WATCHDOG */

    return et7304_i2c_write8(tcpc, ET7304M_REG_RT_MASK, rt_mask);
}

/*
static inline void et7304_poll_ctrl(struct et7304_chip *chip)
{
    cancel_delayed_work_sync(&chip->poll_work);

    if (atomic_read(&chip->poll_count) == 0) {
        atomic_inc(&chip->poll_count);
        cpu_idle_poll_ctrl(true);
    }

    schedule_delayed_work(
        &chip->poll_work, msecs_to_jiffies(40));
}
*/

static void et7304_irq_work_handler(struct kthread_work *work)
{
    struct et7304_chip *chip =
            container_of(work, struct et7304_chip, irq_work);
    int regval = 0;
    int gpio_val;

    // et7304_poll_ctrl(chip);
    /* make sure I2C bus had resumed */
    down(&chip->suspend_lock);
    tcpci_lock_typec(chip->tcpc);

#if DEBUG_GPIO
    gpio_set_value(DEBUG_GPIO, 1);
#endif

    do {
        regval = tcpci_alert(chip->tcpc);
        if (regval)
            break;
        gpio_val = gpio_get_value(chip->irq_gpio);
    } while (gpio_val == 0);

    tcpci_unlock_typec(chip->tcpc);
    up(&chip->suspend_lock);

#if DEBUG_GPIO
    gpio_set_value(DEBUG_GPIO, 1);
#endif
}

/*
static void et7304_poll_work(struct work_struct *work)
{
    struct et7304_chip *chip = container_of(
        work, struct et7304_chip, poll_work.work);

    if (atomic_dec_and_test(&chip->poll_count))
        cpu_idle_poll_ctrl(false);
}
*/

static irqreturn_t et7304_intr_handler(int irq, void *data)
{
    struct et7304_chip *chip = data;

    __pm_wakeup_event(chip->irq_wake_lock, ET7304M_IRQ_WAKE_TIME);

#if DEBUG_GPIO
    gpio_set_value(DEBUG_GPIO, 0);
#endif
    kthread_queue_work(&chip->irq_worker, &chip->irq_work);
    return IRQ_HANDLED;
}

static int et7304_init_alert(struct tcpc_device *tcpc)
{
    struct et7304_chip *chip = tcpc_get_dev_data(tcpc);
    struct sched_param param = { .sched_priority = MAX_RT_PRIO - 1 };
    int ret;
    char *name;
    int len;

    /* Clear Alert Mask & Status */
    et7304_write_word(chip->client, TCPC_V10_REG_ALERT_MASK, 0);
    et7304_write_word(chip->client, TCPC_V10_REG_ALERT, 0xffff);

    len = strlen(chip->tcpc_desc->name);
    name = devm_kzalloc(chip->dev, len+5, GFP_KERNEL);
    if (!name)
        return -ENOMEM;

    snprintf(name, PAGE_SIZE, "%s-IRQ", chip->tcpc_desc->name);

    pr_info("%s name = %s, gpio = %d\n", __func__,
                chip->tcpc_desc->name, chip->irq_gpio);

    ret = devm_gpio_request(chip->dev, chip->irq_gpio, name);
#if DEBUG_GPIO
    gpio_request(DEBUG_GPIO, "debug_latency_pin");
    gpio_direction_output(DEBUG_GPIO, 1);
#endif
    if (ret < 0) {
        pr_err("Error: failed to request GPIO%d (ret = %d)\n",
        chip->irq_gpio, ret);
        goto init_alert_err;
    }

    ret = gpio_direction_input(chip->irq_gpio);
    if (ret < 0) {
        pr_err("Error: failed to set GPIO%d as input pin(ret = %d)\n",
        chip->irq_gpio, ret);
        goto init_alert_err;
    }

    chip->irq = gpio_to_irq(chip->irq_gpio);
    if (chip->irq <= 0) {
        pr_err("%s gpio to irq fail, chip->irq(%d)\n",
                        __func__, chip->irq);
        goto init_alert_err;
    }

    pr_info("%s : IRQ number = %d\n", __func__, chip->irq);

    kthread_init_worker(&chip->irq_worker);
    chip->irq_worker_task = kthread_run(kthread_worker_fn,
            &chip->irq_worker, "%s", chip->tcpc_desc->name);
    if (IS_ERR(chip->irq_worker_task)) {
        pr_err("Error: Could not create tcpc task\n");
        goto init_alert_err;
    }

    sched_setscheduler(chip->irq_worker_task, SCHED_FIFO, &param);
    kthread_init_work(&chip->irq_work, et7304_irq_work_handler);

    pr_info("IRQF_NO_THREAD Test\n");
    ret = request_irq(chip->irq, et7304_intr_handler,
        IRQF_TRIGGER_FALLING | IRQF_NO_THREAD, name, chip);
    if (ret < 0) {
        pr_err("Error: failed to request irq%d (gpio = %d, ret = %d)\n",
            chip->irq, chip->irq_gpio, ret);
        goto init_alert_err;
    }

    enable_irq_wake(chip->irq);
    return 0;
init_alert_err:
    return -EINVAL;
}

int et7304_alert_status_clear(struct tcpc_device *tcpc, uint32_t mask)
{
    int ret;
    uint16_t mask_t1;

#if CONFIG_TCPC_VSAFE0V_DETECT_IC
    uint8_t mask_t2;
#endif

    /* Tab A9_V code for P240807-00214 by zhangziyi at 20240927 start */
    ret = et7304_i2c_write8(tcpc, ET7304M_REG_UNLOCK_PW_1, 0x0);
    if (ret < 0) {
        pr_err("%s ET7304M_REG_UNLOCK_PW_1 failed\n", __func__);
        return ret;
    }

    ret = et7304_i2c_write8(tcpc, ET7304M_REG_UNLOCK_PW_2, 0x0);
    if (ret < 0) {
        pr_err("%s ET7304M_REG_UNLOCK_PW_2 failed\n", __func__);
        return ret;
    }
    /* Tab A9_V code for P240807-00214 by zhangziyi at 20240927 end */

    /* Write 1 clear */
    mask_t1 = (uint16_t) mask;
    if (mask_t1) {
        ret = et7304_i2c_write16(tcpc, TCPC_V10_REG_ALERT, mask_t1);
        if (ret < 0)
            return ret;
    }

#if CONFIG_TCPC_VSAFE0V_DETECT_IC
    mask_t2 = mask >> 16;
    if (mask_t2) {
        ret = et7304_i2c_write8(tcpc, ET7304M_REG_RT_INT, mask_t2);
        if (ret < 0)
            return ret;
    }
#endif

    return 0;
}

static int et7304h_set_clock_gating(struct tcpc_device *tcpc, bool en)
{
    int ret = 0;

#if CONFIG_TCPC_CLOCK_GATING
    int i = 0;
    uint8_t clk2 = ET7304M_REG_CLK_DIV_600K_EN
        | ET7304M_REG_CLK_DIV_300K_EN | ET7304M_REG_CLK_CK_300K_EN;
    uint8_t clk3 = ET7304M_REG_CLK_DIV_2P4M_EN;

    if (!en) {
        clk2 |=
            ET7304M_REG_CLK_BCLK2_EN | ET7304M_REG_CLK_BCLK_EN;
        clk3 |=
            ET7304M_REG_CLK_CK_24M_EN | ET7304M_REG_CLK_PCLK_EN;
    }

    if (en) {
        for (i = 0; i < 2; i++)
            ret = et7304_alert_status_clear(tcpc,
                TCPC_REG_ALERT_RX_ALL_MASK);
    }

    if (ret == 0)
        ret = et7304_i2c_write8(tcpc, ET7304M_REG_CLK_CTRL2, clk2);
    if (ret == 0)
        ret = et7304_i2c_write8(tcpc, ET7304M_REG_CLK_CTRL3, clk3);
#endif    /* CONFIG_TCPC_CLOCK_GATING */

    return ret;
}

static inline int et7304h_init_cc_params(
            struct tcpc_device *tcpc, uint8_t cc_res)
{
    int rv = 0;

#if IS_ENABLED(CONFIG_USB_POWER_DELIVERY)
#if CONFIG_USB_PD_SNK_DFT_NO_GOOD_CRC
    uint8_t en, sel;
    struct et7304_chip *chip = tcpc_get_dev_data(tcpc);

    if (cc_res == TYPEC_CC_VOLT_SNK_DFT) {    /* 0.55 */
        en = 0;
        sel = 0x81;
    } else if (chip->chip_id >= RT1715_DID_D) {    /* 0.35 & 0.75 */
        en = 1;
        sel = 0x81;
    } else {    /* 0.4 & 0.7 */
        en = 1;
        sel = 0x80;
    }

    rv = et7304_i2c_write8(tcpc, ET7304M_REG_BMCIO_RXDZEN, en);
    if (rv == 0)
        rv = et7304_i2c_write8(tcpc, ET7304M_REG_BMCIO_RXDZSEL, sel);
#endif    /* CONFIG_USB_PD_SNK_DFT_NO_GOOD_CRC */
#endif    /* CONFIG_USB_POWER_DELIVERY */

    return rv;
}

static int et7304_tcpc_init(struct tcpc_device *tcpc, bool sw_reset)
{
    int ret;
    bool retry_discard_old = false;
    struct et7304_chip *chip = tcpc_get_dev_data(tcpc);

    RT1711_INFO("\n");

    /* Tab A9_V code for P240807-00214 by zhangziyi at 20240927 start */
    ret = et7304_i2c_write8(tcpc, ET7304M_REG_UNLOCK_PW_1, 0x0);
    if (ret < 0) {
        pr_err("et7304 ET7304M_REG_UNLOCK_PW_1 failed\n");
        return ret;
    }
    ret = et7304_i2c_write8(tcpc, ET7304M_REG_UNLOCK_PW_2, 0x0);
    if (ret < 0) {
        pr_err("et7304 ET7304M_REG_UNLOCK_PW_2 failed\n");
        return ret;
    }
    /* Tab A9_V code for P240807-00214 by zhangziyi at 20240927 end */

    if (sw_reset) {
        ret = et7304_software_reset(tcpc);
        if (ret < 0)
            return ret;
    }

#if CONFIG_TCPC_I2CRST_EN
    et7304_i2c_write8(tcpc,
        ET7304M_REG_I2CRST_CTRL,
        ET7304M_REG_I2CRST_SET(true, 0x0f));
#endif    /* CONFIG_TCPC_I2CRST_EN */

    /* UFP Both RD setting */
    /* DRP = 0, RpVal = 0 (Default), Rd, Rd */
    et7304_i2c_write8(tcpc, TCPC_V10_REG_ROLE_CTRL,
        TCPC_V10_REG_ROLE_CTRL_RES_SET(0, 0, CC_RD, CC_RD));

    if (chip->chip_id == ET7304M_DID_A) {
        et7304_i2c_write8(tcpc, TCPC_V10_REG_FAULT_CTRL,
            TCPC_V10_REG_FAULT_CTRL_DIS_VCONN_OV);
    }

    /*
     * CC Detect Debounce : 26.7*val us
     * Transition window count : spec 12~20us, based on 2.4MHz
     * DRP Toggle Cycle : 51.2 + 6.4*val ms
     * DRP Duyt Ctrl : dcSRC: /1024
     */

    et7304_i2c_write8(tcpc, ET7304M_REG_TTCPC_FILTER, 10);
    et7304_i2c_write8(tcpc, ET7304M_REG_DRP_TOGGLE_CYCLE, 4);
    et7304_i2c_write16(tcpc,
        ET7304M_REG_DRP_DUTY_CTRL, TCPC_NORMAL_RP_DUTY);

    /* Vconn OC */
    et7304_i2c_write8(tcpc, ET7304M_REG_VCONN_CLIMITEN, 1);

    /* RX/TX Clock Gating (Auto Mode)*/
    if (!sw_reset)
        et7304h_set_clock_gating(tcpc, true);

    if (!(tcpc->tcpc_flags & TCPC_FLAGS_RETRY_CRC_DISCARD))
        retry_discard_old = true;

    et7304_i2c_write8(tcpc, ET7304M_REG_CONFIG_GPIO0, 0x80);

    /* For BIST, Change Transition Toggle Counter (Noise) from 3 to 7 */
    et7304_i2c_write8(tcpc, ET7304M_REG_PHY_CTRL1,
        ET7304M_REG_PHY_CTRL1_SET(retry_discard_old, 7, 0, 1));

    tcpci_alert_status_clear(tcpc, 0xffffffff);

    et7304_init_power_status_mask(tcpc);
    et7304_init_alert_mask(tcpc);
    et7304_init_fault_mask(tcpc);
    et7304_init_rt_mask(tcpc);

    /* CK_300K from 320K, SHIPPING off, AUTOIDLE enable, TIMEOUT = 6.4ms */
    et7304_i2c_write8(tcpc, ET7304M_REG_IDLE_CTRL,
        ET7304M_REG_IDLE_SET(0, 1, 1, 0));
    mdelay(1);

    return 0;
}

static inline int et7304_fault_status_vconn_ov(struct tcpc_device *tcpc)
{
    int ret;

    ret = et7304_i2c_read8(tcpc, ET7304M_REG_BMC_CTRL);
    if (ret < 0)
        return ret;

    ret &= ~ET7304M_REG_DISCHARGE_EN;
    return et7304_i2c_write8(tcpc, ET7304M_REG_BMC_CTRL, ret);
}

int et7304_fault_status_clear(struct tcpc_device *tcpc, uint8_t status)
{
    int ret;

    if (status & TCPC_V10_REG_FAULT_STATUS_VCONN_OV)
        ret = et7304_fault_status_vconn_ov(tcpc);

    et7304_i2c_write8(tcpc, TCPC_V10_REG_FAULT_STATUS, status);
    return 0;
}

int et7304_get_alert_mask(struct tcpc_device *tcpc, uint32_t *mask)
{
    int ret;
#if CONFIG_TCPC_VSAFE0V_DETECT_IC
    uint8_t v2;
#endif

    ret = et7304_i2c_read16(tcpc, TCPC_V10_REG_ALERT_MASK);
    if (ret < 0)
        return ret;

    *mask = (uint16_t) ret;

#if CONFIG_TCPC_VSAFE0V_DETECT_IC
    ret = et7304_i2c_read8(tcpc, ET7304M_REG_RT_MASK);
    if (ret < 0)
        return ret;

    v2 = (uint8_t) ret;
    *mask |= v2 << 16;
#endif

    return 0;
}

int et7304_get_alert_status(struct tcpc_device *tcpc, uint32_t *alert)
{
    int ret;
#if CONFIG_TCPC_VSAFE0V_DETECT_IC
    uint8_t v2;
#endif

    ret = et7304_i2c_read16(tcpc, TCPC_V10_REG_ALERT);
    if (ret < 0)
        return ret;

    *alert = (uint16_t) ret;

#if CONFIG_TCPC_VSAFE0V_DETECT_IC
    ret = et7304_i2c_read8(tcpc, ET7304M_REG_RT_INT);
    if (ret < 0)
        return ret;

    v2 = (uint8_t) ret;
    *alert |= v2 << 16;
#endif

    return 0;
}

static int et7304_get_power_status(
        struct tcpc_device *tcpc, uint16_t *pwr_status)
{
    int ret;

    ret = et7304_i2c_read8(tcpc, TCPC_V10_REG_POWER_STATUS);
    if (ret < 0)
        return ret;

    *pwr_status = 0;

    if (ret & TCPC_V10_REG_POWER_STATUS_VBUS_PRES)
        *pwr_status |= TCPC_REG_POWER_STATUS_VBUS_PRES;

#if CONFIG_TCPC_VSAFE0V_DETECT_IC
    ret = et7304_i2c_read8(tcpc, ET7304M_REG_RT_STATUS);
    if (ret < 0)
        return ret;

    if (ret & ET7304M_REG_VBUS_80)
        *pwr_status |= TCPC_REG_POWER_STATUS_EXT_VSAFE0V;
#endif
    return 0;
}

int et7304_get_fault_status(struct tcpc_device *tcpc, uint8_t *status)
{
    int ret;

    ret = et7304_i2c_read8(tcpc, TCPC_V10_REG_FAULT_STATUS);
    if (ret < 0)
        return ret;
    *status = (uint8_t) ret;
    return 0;
}

static int et7304_get_cc(struct tcpc_device *tcpc, int *cc1, int *cc2)
{
    int status, role_ctrl, cc_role;
    bool act_as_sink, act_as_drp;

    status = et7304_i2c_read8(tcpc, TCPC_V10_REG_CC_STATUS);
    if (status < 0)
        return status;

    role_ctrl = et7304_i2c_read8(tcpc, TCPC_V10_REG_ROLE_CTRL);
    if (role_ctrl < 0)
        return role_ctrl;

    if (status & TCPC_V10_REG_CC_STATUS_DRP_TOGGLING) {
        *cc1 = TYPEC_CC_DRP_TOGGLING;
        *cc2 = TYPEC_CC_DRP_TOGGLING;
        return 0;
    }

    *cc1 = TCPC_V10_REG_CC_STATUS_CC1(status);
    *cc2 = TCPC_V10_REG_CC_STATUS_CC2(status);

    act_as_drp = TCPC_V10_REG_ROLE_CTRL_DRP & role_ctrl;

    if (act_as_drp) {
        act_as_sink = TCPC_V10_REG_CC_STATUS_DRP_RESULT(status);
    } else {
        cc_role =  TCPC_V10_REG_CC_STATUS_CC1(role_ctrl);
        if (cc_role == TYPEC_CC_RP)
            act_as_sink = false;
        else
            act_as_sink = true;
    }

    /*
     * If status is not open, then OR in termination to convert to
     * enum tcpc_cc_voltage_status.
     */

    if (*cc1 != TYPEC_CC_VOLT_OPEN)
        *cc1 |= (act_as_sink << 2);

    if (*cc2 != TYPEC_CC_VOLT_OPEN)
        *cc2 |= (act_as_sink << 2);

    et7304h_init_cc_params(tcpc,
        (uint8_t)tcpc->typec_polarity ? *cc2 : *cc1);

    return 0;
}

#if CONFIG_TCPC_VSAFE0V_DETECT_IC
static int et7304_enable_vsafe0v_detect(
    struct tcpc_device *tcpc, bool enable)
{
    int ret = et7304_i2c_read8(tcpc, ET7304M_REG_RT_MASK);

    if (ret < 0)
        return ret;

    if (enable)
        ret |= ET7304M_REG_M_VBUS_80;
    else
        ret &= ~ET7304M_REG_M_VBUS_80;

    return et7304_i2c_write8(tcpc, ET7304M_REG_RT_MASK, (uint8_t) ret);
}
#endif /* CONFIG_TCPC_VSAFE0V_DETECT_IC */

static int et7304_set_cc(struct tcpc_device *tcpc, int pull)
{
    int ret;
    uint8_t data;
    int rp_lvl = TYPEC_CC_PULL_GET_RP_LVL(pull), pull1, pull2;

    RT1711_INFO("\n");
    pull = TYPEC_CC_PULL_GET_RES(pull);
    if (pull == TYPEC_CC_DRP) {
        /*Tab A9 code for SR-AX6739A-01-479 by wenyaqi at 20230608 start*/
        #ifdef ET7304M_TYPCA_START_PLUG
        et7304_i2c_write8(tcpc, TCPC_V10_REG_ALERT, 0x01); //01:clear cc irq flag
        et7304_i2c_write8(tcpc, ET7304M_REG_BMC_CTRL, 0x0e); //0e:enter low power
        pr_info("ET7304M enter low power\n");
        #endif // ET7304M_TYPCA_START_PLUG
        /*Tab A9 code for SR-AX6739A-01-479 by wenyaqi at 20230608 end*/
        data = TCPC_V10_REG_ROLE_CTRL_RES_SET(
                1, rp_lvl, TYPEC_CC_RD, TYPEC_CC_RD);

        ret = et7304_i2c_write8(
            tcpc, TCPC_V10_REG_ROLE_CTRL, data);

        if (ret == 0) {
#if CONFIG_TCPC_VSAFE0V_DETECT_IC
            et7304_enable_vsafe0v_detect(tcpc, false);
#endif /* CONFIG_TCPC_VSAFE0V_DETECT_IC */
            ret = et7304_command(tcpc, TCPM_CMD_LOOK_CONNECTION);
        }
    } else {
#if IS_ENABLED(CONFIG_USB_POWER_DELIVERY)
        if (pull == TYPEC_CC_RD && tcpc->pd_wait_pr_swap_complete)
            et7304h_init_cc_params(tcpc, TYPEC_CC_VOLT_SNK_DFT);
#endif    /* CONFIG_USB_POWER_DELIVERY */

        pull1 = pull2 = pull;

        if ((pull == TYPEC_CC_RP_DFT || pull == TYPEC_CC_RP_1_5 ||
            pull == TYPEC_CC_RP_3_0) &&
            tcpc->typec_is_attached_src) {
            if (tcpc->typec_polarity)
                pull1 = TYPEC_CC_OPEN;
            else
                pull2 = TYPEC_CC_OPEN;
        }
        data = TCPC_V10_REG_ROLE_CTRL_RES_SET(0, rp_lvl, pull1, pull2);
        ret = et7304_i2c_write8(tcpc, TCPC_V10_REG_ROLE_CTRL, data);
    }

    return 0;
}

static int et7304_set_polarity(struct tcpc_device *tcpc, int polarity)
{
    int data;

    data = et7304h_init_cc_params(tcpc,
        tcpc->typec_remote_cc[polarity]);
    if (data)
        return data;

    data = et7304_i2c_read8(tcpc, TCPC_V10_REG_TCPC_CTRL);
    if (data < 0)
        return data;

    data &= ~TCPC_V10_REG_TCPC_CTRL_PLUG_ORIENT;
    data |= polarity ? TCPC_V10_REG_TCPC_CTRL_PLUG_ORIENT : 0;

    return et7304_i2c_write8(tcpc, TCPC_V10_REG_TCPC_CTRL, data);
}

static int et7304_set_low_rp_duty(struct tcpc_device *tcpc, bool low_rp)
{
    uint16_t duty = low_rp ? TCPC_LOW_RP_DUTY : TCPC_NORMAL_RP_DUTY;

    return et7304_i2c_write16(tcpc, ET7304M_REG_DRP_DUTY_CTRL, duty);
}

static int et7304_set_vconn(struct tcpc_device *tcpc, int enable)
{
    int rv;
    int data;

    data = et7304_i2c_read8(tcpc, TCPC_V10_REG_POWER_CTRL);
    if (data < 0)
        return data;

    data &= ~TCPC_V10_REG_POWER_CTRL_VCONN;
    data |= enable ? TCPC_V10_REG_POWER_CTRL_VCONN : 0;

    rv = et7304_i2c_write8(tcpc, TCPC_V10_REG_POWER_CTRL, data);
    if (rv < 0)
        return rv;

    return et7304_i2c_write8(tcpc, ET7304M_REG_IDLE_CTRL,
        ET7304M_REG_IDLE_SET(0, 1, enable ? 0 : 1, 0));
}

#if CONFIG_TCPC_LOW_POWER_MODE
static int et7304_is_low_power_mode(struct tcpc_device *tcpc)
{
    int rv = et7304_i2c_read8(tcpc, ET7304M_REG_BMC_CTRL);

    if (rv < 0)
        return rv;

    return (rv & ET7304M_REG_BMCIO_LPEN) != 0;
}

static int et7304_set_low_power_mode(
        struct tcpc_device *tcpc, bool en, int pull)
{
    int ret = 0;
    uint8_t data;

    ret = et7304_i2c_write8(tcpc, ET7304M_REG_IDLE_CTRL,
        ET7304M_REG_IDLE_SET(0, 1, en ? 0 : 1, 0));
    if (ret < 0)
        return ret;
#if CONFIG_TCPC_VSAFE0V_DETECT_IC
    et7304_enable_vsafe0v_detect(tcpc, !en);
#endif /* CONFIG_TCPC_VSAFE0V_DETECT_IC */
    if (en) {
        data = ET7304M_REG_BMCIO_LPEN;

        if (pull & TYPEC_CC_RP)
            data |= ET7304M_REG_BMCIO_LPRPRD;

#if CONFIG_TYPEC_CAP_NORP_SRC
        data |= ET7304M_REG_BMCIO_BG_EN | ET7304M_REG_VBUS_DET_EN;
#endif
    } else {
        data = ET7304M_REG_BMCIO_BG_EN |
            ET7304M_REG_VBUS_DET_EN | ET7304M_REG_BMCIO_OSC_EN;
    }

    return et7304_i2c_write8(tcpc, ET7304M_REG_BMC_CTRL, data);
}
#endif    /* CONFIG_TCPC_LOW_POWER_MODE */

#if CONFIG_TCPC_WATCHDOG_EN
int et7304h_set_watchdog(struct tcpc_device *tcpc, bool en)
{
    uint8_t data = ET7304M_REG_WATCHDOG_CTRL_SET(en, 7);

    return et7304_i2c_write8(tcpc,
        ET7304M_REG_WATCHDOG_CTRL, data);
}
#endif    /* CONFIG_TCPC_WATCHDOG_EN */

#if CONFIG_TCPC_INTRST_EN
int et7304h_set_intrst(struct tcpc_device *tcpc, bool en)
{
    return et7304_i2c_write8(tcpc,
        ET7304M_REG_INTRST_CTRL, ET7304M_REG_INTRST_SET(en, 3));
}
#endif    /* CONFIG_TCPC_INTRST_EN */

static int et7304_tcpc_deinit(struct tcpc_device *tcpc)
{
#if IS_ENABLED(CONFIG_RT_REGMAP)
    struct et7304_chip *chip = tcpc_get_dev_data(tcpc);
#endif /* CONFIG_RT_REGMAP */

#if CONFIG_TCPC_SHUTDOWN_CC_DETACH
    et7304_set_cc(tcpc, TYPEC_CC_DRP);
    et7304_set_cc(tcpc, TYPEC_CC_OPEN);

    et7304_i2c_write8(tcpc,
        ET7304M_REG_I2CRST_CTRL,
        ET7304M_REG_I2CRST_SET(true, 4));

    et7304_i2c_write8(tcpc,
        ET7304M_REG_INTRST_CTRL,
        ET7304M_REG_INTRST_SET(true, 0));
#else
    et7304_i2c_write8(tcpc, ET7304M_REG_SWRESET, 1);
#endif    /* CONFIG_TCPC_SHUTDOWN_CC_DETACH */
#if IS_ENABLED(CONFIG_RT_REGMAP)
    rt_regmap_cache_reload(chip->m_dev);
#endif /* CONFIG_RT_REGMAP */

    return 0;
}

#if IS_ENABLED(CONFIG_USB_POWER_DELIVERY)
static int et7304_set_msg_header(
    struct tcpc_device *tcpc, uint8_t power_role, uint8_t data_role)
{
    uint8_t msg_hdr = TCPC_V10_REG_MSG_HDR_INFO_SET(
        data_role, power_role);

    return et7304_i2c_write8(
        tcpc, TCPC_V10_REG_MSG_HDR_INFO, msg_hdr);
}

static int et7304_protocol_reset(struct tcpc_device *tcpc)
{
    et7304_i2c_write8(tcpc, ET7304M_REG_PRL_FSM_RESET, 0);
    mdelay(1);
    et7304_i2c_write8(tcpc, ET7304M_REG_PRL_FSM_RESET, 1);
    return 0;
}

static int et7304_set_rx_enable(struct tcpc_device *tcpc, uint8_t enable)
{
    int ret = 0;

    if (enable)
        ret = et7304h_set_clock_gating(tcpc, false);

    if (ret == 0)
        ret = et7304_i2c_write8(tcpc, TCPC_V10_REG_RX_DETECT, enable);

    if ((ret == 0) && (!enable)) {
        et7304_protocol_reset(tcpc);
        ret = et7304h_set_clock_gating(tcpc, true);
    }

    return ret;
}

static int et7304_get_message(struct tcpc_device *tcpc, uint32_t *payload,
            uint16_t *msg_head, enum tcpm_transmit_type *frame_type)
{
    struct et7304_chip *chip = tcpc_get_dev_data(tcpc);
    int rv;
    uint8_t type, cnt = 0;
    uint8_t buf[4];
    const uint16_t alert_rx =
        TCPC_V10_REG_ALERT_RX_STATUS|TCPC_V10_REG_RX_OVERFLOW;

    rv = et7304_block_read(chip->client,
            TCPC_V10_REG_RX_BYTE_CNT, 4, buf);
    cnt = buf[0];
    type = buf[1];
    *msg_head = *(uint16_t *)&buf[2];

    /* TCPC 1.0 ==> no need to subtract the size of msg_head */
    if (rv >= 0 && cnt > 3) {
        cnt -= 3; /* MSG_HDR */
        rv = et7304_block_read(chip->client, TCPC_V10_REG_RX_DATA, cnt,
                (uint8_t *) payload);
    }

    *frame_type = (enum tcpm_transmit_type) type;

    /* Read complete, clear RX status alert bit */
    tcpci_alert_status_clear(tcpc, alert_rx);

    /*mdelay(1); */
    return rv;
}

static int et7304_set_bist_carrier_mode(
    struct tcpc_device *tcpc, uint8_t pattern)
{
    /* Don't support this function */
    return 0;
}

#if CONFIG_USB_PD_RETRY_CRC_DISCARD
static int et7304_retransmit(struct tcpc_device *tcpc)
{
    return et7304_i2c_write8(tcpc, TCPC_V10_REG_TRANSMIT,
            TCPC_V10_REG_TRANSMIT_SET(
            tcpc->pd_retry_count, TCPC_TX_SOP));
}
#endif

#pragma pack(push, 1)
struct tcpc_transmit_packet {
    uint8_t cnt;
    uint16_t msg_header;
    uint8_t data[sizeof(uint32_t)*7];
};
#pragma pack(pop)

static int et7304_transmit(struct tcpc_device *tcpc,
    enum tcpm_transmit_type type, uint16_t header, const uint32_t *data)
{
    struct et7304_chip *chip = tcpc_get_dev_data(tcpc);
    int rv;
    int data_cnt;
    struct tcpc_transmit_packet packet;

    if (type < TCPC_TX_HARD_RESET) {
        data_cnt = sizeof(uint32_t) * PD_HEADER_CNT(header);

        packet.cnt = data_cnt + sizeof(uint16_t);
        packet.msg_header = header;

        if (data_cnt > 0)
            memcpy(packet.data, (uint8_t *) data, data_cnt);

        rv = et7304_block_write(chip->client,
                TCPC_V10_REG_TX_BYTE_CNT,
                packet.cnt+1, (uint8_t *) &packet);
        if (rv < 0)
            return rv;
    }

    rv = et7304_i2c_write8(tcpc, TCPC_V10_REG_TRANSMIT,
            TCPC_V10_REG_TRANSMIT_SET(
            tcpc->pd_retry_count, type));
    return rv;
}

static int et7304_set_bist_test_mode(struct tcpc_device *tcpc, bool en)
{
    int data;

    data = et7304_i2c_read8(tcpc, TCPC_V10_REG_TCPC_CTRL);
    if (data < 0)
        return data;

    data &= ~TCPC_V10_REG_TCPC_CTRL_BIST_TEST_MODE;
    data |= en ? TCPC_V10_REG_TCPC_CTRL_BIST_TEST_MODE : 0;

    return et7304_i2c_write8(tcpc, TCPC_V10_REG_TCPC_CTRL, data);
}
#endif /* CONFIG_USB_POWER_DELIVERY */

static struct tcpc_ops et7304_tcpc_ops = {
    .init = et7304_tcpc_init,
    .alert_status_clear = et7304_alert_status_clear,
    .fault_status_clear = et7304_fault_status_clear,
    .get_alert_mask = et7304_get_alert_mask,
    .get_alert_status = et7304_get_alert_status,
    .get_power_status = et7304_get_power_status,
    .get_fault_status = et7304_get_fault_status,
    .get_cc = et7304_get_cc,
    .set_cc = et7304_set_cc,
    .set_polarity = et7304_set_polarity,
    .set_low_rp_duty = et7304_set_low_rp_duty,
    .set_vconn = et7304_set_vconn,
    .deinit = et7304_tcpc_deinit,

#if CONFIG_TCPC_LOW_POWER_MODE
    .is_low_power_mode = et7304_is_low_power_mode,
    .set_low_power_mode = et7304_set_low_power_mode,
#endif    /* CONFIG_TCPC_LOW_POWER_MODE */

#if CONFIG_TCPC_WATCHDOG_EN
    .set_watchdog = et7304h_set_watchdog,
#endif    /* CONFIG_TCPC_WATCHDOG_EN */

#if CONFIG_TCPC_INTRST_EN
    .set_intrst = et7304h_set_intrst,
#endif    /* CONFIG_TCPC_INTRST_EN */

#if IS_ENABLED(CONFIG_USB_POWER_DELIVERY)
    .set_msg_header = et7304_set_msg_header,
    .set_rx_enable = et7304_set_rx_enable,
    .protocol_reset = et7304_protocol_reset,
    .get_message = et7304_get_message,
    .transmit = et7304_transmit,
    .set_bist_test_mode = et7304_set_bist_test_mode,
    .set_bist_carrier_mode = et7304_set_bist_carrier_mode,
#endif    /* CONFIG_USB_POWER_DELIVERY */

#if CONFIG_USB_PD_RETRY_CRC_DISCARD
    .retransmit = et7304_retransmit,
#endif    /* CONFIG_USB_PD_RETRY_CRC_DISCARD */
};

static int rt_parse_dt(struct et7304_chip *chip, struct device *dev)
{
    struct device_node *np = NULL;
    int ret = 0;

    pr_info("%s\n", __func__);

    np = of_find_node_by_name(NULL, "et7304_type_c_port0");
    if (!np) {
        pr_notice("%s find node et7304_type_c_port0 fail\n", __func__);
        return -ENODEV;
    }
    dev->of_node = np;

#if !IS_ENABLED(CONFIG_MTK_GPIO) || IS_ENABLED(CONFIG_MTK_GPIOLIB_STAND)
    ret = of_get_named_gpio(np, "et7304pd,intr_gpio", 0);
    if (ret < 0) {
        pr_err("%s no intr_gpio info\n", __func__);
        return ret;
    }
    chip->irq_gpio = ret;
#else
    ret = of_property_read_u32(np,
        "et7304pd,intr_gpio_num", &chip->irq_gpio);
    if (ret < 0)
        pr_err("%s no intr_gpio info\n", __func__);
#endif
    return ret < 0 ? ret : 0;
}

/*Tab A9 U code for AX6739AU-120|P240115-07380 by wenyaqi at 20240312 start*/
static inline int et7304_try_to_reset(struct i2c_client *client)
{
    unsigned short addr;
    union i2c_smbus_data data;

    data.byte = 1;

    for (addr = 0x40; addr <= 0x4F; addr++) {
        if (!i2c_smbus_xfer(client->adapter, addr, client->flags,
                    I2C_SMBUS_WRITE, 0xA0, I2C_SMBUS_BYTE_DATA,
                    &data)) {
            dev_info(&client->dev, "reset et7304 at 0x%2x\n", addr);
            return 0;
        }
    }
    dev_info(&client->dev, "reset et7304 failed\n");
    return -EIO;
}
/*Tab A9 U code for AX6739AU-120|P240115-07380 by wenyaqi at 20240312 end*/


/*
 * In some platform pr_info may spend too much time on printing debug message.
 * So we use this function to test the printk performance.
 * If your platform cannot not pass this check function, please config
 * PD_DBG_INFO, this will provide the threaded debug message for you.
 */
#if TCPC_ENABLE_ANYMSG
static void check_printk_performance(void)
{
    int i;
    u64 t1, t2;
    u32 nsrem;

#if IS_ENABLED(CONFIG_PD_DBG_INFO)
    for (i = 0; i < 10; i++) {
        t1 = local_clock();
        pd_dbg_info("%d\n", i);
        t2 = local_clock();
        t2 -= t1;
        nsrem = do_div(t2, 1000000000);
        pd_dbg_info("pd_dbg_info : t2-t1 = %lu\n",
                (unsigned long)nsrem / 1000);
    }
    for (i = 0; i < 10; i++) {
        t1 = local_clock();
        pr_info("%d\n", i);
        t2 = local_clock();
        t2 -= t1;
        nsrem = do_div(t2, 1000000000);
        pr_info("pr_info : t2-t1 = %lu\n",
                (unsigned long)nsrem / 1000);
    }
#else
    for (i = 0; i < 10; i++) {
        t1 = local_clock();
        pr_info("%d\n", i);
        t2 = local_clock();
        t2 -= t1;
        nsrem = do_div(t2, 1000000000);
        pr_info("t2-t1 = %lu\n",
                (unsigned long)nsrem /  1000);
        PD_BUG_ON(nsrem > 100*1000);
    }
#endif /* CONFIG_PD_DBG_INFO */
}
#endif /* TCPC_ENABLE_ANYMSG */

static int et7304_tcpcdev_init(struct et7304_chip *chip, struct device *dev)
{
    struct tcpc_desc *desc;
    struct device_node *np = dev->of_node;
    u32 val, len;
    const char *name = "default";

    dev_info(dev, "%s\n", __func__);

    desc = devm_kzalloc(dev, sizeof(*desc), GFP_KERNEL);
    if (!desc)
        return -ENOMEM;
    if (of_property_read_u32(np, "et-tcpc,role_def", &val) >= 0) {
        if (val >= TYPEC_ROLE_NR)
            desc->role_def = TYPEC_ROLE_DRP;
        else
            desc->role_def = val;
    } else {
        dev_info(dev, "use default Role DRP\n");
        desc->role_def = TYPEC_ROLE_DRP;
    }

    if (of_property_read_u32(
        np, "et-tcpc,notifier_supply_num", &val) >= 0) {
        if (val < 0)
            desc->notifier_supply_num = 0;
        else
            desc->notifier_supply_num = val;
    } else
        desc->notifier_supply_num = 0;

    if (of_property_read_u32(np, "et-tcpc,rp_level", &val) >= 0) {
        switch (val) {
        case 0: /* RP Default */
            desc->rp_lvl = TYPEC_CC_RP_DFT;
            break;
        case 1: /* RP 1.5V */
            desc->rp_lvl = TYPEC_CC_RP_1_5;
            break;
        case 2: /* RP 3.0V */
            desc->rp_lvl = TYPEC_CC_RP_3_0;
            break;
        default:
            break;
        }
    }

#if CONFIG_TCPC_VCONN_SUPPLY_MODE
    if (of_property_read_u32(np, "et-tcpc,vconn_supply", &val) >= 0) {
        if (val >= TCPC_VCONN_SUPPLY_NR)
            desc->vconn_supply = TCPC_VCONN_SUPPLY_ALWAYS;
        else
            desc->vconn_supply = val;
    } else {
        dev_info(dev, "use default VconnSupply\n");
        desc->vconn_supply = TCPC_VCONN_SUPPLY_ALWAYS;
    }
#endif    /* CONFIG_TCPC_VCONN_SUPPLY_MODE */

    if (of_property_read_string(np, "et-tcpc,name",
                (char const **)&name) < 0) {
        dev_info(dev, "use default name\n");
    }

    len = strlen(name);
    desc->name = kzalloc(len+1, GFP_KERNEL);
    if (!desc->name)
        return -ENOMEM;

    strlcpy((char *)desc->name, name, len+1);

    chip->tcpc_desc = desc;

    chip->tcpc = tcpc_device_register(dev,
            desc, &et7304_tcpc_ops, chip);
    if (IS_ERR(chip->tcpc))
        return -EINVAL;

    chip->tcpc->tcpc_flags = TCPC_FLAGS_LPM_WAKEUP_WATCHDOG |
            TCPC_FLAGS_VCONN_SAFE5V_ONLY;

    if (chip->chip_id > ET7304M_DID_B)
        chip->tcpc->tcpc_flags |= TCPC_FLAGS_CHECK_RA_DETACHE;

#if CONFIG_USB_PD_RETRY_CRC_DISCARD
    if (chip->chip_id > RT1715_DID_D)
        chip->tcpc->tcpc_flags |= TCPC_FLAGS_RETRY_CRC_DISCARD;
#endif  /* CONFIG_USB_PD_RETRY_CRC_DISCARD */

#if CONFIG_USB_PD_REV30
    if (chip->chip_id >= RT1715_DID_D)
        chip->tcpc->tcpc_flags |= TCPC_FLAGS_PD_REV30;

    if (chip->tcpc->tcpc_flags & TCPC_FLAGS_PD_REV30)
        dev_info(dev, "PD_REV30\n");
    else
        dev_info(dev, "PD_REV20\n");
#endif    /* CONFIG_USB_PD_REV30 */
    chip->tcpc->tcpc_flags |= TCPC_FLAGS_ALERT_V10;

    return 0;
}

#define RICHTEK_1711_VID    0x29cf
#define RICHTEK_1711_PID    0x1711

 #define ETEK_7304_VID    0x6dcf
 #define ETEK_7304_PID    0x1711

static inline int et7304h_check_revision(struct i2c_client *client)
{
    u16 vid, pid, did;
    int ret;
    u8 data = 1;

    /*Tab A9 U code for P240115-07380 by wenyaqi at 20240312 start*/
    if (et7304_try_to_reset(client)) {
        dev_err(&client->dev, "%s: reset IC fail\n", __func__);
        return -EIO;
    } else {
        usleep_range(1000, 2000);
        ret =  et7304_read_device(client, TCPC_V10_REG_VID, 2, &vid);
        if (ret < 0) {
            dev_err(&client->dev, "%s: read chip ID fail\n", __func__);
            return ret;
        }
    }
    /*Tab A9 U code for P240115-07380 by wenyaqi at 20240312 end*/

    if(vid==0x29CF||vid==0x2dCF||vid==0x69CF||vid==0x6dCF)
    {
      ret = et7304_write_device(client, 0xA0, 1, &data);
        if (ret < 0)
        return ret;
        usleep_range(1000, 2000);
       ret = et7304_read_device(client, TCPC_V10_REG_VID, 2, &vid);
        if (ret < 0)
        return ret;
    }

    if (vid != ETEK_7304_VID ) {
        pr_info("%s failed, VID=0x%04x\n", __func__, vid);
        return -ENODEV;
    }

    ret = et7304_read_device(client, TCPC_V10_REG_PID, 2, &pid);
    if (ret < 0) {
        dev_err(&client->dev, "read product ID fail\n");
        return -EIO;
    }

    if (pid != ETEK_7304_PID) {
        pr_info("%s failed, PID=0x%04x\n", __func__, pid);
        return -ENODEV;
    }

    ret = et7304_write_device(client, ET7304M_REG_SWRESET, 1, &data);
    if (ret < 0)
        return ret;

    usleep_range(1000, 2000);

    ret = et7304_read_device(client, TCPC_V10_REG_DID, 2, &did);
    if (ret < 0) {
        dev_err(&client->dev, "read device ID fail\n");
        return -EIO;
    }

    return did;
}

static int et7304_i2c_probe(struct i2c_client *client,
                const struct i2c_device_id *id)
{
    struct et7304_chip *chip;
    int ret = 0, chip_id;
    bool use_dt = client->dev.of_node;

    pr_info("%s (%s)\n", __func__, ET7304M_DRV_VERSION);
    if (i2c_check_functionality(client->adapter,
            I2C_FUNC_SMBUS_I2C_BLOCK | I2C_FUNC_SMBUS_BYTE_DATA))
        pr_info("I2C functionality : OK...\n");
    else
        pr_info("I2C functionality check : failuare...\n");

    chip_id = et7304h_check_revision(client);
    if (chip_id < 0)
        return chip_id;

    /*Tab A9 code for SR-AX6739A-01-470 by wenyaqi at 20230504 start*/
    /* set for tcpc_info */
    gxy_bat_set_tcpcinfo(GXY_BAT_TCPC_INFO_ET7304MQ);
    /*Tab A9 code for SR-AX6739A-01-470 by wenyaqi at 20230504 end*/

#if TCPC_ENABLE_ANYMSG
    check_printk_performance();
#endif /* TCPC_ENABLE_ANYMSG */

    chip = devm_kzalloc(&client->dev, sizeof(*chip), GFP_KERNEL);
    if (!chip)
        return -ENOMEM;

    if (use_dt) {
        ret = rt_parse_dt(chip, &client->dev);
        if (ret < 0)
            return ret;
    } else {
        dev_err(&client->dev, "no dts node\n");
        return -ENODEV;
    }
    chip->dev = &client->dev;
    chip->client = client;
    sema_init(&chip->io_lock, 1);
    sema_init(&chip->suspend_lock, 1);
    i2c_set_clientdata(client, chip);
    // INIT_DELAYED_WORK(&chip->poll_work, et7304_poll_work);
    chip->irq_wake_lock =
        wakeup_source_register(chip->dev, "et7304h_irq_wake_lock");

    chip->chip_id = chip_id;
    pr_info("et7304h_chipID = 0x%0x\n", chip_id);

    ret = et7304_regmap_init(chip);
    if (ret < 0) {
        dev_err(chip->dev, "et7304 regmap init fail\n");
        goto err_regmap_init;
    }

    ret = et7304_tcpcdev_init(chip, &client->dev);
    if (ret < 0) {
        dev_err(&client->dev, "et7304 tcpc dev init fail\n");
        goto err_tcpc_reg;
    }

    ret = et7304_init_alert(chip->tcpc);
    if (ret < 0) {
        pr_err("et7304 init alert fail\n");
        goto err_irq_init;
    }

    tcpc_schedule_init_work(chip->tcpc);
    pr_info("%s probe OK!\n", __func__);
    return 0;

err_irq_init:
    tcpc_device_unregister(chip->dev, chip->tcpc);
err_tcpc_reg:
    et7304_regmap_deinit(chip);
err_regmap_init:
    wakeup_source_unregister(chip->irq_wake_lock);
    return ret;
}

static int et7304_i2c_remove(struct i2c_client *client)
{
    struct et7304_chip *chip = i2c_get_clientdata(client);

    if (chip) {
        cancel_delayed_work_sync(&chip->poll_work);

        tcpc_device_unregister(chip->dev, chip->tcpc);
        et7304_regmap_deinit(chip);
    }

    return 0;
}

#if CONFIG_PM
static int et7304_i2c_suspend(struct device *dev)
{
    struct et7304_chip *chip;
    struct i2c_client *client = to_i2c_client(dev);

    if (client) {
        chip = i2c_get_clientdata(client);
        if (chip)
            down(&chip->suspend_lock);
    }

    return 0;
}

static int et7304_i2c_resume(struct device *dev)
{
    struct et7304_chip *chip;
    struct i2c_client *client = to_i2c_client(dev);

    if (client) {
        chip = i2c_get_clientdata(client);
        if (chip)
            up(&chip->suspend_lock);
    }

    return 0;
}

static void et7304_shutdown(struct i2c_client *client)
{
    struct et7304_chip *chip = i2c_get_clientdata(client);

    /* Please reset IC here */
    if (chip != NULL) {
        if (chip->irq)
            disable_irq(chip->irq);
        tcpm_shutdown(chip->tcpc);
    } else {
        i2c_smbus_write_byte_data(
            client, ET7304M_REG_SWRESET, 0x01);
    }
}

#if IS_ENABLED(CONFIG_PM_RUNTIME)
static int et7304_pm_suspend_runtime(struct device *device)
{
    dev_dbg(device, "pm_runtime: suspending...\n");
    return 0;
}

static int et7304_pm_resume_runtime(struct device *device)
{
    dev_dbg(device, "pm_runtime: resuming...\n");
    return 0;
}
#endif /* CONFIG_PM_RUNTIME */

static const struct dev_pm_ops et7304_pm_ops = {
    SET_SYSTEM_SLEEP_PM_OPS(
            et7304_i2c_suspend,
            et7304_i2c_resume)
#if IS_ENABLED(CONFIG_PM_RUNTIME)
    SET_RUNTIME_PM_OPS(
        et7304_pm_suspend_runtime,
        et7304_pm_resume_runtime,
        NULL
    )
#endif /* CONFIG_PM_RUNTIME */
};
#define RT1711_PM_OPS    (&et7304_pm_ops)
#else
#define RT1711_PM_OPS    (NULL)
#endif /* CONFIG_PM */

static const struct i2c_device_id et7304_id_table[] = {
    {"et7304h", 0},
    {"rt1715", 0},
    {"rt1716", 0},
    {},
};
MODULE_DEVICE_TABLE(i2c, et7304_id_table);

static const struct of_device_id rt_match_table[] = {
    {.compatible = "etek,et7304",},
    {},
};

static struct i2c_driver et7304_driver = {
    .driver = {
        .name = "usb_type_c",
        .owner = THIS_MODULE,
        .of_match_table = rt_match_table,
        .pm = RT1711_PM_OPS,
    },
    .probe = et7304_i2c_probe,
    .remove = et7304_i2c_remove,
    .shutdown = et7304_shutdown,
    .id_table = et7304_id_table,
};

static int __init et7304_init(void)
{
    struct device_node *np;

    pr_info("%s (%s): initializing...\n", __func__, ET7304M_DRV_VERSION);
    np = of_find_node_by_name(NULL, "usb_type_c");
    if (np != NULL)
        pr_info("usb_type_c node found...\n");
    else
        pr_info("usb_type_c node not found...\n");

    return i2c_add_driver(&et7304_driver);
}
subsys_initcall(et7304_init);

static void __exit et7304_exit(void)
{
    i2c_del_driver(&et7304_driver);
}
module_exit(et7304_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jeff Chang <jeff_chang@richtek.com>");
MODULE_DESCRIPTION("RT1711 TCPC Driver");
MODULE_VERSION(ET7304M_DRV_VERSION);

/**** Release Note ****
 * 2.0.5_MTK
 * (1) Utilize rt-regmap to reduce I2C accesses
 * (2) Decrease VBUS present threshold (VBUS_CAL) by 60mV (2LSBs)
 *
 * 2.0.4_MTK
 * (1) Mask vSafe0V IRQ before entering low power mode
 * (2) Disable auto idle mode before entering low power mode
 * (3) Reset Protocol FSM and clear RX alerts twice before clock gating
 *
 * 2.0.3_MTK
 * (1) Single Rp as Attatched.SRC for Ellisys TD.4.9.4
 *
 * 2.0.2_MTK
 * (1) Replace wake_lock with wakeup_source
 * (2) Move down the shipping off
 * (3) Add support for NoRp.SRC
 * (4) Reg0x71[7] = 1'b1 to workaround unstable VDD Iq in low power mode
 * (5) Add get_alert_mask of tcpc_ops
 *
 * 2.0.1_MTK
 * First released PD3.0 Driver on MTK platform
 */
