#ifndef GAME_CLIENT_COMPONENTS_BINDWHEEL_H
#define GAME_CLIENT_COMPONENTS_BINDWHEEL_H

#include <game/client/component.h>

class CBindWheel : public CComponent
{
	static constexpr int MAX_WHEEL_ITEMS = 8;

	struct CWheelItem
	{
		char m_aLabel[64];
		char m_aCommand[128];
		int m_Key;
	};

	int m_Active;
	int m_SelectedItem;
	CWheelItem m_aItems[MAX_WHEEL_ITEMS];
	int m_NumItems;

	void LoadItems();

public:
	int Sizeof() const override { return sizeof(*this); }
	void OnConsoleInit() override;
	void OnInit() override;
	void OnRender() override;
	bool OnInput(const IInput::CEvent &Event) override;
};

#endif
