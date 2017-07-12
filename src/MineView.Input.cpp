#include "mineview.h"
#include "dle-xp.h"

CInputHandler::CInputHandler (CMineView *pMineView)
	: m_pMineView (pMineView)
{
m_clickStartPos = nullptr;
m_zoomStartPos = nullptr;
for (int i = 0; i < eModifierCount; i++) {
	m_bModifierActive [i] = false;
	}
for (int i = 0; i < eKeyCommandCount; i++) {
	m_bKeyCommandActive [i] = false;
	}
m_bInputLockActive = false;
m_nMovementCommandsActive = 0;
ZeroMemory (m_keyBindings, sizeof (m_keyBindings));
ZeroMemory (m_stateConfigs, sizeof (m_stateConfigs));
LoadSettings ();
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

void CInputHandler::LoadSettings()
{
// Set default settings where applicable
m_stateConfigs [eMouseStateButtonDown].button = MK_LBUTTON | MK_RBUTTON;
m_stateConfigs [eMouseStateSelect].modifiers [eModifierShift] = true;
m_stateConfigs [eMouseStatePan].modifiers [eModifierCtrl] = true;
m_stateConfigs [eMouseStateRotate].modifiers [eModifierShift] = true;
m_stateConfigs [eMouseStateRotate].modifiers [eModifierCtrl] = true;
m_stateConfigs [eMouseStateZoom].button = MK_LBUTTON | MK_RBUTTON;
m_stateConfigs [eMouseStateZoom].modifiers [eModifierCtrl] = true;
m_stateConfigs [eMouseStateZoomIn].button = MK_LBUTTON;
m_stateConfigs [eMouseStateZoomOut].button = MK_RBUTTON;
m_stateConfigs [eMouseStateApplySelect].button = MK_LBUTTON;
m_stateConfigs [eMouseStateTagRubberBand].button = MK_LBUTTON | MK_RBUTTON;
m_stateConfigs [eMouseStateUnTagRubberBand].button = MK_LBUTTON | MK_RBUTTON;
m_stateConfigs [eMouseStateUnTagRubberBand].modifiers [eModifierShift] = true;
m_stateConfigs [eMouseStateQuickTag].button = MK_LBUTTON;
m_stateConfigs [eMouseStateQuickTag].modifiers [eModifierShift] = true;
m_stateConfigs [eMouseStateDoContextMenu].button = MK_RBUTTON;
m_stateConfigs [eMouseStateDoContextMenu].modifiers [eModifierShift] = true;

LoadKeyBinding (m_keyBindings [eKeyCommandMoveForward], "MoveForward");
LoadKeyBinding (m_keyBindings [eKeyCommandMoveBackward], "MoveBackward");
LoadKeyBinding (m_keyBindings [eKeyCommandMoveLeft], "MoveLeft");
LoadKeyBinding (m_keyBindings [eKeyCommandMoveRight], "MoveRight");
LoadKeyBinding (m_keyBindings [eKeyCommandMoveUp], "MoveUp");
LoadKeyBinding (m_keyBindings [eKeyCommandMoveDown], "MoveDown");
LoadKeyBinding (m_keyBindings [eKeyCommandRotateUp], "RotateUp");
LoadKeyBinding (m_keyBindings [eKeyCommandRotateDown], "RotateDown");
LoadKeyBinding (m_keyBindings [eKeyCommandRotateLeft], "RotateLeft");
LoadKeyBinding (m_keyBindings [eKeyCommandRotateRight], "RotateRight");
LoadKeyBinding (m_keyBindings [eKeyCommandRotateBankLeft], "BankLeft");
LoadKeyBinding (m_keyBindings [eKeyCommandRotateBankRight], "BankRight");
LoadKeyBinding (m_keyBindings [eKeyCommandZoomIn], "ZoomIn");
LoadKeyBinding (m_keyBindings [eKeyCommandZoomOut], "ZoomOut");
LoadKeyBinding (m_keyBindings [eKeyCommandInputLock], "InputLock");

// Idle and ApplyDrag states don't need configs due to transition rules
LoadStateConfig (m_stateConfigs [eMouseStateButtonDown], "ButtonDown");
LoadStateConfig (m_stateConfigs [eMouseStateSelect], "AdvSelect");
LoadStateConfig (m_stateConfigs [eMouseStatePan], "Pan");
LoadStateConfig (m_stateConfigs [eMouseStateRotate], "Rotate");
LoadStateConfig (m_stateConfigs [eMouseStateZoom], "Zoom");
LoadStateConfig (m_stateConfigs [eMouseStateZoomIn], "ZoomIn");
LoadStateConfig (m_stateConfigs [eMouseStateZoomOut], "ZoomOut");
LoadStateConfig (m_stateConfigs [eMouseStateQuickSelect], "QuickSelect");
LoadStateConfig (m_stateConfigs [eMouseStateApplySelect], "ApplyAdvSelect");
LoadStateConfig (m_stateConfigs [eMouseStateTagRubberBand], "Mark");
LoadStateConfig (m_stateConfigs [eMouseStateUnTagRubberBand], "Unmark");
LoadStateConfig (m_stateConfigs [eMouseStateQuickTag], "QuickMark");
LoadStateConfig (m_stateConfigs [eMouseStateDoContextMenu], "ContextMenu");

// Drag and RubberBand are copied from ButtonDown
memcpy_s (&m_stateConfigs [eMouseStateDrag], sizeof (m_stateConfigs [eMouseStateDrag]),
	&m_stateConfigs [eMouseStateButtonDown], sizeof (m_stateConfigs [eMouseStateButtonDown]));
memcpy_s (&m_stateConfigs [eMouseStateRubberBand], sizeof (m_stateConfigs [eMouseStateRubberBand]),
	&m_stateConfigs [eMouseStateButtonDown], sizeof (m_stateConfigs [eMouseStateButtonDown]));
}

void CInputHandler::UpdateMovement (double timeElapsed)
{
	double scale = 1.0;
	// MineView/Renderer already apply view move rate
	double moveAmount = timeElapsed * scale;
	static double zoomAmount = 0;

if (m_bKeyCommandActive [eKeyCommandMoveForward])
	m_pMineView->Pan ('Z', moveAmount);
if (m_bKeyCommandActive [eKeyCommandMoveBackward])
	m_pMineView->Pan ('Z', -moveAmount);
if (m_bKeyCommandActive [eKeyCommandMoveLeft])
	m_pMineView->Pan ('X', -moveAmount);
if (m_bKeyCommandActive [eKeyCommandMoveRight])
	m_pMineView->Pan ('X', moveAmount);
if (m_bKeyCommandActive [eKeyCommandMoveUp])
	m_pMineView->Pan ('Y', moveAmount);
if (m_bKeyCommandActive [eKeyCommandMoveDown])
	m_pMineView->Pan ('Y', -moveAmount);
if (m_bKeyCommandActive [eKeyCommandRotateUp])
	m_pMineView->Rotate ('X', moveAmount);
if (m_bKeyCommandActive [eKeyCommandRotateDown])
	m_pMineView->Rotate ('X', -moveAmount);
if (m_bKeyCommandActive [eKeyCommandRotateLeft])
	m_pMineView->Rotate ('Y', moveAmount);
if (m_bKeyCommandActive [eKeyCommandRotateRight])
	m_pMineView->Rotate ('Y', -moveAmount);
if (m_bKeyCommandActive [eKeyCommandRotateBankLeft])
	m_pMineView->Rotate ('Z', moveAmount);
if (m_bKeyCommandActive [eKeyCommandRotateBankRight])
	m_pMineView->Rotate ('Z', -moveAmount);

if (m_bKeyCommandActive [eKeyCommandZoomIn])
	zoomAmount += moveAmount;
if (m_bKeyCommandActive [eKeyCommandZoomOut])
	zoomAmount -= moveAmount;
if (fabs (zoomAmount) >= 1) {
	m_pMineView->ZoomIn ((int)zoomAmount);
	zoomAmount -= (int)zoomAmount;
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
		for (unsigned int i = 0; i < nRepCnt; i++) {
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
		for (unsigned int i = 0; i < nRepCnt; i++) {
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
			// In default config, RMB can be used for drag/rubber band like this.
			// Can recheck input if this becomes a problem
			if (CheckValidDragTarget (*m_clickStartPos))
				return eMouseStateDrag;
			else
				return eMouseStateRubberBand;
			}
		if (HasExitedState (msg)) {
			// If this is a mouse-up: move to quick select state
			if (HasEnteredTransitionalState (eMouseStateQuickSelect, msg) == eMatchExact)
				return eMouseStateQuickSelect;
			if (HasEnteredTransitionalState (eMouseStateQuickTag, msg) == eMatchExact)
				return eMouseStateQuickTag;
			if (HasEnteredTransitionalState (eMouseStateDoContextMenu, msg) == eMatchExact)
				return eMouseStateDoContextMenu;

			if (HasEnteredTransitionalState (eMouseStateQuickSelect, msg))
				return eMouseStateQuickSelect;
			if (HasEnteredTransitionalState (eMouseStateQuickTag, msg))
				return eMouseStateQuickTag;
			if (HasEnteredTransitionalState (eMouseStateDoContextMenu, msg))
				return eMouseStateDoContextMenu;

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
		if (HasExitedState (msg)) {
			if (HasEnteredTransitionalState (eMouseStateTagRubberBand, msg) == eMatchExact)
				return eMouseStateTagRubberBand;
			if (HasEnteredTransitionalState (eMouseStateUnTagRubberBand, msg) == eMatchExact)
				return eMouseStateUnTagRubberBand;

			if (HasEnteredTransitionalState (eMouseStateTagRubberBand, msg))
				return eMouseStateTagRubberBand;
			if (HasEnteredTransitionalState (eMouseStateUnTagRubberBand, msg))
				return eMouseStateUnTagRubberBand;

			return eMouseStateIdle;
			}
		break;

	case eMouseStateSelect:
		// If modifiers do not map to power select action, move to idle state
		if (HasExitedState (msg)) {
			// If click, move to apply power select state
			if (HasEnteredTransitionalState (eMouseStateApplySelect, msg))
				return eMouseStateApplySelect;
			return eMouseStateIdle;
			}
		// Else no change
		break;

	case eMouseStateZoom:
		// If this is a mouse-up and mouse hasn't moved: move to ZoomIn/ZoomOut state
		if (HasExitedState (msg) && point == m_lastMousePos) {
			if (HasEnteredTransitionalState (eMouseStateZoomIn, msg) == eMatchExact)
				return eMouseStateZoomIn;
			if (HasEnteredTransitionalState (eMouseStateZoomOut, msg) == eMatchExact)
				return eMouseStateZoomOut;

			if (HasEnteredTransitionalState (eMouseStateZoomIn, msg))
				return eMouseStateZoomIn;
			if (HasEnteredTransitionalState (eMouseStateZoomOut, msg))
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
	// Using bitwise comparisons to allow states to have multiple buttons mapping to them
	switch (msg) {
		case WM_LBUTTONDOWN:
			if (!(m_stateConfigs [state].button & MK_LBUTTON))
				result = eMatchNone;
			break;

		case WM_MBUTTONDOWN:
			if (!(m_stateConfigs [state].button & MK_MBUTTON))
				result = eMatchNone;
			break;

		case WM_RBUTTONDOWN:
			if (!(m_stateConfigs [state].button & MK_RBUTTON))
				result = eMatchNone;
			break;

		case (0x10000 | WM_XBUTTONDOWN):
			if (!(m_stateConfigs [state].button & MK_XBUTTON1))
				result = eMatchNone;
			break;

		case (0x20000 | WM_XBUTTONDOWN):
			if (!(m_stateConfigs [state].button & MK_XBUTTON2))
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

eMouseStateMatchResults CInputHandler::HasEnteredTransitionalState (eMouseStates state, UINT msg) const
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

// Check mouse button
if (!ButtonUpMatchesState (state, msg))
	result = eMatchNone;

return result;
}

bool CInputHandler::ButtonUpMatchesState (eMouseStates state, UINT msg) const
{
switch (msg) {
	// Using bitwise again. Note that this will allow any button up to leave a state
	// even if others are still held. Currently this is what we want but it may change later
	case WM_LBUTTONUP:
		if (m_stateConfigs [state].button & MK_LBUTTON)
			return true;
		break;

	case WM_MBUTTONUP:
		if (m_stateConfigs [state].button & MK_MBUTTON)
			return true;
		break;

	case WM_RBUTTONUP:
		if (m_stateConfigs [state].button & MK_RBUTTON)
			return true;
		break;

	case (0x10000 | WM_XBUTTONUP):
		if (m_stateConfigs [state].button & MK_XBUTTON1)
			return true;
		break;

	case (0x20000 | WM_XBUTTONUP):
		if (m_stateConfigs [state].button & MK_XBUTTON2)
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
		m_pMineView->SelectCurrentElement (point.x, point.y, false);
		m_mouseState = eMouseStateIdle;
		break;

	case eMouseStateApplyDrag:
		m_pMineView->FinishDrag (point);
		m_mouseState = eMouseStateIdle;
		break;

	case eMouseStateTagRubberBand:
	case eMouseStateUnTagRubberBand:
		m_pMineView->ResetRubberRect ();
		m_pMineView->TagRubberBandedVertices (*m_clickStartPos, point, m_mouseState == eMouseStateTagRubberBand);
		m_mouseState = eMouseStateIdle;
		break;

	case eMouseStateApplySelect:
		m_pMineView->SelectCurrentElement (point.x, point.y, false);
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

	case eMouseStateQuickTag:
		m_pMineView->SelectCurrentElement (point.x, point.y, true);
		m_mouseState = eMouseStateIdle;
		break;

	case eMouseStateDoContextMenu:
		m_pMineView->DoContextMenu (point);
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

void CInputHandler::ApplyMovement (eKeyCommands command)
{
if (!IsMovementCommand (command))
	return;

switch (command) {
	case eKeyCommandMoveForward:
		m_pMineView->Pan ('Z', 1);
		break;
	case eKeyCommandMoveBackward:
		m_pMineView->Pan ('Z', -1);
		break;
	case eKeyCommandMoveLeft:
		m_pMineView->Pan ('X', -1);
		break;
	case eKeyCommandMoveRight:
		m_pMineView->Pan ('X', 1);
		break;
	case eKeyCommandMoveUp:
		m_pMineView->Pan ('Y', 1);
		break;
	case eKeyCommandMoveDown:
		m_pMineView->Pan ('Y', -1);
		break;
	case eKeyCommandRotateUp:
		m_pMineView->Rotate ('X', 1);
		break;
	case eKeyCommandRotateDown:
		m_pMineView->Rotate ('X', -1);
		break;
	case eKeyCommandRotateLeft:
		m_pMineView->Rotate ('Y', 1);
		break;
	case eKeyCommandRotateRight:
		m_pMineView->Rotate ('Y', -1);
		break;
	case eKeyCommandRotateBankLeft:
		m_pMineView->Rotate ('Z', 1);
		break;
	case eKeyCommandRotateBankRight:
		m_pMineView->Rotate ('Z', -1);
		break;
	case eKeyCommandZoomIn:
		m_pMineView->ZoomIn ();
		break;
	case eKeyCommandZoomOut:
		m_pMineView->ZoomOut ();
		break;
	default:
		break;
	}
}

void CInputHandler::StartMovement (eKeyCommands command)
{
if (!IsMovementCommand (command))
	return;

m_bKeyCommandActive [command] = true;
m_nMovementCommandsActive++;
if (m_nMovementCommandsActive == 1)
	m_pMineView->StartMovementTimer ();
}

void CInputHandler::StopMovement (eKeyCommands command)
{
if (!IsMovementCommand (command))
	return;

m_bKeyCommandActive [command] = false;
m_nMovementCommandsActive--;
if (m_nMovementCommandsActive == 0)
	m_pMineView->StopMovementTimer ();
}

void CInputHandler::LoadKeyBinding (KeyboardBinding &binding, LPCTSTR bindingName)
{
	TCHAR szSettingNameChar [80] = { 0 };
	TCHAR szSettingNameModifiers [80] = { 0 };
	TCHAR szSettingNameInputLock [80] = { 0 };
	TCHAR szChar [80] = { 0 };
	TCHAR szMods [80] = { 0 };

_stprintf_s (szSettingNameChar, _T ("%sKey"), bindingName);
_stprintf_s (szSettingNameModifiers, _T ("%sModifiers"), bindingName);
_stprintf_s (szSettingNameInputLock, _T ("%sInputLock"), bindingName);

GetPrivateProfileString ("DLE.Bindings", szSettingNameChar, "", szChar, ARRAYSIZE (szChar), DLE.IniFile ());
GetPrivateProfileString ("DLE.Bindings", szSettingNameModifiers, "", szMods, ARRAYSIZE (szMods), DLE.IniFile ());
UINT nNeedsInputLock = GetPrivateProfileInt ("DLE.Bindings", szSettingNameInputLock, 0, DLE.IniFile ());

// Leave settings as default if not specified. Boolean values are currently always false by default
if (_tcslen (szChar) > 0)
	binding.nChar = StringToVK (szChar);
if (_tcslen (szMods) > 0)
	LoadModifiers (binding.modifiers, szMods);
binding.bNeedsInputLock = nNeedsInputLock > 0;
}

void CInputHandler::LoadStateConfig (MouseStateConfig &config, LPCTSTR bindingName)
{
	TCHAR szSettingNameButton [80] = { 0 };
	TCHAR szSettingNameModifiers [80] = { 0 };
	TCHAR szSettingNameToggleModifiers [80] = { 0 };
	TCHAR szSettingNameInvertX [80] = { 0 };
	TCHAR szSettingNameInvertY [80] = { 0 };
	TCHAR szButton [80] = { 0 };
	TCHAR szMods [80] = { 0 };

_stprintf_s (szSettingNameButton, _T ("%sButton"), bindingName);
_stprintf_s (szSettingNameModifiers, _T ("%sModifiers"), bindingName);
_stprintf_s (szSettingNameToggleModifiers, _T ("%sToggleModifiers"), bindingName);
_stprintf_s (szSettingNameInvertX, _T ("%sInvertX"), bindingName);
_stprintf_s (szSettingNameInvertY, _T ("%sInvertY"), bindingName);

GetPrivateProfileString ("DLE.Bindings", szSettingNameButton, "", szButton, ARRAYSIZE (szButton), DLE.IniFile ());
GetPrivateProfileString ("DLE.Bindings", szSettingNameModifiers, "", szMods, ARRAYSIZE (szMods), DLE.IniFile ());
UINT nToggleModifiers = GetPrivateProfileInt ("DLE.Bindings", szSettingNameToggleModifiers, 0, DLE.IniFile ());
UINT nInvertX = GetPrivateProfileInt ("DLE.Bindings", szSettingNameInvertX, 0, DLE.IniFile ());
UINT nInvertY = GetPrivateProfileInt ("DLE.Bindings", szSettingNameInvertY, 0, DLE.IniFile ());

// Leave settings as default if not specified. Boolean values are currently always false by default
if (_tcslen (szButton) > 0)
	config.button = StringToMK (szButton);
if (_tcslen (szMods) > 0)
	LoadModifiers (config.modifiers, szMods);
config.bToggleModifiers = nToggleModifiers > 0;
config.bInvertX = nInvertX > 0;
config.bInvertY = nInvertY > 0;
}

void CInputHandler::LoadModifiers (bool (&modifierList) [eModifierCount], LPTSTR szMods)
{
	LPCTSTR delimiters = _T (",");
	LPTSTR context = NULL;
	LPTSTR pszNext = NULL;

pszNext = _tcstok_s (szMods, delimiters, &context);
while (pszNext != NULL) {
	switch (StringToVK (pszNext)) {
		case VK_SHIFT:
			modifierList [eModifierShift] = true;
			break;
		case VK_CONTROL:
			modifierList [eModifierCtrl] = true;
			break;
		case VK_MENU:
			modifierList [eModifierAlt] = true;
			break;
		default:
			break;
		}
	pszNext = _tcstok_s (szMods, delimiters, &context);
	}
}

#define VK_TABLE_ENTRY(vk) \
	{ vk, _T (#vk) }
#define VK_TABLE_ENTRY_NUM(vk) \
	{ 0x30 + vk, _T (#vk) }
#define VK_TABLE_ENTRY_ALPHA(vk) \
	{ 0x41 + (#@vk - 'A'), _T (#vk) }
typedef struct {
	UINT vk;
	LPCTSTR szKey;
	} VKMapTableEntry;

UINT CInputHandler::StringToVK (LPCTSTR pszKey)
{
	static const VKMapTableEntry table [] = {
		VK_TABLE_ENTRY (VK_LBUTTON),
		VK_TABLE_ENTRY (VK_RBUTTON),
		VK_TABLE_ENTRY (VK_MBUTTON),
		VK_TABLE_ENTRY (VK_XBUTTON1),
		VK_TABLE_ENTRY (VK_XBUTTON2),
		VK_TABLE_ENTRY (VK_BACK),
		VK_TABLE_ENTRY (VK_TAB),
		VK_TABLE_ENTRY (VK_RETURN),
		VK_TABLE_ENTRY (VK_SHIFT),
		VK_TABLE_ENTRY (VK_CONTROL),
		VK_TABLE_ENTRY (VK_MENU),
		VK_TABLE_ENTRY (VK_ESCAPE),
		VK_TABLE_ENTRY (VK_SPACE),
		VK_TABLE_ENTRY (VK_PRIOR),
		VK_TABLE_ENTRY (VK_NEXT),
		VK_TABLE_ENTRY (VK_END),
		VK_TABLE_ENTRY (VK_HOME),
		VK_TABLE_ENTRY (VK_LEFT),
		VK_TABLE_ENTRY (VK_UP),
		VK_TABLE_ENTRY (VK_RIGHT),
		VK_TABLE_ENTRY (VK_DOWN),
		VK_TABLE_ENTRY (VK_INSERT),
		VK_TABLE_ENTRY (VK_DELETE),
		VK_TABLE_ENTRY_NUM (0),
		VK_TABLE_ENTRY_NUM (1),
		VK_TABLE_ENTRY_NUM (2),
		VK_TABLE_ENTRY_NUM (3),
		VK_TABLE_ENTRY_NUM (4),
		VK_TABLE_ENTRY_NUM (5),
		VK_TABLE_ENTRY_NUM (6),
		VK_TABLE_ENTRY_NUM (7),
		VK_TABLE_ENTRY_NUM (8),
		VK_TABLE_ENTRY_NUM (9),
		VK_TABLE_ENTRY_ALPHA (A),
		VK_TABLE_ENTRY_ALPHA (B),
		VK_TABLE_ENTRY_ALPHA (C),
		VK_TABLE_ENTRY_ALPHA (D),
		VK_TABLE_ENTRY_ALPHA (E),
		VK_TABLE_ENTRY_ALPHA (F),
		VK_TABLE_ENTRY_ALPHA (G),
		VK_TABLE_ENTRY_ALPHA (H),
		VK_TABLE_ENTRY_ALPHA (I),
		VK_TABLE_ENTRY_ALPHA (J),
		VK_TABLE_ENTRY_ALPHA (K),
		VK_TABLE_ENTRY_ALPHA (L),
		VK_TABLE_ENTRY_ALPHA (M),
		VK_TABLE_ENTRY_ALPHA (N),
		VK_TABLE_ENTRY_ALPHA (O),
		VK_TABLE_ENTRY_ALPHA (P),
		VK_TABLE_ENTRY_ALPHA (Q),
		VK_TABLE_ENTRY_ALPHA (R),
		VK_TABLE_ENTRY_ALPHA (S),
		VK_TABLE_ENTRY_ALPHA (T),
		VK_TABLE_ENTRY_ALPHA (U),
		VK_TABLE_ENTRY_ALPHA (V),
		VK_TABLE_ENTRY_ALPHA (W),
		VK_TABLE_ENTRY_ALPHA (X),
		VK_TABLE_ENTRY_ALPHA (Y),
		VK_TABLE_ENTRY_ALPHA (Z),
		VK_TABLE_ENTRY (VK_NUMPAD0),
		VK_TABLE_ENTRY (VK_NUMPAD1),
		VK_TABLE_ENTRY (VK_NUMPAD2),
		VK_TABLE_ENTRY (VK_NUMPAD3),
		VK_TABLE_ENTRY (VK_NUMPAD4),
		VK_TABLE_ENTRY (VK_NUMPAD5),
		VK_TABLE_ENTRY (VK_NUMPAD6),
		VK_TABLE_ENTRY (VK_NUMPAD7),
		VK_TABLE_ENTRY (VK_NUMPAD8),
		VK_TABLE_ENTRY (VK_NUMPAD9),
		VK_TABLE_ENTRY (VK_MULTIPLY),
		VK_TABLE_ENTRY (VK_ADD),
		VK_TABLE_ENTRY (VK_SUBTRACT),
		VK_TABLE_ENTRY (VK_DECIMAL),
		VK_TABLE_ENTRY (VK_DIVIDE)
		};

for (size_t i = 0; i < ARRAYSIZE (table); i++)
	if (_tcsicmp (pszKey, table [i].szKey) == 0)
		return table [i].vk;
return 0;
}

#define MK_TABLE_ENTRY(mk) \
	{ MK_##mk, _T (#mk) }
typedef struct {
	UINT mk;
	LPCTSTR szButton;
	} MKMapTableEntry;

UINT CInputHandler::StringToMK (LPCTSTR pszButton)
{
	static const MKMapTableEntry table [] = {
		MK_TABLE_ENTRY (LBUTTON),
		MK_TABLE_ENTRY (MBUTTON),
		MK_TABLE_ENTRY (RBUTTON),
		MK_TABLE_ENTRY (XBUTTON1),
		MK_TABLE_ENTRY (XBUTTON2)
		};

for (size_t i = 0; i < ARRAYSIZE (table); i++)
	if (_tcsicmp (pszButton, table [i].szButton) == 0)
		return table [i].mk;
return 0;
}
