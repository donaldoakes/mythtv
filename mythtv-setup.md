# MythTV Setup

## Network attached storage
Hostname: mythnas
Role: Network-attached storage for MythTV content
Synology DS216se
DSM 7.1.1-42962 Update 9
Disk: 8 TB SSD

## Combined frontend and backend machine
Hostname: mythbe2
Role: Combined frontend and backend (frontend playback runs locally on this machine)
Motherboard: Intel DG33TL
CPU: Intel Core 2 Duo E8400 @ 3.00GHz
RAM: 4G DDR2
Swap: 4G DDR2
Disk: 250G SSD
Video: Nvidia GeForce GT 1030
Video Driver: akmod-nvidia-580xx/kmod-nvidia-580xx-7.2.6-200
Display Resolution: 1920x1080 @ 119.88Hz (set to match 1080p content and an even multiple of 59.94 fps; was previously 4k @ 60Hz, which caused playback judder)
Monitor: Sony Bravia 8 II K65XR80CM2
AV Receiver: Yamaha RX-V6A
OS: Fedora 44 7.2.6-200
Desktop Environment: Gnome 50.5 with Wayland
MythTV: Branch fixes/36 built from source
MariaDB: 11.8.8
All MythTV storage groups point to mythnas via NFS
XMLTV Grabber: tv_grab_zz_sdjson_sqlite
LIRC: 0.10.2
Remote: Sofabaton U3
Static IP: 192.168.0.69

## Frontend only machine
Hostname: mythfe
Role: Frontend only (connects to mythbe2 backend over the network)
Motherboard: Gigabyte H110M-A-CF
CPU: Intel Core i3-6100 CPU @ 3.70GHz
RAM: 8G DDR2
Swap: 8G DDR2
Disk: 250G SSD
Video: Nvidia GeForce GT 1030
Video Driver: akmod-nvidia-580xx/kmod-nvidia-580xx-7.1.13-200
Display Resolution: 1920x1080
Monitor: Sony Bravia 4k TV
AV Receiver: Onkyo TX-SR373
OS: Fedora 44 7.1.13-200
Desktop Environment: Gnome 50.4 with Wayland
MythTV: Branch fixes/36 built from source
LIRC: 0.10.2
Remote: Logitech Harmony

## Network
Wired ethernet with connections as depicted in this line diagram:
```
                         OFFICE
                           |
             +---------------------------+
             | CenturyLink C4000BZ       |
             | DSL Modem / Router        |
             +-------------+-------------+
                           |
                           |
             +-------------+-------------+
             | Netgear GS308              |
             | Office Switch              |
             +-------------+-------------+
                           |
                           |
                    to BEDROOM
                           |
             +-------------+-------------+
             | Netgear GS305v3            |
             | Bedroom Switch             |
             +-------------+-------------+
                           |
                           |
                  to LIVING ROOM
                           |
             +-------------+-------------+
             | Netgear GS305v3            |
             | Living Room Switch         |
             +-------------+-------------+
                    |               |
                    |               |
                    |               +---- mythfe
                    |
                    |
             +------+--------------------+
             | Netgear GS308              |
             | Basement Switch            |
             +-------------+-------------+
                           |
                           |
                         mythbe2

```

## cmake
env (~/.bashrc):
```
export MYTHTV_QT5_DIR=/home/donald/mythtv-install-qt5
export MYTHTV_QT6_DIR=/home/donald/mythtv-install-qt6
export MYTHTV_DIR=/home/donald/mythtv-install-qt6
```
Qt5:
```
cmake --preset qt5 -DCMAKE_INSTALL_PREFIX=$MYTHTV_QT5_DIR
cmake --build build-qt5
```
Qt6:
```
cmake --preset qt6 -DCMAKE_INSTALL_PREFIX=$MYTHTV_QT6_DIR
cmake --build build-qt6
```
incremental Qt5:
```
cd build-qt5/MythTV-prefix/src/MythTV-build
cmake --build .
```
incremental Qt6
```
cd build-qt6/MythTV-prefix/src/MythTV-build
cmake --build .
```

mythfrontend no logging:
```
mythfrontend -q -q
```
