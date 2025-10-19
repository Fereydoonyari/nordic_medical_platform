#!/bin/bash

# One-liner Zephyr installation for NISC Medical Device
# Run this single command to install everything:

curl -fsSL https://raw.githubusercontent.com/zephyrproject-rtos/zephyr/main/scripts/requirements.txt | \
xargs -I {} bash -c 'pip3 install --user {}' && \
pip3 install --user west && \
mkdir -p ~/zephyr && cd ~/zephyr && \
west init -m https://github.com/zephyrproject-rtos/zephyr && \
west update && \
pip3 install --user -r zephyr/scripts/requirements.txt && \
wget https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.16.5/zephyr-sdk-0.16.5_linux-x86_64.tar.xz && \
tar xf zephyr-sdk-0.16.5_linux-x86_64.tar.xz && \
cd zephyr-sdk-0.16.5 && ./setup.sh -t arm-zephyr-eabi && \
echo 'export ZEPHYR_BASE="$HOME/zephyr/zephyr"' >> ~/.bashrc && \
echo 'export ZEPHYR_SDK_INSTALL_DIR="$HOME/zephyr/zephyr-sdk-0.16.5"' >> ~/.bashrc && \
echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.bashrc && \
echo "Installation complete! Run 'source ~/.bashrc' to activate."
