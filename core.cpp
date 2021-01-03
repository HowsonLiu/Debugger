#include "core.h"
#include <QDir>
#include <QDebug>

void DebuggerThread::run()
{
	if (m_createPath.isEmpty() && m_attachID == 0) return;

	if (!m_createPath.isEmpty()) {
		m_createPath = QDir::toNativeSeparators(m_createPath);

		STARTUPINFO si = { 0 };
		PROCESS_INFORMATION pi = { 0 };
		auto res = CreateProcess(m_createPath.toStdWString().c_str(), NULL, 0, 0, FALSE, DEBUG_ONLY_THIS_PROCESS, 0, 0, &si, &pi);
		if (res == 0) {
			qDebug() << "Create process failed: last error code " << GetLastError();
			emit sigCreateProcessFailed();
			return;
		}
	}
	else {
		auto res = DebugActiveProcess(m_attachID);
		if (!res) {
			emit sigAttachProcessFailed();
			return;
		}
	}

	m_hEvent[DBGR_EVENT_CONTINUE] = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hEvent[DBGR_EVENT_BREAK] = CreateEvent(NULL, FALSE, FALSE, NULL);
	loop();
}

void DebuggerThread::onContinue(unsigned long pid, unsigned long tid)
{
	m_dwTargetTid = tid;
	m_dwTargetPid = pid;
	SetEvent(m_hEvent[DBGR_EVENT_CONTINUE]);
}

void DebuggerThread::onBreakNow()
{
	SetEvent(m_hEvent[DBGR_EVENT_BREAK]);
}

void DebuggerThread::onExit()
{

}

void DebuggerThread::loop()
{
	m_bRun = true;
	DEBUG_EVENT dbgEvent;
	int dbgrEventIndex;
	while (m_bRun) {
		if (WaitForDebugEvent(&dbgEvent, 150)) {
			switch (dbgEvent.dwDebugEventCode) {
			case CREATE_PROCESS_DEBUG_EVENT: {
				CREATE_PROCESS_DEBUG_INFO info = dbgEvent.u.CreateProcessInfo;
				m_hTargetProcess = info.hProcess;
				m_hTargetMainThread = info.hThread;
				m_threadId2HandleMap[dbgEvent.dwThreadId] = info.hThread;
				emit sig_CREATE_PROCESS_DEBUG_EVENT(dbgEvent.dwProcessId, dbgEvent.dwThreadId, info.hProcess);
				break;
			}
			case EXIT_PROCESS_DEBUG_EVENT: {
				m_bRun = false;
				EXIT_PROCESS_DEBUG_INFO info = dbgEvent.u.ExitProcess;
				emit sig_EXIT_PROCESS_DEBUG_EVENT(dbgEvent.dwProcessId, dbgEvent.dwThreadId, info.dwExitCode);
				break;
			}
			case CREATE_THREAD_DEBUG_EVENT: {
				CREATE_THREAD_DEBUG_INFO info = dbgEvent.u.CreateThread;
				m_threadId2HandleMap[dbgEvent.dwThreadId] = info.hThread;
				break;
			}
			case EXIT_THREAD_DEBUG_EVENT: {
				EXIT_THREAD_DEBUG_INFO info = dbgEvent.u.ExitThread;
				m_threadId2HandleMap.erase(dbgEvent.dwThreadId);
				break;
			}
			case EXCEPTION_DEBUG_EVENT: {
				EXCEPTION_DEBUG_INFO info = dbgEvent.u.Exception;
				emit sig_EXCEPTION_BREAKPOINT(dbgEvent.dwProcessId, dbgEvent.dwThreadId, m_hTargetProcess, m_threadId2HandleMap[dbgEvent.dwThreadId]);
				break;
			}
			}
			if (m_bRun && dbgEvent.dwDebugEventCode != EXCEPTION_DEBUG_EVENT)
				ContinueDebugEvent(dbgEvent.dwProcessId, dbgEvent.dwThreadId, DBG_CONTINUE);
		}

		if ((dbgrEventIndex = WaitForMultipleObjects(2, m_hEvent, FALSE, 150)) != WAIT_TIMEOUT) {
			if(dbgrEventIndex == WAIT_OBJECT_0 + DBGR_EVENT_CONTINUE)
				ContinueDebugEvent(m_dwTargetPid, m_dwTargetTid, DBG_CONTINUE);
			else if (dbgrEventIndex == WAIT_OBJECT_0 + DBGR_EVENT_BREAK)
				DebugBreakProcess(m_hTargetProcess);
		}
	}
}
