#include "hardware_requirements.h"

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

// If the host has less than 2 cores chipeur does not run
static int check_cpu() {
  SYSTEM_INFO systemInfo;
  GetSystemInfo(&systemInfo);
  DWORD numberOfProcessors = systemInfo.dwNumberOfProcessors;
  if (numberOfProcessors < 2) {
#ifdef DEBUG
    fprintf(stderr, "Insufficient CPU cores detected !!\n");
#endif
    return EXIT_FAILURE;
  } else {
    return EXIT_SUCCESS;
  }
}

// If the host has less than 2GB of RAM chipeur does not run
static int check_ram() {
  MEMORYSTATUSEX memoryStatus;
  memoryStatus.dwLength = sizeof(memoryStatus);
  if (!GlobalMemoryStatusEx(&memoryStatus)) {
#ifdef DEBUG
    fprintf(stderr, "Failed to get RAM status !!\n");
#endif
    return EXIT_FAILURE;
  }

  if (memoryStatus.ullTotalPhys / 1024 / 1024 < 2048) {
#ifdef DEBUG
    fprintf(stderr, "Insufficient RAM detected !!\n");
#endif
    return EXIT_FAILURE;
  } else {
    return EXIT_SUCCESS;
  }
}

// If the host has less than 100GB of capacity (not availability) of HDD chipeur
// does not run
static int check_hdd() {
  HANDLE hDevice = CreateFileW(L"\\\\.\\PhysicalDrive0", 0,
                               FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                               OPEN_EXISTING, 0, NULL);
  if (hDevice == INVALID_HANDLE_VALUE) {
#ifdef DEBUG
    fprintf(stderr, "Failed to open physical drive !!\n");
#endif
    return EXIT_FAILURE;
  }

  DISK_GEOMETRY pDiskGeometry;
  DWORD bytesReturned;
  BOOL result = DeviceIoControl(hDevice, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0,
                                &pDiskGeometry, sizeof(pDiskGeometry),
                                &bytesReturned, NULL);
  if (!result) {
#ifdef DEBUG
    fprintf(stderr, "Failed to get disk geometry !!\n");
#endif
    CloseHandle(hDevice);
    return EXIT_FAILURE;
  }

  DWORD diskSizeGB;
  diskSizeGB =
      (DWORD)(pDiskGeometry.Cylinders.QuadPart *
              (ULONG)pDiskGeometry.TracksPerCylinder *
              (ULONG)pDiskGeometry.SectorsPerTrack *
              (ULONG)pDiskGeometry.BytesPerSector / 1024 / 1024 / 1024);
  if (diskSizeGB < 100) {
#ifdef DEBUG
    fprintf(stderr, "Insufficient HDD capacity detected !!\n");
#endif
    CloseHandle(hDevice);
    return EXIT_FAILURE;
  }

  CloseHandle(hDevice);
  return EXIT_SUCCESS;
}

BOOL CALLBACK check_monitor_resolution(HMONITOR hMonitor, HDC hdcMonitor,
                                       LPRECT lpRect, LPARAM data) {
  MONITORINFO monitorInfo;
  monitorInfo.cbSize = sizeof(MONITORINFO);
  GetMonitorInfoW(hMonitor, &monitorInfo);

  int xResolution = monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left;
  int yResolution = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;

  if ((xResolution != 1920 && xResolution != 2560 && xResolution != 1440) ||
      (yResolution != 1080 && yResolution != 1200 && yResolution != 1600 &&
       yResolution != 900)) {
#ifdef DEBUG
    fprintf(stderr, "Non-standard screen resolution detected !!\n");
#endif
    *((BOOL*)data) = TRUE;  // Set the flag if resolution is not standard
  }
  return EXIT_SUCCESS;  // Continue enumeration
}

static int check_resolution() {
  MONITORENUMPROC pMyCallback = (MONITORENUMPROC)check_monitor_resolution;
  int xResolution = GetSystemMetrics(SM_CXSCREEN);
  int yResolution = GetSystemMetrics(SM_CYSCREEN);

  // Exit if the primary monitor resolution is too small
  if (xResolution < 1000 && yResolution < 1000) {
    return EXIT_FAILURE;
  }

  int numberOfMonitors = GetSystemMetrics(SM_CMONITORS);
  BOOL sandbox = FALSE;

  // Enumerate monitors and check resolutions
  EnumDisplayMonitors(NULL, NULL, pMyCallback, (LPARAM)(&sandbox));

  // Exit if a non-standard resolution is detected
  if (sandbox) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

int check_hardware() {
  if (check_cpu() == EXIT_FAILURE) {
    return EXIT_CPU_FAIL;
  }
  if (check_ram() == EXIT_FAILURE) {
    return EXIT_RAM_FAIL;
  }
  if (check_hdd() == EXIT_FAILURE) {
    return EXIT_HDD_FAIL;
  }
  if (check_resolution() == EXIT_FAILURE) {
    return EXIT_RESOLUTION_FAIL;
  }

  return EXIT_SUCCESS;
}
