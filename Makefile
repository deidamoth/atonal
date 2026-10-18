TARGET = DaisyPodVocoderGranular
CPP_SOURCES = DaisyPodVocoder.cpp

LIBDAISY_DIR = libDaisy
DAISYSP_DIR = DaisySP

OCD_DIR = /opt/homebrew/share/openocd/scripts
PGM_DEVICE = interface/stlink-dap.cfg

SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile

# ST-Link V3 (Daisy Programmer) needs dapdirect, not HLA
OCDFLAGS = -f $(PGM_DEVICE) -c "transport select dapdirect_swd" -f target/$(CHIPSET).cfg
