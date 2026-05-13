#!/bin/bash

PORT=1234

echo "Starting gateway..."

./sensor_app $PORT &

GATEWAY_PID=$!

sleep 2

echo "Starting sensor reader..."

./sensor 127.0.0.1 $PORT &

SENSOR_PID=$!

wait $GATEWAY_PID