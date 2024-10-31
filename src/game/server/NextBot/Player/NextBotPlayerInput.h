/* NextBotPlayerInput
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef NEXTBOT_NEXTBOTPLAYER_NEXTBOTPLAYERINPUT_H
#define NEXTBOT_NEXTBOTPLAYER_NEXTBOTPLAYERINPUT_H
#ifdef _WIN32
#pragma once
#endif


class INextBotPlayerInput
{
public:
	virtual void PressFireButton(float duration = -1.0f) = 0;
	virtual void ReleaseFireButton() = 0;
	virtual void PressAltFireButton(float duration = -1.0f) = 0;
	virtual void ReleaseAltFireButton() = 0;
	virtual void PressMeleeButton(float duration = -1.0f) = 0;
	virtual void ReleaseMeleeButton() = 0;
	virtual void PressSpecialFireButton(float duration = -1.0f) = 0;
	virtual void ReleaseSpecialFireButton() = 0;
	virtual void PressUseButton(float duration = -1.0f) = 0;
	virtual void ReleaseUseButton() = 0;
	virtual void PressReloadButton(float duration = -1.0f) = 0;
	virtual void ReleaseReloadButton() = 0;
	virtual void PressForwardButton(float duration = -1.0f) = 0;
	virtual void ReleaseForwardButton() = 0;
	virtual void PressBackwardButton(float duration = -1.0f) = 0;
	virtual void ReleaseBackwardButton() = 0;
	virtual void PressLeftButton(float duration = -1.0f) = 0;
	virtual void ReleaseLeftButton() = 0;
	virtual void PressRightButton(float duration = -1.0f) = 0;
	virtual void ReleaseRightButton() = 0;
	virtual void PressJumpButton(float duration = -1.0f) = 0;
	virtual void ReleaseJumpButton() = 0;
	virtual void PressCrouchButton(float duration = -1.0f) = 0;
	virtual void ReleaseCrouchButton() = 0;
	virtual void PressWalkButton(float duration = -1.0f) = 0;
	virtual void ReleaseWalkButton() = 0;
	virtual void SetButtonScale(float fwd, float side) = 0;
};


#endif
