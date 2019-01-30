################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/msg/src/CMessage.cpp 

OBJS += \
./src/msg/src/CMessage.o 

CPP_DEPS += \
./src/msg/src/CMessage.d 


# Each subdirectory must supply rules for building sources it contributes
src/msg/src/%.o: ../src/msg/src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: NVCC Compiler'
	/usr/local/cuda-8.0/bin/nvcc -DLINKAGE_FUNC=1 -DTRACK_FUNC=0 -I../include -I../src/msg/include -I../src/ptz/inc -I../src/JOS/include -I../src/stateCtrl -I../src/Platform/include -I../src/port -I../src/OSA_CAP/inc -I../src/comm/include -I../src/Manager -I../src/DxTimer/include -I../src/GPIO -G -g -O0 -ccbin aarch64-linux-gnu-g++ -gencode arch=compute_53,code=sm_53 -m64 -odir "src/msg/src" -M -o "$(@:%.o=%.d)" "$<"
	/usr/local/cuda-8.0/bin/nvcc -DLINKAGE_FUNC=1 -DTRACK_FUNC=0 -I../include -I../src/msg/include -I../src/ptz/inc -I../src/JOS/include -I../src/stateCtrl -I../src/Platform/include -I../src/port -I../src/OSA_CAP/inc -I../src/comm/include -I../src/Manager -I../src/DxTimer/include -I../src/GPIO -G -g -O0 --compile -m64 -ccbin aarch64-linux-gnu-g++  -x c++ -o  "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


