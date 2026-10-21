# Production debugger register editor

This isolated hosted GUI acceptance check exercises the production Release/
Sokol application. It opens the debugger with F12, pauses the running machine,
rejects a 16-bit value for the byte-sized A register, edits A to `5A`, verifies
the displayed AF value, and restores the original AF value with Undo.

The workflow uses Xvfb, xdotool, scrot, and Tesseract OCR. It pins the ROM and
source fixture hashes in the driver and retains proof plus screenshots for one
day so they can be inspected and committed before deleting the Actions run.
