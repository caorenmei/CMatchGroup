# AGENTS.md

本文件为参与本项目的 AI Agent 提供协作规范。所有 Agent 在接入本项目时，必须遵循以下准则。

## 构建环境

- 构建系统：CMake
- 预设配置：CMakePresets.json
- 生成器：Ninja

## 开发环境

- 编译器：g++
- C++ 标准：C++20
- 代码规范：Google C++ Style Guide

## 测试环境

- 测试运行器：CTest
- 测试框架：Google Test（gtest）

## 语言规范

- 所有文档、注释、提交信息及交互内容均使用简体中文。
- 代码本身（变量名、函数名、关键字等）保持英文，不受影响。
- 文档注释必须使用简体中文撰写。

## 强制交互准则

- 在需要用户决策、澄清需求或确认方案时，必须使用 `AskUserQuestion` 工具进行提问。
- 每次任务结束时，Agent 必须根据当前上下文提出多个相关后续问题，帮助用户完善下一步工作。
- 结束任务时提出的问题应以多选形式呈现，允许用户选择一个或多个选项继续。
- 在 Auto Permission 模式下，避免使用 `AskUserQuestion` 询问 trivial 决策，直接选择合理方案继续。

## Git 协作规范

- 默认分支：`main`。
- 提交信息格式：遵循 Conventional Commits 规范，描述使用简体中文。
- 提交前必须完成以下检查：
  - `cmake --build --preset ninja-debug` 构建成功。
  - `ctest --preset ninja-debug` 全部测试通过。
  - `clang-format` 已应用，代码格式符合 Google C++ Style。
  - `clang-tidy` 静态分析无新增警告。

## 文档规范

- 所有新增功能必须同步更新 `README.md` 和相关代码注释。
- 公共 API 头文件必须包含 Doxygen 风格文档注释，注释内容使用简体中文。
- 项目说明、提交信息、Issue 与 Pull Request 描述均使用简体中文。
