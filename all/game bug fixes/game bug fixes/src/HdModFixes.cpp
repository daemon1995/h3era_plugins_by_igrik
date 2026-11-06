/////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////// ИСПРАВЛЕНИЕ БАГОВ HD MOD /////////////////////////////////////////
#include <algorithm>
// Координаты боя в HD.
_int_ HD_Battle_X = 0;
_int_ HD_Battle_Y = 0;

namespace HdMod
{

void __stdcall RandomMap_WriteHeader_AssignName(HiHook *h, _HStr_ *this_, LPCSTR mapName, int len)
{
    CALL_3(_HStr_ *, __thiscall, h->GetDefaultFunc(), this_, mapName, len);

    std::string str(this_->c_str);
    str.erase(std::remove(str.begin(), str.end(), '\x12'), str.end());
    str.erase(std::remove(str.begin(), str.end(), '\x1'), str.end());
    str.erase(std::remove(str.begin(), str.end(), '\x2'), str.end());

    this_->Set((char *)str.c_str());
}
// исправление положения подсказки в окне города из-за смещения хинтбара hd-mod
_LHF_(WoG_TownManager_SetCustomHint)
{
    // Исправляем баг SoD с пропаданием выбора игрока при показе списка карт.

    if (auto *hintBar = o_TownMgr->dlg->GetItem(151))
    {
        const int yRedrawPos = hintBar->y + hintBar->parent->y;
        IntAt(0x071286C + 1) = yRedrawPos;
        h->Undo();
    }
    return EXEC_DEFAULT;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

void HdModFixes(Patcher *_P, PatcherInstance *_PI)
{

    // Получение координат поля боя.
    HD_Battle_X = _P->VarGetValue<_int32_>("HD.Battle.X", 0);
    HD_Battle_Y = _P->VarGetValue<_int32_>("HD.Battle.Y", 0);



    // исправление поломки имени случайной карты при генерации со включённым тывиком "HD.Misc.RenameRandMap" в
    // настройках HD Mod
    const int renameRandMap = _P->VarGetValue<bool>("HD.Misc.RenameRandMap", FALSE);
    if (renameRandMap)
    {
        _PI->WriteHiHook(0x54A264, CALL_, EXTENDED_, THISCALL_, RandomMap_WriteHeader_AssignName);

        // H3API solution
        /**

        void __stdcall HiHook_Test3(HiHook* h, H3String* this_, LPCSTR mapName, int len)
        {
            THISCALL_3(char, h->GetDefaultFunc(), this_, mapName, len);
            this_->Remove("\x12");
            this_->Remove("\x1");
            this_->Remove("\x2");
        }

        _PI->WriteHiHook(0x54A264, CALL_, EXTENDED_, THISCALL_, HiHook_Test3);
        */
    }
    // исправление положения подсказки в окне города из-за смещения хинтбара hd-mod
    if (o_HD_Y > 600)
    {
        _PI->WriteLoHook(0x071285E, WoG_TownManager_SetCustomHint);
    }
}
} // namespace HdMod
