#!/bin/bash -v

openssl req -x509 -nodes -days 36500 -newkey rsa:2048 -keyout selfsigned-private.key -out selfsigned-public.crt
