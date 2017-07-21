# linux-realsense-virtual-webcam
# Experimental virtual webcam for the Intel Realsense 3D camera

## To use this you'll need

### Hardware

 * Intel RealSense camera https://software.intel.com/en-us/realsense/

I'm presently using the Intel R200 camera on a Dell XPS13 running Ubuntu 16.10

### Software

 * v4l2loopback https://github.com/umlaeute/v4l2loopback
 * librealsense https://github.com/IntelRealSense/librealsense

### GStreamer

Use, e.g. "$ gst-inspect videoflip" shell command to see available options.


# Using as webcam with browsers

This creates a dummy camera that works with appear.in

gst-launch-1.0 videotestsrc ! tee ! v4l2sink device=/dev/video9


## Getting started

If you haven't already, build and install the librealsense examples and check
that they work correctly with your hardware.   If they don't then fix that
first, before trying this.


## pointcloudcam

To view output try
 * vlc v4l2:///dev/video9

## colorcam

A simple first test to create a virtual video device that copies the
RealSense colour camera.
