#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "windows.h"

namespace Ui {
class MainWindow;
}

class DebuggerThread;
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

signals:
    void sigContinue(unsigned long, unsigned long);
    void sigBreak();

private slots:
    void onOpenButtonClicked();
    void onAttachButtonClicked();
    void onExitButtonClicked();
    void onContinueButtonClicked();
    void onBreakButtonClicked();

    void onCreateProcessFailed();
    void onAttachProcessFailed();
    void on_CREATE_PROCESS_DEBUG_EVENT(unsigned long pid, unsigned long tid, void* hProcess);
    void on_EXCEPTION_BREAKPOINT(unsigned long pid, unsigned long tid, void* hProcess, void* hThread);
    void on_EXIT_PROCESS_DEBUG_EVENT(unsigned long pid, unsigned long tid, unsigned long exitCode);

private:
    void uiBreak();
    void uiRunning();
    void updateContext();
    void update();

private:
    Ui::MainWindow *ui;
    DebuggerThread* m_thread;

    // current view info
    DWORD m_pid = 0;
    DWORD m_tid = 0;
    HANDLE m_hProcess = NULL;
    HANDLE m_hThread = NULL;
};

#endif // MAINWINDOW_H
