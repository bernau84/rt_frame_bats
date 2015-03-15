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
        analysis/freq_analysis.cpp\
        analysis/rt_analysis.cpp\
    outputs/rt_output.cpp \
    inputs/rt_sources.cpp \
    base/rt_basictypes.cpp \
    base/rt_base.cpp \
    base/rt_dataflow.cpp \
    analysis/rt_cpb.cpp \
    analysis/rt_shift.cpp

HEADERS  += mainwindow.h\
	freq_analysis.h\
        analysis/rt_analysis.h\
        base/rt_basictypes.h \
    base/rt_multibuffer.h \
    outputs/rt_output.h \
    inputs/rt_sources.h \
    base/rt_doublebuffer.h \
    base/rt_base.h \
    analysis/freq_filtering.h \
    analysis/rw_buffer.h \
    analysis/freq_analysis.h \
    base/rt_dataflow.h \
    base/rt_setup.h \
    analysis/rt_cpb.h \
    analysis/rt_shift.h \
    base/rt_circularbuffer.h \
    base/rt_node.h

FORMS    += mainwindow.ui

OTHER_FILES += \
    js_config_sndsource.txt \
    js_config_sndsink.txt \
    js_config_cpbanalysis.txt \
    js_config_freqshift.txt \
    js_config_plotgraph.txt

RESOURCES += \
    rt_res_defaults.qrc

