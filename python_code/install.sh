#!/usr/bin/env bash

uv venv --python 3.13.0 --clear
uv pip install -r requirements.txt

sudo ln -sf /home/alex/iot_code/homedoorlocks/python_code/home-door-lock-relay.service /etc/systemd/system/

sudo systemctl enable home-door-lock-relay.service
sudo systemctl start home-door-lock-relay.service
sudo systemctl daemon-reload