/***************************************************************************//**
 * LLVM Linker script for Silicon Labs devices
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 ******************************************************************************/

MEMORY
{
  rom   (rx)  : ORIGIN = 0x8202000, LENGTH = 0x1fe000
  ram   (rwx) : ORIGIN = 0x400, LENGTH = 0x37000
}
MEMORY
{
  udma0 (rwx) : ORIGIN = 0x37400,    LENGTH = 0x400
  udma1 (rwx) : ORIGIN = 0x24061c00, LENGTH = 0x400
}

/*******************************************************************************
 * Program headers: explicit PT_LOAD with correct p_flags so LLD (LLVM/clang)
 * does not merge the RX flash code with the RW RAM data, apply wrong access
 * flags, or emit oversized load segments when one flash region feeds sections
 * whose VMA is in RAM (LMA in flash). ELF p_flags: 4=read, 5=read+execute,
 * 6=read+write. GNU ld honours the same layout.
 ******************************************************************************/
PHDRS
{
  text          PT_LOAD FLAGS(5); /* Flash: isr_vector + code + unwind tables (RX) */
  tables        PT_LOAD FLAGS(4); /* Flash: .copy.table / .zero.table (R) */
  empty         PT_LOAD FLAGS(4); /* Flash: terminates the LMA chain before NOLOAD */
  ram_pre_code  PT_LOAD FLAGS(6); /* RAM: .bss, .stack */
  ram_data      PT_LOAD FLAGS(6); /* RAM: .data (VMA in RAM, LMA in flash) */
  ram_post_data PT_LOAD FLAGS(6); /* RAM: .heap */
  udma0_init    PT_LOAD FLAGS(6); /* .udma_addr0: VMA udma0, LMA rom */
  udma1_init    PT_LOAD FLAGS(6); /* .udma_addr1: VMA udma1, LMA rom */
}

ENTRY(Reset_Handler)

SECTIONS
{
  /* Fallback symbols for copy_functions_to_ram() in startup_si91x.c.
     Sensor hub linker has no text_ram section; these zero-length spans make the copy loop a no-op. */
  PROVIDE(__ramfuncs_start__     = 0);
  PROVIDE(__ramfuncs_end__       = 0);
  PROVIDE(__lma_ramfuncs_start__ = 0);
  PROVIDE(__lma_ramfuncs_end__   = 0);

  .text :
  {
    KEEP(*(.isr_vector))
    KEEP(*(.reset_handler))

    /* Combined + de-duplicated EXCLUDE_FILE list (union of both originals) */
    *(EXCLUDE_FILE(
        *sl_si91x_dma*
        *sl_core_cortexm*
        *sl_platform_wireless*
        *sl_si91x_power_manager_wakeup_handler*
        *sl_wifi*
        *sli_wifi_power_profile*
        *sl_si91x_m4_ps*
        *rsi_sysrtc*
        *sl_si91x_clock_manager*
        *sli_si91x_clock_manager*
        *sl_sleeptimer_hal_si91x_sysrtc*
        *em_core*
        *sl_sleeptimer*
        *sl_si91x_low_power_tickless_mode*
        *sl_si91x_power_manager*
        *sli_si91x_power_manager*
        *sl_si91x_power_manager_handler*
        *sl_si91x_power_manager_debug*
        *sli_si91x_power_manager_wakeup_init*
        *sl_si91x_power_manager_wakeup_init*
        *sl_platform_wireless*
        *sl_si91x_bus*
        *sl_si91x_driver*
        *sli_wifi_command_engine*
        *sli_si91x_wifi_event_handler*
        *sl_rsi_utility*
        *sl_slist*
        *sl_main_process_action*
        *sl_event_handler*
        *cmsis_gcc*
        *cmsis_os2*
        *UDMA*
        *I2C*
        *SPI*
        *USART*
        *stdio*
        *puts*
        *wbuf*
        *writer*
        *memchr*
        *memset*
        *memcpy*
        *strcmp*
        *snprintf*
        *nano-vfprintf*
        *nano-vfprintf_i*
        *printf*
        *lock*
        *fflush*
        *syscalls*
        *sbrk*
        *libc_a*
        *libm_a*
        *libm_a-s_ceil*
        *_arm_*
        *heap_4*
        *heap_*
        *port*
        *queue*
        *list*
        *tasks*
        *timers*
        *croutine*
        *event_groups*
        *stream_buffer*
        *freertos_umm_malloc_host*
        *malloc_buffers*
        *clock_update*
        *system_si91x*
        *rsi_hal_mcu_m4_rom*
        *rsi_hal_mcu_m4_ram*
        *rsi_deepsleep_soc*
        *rsi_ps_ram_func*
        *rsi_egpio*
        *rsi_ipmu*
        *ipmu_apis*
        *rsi_pll*
        *rsi_power_save*
        *rsi_system_config*
        *rsi_time_period*
        *rsi_ulpss_clk*
        *rsi_debug*
        *rsi_usart*
        *rsi_rtc*
        *rsi_timers*
        *sl_si91x_calendar*
        *sl_si91x_ulp_timer*
        *sl_si91x_peripheral_gpio*
        *sl_si91x_driver_gpio*
        *sl_si91x_sdc*
        *aux_reference_volt_config*
        *rsi_adc*
        *rsi_crc*
        *rsi_dac*
        *rsi_gpdma*
        *rsi_i2c*
        *rsi_qspi*
        *rsi_spi*
        *rsi_udma*
        *rsi_udma_wrapper*
        *sensor_hub*
        *sensorhub*
        *sensorhub_app*
        *sensorhub_aws_app*
        *hub_hal_intf*
        *accelerometerSensor_hal*
        *adxl345*
        *adc_sensor_driver*
        *adc_sensor_hal*
        *apds9960*
        *apds9960Sensor_hal*
        *bh1750fvi*
        *lightsensor_hal*
        *lm75*
        *tempSensor_hal*
        *rsi_bod*
        *rsi_temp_sensor*
        *rsi_wwdt*
        *sl_si91x_adc*
        *sl_si91x_ssi*
        *rsi_deepsleep_soc*
        *power_manager_m4_wireless_example*
        *sl_si91x_button*
        *sl_si91x_button_instances*
        *app*
        *main*
        *ps2_state*
        *sl_log*
        *sl_iostream*
        *sl_si91x_usart*
        *sli_cmsis_os2_ext_task_register*
      ) .text* text_*)

    /* .ctors */
    *crtbegin*(.ctors)
    *crtbegin?*(.ctors)
    *(EXCLUDE_FILE(*crtend?* *crtend*) .ctors)
    *(SORT(.ctors.*))
    *(.ctors)

    /* .dtors */
    *crtbegin*(.dtors)
    *crtbegin?*(.dtors)
    *(EXCLUDE_FILE(*crtend?* *crtend*) .dtors)
    *(SORT(.dtors.*))
    *(.dtors)

    KEEP(*(.eh_frame*))
  } > rom :text

  .ARM.extab :
  {
    *(.ARM.extab* .gnu.linkonce.armextab.*)
  } > rom :text

  __exidx_start = .;
  .ARM.exidx :
  {
    *(.ARM.exidx* .gnu.linkonce.armexidx.*)
  } > rom :text
  __exidx_end = .;

  /* Copy/zero tables after exidx; LONG() forward-reference __etext, .data, .bss.
     Clang's Copy_Table()/Zero_Table() consume these rows; GCC (and Clang when
     the tables are empty) falls back to the _sidata/_sdata/_edata + __bss loops. */
  .copy.table :
  {
    . = ALIGN(4);
    __copy_table_start__ = .;
    LONG (__etext)
    LONG (__data_start__)
    LONG ((__data_end__ - __data_start__) / 4)
    __copy_table_end__ = .;
  } > rom :tables

  .zero.table :
  {
    . = ALIGN(4);
    __zero_table_start__ = .;
    LONG (__bss_start__)
    LONG ((__bss_end__ - __bss_start__) / 4)
    __zero_table_end__ = .;
  } > rom :tables

  .data :
  {
    __data_start__ = .;

    /* _sdata is used in startup code */
    _sdata = __data_start__;

    KEEP(*(.ramVector))
    *(.data*)
    *(.rodata*)
    KEEP(*(.init))
    KEEP(*(.fini))

    /* De-duplicated “pin-to-RAM” text list (kept first occurrence only) */
    *cmsis_gcc*(.text* text_*)
    *_arm_*(.text* text_*)
    *libc_a*(.text* text_*)
    *libm_a*(.text* text_*)
    *sbrk*(.text* text_*)
    *libm_a-s_ceil*(.text* text_*)
    *cmsis_os2*(.text* text_*)
    *port*(.text* text_*)
    *queue*(.text* text_*)
    *sl_rsi_utility*(.text* text_*)
    *tasks*(.text* text_*)
    *clock_update*(.text* text_*)
    *rsi_deepsleep_soc*(.text* text_*)
    *rsi_hal_mcu_m4_rom*(.text* text_*)
    *rsi_hal_mcu_m4_ram*(.text* text_*)
    *rsi_egpio*(.text* text_*)
    *rsi_ipmu*(.text* text_*)
    *ipmu_apis*(.text* text_*)
    *rsi_pll*(.text* text_*)
    *rsi_power_save*(.text* text_*)
    *rsi_ps_ram_func*(.text* text_*)
    *rsi_system_config*(.text* text_*)
    *rsi_time_period*(.text* text_*)
    *rsi_ulpss_clk*(.text* text_*)
    *sl_si91x_clock_manager*(.text* text_*)
    *sli_si91x_clock_manager*(.text* text_*)
    *system_si91x*(.text* text_*)
    *sl_slist*(.text* text_*)
    *strcmp*(.text* text_*)
    *sl_wifi*(.text* text_*)
    *sli_wifi_power_profile*(.text* text_*)
    *timers*(.text* text_*)
    *heap_4*(.text* text_*)
    *list*(.text* text_*)
    *sl_si91x_m4_ps*(.text* text_*)
    *sl_si91x_power_manager*(.text* text_*)
    *sli_si91x_power_manager*(.text* text_*)
    *sl_si91x_power_manager_handler*(.text* text_*)
    *sl_si91x_power_manager_debug*(.text* text_*)
    *sli_si91x_power_manager_wakeup_init*(.text* text_*)
    *sl_si91x_power_manager_wakeup_handler*(.text* text_*)
    *sl_sleeptimer*(.text* text_*)
    *sl_sleeptimer_hal_si91x_sysrtc*(.text* text_*)
    *rsi_sysrtc*(.text* text_*)
    *sl_si91x_low_power_tickless_mode*(.text* text_*)
    *sl_core_cortexm*(.text* text_*)
    *UDMA*(.text* text_*)
    *sl_platform_wireless*(.text* text_*)
    *stdio*(.text* text_*)
    *puts*(.text* text_*)
    *wbuf*(.text* text_*)
    *writer*(.text* text_*)
    *memchr*(.text* text_*)
    *memset*(.text* text_*)
    *memcpy*(.text* text_*)
    *nano-vfprintf*(.text* text_*)
    *nano-vfprintf_i*(.text* text_*)
    *printf*(.text* text_*)
    *lock*(.text* text_*)
    *fflush*(.text* text_*)
    *rsi_debug*(.text* text_*)
    *rsi_usart*(.text* text_*)
    *syscalls*(.text* text_*)
    *USART*(.text* text_*)
    *sl_si91x_calendar*(.text* text_*)
    *rsi_rtc*(.text* text_*)
    *sl_si91x_ulp_timer*(.text* text_*)
    *rsi_timers*(.text* text_*)
    *sl_si91x_peripheral_gpio*(.text* text_*)
    *sl_si91x_driver_gpio*(.text* text_*)
    *sl_si91x_sdc*(.text* text_*)
    *I2C*(.text* text_*)
    *SPI*(.text* text_*)
    *aux_reference_volt_config*(.text* text_*)
    *rsi_adc*(.text* text_*)
    *rsi_crc*(.text* text_*)
    *rsi_dac*(.text* text_*)
    *rsi_gpdma*(.text* text_*)
    *rsi_i2c*(.text* text_*)
    *rsi_qspi*(.text* text_*)
    *rsi_spi*(.text* text_*)
    *rsi_udma*(.text* text_*)
    *rsi_udma_wrapper*(.text* text_*)
    *sensor_hub*(.text* text_*)
    *rsi_bod*(.text* text_*)
    *rsi_temp_sensor*(.text* text_*)
    *rsi_wwdt*(.text* text_*)
    *sl_si91x_adc*(.text* text_*)
    *sl_si91x_ssi*(.text* text_*)
    *hub_hal_intf*(.text* text_*)
    *accelerometerSensor_hal*(.text* text_*)
    *adxl345*(.text* text_*)
    *adc_sensor_driver*(.text* text_*)
    *adc_sensor_hal*(.text* text_*)
    *apds9960*(.text* text_*)
    *apds9960Sensor_hal*(.text* text_*)
    *bh1750fvi*(.text* text_*)
    *lightsensor_hal*(.text* text_*)
    *lm75*(.text* text_*)
    *tempSensor_hal*(.text* text_*)
    *sl_main_process_action*(.text* text_*)
    *ps2_state*(.text* text_*)
    *app*(.text* text_*)
    *main*(.text* text_*)
    *sl_event_handler*(.text* text_*)
    *sl_si91x_driver*(.text* text_*)
    *sl_si91x_bus*(.text* text_*)
    *sli_wifi_command_engine*(.text* text_*)
    *sli_si91x_wifi_event_handler*(.text* text_*)
    *croutine*(.text* text_*)
    *event_groups*(.text* text_*)
    *stream_buffer*(.text* text_*)
    *freertos_umm_malloc_host*(.text* text_*)
    *malloc_buffers*(.text* text_*)
    *sensorhub_app*(.text* text_*)
    *sensorhub_aws_app*(.text* text_*)
    *snprintf*(.text* text_*)
    *sl_si91x_dma*(.text* text_*)

    /* Sleep/wake-critical files that are excluded from flash .text above but
       previously had no RAM home (they were landing in flash as orphan
       sections). Buttons are a wake source and their IRQ handlers must run
       with the QSPI flash powered down, so pin them (and the remaining
       excluded-only files) to RAM to keep the exclude/pin lists symmetric. */
    *sl_si91x_button*(.text* text_*)
    *sl_si91x_button_instances*(.text* text_*)
    *em_core*(.text* text_*)
    *sl_si91x_power_manager_wakeup_init*(.text* text_*)
    *sensorhub*(.text* text_*)
    *power_manager_m4_wireless_example*(.text* text_*)

    /* Platform-core logging stack (sl_log + iostream backends + UART). In PS2 the
       QSPI flash is powered down, so any logging/console path that runs during or
       across a sleep transition must execute from RAM. Mirrors the base SoC PS2
       linker's si91x_platform_core section. */
    *sl_log*(.text* text_*)
    *sl_iostream*(.text* text_*)
    *sl_si91x_usart*(.text* text_*)
    *sli_cmsis_os2_ext_task_register*(.text* text_*)

    /* ipmu calibration data */
    *(.common_ipmu_ram*)

    /* ipmu TCM calibration CODE (e.g. update_ipmu_calib_data): executes during
       power-save transitions while flash is off, so it must run from RAM. */
    *(.common_tcm_code*)

    . = ALIGN(4);
    /* preinit data */
    PROVIDE_HIDDEN(__preinit_array_start = .);
    KEEP(*(.preinit_array))
    PROVIDE_HIDDEN(__preinit_array_end = .);

    . = ALIGN(4);
    /* init data */
    PROVIDE_HIDDEN(__init_array_start = .);
    KEEP(*(SORT(.init_array.*)))
    KEEP(*(.init_array))
    PROVIDE_HIDDEN(__init_array_end = .);

    . = ALIGN(4);
    /* finit data */
    PROVIDE_HIDDEN(__fini_array_start = .);
    KEEP(*(SORT(.fini_array.*)))
    KEEP(*(.fini_array))
    PROVIDE_HIDDEN(__fini_array_end = .);

    KEEP(*(.jcr*))
    . = ALIGN(4);
    /* All data end */
    __data_end__ = .;

    /* _edata is used in startup code */
    _edata = __data_end__;
  } > ram AT> rom :ram_data

  /* __etext is the .data LMA (flash load address); _sidata mirrors it for the
     GCC startup copy loop. */
  __etext = LOADADDR(.data);
  _sidata = __etext;
  __rom_start  = ORIGIN(rom);
  __rom_length = LENGTH(rom);

  ASSERT((__etext % 4) == 0, "Data LMA must be word-aligned (Clang copy table)")
  ASSERT((__data_start__ % 4) == 0, "Data VMA must be word-aligned (Clang copy table)")
  ASSERT((__data_end__ % 4) == 0, "Data section must be word-multiple (Clang copy table)")

  /* Terminates the LMA chain in flash before the NOLOAD RAM sections so LLD
     does not carry a stale load address into .bss/.stack/.heap. */
  .empty :
  {
  } > rom :empty

  .bss (NOLOAD) :
  {
    . = ALIGN(4);
    __bss_start__ = .;
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    __bss_end__ = .;
  } > ram :ram_pre_code
  ASSERT(((__bss_end__ - __bss_start__) % 4) == 0, "BSS must be word-multiple (Clang zero table)")

  /* Modify the below value to change the stack size */
  StackSize = 0x3000;
  .stack (NOLOAD):
  {
    __StackLimit = .;
    KEEP(*(.stack*))
    . = ALIGN(4);
    . += StackSize;
    __StackTop = .;
    PROVIDE(__stack = __StackTop);
  } > ram :ram_pre_code

  /* RAM overflow detection */
  __current_ram_used = . - ORIGIN(ram);
  __ram_total = LENGTH(ram);
  __ram_remaining = __ram_total - __current_ram_used;

  /* Check for RAM overflow */
  ASSERT((. <= (ORIGIN(ram) + LENGTH(ram))),
         "RAM OVERFLOW: RAM usage exceeds available memory region")

  /* Check minimum heap space */
  ASSERT((__ram_remaining >= 1024),
         "WARNING: Less than 1KB RAM left for heap - application may run out of dynamic memory")

  /* Check stack overlap */
  ASSERT((__StackLimit >= __bss_end__),
         "CRITICAL: Stack overlaps with BSS section - increase RAM or reduce static allocation")

  /* End of RAM overflow detection section */
  /* Overflow guard symbols derived from MEMORY (no hard-coding) */
  __ram_start__ = ORIGIN(ram);
  __ram_end__   = ORIGIN(ram) + LENGTH(ram);

  .heap (NOLOAD):
  {
    __HeapBase = .;
    __end__ = .;
    end = __end__;
    _end = __end__;
    KEEP(*(.heap*))
    __HeapLimit = ORIGIN(ram) + LENGTH(ram);   /* end of RAM */
  } > ram :ram_post_data

  __heap_size = __HeapLimit - __HeapBase;

  /* Final heap validation */
  ASSERT((__heap_size >= 0),
         "CRITICAL: Heap region is invalid (negative size) - stack and heap are overlapping")

  .udma_addr0 :
  {
    *(.udma_addr0*)
  } > udma0 AT> rom :udma0_init

  .udma_addr1 :
  {
    *(.udma_addr1*)
  } > udma1 AT> rom :udma1_init

  /* nvm3 enable*/  /* Flash Present*/  /* littlefs_enable */  /* Flash Present */

  ASSERT(((__etext + SIZEOF(.data)) <= (ORIGIN(rom) + LENGTH(rom))),
         "ERROR: Flash overflow - code + initialized data exceed ROM region (firmware will not fit in flash).")
}
