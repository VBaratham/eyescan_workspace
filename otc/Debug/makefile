################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/otcLib/subdir.mk
-include src/IICLib/subdir.mk
-include src/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
ELFSIZE += \
otc.elf.size \

ELFCHECK += \
otc.elf.elfcheck \


# All Target
all: otc.elf secondary-outputs

# Tool invocations
otc.elf: $(OBJS) ../src/lscript.ld $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: MicroBlaze gcc linker'
	mb-gcc -Wl,-Map=otc.map -Wl,-T -Wl,../src/lscript.ld -L../../otc_bsp/microblaze_0/lib -mlittle-endian -mno-xl-soft-div -mcpu=v9.2 -mno-xl-soft-mul -mxl-multiply-high -Wl,--no-relax -Wl,--gc-sections -o "otc.elf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

otc.elf.size: otc.elf
	@echo 'Invoking: MicroBlaze Print Size'
	mb-size otc.elf  |tee "otc.elf.size"
	@echo 'Finished building: $@'
	@echo ' '

otc.elf.elfcheck: otc.elf
	@echo 'Invoking: Xilinx ELF Check'
	elfcheck otc.elf -hw ../../hw_platform_0/system.xml -pe microblaze_0  |tee "otc.elf.elfcheck"
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(OBJS)$(C_DEPS)$(EXECUTABLES)$(ELFSIZE)$(ELFCHECK)$(S_UPPER_DEPS) otc.elf
	-@echo ' '

secondary-outputs: $(ELFSIZE) $(ELFCHECK)

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
