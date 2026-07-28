set -euox pipefail

export BINARY="build/verdin_imx8mp_m7/zephyr/zephyr.elf"
REMOTE_STATE="/sys/class/remoteproc/remoteproc0/state"
REMOTE_FW="/sys/class/remoteproc/remoteproc0/firmware"
FW_PATH="/sys/module/firmware_class/parameters/path"

wait_remote_state() {
    expected="$1"
    timeout_s="${2:-5}"
    deadline=$((SECONDS + timeout_s))

    while (( SECONDS < deadline )); do
        current="$(cat "$REMOTE_STATE")"
        if [[ "$current" == "$expected" ]]; then
            return 0
        fi
        echo "Retry for '$current'" >&2
    done

    echo "Timeout waiting for remoteproc state '$expected' (got '$current')" >&2
    return 1
}

if [ ! -f "$BINARY" ]; then
	echo "Binary not found: $BINARY"
	exit 1
fi

if (( EUID != 0 )); then
    exec sudo "$0" "$@"
fi

time (
    # rmmod --verbose --force rpmsg_ctrl || true
    # rmmod --verbose --force rpmsg_char || true
    # modprobe --verbose rpmsg_ctrl
    # modprobe --verbose rpmsg_char

    mkdir -p /root/firmware
    echo /root/firmware > "$FW_PATH"
    echo stop > "$REMOTE_STATE" || true
    wait_remote_state offline 10 || true
    cp "$BINARY" /root/firmware/zephyr.elf
    echo zephyr.elf > "$REMOTE_FW"
    echo start > "$REMOTE_STATE"
    wait_remote_state running 10
)
