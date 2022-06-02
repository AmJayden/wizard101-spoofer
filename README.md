# wizard101-spoofer
Spoof your HWID on Wizard101 and log events and packets by name

# Credits
[@CompiledCode](https://github.com/Compiled-Code/) - Co author of this project

[@xgladius](https://github.com/xgladius/) - Giving us insight and details on any questions we had

# How to use
Using any auto attaching injector, inject `Wizard101 Spoofer.dll` into the game as soon as it starts, or launch it with the embedded login screen and inject before logging in.

# How it works
Using our packet logger, this spoofer intercepts any packets sending your MachineID to the server, and changes the data to a static randomly generated HWID

# How to update???
To update this, simply inject TestCZ, copy the code in the output, replace offsets.hpp with the generated code, and rebuild.
