#include <zephyr.h>

#include <logging/log.h>

#include "fota_report.h"

LOG_MODULE_REGISTER(FOTA_TLV, LOG_LEVEL_DBG);

#define FIRMWARE_VER_ID 1
#define MODEL_NUMBER_ID 2
#define SERIAL_NUMBER_ID 3
#define CLIENT_MANUFACTURER_ID 4

#define HOST_ID 1
#define PORT_ID 2
#define PATH_ID 3
#define AVAILABLE_ID 4

static size_t encode_tlv_string(uint8_t *buf, uint8_t id, const uint8_t *str) {
  size_t ret = 0;
  buf[ret++] = id;
  buf[ret++] = strlen(str);
  for (uint8_t i = 0; i < strlen(str); i++) {
    buf[ret++] = str[i];
  }
  return ret;
}

static inline uint8_t tlv_id(const uint8_t *buf, size_t idx) {
  return buf[idx];
}

static int decode_tlv_string(const uint8_t *buf, size_t *idx, char *str) {
  int len = (int)buf[(*idx)++];
  int i = 0;
  for (i = 0; i < len; i++) {
    str[i] = buf[(*idx)++];
  }
  str[i] = 0;
  return 0;
}

static int decode_tlv_uint32(const uint8_t *buf, size_t *idx, uint32_t *val) {
  size_t len = (size_t)buf[(*idx)++];
  if (len != 4) {
    LOG_ERR("uint32 in TLV buffer isn't 4 bytes it is %d bytes", len);
    return -1;
  }
  *val = 0;
  *val += (buf[(*idx)++] << 24);
  *val += (buf[(*idx)++] << 16);
  *val += (buf[(*idx)++] << 8);
  *val += (buf[(*idx)++]);
  return 0;
}

static int decode_tlv_bool(const uint8_t *buf, size_t *idx, bool *val) {
  size_t len = (size_t)buf[(*idx)++];
  if (len != 1) {
    LOG_ERR("bool in TLV buffer isn't 1 byte (%d bytes)", len);
    return -1;
  }

  *val = (buf[(*idx)++] == 1);
  return 0;
}

size_t encode_fota_report(fota_report_t *report, uint8_t *buffer) {
  size_t sz = 0;
  // The TLV buffer is quite simple - it's just the series of fields.
  char *p = buffer;
  sz += encode_tlv_string(p, FIRMWARE_VER_ID, report->version);
  p += sz;
  sz = encode_tlv_string(p, MODEL_NUMBER_ID, report->model);
  p += sz;
  sz = encode_tlv_string(p, SERIAL_NUMBER_ID, report->serial);
  p += sz;
  sz = encode_tlv_string(p, CLIENT_MANUFACTURER_ID, report->manufacturer);
  return (size_t)(p - (char *)buffer);
}

int decode_fota_response(fota_response_t *resp, const uint8_t *buf,
                         size_t len) {
  size_t idx = 0;
  int err = 0;
  while (idx < len) {
    uint8_t id = buf[idx++];
    switch (id) {
    case HOST_ID:
      err = decode_tlv_string(buf, &idx, resp->host);
      if (err) {
        return err;
      }
      break;
    case PORT_ID:
      err = decode_tlv_uint32(buf, &idx, &resp->port);
      if (err) {
        return err;
      }
      break;
    case PATH_ID:
      err = decode_tlv_string(buf, &idx, resp->path);
      if (err) {
        return err;
      }
      break;
    case AVAILABLE_ID:
      err = decode_tlv_bool(buf, &idx, &resp->update);
      if (err) {
        return err;
      }
      break;
    default:
      LOG_ERR("Unknown field id in FOTA response: %d", id);
      return -1;
    }
  }
  return 0;
}
