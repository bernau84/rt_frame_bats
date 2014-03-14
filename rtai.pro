#-------------------------------------------------
#
# Project created by QtCreator 2012-03-04T22:03:55
#
#-------------------------------------------------

QT       += core widgets\
	multimedia

TARGET = rtai
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp\
	freq_analysis.cpp\
	rt_analysis.cpp\
	rt_sources.cpp\
        t_rt_base.cpp \
    rt_output.cpp

HEADERS  += mainwindow.h\
	freq_analysis.h\
	rt_analysis.h\
	rt_sources.h\
	t_rt_base.h\
	t_rw_buffer.h\
	rt_basictypes.h \
    rt_multibuffer.h \
    rt_output.h

FORMS    += mainwindow.ui

OTHER_FILES += \
    todo_notes.txt \
    js_config_cpbanalysis.txt \
    js_config_freqshift.txt \
    js_config_sndsource.txt \
    js_config_sndsink.txt

RESOURCES += \
    rt_res_defaults.qrc

