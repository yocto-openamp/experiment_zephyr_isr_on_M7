set -euox pipefail

export BINARY="build/verdin_imx8mp_m7/zephyr/zephyr.elf"

if [ ! -f "$BINARY" ]; then
	echo "Binary not found: $BINARY"
	exit 1
fi

if (( EUID != 0 )); then
    exec sudo "$0" "$@"
fi

time (
    rmmod --verbose --force rpmsg_ctrl || true
    rmmod --verbose --force rpmsg_char || true
    modprobe --verbose rpmsg_ctrl
    modprobe --verbose rpmsg_char

    mkdir -p /root/firmware
    echo /root/firmware > /sys/module/firmware_class/parameters/path
    cp "$BINARY" /root/firmware/zephyr.elf
    echo stop > /sys/class/remoteproc/remoteproc0/state
    echo zephyr.elf > /sys/class/remoteproc/remoteproc0/firmware
    echo start > /sys/class/remoteproc/remoteproc0/state
)
