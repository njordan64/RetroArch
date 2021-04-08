/* Copyright  (C) 2020 The RetroArch team
 *
 * ---------------------------------------------------------------------------------------
 * The following license statement only applies to this file (list_files.c).
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

#include <stdlib.h>

#include <formats/rjson.h>
#include <net/net_http.h>
#include <rest/rest.h>

#include "../cloud_storage.h"
#include "../driver_utils.h"
#include "google_internal.h"

#define LIST_FILES_URL "https://www.googleapis.com/drive/v3/files"

static struct http_request_t *_create_http_request(cloud_storage_item_t *folder, char *next_page_token)
{
   struct http_request_t *request;
   char *parent_folder_id;
   char *q;

   request = net_http_request_new();
   net_http_request_set_url(request, LIST_FILES_URL);
   net_http_request_set_method(request, "GET");

   q = cloud_storage_join_strings(NULL, "\"", folder->id, "\" in parents", NULL);
   net_http_request_set_url_param(request, "q", q, true);
   free(q);

   net_http_request_set_url_param(request, "spaces", "appDataFolder", true);
   net_http_request_set_url_param(request, "fields", "nextPageToken,files(id,name,mimeType,md5Checksum)", true);

   if (next_page_token)
   {
      net_http_request_set_url_param(request, "nextPageToken", next_page_token, true);
      free(next_page_token);
   }

   return request;
}

static cloud_storage_item_t *_process_response(
   struct http_response_t *http_response,
   char **next_page_token)
{
   uint8_t *data;
   size_t data_len;
   rjson_t *json;
   bool in_object = false;
   bool in_files = false;
   const char *key_name;
   size_t key_name_len;
   const char *received_next_page_token = NULL;
   size_t received_next_page_token_len;
   cloud_storage_item_t *first_file = NULL;
   cloud_storage_item_t *current_file = NULL;
   cloud_storage_item_t *new_file;

   data = net_http_response_get_data(http_response, &data_len, false);
   json = rjson_open_buffer(data, data_len);

   for (;;)
   {
      switch (rjson_next(json))
      {
         case RJSON_ERROR:
            goto cleanup;
         case RJSON_OBJECT:
            if (in_object)
            {
               goto cleanup;
            } else
            {
               in_object = true;
            }
            break;
         case RJSON_OBJECT_END:
            goto success;
         case RJSON_STRING:
            if (!in_object)
            {
               goto cleanup;
            }

            if ((rjson_get_context_count(json) & 1) == 1)
            {
               key_name = rjson_get_string(json, &key_name_len);
            } else if (strcmp("nextPageToken", key_name) == 0)
            {
               received_next_page_token = rjson_get_string(json, &received_next_page_token_len);
            }

            break;
         case RJSON_ARRAY:
            if (in_object && strcmp("files", key_name) == 0)
            {
               bool done = false;
               while (!done)
               {
                  switch (rjson_next(json))
                  {
                     case RJSON_OBJECT:
                        new_file = cloud_storage_google_parse_file_from_json(json);
                        if (new_file)
                        {
                           if (!current_file)
                           {
                              first_file = new_file;
                              current_file = new_file;
                           } else
                           {
                              current_file->next = new_file;
                              current_file = new_file;
                           }
                        }

                        break;
                     case RJSON_ARRAY_END:
                        done = true;
                        break;
                     default:
                        break;
                  }
               }
            }

            break;
         default:
            break;
      }
   }

success:
   if (received_next_page_token)
   {
      *next_page_token = (char *)malloc(received_next_page_token_len + 1);
      strcpy(*next_page_token, received_next_page_token);
   }

cleanup:
   rjson_free(json);
   return first_file;
}

static cloud_storage_item_t *_get_list_files_next_page(
   cloud_storage_item_t *folder,
   char *next_page_token,
   char **new_value)
{
   struct http_request_t *http_request;
   rest_request_t *rest_request;
   struct http_response_t *http_response;
   cloud_storage_item_t *items = NULL;

   http_request = _create_http_request(folder, next_page_token);
   rest_request = rest_request_new(http_request);

   http_response = google_rest_execute_request(rest_request);
   if (!http_response)
   {
      *new_value = NULL;
      goto complete;
   }

   switch (net_http_response_get_status(http_response))
   {
      case 200:
         items = _process_response(http_response, &next_page_token);
         break;
      default:
         break;
   }

complete:
   if (http_response)
   {
      net_http_response_free(http_response);
   }
   rest_request_free(rest_request);

   return items;
}

void cloud_storage_google_list_files(cloud_storage_item_t *folder)
{
   cloud_storage_item_t *last_child = NULL;
   cloud_storage_item_t *new_items;
   char *next_page_token = NULL;
   char *next_value = NULL;

   if (!folder || folder->item_type != CLOUD_STORAGE_FOLDER)
   {
      return;
   }

   for (last_child = folder->type_data.folder.children;last_child != NULL && last_child->next != NULL;last_child = last_child->next);

   do
   {
      if (next_page_token)
      {
         free(next_page_token);
      }
      next_page_token = next_value;
      next_value = NULL;

      new_items = _get_list_files_next_page(folder, next_page_token, &next_value);
      if (new_items)
      {
         if (!last_child)
         {
            folder->type_data.folder.children = new_items;
         } else
         {
            last_child->next = new_items;
         }

         for (last_child = new_items;last_child != NULL && last_child->next != NULL;last_child = last_child->next);
      }
   } while (next_value);
}
