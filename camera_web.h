#pragma once
#if defined(TERRA_ESP32S3)
// Starts Wi-Fi, the camera and the live viewer.
//   http://<ip>/          viewer page (Camera / Thermal / Overlay / Side by side)
//   http://<ip>/thermal   JSON: 8x8 grid + direction + sensors (polled by the page)
//   http://<ip>:81/stream MJPEG camera stream
void webBegin();
#endif
