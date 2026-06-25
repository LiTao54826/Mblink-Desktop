#pragma once

#include "core/api/mblink.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
  #ifdef MBLINK_DEVTOOLS_BUILDING_DLL
    #define MBLINK_DEVTOOLS_API __declspec(dllexport)
  #else
    #define MBLINK_DEVTOOLS_API __declspec(dllimport)
  #endif
#else
  #define MBLINK_DEVTOOLS_API __attribute__((visibility("default")))
#endif

typedef struct {
    const char* bind_host;
    unsigned short port;
    const char* auth_token;
    bool require_auth;
} MBlinkDevToolsHttpOptions;

typedef struct {
    unsigned short port;
    char* url;
    char* auth_token;
} MBlinkDevToolsHttpInfo;

typedef struct {
    const char* runtime_epoch;
    size_t max_nodes;
    int max_depth;
    const char* root_selector;
    bool include_screenshot;
    bool inline_screenshot;
    const char* screenshot_file;
} MBlinkUiDevSnapshotOptions;

MBLINK_DEVTOOLS_API int mblink_devtools_open(MBlinkHandle handle);
MBLINK_DEVTOOLS_API int mblink_devtools_close(MBlinkHandle handle);
MBLINK_DEVTOOLS_API MBlinkDevToolsHttpOptions mblink_devtools_default_http_options(void);
MBLINK_DEVTOOLS_API int mblink_devtools_http_start(MBlinkHandle handle,
                                                 const MBlinkDevToolsHttpOptions* options,
                                                 MBlinkDevToolsHttpInfo* out_info);
MBLINK_DEVTOOLS_API int mblink_devtools_http_stop(MBlinkHandle handle);
MBLINK_DEVTOOLS_API void mblink_devtools_http_info_free(MBlinkDevToolsHttpInfo* info);

MBLINK_DEVTOOLS_API MBlinkUiDevSnapshotOptions mblink_ui_dev_default_snapshot_options(void);
MBLINK_DEVTOOLS_API int mblink_ui_dev_snapshot_json(MBlinkHandle handle,
                                                  const MBlinkUiDevSnapshotOptions* options,
                                                  char** out_json);
MBLINK_DEVTOOLS_API int mblink_ui_dev_snapshot_file(MBlinkHandle handle,
                                                  const char* output_path,
                                                  const MBlinkUiDevSnapshotOptions* options);
MBLINK_DEVTOOLS_API int mblink_ui_dev_command_json(MBlinkHandle handle,
                                                 const char* command_json,
                                                 char** out_response_json);

#ifdef __cplusplus
}
#endif
