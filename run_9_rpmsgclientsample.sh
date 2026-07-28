set -euox pipefail

cd rpmsgclientsample
/usr/bin/aarch64-tdx-linux-gcc -Wall -Wextra rpmsgclientsample.c -o rpmsgclientsample

sudo rpmsgclientsample
