#-------------------------------------------------
#
# Project created by QtCreator 2012-03-04T22:03:55
#
#-------------------------------------------------

QT       += core\
	multimedia

QT       -= gui

TARGET = rtai
TEMPLATE = app

CONFIG   += console
CONFIG   -= app_bundle

SOURCES += main.cpp\
        analysis/freq_analysis.cpp\
    inputs/rt_snd_in_qt.cpp

HEADERS  +=\
	freq_analysis.h\
        base/rt_basictypes.h \
    base/rt_multibuffer.h \
    base/rt_doublebuffer.h \
    base/rt_base.h \
    analysis/freq_filtering.h \
    analysis/freq_analysis.h \
    base/rt_dataflow.h \
    base/rt_setup.h \
    base/rt_node.h \
    base/rt_common.h \
    base/rt_datatypes.h \
    inputs/rt_snd_in_te.h \
    inputs/rt_snd_in_qt.h \
    analysis/rt_cpb_qt.h \
    base/rt_tracing.h \
    analysis/rt_filter_qt.h \
    analysis/rt_pwr_qt.h \
    analysis/rt_detector_qt.h \
    base/rt_base_slbuf_ex.h \
    analysis/rt_w_pwr_qt.h

FORMS    +=

OTHER_FILES += \
    js_config_sndsource.txt \
    js_config_sndsink.txt \
    js_config_cpbanalysis.txt \
    js_config_freqshift.txt \
    js_config_plotgraph.txt \
    js_config_filteranalysis.txt \
    js_config_w_pwr_calc.txt \
    js_config_w_pwr_filter.txt \
    js_config_pwr_meas.txt \
    js_config_detector.txt

RESOURCES += \
    rt_res_defaults.qrc




