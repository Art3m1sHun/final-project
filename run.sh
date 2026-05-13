#!/bin/bash

source /home/art3mis/anaconda3/etc/profile.d/conda.sh

conda activate Blood_Pressure

PORT=1234

cd /home/art3mis/Desktop/final_project

cleanup()
{
    echo ""

    echo "Stopping gateway..."

    kill -SIGINT $GATEWAY_PID

    wait $GATEWAY_PID

    echo "Stopping sensor..."

    kill $SENSOR_PID 2>/dev/null

    echo "Stopping plotter..."

    kill $PLOT_PID 2>/dev/null

    wait

    echo "Shutdown complete"
}

trap cleanup SIGINT SIGTERM

echo "Starting gateway..."

setsid ./sensor_app $PORT &

GATEWAY_PID=$!

sleep 2

echo "Starting plotter..."

setsid python plot_data.py &

PLOT_PID=$!

#--------------------------------
# WAIT GUI READY
#--------------------------------

sleep 5

echo "Starting sensor..."

setsid ./sensor 127.0.0.1 $PORT &

SENSOR_PID=$!

wait