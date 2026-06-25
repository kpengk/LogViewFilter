# 记住文件对话框上次路径实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 为“选择日志文件”和“保存过滤结果”分别记录上次目录，并通过 `QSettings::IniFormat` 持久化到程序所在目录下的 `LogViewFilter.ini`。

**Architecture:** 在 `MainWindow` 中维护两个字符串成员 `lastOpenDir_` 和 `lastSaveDir_`；构造函数从 `QSettings` 读取，文件对话框关闭后更新并写回配置文件。不引入新的配置类，保持与现有 Qt Widgets 代码风格一致。

**Tech Stack:** Qt 6.5+ / C++17 / CMake / Visual Studio

## Global Constraints

- 配置文件使用 `QSettings::IniFormat`，路径通过 `QCoreApplication::applicationDirPath() + "/LogViewFilter.ini"` 动态获取，**不写入注册表**。
- 打开和保存分别记忆目录：`lastOpenDir` 用于“选择日志文件”，`lastSaveDir` 用于“保存过滤结果”。
- 起始目录不存在时回退到默认行为（空字符串）。
- 用户取消对话框时不更新设置。

---

## File Structure

| 文件 | 操作 | 说明 |
|------|------|------|
| `MainWindow.h` | 修改 | 新增 `lastOpenDir_`、`lastSaveDir_` 两个私有成员。 |
| `MainWindow.cpp` | 修改 | 构造函数读取配置；`onBrowseFile()` 和 `onSave()` 使用并更新对应目录。 |

---

### Task 1: 在 MainWindow 中新增配置读取逻辑

**Files:**
- Modify: `MainWindow.h:55-58`
- Modify: `MainWindow.cpp:1-37`

**Interfaces:**
- Consumes: 无
- Produces: `MainWindow::lastOpenDir_`、`MainWindow::lastSaveDir_`，在构造时从 `QSettings` 初始化。

- [ ] **Step 1: 在 MainWindow.h 中新增两个成员变量**

在 `filtered_lines_` 声明之前添加：

```cpp
    // 上次文件对话框使用的目录
    QString lastOpenDir_;
    QString lastSaveDir_;
```

- [ ] **Step 2: 在 MainWindow.cpp 构造函数中读取配置**

在现有 `#include` 区域添加 `QSettings` 头文件：

```cpp
#include <QSettings>
```

在构造函数体开头（`ui->setupUi(this);` 之后即可）添加：

```cpp
    // 读取上次文件对话框使用的目录
    QSettings settings(QCoreApplication::applicationDirPath() + "/LogViewFilter.ini", QSettings::IniFormat);
    lastOpenDir_ = settings.value("lastOpenDir").toString();
    lastSaveDir_ = settings.value("lastSaveDir").toString();
```

- [ ] **Step 3: 编译验证**

Run: `cmake --build out/build/x64-Debug`
Expected: 编译通过，无新增警告。

- [ ] **Step 4: Commit**

```bash
git add MainWindow.h MainWindow.cpp
git commit -m "feat: 读取上次文件对话框目录配置"
```

---

### Task 2: 让“选择日志文件”记住并更新上次目录

**Files:**
- Modify: `MainWindow.cpp:43-50`

**Interfaces:**
- Consumes: `MainWindow::lastOpenDir_`
- Produces: 更新后的 `MainWindow::lastOpenDir_` 并写回 `QSettings`。

- [ ] **Step 1: 修改 onBrowseFile() 使用 lastOpenDir_ 作为起始目录**

将 `onBrowseFile()` 替换为：

```cpp
void MainWindow::onBrowseFile() {
    const QString path = QFileDialog::getOpenFileName(
        this, tr("选择日志文件"), lastOpenDir_,
        tr("日志文件 (*.log *.txt *.out);;所有文件 (*.*)"));

    if (path.isEmpty()) {
        return;
    }

    ui->lineEdit_filePath->setText(path);

    // 更新并持久化上次打开目录
    lastOpenDir_ = QFileInfo(path).path();
    QSettings settings(QCoreApplication::applicationDirPath() + "/LogViewFilter.ini", QSettings::IniFormat);
    settings.setValue("lastOpenDir", lastOpenDir_);
}
```

- [ ] **Step 2: 编译验证**

Run: `cmake --build out/build/x64-Debug`
Expected: 编译通过。

- [ ] **Step 3: 手动验证**

1. 运行程序，点击“浏览”按钮选择某个目录下的日志文件。
2. 检查程序所在目录是否生成 `LogViewFilter.ini`，并包含 `lastOpenDir=<所选目录>`。
3. 关闭程序后重新打开，点击“浏览”，对话框应从该目录开始。

- [ ] **Step 4: Commit**

```bash
git add MainWindow.cpp
git commit -m "feat: 选择日志文件时记住并持久化上次目录"
```

---

### Task 3: 让“保存过滤结果”记住并更新上次目录

**Files:**
- Modify: `MainWindow.cpp:99-126`

**Interfaces:**
- Consumes: `MainWindow::lastSaveDir_`
- Produces: 更新后的 `MainWindow::lastSaveDir_` 并写回 `QSettings`。

- [ ] **Step 1: 修改 onSave() 使用 lastSaveDir_ 作为起始目录**

将 `onSave()` 中保存对话框相关代码替换为：

```cpp
    // 构造保存对话框的默认路径：优先使用 lastSaveDir_，否则仅使用默认文件名
    const QString defaultSavePath = lastSaveDir_.isEmpty()
                                        ? QStringLiteral("filter_out.log")
                                        : QDir(lastSaveDir_).filePath(QStringLiteral("filter_out.log"));

    const QString savePath = QFileDialog::getSaveFileName(
        this, tr("保存过滤结果"), defaultSavePath, tr("日志文件 (*.log *.txt);;所有文件 (*.*)"));

    if (savePath.isEmpty()) {
        return;
    }

    // 更新并持久化上次保存目录
    lastSaveDir_ = QFileInfo(savePath).path();
    QSettings settings(QCoreApplication::applicationDirPath() + "/LogViewFilter.ini", QSettings::IniFormat);
    settings.setValue("lastSaveDir", lastSaveDir_);
```

- [ ] **Step 2: 编译验证**

Run: `cmake --build out/build/x64-Debug`
Expected: 编译通过。

- [ ] **Step 3: 手动验证**

1. 运行程序，选择日志文件并执行过滤。
2. 点击“保存”按钮，将结果保存到某个目录。
3. 检查程序所在目录下的 `LogViewFilter.ini` 是否包含 `lastSaveDir=<所选目录>`。
4. 关闭程序后重新打开，再次点击“保存”，对话框默认路径应从该目录开始，文件名仍为 `filter_out.log`。

- [ ] **Step 4: Commit**

```bash
git add MainWindow.cpp
git commit -m "feat: 保存过滤结果时记住并持久化上次目录"
```

---

### Task 4: 回归验证与边界测试

**Files:**
- None

**Interfaces:**
- Consumes: Task 1-3 的完整实现

- [ ] **Step 1: 首次运行（无配置文件）验证**

1. 删除程序所在目录下的 `LogViewFilter.ini`。
2. 运行程序，点击“浏览”和“保存”。
3. Expected: 对话框从系统默认位置开始，程序不崩溃。

- [ ] **Step 2: 目录被删除后的回退验证**

1. 先完成一次打开和保存，生成 `LogViewFilter.ini`。
2. 手动删除 `lastOpenDir` 对应的目录。
3. 重新运行程序并点击“浏览”。
4. Expected: 对话框从默认位置开始（`QFileDialog` 在无效起始目录时会自动回退）。

- [ ] **Step 3: 打开与保存目录独立验证**

1. 打开文件时选择目录 A，保存时选择目录 B。
2. 重新运行程序，分别检查打开和保存的起始目录。
3. Expected: 打开从 A 开始，保存从 B 开始，互不干扰。

- [ ] **Step 4: Commit（可选，如无代码变更可跳过）**

若无代码变更则无需提交；若有微调则按常规提交。

---

## Self-Review

**1. Spec coverage:**
- `QSettings::IniFormat` 持久化：Task 1-3 均使用 `QCoreApplication::applicationDirPath() + "/LogViewFilter.ini"`。✅
- 打开与保存分别记录：Task 2 处理 `lastOpenDir_`，Task 3 处理 `lastSaveDir_`。✅
- 不写入注册表：使用 `IniFormat` 与文件路径显式指定。✅
- 无效目录回退：Task 4 Step 2 验证。✅

**2. Placeholder scan:**
- 无 TBD / TODO / “implement later”。✅
- 每个代码步骤均给出具体代码。✅
- 每个验证步骤均给出命令或操作。✅

**3. Type consistency:**
- 成员变量名称在 Task 1-3 中一致：`lastOpenDir_`、`lastSaveDir_`。✅
- `QSettings` 构造方式在所有任务中一致。✅
