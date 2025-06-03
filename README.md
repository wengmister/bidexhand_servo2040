## README

This project now uses `servo2040` from pimoroni - remember to pull and update `pico-sdk` and `pimoroni-pico`

## PICO-SDK

    git clone -b master https://github.com/raspberrypi/pico-sdk.git
    cd pico-sdk
    git submodule update --init

## pimoroni-pico

    git clone https://github.com/pimoroni/pimoroni-pico.git
    cd pimoroni-pico
    git submodule update --init

## Build

    cd build
    cmake ..
    make -j4