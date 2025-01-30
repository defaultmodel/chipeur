#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

// If the host has less than 2 cores chipeur does not run
static int check_cpu() {
  SYSTEM_INFO systemInfo;
  GetSystemInfo(&systemInfo);
  DWORD numberOfProcessors = systemInfo.dwNumberOfProcessors;
  if (numberOfProcessors < 2) {
    fprintf(stderr, "Insufficient CPU cores detected !!\n");
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
    fprintf(stderr, "Failed to get RAM status.\n");
    return EXIT_FAILURE;
  }

  if (memoryStatus.ullTotalPhys / 1024 / 1024 < 2048) {
    fprintf(stderr, "Insufficient RAM detected !!\n");
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
    fprintf(stderr, "Failed to open physical drive.\n");
    return EXIT_FAILURE;
  }

  DISK_GEOMETRY pDiskGeometry;
  DWORD bytesReturned;
  BOOL result = DeviceIoControl(hDevice, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0,
                                &pDiskGeometry, sizeof(pDiskGeometry),
                                &bytesReturned, NULL);
  if (!result) {
    fprintf(stderr, "Failed to get disk geometry.\n");
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
    fprintf(stderr, "Insufficient HDD capacity detected !!\n");
    CloseHandle(hDevice);
    return EXIT_FAILURE;
  }

  CloseHandle(hDevice);
  return EXIT_SUCCESS;
}

int check_hardware() {
  if (check_cpu() == EXIT_FAILURE) {
    fprintf(stderr, "CPU check failed\n");
    return EXIT_FAILURE;
  }
  if (check_ram() == EXIT_FAILURE) {
    fprintf(stderr, "RAM check failed\n");
    return EXIT_FAILURE;
  }
  if (check_hdd() == EXIT_FAILURE) {
    fprintf(stderr, "HDD check failed\n");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
