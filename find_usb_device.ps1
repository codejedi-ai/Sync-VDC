# Find USB Device by VID/PID or Name
# Usage: .\find_usb_device.ps1 [-DeviceName "*"] [-VID "1234"] [-ProductID "5678"]
# Example: .\find_usb_device.ps1 -DeviceName "*Boogy*"

[CmdletBinding()]
param(
    [Parameter(Mandatory=$false)]
    [string]$DeviceName = "*",
    
    [Parameter(Mandatory=$false)]
    [string]$VID = "",
    
    [Parameter(Mandatory=$false)]
    [string]$ProductID = ""
)

# Ensure output is visible
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "USB Device Finder" -ForegroundColor Cyan  
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

try {
    Write-Host "[*] Searching for USB devices..." -ForegroundColor Yellow
    
    # Get all PnP devices
    $allDevices = @()
    try {
        $allDevices = Get-PnpDevice -ErrorAction Stop | Where-Object {
            $_.InstanceId -like "*VID_*"
        }
    } catch {
        Write-Host "[!] Error getting PnP devices: $($_.Exception.Message)" -ForegroundColor Red
        Write-Host "[*] Trying alternative method..." -ForegroundColor Yellow
        
        # Alternative method using WMI
        $allDevices = Get-WmiObject Win32_PnPEntity -ErrorAction SilentlyContinue | Where-Object {
            $_.DeviceID -like "*VID_*"
        } | ForEach-Object {
            [PSCustomObject]@{
                FriendlyName = $_.Name
                InstanceId = $_.DeviceID
                Status = if ($_.Status -eq "OK") { "OK" } else { "Error" }
            }
        }
    }
    
    if ($null -eq $allDevices -or $allDevices.Count -eq 0) {
        Write-Host "[!] No USB devices found with VID identifier." -ForegroundColor Yellow
        Write-Host ""
        Write-Host "[*] Listing all USB-related devices..." -ForegroundColor Yellow
        
        $usbDevices = Get-PnpDevice -ErrorAction SilentlyContinue | Where-Object {
            $_.Class -eq "USB" -or 
            $_.Class -eq "UniversalSerialBus" -or
            $_.FriendlyName -like "*USB*"
        } | Select-Object -First 10
        
        if ($usbDevices) {
            Write-Host "[+] Found $($usbDevices.Count) USB-related device(s):" -ForegroundColor Green
            $usbDevices | ForEach-Object {
                $vid = "N/A"
                $devicePid = "N/A"
                if ($_.InstanceId -match 'VID_([0-9A-F]{4})') {
                    $vid = $matches[1]
                }
                if ($_.InstanceId -match 'PID_([0-9A-F]{4})') {
                    $devicePid = $matches[1]
                }
                Write-Host "  - $($_.FriendlyName)" -ForegroundColor White
                Write-Host "    Status: $($_.Status), VID: $vid, PID: $devicePid" -ForegroundColor Gray
            }
        } else {
            Write-Host "[!] No USB devices found at all." -ForegroundColor Red
        }
        exit 0
    }
    
    Write-Host "[+] Found $($allDevices.Count) USB device(s) with VID identifier" -ForegroundColor Green
    Write-Host ""
    
    # Filter devices based on parameters
    $filteredDevices = $allDevices | Where-Object {
        $match = $true
        
        if ($DeviceName -ne "*") {
            $match = $match -and $_.FriendlyName -like $DeviceName
        }
        
        if ($VID) {
            $match = $match -and $_.InstanceId -like "*VID_$VID*"
        }
        
        if ($ProductID) {
            $match = $match -and $_.InstanceId -like "*PID_$ProductID*"
        }
        
        return $match
    }
    
    if ($filteredDevices -and $filteredDevices.Count -gt 0) {
        Write-Host "[+] Matching device(s):" -ForegroundColor Green
        Write-Host ""
        
        $deviceCount = 0
        foreach ($device in $filteredDevices) {
            $deviceCount++
            
            # Extract VID
            $vid = "N/A"
            if ($device.InstanceId -match 'VID_([0-9A-F]{4})') {
                $vid = $matches[1]
            }
            
            # Extract PID
            $devicePid = "N/A"
            if ($device.InstanceId -match 'PID_([0-9A-F]{4})') {
                $devicePid = $matches[1]
            }
            
            Write-Host "Device #$deviceCount" -ForegroundColor Cyan
            Write-Host "  Name: $($device.FriendlyName)" -ForegroundColor White
            Write-Host "  VID: $vid" -ForegroundColor White
            Write-Host "  PID: $devicePid" -ForegroundColor White
            
            $statusColor = if ($device.Status -eq "OK") { "Green" } else { "Yellow" }
            Write-Host "  Status: $($device.Status)" -ForegroundColor $statusColor
            
            if ($vid -ne "N/A" -and $devicePid -ne "N/A") {
                Write-Host "  Connection String: USB:${vid}:${devicePid}" -ForegroundColor Green
            } else {
                Write-Host "  Connection String: Unable to determine (missing VID/PID)" -ForegroundColor Yellow
            }
            
            Write-Host "  Instance ID: $($device.InstanceId)" -ForegroundColor Gray
            Write-Host ""
        }
        
        Write-Host "========================================" -ForegroundColor Cyan
        Write-Host "Total matching devices: $deviceCount" -ForegroundColor Cyan
        Write-Host "========================================" -ForegroundColor Cyan
        
    } else {
        Write-Host "[!] No matching USB devices found with the specified criteria." -ForegroundColor Yellow
        Write-Host ""
        Write-Host "Search criteria:" -ForegroundColor Cyan
        Write-Host "  Device Name: $DeviceName" -ForegroundColor White
        if ($VID) { Write-Host "  VID: $VID" -ForegroundColor White }
        if ($ProductID) { Write-Host "  PID: $ProductID" -ForegroundColor White }
        Write-Host ""
        Write-Host "[*] Showing first 10 USB devices for reference:" -ForegroundColor Cyan
        Write-Host ""
        
        $count = 0
        $allDevices | Select-Object -First 10 | ForEach-Object {
            $count++
            $vid = if ($_.InstanceId -match 'VID_([0-9A-F]{4})') { $matches[1] } else { "N/A" }
            $devicePid = if ($_.InstanceId -match 'PID_([0-9A-F]{4})') { $matches[1] } else { "N/A" }
            
            Write-Host "$count. $($_.FriendlyName)" -ForegroundColor White
            Write-Host "   VID: $vid, PID: $devicePid, Status: $($_.Status)" -ForegroundColor Gray
        }
        
        if ($allDevices.Count -gt 10) {
            Write-Host ""
            Write-Host "... and $($allDevices.Count - 10) more device(s)" -ForegroundColor Gray
        }
    }
    
} catch {
    Write-Host ""
    Write-Host "[!] Error occurred: $($_.Exception.Message)" -ForegroundColor Red
    if ($_.ScriptStackTrace) {
        Write-Host "[!] Stack trace: $($_.ScriptStackTrace)" -ForegroundColor Red
    }
    exit 1
}

Write-Host ""
