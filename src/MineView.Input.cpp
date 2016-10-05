#include "mineview.h"

CInputHandler::CInputHandler ()
{
	m_lButtonStartPos = nullptr;
	m_mButtonStartPos = nullptr;
	m_rButtonStartPos = nullptr;
	m_x1ButtonStartPos = nullptr;
	m_x2ButtonStartPos = nullptr;
}

CInputHandler::~CInputHandler ()
{
	delete m_lButtonStartPos;
	m_lButtonStartPos = nullptr;
	delete m_mButtonStartPos;
	m_mButtonStartPos = nullptr;
	delete m_rButtonStartPos;
	m_rButtonStartPos = nullptr;
	delete m_x1ButtonStartPos;
	m_x1ButtonStartPos = nullptr;
	delete m_x2ButtonStartPos;
	m_x2ButtonStartPos = nullptr;
}

void CInputHandler::OnKeyUp (UINT nChar, UINT nRepCnt, UINT nFlags)
{
UpdateMouseState (WM_KEYUP, nFlags);
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
UpdateMouseState (WM_KEYDOWN, nFlags);
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
UpdateMouseState (WM_MOUSEMOVE, nFlags, point);
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
		}
	}

m_lastMousePos = point;
}

void CInputHandler::OnLButtonUp (UINT nFlags, CPoint point)
{
m_clickState = nFlags;
// button ups - may just be an action, not a state, if the mouse hasn't moved
UpdateMouseState (WM_LBUTTONUP, nFlags, point);
}

void CInputHandler::OnLButtonDown (UINT nFlags, CPoint point)
{
m_clickState = nFlags;
UpdateMouseState (WM_LBUTTONDOWN, nFlags, point);
}

void CInputHandler::OnRButtonUp (UINT nFlags, CPoint point)
{
m_clickState = nFlags;
UpdateMouseState (WM_RBUTTONUP, nFlags, point);
}

void CInputHandler::OnRButtonDown (UINT nFlags, CPoint point)
{
m_clickState = nFlags;
UpdateMouseState (WM_RBUTTONDOWN, nFlags, point);
}

void CInputHandler::OnMButtonUp (UINT nFlags, CPoint point)
{
m_clickState = nFlags;
UpdateMouseState (WM_MBUTTONUP, nFlags, point);
}

void CInputHandler::OnMButtonDown (UINT nFlags, CPoint point)
{
m_clickState = nFlags;
UpdateMouseState (WM_MBUTTONDOWN, nFlags, point);
}

void CInputHandler::OnXButtonUp (UINT nFlags, UINT nButton, CPoint point)
{
// Does this get wiped by e.g. OnLButtonDown?
m_clickState = nFlags;
// this is a bit weird but testing nFlags would be unreliable -
// if both xbuttons are pressed it'll just see the one it checks first
UpdateMouseState ((nButton << 16) | WM_XBUTTONUP, nFlags, point);
}

void CInputHandler::OnXButtonDown (UINT nFlags, UINT nButton, CPoint point)
{
m_clickState = nFlags;
UpdateMouseState ((nButton << 16) | WM_XBUTTONDOWN, nFlags, point);
}

void CInputHandler::OnMouseWheel (UINT nFlags, short zDelta, CPoint pt)
{
}

//------------------------------------------------------------------------------
// Internal functions

eMouseStates CInputHandler::MapInputToMouseState (UINT msg, UINT nFlags, CPoint point)
{
switch (m_mouseState) {
	case eMouseStateIdle:
		// If modifiers map to power select action, move to power select state
		// Else, if input+modifiers map to select action
			// Valid target selected: move to select pending (ButtonDown), set target
			// No target selected: Move to rubber band
		// Else, if input+modifiers map to move action: move to corresponding move state (pan/rotate/zoom)
		// Otherwise do nothing
		break;

	case eMouseStateButtonDown:
		// If this is a move: move to either drag or rubber band state, depending on whether there is a target
		// If this is a mouse-up: move to quick select state
		// Ignore other inputs (other mouse buttons, modifier changes)
		break;

	case eMouseStateDrag:
		// If this is a move: no state change
		// If this is a mouse-up for the select button: move to apply drag state
		break;

	case eMouseStateRubberBand:
		// If this is a move: no state change
		// If this is a mouse-up for the select button: move to apply rubber band state
		break;

	case eMouseStateSelect:
		// If modifiers do not map to power select action, move to idle state
		// If click, move to apply power select state
		// Else no change
		break;

	case eMouseStateZoom:
		// If this is a mouse-up and mouse hasn't moved: move to ZoomIn/ZoomOut state
	case eMouseStatePan:
	case eMouseStateRotate:
		// If the current state requires a mouse click and that button is down, no change
		// Else, if input+modifiers map to a move state, switch to it
		// Else move to idle state
		break;

	default:
		break;
	}

	// If this is a click, record the start location of the click

	//need to ignore click state unless the mouse has moved since that button went down

//// We check drag first since it collides with rubber band by default
//if (MouseInputMatchesState (eMouseStateDrag, nFlags)) {
//	// Only counts as a drag if it hits a vertex
//	int v = vertexManager.Index (current->Vertex());
//	if ((abs (point.x - vertexManager [v].m_screen.x) < 5) &&
//		 (abs (point.y - vertexManager [v].m_screen.y) < 5))
//		return eMouseStateDrag;
//	}
//
//for (int mouseState = 0; mouseState < eMouseStateCount; mouseState++) {
//	if (mouseState == eMouseStateDrag)
//		continue;
//	if (MouseInputMatchesState ((eMouseStates)mouseState, nFlags))
//		return (eMouseStates)mouseState;
//	}

return eMouseStateIdle; // matches nothing specific, so we're not doing anything
}

void CInputHandler::UpdateMouseState (UINT msg, UINT nFlags, CPoint point)
{
	eMouseStates newState = MapInputToMouseState (msg, nFlags, point);

// Take actions on releasing last state (also in certain modes even when the state hasn't changed)
switch (newState) {
	// Transitional states: apply them then reset to idle
	case eMouseStateQuickSelect:
		// Do selection
		newState = eMouseStateIdle;
		break;

	case eMouseStateApplyDrag:
		ApplyDrag (point);
		newState = eMouseStateIdle;
		break;

	case eMouseStateRubberBand:
		ApplyRubberBand (point);
		newState = eMouseStateIdle;
		break;

	case eMouseStateApplySelect:
		ApplySelect (point);
		newState = eMouseStateIdle;
		break;

	case eMouseStateZoomIn:
		// do zoom in
		newState = eMouseStateIdle;
		break;

	case eMouseStateZoomOut:
		// do zoom out
		newState = eMouseStateIdle;
		break;

	// Move states (we might already be handling these through OnMouseMove? Consider this)
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

	default:
		// No action needed
		break;
	}

// If this is a button down, record where it started. If this is a button up, clear the start location
switch (msg) {
	case WM_LBUTTONDOWN:
		if (m_lButtonStartPos == nullptr) {
			m_lButtonStartPos = new CPoint (point);
			}
		break;

	case WM_LBUTTONUP:
		if (m_lButtonStartPos != nullptr) {
			delete m_lButtonStartPos;
			m_lButtonStartPos = nullptr;
			}
		break;

	case WM_MBUTTONDOWN:
		if (m_mButtonStartPos == nullptr) {
			m_mButtonStartPos = new CPoint (point);
			}
		break;

	case WM_MBUTTONUP:
		if (m_mButtonStartPos != nullptr) {
			delete m_mButtonStartPos;
			m_mButtonStartPos = nullptr;
			}
		break;

	case WM_RBUTTONDOWN:
		if (m_rButtonStartPos == nullptr) {
			m_rButtonStartPos = new CPoint (point);
			}
		break;

	case WM_RBUTTONUP:
		if (m_rButtonStartPos != nullptr) {
			delete m_rButtonStartPos;
			m_rButtonStartPos = nullptr;
			}
		break;

	case (0x10000 | WM_XBUTTONDOWN):
		if (m_x1ButtonStartPos == nullptr) {
			m_x1ButtonStartPos = new CPoint (point);
			}
		break;

	case (0x10000 | WM_XBUTTONUP):
		if (m_x1ButtonStartPos != nullptr) {
			delete m_x1ButtonStartPos;
			m_x1ButtonStartPos = nullptr;
			}
		break;

	case (0x20000 | WM_XBUTTONDOWN):
		if (m_x2ButtonStartPos == nullptr) {
			m_x2ButtonStartPos = new CPoint (point);
			}
		break;

	case (0x20000 | WM_XBUTTONUP):
		if (m_x2ButtonStartPos != nullptr) {
			delete m_x2ButtonStartPos;
			m_x2ButtonStartPos = nullptr;
			}
		break;

	default:
		break;
	}

if (m_mouseState != newState) {
	m_mouseState = newState;
	UpdateCursor (newState);
	}
m_lastMousePos = point;
}
