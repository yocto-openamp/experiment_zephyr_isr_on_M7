# Zephyr App (i.MX 8M Plus)

## References

- [Toradex Verdin i.MX 8M Plus](https://www.toradex.com/de/computer-on-modules/verdin-arm-family/nxp-imx-8m-plus)
- [Toradex Mallow carrier board](https://www.toradex.com/products/carrier-board/mallow-carrier-board)
- [torizon/meta-toradex-torizon](https://github.com/torizon/meta-toradex-torizon)

## Allow ssh to root without password

```bash
sudo su
cd ~/.ssh
cp /home/torizon/.ssh/authorized_keys .
```


## Build

From the repository root:

```sh
rm -r build
west build -d build/verdin_imx8mp_m7 -b verdin_imx8mp/mimx8ml8/m7 zephyr_app -- -DCONF_FILE=prj_verdin_imx8mp.conf
```

## uart4

```bash
tio /dev/ttyUSB0
```

## Deploy firmware and start

```bash
ssh root@verdin-imx8mp-08910183.local mkdir -p /root/firmware
ssh root@verdin-imx8mp-08910183.local 'echo /root/firmware > /sys/module/firmware_class/parameters/path'
scp build/verdin_imx8mp_m7/zephyr/zephyr.elf root@verdin-imx8mp-08910183.local:/root/firmware/zephyr.elf
ssh root@verdin-imx8mp-08910183.local 'echo stop > /sys/class/remoteproc/remoteproc0/state'
ssh root@verdin-imx8mp-08910183.local 'echo zephyr.elf > /sys/class/remoteproc/remoteproc0/firmware'
ssh root@verdin-imx8mp-08910183.local 'echo start > /sys/class/remoteproc/remoteproc0/state'
```

modprobe rpmsg_char
modprobe rpmsg_ctrl

[  239.923399] remoteproc remoteproc0: powering up imx-rproc
[  239.938662] remoteproc remoteproc0: Booting fw image zephyr.elf, size 1219436
[  239.938711] remoteproc remoteproc0: No resource table in elf
[  240.456137] remoteproc remoteproc0: remote processor imx-rproc is now up

