# Divoom port layer — F101 RTOS build links rtos_f101/ only.
# Simulator (VS) compiles port/sim_win/*.c via product.props.

subdir-ccflags-y += -I projects/f101s3/astrotoo/src/port/include
subdir-ccflags-y += -I projects/f101s3/astrotoo/src/middle/include
subdir-ccflags-y += -I include/FreeRTOS_POSIX
subdir-ccflags-y += -D_POSIX_TIMERS

obj-y += rtos_f101/
