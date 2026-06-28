# CMake 库重构设计：重命名库为 cmatch 并集中头文件/源文件

## 背景与目标

用户提出两条明确要求：

1. 无需单独的 `include/` 目录，头文件与 `src` 放在一起。
2. 库名改为 `cmatch`。

本设计在以上约束基础上，采用更现代的 CMake 写法，为后续扩展留出空间。

## 设计决策

### 目录结构

- 在 `src/` 下新建 `cmatch/` 子目录。
- 将 `include/math_utils.h` 移动到 `src/cmatch/math_utils.h`。
- 将 `src/math_utils.cpp` 移动到 `src/cmatch/math_utils.cpp`。
- 删除空的 `include/` 目录。

### 库定义

- 库名从 `mylib` 改为 `cmatch`。
- 不指定 `STATIC` 或 `SHARED`，由 CMake 的 `BUILD_SHARED_LIBS` 变量决定默认类型。
- 添加别名目标 `cmatch::cmatch`，便于外部以现代 CMake 方式链接。

### Include 路径

- 通过 `target_include_directories` 将 `${CMAKE_CURRENT_SOURCE_DIR}` 作为 `PUBLIC` include 路径暴露给库的使用者。
- 外部使用者以 `#include "cmatch/math_utils.h"` 引用公共头文件。

### 根目录清理

- 移除根 `CMakeLists.txt` 中的全局 `include_directories(${PROJECT_SOURCE_DIR}/include)`，改为由目标自行管理 include 路径。

### 测试链接

- `tests/CMakeLists.txt` 中将链接目标从 `mylib` 改为 `cmatch`（或 `cmatch::cmatch`）。
- `tests/test_math_utils.cpp` 中将 `#include "math_utils.h"` 改为 `#include "cmatch/math_utils.h"`。

## 预期变更文件

1. `src/CMakeLists.txt`
2. `CMakeLists.txt`（根目录）
3. `tests/CMakeLists.txt`
4. `tests/test_math_utils.cpp`
5. 新增：`src/cmatch/math_utils.h`
6. 新增：`src/cmatch/math_utils.cpp`
7. 删除：`include/math_utils.h`
8. 删除空目录：`include/`

## 不变更的内容

- 代码实现逻辑与 `namespace cmatch` 保持不变。
- C++ 标准、编译器、测试框架、构建预设均保持不变。
- 构建与测试命令保持不变：
  - `cmake --build --preset ninja-debug`
  - `ctest --preset ninja-debug`

## 验收标准

- `cmake --build --preset ninja-debug` 成功。
- `ctest --preset ninja-debug` 全部通过。
- `clang-format` 与 `clang-tidy` 无新增警告。
- 不再存在 `include/` 目录。
- 库目标名为 `cmatch`，且存在别名 `cmatch::cmatch`。
- 测试代码能正确 `#include "cmatch/math_utils.h"` 并通过编译。
