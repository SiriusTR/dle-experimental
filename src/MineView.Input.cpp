#include "mineview.h"

CInputHandler::CInputHandler (CMineView *pMineView)
	: m_pMineView (pMineView)
{
m_clickStartPos = nullptr;
m_zoomStartPos = nullptr;
for (int i = 0; i < eModifierCount; i++) {
	m_bModifierActive [i] = false;
	}
m_bInputLockActive = false;
}

CInputHandler::~CInputHandler ()
{
if (m_clickStartPos != nullptr) {
	delete m_clickStartPos;
	m_clickStartPos = nullptr;
	}
if (m_zoomStartPos != nullptr) {
	delete m_zoomStartPos;
	m_zoomStartPos = nullptr;
	}
}

void CInputHandler::OnKeyUp (UINT nChar, UINT nRepCnt, UINT nFlags)
{
UpdateModifierStates (WM_KEYUP, nChar, nFlags);
UpdateMouseState (WM_KEYUP);
UpdateInputLockState (WM_KEYUP, nChar);

eKeyCommands command = MapKeyToCommand (nChar);
if (!IsMovementCommand (command))
	return;

switch (m_movementMode) {
	case eMovementModeStepped:
		for (int i = 0; i < nRepCnt; i++) {
			ApplyMovement (command);
			}
		break;
	case eMovementModeContinuous:
		StopMovement (command);
		break;
	default:
		// Unknown mode
		return;
	}
}

void CInputHandler::OnKeyDown (UINT nChar, UINT nRepCnt, UINT nFlags)
{
UpdateModifierStates (WM_KEYDOWN, nChar, nFlags);
UpdateMouseState (WM_KEYDOWN);
UpdateInputLockState (WM_KEYDOWN, nChar);

eKeyCommands command = MapKeyToCommand (nChar);
if (!IsMovementCommand (command))
	return;

switch (m_movementMode) {
	case eMovementModeStepped:
		for (int i = 0; i < nRepCnt; i++) {
			ApplyMovement (command);
			}
		break;
	case eMovementModeContinuous:
		StartMovement (command);
		break;
	default:
		// Unknown mode
		return;
	}
}

void CInputHandler::OnMouseMove (UINT nFlags, CPoint point)
{
UpdateMouseState (WM_MOUSEMOVE, point);

CPoint change = m_lastMousePos - point;
if (change.x || change.y) {
	switch (m_mouseState) {
		case eMouseStateIdle:
			break;

		case eMouseStateDrag:
			m_pMineView->UpdateDragPos (point);
			break;

		case eMouseStateRubberBand:
			m_pMineView->UpdateRubberRect (*m_clickStartPos, point);
			break;

		case eMouseStateSelect:
			m_pMineView->UpdateSelectHighlights (point);
			/*
			if (SelectMode (eSelectPoint) || SelectMode (eSelectLine) || SelectMode (eSelectSide) || SelectMode (eSelectSegment))
				Invalidate (FALSE);
			*/
			break;

		case eMouseStatePan:
			DoMousePan (point);
			break;

		case eMouseStateRotate:
			DoMouseRotate (point);
			break;

		case eMouseStateZoom:
			DoMouseZoom (point);
			break;
		}
	}

m_lastMousePos = point;
}

void CInputHandler::OnLButtonUp (UINT nFlags, CPoint point)
{
UpdateMouseState (WM_LBUTTONUP, point);
}

void CInputHandler::OnLButtonDown (UINT nFlags, CPoint point)
{
UpdateMouseState (WM_LBUTTONDOWN, point);
}

void CInputHandler::OnRButtonUp (UINT nFlags, CPoint point)
{
UpdateMouseState (WM_RBUTTONUP, point);
}

void CInputHandler::OnRButtonDown (UINT nFlags, CPoint point)
{
UpdateMouseState (WM_RBUTTONDOWN, point);
}

void CInputHandler::OnMButtonUp (UINT nFlags, CPoint point)
{
UpdateMouseState (WM_MBUTTONUP, point);
}

void CInputHandler::OnMButtonDown (UINT nFlags, CPoint point)
{
UpdateMouseState (WM_MBUTTONDOWN, point);
}

void CInputHandler::OnXButtonUp (UINT nFlags, UINT nButton, CPoint point)
{
// this is a bit weird but testing nFlags would be unreliable -
// if both xbuttons are pressed it'll just see the one it checks first
UpdateMouseState ((nButton << 16) | WM_XBUTTONUP, point);
}

void CInputHandler::OnXButtonDown (UINT nFlags, UINT nButton, CPoint point)
{
UpdateMouseState ((nButton << 16) | WM_XBUTTONDOWN, point);
}

void CInputHandler::OnMouseWheel (UINT nFlags, short zDelta, CPoint pt)
{
}

//------------------------------------------------------------------------------
// Internal functions

eMouseStates CInputHandler::MapInputToMouseState (UINT msg, const CPoint point) const
{
switch (m_mouseState) {
	case eMouseStateIdle:
		// If modifiers map to power select action, move to power select state
		if (HasEnteredState (eMouseStateSelect, msg) == eMatchExact)
			return eMouseStateSelect;
		// Else, if input+modifiers map to select action
			// Valid target selected: move to select pending (ButtonDown), set target
			// No target selected: Move to rubber band
		if (HasEnteredState (eMouseStateButtonDown, msg) == eMatchExact)
			return eMouseStateButtonDown;
		// Else, if input+modifiers map to move action: move to corresponding move state (pan/rotate/zoom)
		if (HasEnteredState (eMouseStateZoom, msg) == eMatchExact)
			return eMouseStateZoom;
		if (HasEnteredState (eMouseStatePan, msg) == eMatchExact)
			return eMouseStatePan;
		if (HasEnteredState (eMouseStateRotate, msg) == eMatchExact)
			return eMouseStateRotate;

		// No exact match, partial matches are now allowed
		if (HasEnteredState (eMouseStateSelect, msg))
			return eMouseStateSelect;
		if (HasEnteredState (eMouseStateButtonDown, msg))
			return eMouseStateButtonDown;
		if (HasEnteredState (eMouseStateZoom, msg))
			return eMouseStateZoom;
		if (HasEnteredState (eMouseStatePan, msg))
			return eMouseStatePan;
		if (HasEnteredState (eMouseStateRotate, msg))
			return eMouseStateRotate;
		// Otherwise do nothing
		break;

	case eMouseStateButtonDown:
		// If this is a move: move to either drag or rubber band state, depending on whether there is a target
		if (!HasExitedState (msg) && point != *m_clickStartPos) {
			if (CheckValidDragTarget (*m_clickStartPos))
				return eMouseStateDrag;
			else
				return eMouseStateRubberBand;
			}
		if (HasExitedState (msg)) {
			// If this is a mouse-up: move to quick select state
			if (ButtonUpMatchesState (eMouseStateQuickSelect, msg))
				return eMouseStateQuickSelect;
			else
				return eMouseStateIdle;
			}
		// Ignore other inputs (other mouse buttons, modifier changes)
		break;

	case eMouseStateDrag:
		// If this is a move: no state change
		// If this is a mouse-up for the select button: move to apply drag state
		if (HasExitedState (msg))
			return eMouseStateApplyDrag;
		break;

	case eMouseStateRubberBand:
		// If this is a move: no state change
		// If this is a mouse-up for the select button: move to apply rubber band state
		if (HasExitedState (msg))
			return eMouseStateApplyRubberBand;
		break;

	case eMouseStateSelect:
		// If modifiers do not map to power select action, move to idle state
		if (HasExitedState (msg)) {
			// If click, move to apply power select state
			if (ButtonUpMatchesState (eMouseStateApplySelect, msg))
				return eMouseStateApplySelect;
			return eMouseStateIdle;
			}
		// Else no change
		break;

	case eMouseStateZoom:
		// If this is a mouse-up and mouse hasn't moved: move to ZoomIn/ZoomOut state
		if (HasExitedState (msg) && point == m_lastMousePos) {
			if (ButtonUpMatchesState (eMouseStateZoomIn, msg))
				return eMouseStateZoomIn;
			else if (ButtonUpMatchesState (eMouseStateZoomOut, msg))
				return eMouseStateZoomOut;
			}
	case eMouseStatePan:
	case eMouseStateRotate:
		// If the current state requires a mouse click and that button is down, no change
		if (!HasExitedState (msg))
			break;
		// Else, if input+modifiers map to a move state, switch to it
		// Check for exact matches first
		if (HasEnteredState (eMouseStateZoom, msg) == eMatchExact)
			return eMouseStateZoom;
		if (HasEnteredState (eMouseStatePan, msg) == eMatchExact)
			return eMouseStatePan;
		if (HasEnteredState (eMouseStateRotate, msg) == eMatchExact)
			return eMouseStateRotate;
		// No exact match, partial matches are now allowed
		if (HasEnteredState (eMouseStateZoom, msg))
			return eMouseStateZoom;
		if (HasEnteredState (eMouseStatePan, msg))
			return eMouseStatePan;
		if (HasEnteredState (eMouseStateRotate, msg))
			return eMouseStateRotate;
		// Else move to idle state
		return eMouseStateIdle;

	default:
		return eMouseStateIdle;
	}

return m_mouseState;
}

eMouseStateMatchResults CInputHandler::HasEnteredState (eMouseStates state, UINT msg) const
{
	eMouseStateMatchResults result = eMatchExact;

// Check modifiers
for (int i = 0; i < eModifierCount; i++) {
	bool bRequired = m_stateConfigs [state].modifiers [i];

	if (m_bModifierActive [i]) {
		if (!bRequired)
			result = eMatchPartial;
		}
	else if (bRequired) {
		result = eMatchNone;
		break;
		}
	}

if (result != eMatchNone) {
	// Check mouse button
	switch (msg) {
		case WM_LBUTTONDOWN:
			if (m_stateConfigs [state].button != MK_LBUTTON)
				result = eMatchNone;
			break;

		case WM_MBUTTONDOWN:
			if (m_stateConfigs [state].button != MK_MBUTTON)
				result = eMatchNone;
			break;

		case WM_RBUTTONDOWN:
			if (m_stateConfigs [state].button != MK_RBUTTON)
				result = eMatchNone;
			break;

		case (0x10000 | WM_XBUTTONDOWN):
			if (m_stateConfigs [state].button != MK_XBUTTON1)
				result = eMatchNone;
			break;

		case (0x20000 | WM_XBUTTONDOWN):
			if (m_stateConfigs [state].button != MK_XBUTTON2)
				result = eMatchNone;
			break;

		default:
			if (IsClickState (state))
				result = eMatchNone;
			break;
		}
	}

return result;
}

bool CInputHandler::HasExitedState (UINT msg) const
{
// Check modifiers
for (int i = 0; i < eModifierCount; i++) {
	bool bRequired = m_stateConfigs [m_mouseState].modifiers [i];

	if (bRequired && !m_bModifierActive [i])
		return true;
	}

// Check mouse button
return ButtonUpMatchesState (m_mouseState, msg);
}

bool CInputHandler::ButtonUpMatchesState (eMouseStates state, UINT msg) const
{
switch (msg) {
	case WM_LBUTTONUP:
		if (m_stateConfigs [state].button == MK_LBUTTON)
			return true;
		break;

	case WM_MBUTTONUP:
		if (m_stateConfigs [state].button == MK_MBUTTON)
			return true;
		break;

	case WM_RBUTTONUP:
		if (m_stateConfigs [state].button == MK_RBUTTON)
			return true;
		break;

	case (0x10000 | WM_XBUTTONUP):
		if (m_stateConfigs [state].button == MK_XBUTTON1)
			return true;
		break;

	case (0x20000 | WM_XBUTTONUP):
		if (m_stateConfigs [state].button == MK_XBUTTON2)
			return true;
		break;

	default:
		break;
	}

return false;
}

bool CInputHandler::IsClickState (eMouseStates state) const
{
	return m_stateConfigs [state].button != 0;
}

bool CInputHandler::CheckValidDragTarget (const CPoint point) const
{
// Only counts as a drag if it hits a vertex
int v = vertexManager.Index (current->Vertex ());
if ((abs (point.x - vertexManager[v].m_screen.x) < 5) &&
	(abs (point.y - vertexManager[v].m_screen.y) < 5))
	return true;
}

void CInputHandler::ProcessTransitionalStates (CPoint point)
{
// If the new state is a transitional state (i.e. we're releasing/committing an action),
// apply the corresponding action then reset the mouse state to idle
switch (m_mouseState) {
	case eMouseStateQuickSelect:
		// Do selection
		m_mouseState = eMouseStateIdle;
		break;

	case eMouseStateApplyDrag:
		m_pMineView->FinishDrag (point);
		m_mouseState = eMouseStateIdle;
		break;

	case eMouseStateApplyRubberBand:
		ApplyRubberBand (point);
		m_mouseState = eMouseStateIdle;
		break;

	case eMouseStateApplySelect:
		ApplySelect (point);
		m_mouseState = eMouseStateIdle;
		break;

	case eMouseStateZoomIn:
		m_pMineView->ZoomIn (1, true);
		m_mouseState = eMouseStateIdle;
		break;

	case eMouseStateZoomOut:
		m_pMineView->ZoomOut (1, true);
		m_mouseState = eMouseStateIdle;
		break;

	default:
		// No action needed
		break;
	}
}

void CInputHandler::UpdateMouseState (UINT msg, CPoint point)
{
eMouseStates newState = MapInputToMouseState (msg, point);

if (m_mouseState != newState) {
	m_mouseState = newState;
	ProcessTransitionalStates (point);
	m_pMineView->SetCursor (m_mouseState);

	// If this is a state that requires a mouse button down, record where it started.
	// Otherwise, clear the start location if there was one
	if (IsClickState (m_mouseState)) {
		if (m_clickStartPos == nullptr)
			m_clickStartPos = new CPoint (point);
		else
			*m_clickStartPos = point;
		}
	else if (m_clickStartPos != nullptr) {
		delete m_clickStartPos;
		m_clickStartPos = nullptr;
		}

	// Some special handling for zoom - we need to track where we started even if it
	// isn't a click state, because it uses stepping
	if (m_mouseState == eMouseStateZoom) {
		if (m_zoomStartPos == nullptr)
			m_zoomStartPos = new CPoint (point);
		else
			*m_zoomStartPos = point;
		}
	else if (m_zoomStartPos != nullptr) {
		delete m_zoomStartPos;
		m_zoomStartPos = nullptr;
		}
	}
}

void CInputHandler::UpdateMouseState (UINT msg)
{
eMouseStates newState = MapInputToMouseState (msg, m_lastMousePos);

if (m_mouseState != newState) {
	m_mouseState = newState;
	ProcessTransitionalStates (m_lastMousePos);
	m_pMineView->SetCursor (m_mouseState);
	}
}

void CInputHandler::UpdateModifierStates (UINT msg, UINT nChar, UINT nFlags)
{
switch (nChar) {
	case VK_SHIFT:
		m_bModifierActive [eModifierShift] = (msg == WM_KEYDOWN);
		break;

	case VK_CONTROL:
		m_bModifierActive [eModifierCtrl] = (msg == WM_KEYDOWN);
		break;

	case VK_MENU:
		m_bModifierActive[eModifierAlt] = (msg == WM_KEYDOWN);
		break;

	default:
		break;
	}
}

bool CInputHandler::IsMovementCommand (eKeyCommands command)
{
switch (command) {
	case eKeyCommandMoveForward:
	case eKeyCommandMoveBackward:
	case eKeyCommandMoveLeft:
	case eKeyCommandMoveRight:
	case eKeyCommandMoveUp:
	case eKeyCommandMoveDown:
	case eKeyCommandRotateUp:
	case eKeyCommandRotateDown:
	case eKeyCommandRotateLeft:
	case eKeyCommandRotateRight:
	case eKeyCommandRotateBankLeft:
	case eKeyCommandRotateBankRight:
	case eKeyCommandZoomIn:
	case eKeyCommandZoomOut:
		return true;

	default:
		return false;
	}
}

void CInputHandler::UpdateInputLockState (UINT msg, UINT nChar)
{
if (KeyMatchesKeyCommand (eKeyCommandInputLock, nChar))
	m_bInputLockActive = (msg == WM_KEYDOWN);
}

eKeyCommands CInputHandler::MapKeyToCommand (UINT nChar)
{
for (int command = 0; command < eKeyCommandCount; command++)
	if (KeyMatchesKeyCommand ((eKeyCommands)command, nChar))
		return (eKeyCommands)command;

return eKeyCommandUnknown;
}

bool CInputHandler::KeyMatchesKeyCommand (eKeyCommands command, UINT nChar)
{
if (command < 0 || command >= eKeyCommandCount)
	return false;

const KeyboardBinding &binding = m_keyBindings [command];

if (binding.bNeedsInputLock && !m_bInputLockActive)
	return false;

if (nChar != binding.nChar)
	return false;

// Check modifiers
for (int i = 0; i < eModifierCount; i++) {
	bool bRequired = binding.modifiers [i];

	if (bRequired && !m_bModifierActive [i])
		return false;
	}

return true;
}

void CInputHandler::DoMousePan (const CPoint point)
{
	CPoint change = m_lastMousePos - point;
	double scale = 0.5;
	double panAmountX = double(change.x) * scale;
	double panAmountY = -double(change.y) * scale;

if (m_pMineView->Perspective ())
	panAmountX = -panAmountX;
if (m_stateConfigs [eMouseStatePan].bInvertX)
	panAmountX = -panAmountX;
if (m_stateConfigs [eMouseStatePan].bInvertY)
	panAmountY = -panAmountY;

if (panAmountX != 0.0)
	m_pMineView->Pan ('X', panAmountX);
if (panAmountY != 0.0)
	m_pMineView->Pan ('Y', panAmountY);
}

void CInputHandler::DoMouseZoom (const CPoint point)
{
	CPoint zoomOffset = m_pMineView->Perspective () ?
		m_lastMousePos - point :
		point - *m_zoomStartPos;

if (m_stateConfigs [eMouseStateZoom].bInvertX)
	zoomOffset.x = -zoomOffset.x;
if (m_stateConfigs [eMouseStateZoom].bInvertY)
	zoomOffset.y = -zoomOffset.y;

if (m_pMineView->Perspective ()) {
	if ((zoomOffset.x > 0) || ((zoomOffset.x == 0) && (zoomOffset.y > 0))) {
		m_pMineView->ZoomOut (1, true);
		}
	else if ((zoomOffset.x < 0) || ((zoomOffset.x == 0) && (zoomOffset.y < 0))) {
		m_pMineView->ZoomIn (1, true);
		}
	}
else {
	int nChange = zoomOffset.x + zoomOffset.y;
	if (nChange > 30) {
		*m_zoomStartPos = point;
		m_pMineView->ZoomIn (1, true);
		}
	else if (nChange < -30) {
		*m_zoomStartPos = point;
		m_pMineView->ZoomOut (1, true);
		}
	}
}

void CInputHandler::DoMouseRotate (const CPoint point)
{
	CPoint change = m_lastMousePos - point;
	double scale = m_pMineView->Perspective () ? 300.0 : 200.0;
	double rotateAmountX = double(change.y) / scale;
	double rotateAmountY = -double(change.x) / scale;

if (m_pMineView->Perspective ())
	rotateAmountX = -rotateAmountX;
if (m_stateConfigs [eMouseStateRotate].bInvertX)
	rotateAmountX = -rotateAmountX;
if (m_stateConfigs [eMouseStateRotate].bInvertY)
	rotateAmountY = -rotateAmountY;

m_pMineView->Rotate ('Y', rotateAmountY);
m_pMineView->Rotate ('X', rotateAmountX);
}
