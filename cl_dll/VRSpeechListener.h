#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <memory>

#include "../vr_shared/VRShared.h"

class CSpEvent;
struct ISpRecoGrammar;
struct ISpRecoContext;

class VRSpeechListener
{
public:
	VRSpeechCommand GetCommand() const;

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

	const std::unordered_map<VRSpeechCommand, std::unordered_set<std::string>>	m_commands = { {
		{ VRSpeechCommand::WAIT, {{"wait", "stop", "hold"}} },
		{ VRSpeechCommand::FOLLOW, {{"follow me", "come", "lets go"}} },
		{ VRSpeechCommand::HELLO, {{"hello", "good morning", "hey", "hi", "morning"}} },
	} };

	const unsigned __int64		m_grammarId = 0;
	const wchar_t*				m_ruleName1 = L"hlvrRuleName";

	static VRSpeechListener		m_speechListener;
};

