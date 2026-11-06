#include "NativeDlgsImprovements.h"
#include <map>

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

namespace NativeDlg
{
    bool ButtonFrame::Create(_Dlg_* dlg, int x, int y, int height)
    {
        if (height <= 0)
            return false;
        buffer = _Pcx16_::CreateNew("", WIDTH, height);
        if (!buffer)
            return false;
        memset((void*)buffer->buffer, 0, buffer->buf_size);
        const bool trueColor = *(_byte_*)0x5FA22B == 4;
        for (int row = 0; row < height; ++row)
        {
            for (int col = 0; col < WIDTH; ++col)
            {
                if (row && row != height - 1 && col && col != WIDTH - 1)
                    continue;
                _ptr_ pixel = buffer->buffer + row * buffer->scanline_size;
                if (trueColor)
                    ((_dword_*)pixel)[col] = 0xAD8C42;
                else
                    ((_word_*)pixel)[col] = RGB565_fromR8G8B8(173, 140, 66);
            }
        }
        item = _DlgStaticPcx16_::Create(x - 2, y - 3, WIDTH, height, -1, nullptr, DIF_PCX);
        if (!item)
        {
            Release();
            return false;
        }
        item->pcx16 = buffer;
        dlg->AddItem(item);
        return true;
    }

    void ButtonFrame::Release()
    {
        // Detach the unnamed allocation before native widget destruction.
        if (item)
            item->pcx16 = nullptr;
        if (buffer)
            _Pcx16_::Delete(buffer);
        buffer = nullptr;
        item = nullptr;
    }

    char* AutoSkipHint()
    {
        return GetEraJSON("wnd.level_up_dlg.skip_hint");
    }

    bool IsCtrlConfirm(const _EventMsg_* msg, int okayId)
    {
        const bool key = (msg->type == MT_KEYDOWN || msg->type == MT_KEYUP) &&
            (msg->subtype == HK_ENTER || msg->subtype == HK_ESC || msg->subtype == HK_SPACE);
        // Native OK hotkeys can arrive as mouse down before CLICK loses CTRL.
        const bool okay = msg->type == MT_MOUSEBUTTON && msg->item_id == okayId &&
            (msg->subtype == MST_LBUTTONDOWN || msg->subtype == MST_LBUTTONCLICK);
        return (msg->flags & MF_CTRL) && (key || okay);
    }
}

namespace HeroLevelUp
{
    enum { OK_ID = 30722, AUTO_SKIP_ID = 2020, AI_CHOICE = 9999,
        PORTRAIT_ID = 2001, FIRST_SKILL_ID = 2010, SECOND_SKILL_ID = 2011,
        FRAME_HEIGHT = 36 };

    struct Series
    {
        _Hero_* hero;
        _Dlg_* dialog;
        bool eligible;
        bool active;
        bool extraSoundPlayed;
    };

    struct DialogState
    {
        _Hero_* hero = nullptr;
        DWORD lastClick = 0;
        int lastClickItem = 0;
        NativeDlg::ButtonFrame buttonFrame;
    };

    Series* current = nullptr;
    std::map<_Dlg_*, DialogState> dialogs;
    bool restoreHd5Interactions = false;

    void __stdcall CheckLevelUps(HiHook* hook, _Hero_* hero)
    {
        Series series = {hero, nullptr, false, false, false};
        Series* previous = current;
        current = &series;
        CALL_1(void, __thiscall, hook->GetDefaultFunc(), hero);
        current = previous;
    }

    int __stdcall TargetCalculated(LoHook*, HookContext* c)
    {
        // Both native experience paths reach 0x4DAA3A before the first increment.
        if (current && current->hero == (_Hero_*)c->ebx)
            current->eligible = (int)c->edx - current->hero->level > 3;
        return EXEC_DEFAULT;
    }

    int __stdcall LevelUpSound(LoHook*, HookContext* c)
    {
        if (current && current->hero == (_Hero_*)c->ebx && current->active)
        {
            if (current->extraSoundPlayed)
            {
                // Before sound arguments are pushed; retain the following dialog/network logic.
                c->return_address = 0x4DACA0;
                return NO_EXEC_DEFAULT;
            }
            current->extraSoundPlayed = true;
        }
        return EXEC_DEFAULT;
    }

    bool IsCurrentDialog(_Dlg_* dlg)
    {
        return dlg && current && current->eligible && current->dialog == dlg;
    }

    int CloseWithAI(_EventMsg_* msg, int itemId = 10)
    {
        // Mirror the native timeout. With two offers, 0x4DAEAC consumes 9999 and calls the AI.
        o_WndMgr->result_dlg_item_id = AI_CHOICE;
        IntAt(0x6977D4) = 0;
        msg->type = MT_MOUSEBUTTON;
        msg->subtype = MST_EXIT;
        msg->item_id = itemId;
        return 2;
    }

    int __fastcall ButtonCallback(_EventMsg_* msg)
    {
        if (!msg || msg->item_id != AUTO_SKIP_ID || !IsCurrentDialog((_Dlg_*)msg->flags_2))
            return 0;
        if (msg->type == MT_MOUSEBUTTON && msg->subtype == MST_RBUTTONDOWN)
        {
            b_MsgBox(NativeDlg::AutoSkipHint(), MBX_RMC);
            return 1;
        }
        if (msg->type == MT_MOUSEBUTTON && msg->subtype == MST_LBUTTONCLICK)
        {
            current->active = true;
            // The manager copies the callback's message ID into its result after this returns.
            return CloseWithAI(msg, AI_CHOICE);
        }
        return 0;
    }

    _Dlg_* __stdcall DialogCtor(HiHook* hook, _Dlg_* dlg, _Hero_* hero, int primary, int first, int second)
    {
        _Dlg_* result = CALL_5(_Dlg_*, __thiscall, hook->GetDefaultFunc(), dlg, hero, primary, first, second);
        if (!result)
            return result;
        DialogState& state = dialogs[result];
        state.hero = hero;
        auto* okay = (_DlgButton_*)result->GetItem(OK_ID);
        if (okay)
            okay->AddHotKey(HK_SPACE);
        if (!okay || !current || current->hero != hero || !current->eligible)
            return result;
        if (current->active)
            current->dialog = result;
        // Preserve the position and frame adjusted in GEM by the user.
        const int x = result->width - okay->x - 64;
        const int y = okay->y;
        if (x < 2 || y < 3 || !state.buttonFrame.Create(result, x, y, FRAME_HEIGHT))
            return result;
        auto* button = _DlgCallbackButton_::Create(x, y, 64, 30, AUTO_SKIP_ID,
            "iOKAL.def", (_ptr_)ButtonCallback, 0, 1);
        if (button)
        {
            result->AddItem(button);
            current->dialog = result;
        }
        // Hidden level-ups remain disabled: do not access the flag at 0x698450.
        return result;
    }

    int __stdcall DialogProc(HiHook* hook, _Dlg_* dlg, _EventMsg_* msg)
    {
        if (IsCurrentDialog(dlg))
        {
            // ShowAndRun draws each subsequent window before invoking this procedure.
            if (current->active)
                return CloseWithAI(msg);
            if (NativeDlg::IsCtrlConfirm(msg, OK_ID))
            {
                msg->type = MT_MOUSEBUTTON;
                msg->subtype = MST_LBUTTONCLICK;
                msg->item_id = AUTO_SKIP_ID;
                msg->flags_2 = (_dword_)dlg;
                return ButtonCallback(msg);
            }
        }

        // These interactions used to be in HD5_Functions.cpp. Keep the HD version
        // condition, and keep their timers independent from the commander dialog.
        auto it = dialogs.find(dlg);
        if (restoreHd5Interactions && it != dialogs.end() && msg->type == MT_MOUSEBUTTON)
        {
            DialogState& state = it->second;
            if (state.hero && msg->item_id == PORTRAIT_ID &&
                (msg->subtype == MST_LBUTTONDOWN || msg->subtype == MST_RBUTTONDOWN))
            {
                CALL_4(void, __fastcall, 0x4E1A70, state.hero->id, 1, 1, msg->subtype == MST_RBUTTONDOWN);
            }
            else if (msg->subtype == MST_LBUTTONDOWN &&
                (msg->item_id == FIRST_SKILL_ID || msg->item_id == SECOND_SKILL_ID))
            {
                const DWORD now = o_GetTime();
                const int itemId = msg->item_id;
                if (state.lastClickItem == itemId && now - state.lastClick < 300 &&
                    (int)dlg->field_68 == itemId)
                {
                    state.lastClickItem = 0;
                    o_WndMgr->result_dlg_item_id = itemId;
                    msg->type = MT_MOUSEBUTTON;
                    msg->subtype = MST_EXIT;
                    msg->item_id = 10;
                    return 2;
                }
                state.lastClick = now;
                state.lastClickItem = itemId;
            }
        }
        return CALL_2(int, __thiscall, hook->GetDefaultFunc(), dlg, msg);
    }

    int __stdcall DialogDtor(HiHook* hook, _Dlg_* dlg)
    {
        auto it = dialogs.find(dlg);
        if (it != dialogs.end())
        {
            it->second.buttonFrame.Release();
            dialogs.erase(it);
        }
        if (current && current->dialog == dlg)
            current->dialog = nullptr;
        return CALL_1(int, __thiscall, hook->GetDefaultFunc(), dlg);
    }

    void Install(PatcherInstance* pi, DWORD hdVersion)
    {
        restoreHd5Interactions = hdVersion == 0 || hdVersion > 5000000;
        pi->WriteHiHook(0x4DA990, SPLICE_, EXTENDED_, THISCALL_, CheckLevelUps);
        pi->WriteLoHook(0x4DAA3A, TargetCalculated);
        pi->WriteLoHook(0x4DAC91, LevelUpSound);
        pi->WriteHiHook(0x4F8F10, SPLICE_, EXTENDED_, THISCALL_, DialogCtor);
        pi->WriteHiHook(0x4F9E10, SPLICE_, EXTENDED_, THISCALL_, DialogProc);
        pi->WriteHiHook(0x4F9DC0, SPLICE_, EXTENDED_, THISCALL_, DialogDtor);
    }
}

void NativeDlgsImprovements(PatcherInstance *_PI)
{
    HeroLevelUp::Install(_PI, _P->VarValue<DWORD>("HD.Version.Dword"));

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
