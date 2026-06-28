# CMake 库重构实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 将库名从 `mylib` 改为 `cmatch`，将 `include/math_utils.h` 与 `src/math_utils.cpp` 集中到 `src/cmatch/`，并采用现代 CMake 目标管理 include 路径与别名目标。

**Architecture：** 在 `src/` 下建立 `cmatch/` 子目录存放公共头文件与对应实现；通过 `target_include_directories` 将 `src/` 作为 `cmatch` 的 PUBLIC include 根目录，使外部以 `#include "cmatch/math_utils.h"` 引用；同时提供 `cmatch::cmatch` 别名目标供链接时使用。

**Tech Stack：** CMake 3.14+、Ninja、g++、Google Test、CTest、clang-format、clang-tidy

## Global Constraints

- C++ 标准：C++20（`CMAKE_CXX_STANDARD 20`）
- 构建预设：`ninja-debug`
- 测试框架：Google Test（gtest）
- 所有文档、注释、提交信息使用简体中文
- 提交前必须完成：`cmake --build --preset ninja-debug` 构建成功、`ctest --preset ninja-debug` 全部通过、`clang-format` 已应用、`clang-tidy` 无新增警告

---

### Task 1: 移动源文件并定义 `cmatch` 库目标

**Files:**
- Create: `src/cmatch/math_utils.h`
- Create: `src/cmatch/math_utils.cpp`
- Modify: `src/CMakeLists.txt`
- Delete: `include/math_utils.h`
- Delete: `include/`（空目录）

**Interfaces:**
- Produces: `cmatch` 库目标、`cmatch::cmatch` 别名目标
- Produces: `src/cmatch/math_utils.h` 作为公共 API 头文件

- [ ] **Step 1: 创建 `src/cmatch/` 并移动头文件**

```bash
git mv include/math_utils.h src/cmatch/math_utils.h
```

- [ ] **Step 2: 移动源文件到 `src/cmatch/`**

```bash
git mv src/math_utils.cpp src/cmatch/math_utils.cpp
```

- [ ] **Step 3: 更新头文件 include guard 以反映新路径**

修改 `src/cmatch/math_utils.h` 中的 include guard：

```cpp
#ifndef CMATCH_SRC_CMATCH_MATH_UTILS_H_
#define CMATCH_SRC_CMATCH_MATH_UTILS_H_
```

并相应修改文件末尾的 `#endif` 注释：

```cpp
#endif  // CMATCH_SRC_CMATCH_MATH_UTILS_H_
```

- [ ] **Step 4: 更新源文件中的 include 路径**

将 `src/cmatch/math_utils.cpp` 修改为：

```cpp
// 数学工具函数示例实现

#include "cmatch/math_utils.h"

namespace cmatch {

int Add(int a, int b) { return a + b; }

}  // namespace cmatch
```

- [ ] **Step 5: 重写 `src/CMakeLists.txt` 定义 `cmatch` 库**

将 `src/CMakeLists.txt` 完整替换为：

```cmake
# src 目录构建配置

add_library(
  cmatch
  cmatch/math_utils.cpp
)

add_library(cmatch::cmatch ALIAS cmatch)

target_include_directories(
  cmatch
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
```

- [ ] **Step 6: 删除空的 `include/` 目录**

```bash
rmdir include
```

- [ ] **Step 7: 重新配置并单独构建 `cmatch` 目标**

```bash
cmake --build --preset ninja-debug --target cmatch
```

Expected: 构建成功，无编译错误。

- [ ] **Step 8: 提交 Task 1 的变更**

```bash
git add src/cmatch/math_utils.h src/cmatch/math_utils.cpp src/CMakeLists.txt include/
git commit -m "refactor: 将头文件/源文件集中到 src/cmatch 并重命名库为 cmatch

- 将 include/math_utils.h 移动到 src/cmatch/math_utils.h
- 将 src/math_utils.cpp 移动到 src/cmatch/math_utils.cpp
- 更新 src/CMakeLists.txt 定义 cmatch 库与 cmatch::cmatch 别名
- 删除空的 include/ 目录

Co-Authored-By: Claude <noreply@anthropic.com>"
```

---

### Task 2: 更新根 CMake 与测试消费端

**Files:**
- Modify: `CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`
- Modify: `tests/test_math_utils.cpp`

**Interfaces:**
- Consumes: `cmatch` / `cmatch::cmatch` 目标（来自 Task 1）
- Produces: 可成功运行的 `test_math_utils` 测试可执行文件

- [ ] **Step 1: 移除根 `CMakeLists.txt` 的全局 include 目录**

将 `CMakeLists.txt` 中的：

```cmake
# 包含目录
include_directories(${PROJECT_SOURCE_DIR}/include)
```

删除。根 `CMakeLists.txt` 最终内容如下：

```cmake
cmake_minimum_required(VERSION 3.14)

# 项目根配置
project(
  CMatchGroup
  VERSION 0.1.0
  LANGUAGES CXX
  DESCRIPTION "CMatchGroup 项目模板"
)

# 全局 C++ 标准设置
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 导出编译数据库，供 clang-tidy 等工具使用
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 子目录
add_subdirectory(src)

# 启用 CTest 测试
enable_testing()
add_subdirectory(tests)
```

- [ ] **Step 2: 更新 `tests/CMakeLists.txt` 链接目标**

将 `tests/CMakeLists.txt` 中的 `mylib` 改为 `cmatch`：

```cmake
# 测试目录构建配置

find_package(GTest REQUIRED)
include(GoogleTest)

# 示例测试可执行文件
add_executable(
  test_math_utils
  test_math_utils.cpp
)

target_link_libraries(
  test_math_utils
  PRIVATE
  cmatch
  GTest::gtest_main
)

# 自动发现测试并注册到 CTest
gtest_discover_tests(test_math_utils)
```

- [ ] **Step 3: 更新测试文件中的 include 路径**

将 `tests/test_math_utils.cpp` 中的：

```cpp
#include "math_utils.h"
```

改为：

```cpp
#include "cmatch/math_utils.h"
```

完整文件内容如下：

```cpp
// 数学工具函数示例测试

#include <gtest/gtest.h>

#include "cmatch/math_utils.h"

namespace cmatch {
namespace {

TEST(AddTest, PositiveNumbers) { EXPECT_EQ(Add(1, 2), 3); }

TEST(AddTest, NegativeNumbers) { EXPECT_EQ(Add(-1, -2), -3); }

TEST(AddTest, Zero) { EXPECT_EQ(Add(0, 0), 0); }

}  // namespace
}  // namespace cmatch
```

- [ ] **Step 4: 全量构建并运行测试**

```bash
cmake --build --preset ninja-debug
ctest --preset ninja-debug
```

Expected: 构建成功，3 个测试全部通过。

- [ ] **Step 5: 应用代码格式检查**

```bash
clang-format -i src/cmatch/math_utils.h src/cmatch/math_utils.cpp tests/test_math_utils.cpp
```

Expected: 无文件内容变更（若已有变更则提交前需单独 add）。

- [ ] **Step 6: 运行静态分析检查**

```bash
clang-tidy src/cmatch/math_utils.cpp -p build/debug
```

Expected: 无新增警告。

- [ ] **Step 7: 提交 Task 2 的变更**

```bash
git add CMakeLists.txt tests/CMakeLists.txt tests/test_math_utils.cpp
git commit -m "build: 更新根 CMake 与测试以使用 cmatch 库

- 移除根 CMakeLists.txt 的全局 include_directories
- tests/CMakeLists.txt 链接目标改为 cmatch
- tests/test_math_utils.cpp 改为 #include \"cmatch/math_utils.h\"

Co-Authored-By: Claude <noreply@anthropic.com>"
```

---

### Task 3: 最终验证与清理

**Files:**
- 无新文件；验证整个工作树。

- [ ] **Step 1: 确认 `include/` 目录已删除**

```bash
ls -la include 2&1 || echo "include/ directory does not exist"
```

Expected: 显示 `include/` 目录不存在。

- [ ] **Step 2: 确认库目标存在且可构建**

```bash
cmake --build --preset ninja-debug --target cmatch
```

Expected: 构建成功。

- [ ] **Step 3: 运行完整测试套件**

```bash
ctest --preset ninja-debug --output-on-failure
```

Expected: 全部测试通过。

- [ ] **Step 4: 确认 git 状态干净**

```bash
git status --short
```

Expected: 空输出或仅存在预期的未跟踪文件。

---

## Self-Review

**1. Spec coverage:**
- 库名改为 `cmatch` → Task 1 Step 5
- 头文件与 src 放在一起 → Task 1 Step 1、Step 2、Step 6
- 不保留单独 `include/` → Task 1 Step 6
- 现代 CMake 写法（别名目标、target_include_directories）→ Task 1 Step 5
- 测试链接与 include 路径更新 → Task 2

**2. Placeholder scan:**
- 无 TBD/TODO
- 所有代码块为完整文件内容或完整 diff
- 所有命令包含预期输出

**3. Type consistency:**
- 库目标名统一为 `cmatch`
- 别名目标统一为 `cmatch::cmatch`
- include 路径统一为 `"cmatch/math_utils.h"`

---

**Plan complete and saved to `docs/superpowers/plans/2026-06-28-cmake-library-restructure.md`. Two execution options:**

**1. Subagent-Driven (recommended)** - I dispatch a fresh subagent per task, review between tasks, fast iteration

**2. Inline Execution** - Execute tasks in this session using executing-plans, batch execution with checkpoints

**Which approach?**
