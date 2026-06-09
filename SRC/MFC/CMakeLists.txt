# MFC module - the game's MFC app/UI core (dialogs, views, frame, map view).
# Faithful to BOB.DSP: _MFC.CPP is the unity TU that #includes the dialog/view
# fragments (MIGView/MainFrm/MapDlg/MIG/MIGDoc/RDialog/Keystub/... + fullpsys).
# Compiles against the from-scratch minimal MFC compat layer (compat/afxwin.h,
# compat/streams.h). Needs SRC/MFC on the include path (resource.h lives there).
add_library(bob_mfc STATIC
    _MFC.CPP
    _AFX.CPP
    _RAF.CPP
    _LW.CPP
    _SA.CPP
    _TOOL.CPP
    _FULL.CPP
)
target_include_directories(bob_mfc PUBLIC ${BOB_SRC}/compat ${BOB_SRC}/H ${BOB_SRC}/MFC)
