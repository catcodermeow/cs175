Emily Sun & PK Byunn-Rieder

List of all files you submitted:

asst3.cpp

matrix4.h

Note the platform you used for development (Windows, OS X, ...):

Mac OS

Provide instructions on how to compile and run your code, especially if you used a nonstandard Makefile, or you are one of those hackers who insists on doing things differently.

Open asst3.xcodeproj file and press the play button to build.

Indicate if you met all problem set requirements (more importantly, let us know where your bugs are and what you did to try to eliminate the bugs; we want to give you as much partial credit as we can).

Met all problem set requirements. 

Provide some overview of the code design. Don't go into details; just give us the big picture.

Step 1:

We copied the code for the existing cube and shifted its position in the vector as well as changed the RGB values to make it blue.

Step 2:

Used chapter 6 and class notes to get the translational and linear parts of a transformation matrix.

Step 3:

Created a new variable to track which RBT is the active eye RBT and cycled it when 'v' is pressed. When drawing stuff, changed the eye RBT to match the active eye RBT rather than always the default sky. Added console output messages on key press depending on the view mode.

Step 4:

Updated object rotation matrix multiplication to use cube-sky as the auxillary matrix. Updated sky rotation in ego motion mode to perform Y rotations with respect to sky-world and with negated direction, and X rotations with respect to sky-sky. Updated sky rotation in orbit mode to perform Y rotations with respect to world-world (aka the identity matrix) and X rotations with respect to world-sky. Added console output messages on key press depending on the object, view and maniplation mode selected.

Let us know how to run the program; what are the hot keys, mouse button usage, and so on? Describe steps or sequences of steps the TF should take to test and evaluate your code (especially if your implmenentation strays from the assignment specification).

Pressing 'V' Key: Cyles the camera "eye" between Sky, Red Cube, Blue Cube.

Default start is View Mode: Sky. Returns a message in console about which mode you're currently on.
Using mouse to click and drag allows you to change the angle of view.

Pressing 'O' Key: Cycles between objects Sky, Red Cube, Blue Cube to manipulate.

Default start is Manipulated Object: Sky. Returns a message in console about which object is currently selected.
Using mouse to left-click and drag allows you to rotate the object. Using right-click and drag allows you to translate the object across the space.

Pressing 'M' Key: Only works when Sky is selected for both Object and View Mode.

Default start is Manipulation Mode: Ego-Motion. Returns a message in console about which manipulation mode you're currently on and also if you don't have the proper Object or View Mode selected to activate the toggle. Using mouse to left click and drag allows you to rotate the camera roll-free; orbit mode rotates around the center of the scene, and ego mode rotates in place.

Finally, did you implement anything above and beyond the problem set? If so, document it in order for the TFs to test it and evaluate it.

N/A
