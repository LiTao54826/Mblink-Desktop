#!/bin/bash
# MBink 第三方库下载脚本
# 用途: 自动下载和配置所有依赖库

set -e  # 遇到错误立即退出

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
THIRD_PARTY_DIR="$PROJECT_ROOT/third_party"

echo "=========================================="
echo "  MBink 依赖库下载脚本"
echo "=========================================="
echo ""

# 颜色输出
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# ========================================
# 代理配置
# ========================================
# 方式1: 通过环境变量设置（推荐）
#   export HTTP_PROXY="http://127.0.0.1:7890"
#   export HTTPS_PROXY="http://127.0.0.1:7890"
#
# 方式2: 直接在下面设置
#   PROXY="http://127.0.0.1:7890"
#
# 方式3: 运行脚本时设置
#   HTTP_PROXY="http://127.0.0.1:7890" ./download_deps.sh
# ========================================

# 从环境变量读取代理，或使用下面的默认值
PROXY="${HTTP_PROXY:-${HTTPS_PROXY:-}}"

# 如果需要，可以在这里直接设置代理（取消注释）
# PROXY="http://127.0.0.1:7890"
# PROXY="socks5://127.0.0.1:7890"

if [ -n "$PROXY" ]; then
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}  代理配置${NC}"
    echo -e "${BLUE}========================================${NC}"
    echo -e "${GREEN}✓ 使用代理: $PROXY${NC}"

    # 设置环境变量
    export http_proxy="$PROXY"
    export https_proxy="$PROXY"
    export HTTP_PROXY="$PROXY"
    export HTTPS_PROXY="$PROXY"
    export ALL_PROXY="$PROXY"

    # 配置Git代理
    git config --global http.proxy "$PROXY"
    git config --global https.proxy "$PROXY"

    echo -e "${GREEN}✓ Git代理已配置${NC}"
    echo ""
else
    echo -e "${YELLOW}========================================${NC}"
    echo -e "${YELLOW}  未设置代理${NC}"
    echo -e "${YELLOW}========================================${NC}"
    echo -e "${YELLOW}如果下载失败，请设置代理：${NC}"
    echo -e "${YELLOW}  export HTTP_PROXY=\"http://127.0.0.1:7890\"${NC}"
    echo -e "${YELLOW}  export HTTPS_PROXY=\"http://127.0.0.1:7890\"${NC}"
    echo -e "${YELLOW}或者编辑脚本，取消 PROXY= 行的注释${NC}"
    echo ""
fi

cd "$THIRD_PARTY_DIR"

# 检查命令是否存在
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# 下载QuickJS
download_quickjs() {
    echo -e "${YELLOW}[1/4] 下载 QuickJS...${NC}"
    
    if [ -d "quickjs" ]; then
        echo -e "${GREEN}QuickJS 已存在，跳过${NC}"
        return
    fi
    
    if command_exists wget; then
        wget https://bellard.org/quickjs/quickjs-2024-01-13.tar.xz
    elif command_exists curl; then
        curl -O https://bellard.org/quickjs/quickjs-2024-01-13.tar.xz
    else
        echo -e "${RED}错误: 需要 wget 或 curl${NC}"
        exit 1
    fi
    
    tar xf quickjs-2024-01-13.tar.xz
    mv quickjs-2024-01-13 quickjs
    rm quickjs-2024-01-13.tar.xz
    
    echo -e "${GREEN}✓ QuickJS 下载完成${NC}"
    echo ""
}

# 下载SDL3
download_sdl3() {
    echo -e "${YELLOW}[2/4] 下载 SDL3...${NC}"
    
    if [ -d "SDL3" ]; then
        echo -e "${GREEN}SDL3 已存在，跳过${NC}"
        return
    fi
    
    if ! command_exists git; then
        echo -e "${RED}错误: 需要 git${NC}"
        exit 1
    fi
    
    git clone --depth 1 https://github.com/libsdl-org/SDL SDL3
    
    echo -e "${GREEN}✓ SDL3 下载完成${NC}"
    echo ""
}

# 下载Yoga
download_yoga() {
    echo -e "${YELLOW}[3/4] 下载 Yoga...${NC}"
    
    if [ -d "yoga" ]; then
        echo -e "${GREEN}Yoga 已存在，跳过${NC}"
        return
    fi
    
    if ! command_exists git; then
        echo -e "${RED}错误: 需要 git${NC}"
        exit 1
    fi
    
    git clone --depth 1 https://github.com/facebook/yoga.git
    
    echo -e "${GREEN}✓ Yoga 下载完成${NC}"
    echo ""
}

# 下载Skia（可选）
download_skia() {
    echo -e "${YELLOW}[4/4] 下载 Skia (可选，较大)...${NC}"
    
    if [ -d "skia" ]; then
        echo -e "${GREEN}Skia 已存在，跳过${NC}"
        return
    fi
    
    read -p "Skia 较大且编译时间长，是否下载? (y/N): " -n 1 -r
    echo
    
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        echo -e "${YELLOW}跳过 Skia 下载${NC}"
        echo -e "${YELLOW}提示: 可以稍后手动下载或使用预编译版本${NC}"
        return
    fi
    
    if ! command_exists git; then
        echo -e "${RED}错误: 需要 git${NC}"
        exit 1
    fi
    
    git clone https://github.com/google/skia.git
    cd skia
    python3 tools/git-sync-deps
    
    echo -e "${GREEN}✓ Skia 下载完成${NC}"
    echo -e "${YELLOW}注意: Skia 需要单独编译，请参考 third_party/README.md${NC}"
    echo ""
}

# 主函数
main() {
    # 检查必要工具
    if ! command_exists git; then
        echo -e "${RED}错误: 未找到 git，请先安装 git${NC}"
        exit 1
    fi
    
    # 下载所有依赖
    download_quickjs
    download_sdl3
    download_yoga
    download_skia
    
    echo ""
    echo -e "${GREEN}=========================================="
    echo -e "  依赖库下载完成！"
    echo -e "==========================================${NC}"
    echo ""
    echo "下一步："
    echo "  1. 编译QuickJS: cd third_party/quickjs && make"
    echo "  2. 配置CMake: mkdir build && cd build && cmake .."
    echo "  3. 编译项目: cmake --build ."
    echo ""
    echo "详细信息请查看: third_party/README.md"
}

# 运行主函数
main

