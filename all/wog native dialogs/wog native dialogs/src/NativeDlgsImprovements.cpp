
#define NDI_HERO_PORTRAIT_ID 400
DWORD __stdcall BlackMarketDlg__Ctor(HiHook *h, _Dlg_ *dlg, const int x, const int y)
{
    DWORD retAddr = CALL_3(DWORD, __thiscall, h->GetDefaultFunc(), dlg, x, y);
    if (dlg)
    {
        auto heroPortraitFrame = _DlgStaticPcx8_::Create(126, 55, NDI_HERO_PORTRAIT_ID - 1, "TpTavSel.pcx");
        dlg->AddItem(heroPortraitFrame);

        auto heroPortrait =
            _DlgStaticPcx8_::Create(128, 57, NDI_HERO_PORTRAIT_ID, (char *)o_HeroInfo[o_Market_Hero->id].hpl_name);
        dlg->AddItem(heroPortrait);
    }
    return retAddr;
}
int __stdcall BlackMarketDlg__Proc(HiHook *h, const _Dlg_ *dlg, const _DlgMsg_ *msg)
{
    const int result = CALL_2(int, __thiscall, h->GetDefaultFunc(), dlg, msg);
    if (result != 2 && msg->type == MT_MOUSEBUTTON && msg->item_id == NDI_HERO_PORTRAIT_ID &&
        msg->subtype != MST_LBUTTONCLICK)
    {
        o_Market_Hero->ShowHeroScreen(true, true, msg->subtype == MST_RBUTTONDOWN);
    }
    return result;
}
_LHF_(BlackMarketDlg__SetHint)
{
    if (c->eax == NDI_HERO_PORTRAIT_ID)
    {
        sprintf(o_TextBuffer, o_GENRLTXT_TXT->GetString(17), o_Market_Hero->name, o_Market_Hero->Get_className());
        c->Push(reinterpret_cast<int>(o_TextBuffer));
        c->return_address = 0x05EE308;
        return SKIP_DEFAULT;
    }
    return EXEC_DEFAULT;
}

// отображение анимации существ в диалогах с существами на ПКМ
static inline DWORD GetTime()
{
    return CALL_0(DWORD, __stdcall, PtrAt(0x63A354));
}
struct MonDefInfo
{
    int id = -1;
    DWORD defPtr = 0;
} monDefInfo;

_LHF_(RMCdlgProc)
{
    auto dlg = *reinterpret_cast<_Dlg_ **>(c->ebp + 0x8);
    if (dlg->v_table[0] == 0x05F3EC0)
    {
        monDefInfo.defPtr = DwordAt(dlg->Offset(0xB4));
        monDefInfo.id = IntAt(dlg->Offset(0x60));
    }
    auto dlgDef = monDefInfo.defPtr; // animation def
    if (dlgDef)
    {
        DWORD waitUntil = DwordAt(0x6989E8);
        DWORD currentTime = GetTime();

        if (int(currentTime - waitUntil) < 0)
        {
            return EXEC_DEFAULT;
        }
        const int id = monDefInfo.id;
        const bool isWarMachine = CALL_1(bool, __thiscall, 0x047AAB0, id);

        // рисуем следующий кадр анимации
        CALL_1(void, __thiscall, isWarMachine ? 0x04EB330 : 0x04EB140, dlgDef);
        dlg->Redraw();

        waitUntil = DwordAt(0x6989E8);
        int currentTimeA = GetTime() - waitUntil;
        if (currentTimeA < 100)
            currentTimeA = 100;

        DwordAt(0x6989E8) = waitUntil + currentTimeA;
    }
    return EXEC_DEFAULT;
}
Patch *rightClickDlgProc = nullptr;

_LHF_(H3DwellingDlg_ShowRMC)
{
    monDefInfo.defPtr = c->eax;
    monDefInfo.id = IntAt(c->ebp - 0x1C);
    return EXEC_DEFAULT;
}
void __stdcall WndMgr__ShowRightClickDlg(HiHook *hook, DWORD wnd, _Dlg_ *dlg)
{
    if (dlg && (dlg->v_table[0] == 0x05F3EC0 || dlg->v_table[0] == 0x0551AB0))
        rightClickDlgProc->Apply();

    CALL_2(void, __thiscall, hook->GetDefaultFunc(), wnd, dlg);

    if (rightClickDlgProc->IsApplied())
    {
        rightClickDlgProc->Undo();
        monDefInfo = {};
    }
}

// отключение зацикливания при зажатии хоткеев

DWORD __stdcall DefButtonOnHotKey(HiHook *hook, _DlgButton_ *button, DWORD msg)
{
    if (button->state & 1) // isPressed
    {
        return 2;
    }
    return CALL_2(DWORD, __thiscall, hook->GetDefaultFunc(), button, msg);
}

void NativeDlgsImprovements(PatcherInstance *_PI)
{
    // отображение фунционального портрета активного героя в диалоге черного рынка и торговца артефактами
    _PI->WriteHiHook(0x05EA785, CALL_, EXTENDED_, THISCALL_, BlackMarketDlg__Ctor);
    _PI->WriteHiHook(0x05EDDE0, SPLICE_, EXTENDED_, THISCALL_, BlackMarketDlg__Proc);
    _PI->WriteLoHook(0x05EE2B4, BlackMarketDlg__SetHint);

    // отображение анимации существ в диалогах с существами на ПКМ
    _PI->WriteHiHook(0x0603000, SPLICE_, EXTENDED_, THISCALL_, WndMgr__ShowRightClickDlg);
    _PI->WriteLoHook(0x0551EBB, H3DwellingDlg_ShowRMC);

    rightClickDlgProc = _PI->CreateLoHook(0x060306D, RMCdlgProc);
    rightClickDlgProc->Undo();

    // отключение зацикливания при зажатии хоткеев
    _PI->WriteHiHook(0x04562C7, CALL_, EXTENDED_, THISCALL_, DefButtonOnHotKey);
}
