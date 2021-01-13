#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "core.h"
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QLineEdit>
#include <QStandardItemModel>
#include <QDebug>

namespace {
	const int kColNum = 16;
	const int kRowNum = 16;
}

QString toHexString(uint i)
{
	return "0x" + QString::number(i, 16);
}

uint fromHexString(QString str)
{
	return str.right(str.size() - 2).toInt(nullptr, 16);
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    m_thread = new DebuggerThread;
	m_model = new QStandardItemModel(this);
	m_model->setColumnCount(kColNum);
	m_model->setRowCount(kRowNum);
	for (int i = 0; i < kColNum; ++i)
		m_model->setHeaderData(i, Qt::Horizontal, toHexString(i));
	m_memory = new uchar[kRowNum * kColNum];

	ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	ui->tableView->verticalHeader()->sectionResizeMode(QHeaderView::Stretch);
	ui->tableView->setAlternatingRowColors(true);

	connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::onOpenButtonClicked);
	connect(ui->actionAttach, &QAction::triggered, this, &MainWindow::onAttachButtonClicked);
	connect(ui->actionExit, &QAction::triggered, this, &MainWindow::onExitButtonClicked);
	connect(ui->actionContinue, &QAction::triggered, this, &MainWindow::onContinueButtonClicked);
	connect(ui->actionBreak, &QAction::triggered, this, &MainWindow::onBreakButtonClicked);
	connect(ui->addr, &QLineEdit::returnPressed, this, &MainWindow::onGoToAddress);
	connect(ui->go, &QPushButton::clicked, this, &MainWindow::onGoToAddress);
	connect(ui->apply, &QPushButton::clicked, this, &MainWindow::onApplyContext);

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
	delete m_memory;
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
	m_running = true;
	uiRunning();
}

void MainWindow::onBreakButtonClicked()
{
	emit sigBreak();
}

void MainWindow::onGoToAddress()
{
	updateMemory();
}

void MainWindow::onApplyContext()
{
	applyContext();
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
	m_running = false;
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

	SuspendThread(m_hThread);
	GetThreadContext(m_hThread, &m_context);
	ResumeThread(m_hThread);

	ui->eaxE->setText(toHexString(m_context.Eax));
	ui->ebxE->setText(toHexString(m_context.Ebx));
	ui->ecxE->setText(toHexString(m_context.Ecx));
	ui->edxE->setText(toHexString(m_context.Edx));
	ui->ediE->setText(toHexString(m_context.Edi));
	ui->esiE->setText(toHexString(m_context.Esi));
	ui->ebpE->setText(toHexString(m_context.Ebp));
	ui->espE->setText(toHexString(m_context.Esp));
	ui->csE->setText(toHexString(m_context.SegCs));
	ui->flagsE->setText(toHexString(m_context.EFlags));
	ui->eipE->setText(toHexString(m_context.Eip));
	ui->addr->setText(toHexString(m_context.Eip));
	ui->ssE->setText(toHexString(m_context.SegSs));
}

void MainWindow::applyContext()
{
	if (m_hThread == NULL) return;

	m_context.Eax = fromHexString(ui->eaxE->text());
	m_context.Ebx = fromHexString(ui->ebxE->text());
	m_context.Ecx = fromHexString(ui->ecxE->text());
	m_context.Edx = fromHexString(ui->edxE->text());
	m_context.Edi = fromHexString(ui->ediE->text());
	m_context.Esi = fromHexString(ui->esiE->text());
	m_context.Ebp = fromHexString(ui->ebpE->text());
	m_context.Esp = fromHexString(ui->espE->text());
	m_context.SegCs = fromHexString(ui->csE->text());
	m_context.EFlags = fromHexString(ui->flagsE->text());
	m_context.Eip = fromHexString(ui->eipE->text());
	m_context.SegSs = fromHexString(ui->ssE->text());

	SuspendThread(m_hThread);
	SetThreadContext(m_hThread, &m_context);
	ResumeThread(m_hThread);
}

void MainWindow::updateMemory()
{
	LPVOID addr = (LPVOID)fromHexString(ui->addr->text());
	SuspendThread(m_hThread);
	DWORD dwOldAttr;
	SIZE_T bytesRead;
	VirtualProtect((LPVOID)addr, kColNum * kRowNum, PAGE_EXECUTE_READWRITE, &dwOldAttr);
	BOOL res = ReadProcessMemory(m_hProcess, (LPVOID)addr, m_memory, kColNum * kRowNum, &bytesRead);
	VirtualProtect((LPVOID)addr, kColNum * kRowNum, dwOldAttr, &dwOldAttr);
	ResumeThread(m_hThread);

	if (!res || bytesRead == 0) {
		console("ReadProcessMemory Failed: " + QString::number(GetLastError()));
		return;
	}

	// set header
	for (int i = 0; i < kRowNum; ++i)
		m_model->setHeaderData(i, Qt::Vertical, toHexString(i * kColNum + (int)addr));

	// set content
	for (int i = 0; i < bytesRead; ++i) {
		int row = i / kColNum;
		int col = i % kColNum;
		m_model->setItem(row, col, new QStandardItem(toHexString(m_memory[i])));
	}

	ui->tableView->setModel(m_model);
	ui->tableView->show();
}

void MainWindow::update()
{
	ui->pidE->setText(QString::number(m_pid));
	ui->tidE->setText(QString::number(m_tid));
	updateContext();
	updateMemory();
}

void MainWindow::console(const QString& log)
{
	ui->console->setText("Console: " + log);
}
