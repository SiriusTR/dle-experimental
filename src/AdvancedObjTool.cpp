
#include "stdafx.h"
#include "afxpriv.h"
#include <malloc.h>
#include <stdlib.h>
#undef abs
#include <math.h>
#include <mmsystem.h>
#include <stdio.h>

#include "mine.h"
#include "dle-xp.h"
#include "toolview.h"
#include "textures.h"
#include "RobotManager.h"

//------------------------------------------------------------------------------

BEGIN_MESSAGE_MAP (CAdvancedObjTool, CObjectTabDlg)
	ON_EN_KILLFOCUS (IDC_ADVOBJ_SIZE, OnAccept)
	ON_EN_KILLFOCUS (IDC_ADVOBJ_SHIELD, OnAccept)
	ON_EN_KILLFOCUS (IDC_ADVOBJ_MASS, OnAccept)
	ON_EN_KILLFOCUS (IDC_ADVOBJ_DRAG, OnAccept)
	ON_EN_KILLFOCUS (IDC_ADVOBJ_BRAKES, OnAccept)
	ON_EN_KILLFOCUS (IDC_ADVOBJ_TURNROLL, OnAccept)
	ON_EN_KILLFOCUS (IDC_ADVOBJ_FLAGS, OnAccept)
	ON_EN_KILLFOCUS (IDC_ADVOBJ_VX , OnAccept)
	ON_EN_KILLFOCUS (IDC_ADVOBJ_TX , OnAccept)
	ON_EN_KILLFOCUS (IDC_ADVOBJ_RVX, OnAccept)
	ON_EN_KILLFOCUS (IDC_ADVOBJ_VY , OnAccept)
	ON_EN_KILLFOCUS (IDC_ADVOBJ_TY , OnAccept)
	ON_EN_KILLFOCUS (IDC_ADVOBJ_RVY, OnAccept)
	ON_EN_KILLFOCUS (IDC_ADVOBJ_VZ , OnAccept)
	ON_EN_KILLFOCUS (IDC_ADVOBJ_TZ , OnAccept)
	ON_EN_KILLFOCUS (IDC_ADVOBJ_RVZ, OnAccept)
	ON_EN_KILLFOCUS (IDC_ADVOBJ_RTX, OnAccept)
	ON_EN_KILLFOCUS (IDC_ADVOBJ_RTY, OnAccept)
	ON_EN_KILLFOCUS (IDC_ADVOBJ_RTZ, OnAccept)
	ON_EN_KILLFOCUS (IDC_ADVOBJ_MODEL, OnAccept)
	ON_EN_KILLFOCUS (IDC_ADVOBJ_FRAME, OnAccept)
	ON_EN_KILLFOCUS (IDC_ADVOBJ_FRAMENO, OnAccept)
END_MESSAGE_MAP ()

//------------------------------------------------------------------------------

BOOL CAdvancedObjTool::OnInitDialog ()
{
if (!CObjectTabDlg::OnInitDialog ())
	return FALSE;
m_bInited = true;
return TRUE;
}

//------------------------------------------------------------------------------

void CAdvancedObjTool::DoDataExchange (CDataExchange * pDX)
{
if (!HaveData (pDX)) 
	return;
DDX_Text (pDX, IDC_ADVOBJ_MASS, m_mass);
DDX_Text (pDX, IDC_ADVOBJ_DRAG, m_drag);
DDX_Text (pDX, IDC_ADVOBJ_BRAKES, m_brakes);
DDX_Text (pDX, IDC_ADVOBJ_TURNROLL, m_turnRoll);
DDX_Text (pDX, IDC_ADVOBJ_FLAGS, m_flags);
DDX_Text (pDX, IDC_ADVOBJ_SIZE, m_size);
DDX_Text (pDX, IDC_ADVOBJ_SHIELD, m_shields);
DDX_Text (pDX, IDC_ADVOBJ_VX, m_velocity.v.x);
DDX_Text (pDX, IDC_ADVOBJ_VY, m_velocity.v.y);
DDX_Text (pDX, IDC_ADVOBJ_VZ, m_velocity.v.z);
DDX_Text (pDX, IDC_ADVOBJ_TX, m_thrust.v.x);
DDX_Text (pDX, IDC_ADVOBJ_TY, m_thrust.v.y);
DDX_Text (pDX, IDC_ADVOBJ_TZ, m_thrust.v.z);
DDX_Text (pDX, IDC_ADVOBJ_RVX, m_rotVel.v.x);
DDX_Text (pDX, IDC_ADVOBJ_RVY, m_rotVel.v.y);
DDX_Text (pDX, IDC_ADVOBJ_RVZ, m_rotVel.v.z);
DDX_Text (pDX, IDC_ADVOBJ_RTX, m_rotThrust.v.x);
DDX_Text (pDX, IDC_ADVOBJ_RTY, m_rotThrust.v.y);
DDX_Text (pDX, IDC_ADVOBJ_RTZ, m_rotThrust.v.z);
DDX_Text (pDX, IDC_ADVOBJ_MODEL, m_model);
DDX_Text (pDX, IDC_ADVOBJ_FRAME, m_frame);
DDX_Text (pDX, IDC_ADVOBJ_FRAMENO, m_frameNo);
}
								
								/*--------------------------*/

void CAdvancedObjTool::OnAccept (void)
{
if (!(m_bInited && theMine))
	return;
UpdateData (TRUE);
CGameObject *pObject = current->Object ();
pObject->mType.physInfo.mass = m_mass;
pObject->mType.physInfo.drag = m_drag;
pObject->mType.physInfo.brakes = m_brakes;
pObject->mType.physInfo.turnRoll = m_turnRoll;
pObject->mType.physInfo.flags = m_flags;
pObject->mType.physInfo.velocity.x = m_velocity.v.x;
pObject->mType.physInfo.velocity.y = m_velocity.v.y;
pObject->mType.physInfo.velocity.z = m_velocity.v.z;
pObject->mType.physInfo.thrust.x = m_thrust.v.x;
pObject->mType.physInfo.thrust.y = m_thrust.v.y;
pObject->mType.physInfo.thrust.z = m_thrust.v.z;
pObject->mType.physInfo.rotVel.x = m_rotVel.v.x;
pObject->mType.physInfo.rotVel.y = m_rotVel.v.y;
pObject->mType.physInfo.rotVel.z = m_rotVel.v.z;
pObject->mType.physInfo.rotThrust.x = m_rotThrust.v.x;
pObject->mType.physInfo.rotThrust.y = m_rotThrust.v.y;
pObject->mType.physInfo.rotThrust.z = m_rotThrust.v.z;
}

								/*--------------------------*/

BOOL CAdvancedObjTool::OnSetActive ()
{
Refresh ();
return CObjectTabDlg::OnSetActive ();
}

								/*--------------------------*/

BOOL CAdvancedObjTool::OnKillActive ()
{
OnAccept ();
return CObjectTabDlg::OnSetActive ();
}

								/*--------------------------*/

bool CAdvancedObjTool::Refresh (void)
{
if (!(m_bInited && theMine))
	return false;
if (!objectManager.Count ()) {
	CDlgHelpers::EnableControls (IDC_ADVOBJ_SIZE, IDC_ADVOBJ_RTZ, FALSE);
	return false;
	}
CDlgHelpers::EnableControls (IDC_ADVOBJ_SIZE, IDC_ADVOBJ_FRAMENO, TRUE);

CGameObject *pObject = current->Object ();
m_size = pObject->m_info.size;
m_shields = pObject->m_info.shields;
CRobotInfo& robotInfo = *robotManager.RobotInfo (pObject->m_info.id);

switch (pObject->m_info.movementType) {
	case MT_PHYSICS:	
		m_mass = pObject->mType.physInfo.mass ? pObject->mType.physInfo.mass : robotInfo.Info ().mass;
		m_drag = pObject->mType.physInfo.drag ? pObject->mType.physInfo.drag : robotInfo.Info ().drag;
		m_brakes = pObject->mType.physInfo.brakes;
		m_turnRoll = pObject->mType.physInfo.turnRoll;
		m_flags = pObject->mType.physInfo.flags ? pObject->mType.physInfo.flags : robotInfo.Info ().flags;
		m_velocity = pObject->mType.physInfo.velocity;
		m_thrust = pObject->mType.physInfo.thrust;
		m_rotVel = pObject->mType.physInfo.rotVel;
		m_rotThrust = pObject->mType.physInfo.rotThrust;
		break;

	case MT_SPINNING:
	case MT_NONE:
	default:
		CDlgHelpers::EnableControls (IDC_ADVOBJ_MASS, IDC_ADVOBJ_RTZ, FALSE);
		m_mass = 0;
		m_drag = 0;
		m_brakes = 0;
		m_turnRoll = 0;
		m_flags = 0;
		m_velocity.Clear ();
		m_thrust.Clear ();
		m_rotVel.Clear ();
		m_rotThrust.Clear ();
		break;
	}

switch (pObject->m_info.renderType) {
	case RT_MORPH:
	case RT_POLYOBJ:
		m_model = pObject->rType.polyModelInfo.nModel ? pObject->rType.polyModelInfo.nModel : robotInfo.Info ().nModel;
		m_frame = 0;
		m_frameNo = 0;
		CDlgHelpers::EnableControls (IDC_ADVOBJ_FRAME, IDC_ADVOBJ_FRAMENO, FALSE);
		break;

	case RT_WEAPON_VCLIP:
	case RT_HOSTAGE:
	case RT_POWERUP:
	case RT_FIREBALL:
		m_model = pObject->rType.animationInfo.nAnimation;
		m_frame = pObject->rType.animationInfo.nFrameTime;
		m_frameNo = pObject->rType.animationInfo.nFrame;
		break;

	case RT_LASER:
	case RT_NONE:
	default:
		CDlgHelpers::EnableControls (IDC_ADVOBJ_MODEL, IDC_ADVOBJ_FRAMENO, FALSE);
		m_model = 0;
		m_frame = 0;
		m_frameNo = 0;
		break;
	}
UpdateData (FALSE);
return true;
}

//------------------------------------------------------------------------------

//eof advObjTool.cpp