set -euox pipefail

if [ -z "${ZEPHYR_SDK_INSTALL_DIR:-}" ]; then
    echo "Please add to '~/.profile': export ZEPHYR_SDK_INSTALL_DIR=~/zephyr-sdk-1.0.1/"
    export ZEPHYR_SDK_INSTALL_DIR=~/zephyr-sdk-1.0.1/
fi

time (
    rm -rf .venv .west build bootloader modules zephyr

    uv venv --python 3.13.13
    . .venv/bin/activate

    uv pip install west

    west init .

    export WEST_NUCLEO="hal_stm32 cmsis_6"
    export WEST_IMX="hal_nxp open-amp libmetal"
    west update zephyr $WEST_NUCLEO $WEST_IMX

    uv pip install -r zephyr/scripts/requirements-base.txt

    west sdk install --gnu-toolchains arm-zephyr-eabi
)