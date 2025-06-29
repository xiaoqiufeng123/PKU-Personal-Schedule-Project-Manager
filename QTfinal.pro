QT       += core gui widgets sql charts multimedia

# include(Qxlsx/Qxlsx.pri)

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

SOURCES += \
    CourseScheduleWindow.cpp \
    DailyTask.cpp \
    DailyTaskDialog.cpp \
    DatabaseManager.cpp \
    StatisticsWindow.cpp \
    StudySessionDialog.cpp \
    TaskReminderDialog.cpp \
    main.cpp \
    mainwindow.cpp \
    smartroomwidget.cpp

HEADERS += \
    CourseScheduleWindow.h \
    DailyTask.h \
    DailyTaskDialog.h \
    DatabaseManager.h \
    StatisticsWindow.h \
    StudySessionDialog.h \
    TaskReminderDialog.h \
    mainwindow.h \
    smartroomwidget.h

FORMS += \
    mainwindow.ui \
    StudySessionDialog.ui

RESOURCES += \
    resources.qrc

# 使用 QMAKE_POST_LINK 确保 Python 脚本被复制到执行目录
win32 {
    # Windows系统下的文件复制命令
    QMAKE_POST_LINK += $$quote(copy /Y $$shell_path($$PWD/scraper.py) $$shell_path($$OUT_PWD)) &&
    QMAKE_POST_LINK += $$quote(copy /Y $$shell_path($$PWD/query_free_room.py) $$shell_path($$OUT_PWD))
} else {
    # Linux/macOS系统下的文件复制命令
    QMAKE_POST_LINK += $$quote(cp $$shell_path($$PWD/scraper.py) $$shell_path($$OUT_PWD)) &&
    QMAKE_POST_LINK += $$quote(cp $$shell_path($$PWD/query_free_room.py) $$shell_path($$OUT_PWD))
}

DISTFILES += \
    statistics_app.py
