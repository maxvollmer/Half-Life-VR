#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <memory>

#include "../vr_shared/VRShared.h"

class CSpEvent;
struct ISpRecognizer;
struct ISpRecoGrammar;
struct ISpRecoContext;

class VRSpeechListener
{
public:
	VRSpeechCommand GetCommand();

	static VRSpeechListener& Instance();

private:
	friend class std::unique_ptr<VRSpeechListener>;
	friend struct std::default_delete<VRSpeechListener>;

	VRSpeechListener();
	~VRSpeechListener();

	void InitSAPI();
	ISpRecoGrammar* InitGrammar(ISpRecoContext* recoContext);
	void CleanupSAPI();

	void VerifyResult(long result, const std::string& message);

	std::string GetSpeechRecognitionErrorMessageText(long error);


	const wchar_t*					m_ruleName{ L"hlvrRuleName" };
	const unsigned __int64			m_grammarId{ 0 };

	ISpRecognizer*					m_recognizer{ nullptr };
	ISpRecoContext*					m_context{ nullptr };
	ISpRecoGrammar*					m_grammar{ nullptr };

	void*							m_eventHandle{ nullptr };

	bool							m_isInitialized{ false };


	const std::unordered_map<VRSpeechCommand, std::unordered_set<std::wstring>>	m_commands = { {
		{ VRSpeechCommand::WAIT, {{L"wait", L"stop", L"hold"}} },
		{ VRSpeechCommand::FOLLOW, {{L"follow me", L"come", L"lets go"}} },
		{ VRSpeechCommand::HELLO, {{L"hello", L"good morning", L"hey", L"hi", L"morning"}} },
	} };


	static VRSpeechListener		m_speechListener;
};

