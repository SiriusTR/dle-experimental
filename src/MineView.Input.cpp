#include "mineview.h"

CInputHandler::CInputHandler ()
{
m_clickStartPos = nullptr;
}

CInputHandler::~CInputHandler ()
{
if (m_clickStartPos != nullptr) {
	delete m_clickStartPos;
	m_clickStartPos = nullptr;
	}
}

void CInputHandler::OnKeyUp (UINT nChar, UINT nRepCnt, UINT nFlags)
{
UpdateModifierStates (WM_KEYUP, nChar, nFlags);
UpdateMouseState (WM_KEYUP);
if (!IsMovementKey (nChar))
	return;

movementDirection = MapKeyToDirection (nChar);

switch (m_movementMode) {
	case eMovementModeStepped:
		for (int i = 0; i < nRepCnt; i++) {
			ApplyMovement (movementDirection);
			}
		break;
	case eMovementModeContinuous:
		StopMovement (movementDirection);
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
if (!IsMovementKey (nChar))
	return;

movementDirection = MapKeyToDirection (nChar);

switch (m_movementMode) {
	case eMovementModeStepped:
		for (int i = 0; i < nRepCnt; i++) {
			ApplyMovement (movementDirection);
			}
		break;
	case eMovementModeContinuous:
		StartMovement (movementDirection);
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

		case eMouseStatePan:
			double scale = m_nRenderer ? 0.5 : 1.0;
			double d = double (change.x) * scale;
			if (d != 0.0)
				Pan ('X', Perspective() ? -d : d);
			d = double (change.y) * scale;
			if (d != 0.0)
				Pan ('Y', -d);
			break;

		case eMouseStateRotate:
			double scale = Perspective() ? 300.0 : 200.0;
			Rotate ('Y', -double (change.x) / scale);
			Rotate ('X', (m_nRenderer && !Perspective()) ? double (change.y) / scale : -double (change.y) / scale);
			break;

		case eMouseStateSelect:
			if (SelectMode (eSelectPoint) || SelectMode (eSelectLine) || SelectMode (eSelectSide) || SelectMode (eSelectSegment))
				Invalidate (FALSE);
			break;

		case eMouseStateDrag:
			UpdateDragPos ();
			break;

		case eMouseStateZoom:
			if (Perspective ()) {
				if ((change.x > 0) || ((change.x == 0) && (change.y > 0))) {
					ZoomOut (1, true);
					}
				else if ((change.x < 0) || ((change.x == 0) && (change.y < 0))) {
					ZoomIn (1, true);
					}
				}
			else {
				// This is pretty ugly, can we do better?
				// Also won't work when this state just started because it doesn't reset nChange
				static int nChange = 0;
				if ((change.x < 0) || ((change.x == 0) && (change.y < 0))) {
					if (nChange > 0)
						nChange = 0;
					nChange += change.x ? change.x : change.y;
					if (nChange < -30) {
						nChange = 0;
						ZoomIn (1, true);
						}
					}
				else if ((change.x > 0) || ((change.x == 0) && (change.y > 0))) {
					if (nChange < 0)
						nChange = 0;
					nChange += change.x ? change.x : change.y;
					if (nChange > 30) {
						nChange = 0;
						ZoomOut (1, true);
						}
					}
				}
			break;
			
		case eMouseStateRubberBand:
			UpdateRubberRect (point);
			break;

		case eMouseStateSelect:
			UpdateSelectHighlights (point);
			break;

		case eMouseStateZoom:
			ApplyZoom (point);
			break;

		case eMouseStatePan:
			ApplyPan (point);
			break;

		case eMouseStateRotate:
			ApplyRotate (point);
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
		ApplyDrag (point);
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
		ZoomIn (1, true);
		m_mouseState = eMouseStateIdle;
		break;

	case eMouseStateZoomOut:
		ZoomOut (1, true);
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
	UpdateCursor (m_mouseState);

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
	}

m_lastMousePos = point;
}

void CInputHandler::UpdateMouseState (UINT msg)
{
eMouseStates newState = MapInputToMouseState (msg, m_lastMousePos);

if (m_mouseState != newState) {
	m_mouseState = newState;
	ProcessTransitionalStates (m_lastMousePos);
	UpdateCursor (m_mouseState);
	}
}
