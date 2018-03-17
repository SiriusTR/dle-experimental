#include "mineview.h"
#include "dle-xp.h"

CInputHandler::CInputHandler (CMineView *pMineView)
	: m_pMineView (pMineView)
{
m_stateStartPos = nullptr;
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
}

CInputHandler::~CInputHandler ()
{
if (m_stateStartPos != nullptr) {
	delete m_stateStartPos;
	m_stateStartPos = nullptr;
	}
if (m_zoomStartPos != nullptr) {
	delete m_zoomStartPos;
	m_zoomStartPos = nullptr;
	}
}

void CInputHandler::LoadSettings ()
{
// Set default settings where applicable
m_stateConfigs [eMouseStateSelect].modifiers [eModifierShift] = true;
m_stateConfigs [eMouseStatePan].modifiers [eModifierCtrl] = true;
m_stateConfigs [eMouseStateRotate].modifiers [eModifierShift] = true;
m_stateConfigs [eMouseStateRotate].modifiers [eModifierCtrl] = true;
m_stateConfigs [eMouseStateZoomIn].button = MK_LBUTTON;
m_stateConfigs [eMouseStateZoomIn].modifiers [eModifierCtrl] = true;
m_stateConfigs [eMouseStateZoomOut].button = MK_RBUTTON;
m_stateConfigs [eMouseStateZoomOut].modifiers [eModifierCtrl] = true;
m_stateConfigs [eMouseStateQuickSelect].button = MK_LBUTTON;
m_stateConfigs [eMouseStateQuickSelectObject].button = MK_RBUTTON;
m_stateConfigs [eMouseStateRubberBandTag].button = MK_LBUTTON | MK_RBUTTON;
m_stateConfigs [eMouseStateRubberBandUnTag].button = MK_LBUTTON | MK_RBUTTON;
m_stateConfigs [eMouseStateRubberBandUnTag].modifiers [eModifierShift] = true;
m_stateConfigs [eMouseStateQuickTag].button = MK_LBUTTON;
m_stateConfigs [eMouseStateQuickTag].modifiers [eModifierShift] = true;
m_stateConfigs [eMouseStateDoContextMenu].button = MK_RBUTTON;
m_stateConfigs [eMouseStateDoContextMenu].modifiers [eModifierShift] = true;

// Read camera movement settings if specified
m_movementMode = (eMovementModes)appSettings.m_movementMode;
m_moveScale = appSettings.m_kbMoveScale;
m_rotateScale = appSettings.m_kbRotateScale;
m_bFpInputLock = appSettings.m_bFpInputLock > 0;

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

// Idle, LockedRotate, CancelSelect and ApplyDrag states don't need configs due to transition rules.
// The rest are listed here
LoadStateConfig (m_stateConfigs [eMouseStateButtonDown], "ButtonDown");
LoadStateConfig (m_stateConfigs [eMouseStateSelect], "AdvSelect");
LoadStateConfig (m_stateConfigs [eMouseStatePan], "Pan");
LoadStateConfig (m_stateConfigs [eMouseStateRotate], "Rotate");
LoadStateConfig (m_stateConfigs [eMouseStateZoom], "Zoom");
LoadStateConfig (m_stateConfigs [eMouseStateZoomIn], "ZoomIn");
LoadStateConfig (m_stateConfigs [eMouseStateZoomOut], "ZoomOut");
LoadStateConfig (m_stateConfigs [eMouseStateQuickSelect], "QuickSelect");
LoadStateConfig (m_stateConfigs [eMouseStateQuickSelectObject], "QuickSelectObject");
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
// LockedRotate is copied from Rotate
memcpy_s (&m_stateConfigs [eMouseStateLockedRotate], sizeof (m_stateConfigs [eMouseStateLockedRotate]),
	&m_stateConfigs [eMouseStateRotate], sizeof (m_stateConfigs [eMouseStateRotate]));
}

void CInputHandler::UpdateMovement (double timeElapsed)
{
	// MineView/Renderer already apply view move rate
	double moveAmount = timeElapsed * m_moveScale;
	double rotateAmount = timeElapsed * m_rotateScale;
	static double zoomAmount = 0;
	double pspFlip = (m_pMineView->Perspective () ? -1 : 1);

if (m_bKeyCommandActive [eKeyCommandMoveForward])
	m_pMineView->Pan ('Z', moveAmount * pspFlip);
if (m_bKeyCommandActive [eKeyCommandMoveBackward])
	m_pMineView->Pan ('Z', -moveAmount * pspFlip);
if (m_bKeyCommandActive [eKeyCommandMoveLeft])
	m_pMineView->Pan ('X', -moveAmount);
if (m_bKeyCommandActive [eKeyCommandMoveRight])
	m_pMineView->Pan ('X', moveAmount);
if (m_bKeyCommandActive [eKeyCommandMoveUp])
	m_pMineView->Pan ('Y', -moveAmount * pspFlip);
if (m_bKeyCommandActive [eKeyCommandMoveDown])
	m_pMineView->Pan ('Y', moveAmount * pspFlip);
if (m_bKeyCommandActive [eKeyCommandRotateUp])
	m_pMineView->Rotate ('X', rotateAmount * pspFlip);
if (m_bKeyCommandActive [eKeyCommandRotateDown])
	m_pMineView->Rotate ('X', -rotateAmount * pspFlip);
if (m_bKeyCommandActive [eKeyCommandRotateLeft])
	m_pMineView->Rotate ('Y', rotateAmount * pspFlip);
if (m_bKeyCommandActive [eKeyCommandRotateRight])
	m_pMineView->Rotate ('Y', -rotateAmount * pspFlip);
if (m_bKeyCommandActive [eKeyCommandRotateBankLeft])
	m_pMineView->Rotate ('Z', rotateAmount);
if (m_bKeyCommandActive [eKeyCommandRotateBankRight])
	m_pMineView->Rotate ('Z', -rotateAmount);

if (m_bKeyCommandActive [eKeyCommandZoomIn])
	zoomAmount += moveAmount;
if (m_bKeyCommandActive [eKeyCommandZoomOut])
	zoomAmount -= moveAmount;
if (fabs (zoomAmount) >= 1) {
	m_pMineView->ZoomIn ((int)zoomAmount);
	zoomAmount -= (int)zoomAmount;
	}
}

bool CInputHandler::OnKeyUp (UINT nChar, UINT nRepCnt, UINT nFlags)
{
	bool bInputHandled = false;

UpdateModifierStates (WM_KEYUP, nChar, nFlags);
UpdateMouseState (WM_KEYUP);
bInputHandled = UpdateInputLockState (WM_KEYUP, nChar);

eKeyCommands command = MapKeyToCommand (nChar);
if (IsMovementCommand (command)) {
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
			break;
		}
	bInputHandled = true;
	}

return bInputHandled;
}

bool CInputHandler::OnKeyDown (UINT nChar, UINT nRepCnt, UINT nFlags)
{
	bool bInputHandled = false;

UpdateModifierStates (WM_KEYDOWN, nChar, nFlags);
UpdateMouseState (WM_KEYDOWN);
bInputHandled = UpdateInputLockState (WM_KEYDOWN, nChar);

eKeyCommands command = MapKeyToCommand (nChar);
if (IsMovementCommand (command)) {
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
			break;
		}
	bInputHandled = true;
	}

return bInputHandled;
}

void CInputHandler::OnMouseMove (UINT nFlags, CPoint point)
{
UpdateModifierStates (WM_MOUSEMOVE, 0, nFlags);
UpdateMouseState (WM_MOUSEMOVE, point);

CPoint change = m_lastMousePos - point;
if (change.x || change.y) {
	switch (MouseState ()) {
		case eMouseStateIdle:
			break;

		case eMouseStateDrag:
			m_pMineView->UpdateDragPos ();
			break;

		case eMouseStateRubberBandTag:
		case eMouseStateRubberBandUnTag:
			m_pMineView->UpdateRubberRect (*m_stateStartPos, point);
			break;

		case eMouseStateSelect:
			m_pMineView->UpdateSelectHighlights ();
			break;

		case eMouseStatePan:
			DoMousePan (point);
			break;

		case eMouseStateRotate:
			DoMouseRotate (point);
			break;

		case eMouseStateZoomIn:
		case eMouseStateZoomOut:
			DoMouseZoom (point);
			break;

		case eMouseStateLockedRotate:
			DoMouseRotate (point);
			m_lastMousePos = m_pMineView->CenterMouse ();
			break;
		}
	}

if (MouseState () != eMouseStateLockedRotate)
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
if (HasInputLock ())
	return eMouseStateLockedRotate;
//if (!m_pMouseState->HasMaybeExited (msg))
//	return m_pMouseState->GetValue ();

// Check states in three passes - first look for an exact match, then look for
// any partial match that this message was the last remaining condition for.
// The existing state is otherwise preferred, except when it has exited.
for (int i = eMouseStateIdle + 1; i < eMouseStateCount; i++) {
	eMouseStates state = (eMouseStates)i;
	if (state == m_pMouseState->GetValue ())
		continue;
	if (GetMouseState (state)->HasEntered (msg) == eMatchExact)
		if (m_pMouseState->ValidateTransition (state))
			return state;
	}

for (int i = eMouseStateIdle + 1; i < eMouseStateCount; i++) {
	eMouseStates state = (eMouseStates)i;
	if (state == m_pMouseState->GetValue ())
		continue;
	if (GetMouseState (state)->HasEntered (msg) == eMatchPartialCompleted)
		if (m_pMouseState->ValidateTransition (state))
			return state;
	}

if (!m_pMouseState->HasExited (msg))
	return m_pMouseState->GetValue ();
for (int i = eMouseStateIdle + 1; i < eMouseStateCount; i++) {
	eMouseStates state = (eMouseStates)i;
	if (state == m_pMouseState->GetValue ())
		continue;
	if (GetMouseState (state)->HasEntered (msg) == eMatchPartial)
		if (m_pMouseState->ValidateTransition (state))
			return state;
	}

// If all else fails, go back to idle.
return eMouseStateIdle;
}

bool CInputHandler::HasMouseMoved (const CPoint point) const
{
if (m_stateStartPos == nullptr)
	return false;
return point != *m_stateStartPos;
}

bool CInputHandler::CheckValidDragTarget (const CPoint point) const
{
// Only counts as a drag if it hits a vertex
int v = vertexManager.Index (current->Vertex ());
if ((abs (point.x - vertexManager [v].m_screen.x) < 5) &&
	(abs (point.y - vertexManager [v].m_screen.y) < 5))
	return true;
return false;
}

void CInputHandler::UpdateMouseState (UINT msg, CPoint point)
{
eMouseStates newState = MapInputToMouseState (msg, point);

if (MouseState () != newState) {
	m_pMouseState = GetMouseState (newState);
	m_pMouseState->OnExited (msg, point);
	m_pMineView->SetCursor (MouseState ());

	// Record the mouse position where this state started
	if (m_stateStartPos == nullptr)
		m_stateStartPos = new CPoint (point);
	else
		*m_stateStartPos = point;

	// Some special handling for zoom - we need a separate tracking position
	// for it, because it uses stepping
	if (MouseState () == eMouseStateZoomIn ||
		MouseState () == eMouseStateZoomOut) {
		if (m_zoomStartPos == nullptr)
			m_zoomStartPos = new CPoint (point);
		else
			*m_zoomStartPos = point;
		}
	else if (m_zoomStartPos != nullptr) {
		delete m_zoomStartPos;
		m_zoomStartPos = nullptr;
		}

	if (MouseState () == eMouseStateDrag) {
		m_pMineView->InitDrag ();
		}
	}
}

void CInputHandler::UpdateMouseState (UINT msg)
{
UpdateMouseState (msg, m_lastMousePos);
}

void CInputHandler::UpdateModifierStates (UINT msg, UINT nChar, UINT nFlags)
{
if (msg == WM_KEYDOWN || msg == WM_KEYUP) {
	switch (nChar) {
		case VK_SHIFT:
			m_bModifierActive [eModifierShift] = (msg == WM_KEYDOWN);
			break;

		case VK_CONTROL:
			m_bModifierActive [eModifierCtrl] = (msg == WM_KEYDOWN);
			break;

		case VK_MENU:
			m_bModifierActive [eModifierAlt] = (msg == WM_KEYDOWN);
			break;

		default:
			break;
		}
	}
else if (msg == WM_MOUSEMOVE) {
	m_bModifierActive [eModifierShift] = (nFlags & MK_SHIFT) == MK_SHIFT;
	m_bModifierActive [eModifierCtrl] = (nFlags & MK_CONTROL) == MK_CONTROL;
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

bool CInputHandler::UpdateInputLockState (UINT msg, UINT nChar)
{
// Input lock is a toggle, which means we have to handle it somewhat differently
if (KeyMatchesKeyCommand (eKeyCommandInputLock, nChar)) {
	if (msg == WM_KEYDOWN)
		m_bKeyCommandActive [eKeyCommandInputLock] = true;
	else if (msg == WM_KEYUP) {
		if (m_bKeyCommandActive [eKeyCommandInputLock]) {
			m_bKeyCommandActive [eKeyCommandInputLock] = false;
			m_bInputLockActive = !m_bInputLockActive;
			if (!m_bInputLockActive)
				StopAllMovement ();
			if (m_bFpInputLock)
				m_pMineView->OverridePerspective (1, m_bInputLockActive);
			return true;
			}
		}
	}

return false;
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

// Check modifiers (on key down only; we don't want to insist on key up order)
if (!m_bKeyCommandActive [command])
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
if (GetMouseState (eMouseStatePan)->GetConfig ().bInvertX)
	panAmountX = -panAmountX;
if (GetMouseState (eMouseStatePan)->GetConfig ().bInvertY)
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

if (GetMouseState (eMouseStateZoom)->GetConfig ().bInvertX)
	zoomOffset.x = -zoomOffset.x;
if (GetMouseState (eMouseStateZoom)->GetConfig ().bInvertY)
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
	// RotateAmountX/Y are how much we rotate about either axis,
	// which is why it looks like we're swapping them here.
	double rotateAmountX = double(change.y) / scale;
	double rotateAmountY = -double(change.x) / scale;

if (GetMouseState (eMouseStateRotate)->GetConfig ().bInvertY)
	rotateAmountX = -rotateAmountX;
if (GetMouseState (eMouseStateRotate)->GetConfig ().bInvertX)
	rotateAmountY = -rotateAmountY;

m_pMineView->Rotate ('Y', rotateAmountY);
m_pMineView->Rotate ('X', rotateAmountX);
}

void CInputHandler::ApplyMovement (eKeyCommands command)
{
	double pspFlip = (m_pMineView->Perspective () ? -1 : 1);

if (!IsMovementCommand (command))
	return;

switch (command) {
	case eKeyCommandMoveForward:
		m_pMineView->Pan ('Z', 1 * pspFlip);
		break;
	case eKeyCommandMoveBackward:
		m_pMineView->Pan ('Z', -1 * pspFlip);
		break;
	case eKeyCommandMoveLeft:
		m_pMineView->Pan ('X', -1);
		break;
	case eKeyCommandMoveRight:
		m_pMineView->Pan ('X', 1);
		break;
	case eKeyCommandMoveUp:
		m_pMineView->Pan ('Y', -1 * pspFlip);
		break;
	case eKeyCommandMoveDown:
		m_pMineView->Pan ('Y', 1 * pspFlip);
		break;
	case eKeyCommandRotateUp:
		m_pMineView->Rotate ('X', 1 * pspFlip);
		break;
	case eKeyCommandRotateDown:
		m_pMineView->Rotate ('X', -1 * pspFlip);
		break;
	case eKeyCommandRotateLeft:
		m_pMineView->Rotate ('Y', 1 * pspFlip);
		break;
	case eKeyCommandRotateRight:
		m_pMineView->Rotate ('Y', -1 * pspFlip);
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

if (m_bKeyCommandActive [command])
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

if (!m_bKeyCommandActive [command])
	return;

m_bKeyCommandActive [command] = false;
m_nMovementCommandsActive--;
if (m_nMovementCommandsActive == 0)
	m_pMineView->StopMovementTimer ();
}

void CInputHandler::StopAllMovement ()
{
if (m_movementMode != eMovementModeContinuous)
	return;

for (int command = 0; command < eKeyCommandCount; command++)
	if (IsMovementCommand ((eKeyCommands)command))
		StopMovement ((eKeyCommands)command);
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
config.bToggleModifiers = nToggleModifiers > 0; // TODO Currently ignored. Need to figure out if it's needed; delete if not
config.bInvertX = nInvertX > 0;
config.bInvertY = nInvertY > 0;
}

void CInputHandler::LoadModifiers (bool (&modifierList) [eModifierCount], LPTSTR szMods)
{
	LPCTSTR delimiters = _T (",");
	LPTSTR context = NULL;
	LPTSTR pszNext = NULL;

for (unsigned int i = 0; i < eModifierCount; i++)
	modifierList [i] = false;

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
	pszNext = _tcstok_s (NULL, delimiters, &context);
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

template < eMouseStates T >
class CMouseInputStateBase : public IMouseInputState {
	public:
		CMouseInputStateBase (CInputHandler *pInputHandler, const MouseStateConfig &config) :
			m_pInputHandler (pInputHandler),
			m_stateConfig (config)
		{}

		eMouseStates GetValue () const { return T; }

		const MouseStateConfig& GetConfig () const { return m_stateConfig; }

		// Called after the state has changed and should apply any resulting actions
		// msg indicates the event that caused the completion
		virtual void OnCompleted (UINT msg) = 0;
		// Called after the state has changed and should not apply any actions
		// msg indicates the event that caused the cancellation
		virtual void OnCancelled (UINT msg) = 0;

		virtual void OnExited (UINT msg) {
			if (IsMessageMatchingButtonUp (msg))
				OnCompleted (msg);
			else
				OnCancelled (msg);
			}

		virtual eMouseStateMatchResults HasEntered (UINT msg) const {
			eMouseStateMatchResults result = eMatchExact;

			// Check modifiers
			for (int i = 0; i < eModifierCount; i++) {
				bool bRequired = m_stateConfig.modifiers [i];

				if (m_pInputHandler->m_bModifierActive [i]) {
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
						if (!(m_stateConfig.button & MK_LBUTTON))
							result = eMatchNone;
						break;

					case WM_MBUTTONDOWN:
						if (!(m_stateConfig.button & MK_MBUTTON))
							result = eMatchNone;
						break;

					case WM_RBUTTONDOWN:
						if (!(m_stateConfig.button & MK_RBUTTON))
							result = eMatchNone;
						break;

					case (0x10000 | WM_XBUTTONDOWN):
						if (!(m_stateConfig.button & MK_XBUTTON1))
							result = eMatchNone;
						break;

					case (0x20000 | WM_XBUTTONDOWN):
						if (!(m_stateConfig.button & MK_XBUTTON2))
							result = eMatchNone;
						break;

					default:
						if (IsClickState ())
							result = eMatchNone;
						break;
					}
				}

			return result;
			}

		virtual bool HasExited (UINT msg) {
			// Check modifiers
			for (int i = 0; i < eModifierCount; i++) {
				bool bRequired = m_stateConfig.modifiers [i];

				if (bRequired && !m_pInputHandler->m_bModifierActive [i])
					return true;
				}

			// Check mouse button
			return m_pInputHandler->ButtonUpMatchesState (T, msg);
			}

	protected:
		CInputHandler *m_pInputHandler;
		MouseStateConfig m_stateConfig;

		bool IsClickState () const { return m_stateConfig.button != 0; }

		bool IsMessageMatchingButtonUp (UINT msg) const {
			switch (msg) {
				// Using bitwise again. Note that this will allow any button up to leave a state
				// even if others are still held. Currently this is what we want but it may change later
				case WM_LBUTTONUP:
					if (m_stateConfig.button & MK_LBUTTON)
						return true;
					break;

				case WM_MBUTTONUP:
					if (m_stateConfig.button & MK_MBUTTON)
						return true;
					break;

				case WM_RBUTTONUP:
					if (m_stateConfig.button & MK_RBUTTON)
						return true;
					break;

				case (0x10000 | WM_XBUTTONUP):
					if (m_stateConfig.button & MK_XBUTTON1)
						return true;
					break;

				case (0x20000 | WM_XBUTTONUP):
					if (m_stateConfig.button & MK_XBUTTON2)
						return true;
					break;

				default:
					break;
				}

			return false;
			}
};

class CMouseStateIdle : public CMouseInputStateBase < eMouseStateIdle > {
	public:
		CMouseStateIdle (CInputHandler *pInputHandler) :
			CMouseInputStateBase (pInputHandler, { 0 })
		{}

		void OnEntered (UINT) {}
		void OnCompleted (UINT) {}
		void OnCancelled (UINT) {}
		eMouseStateMatchResults HasEntered (UINT) const { return eMatchPartial; }
		bool HasExited (UINT) const { return false; }
		bool ValidateTransition (eMouseStates) const { return true; }
};

class CMouseStateQuickSelect : public CMouseInputStateBase < eMouseStateQuickSelect > {
	public:
		void OnCompleted (UINT msg) {
			m_pInputHandler->m_pMineView->SelectCurrentElement (point.x, point.y, false);
			}
};

class CMouseStateQuickSelectObject : public CMouseInputStateBase < eMouseStateQuickSelectObject > {
	public:
		void OnCompleted (UINT msg) {
			m_pInputHandler->m_pMineView->SelectCurrentObject (point.x, point.y);
			}
};

class CMouseStateDrag : public CMouseInputStateBase < eMouseStateDrag > {
	public:
		void OnCompleted (UINT msg) {
			m_pInputHandler->m_pMineView->FinishDrag (point);
			}

		eMouseStateMatchResults HasEntered (UINT msg, const CPoint point) {
			eMouseStateMatchResults result = CMouseInputStateBase::HasEntered (msg);
			if (result &&
				(!m_pInputHandler->HasMouseMoved (point) ||
				!m_pInputHandler->CheckValidDragTarget (*m_pInputHandler->m_stateStartPos)))
				result = eMatchNone;
			return result;
			}
};

class CMouseStateRubberBandTag : public CMouseInputStateBase < eMouseStateRubberBandTag > {
	public:
		void OnCompleted (UINT msg) {
			m_pMineView->ResetRubberRect ();
			m_pMineView->TagRubberBandedVertices (*m_stateStartPos, point, true);
			}

		eMouseStateMatchResults HasEntered (UINT msg, const CPoint point) {
			eMouseStateMatchResults result = CMouseInputStateBase::HasEntered (msg);
			if (result && !m_pInputHandler->HasMouseMoved (point))
				result = eMatchNone;
			return result;
			}
};

class CMouseStateRubberBandUnTag : public CMouseInputStateBase < eMouseStateRubberBandUnTag > {
	public:
		void OnCompleted (UINT msg) {
			m_pMineView->ResetRubberRect ();
			m_pMineView->TagRubberBandedVertices (*m_stateStartPos, point, false);
			}

		eMouseStateMatchResults HasEntered (UINT msg, const CPoint point) {
			eMouseStateMatchResults result = CMouseInputStateBase::HasEntered (msg);
			if (result && !m_pInputHandler->HasMouseMoved (point))
				result = eMatchNone;
			return result;
			}
};

class CMouseStateQuickTag : public CMouseInputStateBase < eMouseStateQuickTag > {
	public:
		void OnCompleted (UINT msg) {
			m_pMineView->SelectCurrentElement (point.x, point.y, true);
			}
};

class CMouseStateDoContextMenu : public CMouseInputStateBase < eMouseStateDoContextMenu > {
	public:
		void OnCompleted (UINT msg) {
			m_pMineView->DoContextMenu (point);
			// In case we came from Select
			m_pMineView->UpdateSelectHighlights ();
			}
};

class CMouseStateSelect : public CMouseInputStateBase < eMouseStateSelect > {
	public:
		void OnCompleted (UINT msg) {
			m_pMineView->SelectCurrentElement (point.x, point.y, false);
			}

		void OnCancelled (UINT msg) {
			m_pMineView->UpdateSelectHighlights ();
			}

		bool ValidateTransition (eMouseStates newState) const {
			switch (newState) {
				case eMouseStatePan:
				case eMouseStateRotate:
				case eMouseStateZoom:
					// If the mouse has not moved allow transitions to zoom/pan/rotate
					// (unless we clicked to enter this state)
					if (IsClickState () || m_pInputHandler->HasMouseMoved (point))
						return false;
					return true;

				default:
					return true;
				}
			}
};

class CMouseStateLockedRotate : public CMouseInputStateBase < eMouseStateLockedRotate > {
	public:
		bool ValidateTransition (eMouseStates) const { return !m_pInputHandler->HasInputLock (); }
};

class CMouseStatePan : public CMouseInputStateBase < eMouseStatePan > {

};

class CMouseStateRotate : public CMouseInputStateBase < eMouseStateRotate > {

};

class CMouseStateZoom : public CMouseInputStateBase < eMouseStateZoom > {
	public:
		void OnCompleted (UINT msg) {
			if (!HasMouseMoved (point)) {
				if (IsMessageMatchingButtonUp (eMouseStateZoomIn, msg))
					m_pMineView->ZoomIn (1, true);
				if (IsMessageMatchingButtonUp (eMouseStateZoomOut, msg))
					m_pMineView->ZoomOut (1, true);
				}
			}

		bool ValidateTransition (eMouseStates newState) const {
			// If the current state requires a mouse click and that button is down, no change
			if (IsClickState (m_mouseState) && !HasExitedState (msg))
				break;
			}
};
