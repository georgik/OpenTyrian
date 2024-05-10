#!/bin/bash

mkdir -p data/tyrian
cd data/tyrian
# https://github.com/opentyrian/opentyrian?tab=readme-ov-file
wget https://camanis.net/tyrian/tyrian21.zip
unzip tyrian21.zip
mv tyrian21 data
rm data/*.exe tyrain21.zip

