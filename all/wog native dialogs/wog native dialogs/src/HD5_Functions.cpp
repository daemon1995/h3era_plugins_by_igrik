// всё что касается HD мода
PatcherInstance* _HD;
_bool_ isHD = false;
_int_ HD_Version;
#define hdv(type, name) _P->VarValue<type>((_cstr_)(name))

///////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////

_dword_ time_click_MsgBox, dlgHeroLvlUp_heroID;   

int __stdcall Y_Dlg_HeroLvlUp_Proc(HiHook* hook, int this_, _EventMsg_* msg)
{
    // _Dlg_* dlg = *(_Dlg_**)0x699684;

    if (msg->type == MT_MOUSEBUTTON)
    {
        if (msg->subtype == MST_RBUTTONDOWN)
        {
            if (msg->item_id == 2001)
            {
                CALL_4(void, __fastcall, 0x4E1A70, dlgHeroLvlUp_heroID, 1, 1, 1);
            }
        }
        if (msg->subtype == MST_LBUTTONDOWN)
        {
            int ID = msg->item_id;
            if (ID == 2001)
            {
                CALL_4(void, __fastcall, 0x4E1A70, dlgHeroLvlUp_heroID, 1, 1, 0);
            }
            if ( (ID == 2010) || (ID == 2011) )
            {
                if ( (o_GetTime() - time_click) < 300 && *(int*)( *(int*)0x699684 +104) == msg->item_id ) 
                {
                    time_click = 0;
                    o_WndMgr->result_dlg_item_id = *(int*)( *(int*)0x699684 +104) ;
                    msg->type = 512;
                    msg->subtype = 10;
                    msg->item_id = 10;
                    return 2;

                }  else time_click = o_GetTime();
            }
            
        }
    }
    return CALL_2(int, __thiscall, hook->GetDefaultFunc(), this_, msg);
}

int __stdcall Y_Lo_Dlg_HeroLvlUp_Create(LoHook* h, HookContext* c)
{
    dlgHeroLvlUp_heroID = (*(_Hero_**)(c->ebp +8))->id;
    return EXEC_DEFAULT;
} 

///////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////

void StartHD5Functions()
{
    // дабблклик в окне повышения уровня героя
    _PI->WriteHiHook(0x4F9E10, SPLICE_, EXTENDED_, THISCALL_, Y_Dlg_HeroLvlUp_Proc);
    // _PI->WriteHiHook(0x4F8F10, SPLICE_, EXTENDED_, THISCALL_, Y_Dlg_HeroLvlUp_Create);
    _PI->WriteLoHook(0x4F8F15, Y_Lo_Dlg_HeroLvlUp_Create);

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void HD5_Functions(PatcherInstance* _PI)
{
    // создание функции, вырезанных в HD 5й версии
    HD_Version = _P->VarValue<_dword_>("HD.Version.Dword");
    if ( HD_Version == 0 || HD_Version > 5000000 ) { // версия HD 5.000.RC0
        StartHD5Functions();
    }   
}
