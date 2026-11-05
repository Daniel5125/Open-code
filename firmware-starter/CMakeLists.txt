cmake_minimum_required(VERSION 3.20)
project(firmware_starter C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PORT "host" CACHE STRING "Plataforma: host | stm32f411")
set_property(CACHE PORT PROPERTY STRINGS host stm32f411)

set(WARNINGS -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Wstrict-prototypes -Werror)

# Capas portables: no conocen el hardware, solo hal/hal.h
add_library(core STATIC
  src/drivers/led.c src/drivers/tmp102.c src/drivers/button.c
  src/services/scheduler.c src/services/cli.c
  src/comm/frame.c
  src/app/app.c)
target_include_directories(core PUBLIC src)
target_compile_options(core PRIVATE ${WARNINGS})

if(PORT STREQUAL "host")
  add_executable(firmware src/main.c ports/host/hal_host.c)
  target_link_libraries(firmware PRIVATE core m)
  target_compile_options(firmware PRIVATE ${WARNINGS})

  option(FW_SANITIZE "AddressSanitizer + UBSan" OFF)
  if(FW_SANITIZE)
    add_compile_options(-fsanitize=address,undefined -fno-sanitize-recover=all)
    add_link_options(-fsanitize=address,undefined)
  endif()

  include(FetchContent)
  FetchContent_Declare(unity GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git GIT_TAG v2.6.1 GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(unity)
  enable_testing()
  add_executable(test_firmware tests/test_firmware.c tests/fakes/fake_hal.c)
  target_link_libraries(test_firmware PRIVATE core unity)
  add_test(NAME firmware COMMAND test_firmware)

elseif(PORT STREQUAL "stm32f411")
  set(LD ${CMAKE_SOURCE_DIR}/ports/stm32f411/STM32F411CEUx.ld)
  add_executable(firmware.elf src/main.c ports/stm32f411/hal_stm32f411.c ports/stm32f411/startup.c ports/stm32f411/syscalls.c)
  target_include_directories(firmware.elf PRIVATE ports/stm32f411)
  target_link_libraries(firmware.elf PRIVATE core)
  target_compile_options(firmware.elf PRIVATE ${WARNINGS})
  target_link_options(firmware.elf PRIVATE -T${LD} -Wl,-Map=firmware.map -Wl,--print-memory-usage)
  set_target_properties(firmware.elf PROPERTIES LINK_DEPENDS ${LD})
  add_custom_command(TARGET firmware.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary firmware.elf firmware.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex firmware.elf firmware.hex
    COMMAND ${CMAKE_SIZE} firmware.elf
    COMMENT "Generando firmware.bin y firmware.hex")
else()
  message(FATAL_ERROR "PORT desconocido: ${PORT}")
endif()
