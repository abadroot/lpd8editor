include(../common.pri)

TARGET = lpd8-editor

SOURCES += main.cpp \
    db.cpp \
    utils.cpp \
    lpd8_sysex.cpp \
    mainwindow.cpp \
    midivaluedelegate.cpp \
    commands.cpp \
    programsmodel.cpp \
    iomidi.cpp \
    midiconnectionsmodel.cpp \
    sysexhandler.cpp

RESOURCES += \
    sql.qrc

HEADERS += \
    db.h \
    utils.h \
    lpd8_sysex.h \
    mainwindow.h \
    midivaluedelegate.h \
    enums.h \
    commands.h \
    programsmodel.h \
    iomidi.h \
    midiconnectionsmodel.h \
    sysexhandler.h

FORMS += \
    mainwindow.ui
