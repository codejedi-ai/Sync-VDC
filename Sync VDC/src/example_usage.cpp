/**
 * @file example_usage.cpp
 * @brief Example usage of WT13106Connection class for Boogy Board
 * 
 * This file demonstrates how to use the WT13106Connection class
 * to communicate with the Boogy Board via Bluetooth or USB.
 */

#include "../include/WT13106Connection.h"
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>

int main(int argc, char* argv[])
{
    // Get connection string from command line or use default
    std::string connectionString;
    
    if (argc > 1) {
        connectionString = argv[1];
    } else {
        // Default: Bluetooth connection on COM5
        // Change this to match your setup:
        // - Bluetooth: "BT:COM5" (Windows) or "BT:/dev/ttyUSB0" (Linux)
        // - USB: "USB:1234:5678" (replace with your VID:PID)
        connectionString = "BT:COM5";
        std::cout << "Usage: " << argv[0] << " [connection_string]" << std::endl;
        std::cout << "Examples:" << std::endl;
        std::cout << "  " << argv[0] << " BT:COM5          (Bluetooth on Windows)" << std::endl;
        std::cout << "  " << argv[0] << " BT:/dev/ttyUSB0  (Bluetooth on Linux)" << std::endl;
        std::cout << "  " << argv[0] << " USB:1234:5678    (USB connection)" << std::endl;
        std::cout << std::endl;
        std::cout << "Using default: " << connectionString << std::endl;
    }
    
    // Create connection object
    WT13106Connection device(connectionString);
    
    // Connect to device
    std::cout << "Connecting to Boogy Board..." << std::endl;
    if (!device.connect()) {
        std::cerr << "Failed to connect: " << device.getLastError() << std::endl;
        std::cerr << std::endl;
        std::cerr << "Troubleshooting:" << std::endl;
        std::cerr << "  - For Bluetooth: Ensure device is paired and COM port is correct" << std::endl;
        std::cerr << "  - For USB: Check VID/PID and ensure device is connected" << std::endl;
        return 1;
    }
    
    std::cout << "Successfully connected to Boogy Board!" << std::endl;
    std::cout << "Reading input signals (press Ctrl+C to stop)..." << std::endl;
    std::cout << std::endl;
    
    // Continuous signal reading loop
    int sampleCount = 0;
    while (device.isConnected()) {
        // Read signals from the device
        std::vector<uint8_t> signalData = device.receiveResponse(1000); // 1 second timeout
        
        if (!signalData.empty()) {
            sampleCount++;
            std::cout << "Sample #" << sampleCount << " - Received " << signalData.size() << " bytes: ";
            
            // Print raw hex data
            for (size_t i = 0; i < signalData.size(); ++i) {
                printf("%02X ", signalData[i]);
            }
            std::cout << "| ";
            
            // Print as ASCII if printable
            for (uint8_t byte : signalData) {
                if (byte >= 32 && byte < 127) {
                    std::cout << static_cast<char>(byte);
                } else {
                    std::cout << ".";
                }
            }
            std::cout << std::endl;
            
            // Example: Parse signal data (adjust based on your protocol)
            if (signalData.size() >= 2) {
                // Assuming first byte is signal type, second is value
                uint8_t signalType = signalData[0];
                uint8_t signalValue = signalData[1];
                
                std::cout << "  -> Signal Type: " << static_cast<int>(signalType) 
                          << ", Value: " << static_cast<int>(signalValue) << std::endl;
            }
        } else {
            // No data received (timeout)
            // Uncomment to see timeout messages:
            // std::cout << "Waiting for data..." << std::endl;
        }
        
        // Small delay between reads to avoid CPU spinning
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Limit samples for demo (remove in production)
        if (sampleCount >= 100) {
            std::cout << std::endl << "Read 100 samples. Stopping..." << std::endl;
            break;
        }
    }
    
    // Example: Send a command to the device
    std::cout << std::endl << "Sending test command..." << std::endl;
    std::vector<uint8_t> command = {0x01, 0x02, 0x03, 0x04}; // Replace with actual command
    
    if (device.sendCommand(command)) {
        std::cout << "Command sent successfully" << std::endl;
        
        // Wait for response
        std::vector<uint8_t> response = device.receiveResponse(2000);
        if (!response.empty()) {
            std::cout << "Response received (" << response.size() << " bytes): ";
            for (uint8_t byte : response) {
                printf("%02X ", byte);
            }
            std::cout << std::endl;
        }
    } else {
        std::cerr << "Failed to send command: " << device.getLastError() << std::endl;
    }
    
    // Disconnect
    std::cout << std::endl << "Disconnecting..." << std::endl;
    device.disconnect();
    std::cout << "Disconnected successfully." << std::endl;
    
    return 0;
}

