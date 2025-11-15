#!/usr/bin/env python3
import logging

from pymodbus import pymodbus_apply_logging_config
from pymodbus.server import StartTcpServer
from pymodbus.transaction import ModbusSocketFramer
from pymodbus.datastore import (
    ModbusSequentialDataBlock,
    ModbusSlaveContext,
    ModbusServerContext,
)

# Basic logging
logging.basicConfig()
log = logging.getLogger(__file__)
log.setLevel("DEBUG")

# Enable pymodbus internal logging
pymodbus_apply_logging_config("DEBUG")


def run_server(ip: str = "0.0.0.0", port: int = 502):
    """
    Start a Modbus TCP server with:
      - 1 coil            at address 0
      - 1 holding register at address 0
      - unit / device_id  = 0x01
    """
    di = ModbusSequentialDataBlock(1, [0] * 100)   # discrete inputs
    co = ModbusSequentialDataBlock(1, [0] * 100)   # coils
    hr = ModbusSequentialDataBlock(0, [0] * 100)   # holding registers
    ir = ModbusSequentialDataBlock(1, [0] * 100)   # input registers

    # Slave context for unit 0x01
    slave = ModbusSlaveContext(di=di, co=co, hr=hr, ir=ir)

    # Map unit-id -> slave context
    server_context = ModbusServerContext(slaves={0x01: slave}, single=False)

    log.info("Starting Modbus TCP server on %s:%s (unit 1, coil 0, HR 0)", ip, port)

    # This call blocks and runs the server forever
    StartTcpServer(
        context=server_context,
        address=(ip, port),
#        framer=ModbusSocketFramer,  # standard Modbus TCP framing
    )


if __name__ == "__main__":
    run_server()

