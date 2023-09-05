/**
 * Application W32 : indicateurs de l'état du système
 * (horloge, charge CPU et mémoire) restant en permanence
 * à l'avant du bureau.
 */


#include <windows.h>
#include <stdio.h>
#include <math.h>
#include "resource.h"



/* == CONSTANTES === */

/* taille maximale des chaînes de caractères */
#define TAILLE_MAX_MSG_ERR  (1024U)
#define TAILLE_MAX_CHN_AFF  (32U)

/* identifiant du "timer" de MAJ de la fenêtre de l'horloge */
static const UINT_PTR ID_EVNT_TMR_MAJ = 0x004A414D ;   /* chaîne 'MAJ' en hexa */

/* nom de l'application */
static const WCHAR szAppName[] = L"Indicateurs système NT" ;

/* taille de la fenêtre */
static const int LARGEUR_FENETRE = 256 ;
static const int HAUTEUR_FENETRE = 128 ;

/* caractéristiques de la fenêtre */
static const DWORD STYLE_FEN = WS_CAPTION | WS_POPUPWINDOW | WS_VISIBLE ;

/* style pour l'écriture de texte dans notre fenêtre */
static const UINT STYLE_TXT = DT_CENTER | DT_VCENTER | DT_SINGLELINE
                            | DT_NOPREFIX | DT_END_ELLIPSIS ;

/* taille de caractères à utiliser pour l'affichage */
static const int TAILLE_CARAC = -16 ;

/* hauteur en pixels des caractères affichés dans la fenêtre */
static const LONG HAUTEUR_CARAC = 21L ;


/* === VARIABLES GLOBALES === */

static SYSTEMTIME dtHrDerniereMAJ = { .wSecond = 0xFFFF } ;
static WCHAR chnDate[TAILLE_MAX_CHN_AFF] = L"%d" ;
static WCHAR chnHeure[TAILLE_MAX_CHN_AFF] = L"%T" ;
static WCHAR chnCharge[TAILLE_MAX_CHN_AFF] = L"CPU : %d %% — RAM : %u %%" ;
static BOOL batteriePresente = TRUE ;
static WCHAR chnBatt[TAILLE_MAX_CHN_AFF] = L"Batt. : %u %%%s" ;


/* === FONCTIONS === */

static void
ShowErrorMessage (const WCHAR* titre,
                  const WCHAR* introMsg)
{
	/* retrouve le message correspondant à l'erreur système actuelle */
	WCHAR msgErr[TAILLE_MAX_MSG_ERR] ;
	char* errStr = NULL ;
	DWORD num_err = GetLastError () ;
	DWORD nchar = FormatMessageW (FORMAT_MESSAGE_ALLOCATE_BUFFER
	                              | FORMAT_MESSAGE_FROM_SYSTEM,
	                              NULL,
	                              num_err,
	                              0,
	                              (LPWSTR) &errStr,
	                              TAILLE_MAX_MSG_ERR,
	                              NULL) ;
	if (nchar > 0) {
		swprintf (msgErr, TAILLE_MAX_MSG_ERR, L"%s\n%s", introMsg, errStr) ;
		LocalFree (errStr) ;
	} else {
		swprintf (msgErr, TAILLE_MAX_MSG_ERR, L"%s", introMsg) ;
	}
	MessageBoxW (NULL, msgErr, titre, MB_ICONERROR) ;
}


/* procédure appelée par le "timer" de MAJ de la fenêtre */
VOID CALLBACK TimerMAJProc (HWND hwnd,
                            UINT uMsg,
                            UINT_PTR idEvent,
                            DWORD dwTime)
{
	/* paramètres inutiles (en particulier
	   le compteur de "ticks" système) */
	(void) uMsg ; (void) dwTime ;

	/* vérifie que l'on s'adresse au "timer" voulu */
	if (idEvent != ID_EVNT_TMR_MAJ) return ;

	/* retrouve la date et l'heure courantes */
	SYSTEMTIME temps ;
	GetLocalTime (&temps) ;

	/* vérifie que la mise à jour du contenu de la fenêtre est utile */
	if (temps.wSecond == dtHrDerniereMAJ.wSecond) return ;
	dtHrDerniereMAJ = temps ;

	/* mise à jour des chaînes de caractères de date et heure */
	int res = GetDateFormatW (LOCALE_USER_DEFAULT,
	                          DATE_SHORTDATE,
	                          &temps,
	                          NULL,
	                          chnDate,
	                          TAILLE_MAX_CHN_AFF) ;
	if (res <= 0)
	{
		ShowErrorMessage (szAppName,
		                  L"Echec de GetDateFormat() !") ;
		ExitProcess ((UINT) -2) ;
	}
	chnDate[TAILLE_MAX_CHN_AFF - 1] = L'\0' ;
	res = GetTimeFormatW (LOCALE_USER_DEFAULT,
	                      0U,
	                      &temps,
	                      NULL,
	                      chnHeure,
	                      TAILLE_MAX_CHN_AFF) ;
	if (res <= 0)
	{
		ShowErrorMessage (szAppName,
		                  L"Echec de GetTimeFormat() !") ;
		ExitProcess ((UINT) -2) ;
	}
	chnHeure[TAILLE_MAX_CHN_AFF - 1] = L'\0' ;

	/* mise à jour de la chaîne des charges processeur et RAM */
	MEMORYSTATUSEX memStat ;
	memStat.dwLength = sizeof(MEMORYSTATUSEX) ;
	BOOL ok = GlobalMemoryStatusEx (&memStat) ;
	if (!ok)
	{
		ShowErrorMessage (szAppName,
		                  L"Echec de GlobalMemoryStatusEx() !") ;
		ExitProcess ((UINT) -2) ;
	}
	DWORDLONG memOcc = memStat.ullTotalPhys - memStat.ullAvailPhys ;
	unsigned chargeMem = (unsigned) round (100.0 * (double) memOcc
	                                      / (double) memStat.ullTotalPhys) ;

	static ULONGLONG ancienTotal, ancienOccup ;

	FILETIME idleFT, kernFT, userFT;
	ok = GetSystemTimes (&idleFT, &kernFT, &userFT) ;
	if (!ok)
	{
		ShowErrorMessage (szAppName,
		                  L"Echec de GetSystemTimes() !") ;
		ExitProcess ((UINT) -2) ;
	}
	ULARGE_INTEGER idleTicks ;
	idleTicks.LowPart = idleFT.dwLowDateTime ;
	idleTicks.HighPart = idleFT.dwHighDateTime ;
	ULARGE_INTEGER kernTicks ;
	kernTicks.LowPart = kernFT.dwLowDateTime ;
	kernTicks.HighPart = kernFT.dwHighDateTime ;
	ULARGE_INTEGER userTicks ;
	userTicks.LowPart = userFT.dwLowDateTime ;
	userTicks.HighPart = userFT.dwHighDateTime ;
	ULONGLONG total = kernTicks.QuadPart + userTicks.QuadPart ;
	ULONGLONG occup = total - idleTicks.QuadPart ;
	ULONGLONG dTotal = total - ancienTotal ;
	ULONGLONG dOccup = occup - ancienOccup ;
	ancienTotal = total ;
	ancienOccup = occup ;
	unsigned chargeProc = (unsigned) round (100.0 * (double) dOccup
	                                       / (double) dTotal) ;
	swprintf (chnCharge, TAILLE_MAX_CHN_AFF,
	          L"CPU : %u %% — RAM : %u %%",
	          chargeProc, chargeMem) ;

	/* niveau de la batterie et présence / absence de l'adaptateur secteur */
	if (batteriePresente)
	{
		SYSTEM_POWER_STATUS powerStatus ;
		ok = GetSystemPowerStatus (&powerStatus) ;
		if (!ok)
		{
			ShowErrorMessage (szAppName,
			                  L"Echec de GetSystemPowerStatus() !") ;
			ExitProcess ((UINT) -3) ;	
		}
		if (powerStatus.BatteryFlag & BATTERY_FLAG_NO_BATTERY)
		{
			batteriePresente = FALSE ;
		}
		BOOL enCharge = (powerStatus.BatteryFlag & BATTERY_FLAG_CHARGING) ;
		BOOL surSecteur = (powerStatus.ACLineStatus == AC_LINE_ONLINE) ;
		swprintf (chnBatt, TAILLE_MAX_CHN_AFF,
		          L"Batt. : %u %%%s",
		          powerStatus.BatteryLifePercent,
		          ( enCharge ? L" (en charge)"
		                     : ( surSecteur ? L" (sur secteur)" : L"" ) )) ;
	}

	/* force la fenêtre à redessiner son contenu,
	   et la ramène au premier plan */
	ok = InvalidateRect (hwnd, NULL, FALSE) ;
	if (!ok)
	{
		ShowErrorMessage (szAppName,
		                  L"Echec de InvalidateRect() !") ;
		ExitProcess ((UINT) -2) ;
	}
	ok = UpdateWindow (hwnd) ;
	if (!ok)
	{
		ShowErrorMessage (szAppName,
		                  L"Echec de UpdateWindow() !") ;
		ExitProcess ((UINT) -2) ;
	}
	SetWindowPos (hwnd,
	              HWND_TOPMOST,
	              -1, -1, -1, -1,
	              SWP_NOCOPYBITS | SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW) ;
}

/* procédure de la fenêtre principale (et unique) de l'application */
LRESULT CALLBACK MainWndProc (HWND hwnd,
                              UINT message,
                              WPARAM wParam,
                              LPARAM lParam)
{
	static HFONT hFont ;
	static HPEN hPen ;
	static HBRUSH hBrush ;
	HDC hdc ;
	PAINTSTRUCT ps ;
	RECT rectFen, rectDate, rectHeure, rectCharge, rectBatt ;
	BOOL ok ;
	LONG ecart ;

	switch (message)
	{
	case WM_CREATE:
		hFont = CreateFontW (TAILLE_CARAC,
		                     0,
		                     0,
		                     0,
		                     700,
		                     FALSE,
		                     FALSE,
		                     FALSE,
		                     DEFAULT_CHARSET,
		                     OUT_TT_PRECIS,
		                     CLIP_DEFAULT_PRECIS,
		                     PROOF_QUALITY,
		                     FF_SWISS | VARIABLE_PITCH,
		                     NULL ) ;
		hPen = GetStockObject (BLACK_PEN) ;
		hBrush = CreateSolidBrush (COLOR_WINDOW) ;

		ok = GetWindowRect (hwnd, &rectFen) ;
		if (ok)
		{
			RECT rectEcran ;
			ok = SystemParametersInfoW (SPI_GETWORKAREA, 0, &rectEcran, 0) ;
			int x = rectEcran.right - (rectFen.right - rectFen.left) ;
			int y = rectEcran.bottom - (rectFen.bottom - rectFen.top) ;
			SetWindowPos (hwnd,
			              HWND_TOPMOST,
			              x, y,
			              -1, -1,
			              SWP_NOSIZE | SWP_NOCOPYBITS) ;
		}

		TimerMAJProc (hwnd, message, ID_EVNT_TMR_MAJ, 0U) ;
		return 0 ;

	case WM_PAINT:
		/* prépare le "canevas" */
		hdc = BeginPaint (hwnd, &ps) ;
		SelectObject (hdc, hFont) ;
		SelectObject (hdc, hPen) ;
		SelectObject (hdc, hBrush) ;
		GetClientRect (hwnd, &rectFen) ;

		/* position des textes à écrire */
		ecart = ( batteriePresente ? 3L : 8L ) ;
		rectDate.left = rectHeure.left = rectCharge.left = rectBatt.left
		   = rectFen.left + ecart ;
		rectDate.right = rectHeure.right = rectCharge.right = rectBatt.right
		   = rectFen.right - ecart ;
		rectDate.top = rectFen.top + ecart ;
		rectDate.bottom = rectDate.top + HAUTEUR_CARAC ;
		rectHeure.top = rectDate.bottom + ecart ;
		rectHeure.bottom = rectHeure.top + HAUTEUR_CARAC ;
		rectCharge.top = rectHeure.bottom + ecart ;
		rectCharge.bottom = rectCharge.top + HAUTEUR_CARAC ;
		rectBatt.bottom = rectFen.bottom - ecart ;
		rectBatt.top = rectBatt.bottom - HAUTEUR_CARAC ;

		/* affichage des textes */
		FillRect (hdc, &rectDate, (HBRUSH) (COLOR_WINDOW + 1) ) ;
		DrawTextW (hdc,
		           chnDate,
		           -1,
		           &rectDate,
		           STYLE_TXT) ;
		FillRect (hdc, &rectHeure, (HBRUSH) (COLOR_WINDOW + 1) ) ;
		DrawTextW (hdc,
		           chnHeure,
		           -1,
		           &rectHeure,
		           STYLE_TXT) ;
		FillRect (hdc, &rectCharge, (HBRUSH) (COLOR_WINDOW + 1) ) ;
		DrawTextW (hdc,
		           chnCharge,
		           -1,
		           &rectCharge,
		           STYLE_TXT) ;
		if (batteriePresente)
		{
			FillRect (hdc, &rectBatt, (HBRUSH) (COLOR_WINDOW + 1) ) ;
			DrawTextW (hdc,
			           chnBatt,
			           -1,
			           &rectBatt,
			           STYLE_TXT) ;
		}

		SelectObject (hdc, GetStockObject (SYSTEM_FONT)) ;
		SelectObject (hdc, GetStockObject (WHITE_BRUSH)) ;

		EndPaint (hwnd, &ps) ;
		return 0 ;

	case WM_DESTROY:
		DeleteObject (hFont) ;
		DeleteObject (hBrush) ;

		PostQuitMessage (0) ;
		return 0 ;
	}
	return DefWindowProcW (hwnd, message, wParam, lParam) ;
}


/** point d'entrée de l'application **/
int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    PSTR szCmdLine, int iCmdShow)
{
	/* paramètres inutiles */
	(void) hPrevInstance ; (void) szCmdLine ; (void) iCmdShow ;

	/* définition de la classe de notre fenêtre */
	WNDCLASSEXW wndClass;
	wndClass.cbSize        = sizeof(WNDCLASSEX) ;
	wndClass.style         = CS_HREDRAW | CS_VREDRAW ;
	wndClass.lpfnWndProc   = MainWndProc ;
	wndClass.cbClsExtra    = 0 ;
	wndClass.cbWndExtra    = 0 ;
	wndClass.hInstance     = hInstance ;
	wndClass.hIcon         = LoadIcon (hInstance,
	                                   MAKEINTRESOURCE (ID_ICONE_INDIC_SYS)) ;
	wndClass.hCursor       = LoadCursor (NULL, IDC_ARROW) ;
	wndClass.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1) ;
	wndClass.lpszMenuName  = NULL ;
	wndClass.lpszClassName = szAppName ;
	wndClass.hIconSm       = LoadIcon (hInstance,
	                                   MAKEINTRESOURCE (ID_ICONE_INDIC_SYS)) ;
	if (!RegisterClassExW (&wndClass))
	{
		ShowErrorMessage (szAppName,
		                  L"Echec de RegisterClass() !") ;
		return -1 ;
	}

	/* créé la fenêtre principale (et unique) de notre application */
	HWND hwnd = CreateWindowExW (WS_EX_TOPMOST,
	                             szAppName,
	                             szAppName,
	                             STYLE_FEN,
	                             CW_USEDEFAULT,
	                             CW_USEDEFAULT,
	                             LARGEUR_FENETRE,
	                             HAUTEUR_FENETRE,
	                             NULL,
	                             NULL,
	                             hInstance,
	                             NULL) ;
	if (hwnd == NULL)
	{
		ShowErrorMessage (szAppName,
		                  L"Echec de CreateWindow() !") ;
		return -1 ;
	}

	/* affiche la fenêtre nouvellement créée */
	ShowWindow (hwnd, SW_SHOWNORMAL) ;
	BOOL ok = UpdateWindow (hwnd) ;
	if (!ok)
	{
		ShowErrorMessage (szAppName,
		                  L"Echec de UpdateWindow() !") ;
		return -1 ;
	}

	/* création du "timer" de MAJ de la fenêtre, lequel
	   se déclenche 50 fois par seconde
	   (c'est-à-dire toutes les 20 millisecondes) */
	UINT_PTR idTmr = SetTimer (hwnd,
	                           ID_EVNT_TMR_MAJ,
	                           20U,
	                           TimerMAJProc);
	if (idTmr == 0U)
	{
		ShowErrorMessage (szAppName,
		                  L"Echec de SetTimer() !") ;
		return -1 ;
	}

	/* boucle de réception des messages destinés à notre application */
	MSG msg;
	int res = GetMessageW (&msg, NULL, 0, 0) ;
	while (res > 0)
	{
		/* traite le message courant */
		TranslateMessage (&msg) ;
		DispatchMessageW (&msg) ;

		/* attend le message suivant */
		res = GetMessageW (&msg, NULL, 0, 0) ;
	}

	/* arrête et supprime le "timer" de MAJ */
	KillTimer (hwnd, ID_EVNT_TMR_MAJ) ;

	/* teste si une erreur est survenue */
	if (res == -1)
	{
		ShowErrorMessage (szAppName,
		                  L"Echec de GetMessage() !") ;
	}

	/* programme terminé */
	return (int) msg.wParam ;
}


