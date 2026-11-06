#pragma once

namespace NativeDlg
{
    struct ButtonFrame
    {
        enum { WIDTH = 68 };
        _Pcx16_* buffer = nullptr;
        _DlgStaticPcx16_* item = nullptr;

        bool Create(_Dlg_* dlg, int x, int y, int height);
        void Release();
    };

    char* AutoSkipHint();
    bool IsCtrlConfirm(const _EventMsg_* msg, int okayId);
}
