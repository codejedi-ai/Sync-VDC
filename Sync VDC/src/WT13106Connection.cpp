#include "../include/WT13106Connection.h"
#include <stdexcept>
#include <cstring>
#include <sstream>
#include <algorithm>

#ifdef _WIN32
#include <setupapi.h>
#include <initguid.h>
#pragma comment(lib, "setupapi.lib")
#else
// libusb is optional - only include if available
// Uncomment and install libusb-dev package for USB support on Linux
// #include <libusb-1.0/libusb.h>
#endif

WT13106Connection::WT13106Connection(const std::string& connectionString)
    : m_connectionString(connectionString)
    , m_connectionType(ConnectionType::BLUETOOTH)
    , m_isConnected(false)
    , m_lastError("")
    , m_vid(0)
    , m_pid(0)
    , m_baudRate(9600)  // Default baud rate, adjust based on device specs
{
#ifdef _WIN32
    m_serialHandle = INVALID_HANDLE_VALUE;
    m_usbHandle = INVALID_HANDLE_VALUE;
#else
    m_serialFd = -1;
    m_usbFd = -1;
#endif
}

WT13106Connection::~WT13106Connection()
{
    if (m_isConnected) {
        disconnect();
    }
}

bool WT13106Connection::connect()
{
    if (m_isConnected) {
        m_lastError = "Already connected";
        return false;
    }
    
    if (!parseConnectionString()) {
        return false;
    }
    
    bool success = false;
    if (m_connectionType == ConnectionType::BLUETOOTH) {
        success = initializeBluetooth();
    } else if (m_connectionType == ConnectionType::USB) {
        success = initializeUSB();
    }
    
    if (success) {
        m_isConnected = true;
        m_lastError = "";
    }
    
    return success;
}

bool WT13106Connection::disconnect()
{
    if (!m_isConnected) {
        m_lastError = "Not connected";
        return false;
    }
    
    cleanupConnection();
    m_isConnected = false;
    m_lastError = "";
    return true;
}

bool WT13106Connection::isConnected() const
{
    return m_isConnected;
}

bool WT13106Connection::sendCommand(const std::vector<uint8_t>& command)
{
    if (!m_isConnected) {
        m_lastError = "Not connected to device";
        return false;
    }
    
    if (command.empty()) {
        m_lastError = "Command is empty";
        return false;
    }
    
    if (m_connectionType == ConnectionType::BLUETOOTH) {
#ifdef _WIN32
        DWORD bytesWritten = 0;
        if (!WriteFile(m_serialHandle, command.data(), command.size(), &bytesWritten, NULL)) {
            m_lastError = "Failed to write to serial port";
            return false;
        }
        if (bytesWritten != command.size()) {
            m_lastError = "Partial write to serial port";
            return false;
        }
#else
        ssize_t bytesWritten = write(m_serialFd, command.data(), command.size());
        if (bytesWritten < 0) {
            m_lastError = "Failed to write to serial port";
            return false;
        }
        if (bytesWritten != static_cast<ssize_t>(command.size())) {
            m_lastError = "Partial write to serial port";
            return false;
        }
#endif
    } else if (m_connectionType == ConnectionType::USB) {
        // TODO: Implement USB send logic
        // For now, USB uses serial-like interface
        // In production, use libusb_bulk_transfer or similar
        m_lastError = "USB send not fully implemented";
        return false;
    }
    
    m_lastError = "";
    return true;
}

std::vector<uint8_t> WT13106Connection::receiveResponse(uint32_t timeoutMs)
{
    std::vector<uint8_t> response;
    
    if (!m_isConnected) {
        m_lastError = "Not connected to device";
        return response;
    }
    
    if (m_connectionType == ConnectionType::BLUETOOTH) {
#ifdef _WIN32
        // Set timeout
        COMMTIMEOUTS timeouts = {0};
        timeouts.ReadIntervalTimeout = timeoutMs;
        timeouts.ReadTotalTimeoutConstant = timeoutMs;
        timeouts.ReadTotalTimeoutMultiplier = 0;
        timeouts.WriteTotalTimeoutConstant = timeoutMs;
        timeouts.WriteTotalTimeoutMultiplier = 0;
        SetCommTimeouts(m_serialHandle, &timeouts);
        
        // Read available data
        uint8_t buffer[1024];
        DWORD bytesRead = 0;
        if (ReadFile(m_serialHandle, buffer, sizeof(buffer), &bytesRead, NULL)) {
            if (bytesRead > 0) {
                response.assign(buffer, buffer + bytesRead);
            }
        } else {
            DWORD error = GetLastError();
            if (error != ERROR_IO_PENDING) {
                m_lastError = "Failed to read from serial port";
            }
        }
#else
        // Set timeout
        struct termios tty;
        if (tcgetattr(m_serialFd, &tty) == 0) {
            tty.c_cc[VTIME] = timeoutMs / 100;  // Timeout in tenths of seconds
            tty.c_cc[VMIN] = 0;  // Non-blocking
            tcsetattr(m_serialFd, TCSANOW, &tty);
        }
        
        // Read available data
        uint8_t buffer[1024];
        ssize_t bytesRead = read(m_serialFd, buffer, sizeof(buffer));
        if (bytesRead > 0) {
            response.assign(buffer, buffer + bytesRead);
        } else if (bytesRead < 0) {
            m_lastError = "Failed to read from serial port";
        }
#endif
    } else if (m_connectionType == ConnectionType::USB) {
        // TODO: Implement USB receive logic
        m_lastError = "USB receive not fully implemented";
    }
    
    return response;
}

std::vector<uint8_t> WT13106Connection::sendCommandAndReceive(
    const std::vector<uint8_t>& command, 
    uint32_t timeoutMs)
{
    if (!sendCommand(command)) {
        return std::vector<uint8_t>();
    }
    
    return receiveResponse(timeoutMs);
}

std::string WT13106Connection::getLastError() const
{
    return m_lastError;
}

bool WT13106Connection::parseConnectionString()
{
    if (m_connectionString.empty()) {
        m_lastError = "Connection string is empty";
        return false;
    }
    
    // Check for Bluetooth connection (format: "BT:COM5" or "BT:/dev/ttyUSB0")
    if (m_connectionString.substr(0, 3) == "BT:") {
        m_connectionType = ConnectionType::BLUETOOTH;
        m_portName = m_connectionString.substr(3);
        if (m_portName.empty()) {
            m_lastError = "Invalid Bluetooth connection string format. Use 'BT:COM5' or 'BT:/dev/ttyUSB0'";
            return false;
        }
        return true;
    }
    
    // Check for USB connection (format: "USB:1234:5678")
    if (m_connectionString.substr(0, 4) == "USB:") {
        m_connectionType = ConnectionType::USB;
        std::string usbParams = m_connectionString.substr(4);
        size_t colonPos = usbParams.find(':');
        
        if (colonPos == std::string::npos) {
            m_lastError = "Invalid USB connection string format. Use 'USB:VID:PID' (e.g., 'USB:1234:5678')";
            return false;
        }
        
        try {
            m_vid = static_cast<uint16_t>(std::stoul(usbParams.substr(0, colonPos), nullptr, 16));
            m_pid = static_cast<uint16_t>(std::stoul(usbParams.substr(colonPos + 1), nullptr, 16));
        } catch (...) {
            m_lastError = "Invalid VID/PID format. Use hexadecimal (e.g., 'USB:1234:5678')";
            return false;
        }
        
        return true;
    }
    
    // Legacy support: if it starts with COM or /dev, assume Bluetooth
    if (m_connectionString.find("COM") == 0 || m_connectionString.find("/dev/") == 0) {
        m_connectionType = ConnectionType::BLUETOOTH;
        m_portName = m_connectionString;
        return true;
    }
    
    m_lastError = "Unknown connection string format. Use 'BT:COM5' for Bluetooth or 'USB:1234:5678' for USB";
    return false;
}

bool WT13106Connection::initializeBluetooth()
{
#ifdef _WIN32
    // Windows: Open COM port
    std::string portPath = "\\\\.\\" + m_portName;  // Use \\.\ prefix for COM ports > COM9
    
    m_serialHandle = CreateFileA(
        portPath.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );
    
    if (m_serialHandle == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        m_lastError = "Failed to open COM port: " + m_portName + " (Error: " + std::to_string(error) + ")";
        return false;
    }
    
    // Configure serial port settings
    DCB dcb = {0};
    dcb.DCBlength = sizeof(DCB);
    
    if (!GetCommState(m_serialHandle, &dcb)) {
        CloseHandle(m_serialHandle);
        m_serialHandle = INVALID_HANDLE_VALUE;
        m_lastError = "Failed to get COM port state";
        return false;
    }
    
    // Set serial port parameters
    dcb.BaudRate = m_baudRate;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    dcb.fDtrControl = DTR_CONTROL_ENABLE;
    dcb.fRtsControl = RTS_CONTROL_ENABLE;
    
    if (!SetCommState(m_serialHandle, &dcb)) {
        CloseHandle(m_serialHandle);
        m_serialHandle = INVALID_HANDLE_VALUE;
        m_lastError = "Failed to set COM port state";
        return false;
    }
    
    // Set timeouts
    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = 50;
    timeouts.WriteTotalTimeoutMultiplier = 10;
    SetCommTimeouts(m_serialHandle, &timeouts);
    
    return true;
#else
    // Linux/macOS: Open serial port
    m_serialFd = open(m_portName.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    
    if (m_serialFd < 0) {
        m_lastError = "Failed to open serial port: " + m_portName;
        return false;
    }
    
    // Configure serial port settings
    struct termios tty;
    if (tcgetattr(m_serialFd, &tty) != 0) {
        close(m_serialFd);
        m_serialFd = -1;
        m_lastError = "Failed to get serial port attributes";
        return false;
    }
    
    // Set baud rate
    cfsetospeed(&tty, B9600);  // Adjust based on device specs
    cfsetispeed(&tty, B9600);
    
    // 8N1 configuration
    tty.c_cflag &= ~PARENB;  // No parity
    tty.c_cflag &= ~CSTOPB;  // 1 stop bit
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;      // 8 data bits
    tty.c_cflag &= ~CRTSCTS; // No hardware flow control
    tty.c_cflag |= CREAD | CLOCAL; // Enable receiver, ignore modem control
    
    // Disable canonical mode
    tty.c_lflag &= ~ICANON;
    tty.c_lflag &= ~ECHO;
    tty.c_lflag &= ~ECHOE;
    tty.c_lflag &= ~ISIG;
    
    // Disable software flow control
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    
    // Raw output
    tty.c_oflag &= ~OPOST;
    
    // Set timeouts
    tty.c_cc[VTIME] = 1;  // 0.1 second timeout
    tty.c_cc[VMIN] = 0;   // Non-blocking
    
    if (tcsetattr(m_serialFd, TCSANOW, &tty) != 0) {
        close(m_serialFd);
        m_serialFd = -1;
        m_lastError = "Failed to set serial port attributes";
        return false;
    }
    
    return true;
#endif
}

bool WT13106Connection::initializeUSB()
{
    // USB initialization - placeholder implementation
    // In production, use libusb or Windows USB APIs
    
#ifdef _WIN32
    // Windows USB implementation using SetupAPI
    // This is a basic implementation - full USB support requires WinUSB or libusb
    
    GUID guid = {0};  // USB device GUID
    HDEVINFO deviceInfoSet = SetupDiGetClassDevs(&guid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    
    if (deviceInfoSet == INVALID_HANDLE_VALUE) {
        m_lastError = "Failed to enumerate USB devices";
        return false;
    }
    
    // Search for device with matching VID/PID
    SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
    deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
    
    bool found = false;
    for (DWORD i = 0; SetupDiEnumDeviceInterfaces(deviceInfoSet, NULL, &guid, i, &deviceInterfaceData); i++) {
        // Get device path and check VID/PID
        // Full implementation would parse device path and match VID/PID
    }
    
    SetupDiDestroyDeviceInfoList(deviceInfoSet);
    
    if (!found) {
        m_lastError = "USB device not found (VID: " + std::to_string(m_vid) + 
                      ", PID: " + std::to_string(m_pid) + ")";
        return false;
    }
    
    m_lastError = "USB connection requires full WinUSB or libusb implementation. "
                  "For now, use Bluetooth connection (BT:COMx)";
    return false;
#else
    // Linux USB implementation - requires libusb
    // Uncomment and install libusb-dev for USB support:
    /*
    libusb_context* ctx = nullptr;
    libusb_device_handle* handle = nullptr;
    
    int result = libusb_init(&ctx);
    if (result < 0) {
        m_lastError = "Failed to initialize libusb";
        return false;
    }
    
    handle = libusb_open_device_with_vid_pid(ctx, m_vid, m_pid);
    if (!handle) {
        libusb_exit(ctx);
        m_lastError = "USB device not found (VID: " + std::to_string(m_vid) + 
                      ", PID: " + std::to_string(m_pid) + ")";
        return false;
    }
    
    // Claim interface (assuming interface 0)
    if (libusb_claim_interface(handle, 0) < 0) {
        libusb_close(handle);
        libusb_exit(ctx);
        m_lastError = "Failed to claim USB interface";
        return false;
    }
    
    // Store handle (in production, store ctx and handle as member variables)
    libusb_release_interface(handle, 0);
    libusb_close(handle);
    libusb_exit(ctx);
    */
    
    m_lastError = "USB connection requires libusb. Install with: sudo apt-get install libusb-1.0-0-dev";
    return false;
#endif
}

void WT13106Connection::cleanupConnection()
{
    if (m_connectionType == ConnectionType::BLUETOOTH) {
#ifdef _WIN32
        if (m_serialHandle != INVALID_HANDLE_VALUE) {
            CloseHandle(m_serialHandle);
            m_serialHandle = INVALID_HANDLE_VALUE;
        }
#else
        if (m_serialFd >= 0) {
            close(m_serialFd);
            m_serialFd = -1;
        }
#endif
    } else if (m_connectionType == ConnectionType::USB) {
        // Cleanup USB resources
        // Implementation depends on USB library used
    }
}
