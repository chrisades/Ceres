# Project Name
TARGET = Ceres

USE_DAISYSP_LGPL = 1

# Sources
CPP_SOURCES = Ceres.cpp 

CPP_SOURCES = \
    Ceres.cpp \
    $(wildcard source/*.cpp) \

# Library Locations
LIBDAISY_DIR = lib/libDaisy/
DAISYSP_DIR = lib/DaisySP/

C_INCLUDES = -Isource/

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile

CPP_STANDARD = -std=gnu++17

libs:
	cd $(LIBDAISY_DIR) && $(MAKE)
	cd $(DAISYSP_DIR) && $(MAKE)

clean-libs:
	cd $(LIBDAISY_DIR) && $(MAKE) clean
	cd $(DAISYSP_DIR) && $(MAKE) clean