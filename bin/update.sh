#!/bin/bash

BINARY=./weatherstation

set -x

sudo mkdir -p /mnt/pico && \
  sudo mount /dev/disk/by-label/RPI-RP2 /mnt/pico && \
  sudo cp "${BINARY}.uf2" /mnt/pico/ && \
  sudo sync && \
  sudo umount /mnt/pico
