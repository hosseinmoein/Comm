## Hossein Moein
## March 25, 2018

LOCAL_LIB_DIR = ../lib/$(BUILD_PLATFORM)
LOCAL_BIN_DIR = ../bin/$(BUILD_PLATFORM)
LOCAL_OBJ_DIR = ../obj/$(BUILD_PLATFORM)
LOCAL_INCLUDE_DIR = ../include
PROJECT_LIB_DIR = ../../../lib/$(BUILD_PLATFORM)
PROJECT_INCLUDE_DIR = ../../include

# -----------------------------------------------------------------------------

SRCS = SocketBase.cc \
       RegularSocket.cc \
       socket_tester.cc \
       messageq_tester.cc
HEADERS = $(LOCAL_INCLUDE_DIR)/Communication.h \
          $(LOCAL_INCLUDE_DIR)/SocketBase.h \
          $(LOCAL_INCLUDE_DIR)/Selector.h \
          $(LOCAL_INCLUDE_DIR)/Pipe.h \
          $(LOCAL_INCLUDE_DIR)/RegularSocket.h \
          $(LOCAL_INCLUDE_DIR)/FixedSizeSocket.h \
          $(LOCAL_INCLUDE_DIR)/Acceptor.h \
          $(LOCAL_INCLUDE_DIR)/Acceptor.tcc \
          $(LOCAL_INCLUDE_DIR)/MessageQueue.h \
          $(LOCAL_INCLUDE_DIR)/MessageQueue.tcc

LIB_NAME = Comm
TARGET_LIB = $(LOCAL_LIB_DIR)/lib$(LIB_NAME).a

TARGETS = $(TARGET_LIB) \
          $(LOCAL_BIN_DIR)/socket_tester \
          $(LOCAL_BIN_DIR)/messageq_tester

# -----------------------------------------------------------------------------

LFLAGS += -Bstatic -L$(LOCAL_LIB_DIR) -L$(PROJECT_LIB_DIR)

LIBS = $(LFLAGS) -l$(LIB_NAME) $(PLATFORM_LIBS)
INCLUDES += -I. -I$(LOCAL_INCLUDE_DIR) -I$(PROJECT_INCLUDE_DIR)
DEFINES = -D_REENTRANT -DDMS_INCLUDE_SOURCE \
          -DP_THREADS -D_POSIX_PTHREAD_SEMANTICS -DDMS_$(BUILD_DEFINE)__

# -----------------------------------------------------------------------------

# object file
#
LIB_OBJS = $(LOCAL_OBJ_DIR)/SocketBase.o \
           $(LOCAL_OBJ_DIR)/RegularSocket.o \
           $(LOCAL_OBJ_DIR)/FixedSizeSocket.o

# -----------------------------------------------------------------------------

# set up C++ suffixes and relationship between .cc and .o files
#
.SUFFIXES: .cc

$(LOCAL_OBJ_DIR)/%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

.cc :
	$(CXX) $(CXXFLAGS) $< -o $@ -lm $(TLIB) -lg++

# -----------------------------------------------------------------------------

all: PRE_BUILD $(TARGETS)

PRE_BUILD:
	mkdir -p $(LOCAL_LIB_DIR)
	mkdir -p $(LOCAL_BIN_DIR)
	mkdir -p $(LOCAL_OBJ_DIR)
	mkdir -p $(PROJECT_LIB_DIR)
	mkdir -p $(PROJECT_INCLUDE_DIR)

$(TARGET_LIB): $(LIB_OBJS)
	ar -clrs $(TARGET_LIB) $(LIB_OBJS)

SOCKET_TESTER_OBJ = $(LOCAL_OBJ_DIR)/socket_tester.o
$(LOCAL_BIN_DIR)/socket_tester: $(SOCKET_TESTER_OBJ) $(HEADERS)
	$(CXX) -o $@ $(SOCKET_TESTER_OBJ) $(LIBS)

MESSAGEQ_TESTER_OBJ = $(LOCAL_OBJ_DIR)/messageq_tester.o
$(LOCAL_BIN_DIR)/messageq_tester: $(MESSAGEQ_TESTER_OBJ) $(HEADERS)
	$(CXX) -o $@ $(MESSAGEQ_TESTER_OBJ) $(LIBS)

# -----------------------------------------------------------------------------

depend:
	makedepend $(CXXFLAGS) -Y $(SRC)

clobber:
	rm -f $(LIB_OBJS) $(TARGETS) $(SOCKET_TESTER_OBJ) $(MESSAGEQ_TESTER_OBJ)

install_lib:
	cp -pf $(TARGET_LIB) $(PROJECT_LIB_DIR)/.

install_hdr:
	cp -pf $(HEADERS) $(PROJECT_INCLUDE_DIR)/.

# -----------------------------------------------------------------------------

## Local Variables:
## mode:Makefile
## tab-width:4
## End:
