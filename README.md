# Final-Project-Systems-Programming
A map of connected rooms. Four hunters, each with a single device to take readings of evidence from a room. A ghost, wandering around and leaving evidence for hunters to find. When a hunter is in a room with a ghost, their fear level goes from 0 to 15, and when they reach 15, they’re out of there!

- Made by Will Rakita, for Systems Programming 2401
- Use ./ghost_hunter_sim to run the program after using make to create the required files.
File descriptions:

defs.h
This header file is the central configuration point for the entire simulation. It defines all constants, data structures (like Room, Hunter, and Ghost), and enumerations for evidence and ghost types. All other .c files include this to ensure they share the same definitions for a consistent program state.

helpers.h and helpers.c
These files contain utility functions that support the main simulation logic. They handle tasks like converting enums to strings, providing lists of valid types, generating thread-safe random numbers, and writing all activity to log files, which keeps the core code cleaner and more modular.

house.c
This file manages the lifecycle and structure of the House, which is the main container for the simulation. It initializes the house, its rooms, and the collection of hunters, and is responsible for the coordinated cleanup of all these resources when the program ends.

room.c
This module implements the behavior of individual rooms within the house. It contains functions for managing room connections, adding/removing hunters and the ghost, handling evidence, and facilitating thread-safe movement of entities between rooms using semaphores.

hunter.c
This file defines the behavior of the hunter entities. It controls a hunter's lifecycle, their decision-making for moving and collecting evidence, and their state management (boredom and fear). The core loop for each hunter thread is executed here.

ghost.c
This module controls the ghost entity's behavior. It handles the ghost's initialization, its random actions (idling, leaving evidence, or moving), and tracks its boredom level. The main logic for the ghost thread is contained in this file.

evidence.c
This file provides the logic for managing and manipulating evidence. It includes functions for adding/removing evidence from a bitmask and handling the shared CaseFile where hunters store the evidence they have collected, using a semaphore for thread-safe access.

main.c
This is the entry point of the program. It orchestrates the entire simulation by initializing the house, creating hunters based on user input, launching the ghost and hunter threads, and finally printing the results after all threads have completed along with cleaning up objects created by calling the house cleanup method.

validate_logs.py
This is a support script written in Python. It reads the log files generated during a simulation run and validates that all the recorded events (movements, evidence collection, etc.) are logically consistent and adhere to the rules of the house layout. This was provided as base code for the assigment. 
