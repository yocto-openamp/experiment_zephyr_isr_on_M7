
zephyr/samples/subsys/ipc/rpmsg_service/src/main.c
CONFIG_RPMSG_SERVICE=y: enables RPMsg service layer.
CONFIG_RPMSG_SERVICE_MODE_MASTER=y: selects RPMsg master mode.
CONFIG_OPENAMP_SLAVE=n: explicitly disables OpenAMP slave role.

zephyr/samples/subsys/ipc/rpmsg_service/remote/src/main.c   <======
CONFIG_RPMSG_SERVICE=y: same as above.
CONFIG_RPMSG_SERVICE_MODE_REMOTE=y: selects RPMsg remote mode.
CONFIG_OPENAMP_MASTER=n: explicitly disables OpenAMP master role.

Role selection flips
Master mode acts as RPMsg host side.
Remote mode acts as RPMsg remote/device side.

For your Linux remoteproc + Zephyr M7 case, remote mode (RPMSG_SERVICE_MODE_REMOTE=y) is typically the correct side
