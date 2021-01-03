#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "core.h"
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QLineEdit>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    m_thread = new DebuggerThread;

	connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::onOpenButtonClicked);
	connect(ui->actionAttach, &QAction::triggered, this, &MainWindow::onAttachButtonClicked);
	connect(ui->actionExit, &QAction::triggered, this, &MainWindow::onExitButtonClicked);
	connect(ui->actionContinue, &QAction::triggered, this, &MainWindow::onContinueButtonClicked);
	connect(ui->actionBreak, &QAction::triggered, this, &MainWindow::onBreakButtonClicked);

	connect(this, &MainWindow::sigContinue, m_thread, &DebuggerThread::onContinue);
	connect(this, &MainWindow::sigBreak, m_thread, &DebuggerThread::onBreakNow);

	connect(m_thread, &DebuggerThread::sigCreateProcessFailed, this, &MainWindow::onCreateProcessFailed);
	connect(m_thread, &DebuggerThread::sigAttachProcessFailed, this, &MainWindow::onAttachProcessFailed);
	connect(m_thread, &DebuggerThread::sig_CREATE_PROCESS_DEBUG_EVENT, this, &MainWindow::on_CREATE_PROCESS_DEBUG_EVENT);
	connect(m_thread, &DebuggerThread::sig_EXCEPTION_BREAKPOINT, this, &MainWindow::on_EXCEPTION_BREAKPOINT);
	connect(m_thread, &DebuggerThread::sig_EXIT_PROCESS_DEBUG_EVENT, this, &MainWindow::on_EXIT_PROCESS_DEBUG_EVENT);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onOpenButtonClicked()
{
	static QString lastPath;
    lastPath = "D:\\code\\self\\LAssembly\\LAssembly\\Debug";
	lastPath = "C:\\windows\\system32";
    QString filePath = QFileDialog::getOpenFileName(this, "Open", lastPath, "Exe(*.exe)");
    lastPath = QFileInfo(filePath).absoluteFilePath();

    m_thread->clear();
    m_thread->setCreateProcessPath(filePath);

    m_thread->start();
}

void MainWindow::onAttachButtonClicked()
{

}

void MainWindow::onExitButtonClicked()
{
    m_thread->wait();
}

void MainWindow::onContinueButtonClicked()
{
	emit sigContinue(m_pid, m_tid);
	uiRunning();
}

void MainWindow::onBreakButtonClicked()
{
	emit sigBreak();
}

void MainWindow::onCreateProcessFailed()
{
	QMessageBox::warning(this, "Warning", "Create process failed", QMessageBox::Yes);
}

void MainWindow::onAttachProcessFailed()
{
	QMessageBox::warning(this, "Warning", "Attach process failed", QMessageBox::Yes);
}

void MainWindow::on_CREATE_PROCESS_DEBUG_EVENT(unsigned long pid, unsigned long tid, void* hProcess)
{
	m_pid = pid;
	m_tid = tid;
	m_hProcess = hProcess;
	update();
}

void MainWindow::on_EXCEPTION_BREAKPOINT(unsigned long pid, unsigned long tid, void* hProcess, void* hThread)
{
	m_pid = pid;
	m_tid = tid;
	m_hProcess = hProcess;
	m_hThread = hThread;
	update();
	uiBreak();
}

void MainWindow::on_EXIT_PROCESS_DEBUG_EVENT(unsigned long pid, unsigned long tid, unsigned long exitCode)
{
	QString text = QString("Process {0} exit with code: {1}").arg(QString::number(pid)).arg(QString::number(exitCode));
	QMessageBox::information(this, "Exit", text, QMessageBox::Yes);
}

void MainWindow::uiBreak()
{
	QPalette palette(this->palette());
	palette.setColor(QPalette::Background, Qt::red);
	this->setPalette(palette);
}

void MainWindow::uiRunning()
{
	QPalette palette(this->palette());
	palette.setColor(QPalette::Background, Qt::green);
	this->setPalette(palette);
}

void MainWindow::updateContext()
{
	if (m_hThread == NULL) return;
	CONTEXT ctxt;
	SuspendThread(m_hThread);
	GetThreadContext(m_hThread, &ctxt);
	ResumeThread(m_hThread);

	ui->eaxE->setText(QString::number(ctxt.Eax, 16));
	ui->ebxE->setText(QString::number(ctxt.Ebx, 16));
	ui->ecxE->setText(QString::number(ctxt.Ecx, 16));
	ui->edxE->setText(QString::number(ctxt.Edx, 16));
	ui->ediE->setText(QString::number(ctxt.Edi, 16));
	ui->esiE->setText(QString::number(ctxt.Esi, 16));
	ui->ebpE->setText(QString::number(ctxt.Ebp, 16));
	ui->espE->setText(QString::number(ctxt.Esp, 16));
	ui->csE->setText(QString::number(ctxt.SegCs, 16));
	ui->flagsE->setText(QString::number(ctxt.EFlags, 16));
	ui->eipE->setText(QString::number(ctxt.Eip, 16));
	ui->ssE->setText(QString::number(ctxt.SegSs, 16));
}

void MainWindow::update()
{
	ui->pidE->setText(QString::number(m_pid));
	ui->tidE->setText(QString::number(m_tid));
	updateContext();
}
