# volutils
Volume Processing Utilities

This package contains the following set of tools:
### 1. createRGBA
Converts an RGB volume (3 channel, 8-bit/channel) into an RGBA volume (4 channel, 8-bit/channel). The alpha values are computed based on occlusion spectrum which gives a low alpha value to outermost (most visible) objects in a volume and vice versa. The input and output volume formats could be anything that ITK supports.

*Dependencies*: ITK
