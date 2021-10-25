#pragma once

#include <stdio.h>
#include <stdint.h>
#include <string.h>


typedef struct {
    char *version;
    char *model;
    char *serial;
    char *manufacturer;
} fota_report_t;

typedef struct
{
	char host[25];
	uint32_t port;
	char path[25];
	bool update;
} fota_response_t;

/**
 * @brief encode report to Span into a byte buffer
 * @param report the report to encode
 * @param buffer buffer to encode to
 * @return length of encoded buffer
 */
size_t encode_fota_report(fota_report_t *report, uint8_t *buffer);

/**
 * @brief decode response from Span into response structure
 * @param resp response output
 * @param buffer buffer to decode
 * @param len length of buffer
 */
int decode_fota_response(fota_response_t *resp, const uint8_t *buffer, const size_t len);
