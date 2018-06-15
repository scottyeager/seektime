# Seektime
This tool try to determine if a target device is a HDD or SSD disk.

Actually, this tool just try to determine if the device behind a file (whatever it is)
is 'slow' or 'fast' doing seek and single sector read.

This test is **not** accurate. This tool output `SSD` or `HDD` based on the seektime,
but this is just an approximation. The nature of the device is absolutly not tested,
all of this is pure speculative and guessing.

# Purpose
It's quite easy, using Linux, to know if a disk is **rotational** using `/sys/block/<device>/queue/rotational`
endpoint, but sometime, when using like RAID Controller or any hardware which hide this, this value is fake.

This tool try, in a generic way, to probe if it's a `HDD` or `SSD` (by `SSD`, understand any non-slow-rotational device)
in case of difficult hardware. Since RAID Controller, etc. are all different, it's difficult to find a generic way.

# How does this work
This tool simply seek on the disk and compute the time it took.
Here is a computed average of seek-time based on rotational disk, in average:

| RPM   | Seek Time |
| ----- | --------- |
| 5400  | 6 ms      |
| 7200  | 5 ms      |
| 10000 | 3 ms      |
| 15000 | 2 ms      |

When using any non-rotational disks, seektime is nearly null since there are no mechanical
part which needs to move.

This tool use the following assumption: if computed seektime is lower than 0.5 ms, we assume it's an **SSD**.

In order to have more relevant value, the test does 128 seek-read, not only one. All of theses seek
are randomly spread over the disk, to try to avoid as much as possible hardware cache.

In addition, each time before doing any seek/read, we ask the kernel to purge any page cache related to this disk.

# Example

## Basic computer using one SSD and one HDD
```
~ # for dev in /dev/sd?; do ./seektime $dev; done
/dev/sda: SSD (103 us)
/dev/sdb: HDD (3336 us)
```

Here are the physical disks
- `/dev/sda: SanDisk SDSSDA240G`
- `/dev/sdb: WDC WD7500BPKT-00PK4T0`

## Dell Server using PERC RAID Controller
This controller can't do passthrough and gives fake values for rotational
and disk name, there are no default (kernel) way to know what's behind.
Some queries needs to be done using megaraid driver.

This server contains 12 HDD and 2 SSD:
```
~ # for dev in /dev/sd?; do ./seektime $dev; done
/dev/sda: HDD (1671 us)
[...]
/dev/sdj: HDD (2721 us)
/dev/sdk: HDD (3061 us)
/dev/sdl: HDD (2827 us)
/dev/sdm: SSD (37 us)
/dev/sdn: SSD (32 us)
```

Physical disks are:
- 12x `HGST HUS726060ALS640`
- 2x `Samsung SSD 860 EVO 2TB`
