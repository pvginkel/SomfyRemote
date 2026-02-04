#pragma once

#define _USE_MATH_DEFINES

using namespace std;

#include <ctype.h>
#include <math.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/unistd.h>

#include <string>

#include "cJSON.h"
#include "error.h"
#include "esp_err.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "nvs_flash.h"
#include "sdkconfig.h"
#include "strformat.h"

#define LOG_TAG(v) [[maybe_unused]] static const char* TAG = #v
