of-ESMShadowMapping
===================

Exponential Shadow Mapping in openFrameworks

Tested with OF 0073 on OS X 10.8 and Arch Linux. This uses OpenGl 2.1 + GLSL 1.20 in order to
as compatible as possible with the way OF works with rendering.
 
main.cpp has a glut display string set: window.setGlutDisplayString("rgb double depth>=32 alpha");
This is needed to request a larger depth buffer than the default.

The overall scale of this scene might be a little smaller unit-wise than other OF examples. The reason
for this is that depth buffers are limited in resolution, so it's best to set up your worlds with smaller units
rather than larger (ie. a near/far of 0.1 to 100.0) to help avoid depth issues.

Screenshot here:
http://www.flickr.com/photos/85184046@N07/8283730333/in/photostream
