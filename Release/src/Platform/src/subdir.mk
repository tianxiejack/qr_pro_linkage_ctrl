################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/Platform/src/Kalman.cpp \
../src/Platform/src/PlatformInterface.cpp \
../src/Platform/src/deviceUser.cpp \
../src/Platform/src/platformControl.cpp \
../src/Platform/src/platformFilter.cpp \
../src/Platform/src/sensorComp.cpp 

OBJS += \
./src/Platform/src/Kalman.o \
./src/Platform/src/PlatformInterface.o \
./src/Platform/src/deviceUser.o \
./src/Platform/src/platformControl.o \
./src/Platform/src/platformFilter.o \
./src/Platform/src/sensorComp.o 

CPP_DEPS += \
./src/Platform/src/Kalman.d \
./src/Platform/src/PlatformInterface.d \
./src/Platform/src/deviceUser.d \
./src/Platform/src/platformControl.d \
./src/Platform/src/platformFilter.d \
./src/Platform/src/sensorComp.d 


# Each subdirectory must supply rules for building sources it contributes
src/Platform/src/%.o: ../src/Platform/src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: NVCC Compiler'
	/usr/local/cuda-8.0/bin/nvcc -I../include -I../src/msg/include -I../src/ptz/inc -I../src/JOS/include -I../src/stateCtrl -I../src/Platform/include -I../src/port -I../src/OSA_CAP/inc -I../src/comm/include -I../src/Manager -I../src/DxTimer/include -I../src/GPIO -O3 -ccbin aarch64-linux-gnu-g++ -gencode arch=compute_30,code=sm_30 -gencode arch=compute_53,code=sm_53 -m64 -odir "src/Platform/src" -M -o "$(@:%.o=%.d)" "$<"
	/usr/local/cuda-8.0/bin/nvcc -I../include -I../src/msg/include -I../src/ptz/inc -I../src/JOS/include -I../src/stateCtrl -I../src/Platform/include -I../src/port -I../src/OSA_CAP/inc -I../src/comm/include -I../src/Manager -I../src/DxTimer/include -I../src/GPIO -O3 --compile -m64 -ccbin aarch64-linux-gnu-g++  -x c++ -o  "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


