#pragma once

#include <cbor.h>

typedef enum {
  VTX_BAND_A,
  VTX_BAND_B,
  VTX_BAND_E,
  VTX_BAND_F,
  VTX_BAND_R,

  VTX_BAND_MAX
} vtx_band_t;

typedef enum {
  VTX_CHANNEL_1,
  VTX_CHANNEL_2,
  VTX_CHANNEL_3,
  VTX_CHANNEL_4,
  VTX_CHANNEL_5,
  VTX_CHANNEL_6,
  VTX_CHANNEL_7,
  VTX_CHANNEL_8,

  VTX_CHANNEL_MAX,
} vtx_channel_t;

typedef enum {
  VTX_POWER_LEVEL_1,
  VTX_POWER_LEVEL_2,
  VTX_POWER_LEVEL_3,
  VTX_POWER_LEVEL_4,

  VTX_POWER_LEVEL_MAX,
} vtx_power_level_t;

typedef enum {
  VTX_PIT_MODE_NO_SUPPORT,
  VTX_PIT_MODE_OFF,
  VTX_PIT_MODE_ON,

  VTX_PIT_MODE_MAX,
} vtx_pit_mode_t;

typedef struct {
  uint8_t detected;

  vtx_band_t band;
  vtx_channel_t channel;

  vtx_pit_mode_t pit_mode;
  vtx_power_level_t power_level;
} vtx_settings_t;

void vtx_init();
void vtx_update();

void vtx_set(vtx_settings_t *vtx);
uint8_t vtx_set_frequency(vtx_band_t band, vtx_channel_t channel);
uint8_t vtx_set_pit_mode(vtx_pit_mode_t pit_mode);
uint8_t vtx_set_power_level(vtx_power_level_t power_level);

cbor_result_t cbor_encode_vtx_settings_t(cbor_value_t *enc, const vtx_settings_t *vtx);
cbor_result_t cbor_decode_vtx_settings_t(cbor_value_t *dec, vtx_settings_t *vtx);