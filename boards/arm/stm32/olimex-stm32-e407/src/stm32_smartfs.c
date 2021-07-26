/****************************************************************************
 * boards/arm/samd5e5/metro-m4/src/sam_smartfs.c
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

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <debug.h>

#include <arch/board/board.h>

#ifdef CONFIG_MTD
#include <nuttx/mtd/mtd.h>
#include <nuttx/mtd/configdata.h>
#include <nuttx/drivers/drivers.h>
#endif

#ifdef CONFIG_FS_SMARTFS
#include <nuttx/fs/smart.h>
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#ifdef CONFIG_FS_SMARTFS
  int ret;
  FAR struct mtd_dev_s *mtd;
  FAR struct mtd_geometry_s geo;
#endif

#if defined(CONFIG_MTD_PARTITION_NAMES)
const char *partname = "mnt";
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int stm32_littlefs_init()
{

#if 0 //def CONFIG_RAMMTD littlefs
  /* Create a RAM MTD device if configured */
  uint32_t ramsize = 8*1024;
  FAR uint8_t *ramstart;

  ramstart = (FAR uint8_t *)kmm_malloc(ramsize);
  if (ramstart == NULL)
    {
      syslog(LOG_ERR, "ERROR: Allocation for RAM MTD failed\n");
    }
  else
    {
      /* Initialized the RAM MTD */

      FAR struct mtd_dev_s *mtd = rammtd_initialize(ramstart, ramsize);
      if (mtd == NULL)
        {
          syslog(LOG_ERR, "ERROR: rammtd_initialize failed\n");
          kmm_free(ramstart);
        }
      else
        {
          /* Erase the RAM MTD */

          ret = mtd->ioctl(mtd, MTDIOC_BULKERASE, 0);
          if (ret < 0)
            {
              syslog(LOG_ERR, "ERROR: IOCTL MTDIOC_BULKERASE failed\n");
            }

#ifdef CONFIG_FS_LITTLEFS
          /* Register the MTD driver so that it can be accessed from the
           * VFS.
           */

          ret = register_mtddriver("/dev/rammtd", mtd, 0755, NULL);
          if (ret < 0)
            {
              syslog(LOG_ERR, "ERROR: Failed to register MTD driver: %d\n",
                     ret);
            }

          /* Mount the LittleFS file system */

          ret = nx_mount("/dev/rammtd", "/mnt/lfs", "littlefs", 0,
                         "forceformat");
          if (ret < 0)
            {
              syslog(LOG_ERR,
                     "ERROR: Failed to mount LittleFS at /mnt/lfs: %d\n",
                     ret);
            }
#endif
        }
    }
#endif
    return 0;
}

#ifdef CONFIG_FS_SMARTFS
int stm32_smartfs_initialize(void)
{
    //没有调通
    mtd = stm32f4xx_progmem_initialize(7);
    if (!mtd)
    {
        syslog(LOG_ERR, "ERROR: progmem_initialize failed\n");
        return -1;
    }

    ret = mtd->ioctl(mtd, MTDIOC_GEOMETRY, (unsigned long)((uintptr_t)&geo));
    if (ret < 0)
    {
        syslog(LOG_ERR, "ERROR: mtd->ioctl failed: %d\n", ret);
        return ret;
    }

    syslog(LOG_ERR, "Flash Geometry:\n");
    syslog(LOG_ERR, "  blocksize:      %lu\n", (unsigned long)geo.blocksize);
    syslog(LOG_ERR, "  erasesize:      %lu\n", (unsigned long)geo.erasesize);
    syslog(LOG_ERR, "  neraseblocks:   %lu\n", (unsigned long)geo.neraseblocks);


    // ret = smart_initialize(0, mtd, NULL);
    // syslog(LOG_ERR, "Error: smart_initialize ret %d\n", ret);

    // return ret;

#ifdef CONFIG_MTD_PARTITION
    {
        int partno;
        int partsize;
        int partoffset;
        int partszbytes;
        int erasesize;
        const char *partstring = "128";
        const char *ptr;
        FAR struct mtd_dev_s *mtd_part;
        char partref[4];

        /* Now create a partition on the FLASH device */

        partno = 0;
        ptr = partstring;
        partoffset = 7;

        /* Get the Flash erase size */

        erasesize = geo.erasesize;

        while (*ptr != '\0')
        {
            /* Get the partition size */

            partsize = atoi(ptr);
            partszbytes = (partsize << 10); /* partsize is defined in KB */
            printf("partsize %d partszbytes %d\n", partsize, partszbytes);

            /* Check if partition size is bigger then erase block */

            if (partszbytes < erasesize)
            {
                syslog(LOG_ERR,
                       "ERROR: Partition size is lesser than erasesize!\n");
                return -1;
            }

            /* Check if partition size is multiple of erase block */

            if ((partszbytes % erasesize) != 0)
            {
                syslog(LOG_ERR,
                       "ERROR: Partition size is not multiple of erasesize!\n");
                return -1;
            }

            mtd_part = mtd_partition(mtd, partoffset,
                                     partszbytes / erasesize);
            if (mtd_part == NULL)
            {
                syslog(LOG_ERR, "ERROR: mtd_partition return is NULL!\n");
                return -4;
            }
            partoffset += partszbytes / erasesize;

            /* Test if this is the config partition */

#if defined CONFIG_MTD_CONFIG
            if (partno == 0)
            {
                /* Register the partition as the config device */

                mtdconfig_register(mtd_part);
            }
            else
#endif
            {
                /* Now initialize a SMART Flash block device
                   * and bind it to the MTD device.
                   */

#if defined(CONFIG_MTD_SMART) && defined(CONFIG_FS_SMARTFS)
                // sprintf(partref, "p%d", partno);
                // smart_initialize(0, mtd_part, partref);
                smart_initialize(partno, mtd_part, NULL);
                
#endif
            }

            /* Set the partition name */

#ifdef CONFIG_MTD_PARTITION_NAMES
            if (!mtd_part)
            {
                syslog(LOG_ERR,
                       "Error: failed to create partition %s\n",
                       partname);
                return -1;
            }

            mtd_setpartitionname(mtd_part, partname);

            /* Now skip to next name.
                   * We don't need to split the string here
                   * because the MTD partition logic will only
                   * display names up to the comma,
                   * thus allowing us to use a single static name
                   * in the code.
                   */

            while (*partname != ',' && *partname != '\0')
            {
                /* Skip to next ',' */

                partname++;
            }

            if (*partname == ',')
            {
                partname++;
            }
#endif

            /* Update the pointer to point to the next size in the list */

            while ((*ptr >= '0') && (*ptr <= '9'))
            {
                ptr++;
            }

            if (*ptr == ',')
            {
                ptr++;
            }

            /* Increment the part number */

            partno++;
        }
    }
#else /* CONFIG_MTD_PARTITION */

    ret = smart_initialize(0, mtd_part, NULL);
    syslog(LOG_ERR, "Error: smart_initialize ret %d\n", ret);
#endif

    return ret;
}

#endif /* CONFIG_FS_SMARTFS */



    // #if defined(CONFIG_FS_SMARTFS)
    //   FAR struct mtd_dev_s *mtd;

    //   mtd = progmem_initialize();
    //   if (!mtd)
    //   {
    //     syslog(LOG_ERR, "ERROR: progmem_initialize failed\n");
    //     return -1;
    //   }

    //   /* Initialize to provide SMARTFS on the MTD interface */

    //   // MTD_IOCTL(mtd, MTDIOC_BULKERASE, 0);
    //   ret = smart_initialize(1, mtd, "smartfs");
    //   if (ret < 0)
    //   {
    //     ferr("ERROR: SmartFS initialization failed: %d\n", ret);
    //     return ret;
    //   }

    //   // mksmartfs("/dev/smart1", 1024);

    //   // ret = mount("/dev/smart1", "/mnt/smart", "smartfs", 0, NULL);
    //   // if (ret < 0)
    //   // {
    //   //   ferr("ERROR: Failed to mount the SMART volume: %d\n", errno);
    //   //   return ret;
    //   // }

    // #endif

    // ret = ftl_initialize(0, mtd);
    // if (ret < 0)
    // {
    //   ferr("ERROR: Initializing the FTL layer: %d\n", ret);
    //   return ret;
    // }

    // #if defined(CONFIG_FS_NXFFS)

    //   /* Initialize to provide NXFFS on the MTD interface */

    //   ret = nxffs_initialize(mtd);
    //   if (ret < 0)
    //   {
    //     syslog(LOG_ERR, "ERROR: NXFFS initialization failed: %d\n", ret);
    //     return ret;
    //   }

    //   ret = nx_mount(NULL, "/mnt/nxffs", "nxffs", 0, NULL);
    //   if (ret < 0)
    //   {
    //     syslog(LOG_ERR, "ERROR: Failed to mount the NXFFS volume: %d\n", ret);
    //     return ret;
    //   }
    // #endif

    // FAR struct mtd_dev_s *mtdpart_archinitialize(void)
    // {
    //     return progmem_initialize();
    // }



  // up_progmem_eraseblock(11);
  // stm32_flash_writeprotect(11, false);
  // ret = up_progmem_write(0x080E0000, "?111432543525327", 16);
  // stm32_flash_writeprotect(11, true);
  // syslog(LOG_ERR, "up_progmem_write :  %d\n", ret);

  // char buf[32] = {0};
  // ret = up_progmem_read(0x080E0000, buf, 16);
  // syslog(LOG_ERR, "up_progmem_read :  %d %s\n", ret, buf);

//-----

  // FAR struct mtd_dev_s *master = stm32f4xx_progmem_initialize(224);
  // if (!master)
  // {
  //   syslog(LOG_ERR, "ERROR: progmem_initialize failed\n");
  //   return -1;
  // }

  // ret = ftl_initialize(0, master);
  // if (ret < 0)
  // {
  //   printf("ERROR: ftl_initialize /dev/mtdblock0 failed: %d\n", ret);
  //   fflush(stdout);
  //   exit(2);
  // }

  // /* Now create a character device on the block device */

  // ret = bchdev_register("/dev/mtdblock0", "/dev/mtd0", false);
  // if (ret < 0)
  // {
  //   printf("ERROR: bchdev_register /dev/mtd0 failed: %d\n", ret);
  //   fflush(stdout);
  //   exit(3);
  // }