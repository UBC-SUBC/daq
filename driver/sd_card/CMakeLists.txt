target_sources(app PRIVATE sd_card.c) # Add sd_card.c to the Zephyr app target.

target_include_directories(app PRIVATE .) # Include the current directory for sd_card.h.
