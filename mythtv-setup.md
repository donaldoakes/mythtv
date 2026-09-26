# MythTV Setup

## Combined frontend and backend machine
Hostname: mythbe2
Motherboard: Intel DG33TL
CPU: Intel Core 2 Duo E8400 @ 3.00GHz
RAM: 4G DDR2
Swap: 4G DDR2
Disk: 250G SSD
Video: Nvidia GeForce GT 1030
Video Driver: akmod-nvidia-580xx/kmod-nvidia-580xx-7.2.6-200
Monitor: Sony Bravia 8 II K65XR80CM2
AV Receiver: Yahama RX-V6A
OS: Fedora 44 7.2.6-200
Desktop Environment: Gnome 50.5 with Wayland
MythTV: Branch fixes/36 built from source
MariaDB: 11.8.8
XMLTV Grabber: tv_grab_zz_sdjson_sqlite
LIRC: 0.10.2

## Frontend only machine
Hostname: mythfe
Motherboard: Gigabyte H110M-A-CF
CPU: Intel Core i3-6100 CPU @ 3.70GHz
RAM: 8G DDR2
Swap: 8G DDR2
Disk: 250G SSD
Video: Nvidia GeForce GT 1030
Video Driver: akmod-nvidia-580xx/kmod-nvidia-580xx-7.1.13-200
Monitor: Sony Bravia TV
AV Receiver: Onkyo TX-SR373
OS: Fedora 44 7.1.13-200
Desktop Environment: Gnome 50.4 with Wayland
MythTV: Branch fixes/36 built from source
LIRC: 0.10.2

## Network attached storage
Hostname: mythnas
Synology DS216se
DSM 7.1.1-42962 Update 9
Disk: 8 TB SSD

Network:
Wired ethernet with Netgear routers connected via Cat5 LAN cables 
All MythTV storage groups point to mythnas via NFS
