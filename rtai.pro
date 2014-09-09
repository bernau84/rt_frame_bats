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
	analysis\freq_analysis.cpp\
	analysis\rt_analysis.cpp\
	sources\rt_sources.cpp\
        base\t_rt_base.cpp \
    outputs\rt_output.cpp \
    graphs\rt_graphics.cpp

HEADERS  += mainwindow.h\
	freq_analysis.h\
	analysis\rt_analysis.h\
	sources\rt_sources.h\
	base\t_rt_base.h\
	base\t_rw_buffer.h\
	base\rt_basictypes.h \
    base\rt_multibuffer.h \
    outputs\rt_output.h \
    analysis\freq_rt_filtering.h \
    graphs\rt_graphics.h

FORMS    += mainwindow.ui

OTHER_FILES += \
    js_config_sndsource.txt \
    js_config_sndsink.txt \
    js_config_cpbanalysis.txt \
    js_config_freqshift.txt \
    js_config_plotgraph.txt

RESOURCES += \
    rt_res_defaults.qrc

