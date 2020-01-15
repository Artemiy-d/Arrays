TEMPLATE = app
CONFIG += console c++1z
CONFIG -= app_bundle
CONFIG -= qt

equals(QMAKE_CXX, clang++) {
    message("enabling c++17 support in clang")
    QMAKE_CXXFLAGS += -std=c++17
}

SOURCES += \
        main.cpp
