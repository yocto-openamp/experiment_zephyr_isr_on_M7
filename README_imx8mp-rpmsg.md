https://github.com/zephyrproject-rtos/zephyr/blob/main/samples/subsys/ipc/openamp_rsc_table/README.rst

# TODO

You are very close. The remoteproc side is up, but the RPMsg service binding is not happening yet.

Most likely missing pieces:

1. No Linux Name Service binding path
- Your Zephyr app creates service endpoints named rpmsg-client-sample and rpmsg-tty at main.c and main.c.
- The Linux sample module only probes when a channel with name rpmsg-client-sample appears.
- If CONFIG_RPMSG_NS is disabled (or rpmsg_ns module not loaded), that channel may never be created on Linux, so modprobe looks silent.

2. Silent modprobe is normal by itself
- modprobe rpmsg-client-sample only loads the driver.
- You will only see module messages after channel probe, such as new channel and incoming msg.

3. Your local userspace helper defaults to the wrong service name for this app
- rpmsgclientsample.c defaults to service name demo, not rpmsg-client-sample.
- So run_9 is not aligned with your Zephyr service unless you pass arguments.

4. Your startup script does not load rpmsg core helper modules
- In run_2_start_imx8mp.sh to run_2_start_imx8mp.sh, rpmsg_ctrl and rpmsg_char modprobe lines are commented.
- Your README expects these modules for userspace endpoint flow at README_imx8mp.md and README_imx8mp.md.

Run these checks on the target Linux board:

    lsmod | grep -E 'rpmsg_client_sample|rpmsg_ns|virtio_rpmsg_bus|rpmsg_char|rpmsg_ctrl|rpmsg_tty'
    zcat config.gz | grep -E '^CONFIG_RPMSG_(NS|CHAR|CTRL|TTY)='
    ls /sys/bus/rpmsg/devices
    for d in /sys/bus/rpmsg/devices/*; do [ -f "$d/name" ] && echo "$d $(cat "$d/name")"; done
    dmesg -w | grep -E 'remoteproc|virtio|rpmsg'

Recommended order to test:

    modprobe rpmsg_ns || true
    modprobe rpmsg_ctrl || true
    modprobe rpmsg_char || true
    modprobe rpmsg-client-sample
    run_2_start_imx8mp.sh

If binding works, you should see rpmsg-client-sample channel/probe logs in dmesg, then on UART you should eventually see incoming msg logs from main.c.

For RPMSG_TTY specifically, enabled means:
- Kernel config has CONFIG_RPMSG_TTY=y or m
- Module loaded: rpmsg_tty
- Device appears: /dev/ttyRPMSG0 (or similar)
- Zephyr side endpoint exists at main.c

If you want, I can prepare a small patch to run_2_start_imx8mp.sh that auto-loads rpmsg_ns, rpmsg_ctrl, rpmsg_char, rpmsg_tty, and optionally rpmsg-client-sample in a correct startup sequence.
