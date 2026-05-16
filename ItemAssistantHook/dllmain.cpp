#include <windows.h>
#include <detours.h>
#include <stdio.h>
#include <stdlib.h>
#include <set>
#include <memory>    // Replaces boost/smart_ptr.hpp
#include <mutex>     // Replaces boost/thread.hpp
#include <string>
#include <sstream>
#include <process.h>  // For _beginthread
#include "DataQueue.h"

// --- Logging Helper ---
#include "HookLog.h"
HookLog g_log;
#define LOG(streamdef) \
{ \
    std::ostringstream ss; \
    ss << streamdef; \
    std::string msg = ss.str(); \
    g_log.out(msg); \
    msg += "\n"; \
    OutputDebugStringA(msg.c_str()); \
}

// --- Constants and Enums ---
enum MessageType {
	TYPE_INCOMING = 1,
	TYPE_OUTGOIING = 2,
};

// --- Function Pointer Definitions ---
typedef void Message_t;

// Incoming: ?DataBlockToMessage@@YAPAVMessage_t@@IPAX@Z (__cdecl)
typedef Message_t* (__cdecl *DataBlockToMessage_t)(int _Size, void* _pDataBlock);
DataBlockToMessage_t pOriginalDataBlockToMessage = nullptr;

// Outgoing: ?Send@Connection_t@@QAEHIIPBX@Z (__thiscall)
typedef int(__thiscall *OriginalSend_t)(void* pThis, unsigned int len, unsigned short arg2, void const* pData);
OriginalSend_t pOriginalSend = nullptr;

// --- Global Variables ---
HANDLE g_hEvent = NULL;
DataQueue g_dataQueue;
HWND g_targetWnd = NULL;
std::set<unsigned int> g_messageFilter;
DWORD g_lastThreadTick = 0;

// --- Background Worker Thread ---
void WorkerThreadMethod(void*)
{
	while ((g_hEvent != NULL) && (WaitForSingleObject(g_hEvent, INFINITE) == WAIT_OBJECT_0))
	{
		if (g_hEvent == NULL) break;

		DWORD tick = GetTickCount();
		if ((tick - g_lastThreadTick > 10000) || (g_targetWnd == NULL)) {
			g_targetWnd = FindWindowA("ItemAssistantWindowClass", NULL);
			g_lastThreadTick = GetTickCount();
		}

		while (!g_dataQueue.empty())
		{
			DataItemPtr item = g_dataQueue.pop();
			if (item == nullptr || g_targetWnd == NULL) continue;

			COPYDATASTRUCT data;
			data.dwData = item->type();
			data.lpData = item->data();
			data.cbData = item->size();

			SendMessageA(g_targetWnd, WM_COPYDATA, 0, (LPARAM)&data);
		}
	}
}

void StartWorkerThread() {
	g_hEvent = CreateEventA(NULL, FALSE, FALSE, "IA_Worker");
	_beginthread(WorkerThreadMethod, 0, 0);
}

void EndWorkerThread() {
	if (g_hEvent) {
		SetEvent(g_hEvent);
		CloseHandle(g_hEvent);
		g_hEvent = NULL;
	}
}

// --- Hook Functions ---

// Incoming Message Hook
Message_t* DataBlockToMessageHook(int _Size, void* _pDataBlock)
{
	// Capture data using std::shared_ptr
	DataItemPtr item = std::make_shared<DataItem>(TYPE_INCOMING, _Size, (char*)_pDataBlock);
	g_dataQueue.push(item);
	if (g_hEvent) SetEvent(g_hEvent);

	return pOriginalDataBlockToMessage(_Size, _pDataBlock);
}

// Outgoing Message Hook (__fastcall captures ECX as 'pThis' for the __thiscall function)
int __fastcall SendConnectionHook(void* pThis, void* edx_dummy, unsigned int len, unsigned short arg2, void const* pData)
{
	// Capture data
	DataItemPtr item = std::make_shared<DataItem>(TYPE_OUTGOIING, len, (char*)pData);
	g_dataQueue.push(item);
	if (g_hEvent) SetEvent(g_hEvent);

	// Call original function via the trampoline pointer
	return pOriginalSend(pThis, len, arg2, pData);
}

// --- Process Attach / Detach ---

int ProcessAttach(HINSTANCE _hModule)
{
	// 1. Resolve Addresses
	pOriginalDataBlockToMessage = (DataBlockToMessage_t)::GetProcAddress(
		::GetModuleHandleA("MessageProtocol.dll"),
		"?DataBlockToMessage@@YAPAVMessage_t@@IPAX@Z");

	PVOID targetSendAddr = (PVOID)::GetProcAddress(
		::GetModuleHandleA("Connection.dll"),
		"?Send@Connection_t@@QAEHIIPBX@Z");

	// 2. Perform Hooks
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	if (pOriginalDataBlockToMessage) {
		DetourAttach(&(PVOID&)pOriginalDataBlockToMessage, DataBlockToMessageHook);
	}

	if (targetSendAddr) {
		// Detours updates targetSendAddr to point to the trampoline
		DetourAttach(&targetSendAddr, (PVOID)SendConnectionHook);
	}

	if (DetourTransactionCommit() == NO_ERROR) {
		// Save the trampoline for SendConnection
		pOriginalSend = (OriginalSend_t)targetSendAddr;
		LOG("IA_HOOK: Hooks applied successfully.");
	}

	StartWorkerThread();
	return TRUE;
}

int ProcessDetach(HINSTANCE _hModule)
{
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	if (pOriginalDataBlockToMessage) {
		DetourDetach(&(PVOID&)pOriginalDataBlockToMessage, DataBlockToMessageHook);
	}

	if (pOriginalSend) {
		DetourDetach(&(PVOID&)pOriginalSend, (PVOID)SendConnectionHook);
	}

	DetourTransactionCommit();
	EndWorkerThread();
	return TRUE;
}

BOOL APIENTRY DllMain(HINSTANCE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		return ProcessAttach(hModule);
	case DLL_PROCESS_DETACH:
		return ProcessDetach(hModule);
	}
	return TRUE;
}
