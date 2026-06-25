#pragma once

#include "quickjs.h"
#include "core/dom/file_list.h"

namespace mblink {
namespace bindings {

void InitFileListBinding(JSContext* ctx);
JSValue WrapFile(JSContext* ctx, const FileInfo& file);
JSValue WrapFileList(JSContext* ctx, const FileList& files);
bool FileListFromJSValue(JSContext* ctx, JSValueConst value, FileList* files);

} // namespace bindings
} // namespace mblink
