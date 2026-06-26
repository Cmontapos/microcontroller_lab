##
## USB CDC communication example for STM32F429I Discovery.
##

BINARY = main
OBJS += stm_comm.o sha1_sw.o

OOCD_INTERFACE = stlink-v2-1

LDSCRIPT = ../../../../../libopencm3-plus/lib/libopencm3_plus_stm32f429idiscovery.ld

LDLIBS += -lopencm3_plus_stm32f429idiscovery -lm

include ../../Makefile.include
