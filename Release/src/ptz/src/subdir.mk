################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/ptz/src/PTZ_control.cpp \
../src/ptz/src/PTZ_speedTransfer.cpp \
../src/ptz/src/pelcoBase.cpp \
../src/ptz/src/pelcoDformat.cpp \
../src/ptz/src/pelcoFactory.cpp \
../src/ptz/src/pelcoPformat.cpp \
../src/ptz/src/ptzInterface.cpp 

OBJS += \
./src/ptz/src/PTZ_control.o \
./src/ptz/src/PTZ_speedTransfer.o \
./src/ptz/src/pelcoBase.o \
./src/ptz/src/pelcoDformat.o \
./src/ptz/src/pelcoFactory.o \
./src/ptz/src/pelcoPformat.o \
./src/ptz/src/ptzInterface.o 

CPP_DEPS += \
./src/ptz/src/PTZ_control.d \
./src/ptz/src/PTZ_speedTransfer.d \
./src/ptz/src/pelcoBase.d \
./src/ptz/src/pelcoDformat.d \
./src/ptz/src/pelcoFactory.d \
./src/ptz/src/pelcoPformat.d \
./src/ptz/src/ptzInterface.d 


# Each subdirectory must supply rules for building sources it contributes
src/ptz/src/%.o: ../src/ptz/src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: NVCC Compiler'
	/usr/local/cuda-8.0/bin/nvcc -I../include -I../src/msg/include -I../src/ptz/inc -I../src/JOS/include -I../src/stateCtrl -I../src/Platform/include -I../src/port -I../src/OSA_CAP/inc -I../src/comm/include -I../src/Manager -I../src/DxTimer/include -I../src/GPIO -O3 -ccbin aarch64-linux-gnu-g++ -gencode arch=compute_20,code=sm_20 -gencode arch=compute_53,code=sm_53 -m64 -odir "src/ptz/src" -M -o "$(@:%.o=%.d)" "$<"
	/usr/local/cuda-8.0/bin/nvcc -I../include -I../src/msg/include -I../src/ptz/inc -I../src/JOS/include -I../src/stateCtrl -I../src/Platform/include -I../src/port -I../src/OSA_CAP/inc -I../src/comm/include -I../src/Manager -I../src/DxTimer/include -I../src/GPIO -O3 --compile -m64 -ccbin aarch64-linux-gnu-g++  -x c++ -o  "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


