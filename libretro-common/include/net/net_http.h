/* Copyright  (C) 2010-2020 The RetroArch team
 *
 * ---------------------------------------------------------------------------------------
 * The following license statement only applies to this file (net_http.h).
 * ---------------------------------------------------------------------------------------
 *
 * Permission is hereby granted, free of charge,
 * to any person obtaining a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _LIBRETRO_SDK_NET_HTTP_H
#define _LIBRETRO_SDK_NET_HTTP_H

#include <stdint.h>
#include <boolean.h>
#include <string.h>

#include <retro_common_api.h>
#include <streams/file_stream.h>

RETRO_BEGIN_DECLS

struct http_request_t;
struct http_connection_t;
struct http_t;
struct http_url_parameters_t;
struct http_headers_t;
struct http_response_t;

/* HTTP Request */
struct http_request_t *net_http_request_new();

void net_http_request_set_url(struct http_request_t *request, const char *url);

void net_http_request_set_method(struct http_request_t *request, const char *method);

void net_http_request_set_url_param(struct http_request_t *request, const char *name, const char *value, bool replace);

void net_http_request_set_header(struct http_request_t *request, const char *name, const char *value, bool replace);

void net_http_request_set_body_raw(struct http_request_t *request, uint8_t *data, const size_t len);

void net_http_request_set_body_file(struct http_request_t *request, RFILE *file, int64_t max_bytes);

void net_http_request_set_response_file(struct http_request_t *request, const char *filename);

void net_http_request_free(struct http_request_t *request);

/* HTTP Connection */
struct http_connection_t *net_http_connection_new(struct http_request_t *request);

void net_http_connection_set_logging(struct http_connection_t *conn,
      bool log_request_body, bool log_response_body);

bool net_http_connection_iterate(struct http_connection_t *conn);

bool net_http_connection_done(struct http_connection_t *conn);

void net_http_connection_free(struct http_connection_t *conn, bool free_request);

char *net_http_headers_get_first_value(struct http_headers_t *headers, const char *name);

char **net_http_headers_get_values(struct http_headers_t *headers, const char *name, size_t *values_len);

const char *net_http_connection_url(struct http_connection_t *conn);

/* HTTP State */
struct http_t *net_http_new(struct http_connection_t *conn);

/* You can use this to call net_http_update
 * only when something will happen; select() it for reading. */
int net_http_fd(struct http_t *state);

/* Returns true if it's done, or if something broke.
 * 'total' will be 0 if it's not known. */
bool net_http_update(struct http_t *state, size_t* progress, size_t* total);

struct http_response_t *net_http_get_response(struct http_t *state);

/* Cleans up all memory. */
void net_http_delete(struct http_t *state);

/* HTTP Response */
/* 200, 404, or whatever.  */
int net_http_response_get_status(struct http_response_t *response);

bool net_http_response_is_error(struct http_response_t *response);

char *net_http_response_get_header_first_value(struct http_response_t *response, const char *name);

char **net_http_response_get_header_values(struct http_response_t *response, const char *name, size_t *count);

/* Returns the downloaded data. The returned buffer is owned by the
 * HTTP handler; it's freed by net_http_delete.
 *
 * If the status is not 20x and accept_error is false, it returns NULL. */
uint8_t* net_http_response_get_data(struct http_response_t *response, size_t* len, bool accept_error);

void net_http_response_release_data(struct http_response_t *response);

void net_http_response_free(struct http_response_t *response);

/* URL Encoding */
void net_http_urlencode(char **dest, const char *source);

/* Re-encode a full URL */
void net_http_urlencode_full(char *dest, const char *source, size_t size);

RETRO_END_DECLS

#endif
