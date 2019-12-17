#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>

#include "defines.h"
#include "drv_time.h"
#include "sixaxis.h"
#include "util.h"
#include "iir_filter.h"

#define ACC_1G 1.0f
#define BFPV_IMU
// disable drift correction ( for testing)
#define DISABLE_ACC 0

// filter times in seconds
// time to correct gyro readings using the accelerometer
// 1-4 are generally good
#define FASTFILTER 0.05 //onground filter
//#define PREFILTER 0.2   //in_air prefilter (this can be commented out)
#define FILTERTIME 2.0 //in_air fusion filter

// accel magnitude limits for drift correction
#define ACC_MIN 0.7f
#define ACC_MAX 1.3f

#define _sinf(val) sinf(val)
#define _cosf(val) cosf(val)

float GEstG[3] = {0, 0, ACC_1G};
float attitude[3];

extern float gyro[3];
extern float accel[3];
extern float accelcal[3];

void imu_init(void) {
  // init the gravity vector with accel values
  for (int xx = 0; xx < 100; xx++) {
    sixaxis_read();

    for (int x = 0; x < 3; x++) {
      lpf(&GEstG[x], accel[x] * (1 / 2048.0f), 0.85);
    }
    delay(1000);
  }
}

float calcmagnitude(float vector[3]) {
  float accmag = 0;
  for (uint8_t axis = 0; axis < 3; axis++) {
    accmag += vector[axis] * vector[axis];
  }
  accmag = 1.0f / Q_rsqrt(accmag);
  return accmag;
}

void vectorcopy(float *vector1, float *vector2) {
  for (int axis = 0; axis < 3; axis++) {
    vector1[axis] = vector2[axis];
  }
}

extern float looptime;

void imu_calc(void) {
  // remove bias and reduce to accel in G
  accel[0] = (accel[0] - accelcal[0]) * (1 / 2048.0f);
  accel[1] = (accel[1] - accelcal[1]) * (1 / 2048.0f);
  accel[2] = (accel[2] - accelcal[2]) * (1 / 2048.0f);

#ifdef BFPV_IMU
  accel[0] = LPF2pApply_1(accel[0]);
  accel[1] = LPF2pApply_2(accel[1]);
  accel[2] = LPF2pApply_3(accel[2]);

  float EstG[3];

	vectorcopy(&EstG[0], &GEstG[0]);

  float gyros[3];
  for (int i = 0; i < 3; i++){
      gyros[i] = gyro[i] * looptime;
  }
  float mat[3][3];
	float cosx, sinx, cosy, siny, cosz, sinz;
	float coszcosx, coszcosy, sinzcosx, coszsinx, sinzsinx;

	cosx = _cosf(gyros[1]);
	sinx = _sinf(gyros[1]);
	cosy = _cosf(gyros[0]);
	siny = _sinf(-gyros[0]);
	cosz = _cosf(gyros[2]);
	sinz = _sinf(-gyros[2]);

	coszcosx = cosz * cosx;
	coszcosy = cosz * cosy;
	sinzcosx = sinz * cosx;
	coszsinx = sinx * cosz;
	sinzsinx = sinx * sinz;

	mat[0][0] = coszcosy;
	mat[0][1] = -cosy * sinz;
	mat[0][2] = siny;
	mat[1][0] = sinzcosx + (coszsinx * siny);
	mat[1][1] = coszcosx - (sinzsinx * siny);
	mat[1][2] = -sinx * cosy;
	mat[2][0] = (sinzsinx) - (coszcosx * siny);
	mat[2][1] = (coszsinx) + (sinzcosx * siny);
	mat[2][2] = cosy * cosx;

	EstG[0] = GEstG[0] * mat[0][0] + GEstG[1] * mat[1][0] + GEstG[2] * mat[2][0];
	EstG[1] = GEstG[0] * mat[0][1] + GEstG[1] * mat[1][1] + GEstG[2] * mat[2][1];
	EstG[2] = GEstG[0] * mat[0][2] + GEstG[1] * mat[1][2] + GEstG[2] * mat[2][2];


  vectorcopy(&GEstG[0], &EstG[0]);
#else
  const float gyro_delta_angle[3] = {
      gyro[0] * looptime,
      gyro[1] * looptime,
      gyro[2] * looptime,
  };

  GEstG[2] = GEstG[2] - (gyro_delta_angle[0]) * GEstG[0];
  GEstG[0] = (gyro_delta_angle[0]) * GEstG[2] + GEstG[0];

  GEstG[1] = GEstG[1] + (gyro_delta_angle[1]) * GEstG[2];
  GEstG[2] = -(gyro_delta_angle[1]) * GEstG[1] + GEstG[2];

  GEstG[0] = GEstG[0] - (gyro_delta_angle[2]) * GEstG[1];
  GEstG[1] = (gyro_delta_angle[2]) * GEstG[0] + GEstG[1];
#endif

  extern int onground;
  if (onground) { //happyhour bartender - quad is ON GROUND and disarmed
    // calc acc mag
    float accmag = calcmagnitude(&accel[0]);
    if ((accmag > ACC_MIN * ACC_1G) && (accmag < ACC_MAX * ACC_1G)) {
      // normalize acc
      for (int axis = 0; axis < 3; axis++) {
        accel[axis] = accel[axis] * (ACC_1G / accmag);
      }

      float filtcoeff = lpfcalc_hz(looptime, 1.0f / (float)FASTFILTER);
      for (int x = 0; x < 3; x++) {
        lpf(&GEstG[x], accel[x], filtcoeff);
      }
    }
  } else {
#ifdef BFPV_IMU
	  float accmag = calcmagnitude(&accel[0]);
      for (int axis = 0; axis < 3; axis++)
          {
              accel[axis] = accel[axis] * ( ACC_1G / accmag);
          }
          float filtcoeff = lpfcalc_hz( looptime, 1.0f/(float)FILTERTIME);
          for (int x = 0; x < 3; x++)
          {
             lpf(&GEstG[x], accel[x], filtcoeff);
          }
#else
    //lateshift bartender - quad is IN AIR and things are getting wild
    // hit accel[3] with a sledgehammer
#ifdef PREFILTER
    float filtcoeff = lpfcalc_hz(looptime, 1.0f / (float)PREFILTER);
    for (int x = 0; x < 3; x++) {
      lpf(&accel[x], accel[x], filtcoeff);
    }
#endif

    // calc mag of filtered acc
    float accmag = calcmagnitude(&accel[0]);
    if ((accmag > ACC_MIN * ACC_1G) && (accmag < ACC_MAX * ACC_1G)) {
      // normalize acc
      for (int axis = 0; axis < 3; axis++) {
        accel[axis] = accel[axis] * (ACC_1G / accmag);
      }
      // filter accel on to GEstG
      float filtcoeff = lpfcalc_hz(looptime, 1.0f / (float)FILTERTIME);
      for (int x = 0; x < 3; x++) {
        lpf(&GEstG[x], accel[x], filtcoeff);
      }
    }
#endif
    //heal the gravity vector after fusion with accel
    float GEstGmag = calcmagnitude(&GEstG[0]);
    for (int axis = 0; axis < 3; axis++) {
      GEstG[axis] = GEstG[axis] * (ACC_1G / GEstGmag);
    }
  }

  if (rx_aux_on(AUX_HORIZON)) {
    attitude[0] = atan2approx(GEstG[0], GEstG[2]);
    attitude[1] = atan2approx(GEstG[1], GEstG[2]);
  }
}