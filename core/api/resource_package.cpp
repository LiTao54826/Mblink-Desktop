#include "resource_package.h"
#include "core/utils/encoding_utils.h"
#include "tools/esm_loader/embedded_js.h"
#include "tools/app_bundler/bytecode_compiler.h"
#include "tools/app_bundler/module_resolver.h"

#include <cctype>
#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>

extern "C" {
#include "quickjs/quickjs.h"
}

namespace fs = std::filesystem;

namespace mbink::resourcepkg {
namespace {

constexpr uint32_t PKG_MAGIC = 0x5052424d;  // MBRP
constexpr uint32_t PKG_VERSION = 1;
constexpr uint32_t BC_MAGIC = 0x4342424d;   // MBBC

fs::path Utf8PathToFsPath(const char* path) {
#ifdef _WIN32
    return fs::path(mbink::utils::UTF8ToWide(path ? path : ""));
#else
    return fs::path(path ? path : "");
#endif
}

std::string FsPathToUtf8String(const fs::path& path) {
#ifdef _WIN32
    return mbink::utils::WideToUTF8(path.wstring());
#else
    return path.string();
#endif
}

std::string NormalizeFsPath(const fs::path& path) {
    std::string result = FsPathToUtf8String(path.lexically_normal());
    std::replace(result.begin(), result.end(), '\\', '/');
    return result;
}

uint32_t HashKey(const char* key) {
    uint32_t h = 2166136261u;
    const auto* p = reinterpret_cast<const unsigned char*>(key ? key : "");
    while (*p) {
        h ^= *p++;
        h *= 16777619u;
    }
    return h ? h : 0x6d62696eu;
}

void Crypt(std::vector<uint8_t>& data, const char* key) {
    uint32_t s = HashKey(key) ^ 0x9e3779b9u;
    for (size_t i = 0; i < data.size(); ++i) {
        s ^= s << 13;
        s ^= s >> 17;
        s ^= s << 5;
        data[i] ^= static_cast<uint8_t>(s & 0xffu);
    }
}

void W32(std::vector<uint8_t>& out, uint32_t v) {
    out.push_back(static_cast<uint8_t>(v & 0xffu));
    out.push_back(static_cast<uint8_t>((v >> 8) & 0xffu));
    out.push_back(static_cast<uint8_t>((v >> 16) & 0xffu));
    out.push_back(static_cast<uint8_t>((v >> 24) & 0xffu));
}

void W64(std::vector<uint8_t>& out, uint64_t v) {
    for (int i = 0; i < 8; ++i) out.push_back(static_cast<uint8_t>((v >> (i * 8)) & 0xffu));
}

bool ReadU32(const std::vector<uint8_t>& data, size_t& offset, uint32_t& out) {
    if (offset + 4 > data.size()) return false;
    out = static_cast<uint32_t>(data[offset]) |
          (static_cast<uint32_t>(data[offset + 1]) << 8) |
          (static_cast<uint32_t>(data[offset + 2]) << 16) |
          (static_cast<uint32_t>(data[offset + 3]) << 24);
    offset += 4;
    return true;
}

bool ReadU64(const std::vector<uint8_t>& data, size_t& offset, uint64_t& out) {
    if (offset + 8 > data.size()) return false;
    out = 0;
    for (int i = 0; i < 8; ++i) out |= static_cast<uint64_t>(data[offset + i]) << (i * 8);
    offset += 8;
    return true;
}

bool ReadFileBytes(const fs::path& path, std::vector<uint8_t>& out) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return false;
    const auto size = file.tellg();
    if (size < 0) return false;
    file.seekg(0, std::ios::beg);
    out.resize(static_cast<size_t>(size));
    return out.empty() || static_cast<bool>(file.read(reinterpret_cast<char*>(out.data()), size));
}

bool WriteFileBytes(const fs::path& path, const std::vector<uint8_t>& data) {
    std::ofstream file(path, std::ios::binary);
    return file.is_open() &&
           (data.empty() || static_cast<bool>(file.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()))));
}

bool IsJsFile(const fs::path& path) {
    std::string ext = NormalizeFsPath(path.extension());
    for (char& ch : ext) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    return ext == ".js" || ext == ".mjs";
}

fs::path FindOfficialPreactRoot() {
    static const fs::path kOfficialPreactRelativeRoot =
        Utf8PathToFsPath("third_party") / Utf8PathToFsPath("preact");
    fs::path current = fs::absolute(Utf8PathToFsPath(__FILE__)).parent_path();
    while (!current.empty()) {
        const fs::path candidate = current / kOfficialPreactRelativeRoot / "package.json";
        if (fs::exists(candidate)) {
            return current / kOfficialPreactRelativeRoot;
        }
        if (!current.has_parent_path() || current == current.parent_path()) {
            break;
        }
        current = current.parent_path();
    }
    throw std::runtime_error("Unable to locate official Preact sources under third_party/preact");
}

std::string BuildOfficialPreactModule(const fs::path& entry_path) {
    return "export * from '" + NormalizeFsPath(fs::absolute(entry_path)) + "';";
}

std::string EmbeddedOfficialPreactModule(const char* path) {
    auto source = mbink::embedded::GetEmbeddedJS(path);
    if (source.empty()) {
        throw std::runtime_error(std::string("Missing embedded official Preact module: ") + path);
    }
    return std::string(source);
}

std::string StripJsExtension(std::string path) {
    if (path.size() > 3 && path.substr(path.size() - 3) == ".js") {
        path.resize(path.size() - 3);
    }
    return path;
}

std::string OfficialPreactModuleId(const char* path) {
    std::string id(path ? path : "");
    static const std::string prefix = "third_party/preact/";
    if (id.rfind(prefix, 0) == 0) {
        id.replace(0, prefix.size(), "__mbink_official_preact/");
    }
    return id;
}

std::string BuildEmbeddedOfficialPreactModule(const char* path) {
    return "export * from '" + OfficialPreactModuleId(path) + "';";
}

void RegisterOfficialPreactSource(mbink::ModuleResolver& resolver, const char* path) {
    const auto source = EmbeddedOfficialPreactModule(path);
    const auto module_id = OfficialPreactModuleId(path);
    resolver.RegisterBuiltinModule(module_id, source);
    resolver.RegisterBuiltinModule(StripJsExtension(module_id), source);
}

void RegisterOfficialPreactBuiltinModules(mbink::ModuleResolver& resolver) {
    static constexpr const char* kOfficialPreactSources[] = {
        "third_party/preact/src/index.js",
        "third_party/preact/src/render.js",
        "third_party/preact/src/create-element.js",
        "third_party/preact/src/component.js",
        "third_party/preact/src/options.js",
        "third_party/preact/src/util.js",
        "third_party/preact/src/constants.js",
        "third_party/preact/src/clone-element.js",
        "third_party/preact/src/create-context.js",
        "third_party/preact/src/diff/index.js",
        "third_party/preact/src/diff/children.js",
        "third_party/preact/src/diff/props.js",
        "third_party/preact/src/diff/catch-error.js",
        "third_party/preact/hooks/src/index.js",
        "third_party/preact/jsx-runtime/src/index.js",
        "third_party/preact/jsx-runtime/src/utils.js",
    };
    for (const auto* path : kOfficialPreactSources) {
        RegisterOfficialPreactSource(resolver, path);
    }
    resolver.RegisterBuiltinModule("preact", BuildEmbeddedOfficialPreactModule("third_party/preact/src/index.js"));
    resolver.RegisterBuiltinModule("preact/hooks", BuildEmbeddedOfficialPreactModule("third_party/preact/hooks/src/index.js"));
    resolver.RegisterBuiltinModule("preact/jsx-runtime", BuildEmbeddedOfficialPreactModule("third_party/preact/jsx-runtime/src/index.js"));
    resolver.RegisterBuiltinModule("preact/jsx-dev-runtime", BuildEmbeddedOfficialPreactModule("third_party/preact/jsx-runtime/src/index.js"));
}

std::vector<uint8_t> CompileJsFile(const fs::path& path,
                                   const std::string& package_module_path,
                                   std::string& error) {
    mbink::ModuleResolver resolver;
    RegisterOfficialPreactBuiltinModules(resolver);

    auto modules = resolver.Resolve(FsPathToUtf8String(path));
    if (resolver.HasErrors()) {
        error = resolver.GetErrors().front().message;
        return {};
    }

    const std::string virtual_entry = package_module_path.empty()
        ? std::string("/")
        : std::string("/") + package_module_path;
    const fs::path source_entry = path.lexically_normal();
    const fs::path virtual_entry_path = fs::path(virtual_entry).lexically_normal();

    std::unordered_map<std::string, std::string> rewritten_ids;
    for (auto& module : modules) {
        if (module.is_builtin || module.path.empty()) continue;
        const std::string original_id = module.id;
        fs::path source_module = Utf8PathToFsPath(module.path.c_str()).lexically_normal();
        fs::path rel = fs::relative(source_module, source_entry.parent_path());
        fs::path virtual_module = (virtual_entry_path.parent_path() / rel).lexically_normal();
        module.id = NormalizeFsPath(virtual_module);
        module.path = FsPathToUtf8String(source_module);
        rewritten_ids[original_id] = module.id;
    }

    for (auto& module : modules) {
        for (auto& dep : module.dependencies) {
            auto it = rewritten_ids.find(dep);
            if (it != rewritten_ids.end()) {
                if (module.is_builtin) {
                    const std::string old_specifier = "'" + dep + "'";
                    const std::string new_specifier = "'" + it->second + "'";
                    size_t pos = 0;
                    while ((pos = module.source.find(old_specifier, pos)) != std::string::npos) {
                        module.source.replace(pos, old_specifier.size(), new_specifier);
                        pos += new_specifier.size();
                    }
                }
                dep = it->second;
            }
        }
    }

    mbink::BytecodeCompiler compiler;
    compiler.SetStripSource(true);
    compiler.SetStripDebug(true);
    compiler.SetEntryDir(NormalizeFsPath(virtual_entry_path.parent_path()));
    auto compiled = compiler.CompileModules(modules);
    if (compiler.HasErrors()) {
        error = compiler.GetErrors().front().message;
        return {};
    }

    auto merged = mbink::BytecodeCompiler::MergeBytecode(compiled);
    std::vector<uint8_t> out;
    W32(out, BC_MAGIC);
    out.insert(out.end(), merged.begin(), merged.end());
    return out;
}

bool BuildPayload(const fs::path& input, std::vector<uint8_t>& payload, std::string& error) {
    std::vector<fs::path> files;
    const bool is_dir = fs::is_directory(input);
	    const fs::path base = is_dir ? input : input.parent_path();

    if (is_dir) {
        for (const auto& entry : fs::recursive_directory_iterator(input)) {
            if (entry.is_regular_file()) files.push_back(entry.path());
        }
    } else if (fs::is_regular_file(input)) {
        files.push_back(input);
    } else {
        error = "input path not found";
        return false;
    }

    W32(payload, static_cast<uint32_t>(files.size()));
    for (const auto& file : files) {
        std::string relative = is_dir ? NormalizeFsPath(fs::relative(file, base)) : FsPathToUtf8String(file.filename());
        std::vector<uint8_t> data;
        if (!ReadFileBytes(file, data)) {
            error = "failed to read file: " + FsPathToUtf8String(file);
            return false;
        }

        W32(payload, static_cast<uint32_t>(relative.size()));
        W32(payload, 0u);
        W64(payload, static_cast<uint64_t>(data.size()));
        payload.insert(payload.end(), relative.begin(), relative.end());
        payload.insert(payload.end(), data.begin(), data.end());
    }
    return true;
}

}  // namespace



bool CompileResources(const char* input_path,
                      const char* output_file,
                      const char* encryption_key,
                      std::string& error) {
    if (!input_path || !output_file) {
        error = "invalid parameter";
        return false;
    }

    const fs::path input = fs::absolute(Utf8PathToFsPath(input_path));
    const fs::path output = Utf8PathToFsPath(output_file);

    std::vector<uint8_t> payload;
    if (!BuildPayload(input, payload, error)) return false;

    Crypt(payload, encryption_key);

    std::vector<uint8_t> file;
    W32(file, PKG_MAGIC);
    W32(file, PKG_VERSION);
    W64(file, static_cast<uint64_t>(payload.size()));
    file.insert(file.end(), payload.begin(), payload.end());

    if (!WriteFileBytes(output, file)) {
        error = "failed to write package";
        return false;
    }
    return true;
}

bool LoadResourceFile(const char* package_file,
                      const char* resource_path,
                      const char* encryption_key,
                      std::vector<uint8_t>& out_data,
                      uint32_t* out_flags,
                      std::string& error) {
    if (!package_file || !resource_path) {
        error = "invalid parameter";
        return false;
    }

    std::vector<uint8_t> file;
    if (!ReadFileBytes(Utf8PathToFsPath(package_file), file) || file.size() < 16) {
        error = "failed to read package";
        return false;
    }

    size_t offset = 0;
    uint32_t magic = 0;
    uint32_t version = 0;
    uint64_t payload_size = 0;
    if (!ReadU32(file, offset, magic) || !ReadU32(file, offset, version) ||
        !ReadU64(file, offset, payload_size)) {
        error = "invalid package header";
        return false;
    }
    if (magic != PKG_MAGIC || version != PKG_VERSION) {
        error = "invalid package header";
        return false;
    }
    if (offset + payload_size > file.size()) {
        error = "invalid package size";
        return false;
    }

    std::vector<uint8_t> payload(file.begin() + offset,
                                 file.begin() + offset + static_cast<size_t>(payload_size));
    Crypt(payload, encryption_key);

    offset = 0;
    uint32_t count = 0;
    if (!ReadU32(payload, offset, count)) {
        error = "invalid payload";
        return false;
    }

    const std::string target = fs::path(resource_path).generic_string();
    for (uint32_t i = 0; i < count; ++i) {
        uint32_t name_len = 0;
        uint32_t flags = 0;
        uint64_t size = 0;
        if (!ReadU32(payload, offset, name_len) || !ReadU32(payload, offset, flags) ||
            !ReadU64(payload, offset, size)) {
            error = "invalid payload entry";
            return false;
        }
        if (offset + name_len + size > payload.size()) {
            error = "invalid payload entry";
            return false;
        }

        std::string name(payload.begin() + offset,
                         payload.begin() + offset + name_len);
        offset += name_len;
        if (name == target) {
            out_data.assign(payload.begin() + offset,
                            payload.begin() + offset + static_cast<size_t>(size));
            if (out_flags) *out_flags = flags;
            return true;
        }
        offset += static_cast<size_t>(size);
    }

    error = "resource not found";
    return false;
}

bool ResourceExists(const char* package_file,
                    const char* resource_path,
                    const char* encryption_key) {
    std::vector<uint8_t> data;
    std::string error;
    return LoadResourceFile(package_file, resource_path, encryption_key, data, nullptr, error);
}

bool EvalMaybeMergedBytecode(JSContext* js_ctx,
                             const void* data,
                             size_t size,
                             std::string& error) {
    if (!js_ctx || !data || size == 0) {
        error = "invalid parameter";
        return false;
    }

    const auto* bytes = static_cast<const uint8_t*>(data);
    const uint32_t magic = size >= 4
        ? (static_cast<uint32_t>(bytes[0]) |
           (static_cast<uint32_t>(bytes[1]) << 8) |
           (static_cast<uint32_t>(bytes[2]) << 16) |
           (static_cast<uint32_t>(bytes[3]) << 24))
        : 0;

    if (magic == BC_MAGIC) {
        std::vector<uint8_t> merged(bytes + 4, bytes + size);
        auto modules = mbink::BytecodeCompiler::ParseMergedBytecode(merged);
        for (auto& module : modules) {
            JSValue obj = JS_ReadObject(js_ctx, module.bytecode.data(), module.bytecode.size(), JS_READ_OBJ_BYTECODE);
            if (JS_IsException(obj)) {
                error = "Failed to read merged bytecode";
                return false;
            }
            JSValue result = JS_EvalFunction(js_ctx, obj);
            if (JS_IsException(result)) {
                JSValue exc = JS_GetException(js_ctx);
                const char* err = JS_ToCString(js_ctx, exc);
                if (err) {
                    error = err;
                    JS_FreeCString(js_ctx, err);
                }
                JS_FreeValue(js_ctx, exc);
                JS_FreeValue(js_ctx, result);
                return false;
            }
            JS_FreeValue(js_ctx, result);
        }
        return true;
    }

    JSValue obj = JS_ReadObject(js_ctx, bytes, size, JS_READ_OBJ_BYTECODE);
    if (JS_IsException(obj)) {
        error = "Failed to read bytecode";
        return false;
    }
    JSValue result = JS_EvalFunction(js_ctx, obj);
    if (JS_IsException(result)) {
        JSValue exc = JS_GetException(js_ctx);
        const char* err = JS_ToCString(js_ctx, exc);
        if (err) {
            error = err;
            JS_FreeCString(js_ctx, err);
        }
        JS_FreeValue(js_ctx, exc);
        JS_FreeValue(js_ctx, result);
        return false;
    }
    JS_FreeValue(js_ctx, result);
    return true;
}

}  // namespace mbink::resourcepkg

