#include "VRSpeechListener.h"

#include <windows.h>
#include <sperror.h>
#include <sapi.h>
#include <comdef.h>

#include <iostream>
#include <sstream>
#include <string>
#include <regex>

#include "hud.h"
#include "cl_util.h"

VRSpeechListener VRSpeechListener::m_speechListener;


VRSpeechListener::VRSpeechListener()
{
}

VRSpeechListener::~VRSpeechListener()
{
	CleanupSAPI();
}

VRSpeechListener& VRSpeechListener::Instance()
{
	return m_speechListener;
}


VRSpeechCommand VRSpeechListener::GetCommand()
{
	if (CVAR_GET_FLOAT("vr_speech_commands_enabled") == 0.f)
		return VRSpeechCommand::NONE;

	if (!m_isInitialized)
		InitSAPI();

	// check again, in case InitSAPI failed
	if (!m_isInitialized || CVAR_GET_FLOAT("vr_speech_commands_enabled") == 0.f)
		return VRSpeechCommand::NONE;

	// reinit if command strings changed
	if (m_commandsHaveChanged)
	{
		m_commandsHaveChanged = false;

		CleanupSAPI();
		InitSAPI();

		// check again, in case InitSAPI failed
		if (!m_isInitialized || CVAR_GET_FLOAT("vr_speech_commands_enabled") == 0.f)
			return VRSpeechCommand::NONE;
	}

	if (WAIT_OBJECT_0 == WaitForSingleObjectEx(m_eventHandle, 0, FALSE))
	{
		try
		{
			SPEVENT event{ 0 };
			ULONG eventCount{ 0 };
			HRESULT hr = m_context->GetEvents(1, &event, &eventCount);
			if (!(hr == S_OK || hr == S_FALSE))
			{
				VerifyResult(hr, "Couldn't get recognizer events");
			}

			ISpRecoResult* result = reinterpret_cast<ISpRecoResult*>(event.lParam);

			wchar_t* phraseBuffer{ nullptr };
			VerifyResult(result->GetText(SP_GETWHOLEPHRASE, SP_GETWHOLEPHRASE, FALSE, &phraseBuffer, nullptr), "Couldn't get recognized phrase text");
			if (phraseBuffer)
			{
				std::wstring phrase{ phraseBuffer };
				CoTaskMemFree(phraseBuffer);

				for (const auto& [command, commandPhrases] : m_commands)
				{
					if (commandPhrases.find(phrase) != commandPhrases.end())
					{
						return command;
					}
				}
			}

			return VRSpeechCommand::MUMBLE;
		}
		catch (std::string error)
		{
			gEngfuncs.Con_DPrintf("Failed to handle speech recognition: %s\n", error.data());
			gEngfuncs.Cvar_SetValue("vr_speech_commands_enabled", 0.f);
			CleanupSAPI();
		}
	}

	return VRSpeechCommand::NONE;
}

void VRSpeechListener::ExportCommandStrings(std::vector<std::string>& settingslines)
{
	settingslines.push_back("vr_speech_commands_follow=" + m_followcommandstring);
	settingslines.push_back("vr_speech_commands_wait=" + m_waitcommandstring);
	settingslines.push_back("vr_speech_commands_hello=" + m_hellocommandstring);
}

void VRSpeechListener::RegisterCommandString(const std::string& name, const std::string& commands)
{
	VRSpeechCommand command;
	if (name == "vr_speech_commands_follow")
	{
		if (m_followcommandstring == commands)
			return;

		m_followcommandstring = commands;
		command = VRSpeechCommand::FOLLOW;
	}
	else if (name == "vr_speech_commands_wait")
	{
		if (m_waitcommandstring == commands)
			return;

		m_waitcommandstring = commands;
		command = VRSpeechCommand::WAIT;
	}
	else if (name == "vr_speech_commands_hello")
	{
		if (m_hellocommandstring == commands)
			return;

		m_hellocommandstring = commands;
		command = VRSpeechCommand::HELLO;
	}
	else
	{
		return;
	}

	m_commands[command] = GetCommandsFromCommandString(command, commands);
	m_commandsHaveChanged = true;
}

const char* CommandTypeToString(VRSpeechCommand commandtype)
{
	switch (commandtype)
	{
	case VRSpeechCommand::FOLLOW: return "FOLLOW";
	case VRSpeechCommand::WAIT: return "WAIT";
	case VRSpeechCommand::HELLO: return "HELLO";
	case VRSpeechCommand::MUMBLE: return "MUMBLE";
	case VRSpeechCommand::NONE: return "NONE";
	default: return "<INVALID>";
	}
}

std::unordered_set<std::wstring> VRSpeechListener::GetCommandsFromCommandString(VRSpeechCommand commandtype, const std::string& commandstring)
{
	std::unordered_set<std::wstring> result;

	std::istringstream commandstringstream{ commandstring };
	std::string command;
	int index = 0;
	while (getline(commandstringstream, command, ','))
	{
		command = std::regex_replace(command, std::regex{ "^[\\s]*(.*)[\\s]*$" }, "$1");
		int requiredsize = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, command.data(), -1, NULL, 0);
		if (requiredsize > 0)
		{
			std::vector<wchar_t> wcommand;
			wcommand.resize(requiredsize * 4 + 1024);	// paranoia
			MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, command.data(), -1, wcommand.data(), requiredsize);
			result.insert(std::wstring{ wcommand.data() });
		}
		else
		{
			gEngfuncs.Con_DPrintf("Invalid speech command for type %s at index %i\n", CommandTypeToString(commandtype), index);
		}
		index++;
	}

	return result;
}

void VRSpeechListener::InitSAPI()
{
	if (CVAR_GET_FLOAT("vr_speech_commands_enabled") == 0.f)
	{
		m_isInitialized = false;
		return;
	}

	try
	{
		if (FAILED(::CoInitialize(nullptr)))
		{
			throw std::string{ "Unable to initialise COM objects" };
		}

		VerifyResult(CoCreateInstance(CLSID_SpSharedRecognizer, nullptr, CLSCTX_ALL, IID_ISpRecognizer, reinterpret_cast<void**>(&m_recognizer)), "Failed to create speech recognizer instance");

		VerifyResult(m_recognizer->CreateRecoContext(&m_context), "Failed to create speech recognition context");

		VerifyResult(m_context->Pause(0), "Failed to pazuse speech recognition for further initialization");

		VerifyResult(m_context->SetNotifyWin32Event(), "Failed to set speech recognition notify event");

		m_eventHandle = m_context->GetNotifyEventHandle();
		if (m_eventHandle == INVALID_HANDLE_VALUE)
		{
			throw std::string{ "Unable to get speech recognizer notify event handle" };
		}

		ULONGLONG interest = SPFEI(SPEI_RECOGNITION);
		VerifyResult(m_context->SetInterest(interest, interest), "Failed to set speech recognition event 'interest'");

		m_grammar = InitGrammar(m_context);

		VerifyResult(m_context->Resume(0), "Failed to start speech recognition");

		m_isInitialized = true;
	}
	catch (std::string error)
	{
		gEngfuncs.Con_DPrintf("Failed to initialize speech recognition: %s\n", error.data());
		gEngfuncs.Cvar_SetValue("vr_speech_commands_enabled", 0.f);
		m_isInitialized = false;
	}
}

void VRSpeechListener::CleanupSAPI()
{
	m_isInitialized = false;
	if (m_grammar)
	{
		m_grammar->Release();
	}
	m_grammar = nullptr;
	m_recognizer = nullptr;
	m_context = nullptr;
	CoUninitialize();
}

ISpRecoGrammar* VRSpeechListener::InitGrammar(ISpRecoContext* recoContext)
{
	ISpRecoGrammar* grammar;
	VerifyResult(recoContext->CreateGrammar(m_grammarId, &grammar), "Failed to create grammar");

	try
	{
		std::string hexLangId = CVAR_GET_STRING("vr_speech_language_id");
		int langId = std::stoi(hexLangId, nullptr, 16);
		VerifyResult(grammar->ResetGrammar(static_cast<WORD>(langId)), "");
	}
	catch (...)
	{
		gEngfuncs.Con_DPrintf("Warning: Couldn't initialize speech recognition with selected language. Make sure you have installed and properly setup the selected language for speech recognition in your system settings. Falling back to default language.\n");
		VerifyResult(grammar->ResetGrammar(GetUserDefaultUILanguage()), "Couldn't initialize speech recognition with default language.");
	}

	SPSTATEHANDLE rule{ 0 };
	VerifyResult(grammar->GetRule(m_ruleName, 0, SPRAF_TopLevel | SPRAF_Active, true, &rule), "Couldn't initialize new grammar rule.");

	for (const auto& [command, commandPhrases] : m_commands)
	{
		for (const auto& commandPhrase : commandPhrases)
		{
			VerifyResult(grammar->AddWordTransition(rule, nullptr, commandPhrase.data(), L" ", SPWT_LEXICAL, 1, nullptr), "Failed to register command phrase");
		}
	}

	VerifyResult(grammar->LoadDictation(nullptr, SPLO_STATIC), "Failed to load dictation");

	VerifyResult(grammar->Commit(0), "Failed to commit grammar");

	VerifyResult(grammar->SetRuleState(m_ruleName, nullptr, SPRS_ACTIVE), "Failed to activate commands");
	VerifyResult(grammar->SetDictationState(SPRS_ACTIVE), "Failed setting dictation state");

	return grammar;
}

void VRSpeechListener::VerifyResult(long result, const std::string& message)
{
	if (result == S_OK)
		return;

	std::string errorMessage;

	if (result == E_POINTER)
	{
		errorMessage = message + ": Invalid pointer";
	}
	else
	{
		LPSTR errorMessageBuffer{ nullptr };
		size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, result, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&errorMessageBuffer, 0, nullptr);
		if (size == 0)
		{
			errorMessage = message + ": " + GetSpeechRecognitionErrorMessageText(result);
		}
		else
		{
			errorMessage = message + ": " + std::string{ errorMessageBuffer, size };
		}
		LocalFree(errorMessageBuffer);
	}

	throw errorMessage;
}



#ifndef SPERR_FILEMUSTBEUNICODE
#define SPERR_FILEMUSTBEUNICODE 0x8004500a
#endif

#ifndef SP_INSUFFICIENTDATA
#define SP_INSUFFICIENTDATA 0x0004500b
#endif

#ifndef SPERR_NO_PARSE_FOUND
#define SPERR_NO_PARSE_FOUND 0x8004502d
#endif

std::string VRSpeechListener::GetSpeechRecognitionErrorMessageText(long error)
{
	switch (error)
	{
	case SPERR_UNINITIALIZED: return "The object has not been properly initialized.";
	case SPERR_ALREADY_INITIALIZED: return "The object has already been initialized.";
	case SPERR_UNSUPPORTED_FORMAT: return "The caller has specified an unsupported format.";
	case SPERR_INVALID_FLAGS: return "The caller has specified flags that are not valid for this operation.";
	case SP_END_OF_STREAM: return "The operation has reached the end of stream.";
	case SPERR_DEVICE_BUSY: return "The wave device is busy.";
	case SPERR_DEVICE_NOT_SUPPORTED: return "The wave device is not supported.";
	case SPERR_DEVICE_NOT_ENABLED: return "The wave device is not enabled.";
	case SPERR_NO_DRIVER: return "There is no wave driver installed.";
	case SPERR_FILEMUSTBEUNICODE: return "The file must be Unicode.";
	case SP_INSUFFICIENTDATA: return "SP_INSUFFICIENTDATA";
	case SPERR_INVALID_PHRASE_ID: return "The phrase ID specified does not exist or is out of range.";
	case SPERR_BUFFER_TOO_SMALL: return "The caller provided a buffer too small to return a result.";
	case SPERR_FORMAT_NOT_SPECIFIED: return "Caller did not specify a format prior to opening a stream.";
	case SPERR_AUDIO_STOPPED: return "The stream I/O was stopped by setting the audio object to the stopped state. This will be returned for both read and write streams.";
	case SP_AUDIO_PAUSED: return "This will be returned only on input (read) streams when the stream is paused. Reads on paused streams will not block, and this return code indicates that all of the data has been removed from the stream.";
	case SPERR_RULE_NOT_FOUND: return "The rule name passed to ActivateGrammar was not valid.";
	case SPERR_TTS_ENGINE_EXCEPTION: return "An exception was raised during a call to the current TTS driver.";
	case SPERR_TTS_NLP_EXCEPTION: return "An exception was raised during a call to an application sentence filter.";
	case SPERR_ENGINE_BUSY: return "In speech recognition, the current method cannot be performed while a grammar rule is active.";
	case SP_AUDIO_CONVERSION_ENABLED: return "The operation was successful, but only with automatic stream format conversion.";
	case SP_NO_HYPOTHESIS_AVAILABLE: return "There is currently no hypothesis recognition available.";
	case SPERR_CANT_CREATE: return "Cannot create a new object instance for the specified object category.";
	case SPERR_UNDEFINED_FORWARD_RULE_REF: return "A rule reference in a grammar was made to a named rule that was never defined.";
	case SPERR_EMPTY_RULE: return "A non-dynamic grammar rule that has no body.";
	case SPERR_GRAMMAR_COMPILER_INTERNAL_ERROR: return "The grammar compiler failed due to an internal state error.";
	case SPERR_RULE_NOT_DYNAMIC: return "An attempt was made to modify a non-dynamic rule.";
	case SPERR_DUPLICATE_RULE_NAME: return "A rule name was duplicated.";
	case SPERR_DUPLICATE_RESOURCE_NAME: return "A resource name was duplicated for a given rule.";
	case SPERR_TOO_MANY_GRAMMARS: return "Too many grammars have been loaded.";
	case SPERR_CIRCULAR_REFERENCE: return "Circular reference in import rules of grammars.";
	case SPERR_INVALID_IMPORT: return "A rule reference to an imported grammar that could not be resolved.";
	case SPERR_INVALID_WAV_FILE: return "The format of the WAV file is not supported.";
	case SP_REQUEST_PENDING: return "This success code indicates that an SR method called with the SPRIF_ASYNC flag is being processed. When it has finished processing, an SPFEI_ASYNC_COMPLETED event will be generated.";
	case SPERR_ALL_WORDS_OPTIONAL: return "A grammar rule was defined with a null path through the rule. That is, it is possible to satisfy the rule conditions with no words.";
	case SPERR_INSTANCE_CHANGE_INVALID: return "It is not possible to change the current engine or input. This occurs if SelectEngine is called while a recognition context exists.";
	case SPERR_RULE_NAME_ID_CONFLICT: return "A rule exists with matching IDs (names) but different names (IDs).";
	case SPERR_NO_RULES: return "A grammar contains no top-level, dynamic, or exported rules. There is no possible way to activate or otherwise use any rule in this grammar.";
	case SPERR_CIRCULAR_RULE_REF: return "Rule 'A' refers to a second rule 'B' which, in turn, refers to rule 'A'.";
	case SP_NO_PARSE_FOUND: return "Parse path cannot be parsed given the currently active rules.";
	case SPERR_NO_PARSE_FOUND: return "Parse path cannot be parsed given the currently active rules.";
	case SPERR_REMOTE_CALL_TIMED_OUT: return "A marshaled remote call failed to respond.";
	case SPERR_AUDIO_BUFFER_OVERFLOW: return "This will only be returned on input (read) streams when the stream is paused because the SR driver has not retrieved data recently.";
	case SPERR_NO_AUDIO_DATA: return "The result does not contain any audio, nor does the portion of the element chain of the result contain any audio.";
	case SPERR_DEAD_ALTERNATE: return "This alternate is no longer a valid alternate to the result it was obtained from. Returned from ISpPhraseAlt methods.";
	case SPERR_HIGH_LOW_CONFIDENCE: return "The result does not contain any audio, nor does the portion of the element chain of the result contain any audio. Returned from ISpResult::GetAudio and ISpResult::SpeakAudio.";
	case SPERR_INVALID_FORMAT_STRING: return "The XML format string for this RULEREF is not valid, for example not a GUID or REFCLSID.";
	case SP_UNSUPPORTED_ON_STREAM_INPUT: return "The operation is not supported for stream input.";
	case SPERR_APPLEX_READ_ONLY: return "The operation is not valid for all but newly created application lexicons.";
	case SPERR_NO_TERMINATING_RULE_PATH: return "SPERR_NO_TERMINATING_RULE_PATH";
	case SP_WORD_EXISTS_WITHOUT_PRONUNCIATION: return "The word exists but without pronunciation.";
	case SPERR_STREAM_CLOSED: return "An operation was attempted on a stream object that has been closed.";
	case SPERR_NO_MORE_ITEMS: return "When enumerating items, the requested index is greater than the count of items.";
	case SPERR_NOT_FOUND: return "The requested data item (such as as data key or value) was not found.";
	case SPERR_INVALID_AUDIO_STATE: return "Audio state passed to SetState() is not valid.";
	case SPERR_GENERIC_MMSYS_ERROR: return "A generic MMSYS error not caught by _MMRESULT_TO_HRESULT.";
	case SPERR_MARSHALER_EXCEPTION: return "An exception was raised during a call to the marshaling code.";
	case SPERR_NOT_DYNAMIC_GRAMMAR: return "Attempt was made to manipulate a non-dynamic grammar.";
	case SPERR_AMBIGUOUS_PROPERTY: return "Cannot add ambiguous property.";
	case SPERR_INVALID_REGISTRY_KEY: return "The key specified is not valid.";
	case SPERR_INVALID_TOKEN_ID: return "The token specified is not valid.";
	case SPERR_XML_BAD_SYNTAX: return "The xml parser failed due to bad syntax.";
	case SPERR_XML_RESOURCE_NOT_FOUND: return "The XML parser failed to load a required resource (such as a voice or recognizer).";
	case SPERR_TOKEN_IN_USE: return "Attempted to remove registry data from a token that is already in use elsewhere.";
	case SPERR_TOKEN_DELETED: return "Attempted to perform an action on an object token that has had associated registry key deleted.";
	case SPERR_MULTI_LINGUAL_NOT_SUPPORTED: return "The selected voice was registered as multi-lingual. The Speech Platform does not support multi-lingual registration.";
	case SPERR_EXPORT_DYNAMIC_RULE: return "Exported rules cannot refer directly or indirectly to a dynamic rule.";
	case SPERR_STGF_ERROR: return "Error parsing an XML-format grammar.";
	case SPERR_WORDFORMAT_ERROR: return "Incorrect word format, probably due to incorrect pronunciation string.";
	case SPERR_STREAM_NOT_ACTIVE: return "Methods associated with active audio stream cannot be called unless stream is active.";
	case SPERR_ENGINE_RESPONSE_INVALID: return "Arguments or data supplied by the engine are not in a valid format or are inconsistent.";
	case SPERR_SR_ENGINE_EXCEPTION: return "An exception was raised during a call to the current SR engine.";
	case SPERR_STREAM_POS_INVALID: return "Stream position information supplied from engine is inconsistent.";
	case SP_RECOGNIZER_INACTIVE: return "Operation could not be completed because the recognizer is inactive. It is inactive either because the recognition state is currently inactive or because no rules are active.";
	case SPERR_REMOTE_CALL_ON_WRONG_THREAD: return "When making a remote call to the server, the call was made on the wrong thread.";
	case SPERR_REMOTE_PROCESS_TERMINATED: return "The remote process terminated unexpectedly.";
	case SPERR_REMOTE_PROCESS_ALREADY_RUNNING: return "The remote process is already running; it cannot be started a second time.";
	case SPERR_LANGID_MISMATCH: return "An attempt to load a CFG grammar with a LANGID different than other loaded grammars.";
	case SP_PARTIAL_PARSE_FOUND: return "A grammar-ending parse has been found that does not use all available words.";
	case SPERR_NOT_TOPLEVEL_RULE: return "An attempt to deactivate or activate a non top-level rule.";
	case SP_NO_RULE_ACTIVE: return "An attempt to parse when no rule was active.";
	case SPERR_LEX_REQUIRES_COOKIE: return "An attempt to ask a container lexicon for all words at once.";
	case SP_STREAM_UNINITIALIZED: return "An attempt to activate a rule or grammar without calling SetInput first.";
	case SPERR_UNSUPPORTED_LANG: return "The requested language is not supported.";
	case SPERR_VOICE_PAUSED: return "The operation cannot be performed because the voice is currently paused.";
	case SPERR_AUDIO_BUFFER_UNDERFLOW: return "This will only be returned on input (read) streams when the real time audio device stops returning data for a long period of time.";
	case SPERR_AUDIO_STOPPED_UNEXPECTEDLY: return "An audio device stopped returning data from the Read() method even though it was in the run state. This error is only returned in the END_SR_STREAM event.";
	case SPERR_NO_WORD_PRONUNCIATION: return "The SR engine is unable to add this word to a grammar. The application may need to supply an explicit pronunciation for this word.";
	case SPERR_ALTERNATES_WOULD_BE_INCONSISTENT: return "An attempt to call ScaleAudio on a recognition result having previously called GetAlternates. Allowing the call to succeed would result in the previously created alternates located in incorrect audio stream positions.";
	case SPERR_TIMEOUT: return "A task could not complete because the SR engine had timed out.";
	case SPERR_REENTER_SYNCHRONIZE: return "An SR engine called synchronize while inside of a synchronize call.";
	case SPERR_STATE_WITH_NO_ARCS: return "The grammar contains a node no arcs.";
	case SPERR_NOT_ACTIVE_SESSION: return "Neither audio output nor input is supported for non-active console sessions.";
	case SPERR_ALREADY_DELETED: return "The object is a stale reference and is not valid to use. For example, having an ISpeechGrammarRule object reference and then calling ISpeechRecoGrammar::Reset() will cause the rule object to be invalidated. Calling any methods after this will result in this error.";
	case SP_AUDIO_STOPPED: return "This can be returned from Read or Write calls for audio streams when the stream is stopped.";
	case SPERR_RECOXML_GENERATION_FAIL: return "The Recognition Parse Tree could not be generated. For example, a rule name begins with a digit but the XML parser does not allow an element name beginning with a digit.";
	case SPERR_SML_GENERATION_FAIL: return "The SML could not be generated. For example, the transformation xslt template is not well formed.";
	case SPERR_NOT_PROMPT_VOICE: return "The SML could not be generated. For example, the transformation xslt template is not well formed.";
	case SPERR_ROOTRULE_ALREADY_DEFINED: return "There is already a root rule for this grammar. Defining another root rule will fail.";
	case SPERR_SCRIPT_DISALLOWED: return "Support for embedded script not supported because browser security settings have disabled it.";
	case SPERR_REMOTE_CALL_TIMED_OUT_START: return "A time out occurred starting the sapi server.";
	case SPERR_REMOTE_CALL_TIMED_OUT_CONNECT: return "A timeout occurred obtaining the lock for starting or connecting to sapi server.";
	case SPERR_SECMGR_CHANGE_NOT_ALLOWED: return "When there is a cfg grammar loaded, changing the security manager is not permitted.";
	case SP_COMPLETE_BUT_EXTENDABLE: return "Parse is valid but could be extendable (internal use only).";
	case SPERR_FAILED_TO_DELETE_FILE: return "Tried and failed to delete an existing file.";
	case SPERR_RECOGNIZER_NOT_FOUND: return "No recognizer is installed.";
	case SPERR_AUDIO_NOT_FOUND: return "No audio device is installed.";
	case SPERR_NO_VOWEL: return "No vowel in a word.";
	case SPERR_UNSUPPORTED_PHONEME: return "No vowel in a word.";
	case SP_NO_RULES_TO_ACTIVATE: return "The grammar does not have any root or top-level active rules to activate.";
	case SP_NO_WORDENTRY_NOTIFICATION: return "The engine does not need Speech Platform word entry handles for this grammar.";
	case SPERR_WORD_NEEDS_NORMALIZATION: return "The word passed to the GetPronunciations interface needs normalizing first.";
	case SPERR_CANNOT_NORMALIZE: return "The word passed to the normalize interface cannot be normalized.";
	case S_NOTSUPPORTED: return "This combination of function call and input is currently not supported.";
	default: return "Unknown error code: " + std::to_string(error);
	}
}



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


void VRSpeechListener::InitSAPI() const
{
	if (g_sapiIsInitialized)
		return;

	if (FAILED(CoInitialize(nullptr)))
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

	hr = g_sapiRecoCtx->SetNotifyCallbackFunction(SAPICallback, nullptr, nullptr);
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

	hr = g_sapiRecoGrammar->LoadDictation(nullptr, SPLO_STATIC);
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
		g_sapiRecoCtx->SetNotifySink(nullptr);
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
		hr = cpRecoResult->GetText(SP_GETWHOLEPHRASE, SP_GETWHOLEPHRASE, TRUE, &pwszText, nullptr);
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
