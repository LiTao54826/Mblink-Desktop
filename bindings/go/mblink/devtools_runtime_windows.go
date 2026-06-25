//go:build windows

package mblink

/*
#include <stdlib.h>
#include <stdbool.h>
#include <wchar.h>
#include <windows.h>
#include "mblink.h"

typedef struct {
    const char* runtime_epoch;
    size_t max_nodes;
    int max_depth;
    const char* root_selector;
    bool include_screenshot;
    bool inline_screenshot;
    const char* screenshot_file;
} MBlinkUiDevSnapshotOptions;

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

typedef int (__cdecl *mblink_devtools_open_fn)(MBlinkHandle);
typedef int (__cdecl *mblink_devtools_close_fn)(MBlinkHandle);
typedef MBlinkDevToolsHttpOptions (__cdecl *mblink_devtools_default_http_options_fn)(void);
typedef int (__cdecl *mblink_devtools_http_start_fn)(MBlinkHandle, const MBlinkDevToolsHttpOptions*, MBlinkDevToolsHttpInfo*);
typedef int (__cdecl *mblink_devtools_http_stop_fn)(MBlinkHandle);
typedef void (__cdecl *mblink_devtools_http_info_free_fn)(MBlinkDevToolsHttpInfo*);
typedef MBlinkUiDevSnapshotOptions (__cdecl *mblink_ui_dev_default_snapshot_options_fn)(void);
typedef int (__cdecl *mblink_ui_dev_snapshot_json_fn)(MBlinkHandle, const MBlinkUiDevSnapshotOptions*, char**);
typedef int (__cdecl *mblink_ui_dev_snapshot_file_fn)(MBlinkHandle, const char*, const MBlinkUiDevSnapshotOptions*);
typedef int (__cdecl *mblink_ui_dev_command_json_fn)(MBlinkHandle, const char*, char**);

static HMODULE go_mblink_devtools_module;
static mblink_devtools_open_fn go_mblink_devtools_open;
static mblink_devtools_close_fn go_mblink_devtools_close;
static mblink_devtools_default_http_options_fn go_mblink_devtools_default_http_options;
static mblink_devtools_http_start_fn go_mblink_devtools_http_start;
static mblink_devtools_http_stop_fn go_mblink_devtools_http_stop;
static mblink_devtools_http_info_free_fn go_mblink_devtools_http_info_free;
static mblink_ui_dev_default_snapshot_options_fn go_mblink_ui_dev_default_snapshot_options;
static mblink_ui_dev_snapshot_json_fn go_mblink_ui_dev_snapshot_json;
static mblink_ui_dev_snapshot_file_fn go_mblink_ui_dev_snapshot_file;
static mblink_ui_dev_command_json_fn go_mblink_ui_dev_command_json;

static int go_mblink_try_load_devtools_path(const wchar_t* path) {
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
        swprintf(candidate, 32768, L"%ls\\mblink_devtools.dll", path);
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
    go_mblink_devtools_module = LoadLibraryExW(
        full_path,
        NULL,
        LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    return go_mblink_devtools_module != NULL;
}

static int go_mblink_load_devtools(void) {
    if (go_mblink_devtools_module) {
        return 0;
    }

    wchar_t buffer[32768];
    DWORD env_len = GetEnvironmentVariableW(L"MBLINK_DEVTOOLS_PATH", buffer, 32768);
    if (env_len > 0 && env_len < 32768 && go_mblink_try_load_devtools_path(buffer)) {
        goto resolve;
    }

    HMODULE mblink_module = GetModuleHandleW(L"mblink.dll");
    if (mblink_module) {
        DWORD len = GetModuleFileNameW(mblink_module, buffer, 32768);
        if (len > 0 && len < 32768) {
            wchar_t* slash = wcsrchr(buffer, L'\\');
            if (slash) {
                *(slash + 1) = L'\0';
                wcscat_s(buffer, 32768, L"mblink_devtools.dll");
                if (go_mblink_try_load_devtools_path(buffer)) {
                    goto resolve;
                }
            }
        }
    }

    return -3;

resolve:
    go_mblink_devtools_open = (mblink_devtools_open_fn)GetProcAddress(go_mblink_devtools_module, "mblink_devtools_open");
    go_mblink_devtools_close = (mblink_devtools_close_fn)GetProcAddress(go_mblink_devtools_module, "mblink_devtools_close");
    go_mblink_devtools_default_http_options = (mblink_devtools_default_http_options_fn)GetProcAddress(go_mblink_devtools_module, "mblink_devtools_default_http_options");
    go_mblink_devtools_http_start = (mblink_devtools_http_start_fn)GetProcAddress(go_mblink_devtools_module, "mblink_devtools_http_start");
    go_mblink_devtools_http_stop = (mblink_devtools_http_stop_fn)GetProcAddress(go_mblink_devtools_module, "mblink_devtools_http_stop");
    go_mblink_devtools_http_info_free = (mblink_devtools_http_info_free_fn)GetProcAddress(go_mblink_devtools_module, "mblink_devtools_http_info_free");
    go_mblink_ui_dev_default_snapshot_options = (mblink_ui_dev_default_snapshot_options_fn)GetProcAddress(go_mblink_devtools_module, "mblink_ui_dev_default_snapshot_options");
    go_mblink_ui_dev_snapshot_json = (mblink_ui_dev_snapshot_json_fn)GetProcAddress(go_mblink_devtools_module, "mblink_ui_dev_snapshot_json");
    go_mblink_ui_dev_snapshot_file = (mblink_ui_dev_snapshot_file_fn)GetProcAddress(go_mblink_devtools_module, "mblink_ui_dev_snapshot_file");
    go_mblink_ui_dev_command_json = (mblink_ui_dev_command_json_fn)GetProcAddress(go_mblink_devtools_module, "mblink_ui_dev_command_json");
    if (!go_mblink_devtools_open ||
        !go_mblink_devtools_close ||
        !go_mblink_devtools_default_http_options ||
        !go_mblink_devtools_http_start ||
        !go_mblink_devtools_http_stop ||
        !go_mblink_devtools_http_info_free ||
        !go_mblink_ui_dev_default_snapshot_options ||
        !go_mblink_ui_dev_snapshot_json ||
        !go_mblink_ui_dev_snapshot_file ||
        !go_mblink_ui_dev_command_json) {
        return -3;
    }
    return 0;
}

int go_mblink_devtools_open_call(MBlinkHandle h) { return go_mblink_devtools_open(h); }
int go_mblink_devtools_close_call(MBlinkHandle h) { return go_mblink_devtools_close(h); }
MBlinkDevToolsHttpOptions go_mblink_devtools_default_http_options_call(void) { return go_mblink_devtools_default_http_options(); }
int go_mblink_devtools_http_start_call(MBlinkHandle h, const MBlinkDevToolsHttpOptions* o, MBlinkDevToolsHttpInfo* i) { return go_mblink_devtools_http_start(h, o, i); }
int go_mblink_devtools_http_stop_call(MBlinkHandle h) { return go_mblink_devtools_http_stop(h); }
void go_mblink_devtools_http_info_free_call(MBlinkDevToolsHttpInfo* i) { go_mblink_devtools_http_info_free(i); }
MBlinkUiDevSnapshotOptions go_mblink_ui_dev_default_snapshot_options_call(void) { return go_mblink_ui_dev_default_snapshot_options(); }
int go_mblink_ui_dev_snapshot_json_call(MBlinkHandle h, const MBlinkUiDevSnapshotOptions* o, char** out) { return go_mblink_ui_dev_snapshot_json(h, o, out); }
int go_mblink_ui_dev_snapshot_file_call(MBlinkHandle h, const char* p, const MBlinkUiDevSnapshotOptions* o) { return go_mblink_ui_dev_snapshot_file(h, p, o); }
int go_mblink_ui_dev_command_json_call(MBlinkHandle h, const char* c, char** out) { return go_mblink_ui_dev_command_json(h, c, out); }
*/
import "C"

func ensureDevtoolsRuntime() error {
	if rc := C.go_mblink_load_devtools(); rc != 0 {
		return newError(int(rc), "mblink_devtools.dll not found or missing expected exports")
	}
	return nil
}
