/****************************************************************************
 * apps/xled/xled_main.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/ioctl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include <nuttx/leds/userled.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define LED_DEV_PATH "/dev/userleds"

/* LED bit definitions for XIAO nRF52840 RGB LED */
/* Note: LED order follows board.h definitions:
 *   LED1 (bit 0) = P0.6  = Blue
 *   LED2 (bit 1) = P0.26 = Red
 *   LED3 (bit 2) = P0.30 = Green
 */

#define LED_BLUE_BIT   (1 << 0)  /* P0.6  - LED1 */
#define LED_RED_BIT    (1 << 1)  /* P0.26 - LED2 */
#define LED_GREEN_BIT  (1 << 2)  /* P0.30 - LED3 */

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void print_usage(FAR const char *progname)
{
  printf("Usage: %s <command> [args]\n", progname);
  printf("\n");
  printf("Commands:\n");
  printf("  on  <color>   Turn on LED(s) by color name\n");
  printf("  off <color>   Turn off LED(s) by color name\n");
  printf("  set <hex>     Set LED bitmask (hex, e.g. 0x07)\n");
  printf("  get           Get current LED state\n");
  printf("  blink <color> [interval_ms] [count]\n");
  printf("                Blink LED(s), default 500ms, 3 times\n");
  printf("  rgb <r> <g> <b>  Set individual LED states (0 or 1)\n");
  printf("\n");
  printf("Colors (can be combined with +):\n");
  printf("  red, green, blue, all\n");
  printf("\n");
  printf("Examples:\n");
  printf("  %s on red           Turn on red LED\n", progname);
  printf("  %s on red+green     Turn on red and green LEDs\n", progname);
  printf("  %s off all          Turn off all LEDs\n", progname);
  printf("  %s set 0x05         Set LED bitmask (red+blue)\n", progname);
  printf("  %s blink red        Blink red LED 3 times\n", progname);
  printf("  %s blink blue 200 5 Blink blue LED 5 times at 200ms\n",
         progname);
  printf("  %s rgb 1 0 1        Turn on red and blue, off green\n", progname);
}

static uint32_t parse_color(FAR const char *color)
{
  uint32_t ledset = 0;

  if (strcmp(color, "all") == 0)
    {
      ledset = LED_RED_BIT | LED_GREEN_BIT | LED_BLUE_BIT;
    }
  else
    {
      /* Support combined colors like "red+green" */

      if (strstr(color, "red") != NULL)
        {
          ledset |= LED_RED_BIT;
        }

      if (strstr(color, "green") != NULL)
        {
          ledset |= LED_GREEN_BIT;
        }

      if (strstr(color, "blue") != NULL)
        {
          ledset |= LED_BLUE_BIT;
        }
    }

  return ledset;
}

static int led_setall(int fd, uint32_t ledset)
{
  int ret = ioctl(fd, ULEDIOC_SETALL, ledset);
  if (ret < 0)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: ioctl(ULEDIOC_SETALL) failed: %d\n", errcode);
      return -errcode;
    }

  return OK;
}

static int led_getall(int fd, FAR uint32_t *ledset)
{
  int ret = ioctl(fd, ULEDIOC_GETALL, (unsigned long)((uintptr_t)ledset));
  if (ret < 0)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: ioctl(ULEDIOC_GETALL) failed: %d\n", errcode);
      return -errcode;
    }

  return OK;
}

static void print_led_state(uint32_t ledset)
{
  printf("LED state: 0x%02x [", ledset);
  printf("BLUE=%s ", (ledset & LED_BLUE_BIT) ? "ON" : "off");
  printf("RED=%s ", (ledset & LED_RED_BIT) ? "ON" : "off");
  printf("GREEN=%s", (ledset & LED_GREEN_BIT) ? "ON" : "off");
  printf("]\n");
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int fd;
  int ret;
  uint32_t ledset;
  uint32_t mask;

  if (argc < 2)
    {
      print_usage(argv[0]);
      return EXIT_FAILURE;
    }

  fd = open(LED_DEV_PATH, O_WRONLY);
  if (fd < 0)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: Failed to open %s: %d\n", LED_DEV_PATH,
              errcode);
      return EXIT_FAILURE;
    }

  /* Get current LED state first */

  ret = led_getall(fd, &ledset);
  if (ret < 0)
    {
      goto errout_with_fd;
    }

  if (strcmp(argv[1], "on") == 0)
    {
      if (argc < 3)
        {
          fprintf(stderr, "ERROR: 'on' requires a color argument\n");
          ret = -EINVAL;
          goto errout_with_fd;
        }

      mask = parse_color(argv[2]);
      if (mask == 0)
        {
          fprintf(stderr, "ERROR: Unknown color '%s'\n", argv[2]);
          ret = -EINVAL;
          goto errout_with_fd;
        }

      ledset |= mask;
      ret = led_setall(fd, ledset);
      if (ret == OK)
        {
          print_led_state(ledset);
        }
    }
  else if (strcmp(argv[1], "off") == 0)
    {
      if (argc < 3)
        {
          fprintf(stderr, "ERROR: 'off' requires a color argument\n");
          ret = -EINVAL;
          goto errout_with_fd;
        }

      mask = parse_color(argv[2]);
      if (mask == 0)
        {
          fprintf(stderr, "ERROR: Unknown color '%s'\n", argv[2]);
          ret = -EINVAL;
          goto errout_with_fd;
        }

      ledset &= ~mask;
      ret = led_setall(fd, ledset);
      if (ret == OK)
        {
          print_led_state(ledset);
        }
    }
  else if (strcmp(argv[1], "set") == 0)
    {
      if (argc < 3)
        {
          fprintf(stderr, "ERROR: 'set' requires a hex value argument\n");
          ret = -EINVAL;
          goto errout_with_fd;
        }

      ledset = (uint32_t)strtoul(argv[2], NULL, 0);
      ret = led_setall(fd, ledset);
      if (ret == OK)
        {
          print_led_state(ledset);
        }
    }
  else if (strcmp(argv[1], "get") == 0)
    {
      print_led_state(ledset);
      ret = OK;
    }
  else if (strcmp(argv[1], "blink") == 0)
    {
      if (argc < 3)
        {
          fprintf(stderr, "ERROR: 'blink' requires a color argument\n");
          ret = -EINVAL;
          goto errout_with_fd;
        }

      mask = parse_color(argv[2]);
      if (mask == 0)
        {
          fprintf(stderr, "ERROR: Unknown color '%s'\n", argv[2]);
          ret = -EINVAL;
          goto errout_with_fd;
        }

      int interval_ms = 500;
      int count = 3;

      if (argc >= 4)
        {
          interval_ms = atoi(argv[3]);
          if (interval_ms <= 0)
            {
              interval_ms = 500;
            }
        }

      if (argc >= 5)
        {
          count = atoi(argv[4]);
          if (count <= 0)
            {
              count = 3;
            }
        }

      uint32_t orig = ledset;
      int i;

      for (i = 0; i < count; i++)
        {
          ledset |= mask;
          led_setall(fd, ledset);
          usleep(interval_ms * 1000);

          ledset &= ~mask;
          led_setall(fd, ledset);
          usleep(interval_ms * 1000);
        }

      /* Restore original state */

      led_setall(fd, orig);
      ret = OK;
    }
  else if (strcmp(argv[1], "rgb") == 0)
    {
      if (argc < 5)
        {
          fprintf(stderr,
                  "ERROR: 'rgb' requires 3 arguments: <r> <g> <b> (0 or 1)\n");
          ret = -EINVAL;
          goto errout_with_fd;
        }

      ledset = 0;
      if (atoi(argv[2]))
        {
          ledset |= LED_RED_BIT;
        }

      if (atoi(argv[3]))
        {
          ledset |= LED_GREEN_BIT;
        }

      if (atoi(argv[4]))
        {
          ledset |= LED_BLUE_BIT;
        }

      ret = led_setall(fd, ledset);
      if (ret == OK)
        {
          print_led_state(ledset);
        }
    }
  else
    {
      fprintf(stderr, "ERROR: Unknown command '%s'\n", argv[1]);
      ret = -EINVAL;
    }

errout_with_fd:
  close(fd);
  return ret < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
