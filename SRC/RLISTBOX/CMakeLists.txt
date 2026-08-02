# R* ActiveX controls module - the genuine COleControl subclasses (CRListBoxCtrl,
# CRComboCtrl, ...) brought into the Linux build so the front-end config/loadout/map
# screens host the real controls (data + OnDraw) via the bob_ole compat hosting layer,
# rather than the no-op InvokeHelper path. The design-time property-page sources
# (RLISTBXP/LP2/RCOMBOP) are NOT compiled -- never instantiated at runtime, only
# referenced inside no-op PROPPAGEID macros.
add_library(bob_rlistbox STATIC
    RLISTBXC.CPP                  # CRListBoxCtrl
    ${BOB_SRC}/RCOMBO/RCOMBOC.CPP # CRComboCtrl
    ${BOB_SRC}/RSTATIC/RSTATICC.CPP # CRStaticCtrl
    ${BOB_SRC}/RBUTTON/RBUTTONC.CPP # CRButtonCtrl
    ${BOB_SRC}/REDIT/REDITCTL.CPP # CREditCtrl (S125: campaign enter-name edit)
    ${BOB_SRC}/RRADIO/RRADIOC.CPP # CRRadioCtrl (S128: Quick-Shots page tabs)
    bob_ole.cpp                   # control-agnostic side-table + entry points
    bob_ole_rlistbox.cpp          # RListBox host (its RLISTBOX/resource.h context)
    ${BOB_SRC}/RCOMBO/bob_ole_rcombo.cpp   # RCombo host
    ${BOB_SRC}/RSTATIC/bob_ole_rstatic.cpp # RStatic host
    ${BOB_SRC}/RBUTTON/bob_ole_rbutton.cpp # RButton host
    ${BOB_SRC}/REDIT/bob_ole_redit.cpp # REdit host (S125)
    ${BOB_SRC}/RRADIO/bob_ole_rradio.cpp # RRadio host (S128)
    ${BOB_SRC}/RBUTTON/GETFILE.CPP  # GetFileNum (FileNumString -> FileNum, CRButtonCtrl)
    ${BOB_SRC}/MFC/GETSHADW.CPP   # shadow-text offset helpers the controls draw with
    ${BOB_SRC}/MFC/GETRESRC.CPP   # GetResourceNumberFromID / ConvertResourceID (RStatic)
)
target_include_directories(bob_rlistbox PUBLIC
    ${BOB_SRC}/compat ${BOB_SRC}/H ${BOB_SRC}/MFC ${BOB_SRC}/RLISTBOX ${BOB_SRC}/RCOMBO ${BOB_SRC}/RSTATIC ${BOB_SRC}/RBUTTON ${BOB_SRC}/REDIT ${BOB_SRC}/RRADIO)
# GETFILE.CPP reads h/F_GRAFIX.G (button-art FileNum resolution) from the repo source tree.
set_source_files_properties(${BOB_SRC}/RBUTTON/GETFILE.CPP PROPERTIES
    COMPILE_DEFINITIONS "BOB_SRC_DIR=\"${BOB_SRC}\"")
