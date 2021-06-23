#!/bin/bash

set -x

sudo mount /dev/disk/by-label/RPI-RP2 /mnt/pico && \
  sudo cp weatherstation.uf2 /mnt/pico/ && \
  sudo sync && \
  sudo umount /mnt/pico
