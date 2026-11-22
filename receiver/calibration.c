/*
	This will be used to handle the BOOT button long press, average the captured flex and
	IMU readings, check each finger's open and closed readings, and load/save the result
	to flash.

	Input: "glove_packet": packet [defined in glove_packet.h], "boot_state": bool

	Output: "calibration": per-finger open/closed readings,
	"enabled_mask", flat-hand baseline, to state.c and storage

	Interfaces with: state.c, calibration.h, receiver_config.h, light_map.c
*/
#include <stdio.h>
#include <string.h>
#include "driver/gpio.h"
#include "nvs.h"
#include "calibration.h"
#include "receiver_config.h"

#define CAL_NVS_NAMESPACE "glovecal"
#define CAL_NVS_VER_KEY "ver"
#define CAL_NVS_DATA_KEY "data"
#define CAL_VERSION 1

typedef struct {
  float pitch_sum;
  float roll_sum;
  float flex_sum[GLOVE_FINGER_COUNT];
  uint32_t count;
} Capture;

static CalData cal;
static bool valid = false;
static Capture capture;
static CalData pending_flat;   // flat-hand results, kept until the fist step
static bool flat_ready = false;

#if RECEIVER_TEST_MODE
static void load_test_values(void) {
  for (int i = 0; i < GLOVE_FINGER_COUNT; i++) {
    cal.flex_open[i] = TEST_CAL_FLEX_OPEN;
    cal.flex_closed[i] = TEST_CAL_FLEX_CLOSED;
  }
  cal.pitch_zero = TEST_CAL_PITCH_ZERO;
  cal.roll_zero = TEST_CAL_ROLL_ZERO;
  cal.enabled_mask = TEST_CAL_ENABLED_MASK;
  valid = true;
}
#else
// Valid only if the version key matches and the stored blob is exactly sizeof(CalData).
static void load_from_nvs(void) {
  nvs_handle_t handle;
  if (nvs_open(CAL_NVS_NAMESPACE, NVS_READONLY, &handle) != ESP_OK) return;

  uint8_t ver = 0;
  CalData loaded;
  size_t len = sizeof(CalData);
  bool ok = false;
  if (nvs_get_u8(handle, CAL_NVS_VER_KEY, &ver) == ESP_OK && ver == CAL_VERSION) {
    if (nvs_get_blob(handle, CAL_NVS_DATA_KEY, &loaded, &len) == ESP_OK && len == sizeof(CalData)) {
      ok = true;
    }
  }
  nvs_close(handle);

  if (ok) {
    cal = loaded;
    valid = true;
  }
}

// Writes the data blob, then the version key, and commits. Test mode never calls this.
static bool save_to_nvs(const CalData *c) {
  nvs_handle_t handle;
  if (nvs_open(CAL_NVS_NAMESPACE, NVS_READWRITE, &handle) != ESP_OK) return false;

  bool ok = nvs_set_blob(handle, CAL_NVS_DATA_KEY, c, sizeof(CalData)) == ESP_OK;
  if (ok) ok = nvs_set_u8(handle, CAL_NVS_VER_KEY, CAL_VERSION) == ESP_OK;
  if (ok) ok = nvs_commit(handle) == ESP_OK;
  nvs_close(handle);
  return ok;
}
#endif

void calibration_begin(void) {
  valid = false;
#if RECEIVER_TEST_MODE
  load_test_values();
#else
  gpio_set_direction((gpio_num_t)CAL_BOOT_GPIO, GPIO_MODE_INPUT);
  load_from_nvs();
#endif
  if (valid) {
    printf("LOG CAL loaded enabled=0x%02X\n", (unsigned)cal.enabled_mask);
  } else {
    printf("LOG CAL none\n");
  }
}

bool calibration_valid(void) {
  return valid;
}

const CalData *calibration_get(void) {
  if (!valid) return NULL;
  return &cal;
}

bool cal_button_held(uint32_t now_ms) {
#if RECEIVER_TEST_MODE
  (void)now_ms;
  return false;
#else
  static bool pressed = false;
  static bool fired = false;
  static uint32_t press_start_ms = 0;

  bool down = gpio_get_level((gpio_num_t)CAL_BOOT_GPIO) == 0;
  if (!down) {
    pressed = false;
    fired = false;
    return false;
  }
  if (!pressed) {
    pressed = true;
    press_start_ms = now_ms;
  }
  if (!fired && now_ms - press_start_ms >= CAL_HOLD_MS) {
    fired = true;
    return true;
  }
  return false;
#endif
}

void cal_capture_clear(void) {
  memset(&capture, 0, sizeof(capture));
}

void cal_capture_add(const GlovePacket *p) {
  if (p == NULL) return;
  capture.pitch_sum += p->pitch_deg;
  capture.roll_sum += p->roll_deg;
  for (int i = 0; i < GLOVE_FINGER_COUNT; i++) {
    capture.flex_sum[i] += (float)p->flex_raw[i];
  }
  capture.count++;
}

bool cal_finish_flat(void) {
  if (capture.count == 0) {
    flat_ready = false;
    cal_capture_clear();
    return false;
  }
  float n = (float)capture.count;
  pending_flat.pitch_zero = capture.pitch_sum / n;
  pending_flat.roll_zero = capture.roll_sum / n;
  for (int i = 0; i < GLOVE_FINGER_COUNT; i++) {
    pending_flat.flex_open[i] = (uint16_t)(capture.flex_sum[i] / n + 0.5f);
  }
  flat_ready = true;
  cal_capture_clear();
  return true;
}

CalResult cal_finish_fist(void) {
  if (!flat_ready || capture.count == 0) {
    flat_ready = false;
    cal_capture_clear();
    return CAL_NO_SAMPLES;
  }

  float n = (float)capture.count;
  CalData candidate = pending_flat;
  uint8_t mask = 0;
  for (int i = 0; i < GLOVE_FINGER_COUNT; i++) {
    candidate.flex_closed[i] = (uint16_t)(capture.flex_sum[i] / n + 0.5f);
    int open = (int)candidate.flex_open[i];
    int closed = (int)candidate.flex_closed[i];
    int span = closed - open;
    if (span < 0) span = -span;
    if (span < CAL_MIN_SPAN) {
      printf("LOG CAL finger=%d disabled\n", i);
    } else {
      mask |= (uint8_t)(1u << i);
    }
  }
  candidate.enabled_mask = mask;
  flat_ready = false;
  cal_capture_clear();

  if (mask == 0) return CAL_NO_FINGER;

#if !RECEIVER_TEST_MODE
  if (!save_to_nvs(&candidate)) return CAL_SAVE_FAILED;
#endif

  cal = candidate;
  valid = true;
  printf("LOG CAL saved enabled=0x%02X\n", (unsigned)mask);
  return CAL_OK;
}
