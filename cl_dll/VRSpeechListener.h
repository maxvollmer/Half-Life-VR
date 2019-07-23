#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <memory>

class CSpEvent;
struct ISpRecoGrammar;
struct ISpRecoContext;

class VRSpeechListener
{
public:
	enum class Command
	{
		WAIT,
		FOLLOWME,
		GREETING,
		NONE
	};

	Command GetCommand() const;

	static const VRSpeechListener& Instance();

private:
	friend class std::unique_ptr<VRSpeechListener>;
	friend struct std::default_delete<VRSpeechListener>;

	VRSpeechListener();
	~VRSpeechListener();

	void InitSAPI() const;
	void CleanupSAPI() const;

	ISpRecoGrammar* InitGrammar(ISpRecoContext* recoContext) const;
	std::string ExtractInput(const CSpEvent& event) const;

	const std::unordered_map<Command, std::unordered_set<std::string>>	m_commands = { {
		{ Command::WAIT, {{"wait", "stop", "hold"}} },
		{ Command::FOLLOWME, {{"follow me", "come", "lets go"}} },
		{ Command::GREETING, {{"hello", "good morning", "hey", "hi", "morning"}} },
	} };

	const unsigned __int64		m_grammarId = 0;
	const wchar_t*				m_ruleName1 = L"hlvrRuleName";

	static VRSpeechListener		m_speechListener;
};

