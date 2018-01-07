# Citadel Test Harness #

This project houses the code needed to run tests from a host connected to the
citdadel chip.

## Requirements ##

  * Software
   * bazel
   * build-essential
   * gcc-arm-none-eabi
  * Hardware
   * FPGA test setup with ultradebug or Citadel test board

## Quickstart

The command to run the tests is:
> bazel run runtests


