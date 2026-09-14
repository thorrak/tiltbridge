Configuration files (config.json, <color>-cal.json) are written here at runtime.

The directory is also created on boot by filesystem_ensure_dir() in main.cpp,
so this file is not what makes it exist any more -- it just means a freshly
flashed filesystem image already has it.
