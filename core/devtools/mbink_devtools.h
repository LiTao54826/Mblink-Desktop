#pragma once

#include "core/api/mbink.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
  #ifdef MBINK_DEVTOOLS_BUILDING_DLL
    #define MBINK_DEVTOOLS_API __declspec(dllexport)
  #else
    #define MBINK_DEVTOOLS_API __declspec(dllimport)
  #endif
#else
  #define MBINK_DEVTOOLS_API __attribute__((visibility("default")))
#endif

typedef struct {
    const char* bind_host;
    unsigned short port;
    const char* auth_token;
    bool require_auth;
} MBinkDevToolsHttpOptions;

typedef struct {
    unsigned short port;
    char* url;
    char* auth_token;
} MBinkDevToolsHttpInfo;

typedef struct {
    const char* runtime_epoch;
    size_t max_nodes;
    int max_depth;
    const char* root_selector;
    bool include_screenshot;
    bool inline_screenshot;
    const char* screenshot_file;
} MBinkUiDevSnapshotOptions;

MBINK_DEVTOOLS_API int mbink_devtools_open(MBinkHandle handle);
MBINK_DEVTOOLS_API int mbink_devtools_close(MBinkHandle handle);
MBINK_DEVTOOLS_API MBinkDevToolsHttpOptions mbink_devtools_default_http_options(void);
MBINK_DEVTOOLS_API int mbink_devtools_http_start(MBinkHandle handle,
                                                 const MBinkDevToolsHttpOptions* options,
                                                 MBinkDevToolsHttpInfo* out_info);
MBINK_DEVTOOLS_API int mbink_devtools_http_stop(MBinkHandle handle);
MBINK_DEVTOOLS_API void mbink_devtools_http_info_free(MBinkDevToolsHttpInfo* info);

MBINK_DEVTOOLS_API MBinkUiDevSnapshotOptions mbink_ui_dev_default_snapshot_options(void);
MBINK_DEVTOOLS_API int mbink_ui_dev_snapshot_json(MBinkHandle handle,
                                                  const MBinkUiDevSnapshotOptions* options,
                                                  char** out_json);
MBINK_DEVTOOLS_API int mbink_ui_dev_snapshot_file(MBinkHandle handle,
                                                  const char* output_path,
                                                  const MBinkUiDevSnapshotOptions* options);
MBINK_DEVTOOLS_API int mbink_ui_dev_command_json(MBinkHandle handle,
                                                 const char* command_json,
                                                 char** out_response_json);

#ifdef __cplusplus
}
#endif
