//
// To view output try
// vlc v4l2:///dev/video9


#include <librealsense/rs.h>
#include <linux/videodev2.h>
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>

/* Additionall,y include the librealsense utilities header file */
#include <librealsense/rsutil.h>

/* Also include GLFW to allow for graphical display */
#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>


double yaw, pitch, lastX, lastY; int ml;
static void on_mouse_button(GLFWwindow * win, int button, int action, int mods)
{
    if(button == GLFW_MOUSE_BUTTON_LEFT) ml = action == GLFW_PRESS;
}
static double clamp(double val, double lo, double hi) { return val < lo ? lo : val > hi ? hi : val; }
static void on_cursor_pos(GLFWwindow * win, double x, double y)
{
    if(ml)
    {
        yaw = clamp(yaw - (x - lastX), -120, 120);
        pitch = clamp(pitch + (y - lastY), -80, 80);
    }
    lastX = x;
    lastY = y;
}

/* Function calls to librealsense may raise errors of type rs_error */
rs_error * e = 0;
void check_error()
{
    if(e)
    {
        printf("rs_error was raised when calling %s(%s):\n", rs_get_failed_function(e), rs_get_failed_args(e));
        printf("    %s\n", rs_get_error_message(e));
        exit(EXIT_FAILURE);
    }
}

int want = 1;

static void prepare_buffer(GstAppSrc* appsrc, char *pixels) {
  static GstClockTime timestamp = 0;
  GstBuffer *buffer;
  guint size;
  GstFlowReturn ret;

  if (!want) return;
  want = 0;

  size = 640 * 480 * 8;

  buffer = gst_buffer_new_wrapped_full( 0, (gpointer)pixels, size, 0, size, NULL, NULL );

  //GST_BUFFER_PTS (buffer) = timestamp;
  //GST_BUFFER_DURATION (buffer) = gst_util_uint64_scale_int (1, GST_SECOND, 4);

  timestamp += GST_BUFFER_DURATION (buffer);

  ret = gst_app_src_push_buffer(appsrc, buffer);

  if (ret != GST_FLOW_OK) {
    /* something wrong, stop pushing */
    // g_main_loop_quit (loop);
  }
}

char *pixel_data;

static void cb_need_data (GstElement *appsrc, guint unused_size, gpointer user_data) {
  prepare_buffer((GstAppSrc*)appsrc, pixel_data);
  want = 1;
}

gint main(gint argc, gchar**argv)
{
  GstElement *pipeline, *appsrc, *conv, *flip, *tee, *videosink;

  /* init GStreamer */
  gst_init (&argc, &argv);

  /* setup pipeline */
  pipeline = gst_pipeline_new ("pipeline");
  appsrc = gst_element_factory_make ("appsrc", "source");
  conv = gst_element_factory_make ("videoconvert", "conv");
  flip = gst_element_factory_make ("videoflip", "flip");

  // videoconvert ! "video/x-raw,format=YUY2" ! v4l2sink device=/dev/video1


  tee = gst_element_factory_make ("tee", "tee");
  //videosink = gst_element_factory_make ("v4l2sink", "videosink");
  videosink = gst_element_factory_make ("xvimagesink", "videosink");

  /* setup */
  #if 1
  g_object_set (G_OBJECT (appsrc), "caps",
  		gst_caps_new_simple ("video/x-raw",
				     "format", G_TYPE_STRING, "RGB16",
				     "width", G_TYPE_INT, 640,
				     "height", G_TYPE_INT, 480,
				     "framerate", GST_TYPE_FRACTION, 30, 1,
				     NULL), NULL);
#else
g_object_set (G_OBJECT (appsrc), "caps",
  gst_caps_new_simple ("video/x-raw-rgb",
              "width", G_TYPE_INT, 640,
              "height", G_TYPE_INT, 480,
              "bpp", G_TYPE_INT, 24,
              "depth", G_TYPE_INT, 24,
              "red_mask",   G_TYPE_INT, 0x00ff0000,
              "green_mask", G_TYPE_INT, 0x0000ff00,
              "blue_mask",  G_TYPE_INT, 0x000000ff,
              "framerate", GST_TYPE_FRACTION, 30, 1,
              "endianness", G_TYPE_INT, G_BIG_ENDIAN,
              NULL), NULL);

// NB set glReadPixels to match.  See https://www.khronos.org/registry/OpenGL-Refpages/es2.0/xhtml/glReadPixels.xml
#endif

  g_object_set (G_OBJECT (conv), "video/x-raw,format", "YUY2", NULL);

  g_object_set (G_OBJECT (flip), "method", "vertical-flip", NULL);

  g_object_set (G_OBJECT (videosink), "device", "/dev/video9", NULL);

  gst_bin_add_many (GST_BIN (pipeline), appsrc, conv, flip, tee, videosink, NULL);
  gst_element_link_many (appsrc, conv, flip, tee, videosink, NULL);

  /* setup appsrc */
  g_object_set (G_OBJECT (appsrc),
		"stream-type", 0, // GST_APP_STREAM_TYPE_STREAM
		"format", GST_FORMAT_TIME,
    "is-live", TRUE,
    NULL);
  g_signal_connect (appsrc, "need-data", G_CALLBACK (cb_need_data), NULL);

  /* play */
  gst_element_set_state (pipeline, GST_STATE_PLAYING);

  pixel_data = malloc(8*640*480);

  // Now the RealSense code.

    /* Turn on logging. We can separately enable logging to console or to file, and use different severity filters for each. */
    rs_log_to_console(RS_LOG_SEVERITY_WARN, &e);
    check_error();
    /*rs_log_to_file(RS_LOG_SEVERITY_DEBUG, "librealsense.log", &e);
    check_error();*/

    /* Create a context object. This object owns the handles to all connected realsense devices. */
    rs_context * ctx = rs_create_context(RS_API_VERSION, &e);
    check_error();
    printf("There are %d connected RealSense devices.\n", rs_get_device_count(ctx, &e));
    check_error();
    if(rs_get_device_count(ctx, &e) == 0) return EXIT_FAILURE;

    /* This tutorial will access only a single device, but it is trivial to extend to multiple devices */
    rs_device * dev = rs_get_device(ctx, 0, &e);
    check_error();
    printf("\nUsing device 0, an %s\n", rs_get_device_name(dev, &e));
    check_error();
    printf("    Serial number: %s\n", rs_get_device_serial(dev, &e));
    check_error();
    printf("    Firmware version: %s\n", rs_get_device_firmware_version(dev, &e));
    check_error();

    /* Configure depth and color to run with the device's preferred settings */
    rs_enable_stream_preset(dev, RS_STREAM_DEPTH, RS_PRESET_BEST_QUALITY, &e);
    check_error();
    rs_enable_stream_preset(dev, RS_STREAM_COLOR, RS_PRESET_BEST_QUALITY, &e);
    check_error();
    rs_start_device(dev, &e);
    check_error();

    /* Open a GLFW window to display our output */
    glfwInit();
    GLFWwindow * win = glfwCreateWindow(640, 480, "librealsense tutorial #3", NULL, NULL);
    glfwSetCursorPosCallback(win, on_cursor_pos);
    glfwSetMouseButtonCallback(win, on_mouse_button);
    glfwMakeContextCurrent(win);
    while(!glfwWindowShouldClose(win))
    {
        /* Wait for new frame data */
        glfwPollEvents();
        rs_wait_for_frames(dev, &e);
        check_error();

        /* Retrieve our images */
        const uint16_t * depth_image = (const uint16_t *)rs_get_frame_data(dev, RS_STREAM_DEPTH, &e);
        check_error();
        const uint8_t * color_image = (const uint8_t *)rs_get_frame_data(dev, RS_STREAM_COLOR, &e);
        check_error();

        /* Retrieve camera parameters for mapping between depth and color */
        rs_intrinsics depth_intrin, color_intrin;
        rs_extrinsics depth_to_color;
        rs_get_stream_intrinsics(dev, RS_STREAM_DEPTH, &depth_intrin, &e);
        check_error();
        rs_get_device_extrinsics(dev, RS_STREAM_DEPTH, RS_STREAM_COLOR, &depth_to_color, &e);
        check_error();
        rs_get_stream_intrinsics(dev, RS_STREAM_COLOR, &color_intrin, &e);
        check_error();
        float scale = rs_get_device_depth_scale(dev, &e);
        check_error();

        /* Set up a perspective transform in a space that we can rotate by clicking and dragging the mouse */
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluPerspective(60, (float)1280/960, 0.01f, 20.0f);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        gluLookAt(0,0,0, 0,0,1, 0,-1,0);
        glTranslatef(0,0,+0.5f);
        glRotated(pitch, 1, 0, 0);
        glRotated(yaw, 0, 1, 0);
        glTranslatef(0,0,-0.5f);

        //glScalef(1,-1,1);

        /* We will render our depth data as a set of points in 3D space */
        glPointSize(2);
        glEnable(GL_DEPTH_TEST);
        glBegin(GL_POINTS);

        int dx, dy;
        for(dy=0; dy<depth_intrin.height; ++dy)
        {
            for(dx=0; dx<depth_intrin.width; ++dx)
            {
                /* Retrieve the 16-bit depth value and map it into a depth in meters */
                uint16_t depth_value = depth_image[dy * depth_intrin.width + dx];
                float depth_in_meters = depth_value * scale;

                /* Skip over pixels with a depth value of zero, which is used to indicate no data */
                if(depth_value == 0) continue;

                /* Map from pixel coordinates in the depth image to pixel coordinates in the color image */
                float depth_pixel[2] = {(float)dx, (float)dy};
                float depth_point[3], color_point[3], color_pixel[2];
                rs_deproject_pixel_to_point(depth_point, &depth_intrin, depth_pixel, depth_in_meters);
                rs_transform_point_to_point(color_point, &depth_to_color, depth_point);
                rs_project_point_to_pixel(color_pixel, &color_intrin, color_point);

                /* Use the color from the nearest color pixel, or pure white if this point falls outside the color image */
                const int cx = (int)roundf(color_pixel[0]), cy = (int)roundf(color_pixel[1]);
                if(cx < 0 || cy < 0 || cx >= color_intrin.width || cy >= color_intrin.height)
                {
                    glColor3ub(255, 255, 255);
                }
                else
                {
                    glColor3ubv(color_image + (cy * color_intrin.width + cx) * 3);
                }

                /* Emit a vertex at the 3D location of this depth pixel */
                glVertex3f(depth_point[0], depth_point[1], depth_point[2]);
            }
        }
        glEnd();
        glReadBuffer(GL_FRONT);
        glReadPixels(0, 0, 640, 480, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, pixel_data);
        g_main_context_iteration(g_main_context_default(),FALSE);
        glfwSwapBuffers(win);
    }

    return EXIT_SUCCESS;
}
