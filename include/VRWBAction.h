#ifndef _VRWBACTION_H_
#define _VRWBACTION_H_

#include "VRAction.h"

class VRSculptSet : public VRActionSet
{
	public:
		VRSculptSet() : VRActionSet(), m_pDrawAction(0) {}
		virtual ~VRSculptSet() {  }

		virtual void IdleDrawCallback(void);
		
		void SetDrawAction(VRAction *pAction) { m_pDrawAction = pAction; }

	private:
		VRAction *m_pDrawAction;
};

class VRChangeBrush : public VRWandAction
{
	public:
		VRChangeBrush() : VRWandAction("change_brush") {}
		virtual ~VRChangeBrush() {}

		virtual void ButtonUp(void);

	protected:
};

class VRChangeSpecificTool : public VRWandAction
{
	public:
		VRChangeSpecificTool(unsigned int tool=0) : VRWandAction("change_specific_tool"), m_tool(tool) {}
		virtual ~VRChangeSpecificTool() {}

		virtual void ButtonUp(void);

	private:
		unsigned int m_tool;
};

class VRChangeTool : public VRWandAction
{
	public:
		VRChangeTool() : VRWandAction("change_tool") {}
		virtual ~VRChangeTool() {}

		virtual void ButtonUp(void);

	protected:
};

class VRChangeTSR : public VRWandAction
{
	public:
		VRChangeTSR() : VRWandAction("change_tsr") {}
		virtual ~VRChangeTSR() {}

		virtual void ButtonUp(void);

	protected:
};

class VRClearSelect : public VRWandAction
{
	public:
		VRClearSelect() : VRWandAction("clear_select") {}
		virtual ~VRClearSelect() {}

	protected:
		
};

class VRDelete : public VRWandAction
{
	public:
		VRDelete() : VRWandAction("delete") {}
		virtual ~VRDelete() {}

	protected:
		virtual void	ButtonUp(void);
};

class VRDuplicate : public VRWandAction
{
	public:
		VRDuplicate() : VRWandAction("duplicate") {}
		virtual ~VRDuplicate() {}

	protected:
		virtual void	ButtonUp(void);
};

class VRErase : public VRWandAction
{
	public:
		VRErase() : VRWandAction("erase"), m_beamPoint(0.f, 0.f, 0.f) {}
		virtual ~VRErase() {}

		virtual void ButtonDown(void);
		virtual void WandMove(void);
		virtual void ButtonUp(void);
		virtual void JoystickMove(void);
		virtual void DrawCallback(void);

	private:

		jvec3 m_beamPoint;
};

class VRExportPly : public VRWandAction
{
	public:
		VRExportPly() : VRWandAction("export_ply") {}
		virtual ~VRExportPly() {}

		virtual void ButtonUp(void);

	protected:
};

class VRJoystickResize : public VRWandAction
{
	public:
		VRJoystickResize() : VRWandAction("joystick_resize") {}
		virtual ~VRJoystickResize() {}

		virtual void JoystickMove(void);

	protected:
};

class VRPaint : public VRWandAction
{
	public:
		VRPaint() : VRWandAction("paint") {}
		virtual ~VRPaint() {}

		virtual void ButtonDown(void);
		virtual void WandMove(void);
		virtual void ButtonUp(void);
		virtual void JoystickMove(void);
		virtual void DrawCallback(void);

	private:
};

class VRResetSculpt : public VRWandAction
{
	public:
		VRResetSculpt() : VRWandAction("reset_sculpt"), m_timeDown(0.f), m_bReset(false) {}
		virtual ~VRResetSculpt() {}

		virtual void ButtonDown(void);
		virtual void WandMove(void);
		virtual void ButtonUp(void);

	protected:

		float m_timeDown;
		bool m_bReset;
};

class VRRotate : public VRWandAction
{
	public:
		VRRotate() : VRWandAction("rotate"), m_fSelectionDistance(0.f), m_vOffset(0.f, 0.f, 0.f), m_bDoRotate(false) {}
		virtual ~VRRotate() {}

		virtual	void	ButtonDown(void);
		virtual void	ButtonUp(void);
		virtual void	WandMove(void);
		virtual void	JoystickMove(void);

	protected:
		float m_fSelectionDistance;
		jvec3 m_vOffset;
		bool  m_bDoRotate;
};

class VRSave : public VRWandAction
{
	public:
		VRSave() : VRWandAction("save") {}
		virtual ~VRSave() {}

		virtual void ButtonUp(void);

	protected:
};

class VRScale : public VRWandAction
{
	public:
		VRScale() : VRWandAction("scale"), m_fSelectionDistance(0.f), m_vOffset(0.f, 0.f, 0.f), m_bDoScale(false), m_bLarger(true) {}
		virtual ~VRScale() {}

		void SetLarger(bool bLarger) { m_bLarger = bLarger; }

		virtual	void	ButtonDown(void);
		virtual void	ButtonUp(void);
		virtual void	WandMove(void);
		virtual void	JoystickMove(void);

	protected:

	private:
		float m_fSelectionDistance;
		jvec3  m_vOffset;
		bool  m_bDoScale;
		bool  m_bLarger;
};

class VRSculpt : public VRWandAction
{
	public:
		VRSculpt() : VRWandAction("sculpt"), m_bCubeAdded(false), m_bUseSecondTracker(false), 
				m_addCubeP1(-0.05f, -0.05f, -0.05f), m_addCubeP2(0.05f, 0.05f, 0.05f) {}
		
		virtual ~VRSculpt() {}

		void SetUseSecondTracker(bool second) { m_bUseSecondTracker=second; }

		virtual void ButtonDown(void);
		virtual void WandMove(void);
		virtual void ButtonUp(void);
		virtual void JoystickMove(void);
		virtual void DrawCallback(void);

	private:

		bool m_bCubeAdded;
		bool m_bUseSecondTracker;

		jvec3 m_addCubeP1;
		jvec3 m_addCubeP2;
};

class VRSelect : public VRWandAction
{
	public:
		VRSelect() : VRWandAction("select"), m_fSelectionDistance(0.f), m_vOffset(0.f, 0.f, 0.f), m_bDoTranslate(false) {}
		virtual ~VRSelect() {}

		virtual void	ButtonUp(void);
		virtual	void	ButtonDown(void);
		virtual void	WandMove(void);
		virtual void	JoystickMove(void);

	private:
		float m_fSelectionDistance;
		jvec3 m_vOffset;
		bool  m_bDoTranslate;
};

class VRSwitchModes : public VRWandAction
{
	public:
		VRSwitchModes() : VRWandAction("switch_modes") {}
		virtual ~VRSwitchModes() {}

		virtual void ButtonUp(void);

	protected:

};

class VRTranslate : public VRWandAction
{
	public:
		VRTranslate() : VRWandAction("translate"), m_fSelectionDistance(0.f), m_vOffset(0.f, 0.f, 0.f), m_bDoTranslate(false) {}
		virtual ~VRTranslate() {}

		virtual	void	ButtonDown(void);
		virtual void	WandMove(void);
		virtual void	JoystickMove(void);
		virtual void	ButtonUp(void);

	protected:

	private:
		float m_fSelectionDistance;
		jvec3 m_vOffset;
		bool  m_bDoTranslate;
};

class VRUseTool : public VRWandAction
{
	public:
		VRUseTool() : VRWandAction("use_tool") {}

		virtual ~VRUseTool() {}

	protected:

		virtual void ButtonDown(void);
		virtual void WandMove(void);
		virtual void ButtonUp(void);
		virtual void JoystickMove(void);
		virtual void DrawCallback(void);

	private:
		VRSculpt m_sculptAction;
		VRPaint m_paintAction;
		VRErase m_eraseAction;
};

class VRTSR : public VRWandAction
{
	public:
		VRTSR() : VRWandAction("tsr") {}

		virtual ~VRTSR() {}

	protected:

		virtual void ButtonDown(void);
		virtual void WandMove(void);
		virtual void ButtonUp(void);
		virtual void JoystickMove(void);
		virtual void DrawCallback(void);

	private:
		VRTranslate m_translateAction;
		VRScale m_scaleAction;
		VRRotate m_rotateAction;
};
#endif