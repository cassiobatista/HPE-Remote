Coruja Remote
=============

Coruja Remote is a universal, multimodal remote control developed by FalaBrasil
Group at Federal University of Pará (UFPA), Brazil. Conventional, universal
remote control devices or apps on the smartphone, force the use of hands by the
user, which can be cumbersome or even impossible, specially for people with
special needs. Coruja Remote accepts both voice commands and head poses as input
commands, which turns itself a good -- and maybe the only one -- solution for
visually or motor impaired people.

HPE stands for Head Pose Estimation

Check the project page on Hackaday.IO:    
- https://hackaday.io/project/26830-tv-remote-control-based-on-head-gestures

## Instructions

On `desktop/original`, you may do the following to compile:      
```bash
mkdir build
cd build
cmake ..
make
```

And to execute:      
```bash
./hpe_remote
```


## To Install OpenCV Dependencies:
```bash
sudo apt-get install \
	build-essential cmake git python-dev python-numpy \
	libgtk2.0-dev pkg-config libavcodec-dev libavformat-dev libswscale-dev \
	libtbb2 libtbb-dev libjpeg-dev libpng-dev libtiff-dev libjasper-dev libdc1394-22-dev \
	gstreamer1.0-plugins-base libgstreamer1.0-0 libgstreamer1.0-dev libgstreamer-plugins-base1.0-* \
	libavresample-dev libavresample1
```
