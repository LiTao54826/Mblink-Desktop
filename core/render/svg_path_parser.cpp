/**
 * @file svg_path_parser.cpp
 * @brief SVG路径解析器实现
 */

#include "svg_path_parser.h"
#include <cmath>
#include <cctype>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace lightui {

// ========== SVGPathParser 实现 ==========

size_t SVGPathParser::SkipWhitespace(const std::string& str, size_t pos) {
    while (pos < str.length() && std::isspace(str[pos])) {
        ++pos;
    }
    return pos;
}

size_t SVGPathParser::SkipCommaWhitespace(const std::string& str, size_t pos) {
    pos = SkipWhitespace(str, pos);
    if (pos < str.length() && str[pos] == ',') {
        ++pos;
        pos = SkipWhitespace(str, pos);
    }
    return pos;
}

bool SVGPathParser::ParseNumber(const std::string& str, size_t& pos, float& value) {
    pos = SkipCommaWhitespace(str, pos);
    if (pos >= str.length()) return false;

    size_t start = pos;
    
    // 可选的符号
    if (str[pos] == '+' || str[pos] == '-') {
        ++pos;
    }
    
    // 整数部分
    bool has_digits = false;
    while (pos < str.length() && std::isdigit(str[pos])) {
        ++pos;
        has_digits = true;
    }
    
    // 小数部分
    if (pos < str.length() && str[pos] == '.') {
        ++pos;
        while (pos < str.length() && std::isdigit(str[pos])) {
            ++pos;
            has_digits = true;
        }
    }
    
    // 指数部分
    if (pos < str.length() && (str[pos] == 'e' || str[pos] == 'E')) {
        ++pos;
        if (pos < str.length() && (str[pos] == '+' || str[pos] == '-')) {
            ++pos;
        }
        while (pos < str.length() && std::isdigit(str[pos])) {
            ++pos;
        }
    }
    
    if (!has_digits || start == pos) return false;
    
    try {
        value = std::stof(str.substr(start, pos - start));
        return true;
    } catch (...) {
        return false;
    }
}

bool SVGPathParser::ParseFlag(const std::string& str, size_t& pos, bool& flag) {
    pos = SkipCommaWhitespace(str, pos);
    if (pos >= str.length()) return false;
    
    if (str[pos] == '0') {
        flag = false;
        ++pos;
        return true;
    } else if (str[pos] == '1') {
        flag = true;
        ++pos;
        return true;
    }
    return false;
}

SkPath SVGPathParser::Parse(const std::string& d) {
    SkPath path;
    if (d.empty()) return path;

    size_t pos = 0;
    float current_x = 0, current_y = 0;
    float start_x = 0, start_y = 0;  // subpath起点
    float last_control_x = 0, last_control_y = 0;  // 上一个控制点（用于S/T命令）
    char last_command = 0;

    while (pos < d.length()) {
        pos = SkipWhitespace(d, pos);
        if (pos >= d.length()) break;

        char cmd = d[pos];
        bool is_relative = std::islower(cmd);
        cmd = std::toupper(cmd);

        // 检查是否是有效命令
        if (std::isalpha(d[pos]) && std::string("MLHVCSQTAZ").find(cmd) != std::string::npos) {
            ++pos;
        } else if (std::isdigit(d[pos]) || d[pos] == '-' || d[pos] == '+' || d[pos] == '.') {
            // 重复上一个命令（除了M变L）
            if (last_command == 'M') cmd = 'L';
            else if (last_command == 'm') { cmd = 'L'; is_relative = true; }
            else { cmd = std::toupper(last_command); is_relative = std::islower(last_command); }
        } else {
            ++pos;
            continue;
        }

        last_command = is_relative ? std::tolower(cmd) : cmd;

        // 解析参数并执行命令
        switch (cmd) {
            case 'M': {  // MoveTo
                float x, y;
                if (!ParseNumber(d, pos, x) || !ParseNumber(d, pos, y)) break;
                if (is_relative) { x += current_x; y += current_y; }
                path.moveTo(x, y);
                current_x = start_x = x;
                current_y = start_y = y;
                last_command = is_relative ? 'l' : 'L';  // 后续坐标作为LineTo
                // 继续解析额外的坐标对
                while (pos < d.length()) {
                    pos = SkipCommaWhitespace(d, pos);
                    float nx, ny;
                    size_t saved_pos = pos;
                    if (!ParseNumber(d, pos, nx) || !ParseNumber(d, pos, ny)) { pos = saved_pos; break; }
                    if (is_relative) { nx += current_x; ny += current_y; }
                    path.lineTo(nx, ny);
                    current_x = nx; current_y = ny;
                }
                break;
            }

            case 'L': {  // LineTo
                float x, y;
                while (ParseNumber(d, pos, x) && ParseNumber(d, pos, y)) {
                    if (is_relative) { x += current_x; y += current_y; }
                    path.lineTo(x, y);
                    current_x = x; current_y = y;
                }
                break;
            }

            case 'H': {  // Horizontal LineTo
                float x;
                while (ParseNumber(d, pos, x)) {
                    if (is_relative) x += current_x;
                    path.lineTo(x, current_y);
                    current_x = x;
                }
                break;
            }

            case 'V': {  // Vertical LineTo
                float y;
                while (ParseNumber(d, pos, y)) {
                    if (is_relative) y += current_y;
                    path.lineTo(current_x, y);
                    current_y = y;
                }
                break;
            }

            case 'C': {  // Cubic Bezier
                float x1, y1, x2, y2, x, y;
                while (ParseNumber(d, pos, x1) && ParseNumber(d, pos, y1) &&
                       ParseNumber(d, pos, x2) && ParseNumber(d, pos, y2) &&
                       ParseNumber(d, pos, x) && ParseNumber(d, pos, y)) {
                    if (is_relative) {
                        x1 += current_x; y1 += current_y;
                        x2 += current_x; y2 += current_y;
                        x += current_x; y += current_y;
                    }
                    path.cubicTo(x1, y1, x2, y2, x, y);
                    last_control_x = x2; last_control_y = y2;
                    current_x = x; current_y = y;
                }
                break;
            }

            case 'S': {  // Smooth Cubic Bezier
                float x2, y2, x, y;
                while (ParseNumber(d, pos, x2) && ParseNumber(d, pos, y2) &&
                       ParseNumber(d, pos, x) && ParseNumber(d, pos, y)) {
                    // 计算反射的第一个控制点
                    float x1 = 2 * current_x - last_control_x;
                    float y1 = 2 * current_y - last_control_y;
                    if (is_relative) {
                        x2 += current_x; y2 += current_y;
                        x += current_x; y += current_y;
                    }
                    path.cubicTo(x1, y1, x2, y2, x, y);
                    last_control_x = x2; last_control_y = y2;
                    current_x = x; current_y = y;
                }
                break;
            }

            case 'Q': {  // Quadratic Bezier
                float x1, y1, x, y;
                while (ParseNumber(d, pos, x1) && ParseNumber(d, pos, y1) &&
                       ParseNumber(d, pos, x) && ParseNumber(d, pos, y)) {
                    if (is_relative) {
                        x1 += current_x; y1 += current_y;
                        x += current_x; y += current_y;
                    }
                    path.quadTo(x1, y1, x, y);
                    last_control_x = x1; last_control_y = y1;
                    current_x = x; current_y = y;
                }
                break;
            }

            case 'T': {  // Smooth Quadratic Bezier
                float x, y;
                while (ParseNumber(d, pos, x) && ParseNumber(d, pos, y)) {
                    // 计算反射的控制点
                    float x1 = 2 * current_x - last_control_x;
                    float y1 = 2 * current_y - last_control_y;
                    if (is_relative) { x += current_x; y += current_y; }
                    path.quadTo(x1, y1, x, y);
                    last_control_x = x1; last_control_y = y1;
                    current_x = x; current_y = y;
                }
                break;
            }

            case 'A': {  // Arc
                float rx, ry, angle, x, y;
                bool large_arc, sweep;
                while (ParseNumber(d, pos, rx) && ParseNumber(d, pos, ry) &&
                       ParseNumber(d, pos, angle) &&
                       ParseFlag(d, pos, large_arc) && ParseFlag(d, pos, sweep) &&
                       ParseNumber(d, pos, x) && ParseNumber(d, pos, y)) {
                    if (is_relative) { x += current_x; y += current_y; }
                    ArcToBezier(path, current_x, current_y, rx, ry, angle, large_arc, sweep, x, y);
                    current_x = x; current_y = y;
                }
                break;
            }

            case 'Z': {  // ClosePath
                path.close();
                current_x = start_x;
                current_y = start_y;
                break;
            }
        }
    }

    return path;
}

void SVGPathParser::ArcToBezier(SkPath& path,
                                float x1, float y1,
                                float rx, float ry,
                                float angle,
                                bool large_arc,
                                bool sweep,
                                float x2, float y2) {
    // 处理特殊情况
    if (rx == 0 || ry == 0) {
        path.lineTo(x2, y2);
        return;
    }
    
    // 使用Skia的arcTo方法
    // 将角度转换为弧度
    float rad = angle * M_PI / 180.0f;
    
    // Skia期望的是扫描方向，而SVG使用的是sweep标志
    SkPath::ArcSize arc_size = large_arc ? SkPath::kLarge_ArcSize : SkPath::kSmall_ArcSize;
    SkPathDirection direction = sweep ? SkPathDirection::kCW : SkPathDirection::kCCW;
    
    path.arcTo(rx, ry, angle, arc_size, direction, x2, y2);
}

std::vector<SVGPathSegment> SVGPathParser::ParseToSegments(const std::string& d) {
    // TODO: 实现分段解析
    return {};
}

// ========== SVGTransformParser 实现 ==========

SkMatrix SVGTransformParser::Parse(const std::string& transform) {
    SkMatrix result = SkMatrix::I();
    if (transform.empty()) return result;

    size_t pos = 0;
    while (pos < transform.length()) {
        // 跳过空白
        while (pos < transform.length() && std::isspace(transform[pos])) ++pos;
        if (pos >= transform.length()) break;

        SkMatrix m;
        if (ParseFunction(transform, pos, m)) {
            result = SkMatrix::Concat(result, m);
        } else {
            ++pos;  // 跳过无法识别的字符
        }
    }

    return result;
}

bool SVGTransformParser::ParseFunction(const std::string& str, size_t& pos, SkMatrix& matrix) {
    // 读取函数名
    size_t name_start = pos;
    while (pos < str.length() && std::isalpha(str[pos])) ++pos;
    std::string func_name = str.substr(name_start, pos - name_start);

    // 跳过空白，找到左括号
    while (pos < str.length() && std::isspace(str[pos])) ++pos;
    if (pos >= str.length() || str[pos] != '(') return false;
    ++pos;

    // 解析参数
    auto params = ParseParams(str, pos);

    // 跳过右括号
    while (pos < str.length() && std::isspace(str[pos])) ++pos;
    if (pos < str.length() && str[pos] == ')') ++pos;

    // 根据函数名构建矩阵
    if (func_name == "translate") {
        if (params.size() >= 1) {
            float tx = params[0];
            float ty = params.size() >= 2 ? params[1] : 0;
            matrix = SkMatrix::Translate(tx, ty);
            return true;
        }
    } else if (func_name == "scale") {
        if (params.size() >= 1) {
            float sx = params[0];
            float sy = params.size() >= 2 ? params[1] : sx;
            matrix = SkMatrix::Scale(sx, sy);
            return true;
        }
    } else if (func_name == "rotate") {
        if (params.size() >= 1) {
            float angle = params[0];
            if (params.size() >= 3) {
                // rotate(angle, cx, cy)
                float cx = params[1], cy = params[2];
                matrix = SkMatrix::RotateDeg(angle, {cx, cy});
            } else {
                matrix = SkMatrix::RotateDeg(angle);
            }
            return true;
        }
    } else if (func_name == "skewX") {
        if (params.size() >= 1) {
            float angle = params[0] * M_PI / 180.0f;
            matrix = SkMatrix::MakeAll(1, std::tan(angle), 0,
                                       0, 1, 0,
                                       0, 0, 1);
            return true;
        }
    } else if (func_name == "skewY") {
        if (params.size() >= 1) {
            float angle = params[0] * M_PI / 180.0f;
            matrix = SkMatrix::MakeAll(1, 0, 0,
                                       std::tan(angle), 1, 0,
                                       0, 0, 1);
            return true;
        }
    } else if (func_name == "matrix") {
        if (params.size() >= 6) {
            // matrix(a, b, c, d, e, f) -> | a c e |
            //                             | b d f |
            //                             | 0 0 1 |
            matrix = SkMatrix::MakeAll(params[0], params[2], params[4],
                                       params[1], params[3], params[5],
                                       0, 0, 1);
            return true;
        }
    }

    matrix = SkMatrix::I();
    return false;
}

std::vector<float> SVGTransformParser::ParseParams(const std::string& str, size_t& pos) {
    std::vector<float> params;

    while (pos < str.length() && str[pos] != ')') {
        // 跳过空白和逗号
        while (pos < str.length() && (std::isspace(str[pos]) || str[pos] == ',')) ++pos;
        if (pos >= str.length() || str[pos] == ')') break;

        // 解析数字
        size_t start = pos;
        if (str[pos] == '-' || str[pos] == '+') ++pos;
        while (pos < str.length() && (std::isdigit(str[pos]) || str[pos] == '.')) ++pos;
        if (pos < str.length() && (str[pos] == 'e' || str[pos] == 'E')) {
            ++pos;
            if (pos < str.length() && (str[pos] == '-' || str[pos] == '+')) ++pos;
            while (pos < str.length() && std::isdigit(str[pos])) ++pos;
        }

        if (start != pos) {
            try {
                params.push_back(std::stof(str.substr(start, pos - start)));
            } catch (...) {}
        }
    }

    return params;
}

// ========== SVGPointsParser 实现 ==========

std::vector<float> SVGPointsParser::Parse(const std::string& points) {
    std::vector<float> result;
    if (points.empty()) return result;

    size_t pos = 0;
    while (pos < points.length()) {
        // 跳过空白和逗号
        while (pos < points.length() && (std::isspace(points[pos]) || points[pos] == ',')) ++pos;
        if (pos >= points.length()) break;

        // 解析数字
        size_t start = pos;
        if (points[pos] == '-' || points[pos] == '+') ++pos;
        while (pos < points.length() && (std::isdigit(points[pos]) || points[pos] == '.')) ++pos;

        if (start != pos) {
            try {
                result.push_back(std::stof(points.substr(start, pos - start)));
            } catch (...) {}
        }
    }

    return result;
}

SkPath SVGPointsParser::ToPolylinePath(const std::string& points) {
    SkPath path;
    auto coords = Parse(points);

    if (coords.size() >= 2) {
        path.moveTo(coords[0], coords[1]);
        for (size_t i = 2; i + 1 < coords.size(); i += 2) {
            path.lineTo(coords[i], coords[i + 1]);
        }
    }

    return path;
}

SkPath SVGPointsParser::ToPolygonPath(const std::string& points) {
    SkPath path = ToPolylinePath(points);
    path.close();
    return path;
}

} // namespace lightui

