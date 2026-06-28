# CMatchGroup

CMatchGroup 是一个基于 C++20 的项目模板，采用 CMake + Ninja 构建系统，遵循 Google C++ Style Guide，并使用 Google Test 进行单元测试。

## 构建要求

- CMake >= 3.14
- Ninja
- g++（支持 C++20）
- Google Test（gtest）
- clang-format / clang-tidy（可选，用于代码规范检查）

## 快速开始

```bash
# 配置 Debug 构建
cmake --preset ninja-debug

# 构建项目
cmake --build --preset ninja-debug

# 运行测试
ctest --preset ninja-debug
```

## 代码规范

- 遵循 [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)。
- 使用 `clang-format` 格式化代码：
  ```bash
  find src include tests -name "*.cpp" -o -name "*.h" | xargs clang-format -i
  ```
- 使用 `clang-tidy` 进行静态分析：
  ```bash
  clang-tidy -p build/debug src/math_utils.cpp
  ```

## 项目结构

```
include/    # 公共头文件
src/        # 源文件
tests/      # 测试文件
build/      # 构建输出目录（自动生成，不提交）
```

## 贡献指南

- 默认分支为 `main`，禁止直接推送，请通过 Pull Request 合并代码。
- 提交信息遵循 Conventional Commits 规范，描述使用简体中文。
- 提交前请确保构建、测试、格式检查和静态分析均通过。

## 许可证

本项目采用 [MIT 许可证](LICENSE)。
