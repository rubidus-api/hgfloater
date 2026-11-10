#ifndef HG_RECDEV_H
#define HG_RECDEV_H

#include <windows.h>

/* The recorder itself (R1.10): its clock and its settings file, both reached
 * from BakREC's right-click menu. hg_recbak.c finds the drive; this file talks
 * to what is on it.
 *
 * The clock is the vendor SCSI command the recorder's own SetUDiskTime tool
 * sends (CDB 0xB0, seven bytes of local time). Windows passes it through only
 * for an administrator, so when the direct try is refused a second copy of the
 * program is started elevated with --rec-clock <letter>: it sets the clock,
 * exits with the result, and the elevation ends with it - the running
 * hgfloater never holds administrator rights. Sent only to a USB device with
 * the recorder's vendor id (0x10D6), never to one in firmware-update mode. */

/* Posted to the floater when the elevated copy has exited; w_param = its exit
 * code (0 = the clock is set). */
#define HG_MSG_RECCLOCK_DONE (WM_APP + 62)

/* Sets the clock of the recorder on `drive` to this computer's time. Success
 * flashes the floater; a failure says why in a message box. */
void hg_recdev_set_clock(HWND owner, WCHAR drive);
/* The floater's handler for HG_MSG_RECCLOCK_DONE. */
void hg_recdev_on_clock_done(HWND owner, DWORD code);

/* The settings box: the recorder's settings file (`Seo&0h.txt`) as fields,
 * Save and Cancel. */
void hg_recdev_settings(HWND owner, WCHAR drive);

/* The elevated half: when the command line has --rec-clock <letter>, sets that
 * recorder's clock and returns the exit code (0 = done); -1 otherwise. */
int hg_recdev_cli(LPCWSTR cmd_line);

#endif /* HG_RECDEV_H */
