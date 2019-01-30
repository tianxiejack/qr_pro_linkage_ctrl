################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/comm/src/CConnect.cpp \
../src/comm/src/CNetWork.cpp \
../src/comm/src/CPortBase.cpp \
../src/comm/src/CUartProcess.cpp \
../src/comm/src/PortFactory.cpp 

OBJS += \
./src/comm/src/CConnect.o \
./src/comm/src/CNetWork.o \
./src/comm/src/CPortBase.o \
./src/comm/src/CUartProcess.o \
./src/comm/src/PortFactory.o 

CPP_DEPS += \
./src/comm/src/CConnect.d \
./src/comm/src/CNetWork.d \
./src/comm/src/CPortBase.d \
./src/comm/src/CUartProcess.d \
./src/comm/src/PortFactory.d 


# Each subdirectory must supply rules for building sources it contributes
src/comm/src/%.o: ../src/comm/src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: NVCC Compiler'
	/usr/local/cuda-8.0/bin/nvcc -DLINKAGE_FUNC=1 -DTRACK_FUNC=0 -I../include -I../src/msg/include -I../src/ptz/inc -I../src/JOS/include -I../src/stateCtrl -I../src/Platform/include -I../src/port -I../src/OSA_CAP/inc -I../src/comm/include -I../src/Manager -I../src/DxTimer/include -I../src/GPIO -G -g -O0 -ccbin aarch64-linux-gnu-g++ -gencode arch=compute_53,code=sm_53 -m64 -odir "src/comm/src" -M -o "$(@:%.o=%.d)" "$<"
	/usr/local/cuda-8.0/bin/nvcc -DLINKAGE_FUNC=1 -DTRACK_FUNC=0 -I../include -I../src/msg/include -I../src/ptz/inc -I../src/JOS/include -I../src/stateCtrl -I../src/Platform/include -I../src/port -I../src/OSA_CAP/inc -I../src/comm/include -I../src/Manager -I../src/DxTimer/include -I../src/GPIO -G -g -O0 --compile -m64 -ccbin aarch64-linux-gnu-g++  -x c++ -o  "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


