#!/bin/bash

set -e

flask db upgrade
gunicorn --bind 0.0.0.0:5454 --threads 4 --workers 4 app:app
