# Taffy - CSS Layout Engine

Taffy is a flexible, high-performance, cross-platform UI layout library written in Rust.

## Features

- ✅ **Block Layout** - Standard CSS block layout
- ✅ **Flexbox Layout** - Complete Flexbox implementation
- ✅ **CSS Grid Layout** - Full CSS Grid support
- ✅ **W3C Compliant** - Faithfully implements W3C specifications
- ✅ **High Performance** - Used by Servo browser engine

## Version

- **Taffy Version**: 0.9.1
- **C Bindings**: From PR #404
- **License**: MIT

## Directory Structure

```
taffy/
├── README.md           # This file
├── include/            # C header files
│   └── taffy.h        # Main C API header
├── lib/               # Precompiled libraries
│   ├── windows/       # Windows .lib files
│   ├── linux/         # Linux .a files
│   └── macos/         # macOS .a files
└── src/               # Taffy source (for reference/rebuilding)
```

## Building from Source

If you need to rebuild the library:

### Prerequisites

- Rust 1.65+ (install from https://rustup.rs/)
- Cargo (comes with Rust)

### Build Steps

```bash
cd third_party/taffy/src
cargo build --release
```

The compiled library will be in `target/release/`:
- Windows: `taffy.lib` or `taffy.dll.lib`
- Linux: `libtaffy.a` or `libtaffy.so`
- macOS: `libtaffy.a` or `libtaffy.dylib`

## Integration with MBink

The library is integrated into MBink's build system via CMakeLists.txt:

```cmake
# Link Taffy library
target_link_libraries(lightui_render PUBLIC taffy)
```

## API Usage

See `include/taffy.h` for the complete C API documentation.

Basic usage:

```c
#include <taffy.h>

// Create a tree
TaffyTree* tree = TaffyTree_New();

// Create a node with flexbox layout
TaffyStyle* style = TaffyStyle_New();
TaffyStyle_SetDisplay(style, TAFFY_DISPLAY_FLEX);
TaffyStyle_SetFlexDirection(style, TAFFY_FLEX_DIRECTION_ROW);

TaffyNodeId node = TaffyTree_NewNode(tree, style);

// Compute layout
TaffyTree_ComputeLayout(tree, node, 800.0, 600.0);

// Read layout results
TaffyLayout layout = TaffyTree_GetLayout(tree, node);

// Cleanup
TaffyTree_Free(tree);
```

## Resources

- **Official Repo**: https://github.com/DioxusLabs/taffy
- **C Bindings PR**: https://github.com/DioxusLabs/taffy/pull/404
- **Documentation**: https://docs.rs/taffy/

## Notes

- The precompiled libraries are provided for convenience
- Developers do NOT need Rust installed to build MBink
- Only CI/CD or maintainers need Rust to rebuild Taffy

