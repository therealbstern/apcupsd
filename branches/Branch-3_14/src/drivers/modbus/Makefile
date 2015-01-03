topdir:=../../..
include $(topdir)/autoconf/targets.mak

SRCS = mapping.cpp modbus.cpp ModbusComm.cpp ModbusRs232Comm.cpp \
       $(if $(MODBUSUSB),ModbusUsbComm.cpp)

all-targets: libmodbusdrv.a

libmodbusdrv.a: $(OBJS)
	$(MAKELIB)

# Include dependencies
-include $(DEPS)
