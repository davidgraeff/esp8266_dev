#############################################################
# Required variables for each makefile
# Discard this section from all parent makefiles
# Expected variables (with automatic defaults):
#   CSRCS (all "C" files in the dir)
#   SUBDIRS (all subdirs with a Makefile)
#   GEN_LIBS - list of libs to be generated ()
#   GEN_IMAGES - list of object file images to be generated ()
#   GEN_BINS - list of binaries to be generated ()
#   COMPONENTS_xxx - a list of libs/objs in the form
#     subdir/lib to be extracted and rolled up into
#     a generated lib/image xxx.a ()
#

ifndef TARGET
TARGET = eagle
endif
#FLAVOR = release

ifndef FLAVOR
FLAVOR = debug
endif

#EXTRA_CCFLAGS += -u

ifndef PDIR # {
SPECIAL_MKTARGETS=$(APP_MKTARGETS)
SUBDIRS=    \
	driver network

endif # } PDIR

INCLUDES := $(INCLUDES) -I $(PDIR)

APPDIR = .
LDDIR = ../ld

CCFLAGS += -Os -Wall

TARGET_LDFLAGS =		\
	-nostdlib		\
	-Wl,-EL \
	--longcalls \
	--text-section-literals

ifeq ($(FLAVOR),debug)
    TARGET_LDFLAGS += -g -O2
    CCFLAGS += -DDEBUG=1
endif

ifeq ($(FLAVOR),release)
    TARGET_LDFLAGS += -g -O0
    CCFLAGS += -DDEBUG=0
endif

COMPONENTS_eagle.app.v6 = \
	driver/libdriver.a \
	network/libnetwork.a

CONFIGURATION_DEFINES =	-DICACHE_FLASH

DEFINES +=				\
	$(UNIVERSAL_TARGET_DEFINES)	\
	$(CONFIGURATION_DEFINES)

DDEFINES +=				\
	$(UNIVERSAL_TARGET_DEFINES)	\
	$(CONFIGURATION_DEFINES)


#############################################################
# Recursion Magic - Don't touch this!!
#
# Each subtree potentially has an include directory
#   corresponding to the common APIs applicable to modules
#   rooted at that subtree. Accordingly, the INCLUDE PATH
#   of a module can only contain the include directories up
#   its parent path, and not its siblings
#
# Required for each makefile to inherit from the parent
#

INCLUDES := $(INCLUDES) -I $(PDIR)include
PDIR := ../$(PDIR)
sinclude $(PDIR)Makefile

.PHONY: FORCE
FORCE:

