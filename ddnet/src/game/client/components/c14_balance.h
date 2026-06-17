/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_C14_BALANCE_H
#define GAME_CLIENT_COMPONENTS_C14_BALANCE_H

#include <engine/client.h>
#include <engine/console.h>

#include <game/client/component.h>

class CC14Balance : public CComponent
{
public:
	CC14Balance();
	int Sizeof() const override { return sizeof(*this); }

	void OnConsoleInit() override;
	void OnReset() override;
	void Apply(CNetObj_PlayerInput *pInput);

private:
	struct CKeyInputState
	{
		CC14Balance *m_pBalance;
		int *m_apVariables[NUM_DUMMIES];
	};
	static void ConKeyInputState(IConsole::IResult *pResult, void *pUserData);

	int m_aKeyDown[NUM_DUMMIES];
	int m_LastDirection = 0;

	// Lock-on balance: target X is captured when +balance is first pressed
	// and the bot tries to keep the tee on that exact X line.
	float m_TargetX = 0.0f;
	bool m_HasTarget = false;
};

#endif
