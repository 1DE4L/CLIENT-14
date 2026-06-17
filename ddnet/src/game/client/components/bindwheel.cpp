#include "bindwheel.h"

#include <engine/graphics.h>
#include <engine/input.h>
#include <engine/shared/config.h>
#include <engine/textrender.h>

#include <game/client/gameclient.h>

void CBindWheel::LoadItems()
{
	m_NumItems = 0;

	m_aItems[m_NumItems++] = {"Kill", "kill", KEY_K};
	m_aItems[m_NumItems++] = {"Pause", "say /pause", KEY_P};
	m_aItems[m_NumItems++] = {"Spec", "say /spec", KEY_O};
	m_aItems[m_NumItems++] = {"Info", "say_team info", KEY_I};

	if(m_NumItems < MAX_WHEEL_ITEMS)
	{
		m_aItems[m_NumItems++] = {"Emote 1", "emote 1", KEY_1};
		m_aItems[m_NumItems++] = {"Emote 2", "emote 2", KEY_2};
		m_aItems[m_NumItems++] = {"Emote 3", "emote 3", KEY_3};
		m_aItems[m_NumItems++] = {"Ready", "ready_change", KEY_R};
	}
}

void CBindWheel::OnConsoleInit()
{
}

void CBindWheel::OnInit()
{
	m_Active = 0;
	m_SelectedItem = -1;
	LoadItems();
}

void CBindWheel::OnRender()
{
	if(!m_Active)
		return;

	float ScreenW = (float)Graphics()->ScreenWidth();
	float ScreenH = (float)Graphics()->ScreenHeight();
	float Cx = ScreenW * 0.5f;
	float Cy = ScreenH * 0.5f;

	float InnerRadius = 40.0f;
	float OuterRadius = 140.0f;
	float TextRadius = 90.0f;

	vec2 Center(Cx, Cy);

	Graphics()->TextureClear();
	Graphics()->QuadsBegin();

	for(int i = 0; i < m_NumItems; i++)
	{
		float Angle = 2.0f * pi * i / (float)m_NumItems - pi / 2.0f;
		float NextAngle = 2.0f * pi * (i + 1) / (float)m_NumItems - pi / 2.0f;
		float MidAngle = Angle + pi / m_NumItems;

		ColorRGBA Color = (i == m_SelectedItem) ? ColorRGBA(1.0f, 0.4f, 0.1f, 0.85f) : ColorRGBA(0.15f, 0.15f, 0.15f, 0.75f);

		Graphics()->SetColor(Color);
		float Segments = 10.0f;
		for(int s = 0; s < (int)Segments; s++)
		{
			float a0 = Angle + (NextAngle - Angle) * s / Segments;
			float a1 = Angle + (NextAngle - Angle) * (s + 1) / Segments;
			vec2 p0 = Center + vec2(cosf(a0) * InnerRadius, sinf(a0) * InnerRadius);
			vec2 p1 = Center + vec2(cosf(a1) * InnerRadius, sinf(a1) * InnerRadius);
			vec2 p2 = Center + vec2(cosf(a1) * OuterRadius, sinf(a1) * OuterRadius);
			vec2 p3 = Center + vec2(cosf(a0) * OuterRadius, sinf(a0) * OuterRadius);
			IGraphics::CFreeformItem Item(p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, p3.x, p3.y);
			Graphics()->QuadsDrawFreeform(&Item, 1);
		}
	}

	Graphics()->QuadsEnd();

	for(int i = 0; i < m_NumItems; i++)
	{
		float MidAngle = 2.0f * pi * i / (float)m_NumItems - pi / 2.0f + pi / m_NumItems;

		float FontSize = 13.0f;
		STextContainerIndex Idx;
		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), "%s", m_aItems[i].m_aLabel);

		CTextCursor Cursor;
		Cursor.m_FontSize = FontSize;
		Cursor.m_Flags = TEXTFLAG_RENDER | TEXTALIGN_MC;
		TextRender()->CreateTextContainer(Idx, &Cursor, aBuf);

		STextBoundingBox Box = TextRender()->GetBoundingBoxTextContainer(Idx);
		vec2 TextPos = Center + vec2(cosf(MidAngle) * TextRadius, sinf(MidAngle) * TextRadius);
		ColorRGBA TextColor = (i == m_SelectedItem) ? ColorRGBA(1.0f, 1.0f, 1.0f, 1.0f) : ColorRGBA(0.85f, 0.85f, 0.85f, 0.7f);
		TextRender()->RenderTextContainer(Idx, TextColor, ColorRGBA(0.0f, 0.0f, 0.0f, 0.5f), TextPos.x - Box.m_W * 0.5f, TextPos.y - Box.m_H * 0.5f);
		TextRender()->DeleteTextContainer(Idx);
	}
}

bool CBindWheel::OnInput(const IInput::CEvent &Event)
{
	if(!Input()->KeyIsPressed(KEY_TAB))
	{
		if(m_Active)
		{
			if(m_SelectedItem >= 0 && m_SelectedItem < m_NumItems)
			{
				Console()->ExecuteLine(m_aItems[m_SelectedItem].m_aCommand, IConsole::CLIENT_ID_UNSPECIFIED);
			}
			m_Active = 0;
			m_SelectedItem = -1;
		}
		return false;
	}

	if(m_Active && (Event.m_Flags & IInput::FLAG_PRESS) && Event.m_Key == KEY_TAB)
	{
		m_Active = 1;
		m_SelectedItem = -1;
		return true;
	}

	if(m_Active && (Event.m_Flags & IInput::FLAG_PRESS) && Event.m_Key == KEY_MOUSE_1)
	{
		if(m_SelectedItem >= 0 && m_SelectedItem < m_NumItems)
		{
			Console()->ExecuteLine(m_aItems[m_SelectedItem].m_aCommand, IConsole::CLIENT_ID_UNSPECIFIED);
		}
		m_Active = 0;
		m_SelectedItem = -1;
		return true;
	}

	if(m_Active)
	{
		float ScreenW = (float)Graphics()->ScreenWidth();
		float ScreenH = (float)Graphics()->ScreenHeight();
		float Cx = ScreenW * 0.5f;
		float Cy = ScreenH * 0.5f;

		vec2 MousePos = Input()->NativeMousePos();
		MousePos.x = (MousePos.x / Graphics()->WindowWidth()) * ScreenW;
		MousePos.y = (MousePos.y / Graphics()->WindowHeight()) * ScreenH;

		vec2 Delta = MousePos - vec2(Cx, Cy);
		float Dist = length(Delta);
		if(Dist > 30.0f)
		{
			float MouseAngle = angle(Delta) + pi / 2.0f;
			MouseAngle = fmod(MouseAngle, 2.0f * pi);
			if(MouseAngle < 0.0f)
				MouseAngle += 2.0f * pi;
			m_SelectedItem = (int)(MouseAngle / (2.0f * pi) * m_NumItems) % m_NumItems;
		}
		else
		{
			m_SelectedItem = -1;
		}
		return true;
	}

	return false;
}
