#include <Arduino.h>

// Comment to ensure the Arduino.h header stays at the top.

#include "support.h"

#include "Application.h"

LOG_TAG(main);

extern "C" void app_main() {
    initArduino();

    // If we've restarted because of a brownout or watchdog reset,
    // perform a silent startup.
    const auto resetReason = esp_reset_reason();
    const auto silent = resetReason == ESP_RST_BROWNOUT || resetReason == ESP_RST_WDT;

#ifdef CONFIG_DEVICE_SHOW_CPU_USAGE
    show_task_statistics();
#endif

    Application application;

    application.begin(silent);

    while (1) {
        application.process();

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
