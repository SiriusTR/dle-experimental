// dlcView.cpp : implementation of the CMineView class
//

#include "stdafx.h"
#include "afxpriv.h"
#include "dle-xp.h"

#include "dlcDoc.h"
#include "mineview.h"
#include "toolview.h"

#include "PaletteManager.h"
#include "textures.h"
#include "global.h"
#include "FileManager.h"

#include <math.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CToolView

BEGIN_MESSAGE_MAP(CDiagTool, CToolDlg)
	ON_BN_CLICKED (IDC_DIAG_CHECK, OnCheckMine)
	ON_BN_CLICKED (IDC_DIAG_SHOW, OnShowBug)
	ON_BN_CLICKED (IDC_DIAG_CLEAR, OnClearList)
	ON_BN_CLICKED (IDC_DIAG_FIXBUGS, OnFixBugs)
	ON_BN_CLICKED (IDC_DIAG_WARNINGS, OnShowWarnings)
	ON_LBN_SELCHANGE (IDC_DIAG_BUGLIST, OnShowBug)
END_MESSAGE_MAP()

//------------------------------------------------------------------------------

CDiagTool::CDiagTool (CPropertySheet *pParent)
	: CToolDlg (IDD_DIAGDATA_HORZ + nLayout, pParent)
{
m_bAutoFixBugs = TRUE; 
m_bShowWarnings = true;
m_bCheckMsgs = false;
m_statsWidth = 0;
}

//------------------------------------------------------------------------------

CDiagTool::~CDiagTool ()
{
ClearBugList ();
}

//------------------------------------------------------------------------------

BOOL CDiagTool::OnInitDialog ()
{
if (!CToolDlg::OnInitDialog ())
   return FALSE;
CListCtrl& plc = LVStats ()->GetListCtrl ();
CRect rc;
plc.GetClientRect (rc);
int cx = rc.Width () - 16;
if (plc.InsertColumn (0, "item", LVCFMT_LEFT, (cx * 43) / 100) == -1)
   return FALSE;
if (plc.InsertColumn (1, "count", LVCFMT_LEFT, (cx * 25) / 100) == -1)
   return FALSE;
if (plc.InsertColumn (2, "max #", LVCFMT_LEFT, cx - (cx * 43) / 100 - (cx * 25) / 100) == -1)
   return FALSE;
m_bInited = true;
Refresh ();
return TRUE;
}

//------------------------------------------------------------------------------

LPSTR CDiagTool::ItemText (int nValue, LPSTR pszPrefix)
{
	static	char szText [20];

if (pszPrefix)
	sprintf_s (szText, sizeof (szText), "%s: %d", pszPrefix, nValue);
else
	sprintf_s (szText, sizeof (szText), "%d", nValue);
return szText;
}

//------------------------------------------------------------------------------

void CDiagTool::CountObjects (void)
{
CGameObject *pObject = objectManager.Object (0);
memset (m_nObjects, 0, sizeof (m_nObjects));
memset (m_nContained, 0, sizeof (m_nContained));
int i, j;
for (i = objectManager.Count (), j = 0; i; i--, j++, pObject++)
	switch(pObject->Type ()) {
		case OBJ_ROBOT:
			m_nObjects [0]++;
			m_nContained [0] += pObject->m_info.contents.count;
			break;
		case OBJ_HOSTAGE:
			m_nObjects [1]++;
			break;
		case OBJ_PLAYER:
			m_nObjects [2]++;
			break;
		case OBJ_COOP:
			m_nObjects [3]++;
			break;
		case OBJ_WEAPON:
			m_nObjects [4]++;
			break;
		case OBJ_POWERUP:
			switch (powerupTypeTable [pObject->Id ()]) {
				case POWERUP_WEAPON_MASK:
					m_nObjects [4]++;
					break;
				case POWERUP_POWERUP_MASK:
					m_nObjects [5]++;
					m_nContained [1] += pObject->m_info.contents.count;
					break;
				case POWERUP_KEY_MASK:
					m_nObjects [6]++;
					break;
				default:
					break;
				}
			break;
		case OBJ_REACTOR:
			m_nObjects [7]++;
			break;
		case OBJ_EFFECT:
			m_nObjects [8]++;
			break;
		default:
			m_nObjects [9]++;
			break;
		}
}

//------------------------------------------------------------------------------

int CDiagTool::CountTextures (void)
{
	CSegment *pSegment = segmentManager.Segment (0);
	CSide *pSide;
	char bUsed [(MAX_TEXTURES_D2 + 7) / 8];
	int t, i, j, h = wallManager.WallCount ();
	int nUsed = 0;

memset (bUsed, 0, sizeof (bUsed));
for (i = segmentManager.Count (); i; i--, pSegment++)
	for (j = 0, pSide = pSegment->m_sides; j < MAX_SIDES_PER_SEGMENT; j++, pSide++)
		if ((pSegment->ChildId (j) == -1) || (pSide->m_info.nWall < h)) {
			t = pSide->BaseTex ();
//			CBRK ((t >> 3) >= (MAX_TEXTURES_D2 + 7) / 8);
			if ((t >= 0) && (t < MAX_TEXTURES_D2) && (!(bUsed [t >> 3] & (1 << (t & 7))))) {
				nUsed++;
				bUsed [t >> 3] |= (1 << (t & 7));
				}
			t = pSide->OvlTex (0);
//			CBRK ((t >> 3) >= (MAX_TEXTURES_D2 + 7) / 8);
			if ((t > 0) && (t < MAX_TEXTURES_D2) && (!(bUsed [t >> 3] & (1 << (t & 7))))) {
				nUsed++;
				bUsed [t >> 3] |= (1 << (t & 7));
				}
			}
return nUsed;
}

//------------------------------------------------------------------------------

void CDiagTool::Reset (void)
{
CHECKMINE;
if (!Inited ())
	return;
ClearBugList ();
segmentManager.UnTagAll ();
}
//------------------------------------------------------------------------------

void CDiagTool::Refresh (void)
{
if (!(m_bInited && theMine))
	return;
	CListCtrl& plc = LVStats ()->GetListCtrl ();
	LPSTR	*psz;
	int i;

	static LPSTR szItems [] = {
		"segments", "vertices", "mat cens", "fuel centers", "walls", "triggers", "objects:", 
		"  robots", "  hostages", "  players", "  coop players", "  powerups", "  weapons", "  keys", "  reactors", "  effects", "  other", "textures",
		null};

plc.DeleteAllItems ();
for (psz = szItems, i = 0; *psz; psz++, i++)
	plc.InsertItem (i, *psz);
CountObjects ();
plc.SetItemText (0, 1, ItemText (segmentManager.Count ()));
plc.SetItemText (1, 1, ItemText (vertexManager.Count ()));
plc.SetItemText (2, 1, ItemText (segmentManager.RobotMakerCount ()));
plc.SetItemText (3, 1, ItemText (segmentManager.ProducerCount ()));
plc.SetItemText (4, 1, ItemText (wallManager.WallCount ()));
plc.SetItemText (5, 1, ItemText (triggerManager.WallTriggerCount ()));
plc.SetItemText (6, 1, ItemText (objectManager.Count ()));
for (i = 0; i < 10; i++)
	plc.SetItemText (7 + i, 1, ItemText (m_nObjects [i]));
plc.SetItemText (17, 1, ItemText (CountTextures ()));
plc.SetItemText (0, 2, ItemText (MAX_SEGMENTS));
plc.SetItemText (1, 2, ItemText (MAX_VERTICES));
plc.SetItemText (2, 2, ItemText (MAX_MATCENS));
plc.SetItemText (3, 2, ItemText (MAX_NUM_RECHARGERS));
plc.SetItemText (4, 2, ItemText (MAX_WALLS));
plc.SetItemText (5, 2, ItemText (MAX_TRIGGERS));
plc.SetItemText (6, 2, ItemText (MAX_OBJECTS));
plc.SetItemText (7, 2, ItemText (m_nContained [0], "cont"));
plc.SetItemText (9, 2, ItemText (MAX_PLAYERS));
plc.SetItemText (10, 2, ItemText (3));
plc.SetItemText (11, 2, ItemText (m_nContained [1], "cont"));
plc.SetItemText (13, 2, ItemText (3));
plc.SetItemText (14, 2, ItemText (DLE.IsStdLevel () ? 1 : 100));
plc.SetItemText (17, 2, ItemText (MAX_TEXTURES));
}

//------------------------------------------------------------------------------

void CDiagTool::DoDataExchange (CDataExchange * pDX)
{
if (!HaveData (pDX)) 
	return;
DDX_Check (pDX, IDC_DIAG_FIXBUGS, m_bAutoFixBugs);
DDX_Check (pDX, IDC_DIAG_WARNINGS, m_bShowWarnings);
}

								/*--------------------------*/

BOOL CDiagTool::OnSetActive ()
{
Refresh ();
return CToolDlg::OnSetActive ();
}

								/*--------------------------*/

void CDiagTool::OnClearList ()
{
ClearBugList ();
}

								/*--------------------------*/

void CDiagTool::OnFixBugs ()
{
UpdateData (TRUE);
}

								/*--------------------------*/

void CDiagTool::OnShowWarnings ()
{
UpdateData (TRUE);
}

//------------------------------------------------------------------------------

int CDiagTool::AddMessage (const char *pszMsg, int nMaxMsgs, bool bCheckMsg)
{
if (!(m_bInited && theMine))
	return -1;
if (bCheckMsg != m_bCheckMsgs) {
	ClearBugList ();
	m_bCheckMsgs = bCheckMsg;
	}
CListBox *plb = LBBugs ();
int nCount = plb->GetCount ();
if (nCount) {
	char	szMsg [256];
	if ((plb->GetText (nCount - 1, szMsg) != LB_ERR) && !_stricmp (szMsg, pszMsg))
		return -1;
	}
if (nMaxMsgs >= 0)
	for (; nCount >= nMaxMsgs; nCount--)
		plb->DeleteString (0);
return plb->AddString (pszMsg);
}

//------------------------------------------------------------------------------

//eof diagdlg.cpp