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

	void InitCommandStrings();
	bool DidCommandStringsChange();
	std::unordered_set<std::wstring> GetCommandsFromCommandString(const std::string& commandstring);

	void VerifyResult(long result, const std::string& message);

	std::string GetSpeechRecognitionErrorMessageText(long error);

	const wchar_t* m_ruleName{ L"hlvrRuleName" };
	const unsigned __int64 m_grammarId{ 0 };

	ISpRecognizer* m_recognizer{ nullptr };
	ISpRecoContext* m_context{ nullptr };
	ISpRecoGrammar* m_grammar{ nullptr };

	void* m_eventHandle{ nullptr };

	std::string m_followcommandstring;
	std::string m_waitcommandstring;
	std::string m_hellocommandstring;

	std::unordered_map<VRSpeechCommand, std::unordered_set<std::wstring>> m_commands;

	bool m_isInitialized{ false };

	static VRSpeechListener m_speechListener;
};
