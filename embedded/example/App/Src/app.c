#include <math.h>

#include "app.h"
#include "app_debug.h"

#include "frame.h"
#include "debugger.h"
#include "ntc.h"

#include "platform_adc.h"
#include "platform.h"

static const char tag[] = "APP";

static int8_t counter = -100;
static uint16_t adc_temperature = 0;
static uint32_t counter_delay = 0;

void app_init(void) {
    platform_adc_init();

#if DEBUGGER_ENABLED
    app_debug_init();
#endif /* DEBUGGER_ENABLED */
}

void app_update(void) {
#if DEBUGGER_ENABLED
    app_debug_poll();
#endif /* DEBUGGER_ENABLED */

    if (platform_adc_get(&adc_temperature)) {
        const float temperature = ntc_calculate_temperature(adc_temperature);
        if (!isnan(temperature)) {
            debugger_plot_signal(SIGNAL_TEMPERATURE, &temperature);
        }
    }
    if (counter_delay == 1000) {
        counter++;
        debugger_plot_signal(SIGNAL_COUNTER8, &counter);
        debugger_log_info(tag, "Example info message");
        counter_delay = 0;
    } else {
        counter_delay++;
    }

    platform_delay(1);
}
