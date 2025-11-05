# SL FILE SYSTEM FOR COMMON FLASH

## Table of Contents

- [SL FILE SYSTEM FOR COMMON FLASH](#sl-file-system-for-common-flash)
  - [Table of Contents](#table-of-contents)
  - [Purpose/Scope](#purposescope)
  - [Overview](#overview)
  - [About Example Code](#about-example-code)
  - [Prerequisites/Setup Requirements](#prerequisitessetup-requirements)
    - [Hardware Requirements](#hardware-requirements)
    - [Software Requirements](#software-requirements)
    - [Setup Diagram](#setup-diagram)
  - [Getting Started](#getting-started)
  - [Application Build Environment](#application-build-environment)
  - [Test the Application](#test-the-application)

## Purpose/Scope

This example demonstrates how to use the LittleFS file system on the Si91x SoC using common flash memory. The application tracks boot count by updating a "boot_count" file on every boot-up, showing persistent storage capabilities.

**Key Features:**
- File system operations using LittleFS on common flash
- Persistent data storage that survives power cycles
- Automatic file system formatting on first run
- Boot count tracking to demonstrate read/write operations

**boot_count File:** This file stores the boot count value in flash memory and persists across device resets.

## Overview

- This example interfaces with flash through QSPI interface using littlefs.
- The program can be interrupted at any time without losing track of how many times it has been booted and without corrupting the filesystem.

## About Example Code

- The example code in **file_system_example.c** shows how to set up QSPI to access flash memory for a file system using the LittleFS library.
- Initialize the wireless for LittleFS using `sl_net_init()`.
- To use the file system, call `lfs_mount()`.
- Open a file and read the current boot count  `lfs_file_read()`.
- Update the boot count `lfs_file_write()`.
- Close the file using `lfs_file_close()` and unmount the file system using `lfs_unmount()`.

## Prerequisites/Setup Requirements

### Hardware Requirements

- Windows PC
- Silicon Labs Si917 Evaluation Kit + External Flash

**Note:**
>- For detailed information about pinset configurations, refer to the Flash and PSRAM Combinations section in the [Software Reference Manual](https://github.com/SiliconLabs/wiseconnect/blob/release/v3.4.2/docs/software-reference/manuals/siwx91x-software-reference-manual.md).

### Software Requirements

- Simplicity Studio
- Serial console Setup
  - For Serial Console setup instructions, refer to [here](https://docs.silabs.com/wiseconnect/latest/wiseconnect-developers-guide-developing-for-silabs-hosts/#console-input-and-output).

### Setup Diagram

> ![Figure: Introduction](resources/readme/setupdiagram.png)

## Getting Started

Refer to the instructions [here](https://docs.silabs.com/wiseconnect/latest/wiseconnect-getting-started/) to:

- [Install Simplicity Studio](https://docs.silabs.com/wiseconnect/latest/wiseconnect-developers-guide-developing-for-silabs-hosts/#install-simplicity-studio)
- [Install WiSeConnect extension](https://docs.silabs.com/wiseconnect/latest/wiseconnect-developers-guide-developing-for-silabs-hosts/#install-the-wi-se-connect-extension)
- [Connect your device to the computer](https://docs.silabs.com/wiseconnect/latest/wiseconnect-developers-guide-developing-for-silabs-hosts/#connect-si-wx91x-to-computer)
- [Upgrade your connectivity firmware ](https://docs.silabs.com/wiseconnect/latest/wiseconnect-developers-guide-developing-for-silabs-hosts/#update-si-wx91x-connectivity-firmware)
- [Create a Studio project ](https://docs.silabs.com/wiseconnect/latest/wiseconnect-developers-guide-developing-for-silabs-hosts/#create-a-project)

## Application Build Environment

> **Note**: For recommended settings, please refer the [recommendations guide](https://docs.silabs.com/wiseconnect/latest/wiseconnect-developers-guide-prog-recommended-settings/).

## Test the Application

1. Run the application 
2. Observe the step-by-step file system operations on console output

    **Expected Console Output:**
 
   > ![Figure: Build run and Debug](resources/readme/file_system_output.png)

3. Reset the MCU, every reset will increase the boot count.

> **Note**:
>
>- When running the LittleFS example for the first time, after flash erase you may see the error **(error: Corrupted dir pair at {0x0, 0x1})** because flash is not yet formatted for LittleFS. The application will automatically format the flash and continue.
>- To reset the boot count, erase the chip.
>- Each step shows the progress of LittleFS operations for easier debugging and understanding.
