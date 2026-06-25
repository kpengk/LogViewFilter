# 记住文件对话框上次路径设计

## 背景与目标

当前 `LogViewFilter` 在“选择日志文件”和“保存过滤结果”时，每次都会从默认位置开始，没有记住用户上次选择的目录。本设计为打开和保存分别记录最后一次目录，并在下次运行程序时仍然有效。

## 设计决策

- **持久化方式**：使用 `QSettings::IniFormat`，文件保存到程序所在目录下的 `LogViewFilter.ini`，通过 `QCoreApplication::applicationDirPath()` 动态获取路径，**不写入注册表**。
- **目录记忆策略**：打开和保存分别记忆，互不干扰。
  - `lastOpenDir`：用于“选择日志文件”。
  - `lastSaveDir`：用于“保存过滤结果”。
- **改动范围**：集中在 `MainWindow.cpp` / `MainWindow.h`，不引入新的配置类，保持与现有代码规模一致。

## 持久化位置

```cpp
QSettings settings(QCoreApplication::applicationDirPath() + "/LogViewFilter.ini", QSettings::IniFormat);
```

- 示例：若程序位于 `D:\Project\LogViewFilter\out\build\x64-Debug\LogViewFilter.exe`，则配置文件为 `D:\Project\LogViewFilter\out\build\x64-Debug\LogViewFilter.ini`。

## 行为规格

### 打开文件

1. 启动时从 `QSettings` 读取 `lastOpenDir`。
2. 调用 `QFileDialog::getOpenFileName` 时，若 `lastOpenDir` 存在且为有效目录，则作为起始目录；否则传空字符串，让系统使用默认位置。
3. 用户选择文件并确认后，提取文件所在目录，写入 `lastOpenDir`。

### 保存文件

1. 启动时从 `QSettings` 读取 `lastSaveDir`。
2. 调用 `QFileDialog::getSaveFileName` 时，若 `lastSaveDir` 存在且为有效目录，则起始目录为 `lastSaveDir/filter_out.log`；否则仍使用默认文件名 `filter_out.log`。
3. 用户选择保存路径并确认后，提取文件所在目录，写入 `lastSaveDir`。

## 数据结构

在 `MainWindow` 中新增两个成员：

```cpp
QString lastOpenDir_;
QString lastSaveDir_;
```

并在构造函数中初始化：

```cpp
QSettings settings(QCoreApplication::applicationDirPath() + "/LogViewFilter.ini", QSettings::IniFormat);
lastOpenDir_ = settings.value("lastOpenDir").toString();
lastSaveDir_ = settings.value("lastSaveDir").toString();
```

## 边界处理

- 读取到的目录不存在时，视为无效，回退到默认行为。
- 用户取消对话框时，不更新设置。
- 文件路径中的反斜杠统一使用 `QDir::toNativeSeparators` 显示，但保存到 ini 时仍使用 Qt 内部统一格式。

## 涉及文件

- `MainWindow.h`：新增 `lastOpenDir_`、`lastSaveDir_` 成员。
- `MainWindow.cpp`：
  - 构造函数读取配置。
  - `onBrowseFile()` 使用并更新 `lastOpenDir_`。
  - `onSave()` 使用并更新 `lastSaveDir_`。

## 测试考虑

- 首次运行无配置文件时，对话框应从默认位置开始。
- 选择文件后关闭程序，再次启动应从上次目录开始。
- 打开和保存的目录分别独立更新。
