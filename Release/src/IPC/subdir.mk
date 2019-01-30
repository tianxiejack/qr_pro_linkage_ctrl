################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/IPC/ipcProc.cpp 

OBJS += \
./src/IPC/ipcProc.o 

CPP_DEPS += \
./src/IPC/ipcProc.d 


# Each subdirectory must supply rules for building sources it contributes
src/IPC/%.o: ../src/IPC/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: NVCC Compiler'
	/usr/local/cuda-8.0/bin/nvcc -I../include -I../src/msg/include -I../src/ptz/inc -I../src/JOS/include -I../src/stateCtrl -I../src/Platform/include -I../src/port -I../src/OSA_CAP/inc -I../src/comm/include -I../src/Manager -I../src/DxTimer/include -I../src/GPIO -O3 -ccbin aarch64-linux-gnu-g++ -gencode arch=compute_20,code=sm_20 -gencode arch=compute_53,code=sm_53 -m64 -odir "src/IPC" -M -o "$(@:%.o=%.d)" "$<"
	/usr/local/cuda-8.0/bin/nvcc -I../include -I../src/msg/include -I../src/ptz/inc -I../src/JOS/include -I../src/stateCtrl -I../src/Platform/include -I../src/port -I../src/OSA_CAP/inc -I../src/comm/include -I../src/Manager -I../src/DxTimer/include -I../src/GPIO -O3 --compile -m64 -ccbin aarch64-linux-gnu-g++  -x c++ -o  "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


