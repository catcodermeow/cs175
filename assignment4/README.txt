Emily Sun & PK Byunn-Rieder

List of all files you submitted:

asst4.cpp

arcball.h

rigtform.h

Note the platform you used for development (Windows, OS X, ...):

Mac OS

Provide instructions on how to compile and run your code, especially if you used a nonstandard Makefile, or you are one of those hackers who insists on doing things differently.

Clone repository, open cs175-asst4.xcodeproj file and press play.

Indicate if you met all problem set requirements (more importantly, let us know where your bugs are and what you did to try to eliminate the bugs; we want to give you as much partial credit as we can).

We met all problem set requirements. 

Provide some overview of the code design. Don't go into details; just give us the big picture.

Task 1 - We replaced the Matrix4 with RigTform and also the MVM using rigTFormToMatrix.

Task 2 - Draw sphere using slices and stacks. Arcball rotation implemented by calculating vector based on where mouse clicks and radius of arcball to then get the angle of movement and speed. Used mouse inputs to allow scaling of arcball using screen coordinates and eye coordinates. 

Task 3 - 

Let us know how to run the program; what are the hot keys, mouse button usage, and so on? Describe steps or sequences of steps the TF should take to test and evaluate your code (especially if your implemenentation strays from the assignment specification).

Press 'O' to visualize the arcball frame on the object that is currently being manipulated. Use left click to rotate manipulated object with arcball. 

Press space + left click to scale the manipulated object. 

Press right click to move and translate the object.

Finally, did you implement anything above and beyond the problem set? If so, document it in order for the TFs to test it and evaluate it.

N/A
