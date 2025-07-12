
#pragma once

#include <Arduino.h>  


static inline void gpio_ll_get_drive_capability(gpio_dev_t *hw, uint32_t gpio_num, gpio_drive_cap_t *strength)
{
    uint32_t drv_cap = GET_PERI_REG_BITS2(IO_MUX_GPIO0_REG + (gpio_num * 4), FUN_DRV_V, FUN_DRV_S);
    // DRV = 1 and DRV = 2 register bits are flipped for IO17, IO18 on the target
    if (gpio_num == 17 || gpio_num == 18) {
        drv_cap = ((drv_cap & 0x1) << 1) | ((drv_cap & 0x2) >> 1); // swap bit0 and bit1
    }
    *strength = (gpio_drive_cap_t)drv_cap;
}

typedef struct {
    uint32_t fun_sel;               /*!< Value of IOMUX function selection */
    uint32_t sig_out;               /*!< Index of the outputting peripheral signal */
    gpio_drive_cap_t drv;           /*!< Value of drive strength */
    bool pu;                        /*!< Status of pull-up enabled or not */
    bool pd;                        /*!< Status of pull-down enabled or not */
    bool ie;                        /*!< Status of input enabled or not */
    bool oe;                        /*!< Status of output enabled or not */
    bool oe_ctrl_by_periph;         /*!< True if use output enable signal from peripheral, otherwise False */
    bool oe_inv;                    /*!< Whether the output enable signal is inversed or not */
    bool od;                        /*!< Status of open-drain enabled or not */
    bool slp_sel;                   /*!< Status of pin sleep mode enabled or not */
} gpio_io_config_t;

static inline void gpio_ll_get_io_config(gpio_dev_t *hw, uint32_t gpio_num, gpio_io_config_t *io_config)
{
    uint32_t bit_shift = (gpio_num < 32) ? gpio_num : (gpio_num - 32);
    uint32_t bit_mask = 1 << bit_shift;
    uint32_t iomux_reg_val = REG_READ(GPIO_PIN_MUX_REG[gpio_num]);
    io_config->pu = (iomux_reg_val & FUN_PU_M) >> FUN_PU_S;
    io_config->pd = (iomux_reg_val & FUN_PD_M) >> FUN_PD_S;
    io_config->ie = (iomux_reg_val & FUN_IE_M) >> FUN_IE_S;
    io_config->oe = (((gpio_num < 32) ? hw->enable : hw->enable1.val) & bit_mask) >> bit_shift;
    io_config->oe_ctrl_by_periph = !(hw->func_out_sel_cfg[gpio_num].oen_sel);
    io_config->oe_inv = hw->func_out_sel_cfg[gpio_num].oen_inv_sel;
    io_config->od = hw->pin[gpio_num].pad_driver;
    gpio_ll_get_drive_capability(hw, gpio_num, &(io_config->drv)); // specific workaround in the LL
    io_config->fun_sel = (iomux_reg_val & MCU_SEL_M) >> MCU_SEL_S;
    io_config->sig_out = hw->func_out_sel_cfg[gpio_num].func_sel;
    io_config->slp_sel = (iomux_reg_val & SLP_SEL_M) >> SLP_SEL_S;
}

typedef struct {
    gpio_isr_t fn;   /*!< isr function */
    void *args;      /*!< isr function args */
} gpio_isr_func_t;

typedef struct {
    gpio_dev_t *dev;
} gpio_hal_context_t;

typedef struct {
    gpio_hal_context_t *gpio_hal;
    portMUX_TYPE gpio_spinlock;
    uint32_t isr_core_id;
    gpio_isr_func_t *gpio_isr_func;
    gpio_isr_handle_t gpio_isr_handle;
    uint64_t isr_clr_on_entry_mask; // for edge-triggered interrupts, interrupt status bits should be cleared before entering per-pin handlers
} gpio_context_t;

#define GPIO_LL_GET_HW(num) (((num) == 0) ? (&GPIO) : NULL)
#define GPIO_HAL_GET_HW(num) GPIO_LL_GET_HW(num)

static gpio_hal_context_t _gpio_hal = {
    .dev = GPIO_HAL_GET_HW(GPIO_PORT_0)
};

#define GPIO_ISR_CORE_ID_UNINIT    (3)

static gpio_context_t gpio_context = {
    .gpio_hal = &_gpio_hal,
    .gpio_spinlock = portMUX_INITIALIZER_UNLOCKED,
    .isr_core_id = GPIO_ISR_CORE_ID_UNINIT,
    .gpio_isr_func = NULL,
    .isr_clr_on_entry_mask = 0,
};

#define ESP_RETURN_ON_FALSE(a, err_code, log_tag, format, ...) do {                             \
        (void)log_tag;                                                                          \
        if (unlikely(!(a))) {                                                                   \
            return err_code;                                                                    \
        }                                                                                       \
    } while(0)

static const char *GPIO_TAG = "gpio";
#define GPIO_CHECK(a, str, ret_val) ESP_RETURN_ON_FALSE(a, ret_val, GPIO_TAG, "%s", str)
#define gpio_hal_get_io_config(hal, gpio_num, out_io_config) gpio_ll_get_io_config((hal)->dev, gpio_num, out_io_config)

esp_err_t gpio_get_io_config(gpio_num_t gpio_num, gpio_io_config_t *out_io_config)
{
    GPIO_CHECK(GPIO_IS_VALID_GPIO(gpio_num), "GPIO number error", ESP_ERR_INVALID_ARG);
    ESP_RETURN_ON_FALSE(out_io_config, ESP_ERR_INVALID_ARG, GPIO_TAG, "out_io_config is a null pointer");

    gpio_hal_get_io_config(gpio_context.gpio_hal, gpio_num, out_io_config);
#if !SOC_GPIO_SUPPORT_RTC_INDEPENDENT && SOC_RTCIO_PIN_COUNT > 0
    if (rtc_gpio_is_valid_gpio(gpio_num)) {
        int rtcio_num = rtc_io_number_get(gpio_num);
        out_io_config->pu = rtcio_hal_is_pullup_enabled(rtcio_num);
        out_io_config->pd = rtcio_hal_is_pulldown_enabled(rtcio_num);
        out_io_config->drv = rtcio_hal_get_drive_capability(rtcio_num);
    }
#endif
    return ESP_OK;
}

/*
#include <stdatomic.h>

static atomic_uint_fast64_t s_reserved_pin_mask = ATOMIC_VAR_INIT(~(SOC_GPIO_VALID_GPIO_MASK));
#define __auto_type auto

bool esp_gpio_is_reserved(uint64_t gpio_mask)
{
    return atomic_load(&s_reserved_pin_mask) & gpio_mask;
}
*/

static inline int gpio_ll_get_in_signal_connected_io(gpio_dev_t *hw, uint32_t in_sig_idx)
{
    typeof(hw->func_in_sel_cfg[in_sig_idx]) reg;
    reg.val = hw->func_in_sel_cfg[in_sig_idx].val;
    return (reg.sig_in_sel ? reg.func_sel : -1);
}

#define gpio_hal_get_in_signal_connected_io(hal, in_sig_idx) gpio_ll_get_in_signal_connected_io((hal)->dev, in_sig_idx)

esp_err_t gpio_dump_io_configuration(FILE *out_stream, uint64_t io_bit_mask)
{
    ESP_RETURN_ON_FALSE(out_stream, ESP_ERR_INVALID_ARG, GPIO_TAG, "out_stream error");
    ESP_RETURN_ON_FALSE(!(io_bit_mask & ~SOC_GPIO_VALID_GPIO_MASK), ESP_ERR_INVALID_ARG, GPIO_TAG, "io_bit_mask error");

    fprintf(out_stream, "================IO DUMP Start================\n");
    while (io_bit_mask) {
        uint32_t gpio_num = __builtin_ffsll(io_bit_mask) - 1;
        io_bit_mask &= ~(1ULL << gpio_num);

        gpio_io_config_t io_config = {};
        gpio_get_io_config((gpio_num_t)gpio_num, &io_config);

        // When the IO is used as a simple GPIO output, oe signal can only be controlled by the oe register
        // When the IO is not used as a simple GPIO output, oe signal could be controlled by the peripheral
        const char *oe_str = io_config.oe ? "1" : "0";
        if (io_config.sig_out != SIG_GPIO_OUT_IDX && io_config.oe_ctrl_by_periph) {
            oe_str = "[periph_sig_ctrl]";
        }

        fprintf(out_stream, "IO[%" PRIu32 "]%s -\n", gpio_num, ""); // esp_gpio_is_reserved(BIT64(gpio_num)) ? " **RESERVED**" : "");
        fprintf(out_stream, "  Pullup: %d, Pulldown: %d, DriveCap: %" PRIu32 "\n", io_config.pu, io_config.pd, (uint32_t)io_config.drv);
        fprintf(out_stream, "  InputEn: %d, OutputEn: %s%s, OpenDrain: %d\n", io_config.ie, oe_str, ((io_config.fun_sel == PIN_FUNC_GPIO) && (io_config.oe_inv)) ? " (inversed)" : "", io_config.od);
        fprintf(out_stream, "  FuncSel: %" PRIu32 " (%s)\n", io_config.fun_sel, (io_config.fun_sel == PIN_FUNC_GPIO) ? "GPIO" : "IOMUX");
        if (io_config.fun_sel == PIN_FUNC_GPIO) {
            fprintf(out_stream, "  GPIO Matrix SigOut ID: %" PRIu32 "%s\n", io_config.sig_out, (io_config.sig_out == SIG_GPIO_OUT_IDX) ? " (simple GPIO output)" : "");
        }
        if (io_config.ie && io_config.fun_sel == PIN_FUNC_GPIO) {
            uint32_t cnt = 0;
            fprintf(out_stream, "  GPIO Matrix SigIn ID:");
            for (int i = 0; i < SIG_GPIO_OUT_IDX; i++) {
                if (gpio_hal_get_in_signal_connected_io(gpio_context.gpio_hal, i) == gpio_num) {
                    cnt++;
                    fprintf(out_stream, " %d", i);
                }
            }
            if (cnt == 0) {
                fprintf(out_stream, " (simple GPIO input)");
            }
            fprintf(out_stream, "\n");
        }
        fprintf(out_stream, "  SleepSelEn: %d\n", io_config.slp_sel);
        fprintf(out_stream, "\n");
    }
    fprintf(out_stream, "=================IO DUMP End=================\n");
    return ESP_OK;
}
