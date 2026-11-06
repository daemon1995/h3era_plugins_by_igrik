//////////////////////////////////
// Author: [igrik]              //
// Date:   31.05.2019г.         //
// Thanks: baratorch, ZVS, MoP  //
//////////////////////////////////

#define _CRT_SECURE_NO_WARNINGS

#include "..\..\..\include\homm3.h"
#include "..\..\..\include\HoMM3_Dlg.cpp"
#include "..\..\..\include\HoMM3_Extra.h" 
#include "..\..\..\include\WogClasses.h"

#include <algorithm>

#include "..\..\..\include\era.h"

Patcher* _P;
PatcherInstance* _PI;

//////////////////////////////////

namespace wndText
{
    const char* PLUGIN_NAME = "WoG native dialogs";
    const char* PLUGIN_AUTHOR = "igrik";
    const char* PLUGIN_DATA = __DATE__;
} 

//////////////////////////////////

// переменная, для чистого WND
// для Берса ее коментирую
#define DOP_FUNK_TO_ERA

struct _TXT_;
_TXT_* txtresWOG;

// текстовые переменные из wnd.json
char* n_BigFont = "bigfont2.fnt";
char* n_MedFont = "medfont2.fnt";
char* n_SmallFont = "smalfont2.fnt";

char* json_CrExpo[12];
char* json_WoGOpt[2];
char* json_Npc[4];
char* json_Combat[2];

// функция получения JSON строк методом ERA
char* GetEraJSON(const char* json_string_name) {
    return Era::tr(json_string_name);
}

// мои текстовые буферы
char myString1[1024];
char myString2[1024];
char myString3[1024];

#define MyString  (char*)myString1
#define MyString1 (char*)myString1
#define MyString2 (char*)myString2
#define MyString3 (char*)myString3

// получение разрешения игры
#define o_HD_X (*(_int_*)0x401448)
#define o_HD_Y (*(_int_*)0x40144F)

// Воговская переменная локализации
#define isRusLang (*(_int_*)0x80321C)

// версия ERA
int ERA_VERSION = NULL;

char* radioBttnDef = "radiobttn.def";
char* checkboxDef = "checkbox.def";
char* textProcD = "%d";
char* textProcS = "%s";

// названия кнопок ОК и ОТМЕНА
#define box64x30Pcx (*(char**)0x5D5D68) // "Box64x30.pcx"
#define iOkayDef (*(char**)0x65F4CC) // "iOkay.def"
#define iCancelDef (*(char**)0x65F4D0) // "iCancel.def"
#define artifactDef (*(char**)0x65F4D8) // "artifact.def"
#define spellsDef (*(char**)0x65F4DC) // "spells.def"
#define pskillDef (*(char**)0x65F4E4) // "pskill.def"
#define sskillDef (*(char**)0x65F4EC) // "sskill.def"
#define twcrportDef (*(char**)0x65F4E8) // "twcrport.def"
#define cprsmallDef (*(char**)0x65F504) // "cprsmall.def"
#define adRollvrPcx (*(char**)0x401F03) // "AdRollvr.pcx"

// подключение всех диалогов по отдельности
#include "src\MsgBox.cpp"
#include "src\WoGMessageBox.cpp"
#include "src\ChangeBmp.cpp"
#include "src\NPC.cpp"
#include "src\CrExpo.cpp"
#include "src\Curse.cpp"
#include "src\DropArt.cpp"
#include "src\Sphinx.cpp"
#include "src\IFG.cpp"
#include "src\ChooseAttack.cpp"
#include "src\ScenarioScroll.cpp"
#include "src\WoGExternalOptions.cpp"
#include "src\WoGOptions.cpp"
#include "src\WoGOptionsStrings.cpp"
#include "src\CastleReBuild.cpp"
#include "src\BattleLogDlg.cpp"

#include "src\WoGOptionsSaveOrLoad.cpp"

#ifdef DOP_FUNK_TO_ERA
#include "src\TownHeroesMeet.cpp"
#include "src\Battle_ShowKilled.cpp"
#include "src\ViewWorldSpells.cpp"
#endif DOP_FUNK_TO_ERA

// Shared frame helpers are also used by the commander dialog in plain WND builds.
#include "src\NativeDlgsImprovements.cpp"

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////

// делаем плавность отображения текста в диалоге Credits (Авторы)
_dword_ timeDlgCreditsSmoothly = 0;

int __stdcall Y_DlgCreditsSmoothly(LoHook* h, HookContext* c)
{
    if (o_GetTime() > timeDlgCreditsSmoothly + 15) {
        *(_byte_*)0x69FE55 = 1;
        timeDlgCreditsSmoothly = o_GetTime();
    }

    return EXEC_DEFAULT;
}

// показываем версию ERA в главном меню
int __stdcall Y_Dlg_MainMenu_Create(HiHook* hook, _Dlg_* dlg) 
{
    int ret = CALL_1(int, __thiscall, hook->GetDefaultFunc(), dlg);

    const char* ERA_version = Era::GetEraVersion();
    sprintf(o_TextBuffer, "HoMM3 ERA %s", ERA_version );
    dlg->AddItem(_DlgStaticText_::Create(590, 570, 200, 20, o_TextBuffer, n_MedFont, 7, 545, ALIGN_H_RIGHT | ALIGN_V_BOTTOM, 0)); 

    return ret;
}

// дополняем сообщение ПКМ на кнопке "Авторы"
void __stdcall OnReportVersion (Era::TEvent* Event) {
    sprintf(MyString, "{%s}: (%s)", wndText::PLUGIN_NAME, wndText::PLUGIN_DATA);
    Era::ReportPluginVersion(MyString);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////

int __stdcall Y_LoadAllTXTinGames(LoHook* h, HookContext* c)
{
    txtresWOG = _TXT_::Load( "\\zvs\\Lib1.res\\txtres.txt" );
    return EXEC_DEFAULT;
}

//-----------------------------------------------------------------------
int __stdcall Y_Hook_MainLoop(LoHook* h, HookContext* c)
{   
    n_BigFont = GetEraJSON("wnd.fonts.big_font");
    n_MedFont = GetEraJSON("wnd.fonts.med_font");
    n_SmallFont = GetEraJSON("wnd.fonts.small_font");
    // загружаем необходимые русскоязычные игровые шрифты
    bigfont2 = _Fnt_::Load(n_BigFont);
    medfont2 = _Fnt_::Load(n_MedFont);
    smalfont2 = _Fnt_::Load(n_SmallFont);

    json_CrExpo[0] = GetEraJSON("wnd.dlg_crexpo.line0");
    json_CrExpo[1] = GetEraJSON("wnd.dlg_crexpo.line1");
    json_CrExpo[2] = GetEraJSON("wnd.dlg_crexpo.line2");
    json_CrExpo[3] = GetEraJSON("wnd.dlg_crexpo.line3");
    json_CrExpo[4] = GetEraJSON("wnd.dlg_crexpo.line4");
    json_CrExpo[5] = GetEraJSON("wnd.dlg_crexpo.line5");
    json_CrExpo[6] = GetEraJSON("wnd.dlg_crexpo.line6");
    json_CrExpo[7] = GetEraJSON("wnd.dlg_crexpo.line7");
    json_CrExpo[8] = GetEraJSON("wnd.dlg_crexpo.line8");
    json_CrExpo[9] = GetEraJSON("wnd.dlg_crexpo.line9");
    json_CrExpo[10] = GetEraJSON("wnd.dlg_crexpo.line10");
    json_CrExpo[11] = GetEraJSON("wnd.dlg_crexpo.line11");

    json_WoGOpt[0] = GetEraJSON("wnd.dlg_wog_options.intro_font");
    json_WoGOpt[1] = GetEraJSON("wnd.dlg_wog_options.bttn_name");

    json_Npc[0] = GetEraJSON("wnd.dlg_commander.bttn_open_hint");
    json_Npc[1] = GetEraJSON("wnd.dlg_commander.bttn_exit_hint");
    json_Npc[2] = GetEraJSON("wnd.dlg_commander.bttn_lvlup_hint");
    json_Npc[3] = GetEraJSON("wnd.dlg_commander.bttn_dismiss_rmc");

    // теперь создаём свои хуки после всех патчей вога
	DlgWoGOptions_SaveOrLoad(_PI); // диалог загрузки и сохранения WOG Опций
    Dlg_WoGOptions(_PI);   // диалог WoG Опций 	
    Dlg_NPC(_PI);          // диалог командира  
    Dlg_MsgBox(_PI);       // новый и расширенный Callback диалога MsgBox

#ifdef DOP_FUNK_TO_ERA
    json_Combat[0] = GetEraJSON("wnd.combat.finish_question");
    json_Combat[1] = GetEraJSON("wnd.combat.show_killed");
    Battle_ShowKilled(_PI);         // показ предполагаемого количества убитых монстров при атаке и стрельбе
    Dlg_TownHeroesMeet(_PI);        // обмен героями в замке ко клавише E
    Dlg_ViewWorldScreen(_PI);       // сохранение эффектов заклинаний просмотра земли и воздуха при открытии диалога просмотра мира
    NativeDlgsImprovements(_PI);    // улучшения нативных диалогов для повышения удобства пользования игрой
#endif DOP_FUNK_TO_ERA

    return EXEC_DEFAULT;
} 


/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    static _bool_ plugin_On = 0;
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        if (!plugin_On)
        {
            plugin_On = 1;    

            _P = GetPatcher();
            _PI = _P->CreateInstance((char*)wndText::PLUGIN_NAME); 

            // подтягиваем ERA
            Era::ConnectEra(hModule, wndText::PLUGIN_NAME);
            WoGExternalOptions::Publish(_P);
            // External locks must apply even when the player never opens the
            // WoG Options dialog. Providers may register from their DllMain,
            // but their callbacks are invoked only after plugin loading.
            Era::RegisterHandler(WoGExternalOptions::OnDeferredLockRefresh,
                "OnAfterWoG");
            Era::RegisterHandler(WoGExternalOptions::OnDeferredLockRefresh,
                "OnAfterCreateWindow");
            Era::RegisterHandler(WoGExternalOptions::OnDeferredLockRefresh,
                "OnAfterReloadLanguageData");
            // и сразу получаем версию ERA
            ERA_VERSION = Era::GetVersionNum();
            Era::RegisterHandler(OnReportVersion, "OnReportVersion");

            // создаем загрузку необходимых тектовиков
            _PI->WriteLoHook(0x4EDD65, Y_LoadAllTXTinGames);

            // загрузка MainLoop
            _PI->WriteLoHook(0x4EEAC0, Y_Hook_MainLoop);

            // делаем показ версии игры в главном меню
            // _PI->WriteHiHook(0x4FB930, SPLICE_, EXTENDED_, THISCALL_, Y_Dlg_MainMenu_Create);
            // делаем более плавное отображение credits
            _PI->WriteLoHook(0x4EE674, Y_DlgCreditsSmoothly);
            _PI->WriteByte(0x4EE6C0 +2, 1); // начало: до достижения копирайтов до верха
            _PI->WriteByte(0x4EE716 +2, 1); // середина: когда текст есть и снизу, и сверху
            _PI->WriteByte(0x4EE75E +2, 1); // конец 1: когда текста снизу нет, только сверху
            _PI->WriteByte(0x4EE761 +2, 1); // конец 2: когда текста снизу нет, только сверху

            // коректировка описаний монстров
            // не в бою
            _PI->WriteDword(0x5F3E3F, 0x660CB4);    // шрифт tiny.fnt
            _PI->WriteDword(0x5F3E4C, 234);         // поз.Y
            _PI->WriteByte(0x5F3E51, 22);           // поз.X 
            // в бою
            _PI->WriteDword(0x5F446A, 0x660CB4);    // шрифт tiny.fnt
            _PI->WriteDword(0x5F4477, 234);         // поз.Y
            _PI->WriteByte(0x5F447C, 22);           // поз.X 
            // при покупке
            _PI->WriteDword(0x5F4885, 0x660CB4);    // шрифт tiny.fnt
            _PI->WriteDword(0x5F4892, 234);         // поз.Y
            _PI->WriteByte(0x5F4897, 22);           // поз.X 

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////

            WoGOptionsStrings(_PI); // вырезаем текстовик "zsetup00.txt"
            Dlg_ChangeBmp(_PI); // все замены кадров
            Dlg_ExpaMon(_PI);   // диалог Экспы монстров
            Dlg_Curse(_PI);     // диалог проклятий и благословлений
            Dlg_DropArt(_PI);   // диалог сброса артефактов на землю    
            Dlg_Sphinx(_PI);    // диалог сфинкса и IF:D/E
            Dlg_IFG(_PI);       // диалог IF:G
            Dlg_ChooseAttack(_PI);  // диалог выбора типа атаки монстром
            Dlg_CastleReBuild(_PI); // диалог перестройки замка
            DlgBattleLog(_PI);  // окно диалога статуса действий и событий в битве

            // char* oVersionERA = "{Game Version:}\n\nHoMM3 ERA 2.7.7 \n (with Wog Native Dialogs)";
            // _PI->WriteDword(0x7066E1 +1, *(int*)&oVersionERA); // ENG
            // _PI->WriteDword(0x7066CE +1, *(int*)&oVersionERA); // RUS            
            Dlg_WoGMessageBox(_PI); // Воговский текстовый диалог

        }
        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}


