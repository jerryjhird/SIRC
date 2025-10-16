#!/bin/bash
set -e

if [ ! -f key.pem ]; then
    openssl ecparam -name secp384r1 -genkey -noout -out key.pem
fi

if [ ! -f cert.pem ]; then
    openssl req -new -x509 -key key.pem -out cert.pem -days 365 \
        -subj "/CN=localhost" -sha384
fi

meson setup build --prefix=$PWD/dist
meson compile -C build
meson install -C build

cp key.pem dist/bin
cp cert.pem dist/bin

mv dist/bin/* dist/
rmdir dist/bin