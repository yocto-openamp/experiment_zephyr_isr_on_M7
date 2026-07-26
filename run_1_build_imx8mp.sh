set -euox pipefail

time (
    . .venv/bin/activate

    # rm -rf build
    west build -d build/verdin_imx8mp_m7 -b verdin_imx8mp/mimx8ml8/m7 zephyr_app -- -DCONF_FILE=prj_verdin_imx8mp.conf
)