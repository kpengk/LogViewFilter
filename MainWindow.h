#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMainWindow>
#include <QStringList>

QT_BEGIN_NAMESPACE
namespace Ui {
    class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

protected:
    // ---------------------------------------------------
    // 事件过滤器：处理 lineEdit_filePath 的拖放事件
    //
    //   DragEnter -> 只接受携带本地文件路径的拖拽
    //   Drop      -> 取第一个文件路径，填入输入框
    // ---------------------------------------------------
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    // 浏览文件
    void onBrowseFile();
    // 执行过滤
    void onFilter();
    // 另存为
    void onSave();
    // 清空结果区
    void onClearResult();
    // 清空关键词
    void onClearKeywords();

    // 从关键词编辑框解析出非空关键词列表
    QStringList parseKeywords() const;

    // 判断该行是否包含任意关键词
    static bool containsKeyword(QStringView line, const QStringList& keywords);

    // 核心过滤函数，与 log_filter.py 逻辑一致
    // contain=true  -> 仅保留包含任意关键词的行
    // contain=false -> 移除包含任意关键词的行
    static QStringList filterLog(const QString& filePath, const QStringList& keywords, bool contain);

    Ui::MainWindow* ui;

    // 上次文件对话框使用的目录
    QString lastOpenDir_;
    QString lastSaveDir_;

    // 当前过滤结果（用于另存为）
    QStringList filtered_lines_;
};

#endif // MAINWINDOW_H
