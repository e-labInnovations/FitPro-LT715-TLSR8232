/*
 * BLE Advertising example for FitPro LT715 / TLSR8232
 * Ported from m6-reveng/example-programs/ble-adv
 */

#include "drivers/5316/bsp.h"
#include "drivers/5316/clock.h"
#include "drivers/5316/compiler.h"
#include "drivers/5316/driver_5316.h"
#include "drivers/5316/gpio.h"
#include "drivers/5316/register.h"
#include "drivers/5316/timer.h"
#include "drivers/5316/uart.h"
#include "stack/ble/ble_common.h"
#include "stack/ble/ble_smp.h"
#include "stack/ble/ll/ll.h"
#include "vendor/common/blt_common.h"

// BLE link-layer packet buffers.
// Without these the LL has no memory to queue packets and advertising silently fails.
MYFIFO_INIT(blt_rxfifo, 64, 8);
MYFIFO_INIT(blt_txfifo, 40, 16);

// FitPro LT715 backlight pin (used as status indicator)
#define PIN_LED GPIO_PB4

#define MY_APP_ADV_CHANNEL BLT_ENABLE_ADV_ALL

#define MY_ADV_INTERVAL_MIN ADV_INTERVAL_30MS
#define MY_ADV_INTERVAL_MAX ADV_INTERVAL_35MS

#define BLE_DEVICE_ADDRESS_TYPE BLE_DEVICE_ADDRESS_PUBLIC

// BLE advertisement data:
//   0x0e, 0x09 = Complete Local Name (14 bytes) "FitPro-LT715"
//   0x03, 0x02, 0x0f, 0x18 = 16-bit UUID list: 0x180f (Battery Service)
const u8 tbl_advData[] = {
    0x0d,
    0x09,
    'F', 'i', 't', 'P', 'r', 'o', '-', 'L', 'T', '7', '1', '5',
    // 0x180f UUID service (battery monitoring).
    0x03,
    0x02,
    0x0f,
    0x18,
};

_attribute_ram_code_ void irq_handler(void) { irq_blt_sdk_handler(); }

int device_in_connection_state;

void ble_remote_terminate(u8 e, u8 *p, int n) { gpio_write(PIN_LED, 0); }

void task_connect(u8 e, u8 *p, int n) {
    gpio_write(PIN_LED, 0);
    bls_l2cap_requestConnParamUpdate(8, 8, 99, 400);
}

void init_led(void) {
    gpio_set_func(PIN_LED, AS_GPIO);
    gpio_set_output_en(PIN_LED, 1);
    gpio_set_input_en(PIN_LED, 0);
    gpio_write(PIN_LED, 1);
}

int main() {
    blc_pm_select_internal_32k_crystal();
    cpu_wakeup_init();
    clock_init(SYS_CLK_16M_Crystal);
    gpio_init();
    init_led();
    blc_app_loadCustomizedParameters();

    rf_drv_init(RF_MODE_BLE_1M);

    u8 mac_public[6];
    u8 mac_random_static[6];
    blc_initMacAddress(CFG_ADR_MAC, mac_public, mac_random_static);

    blc_ll_initBasicMCU(mac_public);
    sleep_ms(2000);
    blc_ll_initAdvertising_module(mac_public);
    blc_ll_initSlaveRole_module();
    blc_ll_initPowerManagement_module();

    extern void my_att_init(void);

    // GATT initialization
    my_att_init();
    // L2CAP initialization
    blc_l2cap_register_handler(blc_l2cap_packet_receive);

    bls_smp_enableParing(0);

    bls_ll_setAdvData((u8 *)tbl_advData, sizeof(tbl_advData));

    u8 status =
        bls_ll_setAdvParam(MY_ADV_INTERVAL_MIN, MY_ADV_INTERVAL_MAX,
                           ADV_TYPE_CONNECTABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC,
                           0, NULL, MY_APP_ADV_CHANNEL, ADV_FP_NONE);
    if (status != BLE_SUCCESS) {
        gpio_write(PIN_LED, 0);
        while (1);
    }

    // Starting advertising.
    bls_ll_setAdvEnable(1);

    // Set advertisement power in dBm.
    rf_set_power_level_index(RF_POWER_7P9dBm);

    bls_app_registerEventCallback(BLT_EV_FLAG_CONNECT, &task_connect);
    bls_app_registerEventCallback(BLT_EV_FLAG_TERMINATE, &ble_remote_terminate);

    bls_pm_setSuspendMask(SUSPEND_DISABLE);

    irq_enable();

    while (1) {
        blt_sdk_main_loop();
    }

    return 0;
}
