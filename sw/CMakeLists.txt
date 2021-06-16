# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(another_world)

set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)

generate_inc_file_for_target(
    app
    src/bank01
    ${gen_dir}/bank01.inc
    )

generate_inc_file_for_target(
    app
    src/bank02
    ${gen_dir}/bank02.inc
    )
generate_inc_file_for_target(
    app
    src/bank03
    ${gen_dir}/bank03.inc
    )
generate_inc_file_for_target(
    app
    src/bank04
    ${gen_dir}/bank04.inc
    )
generate_inc_file_for_target(
    app
    src/bank05
    ${gen_dir}/bank05.inc
    )
generate_inc_file_for_target(
    app
    src/bank06
    ${gen_dir}/bank06.inc
    )
generate_inc_file_for_target(
    app
    src/bank07
    ${gen_dir}/bank07.inc
    )
generate_inc_file_for_target(
    app
    src/bank08
    ${gen_dir}/bank08.inc
    )
generate_inc_file_for_target(
    app
    src/bank09
    ${gen_dir}/bank09.inc
    )
generate_inc_file_for_target(
    app
    src/bank0a
    ${gen_dir}/bank0a.inc
    )
generate_inc_file_for_target(
    app
    src/bank0b
    ${gen_dir}/bank0b.inc
    )
generate_inc_file_for_target(
    app
    src/bank0c
    ${gen_dir}/bank0c.inc
    )
generate_inc_file_for_target(
    app
    src/bank0d
    ${gen_dir}/bank0d.inc
    )

generate_inc_file_for_target(
    app
    src/memlist.bin
    ${gen_dir}/memlist.bin.inc
    )

target_sources(app PRIVATE src/bank.c)
target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE src/resource.c)
target_sources(app PRIVATE src/sysrv.c)
target_sources(app PRIVATE src/video.c)
target_sources(app PRIVATE src/vm.c)
