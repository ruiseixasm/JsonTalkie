# S_EthernetENC_Broadcast

A fork of EthernetENC library with **broadcast UDP support** for ENC28J60 Ethernet boards.

**Based on the original [EthernetENC library](https://github.com/jandrassy/EthernetENC) by Norbert Truchsess and Juraj Andrassy.**

## What's Different?
This version enables reception of broadcast UDP packets:
- **Limited broadcasts** (`255.255.255.255`) via hardware filtering
- **Subnet-directed broadcasts** (e.g., `192.168.1.255`) via software filtering
- Maintains all original EthernetENC features and compatibility

## Usage
Replace in your sketch:
```cpp
#include <EthernetENC.h>	// Original
```
With:
```cpp
#include <EthernetENC_Broadcast.h>	// This version
```
UDP sockets will now accept broadcast packets by default.
## Compatibility
Same as original EthernetENC. See [original documentation](https://github.com/Networking-for-Arduino/EthernetENC/wiki) for details.
## Limitations
- Subnet broadcast detection requires software filtering (added in this fork)
- Same limitations as original EthernetENC apply
## License
Same licenses as original EthernetENC. See `LICENSE` file for details.

