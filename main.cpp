#include <windows.h>
#include <commctrl.h>
#include <dwmapi.h>
#include "movement_calculator.h"
#include "resource.h"
#include <array>
#include <cwchar>
#include <iterator>
#include <string>

namespace {
constexpr int N = movement::kTerrainCount, CALC = 10, CLEAR = 11;
constexpr int ARMY_COUNT = 5, ARMY_TAB_BASE = 60, REGION_TAB_ID = 65, RATE_TOGGLE_ID = 70;
constexpr int REGION_CALC_ID = 71, REGION_CLEAR_ID = 72;
constexpr COLORREF BG = RGB(10,35,27);
constexpr COLORREF CARD = RGB(20,58,43);
constexpr COLORREF CARD_EDGE = RGB(72,112,78);
constexpr COLORREF CREAM = RGB(238,231,205);
constexpr COLORREF MUTED = RGB(185,198,176);
constexpr COLORREF ACCENT = RGB(117,174,125);
constexpr COLORREF YELLOW = RGB(232,196,74);
constexpr COLORREF INPUT_BG = RGB(246,242,225);
constexpr COLORREF RESULT_BG = RGB(27,68,49);
constexpr COLORREF INPUT_FG = RGB(29,48,36);
constexpr COLORREF FG = RGB(239,235,215);
HWND edits[N]{}, labels[N]{}, resultBox, calcButton, clearButton;
HWND spinners[N]{};
HWND armyTabs[ARMY_COUNT]{};
HWND regionTab = nullptr;
HWND rateToggle = nullptr;
HWND regionTitle = nullptr, regionDescription = nullptr;
HWND totalTilesLabel = nullptr, heldTilesLabel = nullptr;
HWND totalTilesEdit = nullptr, heldTilesEdit = nullptr;
HWND percentageResult = nullptr, regionCalculateButton = nullptr, regionClearButton = nullptr;
bool regionTabSelected = false;
// Each army keeps its own counts, result, and rate setting while the app is open.
std::array<bool,ARMY_COUNT> armyParamountRates{};
std::array<std::array<std::wstring,N>,ARMY_COUNT> armyValues{};
std::array<std::wstring,ARMY_COUNT> armyResults{};
int currentArmy = 0;
constexpr wchar_t INITIAL_RESULT[] = L"Enter tile counts, then choose Calculate.\r\n\r\nAvailable stamina per turn: 12.00";
HWND inputTitle, resultTitle;
HFONT font, headingFont;
HBRUSH bgBrush, cardBrush, inputBrush, resultBrush;
const wchar_t* names[N] = {L"Regular terrain",L"Allied road",L"Non-allied road",L"Forest",L"Sea",L"Embark / disembark"};

HWND control(HWND parent, const wchar_t* cls, const wchar_t* text, DWORD style, int id=0, DWORD ex=0) {
    return CreateWindowExW(ex, cls, text, style, 0,0,0,0,parent,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),GetModuleHandleW(nullptr),nullptr);
}
void setFont(HWND h, HFONT f) { SendMessageW(h,WM_SETFONT,reinterpret_cast<WPARAM>(f),TRUE); }
// Region calculator has its own layout
void layout(HWND w) {
    RECT r{}; GetClientRect(w,&r);
    const int width=r.right,height=r.bottom,margin=28,top=132,split=386;
    const int tabGap=6, tabWidth=(width-2*margin-tabGap*ARMY_COUNT)/(ARMY_COUNT+1);
    for(int i=0;i<ARMY_COUNT;i++) MoveWindow(armyTabs[i],margin+i*(tabWidth+tabGap),14,tabWidth,36,TRUE);
    MoveWindow(regionTab,margin+ARMY_COUNT*(tabWidth+tabGap),14,tabWidth,36,TRUE);
    MoveWindow(regionTitle,margin+22,96,width-2*margin-44,34,TRUE);
    MoveWindow(regionDescription,margin+22,138,width-2*margin-44,44,TRUE);
    MoveWindow(totalTilesLabel,margin+54,224,190,30,TRUE); MoveWindow(totalTilesEdit,margin+54,260,190,36,TRUE);
    MoveWindow(heldTilesLabel,margin+54,322,190,30,TRUE); MoveWindow(heldTilesEdit,margin+54,358,190,36,TRUE);
    MoveWindow(percentageResult,margin+300,224,width-2*margin-404,116,TRUE);
    MoveWindow(regionCalculateButton,margin+300,354,160,44,TRUE); MoveWindow(regionClearButton,margin+474,354,120,44,TRUE);
    // Only one calculator screen is visible at a time.
    const int showArmy=regionTabSelected ? SW_HIDE : SW_SHOW;
    for(HWND h: {inputTitle,resultTitle,rateToggle,calcButton,clearButton,resultBox}) ShowWindow(h,showArmy);
    for(HWND h: labels) if(h) ShowWindow(h,showArmy); for(HWND h: edits) if(h) ShowWindow(h,showArmy); for(HWND h: spinners) if(h) ShowWindow(h,showArmy);
    for(HWND h: {regionTitle,regionDescription,totalTilesLabel,heldTilesLabel,totalTilesEdit,heldTilesEdit,percentageResult,regionCalculateButton,regionClearButton}) ShowWindow(h,regionTabSelected ? SW_SHOW : SW_HIDE);
    if(regionTabSelected) return;
    MoveWindow(inputTitle,margin,72,330,24,TRUE); MoveWindow(rateToggle,margin,100,330,26,TRUE); MoveWindow(resultTitle,split,72,width-split-margin,24,TRUE);
    for(int i=0;i<N;i++) { int y=top+i*42; MoveWindow(labels[i],margin,y+3,205,28,TRUE); MoveWindow(edits[i],margin+205,y,112,32,TRUE); MoveWindow(spinners[i],margin+317,y,22,32,TRUE); }
    const int by=top+N*42+14; MoveWindow(calcButton,margin,by,145,38,TRUE); MoveWindow(clearButton,margin+157,by,112,38,TRUE);
    const int availableResultWidth=width-split-margin; MoveWindow(resultBox,split,top,availableResultWidth<720?availableResultWidth:720,height-top-margin,TRUE);
}
void drawButton(const DRAWITEMSTRUCT* item) {
    const bool primary = item->CtlID == CALC || item->CtlID == REGION_CALC_ID;
    const bool pressed = (item->itemState & ODS_SELECTED) != 0;
    const bool focused = (item->itemState & ODS_FOCUS) != 0;
    COLORREF fill = primary ? YELLOW : RGB(36,83,58);
    const COLORREF ink = primary ? INPUT_FG : CREAM;
    if (pressed) fill = primary ? RGB(198,153,38) : RGB(48,99,67);
    HBRUSH brush = CreateSolidBrush(fill);
    FillRect(item->hDC, &item->rcItem, brush);
    DeleteObject(brush);
    HPEN pen = CreatePen(PS_SOLID, 1, primary ? YELLOW : CARD_EDGE);
    HPEN oldPen = static_cast<HPEN>(SelectObject(item->hDC, pen));
    HBRUSH oldBrush = static_cast<HBRUSH>(SelectObject(item->hDC, GetStockObject(HOLLOW_BRUSH)));
    RoundRect(item->hDC, item->rcItem.left, item->rcItem.top,
        item->rcItem.right, item->rcItem.bottom, 10, 10);
    SelectObject(item->hDC, oldBrush);
    SelectObject(item->hDC, oldPen);
    DeleteObject(pen);
    wchar_t caption[64]{};
    GetWindowTextW(item->hwndItem, caption, 64);
    SetBkMode(item->hDC, TRANSPARENT);
    SetTextColor(item->hDC, ink);
    RECT textRect = item->rcItem;
    DrawTextW(item->hDC, caption, -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    if (focused) {
        InflateRect(&textRect, -7, -6);
        DrawFocusRect(item->hDC, &textRect);
    }
}
LRESULT CALLBACK spinnerVisualProc(HWND hwnd, UINT message, WPARAM wp, LPARAM lp,
    UINT_PTR, DWORD_PTR) {
    if (message == WM_ERASEBKGND) return 1;
    if (message == WM_PAINT) {
        PAINTSTRUCT paint{};
        HDC dc = BeginPaint(hwnd, &paint);
        RECT bounds{};
        GetClientRect(hwnd, &bounds);
        const int middle = (bounds.top + bounds.bottom) / 2;

        POINT cursor{};
        GetCursorPos(&cursor);
        ScreenToClient(hwnd, &cursor);
        const bool overTop = cursor.y >= bounds.top && cursor.y < middle
            && cursor.x >= bounds.left && cursor.x < bounds.right;
        const bool overBottom = cursor.y >= middle && cursor.y < bounds.bottom
            && cursor.x >= bounds.left && cursor.x < bounds.right;
        const bool pressed = (GetKeyState(VK_LBUTTON) & 0x8000) != 0;

        HBRUSH spinnerBrush = CreateSolidBrush(YELLOW);
        FillRect(dc, &bounds, spinnerBrush);
        DeleteObject(spinnerBrush);
        if (overTop || overBottom) {
            RECT hoverArea = overTop
                ? RECT{bounds.left + 1, bounds.top + 1, bounds.right - 1, middle}
                : RECT{bounds.left + 1, middle, bounds.right - 1, bounds.bottom - 1};
            HBRUSH hoverBrush = CreateSolidBrush(pressed ? RGB(198,153,38) : RGB(246,216,112));
            FillRect(dc, &hoverArea, hoverBrush);
            DeleteObject(hoverBrush);
        }

        HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(165,130,36));
        HPEN oldPen = static_cast<HPEN>(SelectObject(dc, borderPen));
        HBRUSH oldBrush = static_cast<HBRUSH>(SelectObject(dc, GetStockObject(HOLLOW_BRUSH)));
        Rectangle(dc, bounds.left, bounds.top, bounds.right, bounds.bottom);
        MoveToEx(dc, bounds.left + 1, middle, nullptr);
        LineTo(dc, bounds.right - 1, middle);

        const int centerX = (bounds.left + bounds.right) / 2;
        const int upperY = (bounds.top + middle) / 2;
        const int lowerY = (middle + bounds.bottom) / 2;
        POINT upArrow[] = {{centerX, upperY - 3}, {centerX - 4, upperY + 3}, {centerX + 4, upperY + 3}};
        POINT downArrow[] = {{centerX - 4, lowerY - 3}, {centerX + 4, lowerY - 3}, {centerX, lowerY + 3}};
        HBRUSH arrowBrush = CreateSolidBrush(INPUT_FG);
        SelectObject(dc, arrowBrush);
        SelectObject(dc, GetStockObject(NULL_PEN));
        Polygon(dc, upArrow, 3);
        Polygon(dc, downArrow, 3);

        SelectObject(dc, oldBrush);
        SelectObject(dc, oldPen);
        DeleteObject(arrowBrush);
        DeleteObject(borderPen);
        EndPaint(hwnd, &paint);
        return 0;
    }
    if (message == WM_MOUSEMOVE) {
        TRACKMOUSEEVENT tracking{sizeof(tracking), TME_LEAVE, hwnd, 0};
        TrackMouseEvent(&tracking);
        InvalidateRect(hwnd, nullptr, FALSE);
    } else if (message == WM_MOUSELEAVE || message == WM_LBUTTONDOWN || message == WM_LBUTTONUP) {
        InvalidateRect(hwnd, nullptr, FALSE);
    }
    return DefSubclassProc(hwnd, message, wp, lp);
}
void drawArmyTab(const DRAWITEMSTRUCT* item) {
    const bool selected = !regionTabSelected && item->CtlID == ARMY_TAB_BASE + currentArmy;
    const COLORREF fill = selected ? YELLOW : RGB(36,83,58);
    const COLORREF ink = selected ? INPUT_FG : CREAM;
    HBRUSH brush=CreateSolidBrush(fill);
    FillRect(item->hDC,&item->rcItem,brush);
    DeleteObject(brush);
    HPEN pen=CreatePen(PS_SOLID,1,selected ? YELLOW : CARD_EDGE);
    HPEN oldPen=static_cast<HPEN>(SelectObject(item->hDC,pen));
    HBRUSH oldBrush=static_cast<HBRUSH>(SelectObject(item->hDC,GetStockObject(HOLLOW_BRUSH)));
    RoundRect(item->hDC,item->rcItem.left,item->rcItem.top,item->rcItem.right,item->rcItem.bottom,10,10);
    SelectObject(item->hDC,oldBrush); SelectObject(item->hDC,oldPen); DeleteObject(pen);
    wchar_t label[32]{};
    GetWindowTextW(item->hwndItem,label,32);
    SetBkMode(item->hDC,TRANSPARENT); SetTextColor(item->hDC,ink);
    RECT textRect=item->rcItem;
    DrawTextW(item->hDC,label,-1,&textRect,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
}
void drawRegionTab(const DRAWITEMSTRUCT* item) {
    const bool selected=regionTabSelected; HBRUSH brush=CreateSolidBrush(selected?YELLOW:RGB(36,83,58));
    FillRect(item->hDC,&item->rcItem,brush); DeleteObject(brush); SetBkMode(item->hDC,TRANSPARENT);
    SetTextColor(item->hDC,selected?INPUT_FG:CREAM); RECT textRect=item->rcItem;
    DrawTextW(item->hDC,L"Region %",-1,&textRect,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
}
void drawRateToggle(const DRAWITEMSTRUCT* item) {
    FillRect(item->hDC,&item->rcItem,cardBrush);
    const bool checked=armyParamountRates[currentArmy];
    RECT box{item->rcItem.left+2,item->rcItem.top+4,item->rcItem.left+22,item->rcItem.bottom-4};
    HBRUSH boxBrush=CreateSolidBrush(checked ? YELLOW : RESULT_BG);
    FillRect(item->hDC,&box,boxBrush);
    DeleteObject(boxBrush);
    HPEN borderPen=CreatePen(PS_SOLID,1,checked ? YELLOW : ACCENT);
    HPEN oldPen=static_cast<HPEN>(SelectObject(item->hDC,borderPen));
    HBRUSH oldBrush=static_cast<HBRUSH>(SelectObject(item->hDC,GetStockObject(HOLLOW_BRUSH)));
    Rectangle(item->hDC,box.left,box.top,box.right,box.bottom);
    SelectObject(item->hDC,oldBrush);
    if(checked) {
        HPEN checkPen=CreatePen(PS_SOLID,2,INPUT_FG);
        SelectObject(item->hDC,checkPen);
        MoveToEx(item->hDC,box.left+4,box.top+9,nullptr);
        LineTo(item->hDC,box.left+8,box.bottom-5);
        LineTo(item->hDC,box.right-4,box.top+4);
        SelectObject(item->hDC,borderPen);
        DeleteObject(checkPen);
    }
    SelectObject(item->hDC,oldPen);
    DeleteObject(borderPen);
    SetBkMode(item->hDC,TRANSPARENT);
    SetTextColor(item->hDC,CREAM);
    RECT labelRect=item->rcItem;
    labelRect.left=box.right+9;
    DrawTextW(item->hDC,L"Paramount / Crown House rates",-1,&labelRect,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
    if(item->itemState&ODS_FOCUS) {
        RECT focusRect=labelRect;
        InflateRect(&focusRect,-2,-2);
        DrawFocusRect(item->hDC,&focusRect);
    }
}
bool countValue(HWND edit,int& value) {
    wchar_t s[16]{}; GetWindowTextW(edit,s,16);
    if(!s[0]) { value=0; return true; }
    wchar_t* end=nullptr; unsigned long n=wcstoul(s,&end,10);
    if(end==s || *end || n>static_cast<unsigned long>(movement::kMaxTilesPerTerrain)) return false;
    value=static_cast<int>(n); return true;
}
void initializeArmyStates() {
    for(auto& army:armyValues) army.fill(L"0");
    armyResults.fill(INITIAL_RESULT);
    armyParamountRates.fill(false);
}
// Save the current army before moving to another tab.
void saveArmy() {
    for(int i=0;i<N;i++) {
        wchar_t value[16]{};
        GetWindowTextW(edits[i],value,16);
        armyValues[currentArmy][i]=value;
    }
    const int length=GetWindowTextLengthW(resultBox);
    std::wstring result(static_cast<size_t>(length)+1,L'\0');
    GetWindowTextW(resultBox,result.data(),length+1);
    result.resize(static_cast<size_t>(length));
    armyResults[currentArmy]=result;
}
void loadArmy(int army) {
    for(int i=0;i<N;i++) {
        SetWindowTextW(edits[i],armyValues[army][i].c_str());
        int value=0;
        if(countValue(edits[i],value)) SendMessageW(spinners[i],UDM_SETPOS32,0,value);
    }
    SetWindowTextW(resultBox,armyResults[army].c_str());
}
// Region totals can be larger than a single terrain count.
bool regionValue(HWND edit,int& value) {
    wchar_t s[16]{}; GetWindowTextW(edit,s,16); if(!s[0]) { value=0; return true; }
    wchar_t* end=nullptr; unsigned long n=wcstoul(s,&end,10); if(end==s || *end || n>9999999UL) return false;
    value=static_cast<int>(n); return true;
}
// Work out the share of the region held by a house
void calculateRegion(HWND w) {
    int total=0,held=0;
    if(!regionValue(totalTilesEdit,total)||!regionValue(heldTilesEdit,held)||total<=0||held>total) {
        MessageBoxW(w,L"Enter a total greater than 0 and a held tile count from 0 to the total.",L"Check region tile counts",MB_OK|MB_ICONWARNING);
        SetFocus(total<=0?totalTilesEdit:heldTilesEdit); return;
    }
    wchar_t text[80]{}; swprintf_s(text,L"%.2f%% of the region",100.0*static_cast<double>(held)/static_cast<double>(total));
    SetWindowTextW(percentageResult,text);
}
void calculate(HWND w) {
    std::array<int,N> counts{};
    for(int i=0;i<N;i++) if(!countValue(edits[i],counts[i])) {
        MessageBoxW(w,L"Enter a whole number from 0 to 999 for each terrain type.",L"Check tile counts",MB_OK|MB_ICONWARNING);
        SetFocus(edits[i]); return;
    }
    const auto route = movement::calculate(counts,armyParamountRates[currentArmy]);
    if(route.overLimit) {
        SetWindowTextW(resultBox,L"Route exceeds the 10,000 stamina limit. Reduce tile counts and calculate again.");
        MessageBoxW(w,L"Calculated cost exceeds 10,000 stamina.",L"Route limit reached",MB_OK|MB_ICONWARNING);
        return;
    }
    const auto summary = movement::formatSummary(route);
    SetWindowTextW(resultBox,summary.c_str());
}
LRESULT CALLBACK proc(HWND w,UINT m,WPARAM wp,LPARAM lp) {
    switch(m) {
    case WM_CREATE:
        bgBrush=CreateSolidBrush(BG); cardBrush=CreateSolidBrush(CARD); inputBrush=CreateSolidBrush(INPUT_BG); resultBrush=CreateSolidBrush(RESULT_BG);
        font=CreateFontW(-18,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH|FF_DONTCARE,L"Segoe UI");
        headingFont=CreateFontW(-20,0,0,0,FW_SEMIBOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH|FF_DONTCARE,L"Segoe UI");
        armyTabs[0]=control(w,L"BUTTON",L"Army 1",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_OWNERDRAW,ARMY_TAB_BASE+0);
        armyTabs[1]=control(w,L"BUTTON",L"Army 2",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_OWNERDRAW,ARMY_TAB_BASE+1);
        armyTabs[2]=control(w,L"BUTTON",L"Army 3",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_OWNERDRAW,ARMY_TAB_BASE+2);
        armyTabs[3]=control(w,L"BUTTON",L"Army 4",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_OWNERDRAW,ARMY_TAB_BASE+3);
        armyTabs[4]=control(w,L"BUTTON",L"Army 5",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_OWNERDRAW,ARMY_TAB_BASE+4);
        regionTab=control(w,L"BUTTON",L"",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_OWNERDRAW,REGION_TAB_ID);
        regionTitle=control(w,L"STATIC",L"REGION PERCENTAGE",WS_CHILD|SS_CENTER);
        regionDescription=control(w,L"STATIC",L"",WS_CHILD|SS_CENTER);
        totalTilesLabel=control(w,L"STATIC",L"TOTAL TILES IN REGION",WS_CHILD);
        heldTilesLabel=control(w,L"STATIC",L"TILES HELD",WS_CHILD);
        totalTilesEdit=control(w,L"EDIT",L"0",WS_CHILD|WS_TABSTOP|ES_NUMBER|ES_RIGHT|ES_AUTOHSCROLL,0,WS_EX_CLIENTEDGE);
        heldTilesEdit=control(w,L"EDIT",L"0",WS_CHILD|WS_TABSTOP|ES_NUMBER|ES_RIGHT|ES_AUTOHSCROLL,0,WS_EX_CLIENTEDGE);
        SendMessageW(totalTilesEdit,EM_SETLIMITTEXT,7,0); SendMessageW(heldTilesEdit,EM_SETLIMITTEXT,7,0);
        percentageResult=control(w,L"STATIC",L"Enter tile counts to see your share.",WS_CHILD|SS_LEFT|SS_CENTERIMAGE);
        regionCalculateButton=control(w,L"BUTTON",L"Calculate %",WS_CHILD|WS_TABSTOP|BS_OWNERDRAW,REGION_CALC_ID);
        regionClearButton=control(w,L"BUTTON",L"Clear",WS_CHILD|WS_TABSTOP|BS_OWNERDRAW,REGION_CLEAR_ID);
        rateToggle=control(w,L"BUTTON",L"",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_OWNERDRAW,RATE_TOGGLE_ID);
        inputTitle=control(w,L"STATIC",L"TERRAIN TILE COUNTS",WS_CHILD|WS_VISIBLE);
        resultTitle=control(w,L"STATIC",L"ROUTE RESULT",WS_CHILD|WS_VISIBLE);
        for(int i=0;i<N;i++) {
            labels[i]=control(w,L"STATIC",names[i],WS_CHILD|WS_VISIBLE);
            edits[i]=control(w,L"EDIT",L"0",WS_CHILD|WS_VISIBLE|WS_TABSTOP|ES_NUMBER|ES_RIGHT|ES_AUTOHSCROLL,20+i,WS_EX_CLIENTEDGE);
            SendMessageW(edits[i],EM_SETLIMITTEXT,3,0);
            setFont(labels[i],font); setFont(edits[i],font);
            spinners[i]=control(w,UPDOWN_CLASSW,L"",WS_CHILD|WS_VISIBLE|WS_TABSTOP|UDS_ARROWKEYS|UDS_SETBUDDYINT|UDS_NOTHOUSANDS|UDS_HOTTRACK,40+i);
            SendMessageW(spinners[i],UDM_SETBUDDY,reinterpret_cast<WPARAM>(edits[i]),0);
            SendMessageW(spinners[i],UDM_SETRANGE32,0,movement::kMaxTilesPerTerrain);
            // Holding an arrow key increases the value faster over time.
            const UDACCEL acceleration[]={{0,1},{2,2},{4,5},{7,10}};
            SendMessageW(spinners[i],UDM_SETACCEL,static_cast<WPARAM>(std::size(acceleration)),reinterpret_cast<LPARAM>(acceleration));
            SetWindowSubclass(spinners[i],spinnerVisualProc,1,0);
        }
        calcButton=control(w,L"BUTTON",L"Calculate",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_OWNERDRAW,CALC);
        clearButton=control(w,L"BUTTON",L"Clear",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_OWNERDRAW,CLEAR);
        resultBox=control(w,L"EDIT",INITIAL_RESULT,
            WS_CHILD|WS_VISIBLE|WS_TABSTOP|ES_MULTILINE|ES_READONLY|ES_AUTOVSCROLL|WS_VSCROLL,0,WS_EX_CLIENTEDGE);
        setFont(inputTitle,headingFont); setFont(resultTitle,headingFont);
        setFont(regionTitle,headingFont); setFont(regionDescription,font); setFont(totalTilesLabel,font); setFont(heldTilesLabel,font);
        setFont(totalTilesEdit,font); setFont(heldTilesEdit,font); setFont(percentageResult,headingFont);
        setFont(regionCalculateButton,font); setFont(regionClearButton,font);
        for(HWND tab:armyTabs) setFont(tab,font);
        setFont(regionTab,font);
        setFont(rateToggle,font);
        setFont(calcButton,font); setFont(clearButton,font); setFont(resultBox,font);
        loadArmy(0); layout(w); return 0;
    case WM_SIZE: layout(w); return 0;
    case WM_ERASEBKGND: {
        RECT client{}; GetClientRect(w,&client);
        FillRect(reinterpret_cast<HDC>(wp),&client,bgBrush);
        return 1;
    }
    case WM_PAINT: {
        PAINTSTRUCT paint{}; HDC dc=BeginPaint(w,&paint);
        RECT client{}; GetClientRect(w,&client);
        RECT leftCard{18,60,376,client.bottom-18};
        RECT rightCard{376,60,client.right-18,client.bottom-18};
        if(regionTabSelected) {
            RECT regionCard{18,60,client.right-18,client.bottom-18}; FillRect(dc,&regionCard,cardBrush);
            HPEN regionPen=CreatePen(PS_SOLID,1,CARD_EDGE); HPEN oldPen=static_cast<HPEN>(SelectObject(dc,regionPen));
            HBRUSH oldBrush=static_cast<HBRUSH>(SelectObject(dc,GetStockObject(HOLLOW_BRUSH)));
            RoundRect(dc,regionCard.left,regionCard.top,regionCard.right,regionCard.bottom,14,14);
            SelectObject(dc,oldBrush); SelectObject(dc,oldPen); DeleteObject(regionPen); EndPaint(w,&paint); return 0;
        }
        FillRect(dc,&leftCard,cardBrush); FillRect(dc,&rightCard,cardBrush);
        HPEN pen=CreatePen(PS_SOLID,1,CARD_EDGE);
        HPEN oldPen=static_cast<HPEN>(SelectObject(dc,pen));
        HBRUSH oldBrush=static_cast<HBRUSH>(SelectObject(dc,GetStockObject(HOLLOW_BRUSH)));
        RoundRect(dc,leftCard.left,leftCard.top,leftCard.right,leftCard.bottom,14,14);
        RoundRect(dc,rightCard.left,rightCard.top,rightCard.right,rightCard.bottom,14,14);
        SelectObject(dc,oldBrush); SelectObject(dc,oldPen); DeleteObject(pen);
        EndPaint(w,&paint); return 0;
    }
    case WM_COMMAND:
        // Switch between the army screens and the region percentage screen.
        if(HIWORD(wp)==BN_CLICKED && LOWORD(wp)==REGION_TAB_ID) { regionTabSelected=true; layout(w); InvalidateRect(w,nullptr,TRUE); InvalidateRect(regionTab,nullptr,TRUE); return 0; }
        if(HIWORD(wp)==BN_CLICKED && LOWORD(wp)>=ARMY_TAB_BASE && LOWORD(wp)<ARMY_TAB_BASE+ARMY_COUNT) {
            regionTabSelected=false;
            saveArmy();
            currentArmy=LOWORD(wp)-ARMY_TAB_BASE;
            loadArmy(currentArmy);
            layout(w);
            InvalidateRect(w,nullptr,TRUE);
            for(HWND tab:armyTabs) InvalidateRect(tab,nullptr,TRUE);
            InvalidateRect(regionTab,nullptr,TRUE);
            InvalidateRect(rateToggle,nullptr,TRUE);
            return 0;
        }
        if(LOWORD(wp)==RATE_TOGGLE_ID && HIWORD(wp)==BN_CLICKED) {
            armyParamountRates[currentArmy]=!armyParamountRates[currentArmy];
            SetWindowTextW(resultBox,L"Rate setting changed. Calculate or Clear to apply.");
            InvalidateRect(rateToggle,nullptr,TRUE);
            return 0;
        }
        if(LOWORD(wp)==REGION_CALC_ID && HIWORD(wp)==BN_CLICKED) { calculateRegion(w); return 0; }
        if(LOWORD(wp)==REGION_CLEAR_ID && HIWORD(wp)==BN_CLICKED) { SetWindowTextW(totalTilesEdit,L"0"); SetWindowTextW(heldTilesEdit,L"0"); SetWindowTextW(percentageResult,L"Enter tile counts to see your share."); SetFocus(totalTilesEdit); return 0; }
        if(LOWORD(wp)==CALC && HIWORD(wp)==BN_CLICKED) { calculate(w); return 0; }
        if(LOWORD(wp)==CLEAR && HIWORD(wp)==BN_CLICKED) {
            for(HWND e:edits) SetWindowTextW(e,L"0");
            for(HWND spinner:spinners) SendMessageW(spinner,UDM_SETPOS32,0,0);
            SetWindowTextW(resultBox,INITIAL_RESULT);
            SetFocus(edits[0]); return 0;
        }
        break;
    case WM_DRAWITEM: {
        const auto* item=reinterpret_cast<const DRAWITEMSTRUCT*>(lp);
        if(item->CtlID==REGION_TAB_ID) drawRegionTab(item);
        else if(item->CtlID==RATE_TOGGLE_ID) drawRateToggle(item);
        else if(item->CtlID>=ARMY_TAB_BASE && item->CtlID<ARMY_TAB_BASE+ARMY_COUNT) drawArmyTab(item);
        else drawButton(item);
        return TRUE;
    }
    case WM_CTLCOLORSTATIC: {
        HDC dc=reinterpret_cast<HDC>(wp);
        const HWND item=reinterpret_cast<HWND>(lp);
        SetTextColor(dc,item==percentageResult ? YELLOW : ((item==inputTitle || item==resultTitle || item==regionTitle) ? ACCENT : FG));
        SetBkColor(dc,CARD);
        return reinterpret_cast<LRESULT>(cardBrush);
    }
    case WM_CTLCOLOREDIT: {
        HDC dc=reinterpret_cast<HDC>(wp);
        const bool output=reinterpret_cast<HWND>(lp)==resultBox;
        SetTextColor(dc,output ? CREAM : INPUT_FG);
        SetBkColor(dc,output ? RESULT_BG : INPUT_BG);
        return reinterpret_cast<LRESULT>(output ? resultBrush : inputBrush);
    }
    case WM_DESTROY:
        DeleteObject(bgBrush); DeleteObject(cardBrush); DeleteObject(inputBrush); DeleteObject(resultBrush);
        DeleteObject(font); DeleteObject(headingFont); PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(w,m,wp,lp);
}
}

int WINAPI wWinMain(HINSTANCE h,HINSTANCE,PWSTR,int show) {
    initializeArmyStates();
    INITCOMMONCONTROLSEX controls{sizeof(controls),ICC_UPDOWN_CLASS};
    InitCommonControlsEx(&controls);
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    WNDCLASSEXW wc{}; wc.cbSize=sizeof(wc); wc.lpfnWndProc=proc; wc.hInstance=h;
    wc.hCursor=LoadCursorW(nullptr,IDC_ARROW); wc.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);
    wc.hIcon=LoadIconW(h,MAKEINTRESOURCEW(IDI_APP_ICON)); wc.hIconSm=wc.hIcon;
    wc.lpszClassName=L"MovementCalculatorWindow";
    if(!RegisterClassExW(&wc)) return 1;
    HWND w=CreateWindowExW(0,wc.lpszClassName,L"Movement Stamina Calculator",WS_OVERLAPPEDWINDOW & ~(WS_MAXIMIZEBOX|WS_THICKFRAME),
        CW_USEDEFAULT,CW_USEDEFAULT,1100,700,nullptr,nullptr,h,nullptr);
    if(!w) return 1;
    const auto corners=DWMWCP_ROUND;
    DwmSetWindowAttribute(w,DWMWA_WINDOW_CORNER_PREFERENCE,&corners,sizeof(corners));
    ShowWindow(w,show); UpdateWindow(w);
    MSG msg{}; while(GetMessageW(&msg,nullptr,0,0)>0) { TranslateMessage(&msg); DispatchMessageW(&msg); }
    return static_cast<int>(msg.wParam);
}