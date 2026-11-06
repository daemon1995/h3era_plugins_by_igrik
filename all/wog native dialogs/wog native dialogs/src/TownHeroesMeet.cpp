// окно обмена героями в городе по клавише E

bool inTownDlg = false;
constexpr auto BUTTON_HK = HK_E;
constexpr auto BUTTON_ID = 200;
constexpr LPSTR HMS_DEF_NAME = "townhrtd.def";
constexpr LPSTR HMS_PCX_NAME = "Townhrtr.pcx";
constexpr LPSTR HMS_BUTTON_HINT = "wnd.dlg_town.hms_button.hint";
constexpr LPSTR HMS_BUTTON_RMC = "wnd.dlg_town.hms_button.rmc";
Patch *blockScreenUpdate = nullptr;
int __stdcall Y_DlgTown_Proc(HiHook *hook, _TownMgr_ *tm, _EventMsg_ *msg)
{
    if (msg->type == MT_MOUSEBUTTON && msg->subtype == MST_LBUTTONCLICK && msg->item_id == BUTTON_ID)
    {
        const int heroU_id = tm->town->up_hero_id;
        const int heroD_id = tm->town->down_hero_id;
        const bool isActive = heroU_id != -1 && heroD_id != -1 && (!o_NetworkGame || o_ActivePlayerID == o_MeID);

        if (isActive)
        {
            inTownDlg = true;

            _Hero_ *heroU = o_GameMgr->GetHero(heroU_id);
            _Hero_ *heroD = o_GameMgr->GetHero(heroD_id);

            auto mgr = o_ExecMgr->next;

            o_TownMgr->mgr.SetManagers(&o_AdvMgr->mgr, &o_WndMgr->mgr);
            o_AdvMgr->mgr.SetManagers(nullptr, &o_WndMgr->mgr);
            o_WndMgr->mgr.SetManagers(&o_TownMgr->mgr, &o_MouseMgr->mgr);

            _DlgItem_ *switchButton = tm->dlg->GetItem(BUTTON_ID);
            _DlgItem_ *closeDlgbutton = tm->dlg->GetItem(ID_OK_10);

            // скрываем кнопки, потому что игра продолжает их обрабатывать... не знаю, как исправить
            // P.S.: для карты идёт деактивация кнопко по ID, так что подход относительной верный
            if (switchButton)
                switchButton->Hide();
            if (closeDlgbutton)
                closeDlgbutton->Hide();

            o_TownMgr->mgr.isActive = false;
            blockScreenUpdate->Apply();
            _P->VarValue<_bool_>("HotA.SwapMgrCalledFromTown") = 1;

            // вызываем обмен героями
            heroU->TeachScholar(heroD);
            o_AdvMgr->SwapHeroes(heroU, heroD);

            _P->VarValue<_bool_>("HotA.SwapMgrCalledFromTown") = 0;
            blockScreenUpdate->Undo();

            if (switchButton)
                switchButton->Show();
            if (closeDlgbutton)
                closeDlgbutton->Show();

            o_TownMgr->mgr.isActive = true;
            o_TownMgr->UnHighlightArmy();
            o_TownMgr->Redraw();

            inTownDlg = false;
        }
    }

    return CALL_2(int, __thiscall, hook->GetDefaultFunc(), tm, msg);
}
void __stdcall GarrisonInterface_SetGraphics(HiHook *hook, const DWORD harrisonInterface, const int redraw,
                                             const int selectedCreatureID)
{
    const auto *tm = o_TownMgr;
    if (tm && tm->town && tm->dlg && tm->dlg == o_WndMgr->dlg_last)
    {
        _DlgItem_ *switchButton = tm->dlg->GetItem(BUTTON_ID);
        if (switchButton)
        {
            const int heroU_id = tm->town->up_hero_id;
            const int heroD_id = tm->town->down_hero_id;
            const bool isActive = heroU_id != -1 && heroD_id != -1 && (!o_NetworkGame || o_ActivePlayerID == o_MeID);
            CALL_2(void, __thiscall, 0x05FEF00, switchButton, isActive); //
        }
    }
    return CALL_3(void, __thiscall, hook->GetDefaultFunc(), harrisonInterface, redraw, selectedCreatureID);
}
// перед инициализацией виджетов диалога добавить кнопку в список
_LHF_(TownDlg_Create)
{
    if (_Dlg_ *dlg = reinterpret_cast<_Dlg_ *>(c->edi))
    {
        constexpr int width = 59;
        constexpr int height = 20;
        constexpr int x = 269 - width / 2;
        constexpr int y = 466 - height / 2;
        // создаём рамку
        dlg->AddItemToOwnArrayList(_DlgStaticPcx8_::Create(x - 1, y - 1, -1, HMS_PCX_NAME));
        // создаём кнопку
        dlg->AddItemToOwnArrayList(
            _DlgButton_::Create(x, y, width, height, BUTTON_ID, HMS_DEF_NAME, 0, 1, 0, BUTTON_HK, 2));
    }

    return EXEC_DEFAULT;
}
_LHF_(TownDlg_GetItemHint)
{
    // если ховер на шей кнопке
    if (c->edi == BUTTON_ID)
    {
        c->edi = reinterpret_cast<int>(Era::tr(HMS_BUTTON_HINT));
        c->return_address = 0x05C82B2;
        return NO_EXEC_DEFAULT;
    }

    return EXEC_DEFAULT;
}
_LHF_(TownDlg_GetItemRmcHint)
{
    // если пкм на шей кнопке
    if (IntAt(c->ebp + 0x8) && c->edi == BUTTON_ID)
    {
        b_MsgBoxC(Era::tr(HMS_BUTTON_RMC), MBX_RMC, -1, -1);
    }

    return EXEC_DEFAULT;
}

void __stdcall Y_Dlg_HeroesMeetCreate(HiHook *hook, const _WndMgr_ *wndMgr, _Dlg_ *dlg, const int order, const int draw)
{

    CALL_4(int, __thiscall, hook->GetDefaultFunc(), wndMgr, dlg, order, draw);
    if (dlg)
    {
        if (_DlgButton_ *closeDlgbutton = reinterpret_cast<_DlgButton_ *>(dlg->GetItem(ID_OK_10)))
        {
            closeDlgbutton->SetHotKey(HK_ESC);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Dlg_TownHeroesMeet(PatcherInstance *_PI)
{

    // обмен героями в замке ко клавише E
    if (atoi(GetEraJSON("wnd.dlg_town.hms_button.enabled")))
    {
        blockScreenUpdate = _PI->WriteHexPatch(0x04AAC21, "90 90 90 90 90 90 90 90 90");

        // добавить кнопку в диалог города
        _PI->WriteLoHook(0x05C5C57, TownDlg_Create);

        // основная процедура
        _PI->WriteHiHook(0x5D3640, SPLICE_, EXTENDED_, THISCALL_, Y_DlgTown_Proc);

        // сменить состояние кнопки
        _PI->WriteHiHook(0x05AA0C0, SPLICE_, EXTENDED_, THISCALL_, GarrisonInterface_SetGraphics);

        // хинты. Хз, зачем здесь, если мог в мейн процедуру добавить :thinkking:
        _PI->WriteLoHook(0x05C7D4B, TownDlg_GetItemHint);
        _PI->WriteLoHook(0x05D475B, TownDlg_GetItemRmcHint);
    }

    // добавить "ESC" как хоткей для закрытия диалога встречи
    // из-за HD mod приходиться ставить хук прямо перед показом диалога... жесть
    _PI->WriteHiHook(0x05AED58, CALL_, EXTENDED_, THISCALL_, Y_Dlg_HeroesMeetCreate);
}
