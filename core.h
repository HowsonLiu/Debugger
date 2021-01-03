#ifndef CORE_H
#define CORE_H
#include <windows.h>
#include <QString>
#include <QThread>

#define DBGR_EVENT_CONTINUE 0
#define DBGR_EVENT_BREAK 1

class DebuggerThread : public QThread {
	Q_OBJECT
protected:
	virtual void run() override;

signals:
	void sigCreateProcessFailed();
	void sigAttachProcessFailed();
	void sig_CREATE_PROCESS_DEBUG_EVENT(unsigned long pid, unsigned long tid, void* hProcess);
	void sig_EXCEPTION_BREAKPOINT(unsigned long pid, unsigned long tid, void* hProcess, void* hThread);
	void sig_EXIT_PROCESS_DEBUG_EVENT(unsigned long pid, unsigned long tid, unsigned long exitCode);

public slots:
	void onContinue(unsigned long, unsigned long);
	void onBreakNow();
	void onExit();

public:
	void setCreateProcessPath(const QString& path) { m_createPath = path; };
	void setAttachProcessID(DWORD id) { m_attachID = id; };
	void clear() { m_createPath.clear(); m_attachID = 0; };

private:
	void loop();

private:
	QString m_createPath;
	DWORD m_attachID;

	bool m_bRun;
	HANDLE m_hEvent[2];

	DWORD m_dwTargetPid;
	DWORD m_dwTargetTid;
	HANDLE m_hTargetProcess;
	HANDLE m_hTargetMainThread;
	std::map<DWORD, HANDLE> m_threadId2HandleMap;
};

#endif