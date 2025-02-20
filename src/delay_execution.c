#include <windows.h>

// duration is in miliseconds
// sleep the program to evade sandbox time limit and check if uptime is
// consistent between the before and after sleep (to detect fast-forwards)
int delay_execution(int duration) {
  ULONGLONG uptimeBeforeSleep = GetTickCount64();
  int frequency = 0;  // 0 should not trigger any actual beeper

  // Calls NtDelayExecution under the hood, we could call it directly, but if it
  // is hooked we are screwed anyway
  // And Beep might ring less alarms than Sleep, since it is less common
  // Upside being that Beep is a lot easier to use
  Beep(frequency, duration);
  ULONGLONG uptimeAfterSleep = GetTickCount64();

  // detect fast-fowards
  if ((uptimeAfterSleep - uptimeBeforeSleep) < duration) {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
