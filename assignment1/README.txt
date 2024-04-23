Emily Sun: 51518195 
Lauren (PK) Byunn-Rieder: 31373026

List of all files you submitted:

asst1.cpp
asst1-sq-gl3.fshader
asst1-tr-gl3.fshader
asst1-sq-gl3.vshader
asst1-tr-gl3.vshader

Note the platform you used for development (Windows, OS X, ...):

MacOS M1

Provide instructions on how to compile and run your code, especially if you used a nonstandard Makefile, or you are one of those hackers who insists on doing things differently.

Use git clone and the url of the repository to download all the files needed. Open the cs175-asst1.xcodeproj file in Xcode and then press the play button at the top. 

Indicate if you met all problem set requirements (more importantly, let us know where your bugs are and what you did to try to eliminate the bugs; we want to give you as much partial credit as we can).

We met all the problem set requirements.

Provide some overview of the code design. Don't go into details; just give us the big picture.

Our overall approach to this pset was to use as much of the starter code given as possible and modify it based on what we needed wanted the end result to be.

For Task 1, we set up XCode.

For Task 2, we introduced a new variable for the height and width scale of the window in the C++ file, making sure to add it everywhere we needed. Then we used an if, else loop to help us adjust the ratio of the width and height based on window resizing. Then, we modified the vertex shader and multiplied the positions by the scales to get a correct resizing of all the elements.

For Task 3, we duplicated and modified the existing code for the square graphic so that it fit the specifications for the triangle. Instead of 6 vertices for the 2 triangles formimg the square, we only needed 3 for the one triangle. We also used tex2 for the shield.ppm as the texture for the triangle. We removed the SquareShift code from the triangle because it did not need to shift and added color information to the vector to create the RGB blend on the triangle. We also added new key actions for I,J,K and L modelled off of existing key actions.  

Let us know how to run the program; what are the hot keys, mouse button usage, and so on? Describe steps or sequences of steps the TF should take to test and evaluate your code (especially if your implmenentation strays from the assignment specification).

Use the I, J, K, L keys to move the shield-textured triangle up, left, down and right respectively. Use your mouse on the edge of the window screen to resize it. Right click on the smiley face square to move it left and right. Press H for the help menu, Q to exit the program and S to screenshot.

Finally, did you implement anything above and beyond the problem set? If so, document it in order for the TFs to test it and evaluate it.

N/A
