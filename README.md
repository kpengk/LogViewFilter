# LogViewFilter

一个基于 Qt 6 的轻量级日志过滤桌面工具。选择日志文件后，按行过滤包含或排除指定关键词的内容，并将结果保存到本地。

## 功能

- **文件选择**：通过文件对话框选择日志文件，或直接把文件拖到路径输入框。
- **关键词过滤**：支持多行关键词，每行一个关键词。
- **两种模式**：
  - **包含模式**：只保留包含任意关键词的行。
  - **移除模式**：移除包含任意关键词的行。
- **结果保存**：将过滤后的结果保存为新的日志文件。
- **记住目录**：打开文件和保存文件分别记录上次使用的目录，持久化到程序所在目录的 `LogViewFilter.ini`。
- **拖放支持**：路径输入框支持文件拖入，关键词输入框仅接受纯文本拖入。

## 环境要求

- Windows 10/11
- [Qt 6.5+](https://www.qt.io/download)（需要 Core、Widgets 模块）
- [CMake 3.19+](https://cmake.org/download/)
- Visual Studio 2022（带 C++ 桌面开发工作负载）或兼容的 MSVC 编译器

## 构建

### 使用 CMake 预设

确保已设置环境变量 `QTDIR` 指向 Qt 安装目录，例如 `C:\Qt\6.5.3\msvc2019_64`。

```powershell
cmake --preset x64-Debug
cmake --build out/build/x64-Debug
```

构建完成后，可执行文件位于：

```
out/build/x64-Debug/LogViewFilter.exe
```

### 手动配置

```powershell
cmake -B out/build/x64-Debug -S . -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=%QTDIR%
cmake --build out/build/x64-Debug
```

## 使用

1. 运行 `LogViewFilter.exe`。
2. 点击**浏览**选择日志文件，或直接把日志文件拖到路径输入框。
3. 在关键词编辑框中输入关键词，每行一个。
4. 选择**包含**或**移除**模式。
5. 点击**过滤**查看结果。
6. 点击**保存**将过滤结果导出到新文件。

## 配置文件

程序会在首次使用文件对话框后，在**程序所在目录**生成 `LogViewFilter.ini`，记录：

```ini
[General]
lastOpenDir=上次选择日志文件的目录
lastSaveDir=上次保存结果的目录
```

> 注意：若程序部署在只读目录（如 `C:\Program Files`），写入配置文件可能会失败。开发调试时建议将程序放在有写入权限的目录。

## 项目结构

```
LogViewFilter/
├── CMakeLists.txt          # CMake 构建配置
├── CMakePresets.json       # CMake 预设
├── main.cpp                # 程序入口
├── MainWindow.h            # 主窗口头文件
├── MainWindow.cpp          # 主窗口实现
├── MainWindow.ui           # Qt Designer 界面文件
├── README.md               # 本文件
└── docs/                   # 设计文档与实现计划
```

## 开发记录

- 选择日志文件时记住并持久化上次目录。
- 保存过滤结果时记住并持久化上次目录。
- 持久化目录无效时自动回退到默认位置。
- 配置文件 `LogViewFilter.ini` 位于程序所在目录。
