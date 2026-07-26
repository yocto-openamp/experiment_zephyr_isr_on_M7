#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <linux/rpmsg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

static int parse_u32_arg(const char *text, unsigned int *value)
{
    char *endptr = NULL;
    unsigned long parsed = strtoul(text, &endptr, 0);

    if (endptr == text || *endptr != '\0') {
        return -1;
    }

    *value = (unsigned int)parsed;
    return 0;
}

static int read_file_string(const char *path, char *buf, size_t buf_size)
{
    FILE *file = fopen(path, "r");
    if (!file) {
        return -1;
    }

    if (!fgets(buf, buf_size, file)) {
        fclose(file);
        return -1;
    }

    fclose(file);

    buf[strcspn(buf, "\r\n")] = '\0';
    return 0;
}

static int read_sysfs_u32(const char *path, unsigned int *value)
{
    char buf[32];
    char *endptr = NULL;
    unsigned long parsed;

    if (read_file_string(path, buf, sizeof(buf)) < 0) {
        return -1;
    }

    parsed = strtoul(buf, &endptr, 0);
    if (endptr == buf) {
        return -1;
    }

    *value = (unsigned int)parsed;
    return 0;
}

static int find_rpmsg_dst(const char *service_name, unsigned int *dst)
{
    const char *devices_dir = "/sys/bus/rpmsg/devices";
    DIR *dir;
    struct dirent *entry;
    char path[512];
    char name[64];
    unsigned int value;

    dir = opendir(devices_dir);
    if (!dir) {
        return -1;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') {
            continue;
        }

        snprintf(path, sizeof(path), "%s/%s/name", devices_dir, entry->d_name);
        if (read_file_string(path, name, sizeof(name)) < 0) {
            continue;
        }

        if (strcmp(name, service_name) != 0) {
            continue;
        }

        snprintf(path, sizeof(path), "%s/%s/dst", devices_dir, entry->d_name);
        if (read_sysfs_u32(path, &value) < 0) {
            continue;
        }

        *dst = value;
        closedir(dir);
        return 0;
    }

    closedir(dir);
    return -1;
}

int main(int argc, char **argv)
{
    const char *ctrl_dev = (argc > 1) ? argv[1] : "/dev/rpmsg_ctrl0";
    const char *rpmsg_dev = (argc > 2) ? argv[2] : "/dev/rpmsg0";
    const char *msg = (argc > 3) ? argv[3] : "hello from rpmsgclientsample\n";
    const char *service_name = (argc > 4) ? argv[4] : "demo";
    const char *remote_dst_arg = (argc > 5) ? argv[5] : "0x400";
    int ctrl_fd;
    int data_fd;
    ssize_t written;
    struct rpmsg_endpoint_info eptinfo;
    unsigned int remote_dst;

    ctrl_fd = open(ctrl_dev, O_RDWR | O_CLOEXEC);
    if (ctrl_fd < 0) {
        fprintf(stderr, "open(%s) failed: %s\n", ctrl_dev, strerror(errno));
        return 1;
    }

    memset(&eptinfo, 0, sizeof(eptinfo));
    snprintf(eptinfo.name, sizeof(eptinfo.name), "%s", service_name);
    eptinfo.src = RPMSG_ADDR_ANY;
    if (find_rpmsg_dst(service_name, &remote_dst) < 0) {
        if (parse_u32_arg(remote_dst_arg, &remote_dst) < 0) {
            fprintf(stderr, "could not find rpmsg destination for service %s\n", service_name);
            fprintf(stderr, "and invalid fallback destination: %s\n", remote_dst_arg);
            close(ctrl_fd);
            return 1;
        }

        fprintf(stderr,
            "warning: service %s not announced, using fallback dst 0x%x\n",
            service_name, remote_dst);
    }
    eptinfo.dst = remote_dst;

    if (ioctl(ctrl_fd, RPMSG_CREATE_EPT_IOCTL, &eptinfo) < 0) {
        if (errno != EEXIST) {
            fprintf(stderr, "ioctl(RPMSG_CREATE_EPT_IOCTL) failed: %s\n", strerror(errno));
            close(ctrl_fd);
            return 1;
        }
    }

    data_fd = open(rpmsg_dev, O_RDWR | O_NONBLOCK | O_CLOEXEC);
    if (data_fd < 0) {
        fprintf(stderr, "open(%s) failed: %s\n", rpmsg_dev, strerror(errno));
        close(ctrl_fd);
        return 1;
    }

    written = write(data_fd, msg, strlen(msg));
    if (written < 0) {
        fprintf(stderr, "write(%s) failed: %s\n", rpmsg_dev, strerror(errno));
        close(data_fd);
        close(ctrl_fd);
        return 1;
    }

    printf("sent %zd bytes to %s (created via %s)\n", written, rpmsg_dev, ctrl_dev);

    close(data_fd);
    close(ctrl_fd);
    return 0;
}
