//go:build windows

package mbink

/*
#include <stdlib.h>
#include <stdbool.h>
#include <windows.h>
#include "mbink.h"

typedef struct {
    const char* runtime_epoch;
    size_t max_nodes;
    int max_depth;
    const char* root_selector;
    bool include_screenshot;
    bool inline_screenshot;
    const char* screenshot_file;
} MBinkUiDevSnapshotOptions;

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

typedef int (__cdecl *mbink_devtools_open_fn)(MBinkHandle);
typedef int (__cdecl *mbink_devtools_close_fn)(MBinkHandle);
typedef MBinkDevToolsHttpOptions (__cdecl *mbink_devtools_default_http_options_fn)(void);
typedef int (__cdecl *mbink_devtools_http_start_fn)(MBinkHandle, const MBinkDevToolsHttpOptions*, MBinkDevToolsHttpInfo*);
typedef int (__cdecl *mbink_devtools_http_stop_fn)(MBinkHandle);
typedef void (__cdecl *mbink_devtools_http_info_free_fn)(MBinkDevToolsHttpInfo*);
typedef MBinkUiDevSnapshotOptions (__cdecl *mbink_ui_dev_default_snapshot_options_fn)(void);
typedef int (__cdecl *mbink_ui_dev_snapshot_json_fn)(MBinkHandle, const MBinkUiDevSnapshotOptions*, char**);
typedef int (__cdecl *mbink_ui_dev_snapshot_file_fn)(MBinkHandle, const char*, const MBinkUiDevSnapshotOptions*);
typedef int (__cdecl *mbink_ui_dev_command_json_fn)(MBinkHandle, const char*, char**);

static HMODULE go_mbink_devtools_module;
static mbink_devtools_open_fn go_mbink_devtools_open;
static mbink_devtools_close_fn go_mbink_devtools_close;
static mbink_devtools_default_http_options_fn go_mbink_devtools_default_http_options;
static mbink_devtools_http_start_fn go_mbink_devtools_http_start;
static mbink_devtools_http_stop_fn go_mbink_devtools_http_stop;
static mbink_devtools_http_info_free_fn go_mbink_devtools_http_info_free;
static mbink_ui_dev_default_snapshot_options_fn go_mbink_ui_dev_default_snapshot_options;
static mbink_ui_dev_snapshot_json_fn go_mbink_ui_dev_snapshot_json;
static mbink_ui_dev_snapshot_file_fn go_mbink_ui_dev_snapshot_file;
static mbink_ui_dev_command_json_fn go_mbink_ui_dev_command_json;

static int go_mbink_try_load_devtools_path(const wchar_t* path) {
    if (!path || !*path) {
        return 0;
    }
    if (!((path[0] >= L'A' && path[0] <= L'Z') ||
          (path[0] >= L'a' && path[0] <= L'z') ||
          (path[0] == L'\\' && path[1] == L'\\'))) {
        return 0;
    }
    if (((path[0] >= L'A' && path[0] <= L'Z') ||
         (path[0] >= L'a' && path[0] <= L'z')) &&
        (path[1] != L':' || path[2] != L'\\')) {
        return 0;
    }
    wchar_t full_path[32768];
    DWORD attrs = GetFileAttributesW(path);
    wchar_t candidate[32768];
    if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY)) {
        swprintf(candidate, 32768, L"%ls\\mbink_devtools.dll", path);
        path = candidate;
    }
    attrs = GetFileAttributesW(path);
    if (attrs == INVALID_FILE_ATTRIBUTES || (attrs & FILE_ATTRIBUTE_DIRECTORY)) {
        return 0;
    }
    DWORD full_len = GetFullPathNameW(path, 32768, full_path, NULL);
    if (full_len == 0 || full_len >= 32768) {
        return 0;
    }
    go_mbink_devtools_module = LoadLibraryExW(
        full_path,
        NULL,
        LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    return go_mbink_devtools_module != NULL;
}

static int go_mbink_load_devtools(void) {
    if (go_mbink_devtools_module) {
        return 0;
    }

    wchar_t buffer[32768];
    DWORD env_len = GetEnvironmentVariableW(L"MBINK_DEVTOOLS_PATH", buffer, 32768);
    if (env_len > 0 && env_len < 32768 && go_mbink_try_load_devtools_path(buffer)) {
        goto resolve;
    }

    HMODULE mbink_module = GetModuleHandleW(L"mbink.dll");
    if (mbink_module) {
        DWORD len = GetModuleFileNameW(mbink_module, buffer, MAX_PATH);
        if (len > 0 && len < MAX_PATH) {
            wchar_t* slash = wcsrchr(buffer, L'\\');
            if (slash) {
                *(slash + 1) = L'\0';
                wcscat_s(buffer, MAX_PATH, L"mbink_devtools.dll");
                if (go_mbink_try_load_devtools_path(buffer)) {
                    goto resolve;
                }
            }
        }
    }

    return -3;

resolve:
    go_mbink_devtools_open = (mbink_devtools_open_fn)GetProcAddress(go_mbink_devtools_module, "mbink_devtools_open");
    go_mbink_devtools_close = (mbink_devtools_close_fn)GetProcAddress(go_mbink_devtools_module, "mbink_devtools_close");
    go_mbink_devtools_default_http_options = (mbink_devtools_default_http_options_fn)GetProcAddress(go_mbink_devtools_module, "mbink_devtools_default_http_options");
    go_mbink_devtools_http_start = (mbink_devtools_http_start_fn)GetProcAddress(go_mbink_devtools_module, "mbink_devtools_http_start");
    go_mbink_devtools_http_stop = (mbink_devtools_http_stop_fn)GetProcAddress(go_mbink_devtools_module, "mbink_devtools_http_stop");
    go_mbink_devtools_http_info_free = (mbink_devtools_http_info_free_fn)GetProcAddress(go_mbink_devtools_module, "mbink_devtools_http_info_free");
    go_mbink_ui_dev_default_snapshot_options = (mbink_ui_dev_default_snapshot_options_fn)GetProcAddress(go_mbink_devtools_module, "mbink_ui_dev_default_snapshot_options");
    go_mbink_ui_dev_snapshot_json = (mbink_ui_dev_snapshot_json_fn)GetProcAddress(go_mbink_devtools_module, "mbink_ui_dev_snapshot_json");
    go_mbink_ui_dev_snapshot_file = (mbink_ui_dev_snapshot_file_fn)GetProcAddress(go_mbink_devtools_module, "mbink_ui_dev_snapshot_file");
    go_mbink_ui_dev_command_json = (mbink_ui_dev_command_json_fn)GetProcAddress(go_mbink_devtools_module, "mbink_ui_dev_command_json");
    if (!go_mbink_devtools_open ||
        !go_mbink_devtools_close ||
        !go_mbink_devtools_default_http_options ||
        !go_mbink_devtools_http_start ||
        !go_mbink_devtools_http_stop ||
        !go_mbink_devtools_http_info_free ||
        !go_mbink_ui_dev_default_snapshot_options ||
        !go_mbink_ui_dev_snapshot_json ||
        !go_mbink_ui_dev_snapshot_file ||
        !go_mbink_ui_dev_command_json) {
        return -3;
    }
    return 0;
}

static int go_mbink_devtools_open_call(MBinkHandle h) { return go_mbink_devtools_open(h); }
static int go_mbink_devtools_close_call(MBinkHandle h) { return go_mbink_devtools_close(h); }
static MBinkDevToolsHttpOptions go_mbink_devtools_default_http_options_call(void) { return go_mbink_devtools_default_http_options(); }
static int go_mbink_devtools_http_start_call(MBinkHandle h, const MBinkDevToolsHttpOptions* o, MBinkDevToolsHttpInfo* i) { return go_mbink_devtools_http_start(h, o, i); }
static int go_mbink_devtools_http_stop_call(MBinkHandle h) { return go_mbink_devtools_http_stop(h); }
static void go_mbink_devtools_http_info_free_call(MBinkDevToolsHttpInfo* i) { go_mbink_devtools_http_info_free(i); }
static MBinkUiDevSnapshotOptions go_mbink_ui_dev_default_snapshot_options_call(void) { return go_mbink_ui_dev_default_snapshot_options(); }
static int go_mbink_ui_dev_snapshot_json_call(MBinkHandle h, const MBinkUiDevSnapshotOptions* o, char** out) { return go_mbink_ui_dev_snapshot_json(h, o, out); }
static int go_mbink_ui_dev_snapshot_file_call(MBinkHandle h, const char* p, const MBinkUiDevSnapshotOptions* o) { return go_mbink_ui_dev_snapshot_file(h, p, o); }
static int go_mbink_ui_dev_command_json_call(MBinkHandle h, const char* c, char** out) { return go_mbink_ui_dev_command_json(h, c, out); }
*/
import "C"

func ensureDevtoolsRuntime() error {
	if rc := C.go_mbink_load_devtools(); rc != 0 {
		return newError(int(rc), "mbink_devtools.dll not found or missing expected exports")
	}
	return nil
}
