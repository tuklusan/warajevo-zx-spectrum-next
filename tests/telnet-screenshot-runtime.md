# Telnet screenshot runtime acceptance

The hosted Release/Sokol production host is exercised through the real Control Port. The nine assertions verify the default OS temporary-directory destination, absolute timestamped path, completed readable 448x312 PNG, unchanged running and paused status, byte-identical paused raster, pause/resume behavior, and a failed write that returns no path and preserves status.

Run `gh workflow run telnet-screenshot-runtime.yml --ref main`.
