#include "VRSpeechListener.h"

#include <windows.h>
#include <sphelper.h>
#include <sapi.h>
#include <comdef.h>

#include <iostream>
#include <sstream>
#include <string>


VRSpeechListener VRSpeechListener::m_speechListener;

/*
void __stdcall SAPICallback(WPARAM wParam, LPARAM lParam)
{
	int kfj = 0;
}

namespace
{
	bool						g_sapiIsInitialized{ false };
	CComPtr<ISpRecognizer>		g_sapiEngine;
	CComPtr<ISpRecoContext>		g_sapiRecoCtx;
	CComPtr<ISpRecoGrammar>		g_sapiRecoGrammar;
}
*/

VRSpeechListener::VRSpeechListener()
{
	// InitSAPI();
}

VRSpeechListener::~VRSpeechListener()
{
	CleanupSAPI();
}

/*
void VRSpeechListener::InitSAPI() const
{
	if (g_sapiIsInitialized)
		return;

	if (FAILED(CoInitialize(NULL)))
	{
		throw std::string("Unable to initialise COM objects");
	}

	ULONGLONG ullGramId = 1;
	HRESULT hr = g_sapiEngine.CoCreateInstance(CLSID_SpSharedRecognizer);
	if (FAILED(hr))
	{
		throw std::string("Unable to create recognition engine");
	}

	hr = g_sapiEngine->CreateRecoContext(&g_sapiRecoCtx);
	if (FAILED(hr))
	{
		throw std::string("Failed command recognition");
	}

	hr = g_sapiRecoCtx->SetNotifyCallbackFunction(SAPICallback, NULL, NULL);
	if (FAILED(hr))
	{
		throw std::string("Unable to select notification window");
	}

	const ULONGLONG ullInterest = SPFEI(SPEI_SOUND_START) | SPFEI(SPEI_SOUND_END) |
		SPFEI(SPEI_PHRASE_START) | SPFEI(SPEI_RECOGNITION) |
		SPFEI(SPEI_FALSE_RECOGNITION) | SPFEI(SPEI_HYPOTHESIS) |
		SPFEI(SPEI_INTERFERENCE) | SPFEI(SPEI_RECO_OTHER_CONTEXT) |
		SPFEI(SPEI_REQUEST_UI) | SPFEI(SPEI_RECO_STATE_CHANGE) |
		SPFEI(SPEI_PROPERTY_NUM_CHANGE) | SPFEI(SPEI_PROPERTY_STRING_CHANGE);
	hr = g_sapiRecoCtx->SetInterest(ullInterest, ullInterest);
	if (FAILED(hr))
	{
		throw std::string("Failed to create interest");
	}

	hr = g_sapiRecoCtx->CreateGrammar(ullGramId, &g_sapiRecoGrammar);
	if (FAILED(hr))
	{
		throw std::string("Unable to create grammar");
	}

	hr = g_sapiRecoGrammar->LoadDictation(NULL, SPLO_STATIC);
	if (FAILED(hr))
	{
		throw std::string("Failed to load dictation");
	}

	hr = g_sapiRecoGrammar->SetDictationState(SPRS_ACTIVE);
	if (FAILED(hr))
	{
		throw std::string("Failed setting dictation state");
	}

	g_sapiIsInitialized = true;
}

void VRSpeechListener::CleanupSAPI() const
{
	if (!g_sapiIsInitialized)
		return;

	if (g_sapiRecoGrammar)
	{
		g_sapiRecoGrammar.Release();
	}
	if (g_sapiRecoCtx)
	{
		g_sapiRecoCtx->SetNotifySink(NULL);
		g_sapiRecoCtx.Release();
	}
	if (g_sapiEngine)
	{
		g_sapiEngine.Release();
	}
	CoUninitialize();
	g_sapiIsInitialized = false;
}

std::string VRSpeechListener::ExtractInput(const CSpEvent& event) const
{
	std::string result;
	CComPtr<ISpRecoResult> cpRecoResult = event.RecoResult();

	SPPHRASE* pPhrase{ nullptr };
	HRESULT hr = cpRecoResult->GetPhrase(&pPhrase);
	if (SUCCEEDED(hr))
	{
		WCHAR* pwszText{ nullptr };
		hr = cpRecoResult->GetText(SP_GETWHOLEPHRASE, SP_GETWHOLEPHRASE, TRUE, &pwszText, NULL);
		if (SUCCEEDED(hr))
		{
			_bstr_t bstrText{ pwszText };
			result = bstrText;
			CoTaskMemFree(pwszText);
		}
		CoTaskMemFree(pPhrase);
	}
	return result;
}

VRSpeechListener::Command VRSpeechListener::GetCommand() const
{
	if (!g_sapiIsInitialized)
	{
		InitSAPI();
	}

	Command cmd{ Command::NONE };

	std::stringstream text;

	CSpEvent event;
	while (event.GetFrom(g_sapiRecoCtx) == S_OK)
	{
		if (event.eEventId == SPEI_RECOGNITION)
		{
			text << ExtractInput(event) << " ";
		}
	}

	return cmd;
}
*/

const VRSpeechListener& VRSpeechListener::Instance()
{
	return m_speechListener;
}

/*
void check_result(const HRESULT& result);

void get_text(ISpRecoContext* reco_context)
{
	const ULONG maxEvents = 10;
	SPEVENT events[maxEvents];

	ULONG eventCount;
	HRESULT hr;
	hr = reco_context->GetEvents(maxEvents, events, &eventCount);

	// Warning hr equal S_FALSE if everything is OK 
	// but eventCount < requestedEventCount
	if (!(hr == S_OK || hr == S_FALSE)) {
		check_result(hr);
	}

	ISpRecoResult* recoResult;
	recoResult = reinterpret_cast<ISpRecoResult*>(events[0].lParam);

	wchar_t* text;
	hr = recoResult->GetText(SP_GETWHOLEPHRASE, SP_GETWHOLEPHRASE, FALSE, &text, NULL);
	check_result(hr);

	CoTaskMemFree(text);
}
*/

VRSpeechListener::Command VRSpeechListener::GetCommand() const
{
	//InitSAPI();
	return Command::NONE;
}

void VRSpeechListener::InitSAPI() const
{
	/*
	if (FAILED(::CoInitialize(nullptr)))
	{
		throw std::string("Unable to initialise COM objects");
	}

	ISpRecognizer* recognizer{ nullptr };
	HRESULT hr = CoCreateInstance(CLSID_SpSharedRecognizer, nullptr, CLSCTX_ALL, IID_ISpRecognizer, reinterpret_cast<void**>(&recognizer));
	check_result(hr);

	ISpRecoContext* recoContext{ nullptr };
	hr = recognizer->CreateRecoContext(&recoContext);
	check_result(hr);

	hr = recoContext->Pause(0);
	check_result(hr);

	ISpRecoGrammar* recoGrammar = InitGrammar(recoContext);

	hr = recoContext->SetNotifyWin32Event();
	check_result(hr);

	HANDLE handleEvent;
	handleEvent = recoContext->GetNotifyEventHandle();
	if (handleEvent == INVALID_HANDLE_VALUE)
	{
		check_result(E_FAIL);
	}

	ULONGLONG interest;
	interest = SPFEI(SPEI_RECOGNITION);
	hr = recoContext->SetInterest(interest, interest);
	check_result(hr);

	hr = recoGrammar->SetRuleState(m_ruleName1, 0, SPRS_ACTIVE);
	check_result(hr);

	hr = recoContext->Resume(0);
	check_result(hr);

	HANDLE handles[1];
	handles[0] = handleEvent;
	WaitForMultipleObjects(1, handles, FALSE, INFINITE);
	get_text(recoContext);

	recoGrammar->Release();
	CoUninitialize();
	*/
}

void VRSpeechListener::CleanupSAPI() const
{
}

ISpRecoGrammar* VRSpeechListener::InitGrammar(ISpRecoContext* recoContext) const
{
	/*
	HRESULT hr;
	SPSTATEHANDLE sate;

	ISpRecoGrammar* recoGrammar;
	hr = recoContext->CreateGrammar(m_grammarId, &recoGrammar);
	check_result(hr);

	WORD langId = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_UK);
	hr = recoGrammar->ResetGrammar(langId);
	check_result(hr);
	// TODO: Catch error and use default langId => GetUserDefaultUILanguage()

	// Create rules
	hr = recoGrammar->GetRule(m_ruleName1, 0, SPRAF_TopLevel | SPRAF_Active, true, &sate);
	check_result(hr);

	// Add a word
	hr = recoGrammar->AddWordTransition(sate, NULL, L"follow", L" ", SPWT_LEXICAL, 1, nullptr);
	check_result(hr);

	// Commit changes
	hr = recoGrammar->Commit(0);
	check_result(hr);

	return recoGrammar;
	*/
	return nullptr;
}

/*
void check_result(const HRESULT& result)
{
	if (result == S_OK) {
		return;
	}

	std::string message;

	switch (result) {

	case E_INVALIDARG:
		message = "One or more arguments are invalids.";

	case E_ACCESSDENIED:
		message = "Acces Denied.";

	case E_NOINTERFACE:
		message = "Interface does not exist.";

	case E_NOTIMPL:
		message = "Not implemented method.";

	case E_OUTOFMEMORY:
		message = "Out of memory.";

	case E_POINTER:
		message = "Invalid pointer.";

	case E_UNEXPECTED:
		message = "Unexpecter error.";

	case E_FAIL:
		message = "Failure";

	default:
		message = "Unknown : " + std::to_string(result);
	}

	throw std::exception(message.c_str());
}
*/

