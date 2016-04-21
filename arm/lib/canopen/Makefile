
SRC-$(LIB_CANOPEN) += lib/canopen/canopen.c
SRC-$(LIB_CANOPEN) += lib/canopen/canopenM.c
SRC-$(LIB_CANOPEN) += lib/canopen/hal-can.c
SRC-$(LIB_CANOPEN_ANALYZER) += lib/canopen/analyzer.c

CFLAGS-$(LIB_CANOPEN) += -Ilib/canopen/inc
CFLAGS-$(LIB_CANOPEN) += -Ilib/canopen

LINTFLAGS += '+libm(lib\canopen\*.c)'
LINTFLAGS += '-ilib\canopen\inc'
LINTFLAGS += '-ilib\canopen'

