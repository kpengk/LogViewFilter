#include "MainWindow.h"

#include "ui_MainWindow.h"

#include <QButtonGroup>
#include <QDir>
#include <QElapsedTimer>
#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QMimeData>
#include <QSettings>
#include <QTextStream>
#include <QUrl>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);

    // 读取上次文件对话框使用的目录
    QSettings settings(QDir::homePath() + "/LogViewFilter.ini", QSettings::IniFormat);
    lastOpenDir_ = settings.value("lastOpenDir").toString();
    lastSaveDir_ = settings.value("lastSaveDir").toString();

    // 将两个单选按钮归入同一互斥组（UI 文件中已经互斥，此处仅做保险）
    QButtonGroup* modeGroup = new QButtonGroup(this);
    modeGroup->addButton(ui->radio_contain);
    modeGroup->addButton(ui->radio_remove);

    // 信号连接
    connect(ui->btn_browse, &QPushButton::clicked, this, &MainWindow::onBrowseFile);
    connect(ui->btn_filter, &QPushButton::clicked, this, &MainWindow::onFilter);
    connect(ui->btn_save, &QPushButton::clicked, this, &MainWindow::onSave);
    connect(ui->btn_clear, &QPushButton::clicked, this, &MainWindow::onClearResult);
    connect(ui->btn_clearKeywords, &QPushButton::clicked, this, &MainWindow::onClearKeywords);

    // 为路径输入框启用拖放：安装事件过滤器来处理 DragEnter / Drop
    ui->lineEdit_filePath->setAcceptDrops(true);
    ui->lineEdit_filePath->installEventFilter(this);

    // 为关键词输入框安装事件过滤器，屏蔽文件拖入（只允许纯文本拖拽）
    ui->plainTextEdit_keywords->installEventFilter(this);
}

MainWindow::~MainWindow() {
    delete ui;
}

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
    QSettings settings(QDir::homePath() + "/LogViewFilter.ini", QSettings::IniFormat);
    settings.setValue("lastOpenDir", lastOpenDir_);
}

void MainWindow::onFilter() {
    const QString filePath = ui->lineEdit_filePath->text().trimmed();
    if (filePath.isEmpty()) {
        QMessageBox::warning(this, tr("提示"), tr("请先选择日志文件。"));
        return;
    }

    const QStringList keywords = parseKeywords();
    if (keywords.isEmpty()) {
        QMessageBox::warning(this, tr("提示"), tr("请至少输入一个关键词。"));
        return;
    }

    const bool containMode = ui->radio_contain->isChecked();

    // 计时
    QElapsedTimer timer;
    timer.start();

    filtered_lines_ = filterLog(filePath, keywords, containMode);

    const qint64 elapsed = timer.elapsed();

    if (filtered_lines_.isEmpty() && !QFile::exists(filePath)) {
        QMessageBox::critical(this, tr("错误"), tr("无法打开文件：\n%1").arg(filePath));
        return;
    }

    // 显示结果（最多显示 50 000 行，避免界面卡顿）
    constexpr int kDisplayLimit = 50000;
    const int total = filtered_lines_.size();
    const int displayCount = qMin(total, kDisplayLimit);

    ui->plainTextEdit_result->setPlainText(filtered_lines_.mid(0, displayCount).join(QLatin1Char('\n')));

    // 统计信息
    const QString modeStr = containMode ? tr("包含") : tr("移除");
    QString stats = tr("模式：%1 | 匹配行：%2").arg(modeStr).arg(total);
    if (total > kDisplayLimit) {
        stats += tr("（预览前 %1 行）").arg(kDisplayLimit);
    }
    stats += tr(" | 耗时：%1 ms").arg(elapsed);
    ui->label_stats->setText(stats);

    ui->btn_save->setEnabled(total > 0);
}

void MainWindow::onSave() {
    if (filtered_lines_.isEmpty()) {
        QMessageBox::information(this, tr("提示"), tr("没有可保存的内容。"));
        return;
    }

    const QString savePath = QFileDialog::getSaveFileName(
        this, tr("保存过滤结果"), QStringLiteral("filter_out.log"), tr("日志文件 (*.log *.txt);;所有文件 (*.*)"));

    if (savePath.isEmpty()) {
        return;
    }

    QFile file(savePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr("错误"), tr("无法写入文件：\n%1").arg(savePath));
        return;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    for (const QString& line : std::as_const(filtered_lines_)) {
        out << line << QLatin1Char('\n');
    }
    file.close();

    statusBar()->showMessage(tr("已保存到：%1").arg(savePath), 5000);
}

void MainWindow::onClearResult() {
    ui->plainTextEdit_result->clear();
    ui->label_stats->setText(tr("就绪"));
    ui->btn_save->setEnabled(false);
    filtered_lines_.clear();
}

void MainWindow::onClearKeywords() {
    ui->plainTextEdit_keywords->clear();
}

QStringList MainWindow::parseKeywords() const {
    const QString text = ui->plainTextEdit_keywords->toPlainText();
    const QStringList raw = text.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    QStringList result;
    result.reserve(raw.size());
    for (const QString& kw : raw) {
        const QString trimmed = kw.trimmed();
        if (!trimmed.isEmpty()) {
            result.append(trimmed);
        }
    }
    return result;
}

bool MainWindow::containsKeyword(QStringView line, const QStringList& keywords) {
    for (const QString& kw : keywords) {
        if (line.contains(kw)) {
            return true;
        }
    }
    return false;
}

QStringList MainWindow::filterLog(const QString& filePath, const QStringList& keywords, bool contain) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }

    QStringList result;
    QTextStream in(&file);
    in.setAutoDetectUnicode(true);

    while (!in.atEnd()) {
        QString line = in.readLine();

        // 判断该行是否包含任意关键词
        const bool contain_key = containsKeyword(line, keywords);
        if (contain_key == contain) {
            result.append(std::move(line));
        }
    }

    file.close();
    return result;
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event) {
    // --- 关键词输入框：屏蔽文件拖入，只允许纯文本拖拽 ---
    if (watched == ui->plainTextEdit_keywords) {
        if (event->type() == QEvent::DragEnter) {
            auto* dragEvent = static_cast<QDragEnterEvent*>(event);
            // 拖入的是文件而非纯文本 → 忽略
            if (dragEvent->mimeData()->hasUrls()) {
                return true;
            }
        }
    }

    // --- 路径输入框：只接受本地文件拖入 ---
    if (watched == ui->lineEdit_filePath) {
        if (event->type() == QEvent::DragEnter) {
            auto* dragEvent = static_cast<QDragEnterEvent*>(event);
            if (dragEvent->mimeData()->hasUrls()) {
                // 检查是否包含至少一个本地文件
                const QList<QUrl> urls = dragEvent->mimeData()->urls();
                for (const QUrl& url : urls) {
                    if (url.isLocalFile()) {
                        dragEvent->acceptProposedAction();
                        return true;
                    }
                }
            }
            return true; // 不接受，阻止默认行为
        }

        if (event->type() == QEvent::Drop) {
            auto* dropEvent = static_cast<QDropEvent*>(event);
            const QList<QUrl> urls = dropEvent->mimeData()->urls();
            for (const QUrl& url : urls) {
                if (url.isLocalFile()) {
                    ui->lineEdit_filePath->setText(QDir::toNativeSeparators(url.toLocalFile()));
                    dropEvent->acceptProposedAction();
                    return true;
                }
            }
            return true;
        }
    }

    // 其他事件交给基类处理
    return QMainWindow::eventFilter(watched, event);
}
