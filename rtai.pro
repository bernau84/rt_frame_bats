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
        sources/rt_sources.cpp\
        baset_rt_base.cpp \
    outputs/rt_output.cpp \
    graphs/rt_graphics.cpp \
    graphs/rt_base_graph.cpp \
    graphs/rt_graphs_units.cpp \
    graphs/rt_window.cpp \
    graphs/rt_graph_scaler.cpp \
    graphs/rt_graph_context.cpp \
    graphs/rt_graph_memory.cpp \
    graphs/rt_graph_colormap.cpp \
    graphs/rt_graph_object.cpp \
    base/t_rt_base.cpp \
    inputs/rt_sources.cpp

HEADERS  += mainwindow.h\
	freq_analysis.h\
        analysis/rt_analysis.h\
        sources/rt_sources.h\
        base/t_rt_base.h\
        base/t_rw_buffer.h\
        base/rt_basictypes.h \
    base/rt_multibuffer.h \
    outputs/rt_output.h \
    analysis/freq_rt_filtering.h \
    graphs/rt_graph_units.h \
    graphs/rt_graph_scaler.h \
    graphs/rt_graph_axis.h \
    graphs/rt_graph_colormap.h \
    graphs/rt_graph.h \
    graphs/rt_ogl_frame.h \
    graphs/rt_ogl_context.h \
    graphs/rt_ogl_memory.h \
    graphs/rt_ogl_object.h \
    inputs/rt_sources.h

FORMS    += mainwindow.ui

OTHER_FILES += \
    js_config_sndsource.txt \
    js_config_sndsink.txt \
    js_config_cpbanalysis.txt \
    js_config_freqshift.txt \
    js_config_plotgraph.txt

RESOURCES += \
    rt_res_defaults.qrc

