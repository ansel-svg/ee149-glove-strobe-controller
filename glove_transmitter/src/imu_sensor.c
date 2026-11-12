#include "imu_sensor.h"
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

static Adafruit_MPU6050 mpu;

void init_imu(void) {
  // init I2C and MPU6050
  if (!mpu.begin()) {
    return;
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
}

float get_pitch(void) {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  float accelX = a.acceleration.x;
  float accelY = a.acceleration.y;
  float accelZ = a.acceleration.z;
  return atan2(-accelX, sqrt(accelY * accelY + accelZ * accelZ)) * 180 / PI;
}

float get_roll(void) {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  float accelY = a.acceleration.y;
  float accelZ = a.acceleration.z;
  return atan2(accelY, accelZ) * 180 / PI;
}