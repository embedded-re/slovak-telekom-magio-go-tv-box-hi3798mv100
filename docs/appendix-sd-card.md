# Appendix: the SD card slot (open)

The SD slot is the one part that doesn't work yet.

What works: the controller is found, its clock and pins are set up the way the vendor SDK says, and it
notices when a card goes in or comes out. Driving the controller by hand with `devmem`, the card even
answers every setup command and reaches its "ready to transfer" state.

What doesn't: **no data ever arrives.** Every read times out, in Linux (`error -110 whilst initialising
SD card`) and by hand. Changing the clock timing changed nothing.

My suspicion is the data lines themselves (pad settings or pull-up resistors), or a vendor-specific
controller setting that I haven't found yet. The vendor driver (`himciv200` in the SDK's kernel) is the
place to look next.
