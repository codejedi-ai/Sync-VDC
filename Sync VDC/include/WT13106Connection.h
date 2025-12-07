#ifndef WT13106_CONNECTION_H
#define WT13106_CONNECTION_H

#include <vector>
#include <cstdint>
#include <string>

#ifdef _WIN32
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#endif

/**
 * @brief Connection type enumeration
 */
enum class ConnectionType {
    BLUETOOTH,  // Bluetooth via serial port (COM port)
    USB         // USB connection
};

/**
 * @brief Class for managing connection and communication with Boogy Board device
 * 
 * This class provides an interface for connecting to and communicating
 * with the Boogy Board via Bluetooth (serial port) or USB.
 * 
 * Connection string formats:
 * - Bluetooth: "BT:COM5" or "BT:/dev/ttyUSB0" (Windows/Linux)
 * - USB: "USB:1234:5678" (VID:PID format)
 */
class WT13106Connection {
public:
    /**
     * @brief Constructor
     * @param connectionString Connection string (e.g., "BT:COM5" for Bluetooth, "USB:1234:5678" for USB)
     */
    explicit WT13106Connection(const std::string& connectionString);
    
    /**
     * @brief Destructor - ensures proper disconnection
     */
    ~WT13106Connection();
    
    /**
     * @brief Establish connection to the device
     * @return true if connection successful, false otherwise
     */
    bool connect();
    
    /**
     * @brief Disconnect from the device
     * @return true if disconnection successful, false otherwise
     */
    bool disconnect();
    
    /**
     * @brief Check if device is currently connected
     * @return true if connected, false otherwise
     */
    bool isConnected() const;
    
    /**
     * @brief Send a command to the device
     * @param command Command data to send
     * @return true if command sent successfully, false otherwise
     */
    bool sendCommand(const std::vector<uint8_t>& command);
    
    /**
     * @brief Receive response from the device
     * @param timeoutMs Timeout in milliseconds (0 = no timeout)
     * @return Response data from device, empty vector on error/timeout
     */
    std::vector<uint8_t> receiveResponse(uint32_t timeoutMs = 1000);
    
    /**
     * @brief Send command and wait for response
     * @param command Command data to send
     * @param timeoutMs Timeout in milliseconds
     * @return Response data from device, empty vector on error/timeout
     */
    std::vector<uint8_t> sendCommandAndReceive(const std::vector<uint8_t>& command, 
                                                uint32_t timeoutMs = 1000);
    
    /**
     * @brief Get last error message
     * @return Error message string
     */
    std::string getLastError() const;

private:
    std::string m_connectionString;
    ConnectionType m_connectionType;
    bool m_isConnected;
    std::string m_lastError;
    
#ifdef _WIN32
    HANDLE m_serialHandle;      // For Bluetooth/Serial connections (Windows)
    HANDLE m_usbHandle;         // For USB connections (Windows) - placeholder
#else
    int m_serialFd;            // For Bluetooth/Serial connections (Linux/macOS)
    int m_usbFd;               // For USB connections (Linux/macOS) - placeholder
#endif
    
    // USB-specific members
    uint16_t m_vid;
    uint16_t m_pid;
    
    // Serial/Bluetooth-specific members
    std::string m_portName;
    uint32_t m_baudRate;
    
    /**
     * @brief Parse connection string and determine connection type
     * @return true if parsing successful
     */
    bool parseConnectionString();
    
    /**
     * @brief Initialize Bluetooth/Serial connection
     * @return true if initialization successful
     */
    bool initializeBluetooth();
    
    /**
     * @brief Initialize USB connection
     * @return true if initialization successful
     */
    bool initializeUSB();
    
    /**
     * @brief Clean up connection resources
     */
    void cleanupConnection();
};

#endif // WT13106_CONNECTION_H

