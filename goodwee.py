import asyncio
import goodwe
import binascii

async def main():
    inverter = await goodwe.connect("192.168.1.241", family="ET")
    data = await inverter.read_runtime_data()
    print("Parsed Data:")
    for key, value in data.items():
        print(f"{key}: {value}")

    print("\nRAW HEX:")
    response = await inverter._read_from_socket(inverter._READ_RUNNING_DATA) 
    print("", repr(response))
 


asyncio.run(main())
