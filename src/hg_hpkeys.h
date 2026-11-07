#ifndef HG_HPKEYS_H
#define HG_HPKEYS_H

#include <windows.h>

/* HP laptops' "Action Key Mode" BIOS setting, from the Set list's HP Fn Keys
 * row: action keys first (brightness, volume on the top row; F1-F12 with Fn)
 * or function keys first (F1-F12 on their own). The row is listed only when
 * the machine's manufacturer is HP. */

/* The row's command, in the running program: reads the setting, asks whether
 * to switch it, and starts an elevated copy of this exe to write it. */
void hg_hpkeys_command(HWND owner);

/* That elevated copy: `--hp-fnkeys action|function` on the command line. It
 * writes the setting, asks for the BIOS password when one is set, offers a
 * restart, and exits; it never starts a floater. Returns -1 when the command
 * line is not this, and the process exit code when it is. */
int hg_hpkeys_cli(LPCWSTR cmd_line);

#endif /* HG_HPKEYS_H */
