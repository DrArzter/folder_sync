import json
import os
import socket
import struct
import time
from pathlib import Path
import sys


class ClientConfig:
    def __init__(self, server_ip, folder_path):
        self.server_ip = server_ip
        self.folder_path = folder_path

def read_config(config_file):
    try:
        with open(config_file, 'r') as file:
            data = json.load(file)
            return ClientConfig(data.get('serverIp', '127.0.0.1'), data.get('folderPath', './default_sync_folder'))
    except FileNotFoundError:
        # Provide default values if the file cannot be opened
        return ClientConfig('127.0.0.1', './default_sync_folder')

def start_client(config):
    # Create a socket
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    try:
        # Connect to the server
        server_address = (config.server_ip, 12345)  # Change to the same port as the server
        client_socket.connect(server_address)

        # Send the folder path to the server
        client_socket.send(config.folder_path.encode())

        # Send last modification times of files to the server
        for entry in Path(config.folder_path).rglob('*'):
            if entry.is_file():
                last_modified = int(entry.stat().st_mtime)
                client_socket.send(struct.pack('!Q', last_modified))

        # Receive the synchronization result
        response = client_socket.recv(1024).decode()
        print("Server Response:", response)

    except Exception as e:
        print("Error:", str(e))
    finally:
        # Close the client socket
        client_socket.close()

def main():
    if len(sys.argv) != 2:
        print("Usage: python", sys.argv[0], "<config_file>")
        sys.exit(1)

    config_file = sys.argv[1]
    config = read_config(config_file)

    while True:
        start_client(config)

        # Sleep for a second before the next synchronization
        time.sleep(1)

if __name__ == "__main__":
    main()
