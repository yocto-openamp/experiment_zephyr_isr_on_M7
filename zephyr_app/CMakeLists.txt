cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(experiment_zephyr_isr_on_m7)

target_sources(app PRIVATE src/main.c)
