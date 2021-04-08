/* Copyright  (C) 2020 The RetroArch team
 *
 * ---------------------------------------------------------------------------------------
 * The following license statement only applies to this file (cloud_storage.h).
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

#ifndef _CLOUD_STORAGE_H
#define _CLOUD_STORAGE_H

#include <retro_common_api.h>
#include <configuration.h>

#include <boolean.h>
#include <time.h>

RETRO_BEGIN_DECLS

#define GAME_SAVES_FOLDER_NAME "save_games"
#define GAME_STATES_FOLDER_NAME "save_states"
#define RUNTIME_LOGS_FOLDER_NAME "runtime_logs"
#define SCREENSHOTS_FOLDER_NAME "screenshots"

enum cloud_storage_hash_type_t
{
   SHA1,
   SHA256,
   MD5
};
typedef enum cloud_storage_hash_type_t cloud_storage_hash_type_t;

struct cloud_storage_file_t
{
   cloud_storage_hash_type_t hash_type;
   char *hash_value;
   char *download_url;
};
typedef struct cloud_storage_file_t cloud_storage_file_t;

typedef struct cloud_storage_item_t cloud_storage_item_t;

struct cloud_storage_folder_t
{
   cloud_storage_item_t *children;
};
typedef struct cloud_storage_folder_t cloud_storage_folder_t;

enum cloud_storage_item_type_t
{
   CLOUD_STORAGE_FILE,
   CLOUD_STORAGE_FOLDER
};
typedef enum cloud_storage_item_type_t cloud_storage_item_type_t;
struct cloud_storage_item_t
{
   char *id;
   char *name;
   time_t last_sync_time;
   cloud_storage_item_type_t item_type;
   union
   {
      cloud_storage_file_t file;
      cloud_storage_folder_t folder;
   } type_data;
   cloud_storage_item_t *next;
};

enum folder_type_t
{
   CLOUD_STORAGE_GAME_STATES,
   CLOUD_STORAGE_GAME_SAVES,
   CLOUD_STORAGE_RUNTIME_LOGS,
   CLOUD_STORAGE_SCREENSHOTS
};
typedef enum folder_type_t folder_type_t;

enum authorization_status_t
{
   AUTH_PENDING_ASYNC,
   AUTH_COMPLETE,
   AUTH_FAILED
};
typedef enum authorization_status_t authorization_status_t;

struct cloud_storage_provider_t
{
   bool need_authorization;

   bool (*have_default_credentials)(void);

   bool (*ready_for_request)(void);

   bool (*authenticate)(void);

   authorization_status_t (*authorize)(void (*callback)(bool success));

   void (*list_files)(cloud_storage_item_t *folder);

   bool (*download_file)(
      cloud_storage_item_t *file_to_download,
      char *local_file
   );

   bool (*upload_file)(
      cloud_storage_item_t *remote_dir,
      cloud_storage_item_t *remote_file,
      char *local_file
   );

   cloud_storage_item_t *(*get_folder_metadata)(
      const char *folder_name
   );

   cloud_storage_item_t *(*get_file_metadata)(
      cloud_storage_item_t *file
   );

   cloud_storage_item_t *(*get_file_metadata_by_name)(
      cloud_storage_item_t *folder,
      char *filename
   );

   bool (*delete_file)(
      cloud_storage_item_t *file
   );

   cloud_storage_item_t *(*create_folder)(
      const char *folder_name
   );
};
typedef struct cloud_storage_provider_t cloud_storage_provider_t;

void cloud_storage_init(void);

void cloud_storage_shutdown(void);

bool cloud_storage_need_authorization(void);

bool cloud_storage_have_default_credentials(void);

void cloud_storage_authorize(void (*callback)(bool success));

void cloud_storage_sync_files(void);

void cloud_storage_upload_file(folder_type_t folder_type, char *file_name);

void cloud_storage_set_active_provider(unsigned provider_id);

void cloud_storage_set_logfile_dir(const char *logfile_dir);

RETRO_END_DECLS

#endif
