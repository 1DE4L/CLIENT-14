/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_C14_COMBAT_H
#define GAME_CLIENT_COMPONENTS_C14_COMBAT_H

#include <engine/client.h>
#include <engine/console.h>

#include <game/client/component.h>

class CC14Combat : public CComponent
{
public:
	CC14Combat();
	int Sizeof() const override { return sizeof(*this); }

	void OnConsoleInit() override;
	void OnReset() override;
	void Apply(CNetObj_PlayerInput *pInput);

private:
	struct CKeyInputState
	{
		CC14Combat *m_pCombat;
		int *m_apVariables[NUM_DUMMIES];
	};
	static void ConKeyInputState(IConsole::IResult *pResult, void *pUserData);

	int m_aAutoFireKeyDown[NUM_DUMMIES];
	int m_aAutoFireTimer[NUM_DUMMIES];
	int m_aLastHammerTick[NUM_DUMMIES];
};

#endif
