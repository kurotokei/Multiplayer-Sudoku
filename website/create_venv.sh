#!/bin/bash

# stop at first error
set -e

python3 -m venv project_venv

source ./project_venv/bin/activate

python3 -m pip install 'django'
python3 -m pip install 'channels[daphne]'
python3 -m pip install 'channels_redis'
