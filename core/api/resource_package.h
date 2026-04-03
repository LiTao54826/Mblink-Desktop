#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

struct JSContext;

namespace mbink::resourcepkg {

constexpr uint32_t kResourceFlagBytecode = 1u;

bool CompileResources(const char* input_path,
                      const char* output_file,
                      const char* encryption_key,
                      std::string& error);

bool LoadResourceFile(const char* package_file,
                      const char* resource_path,
                      const char* encryption_key,
                      std::vector<uint8_t>& out_data,
                      uint32_t* out_flags,
                      std::string& error);

bool ResourceExists(const char* package_file,
                    const char* resource_path,
                    const char* encryption_key);

bool EvalMaybeMergedBytecode(JSContext* js_ctx,
                             const void* data,
                             size_t size,
                             std::string& error);

}  // namespace mbink::resourcepkg

