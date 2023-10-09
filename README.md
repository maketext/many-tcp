# many-tcp
C++ TCP Server
- Designed to Windows, using winsock2.h.
- It is basic module intended to FLIR Machine Vision Camera Control. Accepting TCP request from Equipment Hardware.

For production, remove 'break' keyword in following function.
```int keepListening(SOCKET ListenSocket, Router &router) {...}```

